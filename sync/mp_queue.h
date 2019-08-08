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
 *	File:	sync/mp_queue.h
 *	Author:	Avadis Tevanian, Jr.
 *
 *	Type definitions for multiprocessor queues.
 *
 * HISTORY
 * 11-Oct-85  Robert V Baron (rvb) at Carnegie-Mellon University
 *	Added extern's for QhdQ and mpq_allochead();
 *
 * 19-Aug-85  Avadis Tevanian (avie) at Carnegie-Mellon University
 *	Fixed bug in mpqueueempty.  Needed to have it's check masked
 *	with 07 to ignore the low bits (used for locking).
 *
 * 17-Mar-85  Avadis Tevanian (avie) at Carnegie-Mellon University
 *	Created.
 */

#ifndef	_MP_QUEUE_
#define	_MP_QUEUE_

#include "../sync/lock.h"

typedef unsigned long queuelink_t;

struct mpqueue {
	queuelink_t	mpq_next;	/* forward link for queue (offset) */
	queuelink_t	mpq_prev;	/* back link for queue (offset) */
};

typedef struct mpqueue	mpqhead_t;

#define round_mpq(size) ( (((unsigned)(size))+7) & (~7))

#define mpqcast(qc) ((struct mpqueue *) qc)
#define initmpqueue(q) ((mpqcast(q))->mpq_next = (mpqcast(q))->mpq_prev = 0)
#define mpqueueempty(q) (((mpqcast(q))->mpq_next & (~0x7)) == 0)

#define mpqnext(que, type) ((type) ( ((char *) que) + (mpqcast(que)->mpq_next & (~0x7))))
#define mpqtop(x, y) mpqnext(x, y)
#define mpqprev(que, type) ((type) ( ((char *) que) + (mpqcast(que)->mpq_prev & (~0x7))))

#define mpenqueue(head, elt) mpenqueue_tail(head, elt)
#define mpdequeue(head, elt) mpdequeue_head(head, elt)

#define mppush(stack, elt) mpenqueue_head(stack, elt)
#define mppop(stack, elt) mpdequeue_head(stack, elt)

#ifdef 	romp
#define non_blockingP(place) _bit_set(0, &place)
#define non_blockingV(place) _bit_clear(0, &place)
#else 	romp
#define non_blockingP(place) mpset_bit(0, &place)
#define non_blockingV(place) mpclear_bit(0, &place)
#endif 	romp

#define LOCKQ(queue) BUSYP(queue, 0)
#define UNLOCKQ(queue) BUSYV(queue, 0)

#ifdef KERNEL
extern struct mpqueue *QhdQ;
extern struct mpqueue *mpq_allochead();
#endif KERNEL

#endif	_MP_QUEUE_
