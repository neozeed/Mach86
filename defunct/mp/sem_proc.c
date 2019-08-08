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
 *	File:	sem_proc.c
 *	Author:	Avadis Tevanian, Jr.
 *
 *	Maintanence functions for the semaphore entries in the mp proc table.
 *
 ************************************************************************
 * HISTORY
 * 11-Oct-85  Robert V Baron (rvb) at Carnegie-Mellon University
 *	header file changes -- no more mp/mp_shmem.h
 *
 * 17-Nov-84  Avadis Tevanian (avie) at Carnegie-Mellon University
 *	Known bugs:  In sem_exit(), the semaphores should be locked when
 *	doing the automatic V's.  This can be done with the _adawi hack.
 *
 *  1-Nov-84  Avadis Tevanian (avie) at Carnegie-Mellon University
 *	Created.
 *
 ************************************************************************
 */

#include "wb_ml.h"
#include "mach_mp.h"
#include "mach_mpm.h"

#include "../h/map.h"
#include "../h/param.h"
#include "../h/systm.h"
#include "../h/dir.h"
#include "../h/user.h"
#include "../h/proc.h"

#include "../machine/cpu.h"

#include "../sync/mp_queue.h"
#include "../mp/sem_sem.h"
#include "../mp/sem_error.h"
#include "../mp/remote_sw.h"
#include "../mp/remote_prim.h"
#include "../mp/mp_proc.h"


sem_exit()
{
	register struct mp_proc *mpp = &mp_proc[u.u_procp-proc];
	register int i;

	/* deallocate all semaphores we own */

	for (i = 0; i < NUM_SEMAPHORES; i++) {
		if (mpp->sem[i] != NULL) {
			deallocate_semaphore(mpp->sem[i]);
			mpp->sem[i] = NULL;
		}
	}

	/* for each semaphore we have a P on, do an automatic V */

	for (i = 0; i < NUM_SEMAPHORES; i++) {
		if (mpp->cur_sem[i] != NULL) {
			V(mpp->cur_sem[i]);
			mpp->cur_sem[i] = NULL;
		}
	}

	if (mpp->sq != NULL) {
		mpremqueue(&mpp->sq->sq_sem->wait_q, mpp->sq);
		freesq(mpp->sq);
		mpp->sq = NULL;
	}
}

sem_initp()
{	register struct mp_proc *mpp = &mp_proc[u.u_procp-proc];
	register int i;

	for (i = 0; i < NUM_SEMAPHORES; i++) {
		mpp->sem[i] = NULL;
		mpp->cur_sem[i] = NULL;
	}

	mpp->sq = NULL;
}
