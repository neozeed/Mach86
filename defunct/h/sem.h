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
/*CMU:	sem.h	7.1	5/29/84	*/

/*
 *	File:	sem.h
 *	Author:	Avadis Tevanian, Jr.
 *
 *	Copyright 1984 (c) Avadis Tevanian, Jr.
 *
 *	Type definitions for semaphores.
 *
 * HISTORY
 * 20-Jan-84  Avadis Tevanian (avie) at Carnegie-Mellon University
 *	Created.
 *
 */

struct sem_queue {
	struct	rqueue sq_q;		/* queue information */
	struct	proc *sq_proc;		/* process waiting for it */
	caddr_t	sq_chan;		/* channel process is waiting on */
};

struct semaphore {
	struct	rqueue sem_q;		/* queue information */
};

struct semaphore *allocate_semaphore();
