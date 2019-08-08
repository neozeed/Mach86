/*
 ****************************************************************
 * Mach Operating System
 * Copyright (c) 1986 Carnegie-Mellon University
 *  
 * This software was developed by the Mach operating system
 * project at Carnegie-Mellon University's Department of Computer
 * Science. Software contributors as of May 1986 include Mike Accetta, 
 * Robert Baron, William Bolosky, Jonathan Chew, David Golub, 
 * Glenn Marcy, Richard Rashid, Avie Tevanian and Michael Young. 
 * 
 * Some software in these files are derived from sources other
 * than CMU.  Previous copyright and other source notices are
 * preserved below and permission to use such software is
 * dependent on licenses from those institutions.
 * 
 * Permission to use the CMU portion of this software for 
 * any non-commercial research and development purpose is
 * granted with the understanding that appropriate credit
 * will be given to CMU, the Mach project and its authors.
 * The Mach project would appreciate being notified of any
 * modifications and of redistribution of this software so that
 * bug fixes and enhancements may be distributed to users.
 *
 * All other rights are reserved to Carnegie-Mellon University.
 ****************************************************************
 */
/*
 * Copyright (c) 1982 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)init_sysent.c	6.7 (Berkeley) 6/8/85
 */
#if	CMU
/*
 * System call switch table.
 **********************************************************************
 * HISTORY
 * 25-Jan-86  Avadis Tevanian (avie) at Carnegie-Mellon University
 *	Upgraded to 4.3.
 *
 *  9-Aug-85	David L. Black at CMU.  Added new getutime syscall.
 *
 * 28-Jun-85  Mike Accetta (mja) at Carnegie-Mellon University
 *	CS_COMPAT:  changed definition of compat() macro to take the
 *	exact name of the routine as its second parameter rather than
 *	automatically generating one with a "o" prefix using the
 *	comment hack of the C-preprocessor.  When the old approach is
 *	used, lint blows up on this file since it must preserve
 *	comments from the C-preprocessor.  To preserve my sanity,
 *	the name changes in the actual parameters to the compat()
 *	macro were done in place without conditional compilation (they
 *	generate exactly the same code).
 *	[V1(1)]
 *
 * 22-Oct-84  Robert V Baron (rvb) at Carnegie-Mellon University
 *	removed Avie's change for system call #151
 * 	and use the syscall handler in mp/syscall_sw.c instead
 *
 *  1-Apr-84  Avadis Tevanian (avie) at Carnegie-Mellon University
 *	Added IPCAtrium system call (#152) and cmusys system call (153).
 *
 * 31-Mar-84  Avadis Tevanian (avie) at Carnegie-Mellon University
 *	Added mpsys system call (#151).
 *
 **********************************************************************
 */

#include "mach_mp.h"
#include "mach_acc.h"
#include "mach_time.h"
#include "cs_compat.h"
#include "cs_ipc.h"
#include "cs_syscall.h"
#endif	CMU

#include "param.h"
#include "systm.h"

#if	CS_SYSCALL || MACH_MP
	/* serial or parallel system call */
#if	MACH_MP
#define syss(fn,no) {no, 0, fn}
#define sysp(fn,no) {no, 1, fn}
#else	MACH_MP
#define syss(fn,no) {no, fn}
#define sysp(fn,no) {no, fn}
#endif	MACH_MP
#endif	CS_SYSCALL || MACH_MP

int	nosys();

/* 1.1 processes and protection */
int	sethostid(),gethostid(),sethostname(),gethostname(),getpid();
int	fork(),rexit(),execv(),execve(),wait();
#ifdef 	romp
int	exect(); /* Temporary hack to support RT adb. */
#endif	romp
int	getuid(),setreuid(),getgid(),getgroups(),setregid(),setgroups();
int	getpgrp(),setpgrp();

/* 1.2 memory management */
int	sbrk(),sstk();
int	getpagesize(),smmap(),mremap(),munmap(),mprotect(),madvise(),mincore();

/* 1.3 signals */
int	sigvec(),sigblock(),sigsetmask(),sigpause(),sigstack(),sigreturn();
int	kill(), killpg();

/* 1.4 timing and statistics */
int	gettimeofday(),settimeofday();
int	getitimer(),setitimer();
int 	adjtime();

/* 1.5 descriptors */
int	getdtablesize(),dup(),dup2(),close();
int	select(),getdopt(),setdopt(),fcntl(),flock();

/* 1.6 resource controls */
int	getpriority(),setpriority(),getrusage(),getrlimit(),setrlimit();
int	setquota(),qquota();

/* 1.7 system operation support */
int	umount(),smount(),swapon();
int	sync(),reboot(),sysacct();

/* 2.1 generic operations */
int	read(),write(),readv(),writev(),ioctl();

/* 2.2 file system */
int	chdir(),chroot();
int	mkdir(),rmdir();
int	creat(),open(),mknod(),unlink(),stat(),fstat(),lstat();
int	chown(),fchown(),chmod(),fchmod(),utimes();
int	link(),symlink(),readlink(),rename();
int	lseek(),truncate(),ftruncate(),saccess(),fsync();

/* 2.3 communications */
int	socket(),bind(),listen(),accept(),connect();
int	socketpair(),sendto(),send(),recvfrom(),recv();
int	sendmsg(),recvmsg(),shutdown(),setsockopt(),getsockopt();
int	getsockname(),getpeername(),pipe();

int	umask();		/* XXX */

/* 2.4 processes */
int	ptrace();

/* 2.5 terminals */

#if	MACH_ACC
int	InitPort(), Send(), Receive(), LockPorts(), MessagesWaiting();
int	SoftEnable(), Cheat(), Nm(), KPortToPID();
#endif	MACH_ACC

#if	MACH_TIME
int	getutime();
#endif	MACH_TIME

#ifdef COMPAT
/* emulations for backwards compatibility */
#if	CS_COMPAT || MACH_MP
#if	MACH_MP
#define	compat(name,n)	{n, 0, name}
#define	compatp(name,n)	{n, 1, name}
#else	MACH_MP
#define	compat(name,n)	{n, name}
#define	compatp(name,n)	{n, name}
#endif	MACH_MP
#endif	CS_COMPAT || MACH_MP

#if	CS_COMPAT
extern int mpxchan();
extern int stty();
extern int gtty();

#include "acct.h"

/*
 *  System call mode bit macros
 *
 *  M0 - system call is valid under both 4.1 and 4.2
 *  M1 - system call is valid only under 4.1
 *  M2 - system call is valid only under 4.2
 */

#define	M0(code)	(0)
#define	M1(code)	(A41MODE)
#define	M2(code)	(A42MODE)

char sysmode[] =
{
    M0(  0),M0(  1),M0(  2),M0(  3),M0(  4),M0(  5),M0(  6),M1(  7),
    M0(  8),M0(  9),M0( 10),M0( 11),M0( 12),M1( 13),M0( 14),M0( 15),
    M0( 16),M0( 17),M1( 18),M0( 19),M0( 20),M0( 21),M0( 22),M1( 23),
    M0( 24),M1( 25),M0( 26),M1( 27),M1( 28),M1( 29),M1( 30),M1( 31),
    M1( 32),M0( 33),M1( 34),M1( 35),M0( 36),M0( 37),M2( 38),M1( 39),
    M2( 40),M0( 41),M0( 42),M1( 43),M0( 44),M0( 45),M1( 46),M0( 47),
    M1( 48),M0( 49),M0( 50),M0( 51),M1( 52),M1( 53),M0( 54),M0( 55),
    M1( 56),M2( 57),M2( 58),M0( 59),M0( 60),M0( 61),M2( 62),M0( 63),
    M2( 64),M2( 65),M0( 66),M1( 67),M1( 68),M2( 69),M2( 70),M2( 71),
    M0( 72),M2( 73),M2( 74),M2( 75),M0( 76),M1( 77),M2( 78),M2( 79),
    M2( 80),M2( 81),M2( 82),M2( 83),M2( 84),M0( 85),M2( 86),M2( 87),
    M2( 88),M2( 89),M2( 90),M2( 91),M2( 92),M2( 93),M2( 94),M2( 95),
    M2( 96),M2( 97),M2( 98),M2( 99),M2(100),M2(101),M2(102),M2(103),
    M2(104),M2(105),M2(106),M1(107),M2(108),M2(109),M2(110),M2(111),
    M2(112),M2(113),M2(114),M0(115),M2(116),M2(117),M2(118),M0(119),
    M2(120),M2(121),M2(122),M2(123),M2(124),M2(125),M2(126),M2(127),
    M2(128),M2(129),M2(130),M2(131),M2(132),M2(133),M2(134),M2(135),
    M2(136),M2(137),M2(138),M2(139),M2(140),M2(141),M2(142),M2(143),
    M2(144),M2(145),M2(146),M2(147),M2(148),M2(149),M2(150),M2(151),
};
#else	CS_COMPAT
#if	MACH_MP
#else	MACH_MP
#define	compat(n, name)	n, o/**/name
#endif	MACH_MP
#endif	CS_COMPAT

int	owait();		/* now receive message on channel */
int	otime();		/* now use gettimeofday */
int	ostime();		/* now use settimeofday */
int	oalarm();		/* now use setitimer */
int	outime();		/* now use utimes */
int	opause();		/* now use sigpause */
int	onice();		/* now use setpriority,getpriority */
int	oftime();		/* now use gettimeofday */
int	osetpgrp();		/* ??? */
int	otimes();		/* now use getrusage */
int	ossig();		/* now use sigvec, etc */
int	ovlimit();		/* now use setrlimit,getrlimit */
int	ovtimes();		/* now use getrusage */
int	osetuid();		/* now use setreuid */
int	osetgid();		/* now use setregid */
int	ostat();		/* now use stat */
int	ofstat();		/* now use fstat */
#else
#if	MACH_MP
#define	compat(n, name)		{0, 0, nosys}
#define	compatp(n, name)	{0, 1, nosys}
#else	MACH_MP
#define	compat(n, name)	0, nosys
#endif	MACH_MP
#endif

/* BEGIN JUNK */
#ifdef vax
int	resuba();
#endif
#ifdef TRACE
int	vtrace();
#endif
int	profil();		/* 'cuz sys calls are interruptible */
int	vhangup();		/* should just do in exit() */
int	vfork();		/* awaiting fork w/ copy on write */
int	obreak();		/* awaiting new sbrk */
int	ovadvise();		/* awaiting new madvise */
/* END JUNK */

#if	CS_SYSCALL
#if	CS_IPC
extern int IPCAtrium();
#else	CS_IPC
#define	IPCAtrium	nosys
#endif	CS_IPC
extern int setmodes();
extern int getmodes();
extern int setaid();
extern int getaid();
extern int table();
extern int nulldev();			/* to test 4.1/4.2 kernel */

struct sysent cmusysent[] =
{
	syss(getmodes, 0),		/* -9 = get process modes */
	syss(setmodes, 1),		/* -8 = set process modes */
	syss(IPCAtrium, 2),		/* -7 = old CMU IPC */
	syss(table, 5),			/* -6 = table lookup */
	syss(nosys, 0),			/* -5 = old xstat */
	syss(nosys, 0),			/* -4 = old xfstat */
	syss(nulldev, 0),		/* -3 = old chacct */
	syss(getaid, 0),		/* -2 = get account ID */
	syss(setaid, 1),		/* -1 = set account ID */
};
/*
 *  The preceding table is effectively a negative extension of the following
 *  table.  No other definitions should intervene here.  The system call
 *  handler assumes that these two tables are contiguous.
 */
#endif	CS_SYSCALL
#if	CS_SYSCALL || MACH_MP
struct sysent sysent[] = {
	syss(nosys,0),			/*   0 = indir */
	syss(rexit,1),			/*   1 = exit */
	syss(fork,0),			/*   2 = fork */
	syss(read,3),			/*   3 = read */
	syss(write,3),			/*   4 = write */
	syss(open,3),			/*   5 = open */
	syss(close,1),			/*   6 = close */
		compat(owait,0),	/*   7 = old wait */
	syss(creat,2),			/*   8 = creat */
	syss(link,2),			/*   9 = link */
	syss(unlink,1),			/*  10 = unlink */
	syss(execv,2),			/*  11 = execv */
	syss(chdir,1),			/*  12 = chdir */
		compat(otime,0),	/*  13 = old time */
	syss(mknod,3),			/*  14 = mknod */
	syss(chmod,2),			/*  15 = chmod */
	syss(chown,3),			/*  16 = chown; now 3 args */
	syss(obreak,1),			/*  17 = old break */
		compat(ostat,2),	/*  18 = old stat */
	syss(lseek,3),			/*  19 = lseek */
	sysp(getpid,0),			/*  20 = getpid */
	syss(smount,3),			/*  21 = mount */
	syss(umount,1),			/*  22 = umount */
		compat(osetuid,1),	/*  23 = old setuid */
	sysp(getuid,0),			/*  24 = getuid */
		compat(ostime,1),	/*  25 = old stime */
	syss(ptrace,4),			/*  26 = ptrace */
		compat(oalarm,1),	/*  27 = old alarm */
		compat(ofstat,2),	/*  28 = old fstat */
		compat(opause,0),	/*  29 = opause */
		compat(outime,2),	/*  30 = old utime */
#if	CS_COMPAT
		compat(stty,2),		/*  31 = was stty */
		compat(gtty,2),		/*  32 = was gtty */
#else	CS_COMPAT
	syss(nosys,0),			/*  31 = was stty */
	syss(nosys,0),			/*  32 = was gtty */
#endif	CS_COMPAT
	syss(saccess,2),		/*  33 = access */
		compat(onice,1),	/*  34 = old nice */
		compat(oftime,1),	/*  35 = old ftime */
	syss(sync,0),			/*  36 = sync */
	syss(kill,2),			/*  37 = kill */
	syss(stat,2),			/*  38 = stat */
		compat(osetpgrp,2),	/*  39 = old setpgrp */
	syss(lstat,2),			/*  40 = lstat */
	syss(dup,2),			/*  41 = dup */
	syss(pipe,0),			/*  42 = pipe */
		compat(otimes,1),	/*  43 = old times */
	syss(profil,4),			/*  44 = profil */
	syss(nosys,0),			/*  45 = nosys */
		compatp(osetgid,1),	/*  46 = old setgid */
	sysp(getgid,0),			/*  47 = getgid */
		compat(ossig,2),	/*  48 = old sig */
	syss(nosys,0),			/*  49 = reserved for USG */
	syss(nosys,0),			/*  50 = reserved for USG */
	syss(sysacct,1),		/*  51 = turn acct off/on */
	syss(nosys,0),			/*  52 = old set phys addr */
	syss(nosys,0),			/*  53 = old lock in core */
	syss(ioctl,3),			/*  54 = ioctl */
	syss(reboot,1),			/*  55 = reboot */
#if	CS_COMPAT
		compat(mpxchan,4),	/*  56 = old mpxchan */
#else	CS_COMPAT
	syss(nosys,0),			/*  56 = old mpxchan */
#endif	CS_COMPAT
	syss(symlink,2),		/*  57 = symlink */
	syss(readlink,3),		/*  58 = readlink */
	syss(execve,3),			/*  59 = execve */
	syss(umask,1),			/*  60 = umask */
	syss(chroot,1),			/*  61 = chroot */
	syss(fstat,2),			/*  62 = fstat */
	syss(nosys,0),			/*  63 = used internally */
	sysp(getpagesize,1),		/*  64 = getpagesize */
	syss(mremap,5),			/*  65 = mremap */
	syss(vfork,0),			/*  66 = vfork */
	syss(read,3),			/*  67 = old vread */
	syss(write,3),			/*  68 = old vwrite */
	syss(sbrk,1),			/*  69 = sbrk */
	syss(sstk,1),			/*  70 = sstk */
	syss(smmap,6),			/*  71 = mmap */
	syss(ovadvise,1),		/*  72 = old vadvise */
	syss(munmap,2),			/*  73 = munmap */
	syss(mprotect,3),		/*  74 = mprotect */
	syss(madvise,3),		/*  75 = madvise */
	syss(vhangup,1),		/*  76 = vhangup */
		compat(ovlimit,2),	/*  77 = old vlimit */
	syss(mincore,3),		/*  78 = mincore */
	sysp(getgroups,2),		/*  79 = getgroups */
	sysp(setgroups,2),		/*  80 = setgroups */
	sysp(getpgrp,1),		/*  81 = getpgrp */
	sysp(setpgrp,2),		/*  82 = setpgrp */
	syss(setitimer,3),		/*  83 = setitimer */
	syss(wait,0),			/*  84 = wait */
	syss(swapon,1),			/*  85 = swapon */
	syss(getitimer,2),		/*  86 = getitimer */
	sysp(gethostname,2),		/*  87 = gethostname */
	sysp(sethostname,2),		/*  88 = sethostname */
	sysp(getdtablesize,0),		/*  89 = getdtablesize */
	syss(dup2,2),			/*  90 = dup2 */
	sysp(getdopt,2),		/*  91 = getdopt */
	syss(fcntl,3),			/*  92 = fcntl */
	syss(select,5),			/*  93 = select */
	syss(setdopt,2),		/*  94 = setdopt */
	syss(fsync,1),			/*  95 = fsync */
	sysp(setpriority,3),		/*  96 = setpriority */
	syss(socket,3),			/*  97 = socket */
	syss(connect,3),		/*  98 = connect */
	syss(accept,3),			/*  99 = accept */
	sysp(getpriority,2),		/* 100 = getpriority */
	syss(send,4),			/* 101 = send */
	syss(recv,4),			/* 102 = recv */
	syss(sigreturn,1),		/* 103 = sigreturn */
	syss(bind,3),			/* 104 = bind */
	syss(setsockopt,5),		/* 105 = setsockopt */
	syss(listen,2),			/* 106 = listen */
		compat(ovtimes,2),	/* 107 = old vtimes */
	syss(sigvec,3),			/* 108 = sigvec */
	syss(sigblock,1),		/* 109 = sigblock */
	syss(sigsetmask,1),		/* 110 = sigsetmask */
	syss(sigpause,1),		/* 111 = sigpause */
	syss(sigstack,2),		/* 112 = sigstack */
	syss(recvmsg,3),		/* 113 = recvmsg */
	syss(sendmsg,3),		/* 114 = sendmsg */
#ifdef TRACE
	syss(vtrace,2),			/* 115 = vtrace */
#else
	syss(nosys,0),			/* 115 = nosys */
#endif
	sysp(gettimeofday,2),		/* 116 = gettimeofday */
	sysp(getrusage,2),		/* 117 = getrusage */
	syss(getsockopt,5),		/* 118 = getsockopt */
#ifdef vax
	syss(resuba,1),			/* 119 = resuba */
#else
	syss(nosys,0),			/* 119 = nosys */
#endif
	syss(readv,3),			/* 120 = readv */
	syss(writev,3),			/* 121 = writev */
	sysp(settimeofday,2),		/* 122 = settimeofday */
	syss(fchown,3),			/* 123 = fchown */
	syss(fchmod,2),			/* 124 = fchmod */
	syss(recvfrom,6),		/* 125 = recvfrom */
	sysp(setreuid,2),		/* 126 = setreuid */
	sysp(setregid,2),		/* 127 = setregid */
	syss(rename,2),			/* 128 = rename */
	syss(truncate,2),		/* 129 = truncate */
	syss(ftruncate,2),		/* 130 = ftruncate */
	syss(flock,2),			/* 131 = flock */
	syss(nosys,0),			/* 132 = nosys */
	syss(sendto,6),			/* 133 = sendto */
	syss(shutdown,2),		/* 134 = shutdown */
	syss(socketpair,5),		/* 135 = socketpair */
	syss(mkdir,2),			/* 136 = mkdir */
	syss(rmdir,1),			/* 137 = rmdir */
	syss(utimes,2),			/* 138 = utimes */
	syss(nosys,0),			/* 139 = used internally */
	syss(adjtime,2),		/* 140 = adjtime */
	syss(getpeername,3),		/* 141 = getpeername */
	sysp(gethostid,2),		/* 142 = gethostid */
	sysp(sethostid,2),		/* 143 = sethostid */
	sysp(getrlimit,2),		/* 144 = getrlimit */
	sysp(setrlimit,2),		/* 145 = setrlimit */
	syss(killpg,2),			/* 146 = killpg */
	syss(nosys,0),			/* 147 = nosys */
	syss(setquota,2),		/* 148 = quota */
	syss(qquota,4),			/* 149 = qquota */
	syss(getsockname,3),		/* 150 = getsockname */
};
#else	CS_SYSCALL || MACH_MP

struct sysent sysent[] = {
	0, nosys,			/*   0 = indir */
	1, rexit,			/*   1 = exit */
	0, fork,			/*   2 = fork */
	3, read,			/*   3 = read */
	3, write,			/*   4 = write */
	3, open,			/*   5 = open */
	1, close,			/*   6 = close */
	compat(0,wait),			/*   7 = old wait */
	2, creat,			/*   8 = creat */
	2, link,			/*   9 = link */
	1, unlink,			/*  10 = unlink */
	2, execv,			/*  11 = execv */
	1, chdir,			/*  12 = chdir */
	compat(0,time),			/*  13 = old time */
	3, mknod,			/*  14 = mknod */
	2, chmod,			/*  15 = chmod */
	3, chown,			/*  16 = chown; now 3 args */
	1, obreak,			/*  17 = old break */
	compat(2,stat),			/*  18 = old stat */
	3, lseek,			/*  19 = lseek */
	0, getpid,			/*  20 = getpid */
	3, smount,			/*  21 = mount */
	1, umount,			/*  22 = umount */
	compat(1,setuid),		/*  23 = old setuid */
	0, getuid,			/*  24 = getuid */
	compat(1,stime),		/*  25 = old stime */
	4, ptrace,			/*  26 = ptrace */
	compat(1,alarm),		/*  27 = old alarm */
	compat(2,fstat),		/*  28 = old fstat */
	compat(0,pause),		/*  29 = opause */
	compat(2,utime),		/*  30 = old utime */
	0, nosys,			/*  31 = was stty */
	0, nosys,			/*  32 = was gtty */
	2, saccess,			/*  33 = access */
	compat(1,nice),			/*  34 = old nice */
	compat(1,ftime),		/*  35 = old ftime */
	0, sync,			/*  36 = sync */
	2, kill,			/*  37 = kill */
	2, stat,			/*  38 = stat */
	compat(2,setpgrp),		/*  39 = old setpgrp */
	2, lstat,			/*  40 = lstat */
	2, dup,				/*  41 = dup */
	0, pipe,			/*  42 = pipe */
	compat(1,times),		/*  43 = old times */
	4, profil,			/*  44 = profil */
	0, nosys,			/*  45 = nosys */
	compat(1,setgid),		/*  46 = old setgid */
	0, getgid,			/*  47 = getgid */
	compat(2,ssig),			/*  48 = old sig */
	0, nosys,			/*  49 = reserved for USG */
	0, nosys,			/*  50 = reserved for USG */
	1, sysacct,			/*  51 = turn acct off/on */
	0, nosys,			/*  52 = old set phys addr */
	0, nosys,			/*  53 = old lock in core */
	3, ioctl,			/*  54 = ioctl */
	1, reboot,			/*  55 = reboot */
	0, nosys,			/*  56 = old mpxchan */
	2, symlink,			/*  57 = symlink */
	3, readlink,			/*  58 = readlink */
	3, execve,			/*  59 = execve */
	1, umask,			/*  60 = umask */
	1, chroot,			/*  61 = chroot */
	2, fstat,			/*  62 = fstat */
	0, nosys,			/*  63 = used internally */
	1, getpagesize,			/*  64 = getpagesize */
	5, mremap,			/*  65 = mremap */
	0, vfork,			/*  66 = vfork */
	0, read,			/*  67 = old vread */
	0, write,			/*  68 = old vwrite */
	1, sbrk,			/*  69 = sbrk */
	1, sstk,			/*  70 = sstk */
	6, smmap,			/*  71 = mmap */
	1, ovadvise,			/*  72 = old vadvise */
	2, munmap,			/*  73 = munmap */
	3, mprotect,			/*  74 = mprotect */
	3, madvise,			/*  75 = madvise */
	1, vhangup,			/*  76 = vhangup */
	compat(2,vlimit),		/*  77 = old vlimit */
	3, mincore,			/*  78 = mincore */
	2, getgroups,			/*  79 = getgroups */
	2, setgroups,			/*  80 = setgroups */
	1, getpgrp,			/*  81 = getpgrp */
	2, setpgrp,			/*  82 = setpgrp */
	3, setitimer,			/*  83 = setitimer */
	0, wait,			/*  84 = wait */
	1, swapon,			/*  85 = swapon */
	2, getitimer,			/*  86 = getitimer */
	2, gethostname,			/*  87 = gethostname */
	2, sethostname,			/*  88 = sethostname */
	0, getdtablesize,		/*  89 = getdtablesize */
	2, dup2,			/*  90 = dup2 */
	2, getdopt,			/*  91 = getdopt */
	3, fcntl,			/*  92 = fcntl */
	5, select,			/*  93 = select */
	2, setdopt,			/*  94 = setdopt */
	1, fsync,			/*  95 = fsync */
	3, setpriority,			/*  96 = setpriority */
	3, socket,			/*  97 = socket */
	3, connect,			/*  98 = connect */
	3, accept,			/*  99 = accept */
	2, getpriority,			/* 100 = getpriority */
	4, send,			/* 101 = send */
	4, recv,			/* 102 = recv */
	1, sigreturn,			/* 103 = sigreturn */
	3, bind,			/* 104 = bind */
	5, setsockopt,			/* 105 = setsockopt */
	2, listen,			/* 106 = listen */
	compat(2,vtimes),		/* 107 = old vtimes */
	3, sigvec,			/* 108 = sigvec */
	1, sigblock,			/* 109 = sigblock */
	1, sigsetmask,			/* 110 = sigsetmask */
	1, sigpause,			/* 111 = sigpause */
	2, sigstack,			/* 112 = sigstack */
	3, recvmsg,			/* 113 = recvmsg */
	3, sendmsg,			/* 114 = sendmsg */
#ifdef TRACE
	2, vtrace,			/* 115 = vtrace */
#else
	0, nosys,			/* 115 = nosys */
#endif
	2, gettimeofday,		/* 116 = gettimeofday */
	2, getrusage,			/* 117 = getrusage */
	5, getsockopt,			/* 118 = getsockopt */
#ifdef vax
	1, resuba,			/* 119 = resuba */
#else
	0, nosys,			/* 119 = nosys */
#endif
	3, readv,			/* 120 = readv */
	3, writev,			/* 121 = writev */
	2, settimeofday,		/* 122 = settimeofday */
	3, fchown,			/* 123 = fchown */
	2, fchmod,			/* 124 = fchmod */
	6, recvfrom,			/* 125 = recvfrom */
	2, setreuid,			/* 126 = setreuid */
	2, setregid,			/* 127 = setregid */
	2, rename,			/* 128 = rename */
	2, truncate,			/* 129 = truncate */
	2, ftruncate,			/* 130 = ftruncate */
	2, flock,			/* 131 = flock */
	0, nosys,			/* 132 = nosys */
	6, sendto,			/* 133 = sendto */
	2, shutdown,			/* 134 = shutdown */
	5, socketpair,			/* 135 = socketpair */
	2, mkdir,			/* 136 = mkdir */
	1, rmdir,			/* 137 = rmdir */
	2, utimes,			/* 138 = utimes */
	0, nosys,			/* 139 = used internally */
	2, adjtime,			/* 140 = adjtime */
	3, getpeername,			/* 141 = getpeername */
	2, gethostid,			/* 142 = gethostid */
	2, sethostid,			/* 143 = sethostid */
	2, getrlimit,			/* 144 = getrlimit */
	2, setrlimit,			/* 145 = setrlimit */
	2, killpg,			/* 146 = killpg */
	0, nosys,			/* 147 = nosys */
	2, setquota,			/* 148 = quota */
	4, qquota,			/* 149 = qquota */
	3, getsockname,			/* 150 = getsockname */
#ifdef	romp
	3, exect,			/* 151 = execve w/trace */
#else	romp
	0, nosys,			/* 151 = nosys */
#endif	romp
	0, nosys,			/* 152 = nosys */
	0, nosys,			/* 153 = nosys */
	0, nosys,			/* 154 = nosys */
};
#endif	CS_SYSCALL || MACH_MP
int	nsysent = sizeof (sysent) / sizeof (sysent[0]);
#if	CS_SYSCALL
int	ncmusysent = sizeof (cmusysent) / sizeof (cmusysent[0]);
int	nallsysent = (sizeof (cmusysent)+sizeof(sysent)) / sizeof (sysent[0]);
#endif	CS_SYSCALL
