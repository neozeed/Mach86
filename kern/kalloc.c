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
 *	File:	kern/kalloc.c
 *	Author:	Avadis Tevanian, Jr.
 *
 *	Copyright (C) 1985, Avadis Tevanian, Jr.
 *
 *	General kernel memory allocator.  This allocator is designed
 *	to be used by the kernel to manage dynamic memory fast.
 *
 * HISTORY
 * 21-Jun-85  Avadis Tevanian (avie) at Carnegie-Mellon University
 *	Created.
 */

#include "../h/types.h"
#include "../vm/vm_param.h"

#include "../h/zalloc.h"
#include "../h/kalloc.h"
#include "../vm/vm_object.h"

/*
 *	All allocations of size less than PGBYTES are rounded to the
 *	next highest power of 2.  This allocator is built on top of
 *	the zone allocator.  A zone is created for each potential size
 *	that we are willing to get in small blocks.
 */

struct zone	*k_zone[NQUEUES];

/*
 *	Initialize the memory allocator.  This should be called only
 *	once on a system wide basis (i.e. first processor to get here
 *	does the initialization).
 *
 *	This initializes all of the zones.
 */

kallocinit()
{
	register int	i;
	register int	size;

	i = 0;
	size = MINSIZE;

	/*
	 *	Allocate a zone for each size we are going to handle.
	 *	We specify non-paged memory.
	 */

	while (i < NQUEUES) {
		k_zone[i] = zinit(size, 10*PGBYTES, 2*PGBYTES, FALSE, "k-zone");
		size = size*2;
		i++;
	}
}

caddr_t kalloc(size)
{
	register zindex, allocsize;
	caddr_t addr;

	/* compute the size of the block that we will actually allocate */

	allocsize = size;
	if (size < PGBYTES) {
		allocsize = MINSIZE;
		zindex = 0;
		while (allocsize < size) {
			allocsize <<= 1;
			zindex++;
		}
	}

	/*
	 * If our size is still small enough, check the queue for that size
	 * and allocate.
	 */

	if (allocsize < PGBYTES) {
		addr = zalloc(k_zone[zindex]);
	}
	else {
		addr = (caddr_t) vmem_alloc(kernel_data, allocsize);
	}
	return(addr);
}

kfree(data, size)
	caddr_t	data;
	long	size;
{
	register freesize, zindex;

	freesize = size;
	if (size < PGBYTES) {
		freesize = MINSIZE;
		zindex = 0;
		while (freesize < size) {
			freesize <<= 1;
			zindex++;
		}
	}

	if (freesize < PGBYTES)
		zfree(k_zone[zindex], data);
	else vmem_free(kernel_data, data, freesize);
}
