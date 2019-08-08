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
 *	@(#)kern_synch.c	6.10 (Berkeley) 6/8/85
 */

#if	CMU
/*
 **********************************************************************
 * HISTORY
 *  4-Jun-86  Avadis Tevanian (avie) at Carnegie-Mellon University
 *	Call halt_cpu instead of inline halt instructions.
 *
 *  3-Jun-86  Rick Rashid (rfr) at Carnegie-Mellon University
 *	Added ipc_wait and ipc_wakeup routines.
 *
 *  1-Jun-86  Avadis Tevanian (avie) at Carnegie-Mellon University
 *	sched.h is now in ../h.
 *
 *  1-Jun-86  Avadis Tevanian (avie) at Carnegie-Mellon University
 *	Cleaned up some unnecessary conditional compilation.
 *
 * 19-May-86  Avadis Tevanian (avie) at Carnegie-Mellon University
 *	Do simple locking around the sleep queues to allow sleep/wakeup
 *	being called from routines that are otherwise fully parallel.
 *
 * 24-Mar-86  Bill Bolosky (bolosky) at Carnegie-Mellon University
 *	SHOW_LOAD && romp: Turned on romp DDS load display feature.
 *
 * 25-Jan-86  Avadis Tevanian (avie) at Carnegie-Mellon University
 *	Upgraded to 4.3.
 *
 * 03-Aug-85  Mike Accetta (mja) at Carnegie-Mellon University
 *	CS_RPAUSE:  Added rpause() routine.
 *	[V1(1)]
 *
 * 10-Jun-85  Mike Accetta (mja) at Carnegie-Mellon University
 *	CS_COMPAT:  Redid tsleep() to use a private timeout()
 *	routine since piggybacking off the alarm (real interval) timer
 *	is no longer appropriate.
 *	[V1(1)]
 *
 * 15-May-85  Glenn Marcy (gm0w) at Carnegie-Mellon University
 *	Upgraded from 4.1BSD.  Carried over changes below:
 *
 *	CS_COMPAT:  Carry over tsleep() routine from 4.1BSD.
 *	[V1(1)]
 *
 **********************************************************************
 */
 
#include "cs_compat.h"
#include "cs_rpause.h"
#include "mach_mp.h"
#include "mach_vm.h"

#include "cpus.h"
#endif	CMU

#include "../machine/pte.h"

#include "param.h"
#include "systm.h"
#include "dir.h"
#include "user.h"
#include "proc.h"
#include "file.h"
#include "inode.h"
#include "vm.h"
#include "kernel.h"
#include "buf.h"

#if	MACH_MP
#include "../h/queue.h"
#include "../sync/lock.h"
#include "../h/thread.h"
#include "../h/sched.h"

#include "../machine/cpu.h"
#endif	MACH_MP
#if	MACH_VM
#include "../vm/pmap.h"
#endif	MACH_VM

#ifdef vax
#include "../vax/mtpr.h"	/* XXX */
#endif
/*
 * Force switch among equal priority processes every 100ms.
 */
roundrobin()
{

	runrun++;
	aston();
	timeout(roundrobin, (caddr_t)0, hz / 10);
}

/* fraction for digital decay to forget 90% of usage in 5*loadav sec */
#define	filter(loadav) ((2 * (loadav)) / (2 * (loadav) + 1))

double	ccpu = 0.95122942450071400909;		/* exp(-1/20) */

/*
 * Recompute process priorities, once a second
 */
schedcpu()
{
	register double ccpu1 = (1.0 - ccpu) / (double)hz;
	register struct proc *p;
	register int s, a;
	float scale = filter(avenrun[0]);

	wakeup((caddr_t)&lbolt);
#if	defined(SHOW_LOAD) && defined(romp)
	a = avenrun[0] * 10;
	display(((a/10) << 4) + (a % 10)); 	/* display in lights */
#endif	SHOW_LOAD && ROMP
	for (p = allproc; p != NULL; p = p->p_nxt) {
		if (p->p_time != 127)
			p->p_time++;
		if (p->p_stat==SSLEEP || p->p_stat==SSTOP)
			if (p->p_slptime != 127)
				p->p_slptime++;
		/*
		 * If the process has slept the entire second,
		 * stop recalculating its priority until it wakes up.
		 */
		if (p->p_slptime > 1) {
			p->p_pctcpu *= ccpu;
			continue;
		}
		/*
		 * p_pctcpu is only for ps.
		 */
		p->p_pctcpu = ccpu * p->p_pctcpu + ccpu1 * p->p_cpticks;
		p->p_cpticks = 0;
#if	MACH_MP
		if (p->p_flag&SIDLE)
			continue;
#endif	MACH_MP
		a = (int) (scale * (p->p_cpu & 0377)) + p->p_nice;
		if (a < 0)
			a = 0;
		if (a > 255)
			a = 255;
		p->p_cpu = a;
		(void) setpri(p);
		s = splhigh();	/* prevent state changes */
		if (p->p_pri >= PUSER) {
#define	PPQ	(128 / NQS)
#if	MACH_MP
			/*
			 * The following statement needs to check SRUN, because
			 * this can run during process creation.  If we run
			 * before the corresponding thread is created, we can
			 * get into trouble looking at bogus data in the
			 * thread table.
			 */
			if (((p->p_pri / PPQ) != (p->p_usrpri / PPQ)) &&
					(p->p_stat == SRUN)) {
				lock_write(&sched_lock);	/* lock the scheduler */
				if (unix_remrq(p)) {
					p->p_pri = p->p_usrpri;
					unix_setrq(p);
				}
				else {
					p->p_pri = p->p_usrpri;
				}
				lock_write_done(&sched_lock);	/* unlock the scheduler */
			}
#else	MACH_MP
			if ((p != u.u_procp || noproc) &&
			    p->p_stat == SRUN &&
			    (p->p_flag & SLOAD) &&
			    (p->p_pri / PPQ) != (p->p_usrpri / PPQ)) {
				remrq(p);
				p->p_pri = p->p_usrpri;
				setrq(p);
			} else
				p->p_pri = p->p_usrpri;
#endif	MACH_MP
		}
		splx(s);
	}
	vmmeter();
	if (runin!=0) {
		runin = 0;
		wakeup((caddr_t)&runin);
	}
	if (bclnlist != NULL)
		wakeup((caddr_t)&proc[2]);
	timeout(schedcpu, (caddr_t)0, hz);
}

/*
 * Recalculate the priority of a process after it has slept for a while.
 */
updatepri(p)
	register struct proc *p;
{
	register int a = p->p_cpu & 0377;
	float scale = filter(avenrun[0]);

	p->p_slptime--;		/* the first time was done in schedcpu */
	while (a && --p->p_slptime)
		a = (int) (scale * a) /* + p->p_nice */;
	if (a < 0)
		a = 0;
	if (a > 255)
		a = 255;
	p->p_cpu = a;
	(void) setpri(p);
}

#define SQSIZE 0100	/* Must be power of 2 */
#define HASH(x)	(( (int) x >> 5) & (SQSIZE-1))
struct slpque {
	struct proc *sq_head;
	struct proc **sq_tailp;
} slpque[SQSIZE];

#if	MACH_VM
simple_lock_t	sleep_lock;
#endif	MACH_VM
/*
 * Give up the processor till a wakeup occurs
 * on chan, at which time the process
 * enters the scheduling queue at priority pri.
 * The most important effect of pri is that when
 * pri<=PZERO a signal cannot disturb the sleep;
 * if pri>PZERO signals will be processed.
 * Callers of this routine must be prepared for
 * premature return, and check that the reason for
 * sleeping has gone away.
 */
sleep(chan, pri)
	caddr_t chan;
	int pri;
{
	register struct proc *rp;
	register struct slpque *qp;
	register s;

	rp = u.u_procp;
	s = splhigh();
	if (panicstr) {
		/*
		 * After a panic, just give interrupts a chance,
		 * then just return; don't run any other procs 
		 * or panic below, in case this is the idle process
		 * and already asleep.
		 * The splnet should be spl0 if the network was being used
		 * by the filesystem, but for now avoid network interrupts
		 * that might cause another panic.
		 */
		(void) splnet();
		splx(s);
		return;
	}
	if (chan==0 || rp->p_stat != SRUN || rp->p_rlink)
		panic("sleep");
	rp->p_wchan = chan;
	rp->p_slptime = 0;
	rp->p_pri = pri;
	qp = &slpque[HASH(chan)];
#if	MACH_VM
	simple_lock(&sleep_lock);
#endif	MACH_VM
	if (qp->sq_head == 0)
		qp->sq_head = rp;
	else
		*qp->sq_tailp = rp;
	*(qp->sq_tailp = &rp->p_link) = 0;
#if	MACH_VM
	simple_unlock(&sleep_lock);
#endif	MACH_VM
	if (pri > PZERO) {
		/*
		 * If we stop in issig(), wakeup may already have happened
		 * when we return (rp->p_wchan will then be 0).
		 */
		if (ISSIG(rp)) {
			if (rp->p_wchan)
				unsleep(rp);
			rp->p_stat = SRUN;
			(void) spl0();
			goto psig;
		}
		if (rp->p_wchan == 0)
			goto out;
		rp->p_stat = SSLEEP;
		(void) spl0();
		u.u_ru.ru_nvcsw++;
#if	MACH_MP
		unix_swtch((struct proc *) 0, 0);
#else	MACH_MP
		swtch();
#endif	MACH_MP
		if (ISSIG(rp))
			goto psig;
	} else {
		rp->p_stat = SSLEEP;
		(void) spl0();
		u.u_ru.ru_nvcsw++;
#if	MACH_MP
		unix_swtch((struct proc *) 0, 0);
#else	MACH_MP
		swtch();
#endif	MACH_MP
	}
	curpri = rp->p_usrpri;
out:
	splx(s);
	return;

	/*
	 * If priority was low (>PZERO) and
	 * there has been a signal, execute non-local goto through
	 * u.u_qsave, aborting the system call in progress (see trap.c)
#if	CS_COMPAT
	 * (or finishing a tsleep, see below)
#endif	CS_COMPAT
	 */
psig:
	longjmp(&u.u_qsave);
	/*NOTREACHED*/
}
#if	MACH_MP
sleep_and_unlock(chan, pri, lock)
	caddr_t chan;
	int pri;
	simple_lock_t	lock;
{
	register struct proc *rp;
	register struct slpque *qp;
	register s;

	rp = u.u_procp;
	s = splhigh();
	if (chan==0 || rp->p_stat != SRUN || rp->p_rlink)
		panic("sleep");
	rp->p_wchan = chan;
	rp->p_slptime = 0;
	rp->p_pri = pri;
	qp = &slpque[HASH(chan)];
#if	MACH_VM
	simple_lock(&sleep_lock);
#endif	MACH_VM
	if (qp->sq_head == 0)
		qp->sq_head = rp;
	else
		*qp->sq_tailp = rp;
	*(qp->sq_tailp = &rp->p_link) = 0;
#if	MACH_VM
	simple_unlock(&sleep_lock);
#endif	MACH_VM
	rp->p_stat = SSLEEP;
	(void) spl0();
	u.u_ru.ru_nvcsw++;
	simple_unlock(lock);
	unix_swtch((struct proc *) 0, 0);
	curpri = rp->p_usrpri;
	splx(s);
}
#endif	MACH_MP
#if	CS_RPAUSE

/*
 *  rpause - perform a resource pause
 *
 *  rsleep = function to perform resource specific sleep
 *  arg1   = first function parameter
 *  arg2   = second function parameter
 *  mesg1  = first component of user pause message
 *  mesg2  = second component of user pause message
 *
 *  Display the appropriate pause message on the user's controlling terminal.
 *  Save the current non-local goto information and establish a new return
 *  environment to transfer here.  Invoke the supplied function to sleep
 *  (possibly interruptably) until the resource becomes available.  When the
 *  sleep finishes (either normally or abnormally via a non-local goto caused
 *  by a signal), restore the old return environment and display a resume
 *  message on the terminal.
 *
 *  Return: true if the resource has now become available, or false if the wait
 *  was interrupted by a signal.
 */

bool
rpause(rsleep, arg1, arg2, mesg1, mesg2)
int (*rsleep)();
int arg1;
int arg2;
char *mesg1;
char *mesg2;
{
    label_t lsave;
    bool ret = true;

    uprintf("[%s: %s%s, pausing ...]\r\n", u.u_comm, mesg1, mesg2);
    
    bcopy((caddr_t)&u.u_qsave, (caddr_t)&lsave, sizeof(lsave));
    if (setjmp(&u.u_qsave) == 0)
	(*rsleep)(arg1, arg2);
    else
	ret = false;
    bcopy((caddr_t)&lsave, (caddr_t)&u.u_qsave, sizeof(lsave));

    uprintf("[%s: ... continuing]\r\n", u.u_comm);
    return(ret);
}
#endif	CS_RPAUSE
#if	CS_COMPAT



/*
 * Sleep on chan at pri.
 * Return in no more than the indicated number of seconds.
 * (If seconds==0, no timeout implied)
 * Return	TS_OK if chan was awakened normally
 *		TS_TIME if timeout occurred
 *		TS_SIG if asynchronous signal occurred
 */

tsleepexpire(p)
	register struct proc *p;
{
	p->p_flag &= ~STIMO;
	wakeup((caddr_t)&p->p_wchan);
}

tsleep(chan, pri, seconds)
	caddr_t chan;
{
	label_t lqsave;
	register struct proc *pp;
	register n, rval;
	struct timeval when;

	pp = u.u_procp;
	n = spl7();
	if (seconds)
	{
		when = time;
		when.tv_sec += seconds;
		pp->p_flag |= STIMO;
		timeout(tsleepexpire, (caddr_t)pp, hzto(&when));
	}
	bcopy((caddr_t)&u.u_qsave, (caddr_t)&lqsave, sizeof (label_t));
	if (setjmp(&u.u_qsave))
		rval = TS_SIG;
	else {
		sleep(chan, pri);
		if ((pp->p_flag&STIMO)==0 && seconds)
			rval = TS_TIME;
		else
			rval = TS_OK;
		untimeout(tsleepexpire, (caddr_t)pp);
	}
	pp->p_flag &= ~STIMO;
	bcopy((caddr_t)&lqsave, (caddr_t)&u.u_qsave, sizeof (label_t));
	splx(n);
	return(rval);
}
#endif	CS_COMPAT

/*
 * Remove a process from its wait queue
 */
unsleep(p)
	register struct proc *p;
{
	register struct slpque *qp;
	register struct proc **hp;
	int s;

	s = splhigh();
	if (p->p_wchan) {
#if	MACH_VM
		simple_lock(&sleep_lock);
#endif	MACH_VM
		hp = &(qp = &slpque[HASH(p->p_wchan)])->sq_head;
		while (*hp != p)
			hp = &(*hp)->p_link;
		*hp = p->p_link;
		if (qp->sq_tailp == &p->p_link)
			qp->sq_tailp = hp;
#if	MACH_VM
		simple_unlock(&sleep_lock);
#endif	MACH_VM
		p->p_wchan = 0;
	}
	splx(s);
}

/*
 * Wake up all processes sleeping on chan.
 */
wakeup(chan)
	register caddr_t chan;
{
	register struct slpque *qp;
	register struct proc *p, **q;
	int s;

	s = splhigh();
	qp = &slpque[HASH(chan)];
#if	MACH_VM
	simple_lock(&sleep_lock);
#endif	MACH_VM
restart:
	for (q = &qp->sq_head; p = *q; ) {
		if (p->p_rlink || p->p_stat != SSLEEP && p->p_stat != SSTOP)
			panic("wakeup");
		if (p->p_wchan==chan) {
			p->p_wchan = 0;
			*q = p->p_link;
			if (qp->sq_tailp == &p->p_link)
				qp->sq_tailp = q;
			if (p->p_stat == SSLEEP) {
				/* OPTIMIZED INLINE EXPANSION OF setrun(p) */
				if (p->p_slptime > 1)
					updatepri(p);
				p->p_slptime = 0;
				p->p_stat = SRUN;
#if	MACH_MP
				if (p->p_flag & SLOAD) {
					lock_write(&sched_lock);
					unix_setrq(p);
					lock_write_done(&sched_lock);
				}
#else	MACH_MP
				if (p->p_flag & SLOAD)
					setrq(p);
#endif	MACH_MP
				/*
				 * Since curpri is a usrpri,
				 * p->p_pri is always better than curpri.
				 */
				runrun++;
				aston();
				if ((p->p_flag&SLOAD) == 0) {
					if (runout != 0) {
						runout = 0;
						wakeup((caddr_t)&runout);
					}
					wantin++;
				}
				/* END INLINE EXPANSION */
				goto restart;
			}
			p->p_slptime = 0;
		} else
			q = &p->p_link;
	}
#if	MACH_VM
	simple_unlock(&sleep_lock);
#endif	MACH_VM
	splx(s);
}

/*
 * Initialize the (doubly-linked) run queues
 * to be empty.
 */
rqinit()
{
	register int i;

	for (i = 0; i < NQS; i++)
		qs[i].ph_link = qs[i].ph_rlink = (struct proc *)&qs[i];
#if	MACH_VM
	simple_lock_init(&sleep_lock);
#endif	MACH_VM
#if	MACH_MP
	wait_queue_init();
	lock_init(&sched_lock);
	simple_lock_init(&global_runq.lock);
	global_runq.low = 0;
	global_runq.count = 0;
	for (i = 0; i < NRQS; i++) {
		register int j;

		for (j = 0; j < NCPUS; j++) {
			simple_lock_init(&local_runq[j].lock);
			initqueue(&local_runq[j].runq[i]);
			local_runq[j].low = 0;
			local_runq[j].count = 0;
		}
		initqueue(&global_runq.runq[i]);
	}
#endif	MACH_MP
}

/*
 * Set the process running;
 * arrange for it to be swapped in if necessary.
 */
setrun(p)
	register struct proc *p;
{
	register int s;

	s = splhigh();
	switch (p->p_stat) {

	case 0:
	case SWAIT:
	case SRUN:
	case SZOMB:
	default:
		panic("setrun");

	case SSTOP:
	case SSLEEP:
		unsleep(p);		/* e.g. when sending signals */
		break;

	case SIDL:
		break;
	}
	if (p->p_slptime > 1)
		updatepri(p);
	p->p_stat = SRUN;
#if	MACH_MP
	if (p->p_flag & SLOAD) {
		lock_write(&sched_lock);
		unix_setrq(p);
		lock_write_done(&sched_lock);
	}
#else	MACH_MP
	if (p->p_flag & SLOAD)
		setrq(p);
#endif	MACH_MP
	splx(s);
	if (p->p_pri < curpri) {
		runrun++;
		aston();
	}
	if ((p->p_flag&SLOAD) == 0) {
		if (runout != 0) {
			runout = 0;
			wakeup((caddr_t)&runout);
		}
		wantin++;
	}
}

/*
 * Set user priority.
 * The rescheduling flag (runrun)
 * is set if the priority is better
 * than the currently running process.
 */
setpri(pp)
	register struct proc *pp;
{
	register int p;

	p = (pp->p_cpu & 0377)/4;
	p += PUSER + 2 * pp->p_nice;
	if (pp->p_rssize > pp->p_maxrss && freemem < desfree)
		p += 2*4;	/* effectively, nice(4) */
	if (p > 127)
		p = 127;
	if (p < curpri) {
		runrun++;
		aston();
	}
	pp->p_usrpri = p;
	return (p);
}

#if	MACH_MP

/*
 *	Some useful interface routines to make the scheduler transition
 *	smooth.
 */

unix_setrq(p)
	struct proc	*p;
{
	if (p->p_stat != SRUN) {
		printf("unix_setrq: p_stat != SRUN, p = 0x%x, pid = %d.\n", p,
				p->p_pid);
		panic("unix_setrq");
	}
	do_setrq(p, 1);		/* always put on master */
}

/*
 *	do_setrq sets the specified process on a run queue.  If "master"
 *	is TRUE, then the process goes onto the master's run queue, else it
 *	goes on the global run queue.
 */

do_setrq(p, master)
	struct proc	*p;
	int		master;
{
	register struct thread	*th;

#if	MACH_VM
	th = thread_table[p - proc];		/* find thread */
#else	MACH_VM
	th = &thread[p - proc];
#endif	MACH_VM
	th->pri = p->p_pri;
	if (th->whichq) {		/* bound to a cpu? */
		set_runq(th, th->whichq);
		return;
	}

	if (master) {
		set_runq(th, &local_runq[master_cpu]);
	}
	else {
		set_runq(th, &global_runq);
	}
}

unix_remrq(p)
	struct proc *p;
{
	register struct thread	*th;
	register struct run_queue	*rq;

#if	MACH_VM
	th = thread_table[p - proc];		/* find thread */
#else	MACH_VM
	th = &thread[p - proc];
#endif	MACH_VM
/*	th->pri = p->p_pri;*/
	rq = rem_runq(th);
	p->p_rlink = 0;		/* for firewall checks */
	if (rq)
		return(1);
	else
		return(0);
}

long slave_proc[NCPUS];

slave_start()
{
	register struct thread	*th;
	register int		mycpu, s;
	register struct proc	*p;
	extern int		should_exit[];

	/*	Find a thread to execute */

	set_cpu_number();
	mycpu = cpu_number();

	s = splhigh();
	lock_read(&sched_lock);
	th = choose_thread();
	if (th == NULL) {
		lock_read_done(&sched_lock);
		printf("Slave %d failed to find any threads.\n", mycpu);
		printf("Should have at least found idle thread.\n");
		halt_cpu();
	}

#if	MACH_VM
	active_threads[mycpu] = th;
#else	MACH_VM
	p = &proc[th - thread];
	p->p_rlink = 0;		/* for firewall checks */
	p->p_flag |= SACTIVE;
	slave_proc[mycpu] = (long) p;
#endif	MACH_VM
	lock_read_done(&sched_lock);
	splx(s);
#if	MACH_VM
	PMAP_ACTIVATE(vm_map_pmap(th->task->map), th, mycpu);
	load_context(th);
#else	MACH_VM
	load_context(p);
#endif	MACH_VM
	/*NOTREACHED*/
}

/*
 *	Perform a "unix" context switch.  If p is specified, then put
 *	it on a run queue (the master if master is TRUE).
 *	Find any other thread to execute (must at least find the
 *	idle thread for this cpu.
 */

unix_swtch(p, master)
	register struct proc	*p;
	register int		master;
{
	register struct thread	*th;
	register int		mycpu;
	extern			master_idle;

#if	MACH_VM
	th = thread_table[u.u_procp-proc];
	PMAP_DEACTIVATE(vm_map_pmap(th->task->map), th, cpu_number());
#endif	MACH_VM
	if (save_context()) {
		return;
	}

	/* save context leaves us at high ipl */

	lock_write(&sched_lock);
	if (p) {
		do_setrq(p, master);
		p->p_flag &= ~SACTIVE;
	}
	else {
		u.u_procp->p_flag &= ~SACTIVE;
	}
	lock_write_done(&sched_lock);

	mycpu = cpu_number();
	if (mycpu == master_cpu) {
		runrun = 0;
		noproc = 1;
		master_idle = 1;
	}
#if	MACH_VM
	active_threads[mycpu] = THREAD_NULL;
#endif	MACH_VM
	slave_proc[mycpu] = 0;

	if (should_exit[mycpu]) {
		printf("Slave processor %d halting...\n", mycpu);
		halt_cpu();
	}

	lock_read(&sched_lock);
	th = choose_thread();
	if (th == NULL) {
		printf("unix_swtch failed to find any processes.\n");
		printf("Should have found the idle process (at least).\n");
		lock_read_done(&sched_lock);
		panic("unix_swtch");
	}

	if (mycpu == master_cpu) {
		master_idle = 0;
		noproc = 0;
	}
#if	MACH_VM
	active_threads[mycpu] = th;
#else	MACH_VM
	p = &proc[th - thread];
	p->p_rlink = 0;		/* for firewall checks */
	p->p_flag |= SACTIVE;	/* flag as active */
	slave_proc[mycpu] = (long) p;
#endif	MACH_VM
	lock_read_done(&sched_lock);
#if	MACH_VM
	PMAP_ACTIVATE(vm_map_pmap(th->task->map), th, mycpu);
	load_context(th);
#else	MACH_VM
	load_context(p);
#endif	MACH_VM
	/*NOTREACHED*/
}

/*
 * Deactivate current thread for the purpose of waiting for an IPC event.
 * Find next thread to run and run it on the current CPU.  Hold the 
 * specified lock until we have safely saved out context.
 */
ipc_wait(ipc_lock)
	simple_lock_t		ipc_lock;
{
	register struct thread	*th;
	register int		mycpu;
	struct proc *		p;
	extern			master_idle;

#if	MACH_VM
	th = current_thread();
	PMAP_DEACTIVATE(vm_map_pmap(th->task->map), th, cpu_number());
#endif	MACH_VM

	if (save_context()) {
		return;
	}

	/* save context leaves us at high ipl */

	/* It is now save to unlock the ipc_lock */
	simple_unlock(ipc_lock);

	lock_write(&sched_lock);
	u.u_procp->p_flag &= ~SACTIVE;
	lock_write_done(&sched_lock);

	mycpu = cpu_number();
	if (mycpu == master_cpu) {
		runrun = 0;
		noproc = 1;
		master_idle = 1;
	}

#if	MACH_VM
	active_threads[mycpu] = THREAD_NULL;
#endif	MACH_VM

	slave_proc[mycpu] = 0;

	if (should_exit[mycpu]) {
		printf("Slave processor %d halting...\n", mycpu);
		halt_cpu();
	}

	lock_read(&sched_lock);

	th = choose_thread();
	if (th == NULL) {
		printf("ipc_swtch failed to find any processes.\n");
		printf("Should have found the idle process (at least).\n");
		lock_read_done(&sched_lock);
		panic("unix_swtch");
	}

	if (mycpu == master_cpu) {
		master_idle = 0;
		noproc = 0;
	}

#if	MACH_VM
	active_threads[mycpu] = th;
#else	MACH_VM
	p = &proc[th - thread];
	p->p_rlink = 0;		/* for firewall checks */
	p->p_flag |= SACTIVE;	/* flag as active */
	slave_proc[mycpu] = (long) p;
#endif	MACH_VM

	lock_read_done(&sched_lock);

#if	MACH_VM
	PMAP_ACTIVATE(vm_map_pmap(th->task->map), th, mycpu);
	load_context(th);
#else	MACH_VM
	load_context(p);
#endif	MACH_VM

	/*NOTREACHED*/
}

/*
 *	Perform a context switch to a thread waiting on an IPC event.  
 *   	Save the current context.
 *	Put current thread back on run queue, and switch to the
 *	context of the specified thread.
 */

ipc_wakeup(th,ipc_lock)
	register struct thread	*th;
	simple_lock_t		ipc_lock;
{
	register struct thread	*old_th;
	register struct proc	*p;
	register int		mycpu;

#if	MACH_VM
	old_th = current_thread();
	PMAP_DEACTIVATE(vm_map_pmap(old_th->task->map), old_th,
				cpu_number());
#endif	MACH_VM

	if (save_context()) {
		return;
	}

	/* It is safe now to release ipc_lock */
	simple_unlock(ipc_lock);

	/*
	 *	Put current thread back on run queue.
	 */

#if	MACH_VM
#else	MACH_VM
	old_th = &thread[u.u_procp - proc];
#endif	MACH_VM

	if (old_th != th) {
		lock_write(&sched_lock);
		if (old_th->whichq) {
			set_runq(old_th, old_th->whichq);
		}
		else {
			set_runq(old_th, &global_runq);
		}
		u.u_procp->p_flag &= ~SACTIVE;	/* XXX */
		lock_write_done(&sched_lock);
	}

#if	MACH_VM
	mycpu = cpu_number();
	active_threads[mycpu] = th;
#else	MACH_VM
	p = &proc[th - thread];
	p->p_rlink = 0;		/* for firewall checks */
	p->p_flag |= SACTIVE;	/* flag as active */
	mycpu = cpu_number();
	slave_proc[mycpu] = (long) p;
#endif	MACH_VM

#if	MACH_VM
	PMAP_ACTIVATE(vm_map_pmap(th->task->map), th, cpu_number());
	load_context(th);
#else	MACH_VM
	load_context(p);
#endif	MACH_VM
	/*NOTREACHED*/
}

unix_resume(p)
	register struct proc	*p;
{
#if	MACH_VM
	struct thread	*th;

	th = thread_table[p - proc];		/* find thread */
	PMAP_DEACTIVATE(vm_map_pmap(th->task->map), th, cpu_number());
#endif	MACH_VM
	if (save_context()) {
		return;
	}
#if	MACH_VM
	active_threads[cpu_number()] = th;
	PMAP_ACTIVATE(vm_map_pmap(th->task->map), th, cpu_number());
	load_context(th);
#else	MACH_VM
	u.u_procp->p_flag &= ~SACTIVE;
	p->p_flag |= SACTIVE;
	load_context(p);
#endif	MACH_VM
}

/*
 *	Perform a context switch.  Save the current context (probably the idle
 *	thread), put current thread back on run queue, and switch to the
 *	context of the specified thread.
 */

context_swtch(th)
	register struct thread	*th;
{
	register struct thread	*old_th;
	register struct proc	*p;
	register int		mycpu;

#if	MACH_VM
	old_th = current_thread();
	PMAP_DEACTIVATE(vm_map_pmap(old_th->task->map), old_th,
				cpu_number());
#endif	MACH_VM
	if (save_context()) {
		return;
	}

	/*
	 *	Put current thread back on run queue.
	 */

#if	MACH_VM
#else	MACH_VM
	old_th = &thread[u.u_procp - proc];
#endif	MACH_VM
	if (old_th != th) {
		lock_write(&sched_lock);
		if (old_th->whichq) {
			set_runq(old_th, old_th->whichq);
		}
		else {
			set_runq(old_th, &global_runq);
		}
		u.u_procp->p_flag &= ~SACTIVE;	/* XXX */
		lock_write_done(&sched_lock);
	}

#if	MACH_VM
	mycpu = cpu_number();
	active_threads[mycpu] = th;
#else	MACH_VM
	p = &proc[th - thread];
	p->p_rlink = 0;		/* for firewall checks */
	p->p_flag |= SACTIVE;	/* flag as active */
	mycpu = cpu_number();
	slave_proc[mycpu] = (long) p;
#endif	MACH_VM
#if	MACH_VM
	PMAP_ACTIVATE(vm_map_pmap(th->task->map), th, cpu_number());
	load_context(th);
#else	MACH_VM
	load_context(p);
#endif	MACH_VM
	/*NOTREACHED*/
}

/*
 *	This is the idle thread, which just looks for other threads
 *	to execute.
 */

idle_thread()
{
	register struct thread *th;
	register int mycpu;
	register int s;
	register run_queue_t lq;

	u.u_procp->p_pri = 127;
	u.u_procp->p_usrpri = 127;
	unix_swtch(u.u_procp, 0);		/* give up cpu */

	mycpu = cpu_number();			/* who am I? */
	lq = &local_runq[cpu_number()];

	while (1) {
		if (mycpu == master_cpu) {
			runrun = 0;
			noproc = 1;
			master_idle = 1;
		}

		/*	Find a thread to execute */

	 	(void) spl0();		/* idle loop, allow interrupts */
		do {
			if (should_exit[mycpu]) {
				unix_swtch(u.u_procp, 0); /* will exit */
			}
			while ((lq->count == 0) && (global_runq.count == 0))
				;

			s = splhigh();
			lock_read(&sched_lock);
			th = choose_thread();
			if (th == NULL) {
				lock_read_done(&sched_lock);
				splx(s);
			}
		} while (th == NULL);

		if (mycpu == master_cpu) {
			master_idle = 0;
			noproc = 0;
		}
		lock_read_done(&sched_lock);
		context_swtch(th);
		slave_proc[mycpu] = 0;
	}
}
#endif	MACH_MP
