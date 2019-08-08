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
/*	rfs_init.c	CMU	01/20/82	*/

/*
 *  Remote file system - initialization module
 *
 **********************************************************************
 * HISTORY
 * 02-Jan-86  Mike Accetta (mja) at Carnegie-Mellon University
 *	CS_RFS:  Split initialization into two parts since queue
 *	headers must now be valid before the possible multi-processor
 *	newproc() set up but the root file system isn't available until
 *	after.
 *	[V1(1)]
 *
 * 19-Oct-85  Mike Accetta (mja) at Carnegie-Mellon University
 *	Enabled access(), chdir(), chmod(), chroot() and unlink() system
 *	calls.
 *	[V1(1)]
 *
 **********************************************************************
 */

#include "cs_ichk.h"
#include "cs_rfs.h"

#if	CS_RFS
#include "../h/param.h"
#include "../h/systm.h"
#include "../h/buf.h"
#include "../h/mbuf.h"
#include "../h/ioctl.h"
#include "../h/tty.h"
#include "../h/dir.h"
#include "../h/file.h"
#include "../h/inode.h"
#include "../h/user.h"
#include "../h/map.h"
#include "../h/proc.h"
#include "../h/rfs.h"
#include "../h/namei.h"



/*
 *  Initialization for common global data structures.
 */


#ifdef	RFSDEBUG
/*
 *  Non-zero to enable debug tracing messages
 */
int rfsDebug = RFSDEBUG;
#endif	RFSDEBUG


/*
 *  Parallel process table
 *
 *  Space for this table is allocated dynamically during system startup to be
 *  as large as the standard process table.
 */
struct rfsProcessEntry *rfsProcessTable = 0;

/*
 *  Dynamic structure allocation descriptors.
 */
struct rfsFreeDescriptor rfsConnectionDescriptor = rfsInitDescriptor (rfsConnectionBlock);
struct rfsFreeDescriptor rfsCacheDescriptor      = rfsInitDescriptor (rfsCacheEntry);

/*
 *  Panic message strings invoked in macro definitions
 */
char *rfsPanicMsg_Incr = "rfsIncr";
char *rfsPanicMsg_Decr = "rfsDecr";



/*
 *  Remote system call table.
 *
 *  This table provides the switch from rfs_namei() into the appropriate remote
 *  file system call processing code.  Only system calls which transfer into the
 *  remote code through namei() will appear below.  Not all calls which can
 *  potentially use namei() will appear though since some operations simply
 *  don't make sense remotely and are not enabled for remote handling (by
 *  the OKREMOTE flag in their call on namei()).
 *
 *  The routines which implement the other descriptor based remote file system
 *  calls are found in the rfsops structure.  For documentation purposes, they
 *  are flagged in the table with the pseudo routine rfs_fops which is still a
 *  panic if called through this table.
 *
 *  The remaining few which need explicit hooks in the standard system calls
 *  themselves are similarly flagged as rfs_hook which again is still a
 *  panic if called here.
 */

#define		     rfs_fops		rfs_oops
#define		     rfs_hook		rfs_oops


/*
 *  Remote file system call implementation routine mappings.
 */

#define		     rfs_access		rfs_syscall
#define		     rfs_chdir		rfs_chdirec
#define		     rfs_chmod		rfs_syscall
#define		     rfs_chown		rfs_nyi
#define		     rfs_chroot		rfs_chdirec
extern struct inode *rfs_creat();
#define		     rfs_execv		rfs_exec
#define		     rfs_execve		rfs_exec
#define		     rfs_fchmod		rfs_nyi
#define		     rfs_fchown		rfs_nyi
#define		     rfs_flock		rfs_nyi
#define		     rfs_ftruncate	rfs_nyi
#define		     rfs_fsync		rfs_nyi
#define		     rfs_link		rfs_nyi
extern struct inode *rfs_lseek();
#define		     rfs_lstat		rfs_nyi
#define		     rfs_mkdir		rfs_nyi
#define		     rfs_mknod		rfs_nyi
extern struct inode *rfs_open();
extern struct inode *rfs_ofstat();
extern struct inode *rfs_ostat();
#define		     rfs_outime		rfs_syscall
#define		     rfs_readlink	rfs_nyi
#define		     rfs_rename		rfs_nyi
#define		     rfs_rmdir		rfs_nyi
extern struct inode *rfs_stat();
#define		     rfs_symlink	rfs_nyi
#define		     rfs_truncate	rfs_nyi
#define		     rfs_unlink		rfs_syscall
#define		     rfs_utimes		rfs_nyi

extern struct inode *rfs_syscall();	/* common system call handler */
extern struct inode *rfs_chdirec();	/* common chdir/chroot handler*/
extern struct inode *rfs_exec();	/* common execv/execve handler*/

extern struct inode *rfs_oops();	/* can't happen */
extern struct inode *rfs_nyi();		/* not yet implemented */


/*
 *  The dispatch table proper.
 */

struct inode *(*rfs_sysent[])() =
{
	rfs_oops,			/*   0 = indir */
	rfs_oops,			/*   1 = exit */
	rfs_hook,			/*   2 = fork */
	rfs_fops,			/*   3 = read */
	rfs_fops,			/*   4 = write */
	rfs_open,			/*   5 = open */
	rfs_fops,			/*   6 = close */
	rfs_oops,			/*   7 = old wait */
	rfs_creat,			/*   8 = creat */
	rfs_link,			/*   9 = link */
	rfs_unlink,			/*  10 = unlink */
	rfs_execv,			/*  11 = execv */
	rfs_chdir,			/*  12 = chdir */
	rfs_oops,			/*  13 = old time */
	rfs_mknod,			/*  14 = mknod */
	rfs_chmod,			/*  15 = chmod */
	rfs_chown,			/*  16 = chown; now 3 args */
	rfs_oops,			/*  17 = old break */
	rfs_ostat,			/*  18 = old stat */
	rfs_lseek,			/*  19 = lseek */
	rfs_oops,			/*  20 = getpid */
	rfs_oops,			/*  21 = mount */
	rfs_oops,			/*  22 = umount */
	rfs_oops,			/*  23 = old setuid */
	rfs_oops,			/*  24 = getuid */
	rfs_oops,			/*  25 = old stime */
	rfs_oops,			/*  26 = ptrace */
	rfs_oops,			/*  27 = old alarm */
	rfs_ofstat,			/*  28 = old fstat */
	rfs_oops,			/*  29 = opause */
	rfs_outime,			/*  30 = old utime */
	rfs_oops,			/*  31 = was stty */
	rfs_oops,			/*  32 = was gtty */
	rfs_access,			/*  33 = access */
	rfs_oops,			/*  34 = old nice */
	rfs_oops,			/*  35 = old ftime */
	rfs_oops,			/*  36 = sync */
	rfs_oops,			/*  37 = kill */
	rfs_stat,			/*  38 = stat */
	rfs_oops,			/*  39 = old setpgrp */
	rfs_lstat,			/*  40 = lstat */
	rfs_oops,			/*  41 = dup */
	rfs_oops,			/*  42 = pipe */
	rfs_oops,			/*  43 = old times */
	rfs_oops,			/*  44 = profil */
	rfs_oops,			/*  45 = nosys */
	rfs_oops,			/*  46 = old setgid */
	rfs_oops,			/*  47 = getgid */
	rfs_oops,			/*  48 = old sig */
	rfs_oops,			/*  49 = reserved for USG */
	rfs_oops,			/*  50 = reserved for USG */
	rfs_oops,			/*  51 = turn acct off/on */
	rfs_oops,			/*  52 = old set phys addr */
	rfs_oops,			/*  53 = old lock in core */
	rfs_fops,			/*  54 = ioctl */
	rfs_oops,			/*  55 = reboot */
	rfs_oops,			/*  56 = old mpxchan */
	rfs_symlink,			/*  57 = symlink */
	rfs_readlink,			/*  58 = readlink */
	rfs_execve,			/*  59 = execve */
	rfs_hook,			/*  60 = umask */
	rfs_chroot,			/*  61 = chroot */
	rfs_hook,			/*  62 = fstat */
	rfs_oops,			/*  63 = used internally */
	rfs_oops,			/*  64 = getpagesize */
	rfs_oops,			/*  65 = mremap */
	rfs_oops,			/*  66 = vfork */
	rfs_oops,			/*  67 = old vread */
	rfs_oops,			/*  68 = old vwrite */
	rfs_oops,			/*  69 = sbrk */
	rfs_oops,			/*  70 = sstk */
	rfs_oops,			/*  71 = mmap */
	rfs_oops,			/*  72 = old vadvise */
	rfs_oops,			/*  73 = munmap */
	rfs_oops,			/*  74 = mprotect */
	rfs_oops,			/*  75 = madvise */
	rfs_oops,			/*  76 = vhangup */
	rfs_oops,			/*  77 = old vlimit */
	rfs_oops,			/*  78 = mincore */
	rfs_oops,			/*  79 = getgroups */
	rfs_oops,			/*  80 = setgroups */
	rfs_oops,			/*  81 = getpgrp */
	rfs_oops,			/*  82 = setpgrp */
	rfs_oops,			/*  83 = setitimer */
	rfs_oops,			/*  84 = wait */
	rfs_oops,			/*  85 = swapon */
	rfs_oops,			/*  86 = getitimer */
	rfs_oops,			/*  87 = gethostname */
	rfs_oops,			/*  88 = sethostname */
	rfs_oops,			/*  89 = getdtablesize */
	rfs_oops,			/*  90 = dup2 */
	rfs_oops,			/*  91 = getdopt */
	rfs_oops,			/*  92 = fcntl */
	rfs_fops,			/*  93 = select */
	rfs_oops,			/*  94 = setdopt */
	rfs_fsync,			/*  95 = fsync */
	rfs_oops,			/*  96 = setpriority */
	rfs_oops,			/*  97 = socket */
	rfs_oops,			/*  98 = connect */
	rfs_oops,			/*  99 = accept */
	rfs_oops,			/* 100 = getpriority */
	rfs_oops,			/* 101 = send */
	rfs_oops,			/* 102 = recv */
	rfs_oops,			/* 103 = old socketaddr */
	rfs_oops,			/* 104 = bind */
	rfs_oops,			/* 105 = setsockopt */
	rfs_oops,			/* 106 = listen */
	rfs_oops,			/* 107 = old vtimes */
	rfs_oops,			/* 108 = sigvec */
	rfs_oops,			/* 109 = sigblock */
	rfs_oops,			/* 110 = sigsetmask */
	rfs_oops,			/* 111 = sigpause */
	rfs_oops,			/* 112 = sigstack */
	rfs_oops,			/* 113 = recvmsg */
	rfs_oops,			/* 114 = sendmsg */
	rfs_oops,			/* 115 = vtrace */
	rfs_oops,			/* 116 = gettimeofday */
	rfs_oops,			/* 117 = getrusage */
	rfs_oops,			/* 118 = getsockopt */
	rfs_oops,			/* 119 = nosys */
	rfs_oops,			/* 120 = readv */
	rfs_oops,			/* 121 = writev */
	rfs_oops,			/* 122 = settimeofday */
	rfs_fchown,			/* 123 = fchown */
	rfs_fchmod,			/* 124 = fchmod */
	rfs_oops,			/* 125 = recvfrom */
	rfs_oops,			/* 126 = setreuid */
	rfs_oops,			/* 127 = setregid */
	rfs_rename,			/* 128 = rename */
	rfs_truncate,			/* 129 = truncate */
	rfs_ftruncate,			/* 130 = ftruncate */
	rfs_flock,			/* 131 = flock */
	rfs_oops,			/* 132 = nosys */
	rfs_oops,			/* 133 = sendto */
	rfs_oops,			/* 134 = shutdown */
	rfs_oops,			/* 135 = socketpair */
	rfs_mkdir,			/* 136 = mkdir */
	rfs_rmdir,			/* 137 = rmdir */
	rfs_utimes,			/* 138 = utimes */
	rfs_oops,			/* 139 = used internally */
	rfs_oops,			/* 140 = nosys */
	rfs_oops,			/* 141 = getpeername */
	rfs_oops,			/* 142 = gethostid */
	rfs_oops,			/* 143 = sethostid */
	rfs_oops,			/* 144 = getrlimit */
	rfs_oops,			/* 145 = setrlimit */
	rfs_oops,			/* 146 = killpg */
	rfs_oops,			/* 147 = nosys */
	rfs_oops,			/* 148 = quota */
	rfs_oops,			/* 149 = qquota */
	rfs_oops,			/* 150 = getsockname */
};



/*
 *  rfs_oops - catch an unexpected remote file system call dispatch
 *
 *  Indicate the system call number which invoked the error and panic().
 */

struct inode *
rfs_oops()
{
    printf("code=%d ", u.u_rfscode);
    panic("rfs");
    /* NOTREACHED */
}



/*
 *  rfs_init - data structure initialization
 *
 *  Initialize the parallel process table remote connection block queue
 *  headers.
 */

rfs_init()
{
    register struct rfsProcessEntry *rpep;

    for (rpep=rfsProcessTable; rpep < &rfsProcessTable[nproc]; rpep++)
	initQueue(&rpep->rpe_rcbq);
}



/*
 *  rfs_initroot - local root directory initialization
 *
 *  Look for a local root directory (which will have all the remote file system
 *  link inodes installed above it).  If an appropriate local root directory is
 *  found, change the current and root directory pointers for this process to
 *  use it so that all future references above the root (e.g. /../<system-
 *  name/*) will be able to find the remote links.  If no appropriate directory
 *  is found, leave the current and root directory pointers at the physical
 *  root of the file system.
 *
 *  This routine is called very earlier in the system initialization
 *  procedure by process 0.
 *
 *  N.B.  It is usually prudent to always have symbolic link files for at least
 *  "/etc", "/bin" and "/dev" (and all other top level directories if possible)
 *  installed on the physical root.  This aids in switching between operating
 *  systems with and without support for the remote file system by permitting
 *  single-user bootstraps of either version of an operating system.  These
 *  symbolic links should also be relative to the physical root directory
 *  itself so that such a reorganized file system may be mounted when it is not
 *  the root without causing any nasty surprises.
 */

char *rfs_localroot[] = {"/RFS/.LOCALROOT", "/REM/.LOCALROOT", 0};

rfs_initroot()
{
    char **dirpp;

    for (dirpp=rfs_localroot; *dirpp; dirpp++)
    {
	register struct inode *ip;
	register struct nameidata *ndp = &u.u_nd;

	u.u_error = 0;
	ndp->ni_nameiop = LOOKUP;
	ndp->ni_segflg = UIO_SYSSPACE;
	ndp->ni_dirp = *dirpp;
	ip = namei(ndp);
	if (ip != NULL)
	{
	    if ((ip->i_mode&IFMT) == IFDIR)
	    {
		ilock(u.u_cdir);
		iput(u.u_cdir);
		u.u_cdir = ip;
#if	CS_ICHK
		iincr_chk(ip);
#else	CS_ICHK
		ip->i_count++;
#endif	CS_ICHK
		u.u_rdir = ip;
		iunlock(ip);
		return;
	    }
	    else
	    {
		printf("WARNING: %s is not a directory\n", *dirpp);
		iput(ip);
	    }
	}
    }
    printf("WARNING: no remote file system (missing %s)\n", rfs_localroot[0]);
}
#endif	CS_RFS
