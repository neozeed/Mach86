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
 *	File:	slave.c
 *
 *	Copyright (C) 1985, Avadis Tevanian, Jr.
 *
 *	Misc. slave routines.
 *
 * HISTORY
 * 20-May-86  Avadis Tevanian (avie) at Carnegie-Mellon University
 *	Check for current_thread being non-null when deciding whether or
 *	not to reschedule.
 *
 * 28-Feb-86  Bill Bolosky (bolosky) at Carnegie-Mellon University
 *	Conditionalized use of mtpr to be off when ROMP is on.  Made
 *	include of ../vax/{cpu,clock}.h into ../machine/{cpu,clock}.h.
 *
 *  8-Apr-85  Avadis Tevanian (avie) at Carnegie-Mellon University
 *	Created.
 *
 */

#include "cpus.h"
#include "mach_mp.h"

#include "../machine/reg.h"
#ifndef	romp
#include "../machine/psl.h"
#else	romp
#include "../machine/scr.h"
#endif	romp

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/dk.h"
#include "../h/map.h"
#include "../h/dir.h"
#include "../h/user.h"
#include "../h/kernel.h"
#include "../h/proc.h"
#include "../h/vm.h"
#include "../h/text.h"
#include "../machine/cpu.h"
#include "../machine/clock.h"
#ifdef	vax
#include "../vax/mtpr.h"
#endif 	vax

#include "../h/thread.h"
#include "../h/machine.h"

/*
 *	XXX - 4.3 turned this into a macro but didn't put it in a header
 *	file... it is also defined in ../sys/kern_clock.c
 */
#define BUMPTIME(t, usec) { \
	register struct timeval *tp = (t); \
 \
	tp->tv_usec += (usec); \
	if (tp->tv_usec >= 1000000) { \
		tp->tv_usec -= 1000000; \
		tp->tv_sec++; \
	} \
}

slave_main()
{
	register int i;
	extern long masterpaddr;
	extern int nummpm;
	struct proc *p;
	short pid, uid;

	slave_config();
	should_exit[cpu_number()] = 0;
	machine_slot[cpu_number()].running = TRUE;
	printf("Slave started at slot %d.\n", cpu_number());
	printf("Master cpu at slot %d.\n", master_cpu);
	printf("Hit <E> to exit slave.\n");
	slave_cnenable();

	startrtclock();

	slave_start();
}

slave_hardclock(pc, ps)
	caddr_t	pc;
	int	ps;
{
	register struct proc *p;
	register int s, cpstate;
	extern long slave_proc[];

	/*
	 * Charge the time out based on the mode the cpu is in.
	 * Here again we fudge for the lack of proper interval timers
	 * assuming that the current state has been around at least
	 * one tick.
	 */
	if (USERMODE(ps)) {
		/*
		 * CPU was in user state.  Increment
		 * user time counter, and process process-virtual time
		 * interval timer. 
		 */
		BUMPTIME(&u.u_ru.ru_utime, tick);
		if (timerisset(&u.u_timer[ITIMER_VIRTUAL].it_value) &&
		    itimerdecr(&u.u_timer[ITIMER_VIRTUAL], tick) == 0)
			psignal(u.u_procp, SIGVTALRM);
		if (u.u_procp->p_nice > NZERO)
			cpstate = CP_NICE;
		else
			cpstate = CP_USER;
	} else {
		/*
		 * CPU was in system state.  If profiling kernel
		 * increment a counter.  If no process is running
		 * then this is a system tick if we were running
		 * at a non-zero IPL (in a driver).  If a process is running,
		 * then we charge it with system time even if we were
		 * at a non-zero IPL, since the system often runs
		 * this way during processing of system calls.
		 * This is approximate, but the lack of true interval
		 * timers makes doing anything else difficult.
		 */
		cpstate = CP_SYS;
		if (!slave_proc[cpu_number()]) {
			if (BASEPRI(ps))
				cpstate = CP_IDLE;
		}
		BUMPTIME(&u.u_ru.ru_stime, tick);
	}

	/*
	 * If the cpu is currently scheduled to a process, then
	 * charge it with resource utilization for a tick, updating
	 * statistics which run in (user+system) virtual time,
	 * such as the cpu time limit and profiling timers.
	 * This assumes that the current process has been running
	 * the entire last tick.
	 */
	if (slave_proc[cpu_number()]) {
		if ((u.u_ru.ru_utime.tv_sec+u.u_ru.ru_stime.tv_sec+1) >
		    u.u_rlimit[RLIMIT_CPU].rlim_cur) {
			psignal(u.u_procp, SIGXCPU);
			if (u.u_rlimit[RLIMIT_CPU].rlim_cur <
			    u.u_rlimit[RLIMIT_CPU].rlim_max)
				u.u_rlimit[RLIMIT_CPU].rlim_cur += 5;
		}
		if (timerisset(&u.u_timer[ITIMER_PROF].it_value) &&
		    itimerdecr(&u.u_timer[ITIMER_PROF], tick) == 0)
			psignal(u.u_procp, SIGPROF);
		s = u.u_procp->p_rssize;
		u.u_ru.ru_idrss += s; u.u_ru.ru_isrss += 0;	/* XXX */
		if (u.u_procp->p_textp) {
			register int xrss = u.u_procp->p_textp->x_rssize;

			s += xrss;
			u.u_ru.ru_ixrss += xrss;
		}
		if (s > u.u_ru.ru_maxrss)
			u.u_ru.ru_maxrss = s;
	}
	/*
	 * We adjust the priority of the current process.
	 * The priority of a process gets worse as it accumulates
	 * CPU time.  The cpu usage estimator (p_cpu) is increased here
	 * and the formula for computing priorities (in kern_synch.c)
	 * will compute a different value each time the p_cpu increases
	 * by 4.  The cpu usage estimator ramps up quite quickly when
	 * the process is running (linearly), and decays away exponentially,
	 * at a rate which is proportionally slower when the system is
	 * busy.  The basic principal is that the system will 90% forget
	 * that a process used a lot of CPU time in 5*loadav seconds.
	 * This causes the system to favor processes which haven't run
	 * much recently, and to round-robin among other processes.
	 */
	p = u.u_procp;
	p->p_cpticks++;
	if (slave_proc[cpu_number()]) {
		if (++p->p_cpu == 0)
			p->p_cpu--;
		if ((p->p_cpu&3) == 0) {
			(void) setpri(p);
			if (p->p_pri >= PUSER)
				p->p_pri = p->p_usrpri;
		}
	}

	cp_time[cpstate]++;
	tick_count[cpu_number()]++;
	if ((current_thread() != THREAD_NULL) && ((tick_count[cpu_number()] % 10) == 0))
		aston();			/* cause reschedule */
}
