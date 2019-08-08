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
#if	0
/*
 *	File:	kern_smalloc.c
 *	Author:	Avadis Tevanian, Jr.
 *
 *	Kernel memory allocator.  Designed to be used by the kernel to
 *	manage dynamic memory.
 *
 * HISTORY
 * 19-Oct-84  Robert V Baron (rvb) at Carnegie-Mellon University
 *	rqueue => mpqueue
 *
 * 30-Jan-84  Avadis Tevanian (avie) at Carnegie-Mellon University
 *	Created.
 *
 */

#include "../h/param.h"
#include "../h/systm.h"
#include "../sync/mp_queue.h"
#include "../mp/kern_smalloc.h"

/*
 *	All allocations of size less than CLSIZE*NBPG are rounded to the
 *	next highest power of 2.  For each such power of 2, a queue of free
 *	blocks is maintained.  If the queue is empty, non-paged pages are
 *	requested and split up into blocks.  Once such a page is allocated
 *	it will never be returned.  Thus, the total memory requirements at
 *	any particular time reflects the maximum memory requirements that
 *	has been experienced.
 */

struct mpqueue *freeq;			/* queues of free blocks */

/*
 *	Initialize the memory allocator.
 */

smallocinit()
{	register int i = 0;
	register spaceneeded;

	/* allocate space for the free list queues.  */

	spaceneeded = NQUEUES*sizeof(struct mpqueue);

	/* we've got to make sure we allocate at least one page */

	if (spaceneeded < CLSIZE*NBPG)
		spaceneeded = CLSIZE*NBPG;

	freeq = (struct mpqueue *) shmalloc(spaceneeded);
	while (i < NQUEUES) {
		initmpqueue(&freeq[i]);
		i++;
	}
}

caddr_t smalloc(size)
{	static firstcall = 1;
	register qindex, allocsize;
	caddr_t addr;

	if (firstcall) {
		smallocinit();
		firstcall = 0;
	}

	/* compute the size of the block that we will actually allocate */

	allocsize = size;
	if (size < CLSIZE*NBPG) {
		allocsize = MINSIZE;
		qindex = 0;
		while (allocsize < size) {
			allocsize <<= 1;
			qindex++;
		}
	}

	/* if our size is still small enough, check the queue for that size
	   and allocate if possible */

	if (allocsize < CLSIZE*NBPG) {
		if (mpqueueempty(&freeq[qindex]))
			smnewpage(allocsize, qindex);
		mpdequeue_head(&freeq[qindex], &addr);
	}
	else
		addr = (caddr_t) shmalloc(allocsize);
	return(addr);
}

smnewpage(size,qindex)
{	register caddr_t newmem;
	register sizeleft = CLSIZE*NBPG;
	register struct mpqueue *q = &freeq[qindex];

	newmem = (caddr_t ) shmalloc(sizeleft);
	while (sizeleft > 0) {
		mpenqueue_tail(q, newmem);
		sizeleft -= size;
		newmem += size;
	}
}

smfree(data,size)
{	register freesize, qindex;

	freesize = size;
	if (size < CLSIZE*NBPG) {
		freesize = MINSIZE;
		qindex = 0;
		while (freesize < size) {
			freesize <<= 1;
			qindex++;
		}
	}

	if (freesize < CLSIZE*NBPG)
		mpenqueue_tail(&freeq[qindex], data);
	else shmfree(data, freesize);
}
#endif	0
