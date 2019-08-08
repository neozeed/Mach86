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
 *	File:	sem_user.c
 *	Author:	Avadis Tevanian, Jr.
 *
 *	User trap routines for semaphore access.
 *
 * HISTORY
 * 11-Nov-84  Avadis Tevanian (avie) at Carnegie-Mellon University
 *	Created.
 *
 *
 ************************************************************************
 */

#include "mach_mpm.h"

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/dir.h"
#include "../h/user.h"
#include "../h/proc.h"

#include "../machine/cpu.h"

#include "../accent/acc_errors.h"
#include "../sync/mp_queue.h"
#include "../mp/kern_smalloc.h"
#include "../mp/sem_sem.h"
#include "../mp/sem_error.h"
#include "../mp/mp_proc.h"
#include "../mp/remote_sw.h"
#include "../mp/remote_prim.h"

/*
 *	Trap routine for P operation.
 */

TPP(s)
register struct semaphore *s;
{
	return(P(s));
}

/*
 *	Trap routine for V operation.
 */

TPV(s)
register struct semaphore *s;
{
	return(V(s));
}

/*
 *	Trap routine for allocating a semaphore.
 */

TPallocate_semaphore(ss)
struct semaphore **ss;
{	register code;
	register i;
	register struct mp_proc *mpp = &mp_proc[u.u_procp-proc];

	/* find a slot for it in the users mp_proc structure */

	for (i = 0; i < NUM_SEMAPHORES; i++) {
		if (mpp->sem[i] == NULL)
			break;
	}

	if (i == NUM_SEMAPHORES)
		return(SEM_TOOMANY);

	code = allocate_semaphore(ss);
	if (code != Success)
		return(code);

	mpp->sem[i] = *ss;
	return(Success);
}

/*
 *	Trap routine for deallocating a semaphore.
 */

TPdeallocate_semaphore(s)
register struct semaphore *s;
{	register int i;
	register struct mp_proc *mpp = &mp_proc[u.u_procp-proc];

	/* look up the semaphore */

	for (i = 0; i < NUM_SEMAPHORES; i++) {
		if (mpp->sem[i] == s)
			break;
	}

	if (i == NUM_SEMAPHORES)
		return(SEM_INVALID);

	mpp->sem[i] = NULL;
	return(deallocate_semaphore(s));
}
