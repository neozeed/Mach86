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
 *	File:	sched.h
 *	Author:	Avadis Tevanian, Jr.
 *
 *	Copyright (C) 1985, Avadis Tevanian, Jr.
 *
 *	Header file for scheduler.
 *
 * HISTORY
 * 24-Aug-85  Avadis Tevanian (avie) at Carnegie-Mellon University
 *	Created.
 */

#ifndef	_SCHED_
#define	_SCHED_

#ifdef	KERNEL
#include "cpus.h"
#include "../h/queue.h"
#include "../sync/lock.h"
#else	KERNEL
#include <sys/features.h>
#include <sys/queue.h>
#include <sync/lock.h>
#endif	KERNEL

#define NRQS	32			/* 32 run queues per cpu */

struct run_queue {
	queue_head_t		runq[NRQS];	/* one for each priority */
	simple_lock_data_t	lock;		/* one lock for all queues */
	int			low;		/* low queue value */
	int			count;		/* count of threads runable */
};

typedef struct run_queue	*run_queue_t;
#define	RUN_QUEUE_NULL	((run_queue_t) 0)

struct run_queue	global_runq;
struct run_queue	local_runq[NCPUS];

struct lock	sched_lock;		/* scheduler lock */

/*
 *	Scheduler routines.
 */

struct run_queue	*rem_runq();
struct thread		*choose_thread();

#endif	_SCHED_
