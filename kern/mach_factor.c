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
 *	File:	kern/mach_factor.c
 *	Author:	Avadis Tevanian, Jr.
 *
 *	Copyright (C) 1986, Avadis Tevanian, Jr.
 *
 *	Compute the Mach Factor.
 *
 ************************************************************************
 * HISTORY
 *  3-Jun-86  Avadis Tevanian (avie) at Carnegie-Mellon University
 *	Created.
 *
 ************************************************************************
 */

#include "cpus.h"
#include "../h/sched.h"
#include "../h/machine.h"

/*
 *	hz and avenrun are declared globally here because don't want
 *	to figure out which 50 million other include files I'd need to
 *	include to pick them up properly.
 */
extern int hz;
extern double avenrun[];

static double fract[3] = {
	4.0/5.0,		/* 5 second average */
	59.0/60.0,		/* 1 minute average */
	299.0/300.0		/* 5 minute average */
};

compute_mach_factor()
{
	register int			i;
	register int			ncpus;
	register int			nprocs;
	register machine_slot_t		ms;
	double				thisave;
	static				initialized = FALSE;

	ncpus = 0;
	nprocs = global_runq.count;
	ms = machine_slot;
	for (i = 0; i < NCPUS; i++) {
		if (ms->running) {
			ncpus++;
			nprocs += local_runq[i].count;
		}
		ms++;
	}

	if (!initialized) {
		for (i = 0; i < 3; i++)
			avenrun[i] = ncpus;
		initialized = TRUE;
	}

	/*
	 *	Save our values in the Unix avenrun array.
	 */

	if (ncpus > nprocs)
		thisave = ncpus - nprocs;
	else
		thisave = (double)ncpus/(double)(nprocs+1);
	for (i = 0; i < 3; i++)
		avenrun[i] = (avenrun[i]*fract[i]) + (thisave*(1.0-fract[i]));

	/*
	 *	Reschedule ourselves in 1 second.
	 */
	timeout(compute_mach_factor, (caddr_t) 0, hz);
}
