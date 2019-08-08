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
/* $Header: sed.c,v 4.0 85/07/15 00:48:15 ibmacis GAMMA $ */
/* $Source: /ibm/acis/usr/sys_ca/ca/RCS/sed.c,v $ */

#if CMU
/*
 ***********************************************************************
 * HISTORY
 * 28-Feb-86  Bill Bolosky (bolosky) at Carnegie-Mellon University
 *	Added enqueue_tail and dequeue_head.
 *
 ***********************************************************************
 */
#endif CMU

#include "../ca/softint.h"

/* routines done by SED	*/

struct qhdr {
	struct qhdr *link, *rlink;
};

_insque(new, header)
	register struct qhdr *new, *header;
{
/*	new->link = header->link;
	new->rlink = header;
	header->link->rlink = new;
	header->link = new;
*/
	new->link = header;
	new->rlink = header->rlink;
	new->rlink->link = header->rlink = new;
}


_remque(elt)
	register struct qhdr *elt;
{
	elt->link->rlink = elt->rlink;
	elt->rlink->link = elt->link;
}


blkclr(dest, len)
	register char *dest;
	register int len;
{
	mfill(dest, len, 0);
}


setsoftclock()
{

	softlevel |= SOFT_CLOCK;
	asm("setsb r12,22-16");		  /* set bit 22 in scr 12 (ie set the bit in the
					     IRB */
	/* that requests a level 6 interrupt */
}


#ifdef INET
setsoftnet()
{

	softlevel |= SOFT_NET;
	asm("setsb r12,22-16");		  /* set bit 22 in scr 12 (ie set the bit in the
					     IRB */
	/* that requests a level 6 interrupt */
}


#endif INET

enqueue_tail(queue,elt)

{
 _insque(elt,queue);
}

struct qhdr *enqueue_head(queue,elt)
struct qhdr *queue, *elt;

{
	elt->link = queue->link;
	elt->rlink = queue;
	elt->link->rlink = queue->link = elt;
} 


struct qhdr *dequeue_head(queue)
struct qhdr *queue;

{
struct qhdr *elt;
 if (queue->link == queue)
	return(0);
 elt = queue->link;
 _remque(queue->link);
 return(elt);
}

initQueue(queue)
	register struct qhdr *queue;
{
	queue->link = queue;
	queue->rlink = queue;
}

enQueue(queue, elt)
{
	_insque(elt, queue);
}

struct qhdr *deQueue(queue)
register struct  qhdr *queue;
{
	if (queue->link == queue) return(0);
	return(dequeue_head(queue));
}

mpremqueue(queue,elt)
{
	mpdequeue_elem(queue,elt);
	return(elt);
}
