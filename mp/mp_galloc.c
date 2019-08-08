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
/* ************************************************************************ *\
 *									    *
 *									    *
 * File:	mp_galloc.c						    *
 *									    *
 * Abstract:								    *
 *	A heap package to allocate space in shared memory for strs, et all  *
 *	This code is not very general.  It uses once fixed heap that lives
 *	in the shm_stat structure, it does not get more space, BUT it may
 *	be used as soon as shm_stat is initialized, viz. in the mach_stat
 *	initialization.
 *									    *
 * Author:	Robert V. Baron						    *
 *		Copyright (c) 1984 by Robert V. Baron			    *
 *									    *
 * History:
 * 11-Oct-85  Robert V Baron (rvb) at Carnegie-Mellon University
 *	added mpq_allochead_init -- check it out and modified the code for
 *	the new shared memory scheme.
 *
 * 19-Mar-85  Robert V Baron (rvb) at Carnegie-Mellon University
 *	call shmprotect on space borrowed on shmborrowed for q_header q
 *
 * 22-Feb-85  Robert V Baron (rvb) at Carnegie-Mellon University
 *	Created 
\* ************************************************************************ */

#include "mach_vm.h"
#if	MACH_VM
#else	MACH_VM
#include "../h/map.h"
#include "../mp/shmem.h"
#include "../sync/mp_queue.h"

/* ************************************************************************ *
 *		other shared space allocation routines:
 *			Queue headers
 * ************************************************************************ */
struct mpqueue *QhdQ = (struct mpqueue *) 0;

mpqhead_t *
mpq_allochead()
{
	register mpqhead_t *pg;
	register int i;
	mpqhead_t *NewQ = (mpqhead_t *) 0;

	if (mppop(QhdQ, &NewQ), NewQ) {
		initmpqueue(NewQ);
		return NewQ;
	}

	/*
	 * No cells available.  get one!!
	 */
	if (!(pg = (mpqhead_t *) shmustalloc(SHMUNIT))) {
		panic ("headqueue");
	}

	for (i = 0; i < SHMUNIT/sizeof (mpqhead_t); i++) {
		mpenqueue(QhdQ, pg++);
	}
	/*
	 * now get an entry
	 */
	mppop(QhdQ, &NewQ);
	initmpqueue(NewQ);
	return NewQ;

}

mpq_allochead_init()
{
	register mpqhead_t *pg;
	register int i;

	/*
	 * No cells available.  get one!!
	 */
	if (!(pg = (mpqhead_t *) shmustalloc(SHMUNIT))) {
		panic ("headqueue_init");
	}

	QhdQ = pg;
	for (i = 1; i < SHMUNIT/sizeof (mpqhead_t); i++) {
		mpenqueue(QhdQ, ++pg);
	}

}

mpq_freehead(Q)
mpqhead_t *Q;
{
	mppush(QhdQ, Q);
}

#endif	MACH_VM
