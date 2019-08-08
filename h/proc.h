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
 *	@(#)proc.h	6.4 (Berkeley) 6/8/85
 */
#ifndef _PROC_
#define	_PROC_
#if	CMU
/*
 **********************************************************************
 * HISTORY
 * 25-Jan-86  Avadis Tevanian (avie) at Carnegie-Mellon University
 *	Upgraded to 4.3.  Removed CS_NOFILE since Berkeley now allows
 *	larger numbers of file descriptors.
 *
 * 18-Feb-86  Bill Bolosky (bolosky) at Carnegie-Mellon University
 *	Added definition of p_sid0 and p_sid1 in proc structure for
 *	Saialboat under switch romp from IBM code.
 *
 *  3-Sep-85  Avadis Tevanian (avie) at Carnegie-Mellon University
 *	MACH_MP:  Added SACTIVE flag to signify that a process is actually
 *	running on a cpu.
 *
 * 25-May-85  Mike Accetta (mja) at Carnegie-Mellon University
 *	Upgraded from 4.1BSD.  Carried over changes below.
 *	CS_COMPAT: defined temporary SJCSIG bit for 4.1 signal
 *	compatibility.
 *	[V1(1)]
 *
 * 25-Aug-85  Avadis Tevanian (avie) at Carnegie-Mellon University
 *	Allow recursive includes.
 *
 * 20-Aug-81  Mike Accetta (mja) at Carnegie-Mellon University
 *	CS_XONLY:  added SXONLY bit definition to flag execute only
 *	processes;
 *	CS_NOFILE:  added SXNOFILE bit definition to allow
 *	expanded number of file descriptors (V3.00).
 *
 **********************************************************************
 */
 
#ifdef	KERNEL
#include "cs_compat.h"
#include "cs_xonly.h"
#include "mach_mp.h"
#else	KERNEL
#include <sys/features.h>
#endif	KERNEL
#endif	CMU

/*
 * One structure allocated per active
 * process. It contains all data needed
 * about the process while the
 * process may be swapped out.
 * Other per process data (user.h)
 * is swapped with the process.
 */
struct	proc {
	struct	proc *p_link;	/* linked list of running processes */
	struct	proc *p_rlink;
	struct	proc *p_nxt;	/* linked list of allocated proc slots */
	struct	proc **p_prev;		/* also zombies, and free proc's */
	struct	pte *p_addr;	/* u-area kernel map address */
#ifdef romp
       u_short	p_sid0;          /* sid for code/data segment */
       u_short	p_sid1;          /* sid for stack/u struct segment */
#endif romp
	char	p_usrpri;	/* user-priority based on p_cpu and p_nice */
	char	p_pri;		/* priority, negative is high */
	char	p_cpu;		/* cpu usage for scheduling */
	char	p_stat;
	char	p_time;		/* resident time for scheduling */
	char	p_nice;		/* nice for cpu usage */
	char	p_slptime;	/* time since last block */
	char	p_cursig;
	int	p_sig;		/* signals pending to this process */
	int	p_sigmask;	/* current signal mask */
	int	p_sigignore;	/* signals being ignored */
	int	p_sigcatch;	/* signals being caught by user */
	int	p_flag;
	short	p_uid;		/* user id, used to direct tty signals */
	short	p_pgrp;		/* name of process group leader */
	short	p_pid;		/* unique process id */
	short	p_ppid;		/* process id of parent */
	u_short	p_xstat;	/* Exit status for wait */
	struct	rusage *p_ru;	/* mbuf holding exit information */
	short	p_poip;		/* page outs in progress */
	short	p_szpt;		/* copy of page table size */
	size_t	p_tsize;	/* size of text (clicks) */
	size_t	p_dsize;	/* size of data space (clicks) */
	size_t	p_ssize;	/* copy of stack size (clicks) */
	size_t 	p_rssize; 	/* current resident set size in clicks */
	size_t	p_maxrss;	/* copy of u.u_limit[MAXRSS] */
	size_t	p_swrss;	/* resident set size before last swap */
	swblk_t	p_swaddr;	/* disk address of u area when swapped */
	caddr_t p_wchan;	/* event process is awaiting */
	struct	text *p_textp;	/* pointer to text structure */
	struct	pte *p_p0br;	/* page table base P0BR */
	struct	pte *p_p1br;	/* page table base P1BR */
	struct	proc *p_xlink;	/* linked list of procs sharing same text */
	short	p_cpticks;	/* ticks of cpu time */
	float	p_pctcpu;	/* %cpu for this process during p_time */
	short	p_ndx;		/* proc index for memall (because of vfork) */
	short	p_idhash;	/* hashed based on p_pid for kill+exit+... */
	struct	proc *p_pptr;	/* pointer to process structure of parent */
	struct	proc *p_cptr;	/* pointer to youngest living child */
	struct	proc *p_osptr;	/* pointer to older sibling processes */
	struct	proc *p_ysptr;	/* pointer to younger siblings */
	struct	itimerval p_realtimer;
	struct	quota *p_quota;	/* quotas for this process */
};

#define	PIDHSZ		64
#define	PIDHASH(pid)	((pid) & (PIDHSZ - 1))

#ifdef KERNEL
short	pidhash[PIDHSZ];
struct	proc *pfind();
struct	proc *proc, *procNPROC;	/* the proc table itself */
struct	proc *freeproc, *zombproc, *allproc;
			/* lists of procs in various states */
int	nproc;

#define	NQS	32		/* 32 run queues */
struct	prochd {
	struct	proc *ph_link;	/* linked list of running processes */
	struct	proc *ph_rlink;
} qs[NQS];
int	whichqs;		/* bit mask summarizing non-empty qs's */
#endif

/* stat codes */
#define	SSLEEP	1		/* awaiting an event */
#define	SWAIT	2		/* (abandoned state) */
#define	SRUN	3		/* running */
#define	SIDL	4		/* intermediate state in process creation */
#define	SZOMB	5		/* intermediate state in process termination */
#define	SSTOP	6		/* process being traced */

/* flag codes */
#define	SLOAD	0x0000001	/* in core */
#define	SSYS	0x0000002	/* swapper or pager process */
#define	SLOCK	0x0000004	/* process being swapped out */
#define	SSWAP	0x0000008	/* save area flag */
#define	STRC	0x0000010	/* process is being traced */
#define	SWTED	0x0000020	/* another tracing flag */
#define	SULOCK	0x0000040	/* user settable lock in core */
#define	SPAGE	0x0000080	/* process in page wait state */
#define	SKEEP	0x0000100	/* another flag to prevent swap out */
#define	SOMASK	0x0000200	/* restore old mask after taking signal */
#define	SWEXIT	0x0000400	/* working on exiting */
#define	SPHYSIO	0x0000800	/* doing physical i/o (bio.c) */
#define	SVFORK	0x0001000	/* process resulted from vfork() */
#define	SVFDONE	0x0002000	/* another vfork flag */
#define	SNOVM	0x0004000	/* no vm, parent in a vfork() */
#define	SPAGI	0x0008000	/* init data space on demand, from inode */
#define	SSEQL	0x0010000	/* user warned of sequential vm behavior */
#define	SUANOM	0x0020000	/* user warned of random vm behavior */
#define	STIMO	0x0040000	/* timing out during sleep */
/* was SDETACH */
#if	MACH_MP
#define	SACTIVE	0x0080000	/* process is executing */
#endif	MACH_MP
#define	SOUSIG	0x0100000	/* using old signal mechanism */
#define	SOWEUPC	0x0200000	/* owe process an addupc() call at next ast */
#define	SSEL	0x0400000	/* selecting; wakeup/waiting danger */
#define	SLOGIN	0x0800000	/* a login process (legit child of init) */
#define	SPTECHG	0x1000000	/* pte's for process have changed */
#if	CS_COMPAT
#define	SJCSIG	0x10000000	/* using old job control calls */
#endif	CS_COMPAT
#if	CS_XONLY
#define	SXONLY	0x40000000	/* process image read protected	*/
#endif	CS_XONLY
#if	MACH_MP
#define SIDLE	0x80000000	/* is an idle process */
#endif	MACH_MP
#endif	_PROC_
