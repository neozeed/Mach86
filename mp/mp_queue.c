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
 *	File:	mp_queue.c
 *	Author:	Avadis Tevanian, Jr.
 *
 *	Queue procedures that are fit to be used in a multiprocessor
 *	domain.  (All locking is taken care of).  All are machine
 *	independent.
 *
 *	Assumptions:
 *	1)	The underlying architecture/hardware support some type
 *		of semaphore access.  (Called via busyP and busyV).
 *
 *	2)	The queue headers and queue entries are all quad-word
 *		aligned.
 *
 * HISTORY
 * 19-Oct-84  Robert V Baron (rvb) at Carnegie-Mellon University
 *	rqeue => mpqueue, etc
 *
 *  3-Oct-84  Avadis Tevanian (avie) at Carnegie-Mellon University
 *	Created.
 *
 */

/*
 *	Depending on the target machine, some of these routines may
 *	be massaged to call assembly language equivalents when appropriate
 *	instructions are available.  For example, on a VAX, some of the
 *	queue instructions can be done much more efficiently using the
 *	fancy VAX interlocked queue instructions.  This package assumes
 *	that the target machine has at least a way to perform interlocked
 *	bit instructions.  The two assumed calls are bbssi, and bbcci, which
 *	branch on bit set and set, and branch on bit clear and clear.
 */

#include "../sync/lock.h"
#include "../sync/mp_queue.h"

/*
 *	mpenqueue_tail(queue, entry) inserts "entry" onto the tail of the "queue."
 *	If the queue was previously empty, it returns TRUE, else it returns
 *	FALSE.
 */

mpenqueue_tail(queue, entry)
	register struct mpqueue *queue;
	register struct mpqueue *entry;
{
	register struct mpqueue *last;
	register long tmpl;
	register int empty;

	LOCKQ(queue);			/* lock the queue */
	tmpl = (* (long *) queue) & ~(1);
	empty = (queue->mpq_prev == 0);
	last = (struct mpqueue *) (queue->mpq_prev + (char *)queue);
	last->mpq_next &= 1;		/* save the low order bit */
	last->mpq_next |= (char *)entry - (char *)last;
	queue->mpq_prev = (char *)entry - (char *)queue;
	entry->mpq_next = (char *)queue - (char *)entry;
	entry->mpq_prev = (char *)last - (char *)entry;
	UNLOCKQ(queue);			/* release the lock */
	return(empty);
}

/*
 *	mpenqueue_head(queue, entry) inserts "entry" onto the head of the "queue."
 *	If the queue was previously empty, it returns TRUE, else it returns
 *	FALSE.
 */

mpenqueue_head(queue, entry)
	register struct mpqueue *queue;
	register struct mpqueue *entry;
{
	register struct mpqueue *first;
	register long tmpl;
	register int empty;

	LOCKQ(queue);			/* lock the queue */
	tmpl = (* (long *) queue) & ~(1);
	* (long *) queue &= 1;		/* save the low order bit */
	empty = (queue->mpq_prev == 0);
	first = (struct mpqueue *) (tmpl + (char *)queue);
	first->mpq_prev = (char *)entry - (char *)first;
	queue->mpq_next |= (char *)entry - (char *)queue;
	entry->mpq_prev = (char *)queue - (char *)entry;
	entry->mpq_next = (char *)first - (char *)entry;
	UNLOCKQ(queue);			/* release the lock */
	return(empty);
}

/*
 *	mpdequeue_tail(queue, entry) removes an entry from the tail of the queue.
 *	If the queue was empty, NULL is returned in the entry.  If the entry
 *	removed was the last element in the queue, TRUE is returned, else
 *	FALSE is returned.
 */

mpdequeue_tail (queue, entry)
	register struct mpqueue *queue;
	register struct mpqueue **entry;
{
	register struct mpqueue *last;
	register long tmpl;
	register int empty;

	LOCKQ(queue);			/* lock the queue */
	tmpl = (* (long *) queue) & ~(1);
	if (queue->mpq_prev == 0) {
		*entry = 0;
		UNLOCKQ(queue);		/* release the lock */
		return(0);
	}
	*entry = (struct mpqueue *) (queue->mpq_prev + (char *)queue);
	last = (struct mpqueue *) ((*entry)->mpq_prev + (char *)*entry);
	last->mpq_next &= 1;		/* save the low order bit */
	last->mpq_next |= (char *)queue - (char *)last;
	queue->mpq_prev = (char *)last - (char *)queue;
	empty = (queue->mpq_prev == 0);
	UNLOCKQ(queue);			/* release the lock */
	return(empty);
}

/*
 *	mpdequeue_head(queue, entry) removes an entry from the head of the queue.
 *	If the queue was empty, NULL is returned in the entry.  If the entry
 *	removed was the last element in the queue, TRUE is returned, else
 *	FALSE is returned.
 */

mpdequeue_head (queue, entry)
	register struct mpqueue *queue;
	register struct mpqueue **entry;
{
	register struct mpqueue *first;
	register long tmpl;
	register int empty;

	LOCKQ(queue);			/* lock the queue */
	tmpl = (* (long *) queue) & ~(1);
	* (long *) queue &= 1;		/* save the low order bit */
	if (queue->mpq_prev == 0) {
		*entry = 0;
		UNLOCKQ(queue);		/* release the lock */
		return(0);
	}
	*entry = (struct mpqueue *) (tmpl + (char *)queue);
	first = (struct mpqueue *) ((*entry)->mpq_next + (char *)*entry);
	first->mpq_prev = (char *)queue - (char *)first;
	queue->mpq_next |= (char *)first - (char *)queue;
	empty = (queue->mpq_prev == 0);
	UNLOCKQ(queue);			/* release the lock */
	return(empty);
}

/*
 *	mpdequeue_elem(queue, entry) removes the specified entry from the
 *	specified queue.
 */

mpdequeue_elem(queue, entry)
	register struct mpqueue *queue;
	register struct mpqueue *entry;
{
	register struct mpqueue *next, *prev;
	register long tmpl;

	LOCKQ(queue);			/* lock the queue */
	tmpl = (* (long *) queue) & ~(1);
	next = (struct mpqueue *) (tmpl + (char *)entry);
	prev = (struct mpqueue *) (entry->mpq_prev + (char *)entry);
	prev->mpq_next &= 1;		/* save the low order bit */
	prev->mpq_next |= (char *)next - (char *)prev;
	next->mpq_prev = (char *)prev - (char *)next;
	UNLOCKQ(queue);
}
	
/*
 *	LOCKQ(queue) puts a lock on the specified queue.  Since it assumed
 *	that all queue entries (and headers) are quad-word aligned, we
 *	have 2 bits to play with in the header.  To lock the queue, we simply
 *	use the least significant bit as a semaphore.  In actuality, this
 *	is exactly what the VAX hardware does, so we can use this subroutine
 *	to lock out fancy VAX queue instructions.
 */

lockq(queue)
	register struct mpqueue *queue;
{
	BUSYP(queue, 0);		/* spin on the semaphore */
}

/*
 *	UNLOCKQ(queue) releases a lock on the specified queue.  See comments
 *	for LOCKQ for more information.
 */

unlockq(queue)
	register struct mpqueue *queue;
{
	BUSYV(queue, 0);		/* release it */
}
