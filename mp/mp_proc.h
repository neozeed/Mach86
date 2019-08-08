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
 *	File:	mp_proc.h
 *	Author:	Avadis Tevanian, Jr.
 *
 *	One mp_proc structure is allocated per process.  It contains all
 *	the multiprocessor related data for the process while it may be
 *	swapped out.  Other per process multiprocessor data (mp_area.h)
 *	is swapped with the process.
 *
 *
 **************************************************************************
 * HISTORY
 * 24-Aug-85  Avadis Tevanian (avie) at Carnegie-Mellon University
 *	Allow for recursive includes and automatically include sempahore
 *	header file.
 *
 * 31-Oct-84  Avadis Tevanian (avie) at Carnegie-Mellon University
 *	Created.
 *
 **************************************************************************
 */

#ifndef	_MP_PROC_
#define	_MP_PROC_

#ifdef	KERNEL
#include "../mp/sem_sem.h"
#include "../mp/mp_param.h"
#else	KERNEL
#include <sys/features.h>
#include <mp/sem_sem.h>
#include <mp/mp_param.h>
#endif	KERNEL

struct	mp_proc {
	struct	semaphore *sem[NUM_SEMAPHORES];		/* semaphores owned */
	struct  semaphore *cur_sem[NUM_SEMAPHORES];	/* semaphore in use */
	struct	sem_queue *sq;				/* wait structs */
};

struct	mp_proc *mp_proc;	/* the mp_proc table itself */

#endif	_MP_PROC_
