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
 *	@(#)user.h	6.10 (Berkeley) 6/8/85
 */
#if	CMU
/*
 **********************************************************************
 * HISTORY
 *  3-Jun-86  Bill Bolosky (bolosky) at Carnegie-Mellon University
 *	ROMP_DUALCALL: Added uu_calltype field to simultaneously support
 *	new and old calling sequences on the RT.
 *
 * 27-May-86  Avadis Tevanian (avie) at Carnegie-Mellon University
 *	Increased u-area sizes of signal and sigmask arrays by 1 to
 *	really allow for NSIG signals (signals are numbered starting at
 *	1, C arrays start at 0).
 *
 * 25-Apr-86  Mike Accetta (mja) at Carnegie-Mellon University
 *	CS_GENERIC:  Added UMODE_ALLOWOLD definition.
 *
 * 25-Jan-86  Avadis Tevanian (avie) at Carnegie-Mellon University
 *	Upgraded to 4.3.
 *
 * 29-Jan-86  Mike Accetta (mja) at Carnegie-Mellon University
 *	CS_GENERIC:  Added UMODE_DIRTRACE definition.
 *
 * 18-Oct-85  Mike Accetta (mja) at Carnegie-Mellon University
 *	CS_RFS:  added u_rfs definition.
 *	[V1(1)]
 *
 * 07-Sep-85  Mike Accetta (mja) at Carnegie-Mellon University
 *	CS_GENERIC:  added modes field and symbol definitions.
 *	[V1(1)]
 *
 * 03-Aug-85  Mike Accetta (mja) at Carnegie-Mellon University
 *	CS_RPAUSE:  added resource pause field and symbol definitions.
 *	[V1(1)]
 *
 * 22-Jul-85  Mike Accetta (mja) at Carnegie-Mellon University
 *	CS_RFS:  added u_syscode definition.
 *	[V1(1)]
 *
 * 14-May-85  Mike Accetta (mja) at Carnegie-Mellon University
 *	Upgraded to 4.2BSD.  Carried over changes below.
 *	[V1(1)]
 *
 * 28-Jul-82  Mike Accetta (mja) at Carnegie-Mellon University
 *	Changed to include errno.h from include directory (V3.05c).
 *
 * 22-Jun-82  Mike Accetta (mja) at Carnegie-Mellon University
 *	CS_AID:  added u_aid field to record new process account ID
 *	(V3.05a).
 *
 **********************************************************************
 */
 
#ifdef	KERNEL
#include "cs_aid.h"
#include "cs_bugfix.h"
#include "cs_compat.h"
#include "cs_generic.h"
#include "cs_rpause.h"
#include "cs_rfs.h"
#include "mach_tt.h"
#ifdef	romp
#include "romp_dualcall.h"
#endif	romp
#else	KERNEL
#include <sys/features.h>
#endif	KERNEL
#endif	CMU

#ifdef	romp
#ifdef	KERNEL
#include "../h/types.h"
#else	KERNEL
#include <sys/types.h>
#endif	KERNEL
#endif	romp

#ifdef KERNEL
#include "../machine/pcb.h"
#include "dmap.h"
#include "time.h"
#include "resource.h"
#include "namei.h"
#else
#include <machine/pcb.h>
#include <sys/dmap.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/namei.h>
#endif

/*
 * Per process structure containing data that
 * isn't needed in core when the process is swapped out.
 */
 
#define	MAXCOMLEN	16		/* <= MAXNAMLEN, >= sizeof(ac_comm) */
 
#if	MACH_TT
struct	user {
	struct	pcb uu_pcb;
	struct	proc *uu_procp;		/* pointer to proc structure */
	int	*uu_ar0;			/* address of users saved R0 */
	char	uu_comm[MAXCOMLEN + 1];

/* syscall parameters, results and catches */
	int	uu_arg[8];		/* arguments to current system call */
	int	*uu_ap;			/* pointer to arglist */
	label_t	uu_qsave;		/* for non-local gotos on interrupts */
	union {				/* syscall return values */
		struct	{
			int	R_val1;
			int	R_val2;
		} u_rv;
#define	r_val1	u_rv.R_val1
#define	r_val2	u_rv.R_val2
		off_t	r_off;
		time_t	r_time;
	} uu_r;
	char	uu_error;		/* return error code */
	char	uu_eosys;		/* special action on end of syscall */

/* 1.1 - processes and protection */
#if	ROMP_DUALCALL
	char	uu_calltype;		/* 0 - old calling sequence */
#endif	ROMP_DUALCALL
	uid_t	uu_uid;			/* effective user id */
	uid_t	uu_ruid;			/* real user id */
	gid_t	uu_gid;			/* effective group id */
	gid_t	uu_rgid;			/* real group id */
	gid_t	uu_groups[NGROUPS];	/* groups, 0 terminated */

/* 1.2 - memory management */
	size_t	uu_tsize;		/* text size (clicks) */
	size_t	uu_dsize;		/* data size (clicks) */
	size_t	uu_ssize;		/* stack size (clicks) */
	struct	dmap uu_dmap;		/* disk map for data segment */
	struct	dmap uu_smap;		/* disk map for stack segment */
	struct	dmap uu_cdmap, uu_csmap;	/* shadows of u_dmap, u_smap, for
					   use of parent during fork */
	label_t uu_ssave;		/* label variable for swapping */
	size_t	uu_odsize, uu_ossize;	/* for (clumsy) expansion swaps */
	time_t	uu_outime;		/* user time at last sample */

/* 1.3 - signal management */
	int	(*uu_signal[NSIG+1])();	/* disposition of signals */
	int	uu_sigmask[NSIG+1];	/* signals to be blocked */
	int	uu_sigonstack;		/* signals to take on sigstack */
	int	uu_sigintr;		/* signals that interrupt syscalls */
	int	uu_oldmask;		/* saved mask from before sigpause */
	int	uu_code;			/* ``code'' to trap */
	struct	sigstack uu_sigstack;	/* sp & on stack state variable */

/* 1.4 - descriptor management */
	struct	file *uu_ofile[NOFILE];	/* file structures for open files */
	char	uu_pofile[NOFILE];	/* per-process flags of open files */
	int	uu_lastfile;		/* high-water mark of uu_ofile */
#define	UF_EXCLOSE 	0x1		/* auto-close on exec */
#define	UF_MAPPED 	0x2		/* mapped from device */
	struct	inode *uu_cdir;		/* current directory */
	struct	inode *uu_rdir;		/* root directory of current process */
	struct	tty *uu_ttyp;		/* controlling tty pointer */
	dev_t	uu_ttyd;			/* controlling tty dev */
	short	uu_cmask;		/* mask for file creation */

/* 1.5 - timing and statistics */
	struct	rusage uu_ru;		/* stats for this proc */
	struct	rusage uu_cru;		/* sum of stats for reaped children */
	struct	itimerval uu_timer[3];
	int	uu_XXX[3];
	struct	timeval uu_start;
	short	uu_acflag;

	struct uprof {			/* profile arguments */
		short	*pr_base;	/* buffer base */
		unsigned pr_size;	/* buffer size */
		unsigned pr_off;	/* pc offset */
		unsigned pr_scale;	/* pc scaling */
	} uu_prof;
#if	CS_AID
	short	uu_aid;			/* account id */
#endif	CS_AID
#if	CS_RPAUSE
	struct fs *uu_rpsfs;		/* resource pause file system */
	char	uu_rpswhich;		/* resource pause operation selection */
	u_char	uu_rpause;		/* resource pause flags: */
#define	URPS_AGAIN	01		/* - no child processes available */
#define	URPS_NOMEM	02		/* - no memory available */
#define	URPS_NFILE	04		/* - file table overflow */
#define	URPS_NOSPC	010		/* - no space on device */
#endif	CS_RPAUSE
#if	CS_GENERIC
	char	uu_modes;		/* process modes: */
#define	UMODE_P_GID	01		/* - 4.2 parent GID on inode create */
#define	UMODE_NOFOLLOW	02		/* - don't follow symbolic links */
#define	UMODE_ALLOWOLD	010		/* - TEMP: allow 4.1 feature use */
#define	UMODE_DIRTRACE	020		/* - TEMP: trace old directory reads */
#define	UMODE_NEWDIR	040		/* - TEMP: mkdir() new format */
#define	UMODE_SETGROUPS	0100		/* - TEMP: setgroups() done */
#define	UMODE_COMPATRACE 0200		/* - TEMP: trace 4.1/4.2 mode */
#endif	CS_GENERIC
#if	CS_RFS
	char	uu_rfs;			/* remote syscall state bits: */
#define	URFS_CDIR	01		/* - current diretory is remote */
#define	URFS_RDIR	02		/* - root diretory is remote */
	short	uu_rfscode;		/* remote file system call number */
	short	uu_rfsncnt;		/* remote file system namei() call count */
#endif	CS_RFS

/* 1.6 - resource controls */
	struct	rlimit uu_rlimit[RLIM_NLIMITS];
	struct	quota *uu_quota;		/* user's quota structure */
	int	uu_qflags;		/* per process quota flags */

/* namei & co. */
	struct nameicache {		/* last successful directory search */
		int nc_prevoffset;	/* offset at which last entry found */
		ino_t nc_inumber;	/* inum of cached directory */
		dev_t nc_dev;		/* dev of cached directory */
		time_t nc_time;		/* time stamp for cache entry */
	} uu_ncache;
	struct	nameidata uu_nd;

	int	uu_stack[1];
};
#else	MACH_TT
struct	user {
	struct	pcb u_pcb;
	struct	proc *u_procp;		/* pointer to proc structure */
	int	*u_ar0;			/* address of users saved R0 */
	char	u_comm[MAXCOMLEN + 1];

/* syscall parameters, results and catches */
	int	u_arg[8];		/* arguments to current system call */
	int	*u_ap;			/* pointer to arglist */
	label_t	u_qsave;		/* for non-local gotos on interrupts */
	union {				/* syscall return values */
		struct	{
			int	R_val1;
			int	R_val2;
		} u_rv;
#define	r_val1	u_rv.R_val1
#define	r_val2	u_rv.R_val2
		off_t	r_off;
		time_t	r_time;
	} u_r;
	char	u_error;		/* return error code */
	char	u_eosys;		/* special action on end of syscall */

/* 1.1 - processes and protection */
#if	ROMP_DUALCALL
	char	u_calltype;		/* 0 == old calling sequence */
#endif	ROMP_DUALCALL
	uid_t	u_uid;			/* effective user id */
	uid_t	u_ruid;			/* real user id */
	gid_t	u_gid;			/* effective group id */
	gid_t	u_rgid;			/* real group id */
	gid_t	u_groups[NGROUPS];	/* groups, 0 terminated */

/* 1.2 - memory management */
	size_t	u_tsize;		/* text size (clicks) */
	size_t	u_dsize;		/* data size (clicks) */
	size_t	u_ssize;		/* stack size (clicks) */
	struct	dmap u_dmap;		/* disk map for data segment */
	struct	dmap u_smap;		/* disk map for stack segment */
	struct	dmap u_cdmap, u_csmap;	/* shadows of u_dmap, u_smap, for
					   use of parent during fork */
	label_t u_ssave;		/* label variable for swapping */
	size_t	u_odsize, u_ossize;	/* for (clumsy) expansion swaps */
	time_t	u_outime;		/* user time at last sample */

/* 1.3 - signal management */
#if	CS_BUGFIX
	int	(*u_signal[NSIG+1])();	/* disposition of signals */
	int	u_sigmask[NSIG+1];	/* signals to be blocked */
#else	CS_BUGFIX
	int	(*u_signal[NSIG])();	/* disposition of signals */
	int	u_sigmask[NSIG];	/* signals to be blocked */
#endif	CS_BUGFIX
	int	u_sigonstack;		/* signals to take on sigstack */
	int	u_sigintr;		/* signals that interrupt syscalls */
	int	u_oldmask;		/* saved mask from before sigpause */
	int	u_code;			/* ``code'' to trap */
	struct	sigstack u_sigstack;	/* sp & on stack state variable */
#define	u_onstack	u_sigstack.ss_onstack
#define	u_sigsp		u_sigstack.ss_sp

/* 1.4 - descriptor management */
	struct	file *u_ofile[NOFILE];	/* file structures for open files */
	char	u_pofile[NOFILE];	/* per-process flags of open files */
	int	u_lastfile;		/* high-water mark of u_ofile */
#define	UF_EXCLOSE 	0x1		/* auto-close on exec */
#define	UF_MAPPED 	0x2		/* mapped from device */
	struct	inode *u_cdir;		/* current directory */
	struct	inode *u_rdir;		/* root directory of current process */
	struct	tty *u_ttyp;		/* controlling tty pointer */
	dev_t	u_ttyd;			/* controlling tty dev */
	short	u_cmask;		/* mask for file creation */

/* 1.5 - timing and statistics */
	struct	rusage u_ru;		/* stats for this proc */
	struct	rusage u_cru;		/* sum of stats for reaped children */
	struct	itimerval u_timer[3];
	int	u_XXX[3];
	struct	timeval u_start;
	short	u_acflag;

	struct uprof {			/* profile arguments */
		short	*pr_base;	/* buffer base */
		unsigned pr_size;	/* buffer size */
		unsigned pr_off;	/* pc offset */
		unsigned pr_scale;	/* pc scaling */
	} u_prof;
#if	CS_AID
	short	u_aid;			/* account id */
#endif	CS_AID
#if	CS_RPAUSE
	struct fs *u_rpsfs;		/* resource pause file system */
	char	u_rpswhich;		/* resource pause operation selection */
	u_char	u_rpause;		/* resource pause flags: */
#define	URPS_AGAIN	01		/* - no child processes available */
#define	URPS_NOMEM	02		/* - no memory available */
#define	URPS_NFILE	04		/* - file table overflow */
#define	URPS_NOSPC	010		/* - no space on device */
#endif	CS_RPAUSE
#if	CS_GENERIC
	char	u_modes;		/* process modes: */
#define	UMODE_P_GID	01		/* - 4.2 parent GID on inode create */
#define	UMODE_NOFOLLOW	02		/* - don't follow symbolic links */
#define	UMODE_ALLOWOLD	010		/* - TEMP: allow 4.1 feature use */
#define	UMODE_DIRTRACE	020		/* - TEMP: trace old directory reads */
#define	UMODE_NEWDIR	040		/* - TEMP: mkdir() new format */
#define	UMODE_SETGROUPS	0100		/* - TEMP: setgroups() done */
#define	UMODE_COMPATRACE 0200		/* - TEMP: trace 4.1/4.2 mode */
#endif	CS_GENERIC
#if	CS_RFS
	char	u_rfs;			/* remote syscall state bits: */
#define	URFS_CDIR	01		/* - current diretory is remote */
#define	URFS_RDIR	02		/* - root diretory is remote */
	short	u_rfscode;		/* remote file system call number */
	short	u_rfsncnt;		/* remote file system namei() call count */
#endif	CS_RFS

/* 1.6 - resource controls */
	struct	rlimit u_rlimit[RLIM_NLIMITS];
	struct	quota *u_quota;		/* user's quota structure */
	int	u_qflags;		/* per process quota flags */

/* namei & co. */
	struct nameicache {		/* last successful directory search */
		int nc_prevoffset;	/* offset at which last entry found */
		ino_t nc_inumber;	/* inum of cached directory */
		dev_t nc_dev;		/* dev of cached directory */
		time_t nc_time;		/* time stamp for cache entry */
	} u_ncache;
	struct	nameidata u_nd;

	int	u_stack[1];
};
#endif	MACH_TT

/* u_eosys values */
#define	JUSTRETURN	1
#define	RESTARTSYS	2
#define NORMALRETURN	3
#if	CS_COMPAT
#define SIMULATERTI	4
#endif	CS_COMPAT

/* u_error codes */
#ifdef KERNEL
#include "errno.h"
#else
#include <errno.h>
#endif

#ifdef KERNEL
#if	MACH_TT
#ifdef	KERNEL
#include "../h/thread.h"
#else	KERNEL
#include <sys/thread.h>
#endif	KERNEL
#define u	(current_thread()->u_address)
#define u_pcb		utask->uu_pcb
#define u_procp		utask->uu_procp
#define u_ar0		uthread->uu_ar0
#define u_comm		utask->uu_comm

#define u_arg		uthread->uu_arg
#define u_ap		uthread->uu_ap
#define u_qsave		uthread->uu_qsave
#define u_r		uthread->uu_r
#define u_error		uthread->uu_error
#define u_eosys		uthread->uu_eosys

#define u_uid		utask->uu_uid
#define u_ruid		utask->uu_ruid
#define u_gid		utask->uu_gid
#define u_rgid		utask->uu_rgid
#define u_groups	utask->uu_groups

#define u_tsize		utask->uu_tsize
#define u_dsize		utask->uu_dsize
#define u_ssize		utask->uu_ssize
#define u_dmap		utask->uu_dmap
#define u_smap		utask->uu_smap
#define u_cdmap		utask->uu_cdmap
#define u_csmap		utask->uu_csmap
#define u_ssave		utask->uu_ssave
#define u_odsize	utask->uu_odsize
#define u_ossize	utask->uu_ossize
#define u_outime	utask->uu_outime

#define u_signal	utask->uu_signal
#define u_sigmask	utask->uu_sigmask
#define u_sigonstack	utask->uu_sigonstack
#define u_sigintr	utask->uu_sigintr
#define u_oldmask	utask->uu_oldmask
#define u_code		utask->uu_code
#define u_sigstack	utask->uu_sigstack

#define	u_onstack	u_sigstack.ss_onstack
#define	u_sigsp		u_sigstack.ss_sp

#define u_ofile		utask->uu_ofile
#define u_pofile	utask->uu_pofile
#define u_lastfile	utask->uu_lastfile
#define u_cdir		utask->uu_cdir
#define u_rdir		utask->uu_rdir
#define u_ttyp		utask->uu_ttyp
#define u_ttyd		utask->uu_ttyd
#define u_cmask		utask->uu_cmask

#define u_ru		utask->uu_ru
#define u_cru		utask->uu_cru
#define u_timer		utask->uu_timer
#define u_XXX		utask->uu_XXX
#define u_start		utask->uu_start
#define u_acflag	utask->uu_acflag

#define u_prof		utask->uu_prof
#if	CS_AID
#define u_aid		utask->uu_aid
#endif	CS_AID
#if	CS_RPAUSE
#define u_rpsfs		uthread->uu_rpsfs
#define u_rpswhich	uthread->uu_rpswhich
#define u_rpause	uthread->uu_rpause
#endif	CS_RPAUSE
#if	CS_GENERIC
#define u_modes		utask->uu_modes
#endif	CS_GENERIC
#if	CS_RFS
#define u_rfs		uthread->uu_rfs
#define u_rfscode	uthread->uu_rfscode
#define u_rfsncnt	utask->uu_rfsncnt
#endif	CS_RFS

#define u_rlimit	utask->uu_rlimit
#define u_quota		utask->uu_quota
#define u_qflags	utask->uu_qflags

#define u_ncache	uthread->uu_ncache
#define u_nd		uthread->uu_nd
#else	MACH_TT
extern	struct user u;
#endif	MACH_TT
extern	struct user swaputl;
extern	struct user forkutl;
extern	struct user xswaputl;
extern	struct user xswap2utl;
extern	struct user pushutl;
extern	struct user vfutl;
#endif
