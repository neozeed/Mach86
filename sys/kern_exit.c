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
 *	@(#)kern_exit.c	6.10 (Berkeley) 6/8/85
 */
#if	CMU
/*
 **********************************************************************
 * HISTORY
 *  1-Jun-86  Avadis Tevanian (avie) at Carnegie-Mellon University
 *	Conditional compilation for kernel semaphores under MACH_SEM.
 *
 * 22-Mar-86  Avadis Tevanian (avie) at Carnegie-Mellon University
 *	Merged VM and Romp versions.
 *
 * 14-Feb-86  Bill Bolosky (bolosky) at Carnegie-Mellon University
 *	Added Romp support.
 *
 * 25-Feb-86  Avadis Tevanian (avie) at Carnegie-Mellon University
 *	Installed VM changes.
 *
 * 25-Jan-86  Avadis Tevanian (avie) at Carnegie-Mellon University
 *	Upgraded to 4.3.
 *
 * 6-Dec-85  David L. Black (dlb ) at CMU.  Added switch for new user timing.
 *
 * 11-Oct-85  Robert V Baron (rvb) at Carnegie-Mellon University
 *	Added code to panic if the exec of init fails, basically if
 *	init dies.  OF course, icode has to exit not loop for it to
 *	be used.  There used to be similar code which did not print
 *	any return value.
 *
 * 25-Aug-85  Avadis Tevanian (avie) at Carnegie-Mellon University
 *	Added call to thread_exit to clean up the "thread" information.
 *
 * 12-Aug-85  Mike Accetta (mja) at Carnegie-Mellon University
 *	CS_COMPAT:  changed wait1() to allow restarts unless old
 *	signal mechanism is in use without job-control.
 *	[V1(1)]
 *
 *  2-Aug-85  David L. Black (dlb) at CMU.  Added statement so
 *	user time values in ru are from new timing system.
 *
 * 09-May-85  Glenn Marcy (gm0w) at Carnegie-Mellon University
 *	Upgraded from 4.1BSD.  Carried over changes:
 *	CS_IPC:  Added call to IPCSuicide() in exit().
 *	CS_RFS:  Added call to rfs_exit() in exit().
 *	NILDR:  Added call to ildrrma() in exit().
 *	[V1(1)]
 *
 *  1-Nov-84  Avadis Tevanian (avie) at Carnegie-Mellon University
 *	Add call to sem_exit in exit.
 *
 *  5-Sep-84  Robert V Baron (rvb) at Carnegie-Mellon University
 *	Call to IPCsuicide in exit
 *
 * 30-Aug-84  Robert V Baron (rvb) at Carnegie-Mellon University
 *	Added calls to shmfree in exit().
 *
 *  1-Apr-84  Avadis Tevanian (avie) at Carnegie-Mellon University
 *	Added calls to rfs_exit and IPCSuicide in exit().
 *
 **********************************************************************
 */
 
#include "cs_generic.h"
#include "cs_compat.h"
#include "cs_ipc.h"
#include "cs_rfs.h"
#include "mach_mp.h"
#include "mach_sem.h"
#include "mach_shm.h"
#include "mach_acc.h"
#include "mach_time.h"
#include "mach_vm.h"
 
#include "ild.h"
#endif	CMU

#include "../machine/reg.h"
#ifdef	romp
#include "../machine/scr.h"
#else	romp
#include "../machine/psl.h"
#endif	romp

#include "param.h"
#include "systm.h"
#include "map.h"
#include "dir.h"
#include "user.h"
#include "kernel.h"
#include "proc.h"
#include "buf.h"
#include "wait.h"
#include "vm.h"
#include "file.h"
#include "mbuf.h"
#include "inode.h"
#include "syslog.h"

#if	MACH_ACC
#include "../accent/vax/impl_macros.h"
extern int *aproc;			/* close enough; see kern_ipc.h */
#endif	MACH_ACC

#if	MACH_TIME > 0
#include "../h/uproc.h"
extern int mach_time;
#endif	MACH_TIME > 0

#if	MACH_MP
#include "../h/task.h"
#endif	MACH_MP

#if	MACH_VM
#include "../vm/vm_map.h"
#include "../h/thread.h"
#endif	MACH_VM

/*
 * Exit system call: pass back caller's arg
 */
rexit()
{
	register struct a {
		int	rval;
	} *uap;

	uap = (struct a *)u.u_ap;
	exit((uap->rval & 0377) << 8);
}

/*
 * Release resources.
 * Save u. area for parent to look at.
 * Enter zombie state.
 * Wake up parent and init processes,
 * and dispose of children.
 */
exit(rv)
	int rv;
{
	register int i;
	register struct proc *p, *q, *nq;
	register int x;
	struct mbuf *m = m_getclr(M_WAIT, MT_ZOMBIE);

#ifdef PGINPROF
	vmsizmon();
#endif
	p = u.u_procp;
	p->p_flag &= ~(STRC|SULOCK);
	p->p_flag |= SWEXIT;
	p->p_sigignore = ~0;
	p->p_cpticks = 0;
	p->p_pctcpu = 0;
#if	MACH_MP
	thread_exit(p - proc);
#endif	MACH_MP
	for (i = 0; i < NSIG; i++)
		u.u_signal[i] = SIG_IGN;
	untimeout(realitexpire, (caddr_t)p);
#if	CS_IPC
	/* flush IPC before we flush our address space */
	IPCSuicide();
#endif	CS_IPC
#if	MACH_VM
#else	MACH_VM
	/*
	 * Release virtual memory.  If we resulted from
	 * a vfork(), instead give the resources back to
	 * the parent.
	 */
	if ((p->p_flag & SVFORK) == 0)
		vrelvm();
	else {
		p->p_flag &= ~SVFORK;
		wakeup((caddr_t)p);
		while ((p->p_flag & SVFDONE) == 0)
			sleep((caddr_t)p, PZERO - 1);
		p->p_flag &= ~SVFDONE;
	}
#endif	MACH_VM
	for (i = 0; i <= u.u_lastfile; i++) {
		struct file *f;

		f = u.u_ofile[i];
		if (f) {
			u.u_ofile[i] = NULL;
			u.u_pofile[i] = 0;
			closef(f);
		}
	}
#if	CS_RFS
	/*  after closing all files ... */
	rfs_exit();
#endif	CS_RFS
#if	MACH_SEM
	sem_exit();		/* clean up semaphores */
#endif	MACH_SEM
#if	MACH_ACC
	unxIPCSuicide(CurrentProcess());	/* but this one is in vmipc.c */
#endif	MACH_ACC
#if	MACH_SHM
	unxshmpfree();
#endif	MACH_SHM
	ilock(u.u_cdir);
	iput(u.u_cdir);
	if (u.u_rdir) {
		ilock(u.u_rdir);
		iput(u.u_rdir);
	}
#if	NILD > 0
	/*
	 * remove outstanding ingres locks for
	 * the dying process
	 */
	ildrma(p->p_pid);
#endif	NILD
	u.u_rlimit[RLIMIT_FSIZE].rlim_cur = RLIM_INFINITY;
	acct();
#ifdef QUOTA
	qclean();
#endif
#if	MACH_VM
	/* map is deallocated in wait1() below */
#else	MACH_VM
	vrelpt(u.u_procp);
	vrelu(u.u_procp, 0);
#endif	MACH_VM
	(void) spl5();		/* hack for mem alloc race XXX */
	if (*p->p_prev = p->p_nxt)		/* off allproc queue */
		p->p_nxt->p_prev = p->p_prev;
	if (p->p_nxt = zombproc)		/* onto zombproc */
		p->p_nxt->p_prev = &p->p_nxt;
	p->p_prev = &zombproc;
	zombproc = p;
	multprog--;
	p->p_stat = SZOMB;
	noproc = 1;
	i = PIDHASH(p->p_pid);
	x = p - proc;
	if (pidhash[i] == x)
		pidhash[i] = p->p_idhash;
	else {
		for (i = pidhash[i]; i != 0; i = proc[i].p_idhash)
			if (proc[i].p_idhash == x) {
				proc[i].p_idhash = p->p_idhash;
				goto done;
			}
		panic("exit");
	}
	if (p->p_pid == 1) {
#if	CS_GENERIC
		printf("init exited with %d\n",
			rv>>8);
#endif	CS_GENERIC
		if (p->p_dsize == 0) {
			printf("Can't exec /etc/init\n");
			for (;;)
				;
		} else
			panic("init died");
	}
done:
	p->p_xstat = rv;
#if	MACH_TIME
	if (mach_time == 1)
	    u.u_ru.ru_utime = uproc[ u.u_procp - proc ].up_utime.tr_elapsed;
#endif	MACH_TIME
	p->p_ru = mtod(m, struct rusage *);
	*p->p_ru = u.u_ru;
	ruadd(p->p_ru, &u.u_cru);
	if (p->p_cptr)		/* only need this if any child is S_ZOMB */
		wakeup((caddr_t)&proc[1]);
	for (q = p->p_cptr; q != NULL; q = nq) {
		nq = q->p_osptr;
		if (nq != NULL)
			nq->p_ysptr = NULL;
		if (proc[1].p_cptr)
			proc[1].p_cptr->p_ysptr = q;
		q->p_osptr = proc[1].p_cptr;
		q->p_ysptr = NULL;
		proc[1].p_cptr = q;
		q->p_pptr = &proc[1];
		q->p_ppid = 1;
		/*
		 * Traced processes are killed
		 * since their existence means someone is screwing up.
		 * Stopped processes are sent a hangup and a continue.
		 * This is designed to be ``safe'' for setuid
		 * processes since they must be willing to tolerate
		 * hangups anyways.
		 */
		if (q->p_flag&STRC) {
			q->p_flag &= ~STRC;
			psignal(q, SIGKILL);
		} else if (q->p_stat == SSTOP) {
			psignal(q, SIGHUP);
			psignal(q, SIGCONT);
		}
		/*
		 * Protect this process from future
		 * tty signals, clear TSTP/TTIN/TTOU if pending.
		 */
		(void) spgrp(q, -1);
	}
	p->p_cptr = NULL;
	psignal(p->p_pptr, SIGCHLD);
	wakeup((caddr_t)p->p_pptr);
#if	MACH_MP
	unix_swtch(0, 0);
#else	MACH_MP
	swtch();
#endif	MACH_MP
}

wait()
{
	struct rusage ru, *rup;

#ifdef	romp
	if ((u.u_ar0[ICSCS] & ICSCS_HOKEY) != ICSCS_HOKEY) {
#else	romp
	if ((u.u_ar0[PS] & PSL_ALLCC) != PSL_ALLCC) {
#endif	romp
		u.u_error = wait1(0, (struct rusage *)0);
		return;
	}
#ifdef	romp
	rup = (struct rusage *)u.u_ar0[R4];	 /* as per C linkage */
	u.u_error = wait1(u.u_ar0[R3], &ru);     /* as per C linkage */
#else	romp
	rup = (struct rusage *)u.u_ar0[R1];
	u.u_error = wait1(u.u_ar0[R0], &ru);
#endif	romp
	if (u.u_error)
		return;
	if (rup != (struct rusage *)0)
		u.u_error = copyout((caddr_t)&ru, (caddr_t)rup,
		    sizeof (struct rusage));
}

/*
 * Wait system call.
 * Search for a terminated (zombie) child,
 * finally lay it to rest, and collect its status.
 * Look also for stopped (traced) children,
 * and pass back status from them.
 */
wait1(options, ru)
	register int options;
	struct rusage *ru;
{
	register f;
	register struct proc *p, *q;

	f = 0;
loop:
	q = u.u_procp;
	for (p = q->p_cptr; p; p = p->p_osptr) {
		f++;
		if (p->p_stat == SZOMB) {
			u.u_r.r_val1 = p->p_pid;
			u.u_r.r_val2 = p->p_xstat;
			p->p_xstat = 0;
			if (ru && p->p_ru)
				*ru = *p->p_ru;
			if (p->p_ru) {
				ruadd(&u.u_cru, p->p_ru);
				(void) m_free(dtom(p->p_ru));
				p->p_ru = 0;
			}
			p->p_stat = NULL;
			p->p_pid = 0;
#if	MACH_TIME > 0
			uproc[ p - proc ].up_pid = 0; /*dlb clear this also */
#endif	MACH_TIME > 0
			p->p_ppid = 0;
			if (*p->p_prev = p->p_nxt)	/* off zombproc */
				p->p_nxt->p_prev = p->p_prev;
			p->p_nxt = freeproc;		/* onto freeproc */
			freeproc = p;
			if (q = p->p_ysptr)
				q->p_osptr = p->p_osptr;
			if (q = p->p_osptr)
				q->p_ysptr = p->p_ysptr;
			if ((q = p->p_pptr)->p_cptr == p)
				q->p_cptr = p->p_osptr;
			p->p_pptr = 0;
			p->p_ysptr = 0;
			p->p_osptr = 0;
			p->p_cptr = 0;
			p->p_sig = 0;
			p->p_sigcatch = 0;
			p->p_sigignore = 0;
			p->p_sigmask = 0;
			p->p_pgrp = 0;
			p->p_flag = 0;
			p->p_wchan = 0;
			p->p_cursig = 0;
#if	MACH_VM
			/*
			 *	Deallocate the task/thread here.
			 */

			thread_destroy(thread_table[p-proc]);
			task_destroy(task_table[p-proc]);
#endif	MACH_VM
			return (0);
		}
		if (p->p_stat == SSTOP && (p->p_flag&SWTED)==0 &&
		    (p->p_flag&STRC || options&WUNTRACED)) {
			p->p_flag |= SWTED;
			u.u_r.r_val1 = p->p_pid;
			u.u_r.r_val2 = (p->p_cursig<<8) | WSTOPPED;
			return (0);
		}
	}
	if (f == 0)
		return (ECHILD);
	if (options&WNOHANG) {
		u.u_r.r_val1 = 0;
		return (0);
	}
#if	CS_COMPAT
	if (/*(u.u_procp->p_flag&SJCSIG == 0) &&*/ setjmp(&u.u_qsave)) {
#else	CS_COMPAT
	if (setjmp(&u.u_qsave)) {
#endif	CS_COMPAT
		p = u.u_procp;
		if ((u.u_sigintr & sigmask(p->p_cursig)) != 0)
			return(EINTR);
		u.u_eosys = RESTARTSYS;
		return (0);
	}
	sleep((caddr_t)u.u_procp, PWAIT);
	goto loop;
}
