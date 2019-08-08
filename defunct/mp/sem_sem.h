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
 *	File:	mp/sem_sem.h
 *	Author:	Avadis Tevanian, Jr.
 *
 *	Copyright (C) 1985, Avadis Tevanian, Jr.
 *
 *	Type definitions for kernel semaphores.
 *
 * HISTORY
 * 17-Mar-85  Avadis Tevanian (avie) at Carnegie-Mellon University
 *	Created.
 */

#ifndef	_SEM_
#define	_SEM_

#ifdef	KERNEL
#include "../sync/mp_queue.h"
#else	KERNEL
#include <sys/features.h>
#endif	KERNEL

/* these stuctures must have a size that is a multiple of 8 bytes */

struct sem_queue {
	struct	mpqueue sq_q;		/* queue information */
	struct	proc *sq_proc;		/* process waiting for it */
	caddr_t	sq_chan;		/* channel process is waiting on */
	u_short	processor;		/* processor we are executing on */
	short	sq_gone;		/* semaphore destroyed? */
	struct	semaphore *sq_sem;	/* semaphore we are waiting on */
};

struct semaphore {
	struct	mpqueue alloc_q;	/* link for free or inuse */
	struct	mpqueue wait_q;		/* wait  queue */
	struct	semaphore *has_p;	/* list of sem with P per proc */
};

#endif	_SEM_
