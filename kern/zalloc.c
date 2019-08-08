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
 *	File:	kern/zalloc.c
 *	Author:	Avadis Tevanian, Jr.
 *
 *	Copyright (C) 1985, Avadis Tevanian, Jr.
 *
 *	Zone-based memory allocator.  A zone is a collection of fixed size
 *	data blocks for which quick allocation/deallocation is possible.
 *
 * HISTORY
 *  9-Jun-85  Avadis Tevanian (avie) at Carnegie-Mellon University
 *	Created.
 */

#include "../h/types.h"

#include "../sync/lock.h"
#include "../sync/mp_queue.h"
#include "../h/zalloc.h"
#include "../vm/vm_param.h"
#include "../vm/vm_kern.h"

/*
 *	zinit initializes a new zone.  The zone data structures themselves
 *	are stored in a zone, which is initially a static structure that
 *	is initialized by zone_init.
 */

struct zone *zone_zone;		/* this is the zone containing other zones */

extern caddr_t		zdata;
extern vm_size_t	zdata_size;

struct zone *zinit(size, max, alloc, pageable, name)
	vm_size_t	size;		/* the size of an element */
	vm_size_t	max;		/* maximum memory to use */
	vm_size_t	alloc;		/* allocation size */
	boolean_t	pageable;	/* is this zone pageable? */
	char		*name;		/* a name for the zone */
{
	register struct zone	*z;

	z = (struct zone *) zalloc(zone_zone);
	if (z == NULL)
		return(NULL);

	/*
	 *	Round off all the parameters appropriately.
	 */

	max = round_page(max);
	alloc = round_page(alloc);
	if (max < alloc) {
		max = alloc;		/* increase his max */
	}
	size = round_mpq(size);

	initmpqueue(&z->freeq);		/* init free list */
	z->cur_size = 0;
	z->max_size = max;
	z->elem_size = size;
	z->alloc_size = alloc;
	z->pageable = pageable;
	z->zone_name = name;
	simple_lock_init(&z->lock);

	/*
	 *	Flag that we haven't allocated memory
	 *	for this zone yet.  We can't actually allocate space
	 *	to prevent some boot time initialization problems,
	 *	so we defer till later (esp. zones are are crammed).
	 */

	z->base_addr = NULL;

	return(z);
}

/*
 *	Cram the given memory into the specified zone.  (This is currently
 *	only used for the kernel_entry_zone which is preallocated to prevent
 *	the problem of recursively calling zalloc.)
 */

zcram(zone, newmem, size)
	register struct zone	*zone;
	caddr_t			newmem;
	vm_size_t		size;
{
	caddr_t			old;
	vm_size_t		sizeleft;
	vm_size_t		elem_size;

	old = newmem;
	newmem = (caddr_t) round_mpq(old);
	sizeleft = size - (newmem - old);
	elem_size = zone->elem_size;
	simple_lock(&zone->lock);		/* lock this zone */

	/*
	 *	Break up the given memory and cram it into the
	 * 	zone.
	 */

	while (sizeleft >= elem_size) {
		mpenqueue_tail(&zone->freeq, newmem);
		sizeleft -= elem_size;
		newmem += elem_size;
	}

	zone->cur_size += zone->alloc_size;
	zone->base_addr = NULL;
	simple_unlock(&zone->lock);
}

/*
 *	Initialize the "zone of zones" which uses fixed memory allocated
 *	earlier in memory initialization.
 */

zone_init()
{
	register struct zone	*z;
	register int		size;
	register caddr_t	p;
	register int		sizeleft;

	size = round_mpq(sizeof(struct zone));

	zone_zone = (struct zone *) round_mpq(zdata);
	z = zone_zone;
	p = (caddr_t) z + size;
	p = (caddr_t) round_mpq(p);
	sizeleft = ((caddr_t) zdata - p) + zdata_size;

	initmpqueue(&z->freeq);
	z->elem_size = size;
	z->alloc_size = 0;
	z->pageable = FALSE;
	z->cur_size = sizeleft;
	z->max_size = sizeleft;
	z->zone_name = "zone of zones";
	z->base_addr = (vm_offset_t) -1;	/* catch bugs */
	simple_lock_init(&z->lock);

	while (sizeleft >= size) {
		mpenqueue_tail(&z->freeq, p);
		sizeleft -= size;
		p += size;
	}
}

/*
 *	zalloc returns a data block from the specified zone.
 */

caddr_t zalloc(zone)
	struct zone	*zone;
{
	caddr_t		addr;

	if (zone == NULL)
		panic ("zalloc: null zone");

	mpdequeue_head(&zone->freeq, &addr);

	/*
	 *	If there was nothing available, see if we can get more
	 *	memory.  This condition can arise simultaneously on
	 *	more than one cpu if they simultaneously ask for memory
	 *	when none is available... therefore the amount of
	 *	memory used by a zone may go over its limit, but this
	 *	is OK.  This could be fixed by larger grain synchronization.
	 */

	if (addr == NULL) {		/* get more memory if possible */
		z_more_pages(zone);
		mpdequeue_head(&zone->freeq, &addr);
	}

	/*
	 *	If we are over maximum, a NULL will be returned by
	 *	the second attempt to get the memory above.
	 */

	/*
	 *	XXX panic on NULL to catch possible memory allocation
	 *	bugs.
	 */

	if (addr == NULL) {
		printf("zone \"%s\" empty.\n", zone->zone_name);
		panic("zalloc");
	}

	return(addr);
}

z_more_pages(zone)
	register struct zone	*zone;
{
	register caddr_t	newmem;
	register vm_size_t	size;
	register vm_size_t	sizeleft;

	sizeleft = zone->alloc_size;
	size = zone->elem_size;

	simple_lock(&zone->lock);		/* lock this zone */
	if (zone->base_addr == NULL) {
		/*
		 *	Allocate pageable memory for it for now (this
		 *	will reserve address space for the zone).
		 *
		 *	This tries to be careful about zones which
		 *	were initially crammed.
		 */

		zone->base_addr = kmem_alloc_pageable(kernel_map,
					zone->max_size - zone->cur_size);
	}
	if (zone->cur_size + sizeleft > zone->max_size) {
		simple_unlock(&zone->lock);
		return;
	}

	newmem = (caddr_t)zone->base_addr;
	zone->base_addr += zone->alloc_size;
	zone->cur_size += zone->alloc_size;
	simple_unlock(&zone->lock);

	/*
	 *	Allocate the new memory and break it up into
	 *	small blocks.  We do this in an unlocked state
	 *	because kmem_alloc may need to wait for pages.
	 */

	if (!zone->pageable)
		vm_map_pageable(kernel_map, newmem, newmem + sizeleft, FALSE);
	while (sizeleft >= size) {
		mpenqueue_tail(&zone->freeq, newmem);
		sizeleft -= size;
		newmem += size;
	}
}

zfree(zone, elem)
	register struct zone	*zone;
	register caddr_t	elem;
{
	mpenqueue_tail(&zone->freeq, elem);
}
