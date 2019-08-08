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
 *	@(#)kern_fork.c	6.7 (Berkeley) 6/8/85
 */
#if	CMU
/*
 **********************************************************************
 * HISTORY
 *  1-Jun-86  Avadis Tevanian (avie) at Carnegie-Mellon University
 *	Conditional compilation for kernel semaphores under MACH_SEM.
 *
 * 14-Feb-86  Bill Bolosky (bolosky) at Carnegie-Mellon University
 *	Merged in stuff from the IBM source to support running on a
 *	Sailboat.  Switchable under romp.
 *
 * 25-Feb-86  Avadis Tevanian (avie) at Carnegie-Mellon University
 *	Installed VM changes.
 *
 * 25-Jan-86  Avadis Tevanian (avie) at Carnegie-Mellon University
 *	Upgraded to 4.3.
 *
 * 11-Dec-85  Robert V Baron (rvb) at Carnegie-Mellon University
 *	Added a second argument to Fork1 to say whether the accent proc
 *	structure had been set up already.  This is necessary for the
 *	Accent Fork to work correctly.  fork and vfork were modified to
 *	call fork1 zero for the arg.
 *
 * 23-Oct-85  Avadis Tevanian (avie) at Carnegie-Mellon University
 *	Only increment u.u_cdir->i_count if u.u_cdir is non-null.  It is
 *	possible to be null when forking the initial idle processes.
 *
 * 25-Aug-85  Avadis Tevanian (avie) at Carnegie-Mellon University
 *	Added call to thread_copy to copy thread information on a fork.
 *
 * 2-Aug-85	David L. Black (dlb) at CMU.  Added code to init user
 *	timer in uproc table at process creation and pid.
 *
 * 26-Jul-85  Mike Accetta (mja) at Carnegie-Mellon University
 *	CS_COMPAT:  Fixed to avoid clearing A41MODE and A42MODE bits in
 *	accounting flags across a fork since these are still valid for
 *	the new process image.
 *	CS_RFS:  Added hook to inform remote system call handler about
 *	a new process in case the current process is using any remote
 *	objects which must be duplicated.
 *	CS_XONLY:  Fixed to also replicate execute only bit across
 *	fork().
 *	[V1(1)]
 *
 * 11-Jul-85  Robert V Baron (rvb) at Carnegie-Mellon University
 *	made fork1 return the proc ptr of the  child so that we can
 *	quickly get the IPCRECORD of the child for the "accent" fork.
 *
 * 25-May-85  Mike Accetta (mja) at Carnegie-Mellon University
 *	CS_COMPAT:  changed to replicate SJCSIG bit across fork().
 *	[V1(1)].
 *
 * 10-May-85  Glenn Marcy (gm0w) at Carnegie-Mellon University
 *	Upgraded to 4.2BSD.  Carried over changes below [V1(1)].
 *
 *  1-Nov-84  Avadis Tevanian (avie) at Carnegie-Mellon University
 *	Call sem_init at fork time.
 *
 *  5-Sep-84  Robert V Baron (rvb) at Carnegie-Mellon University
 *	call IPCactivate also
 *
 * 30-Aug-84  Robert V Baron (rvb) at Carnegie-Mellon University
 *	Added call to shminit to initialize the child so as to not have it
 *	own any shared memory acquired from the parent.
 *
 * 19-Feb-82  Mike Accetta (mja) at Carnegie-Mellon University
 *	CS_ICHK:  Changed inode reference count modifications to use
 *	incr/decr macros to check for consistency (V3.04c).
 *
 * 17-Nov-81  Mike Accetta (mja) at Carnegie-Mellon University
 *	CS_NOFILE:  Changed newproc() to pass the SNOFILE p_flag bit
 *	through to the child (V3.03a).
 *
 **********************************************************************
 */
 
#include "cs_compat.h"
#include "cs_ichk.h"
#include "cs_nofile.h"
#include "cs_rfs.h"
#include "cs_xonly.h"
#include "mach_mp.h"
#include "mach_sem.h"
#include "mach_shm.h"
#include "mach_acc.h"
#include "mach_time.h"
#include "mach_vm.h"
#endif	CMU

#include "../machine/reg.h"
#include "../machine/pte.h"
#ifdef	vax
#include "../machine/psl.h"
#endif	vax
#ifdef	romp
#include "../ca/rosetta.h"
#endif	romp

#include "param.h"
#include "systm.h"
#include "map.h"
#include "dir.h"
#include "user.h"
#include "kernel.h"
#include "proc.h"
#include "inode.h"
#include "seg.h"
#include "vm.h"
#include "text.h"
#include "file.h"
#include "acct.h"
#include "quota.h"
#if	MACH_TIME
#include "uproc.h"
#endif	MACH_TIME
#if	MACH_ACC
#include "../sync/mp_queue.h"
#include "../accent/accent.h"
extern procp_ unxIPCActivate();
#endif	MACH_ACC
#if	MACH_MP
#include "../h/thread.h"
#endif	MACH_MP

/*
 * fork system call.
 */
fork()
{

	u.u_cdmap = zdmap;
	u.u_csmap = zdmap;
	if (swpexpand(u.u_dsize, u.u_ssize, &u.u_cdmap, &u.u_csmap) == 0) {
		u.u_r.r_val2 = 0;
		return;
	}
#if	MACH_ACC
	fork1(0, (procp_) 0);
#else	MACH_ACC
	fork1(0);
#endif	MACH_ACC
}

#if	MACH_ACC
Afork(ipcp)
	procp_ ipcp;
{

#if	MACH_VM
#else	MACH_VM
	u.u_cdmap = zdmap;
	u.u_csmap = zdmap;
	if (swpexpand(u.u_dsize, u.u_ssize, &u.u_cdmap, &u.u_csmap) == 0) {
		u.u_r.r_val2 = 0;
		return;
	}
#endif	MACH_VM
	fork1(0, ipcp);
}
#endif	MACH_ACC

vfork()
{

#if	MACH_ACC
	fork1(1, (procp_) 0);
#else	MACH_ACC
	fork1(1);
#endif	MACH_ACC
}

#if	MACH_ACC
fork1(isvfork, ipcp)
	int isvfork;
	procp_ ipcp;
#else	MACH_ACC
fork1(isvfork)
	int isvfork;
#endif	MACH_ACC
{
	register struct proc *p1, *p2;
	register a;

	a = 0;
	if (u.u_uid != 0) {
		for (p1 = allproc; p1; p1 = p1->p_nxt)
			if (p1->p_uid == u.u_uid)
				a++;
		for (p1 = zombproc; p1; p1 = p1->p_nxt)
			if (p1->p_uid == u.u_uid)
				a++;
	}
	/*
	 * Disallow if
	 *  No processes at all;
	 *  not su and too many procs owned; or
	 *  not su and would take last slot.
	 */
	p2 = freeproc;
	if (p2==NULL)
		tablefull("proc");
	if (p2==NULL || (u.u_uid!=0 && (p2->p_nxt == NULL || a>MAXUPRC))) {
		u.u_error = EAGAIN;
#if	MACH_VM
#else	MACH_VM
		if (!isvfork) {
			(void) vsexpand(0, &u.u_cdmap, 1);
			(void) vsexpand(0, &u.u_csmap, 1);
		}
#endif	MACH_VM
		goto out;
	}
	p1 = u.u_procp;
	if (newproc(isvfork)) {
		u.u_r.r_val1 = p1->p_pid;
		u.u_r.r_val2 = 1;  /* child */
#if	CS_COMPAT
		u.u_acflag &= (A41MODE|A42MODE);
		u.u_acflag |= AFORK;
#else	CS_COMPAT
		u.u_start = time;
		u.u_acflag = AFORK;
#endif	CS_COMPAT
#if	MACH_SEM
		sem_initp();
#endif	MACH_SEM
#if	MACH_SHM
		unxshmpinit();	/* child is not charged for shm */
#endif	MACH_SHM
#if	MACH_ACC
		unxIPCenv(u.u_procp,
			  (int) ipcp ? ipcp : unxIPCActivate());	/* get (u+p)*id of child for IPC */
#endif	MACH_ACC
		return;
	}
	u.u_r.r_val1 = p2->p_pid;

out:
	u.u_r.r_val2 = 0;
}

/*
 * Create a new process-- the internal version of
 * sys fork.
 * It returns 1 in the new process, 0 in the old.
 */
newproc(isvfork)
	int isvfork;
{
	register struct proc *rpp, *rip;
	register int n;
	register struct file *fp;
#if	MACH_TIME
	register struct uproc *up;
#endif	MACH_TIME
	static int pidchecked = 0;

	/*
	 * First, just locate a slot for a process
	 * and copy the useful info from this process into it.
	 * The panic "cannot happen" because fork has already
	 * checked for the existence of a slot.
	 */
	mpid++;
retry:
	if (mpid >= 30000) {
		mpid = 100;
		pidchecked = 0;
	}
	if (mpid >= pidchecked) {
		int doingzomb = 0;

		pidchecked = 30000;
		/*
		 * Scan the proc table to check whether this pid
		 * is in use.  Remember the lowest pid that's greater
		 * than mpid, so we can avoid checking for a while.
		 */
		rpp = allproc;
again:
		for (; rpp != NULL; rpp = rpp->p_nxt) {
			if (rpp->p_pid == mpid || rpp->p_pgrp == mpid) {
				mpid++;
				if (mpid >= pidchecked)
					goto retry;
			}
			if (rpp->p_pid > mpid && pidchecked > rpp->p_pid)
				pidchecked = rpp->p_pid;
			if (rpp->p_pgrp > mpid && pidchecked > rpp->p_pgrp)
				pidchecked = rpp->p_pgrp;
		}
		if (!doingzomb) {
			doingzomb = 1;
			rpp = zombproc;
			goto again;
		}
	}
	if ((rpp = freeproc) == NULL)
		panic("no procs");

	freeproc = rpp->p_nxt;			/* off freeproc */
	rpp->p_nxt = allproc;			/* onto allproc */
	rpp->p_nxt->p_prev = &rpp->p_nxt;	/*   (allproc is never NULL) */
	rpp->p_prev = &allproc;
	allproc = rpp;

	/*
	 * Make a proc table entry for the new process.
	 */
	rip = u.u_procp;
#ifdef QUOTA
	rpp->p_quota = rip->p_quota;
	rpp->p_quota->q_cnt++;
#endif
	rpp->p_stat = SIDL;
	timerclear(&rpp->p_realtimer.it_value);
#if	CS_COMPAT || CS_NOFILE || CS_XONLY
	rpp->p_flag = SLOAD | (rip->p_flag & (SPAGI|SOUSIG
#if	CS_COMPAT
					      |SJCSIG
#endif	CS_COMPAT
#if	CS_XONLY
					      |SXONLY
#endif	CS_XONLY
					     ));
#else	CS_COMPAT || CS_NOFILE || CS_XONLY
	rpp->p_flag = SLOAD | (rip->p_flag & (SPAGI|SOUSIG));
#endif	CS_COMPAT || CS_NOFILE || CS_XONLY
#if	MACH_VM
	rpp->p_ndx = rpp - proc;
#else	MACH_VM
	if (isvfork) {
		rpp->p_flag |= SVFORK;
#ifndef romp
		rpp->p_ndx = rip->p_ndx;
#else romp
		rpp->p_ndx = rpp - proc;
#endif romp
	} else
		rpp->p_ndx = rpp - proc;
#endif	MACH_VM
	rpp->p_uid = rip->p_uid;
	rpp->p_pgrp = rip->p_pgrp;
	rpp->p_nice = rip->p_nice;
	rpp->p_textp = isvfork ? 0 : rip->p_textp;
	rpp->p_pid = mpid;
	rpp->p_ppid = rip->p_pid;
	rpp->p_pptr = rip;
	rpp->p_osptr = rip->p_cptr;
	if (rip->p_cptr)
		rip->p_cptr->p_ysptr = rpp;
	rpp->p_ysptr = NULL;
	rpp->p_cptr = NULL;
	rip->p_cptr = rpp;
	rpp->p_time = 0;
	rpp->p_cpu = 0;
	rpp->p_sigmask = rip->p_sigmask;
	rpp->p_sigcatch = rip->p_sigcatch;
	rpp->p_sigignore = rip->p_sigignore;
	/* take along any pending signals like stops? */
#if	MACH_VM
	{
#else	MACH_VM
	if (isvfork) {
		rpp->p_tsize = rpp->p_dsize = rpp->p_ssize = 0;
		rpp->p_szpt = clrnd(ctopt(UPAGES));
		forkstat.cntvfork++;
		forkstat.sizvfork += rip->p_dsize + rip->p_ssize;
	} else {
#endif	MACH_VM
		rpp->p_tsize = rip->p_tsize;
		rpp->p_dsize = rip->p_dsize;
		rpp->p_ssize = rip->p_ssize;
		rpp->p_szpt = rip->p_szpt;
		forkstat.cntfork++;
		forkstat.sizfork += rip->p_dsize + rip->p_ssize;
	}
	rpp->p_rssize = 0;
	rpp->p_maxrss = rip->p_maxrss;
	rpp->p_wchan = 0;
	rpp->p_slptime = 0;
	rpp->p_pctcpu = 0;
	rpp->p_cpticks = 0;
	n = PIDHASH(rpp->p_pid);
	rpp->p_idhash = pidhash[n];
	pidhash[n] = rpp - proc;
	multprog++;

#if	MACH_VM
#else	MACH_VM
#if	MACH_MP
	thread_copy(rpp - proc, rip - proc);
#endif	MACH_MP
#endif	MACH_VM

#ifdef romp
	/* values for seg reg 0 and 1 */
	if (rip->p_textp)
		rpp->p_sid0 = rip->p_sid0;
	else
		rpp->p_sid0 = make407sid(rpp);
	rpp->p_sid1 = makeUsid(rpp);
#endif romp

#if	MACH_TIME
	/* now init uproc table entry.  In same slot as proc table entry. */
	up = &uproc[ rpp - proc ];

	up->up_utime.tr_timestamp = 0;
	up->up_utime.tr_elapsed.tv_sec = 0;
	up->up_utime.tr_elapsed.tv_usec = 0;
	up->up_pid = mpid;
#endif	MACH_TIME
	/*
	 * Increase reference counts on shared objects.
	 */
	for (n = 0; n <= u.u_lastfile; n++) {
		fp = u.u_ofile[n];
		if (fp == NULL)
			continue;
		fp->f_count++;
	}
#if	MACH_MP
	if (u.u_cdir)
#endif	MACH_MP
#if	CS_ICHK
	iincr_chk(u.u_cdir);
#else	CS_ICHK
	u.u_cdir->i_count++;
#endif	CS_ICHK
	if (u.u_rdir)
#if	CS_ICHK
		iincr_chk(u.u_rdir);
#else	CS_ICHK
		u.u_rdir->i_count++;
#endif	CS_ICHK

	/*
	 * This begins the section where we must prevent the parent
	 * from being swapped.
	 */
	rip->p_flag |= SKEEP;
#if	CS_RFS
	/*
	 *  Notify the remote system call code that a fork() is happening.
	 */
	rfs_fork(rpp, isvfork);
#endif	CS_RFS
#if	MACH_VM
	if (procdup(rpp, rip))	/* child, parent order */
#else	MACH_VM
	if (procdup(rpp, isvfork))
#endif	MACH_VM
		return (1);
#if	defined(romp) && !defined(MACH_VM)
	if (isvfork)
		rpp->p_ndx = rip->p_ndx;
#endif	romp && !MACH_VM

	/*
	 * Make child runnable and add to run queue.
	 */
#if	MACH_MP
	/*
	 *	We immediately switch to the child in hopes
	 *	that he will exec or exit quickly. If he doesn't
	 *	we've lost nothing anyway.
	 */
	(void) splhigh();
	rpp->p_stat = SRUN;
	context_swtch(thread_table[rpp-proc]);
/*	unix_setrq(rpp);*/
#else	MACH_MP
	(void) spl6();
	rpp->p_stat = SRUN;
	setrq(rpp);
#endif	MACH_MP
	(void) spl0();

	/*
	 * Cause child to take a non-local goto as soon as it runs.
	 * On older systems this was done with SSWAP bit in proc
	 * table; on VAX we use u.u_pcb.pcb_sswap so don't need
	 * to do rpp->p_flag |= SSWAP.  Actually do nothing here.
	 */
	/* rpp->p_flag |= SSWAP; */

	/*
	 * Now can be swapped.
	 */
	rip->p_flag &= ~SKEEP;

#if	MACH_VM
#else	MACH_VM
	/*
	 * If vfork make chain from parent process to child
	 * (where virtal memory is temporarily).  Wait for
	 * child to finish, steal virtual memory back,
	 * and wakeup child to let it die.
	 */
	if (isvfork) {
		u.u_procp->p_xlink = rpp;
		u.u_procp->p_flag |= SNOVM;
		while (rpp->p_flag & SVFORK)
			sleep((caddr_t)rpp, PZERO - 1);
		if ((rpp->p_flag & SLOAD) == 0)
			panic("newproc vfork");
		uaccess(rpp, Vfmap, &vfutl);
		u.u_procp->p_xlink = 0;
		vpassvm(rpp, u.u_procp, &vfutl, &u, Vfmap);
		u.u_procp->p_flag &= ~SNOVM;
		rpp->p_ndx = rpp - proc;
		rpp->p_flag |= SVFDONE;
#ifdef romp
		rpp->p_sid0 = make407sid(rpp);
#endif romp
		wakeup((caddr_t)rpp);
	}
#endif	MACH_VM

	/*
	 * 0 return means parent.
	 */
	return (0);
}
