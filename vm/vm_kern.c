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
 *	File:	vm/vm_kern.c
 *	Author:	Avadis Tevanian, Jr., Michael Wayne Young
 *
 *	Copyright (C) 1985, Avadis Tevanian, Jr., Michael Wayne Young
 *
 *	Kernel memory management.
 *
 * HISTORY
 * 04-Oct-85  Michael Wayne Young (mwyoung) at Carnegie-Mellon University
 *	Relocated from vm_user.c.
 */

#include "../vm/vm_kern.h"
#include "../vm/vm_map.h"
#include "../vm/vm_page.h"

#include "../h/param.h"			/* for PSWP */

/*
 *	kmem_init:
 *
 *	Initialize the kernel's virtual memory map, taking
 *	into account all memory allocated up to this time.
 */
void kmem_init(start, end)
	vm_offset_t	start;
	vm_offset_t	end;
{
	vm_offset_t	addr;
	extern vm_map_t	kernel_map;

	kernel_map = vm_map_create(pmap_kernel(), VM_MIN_KERNEL_ADDRESS, end,
				FALSE);

	addr = VM_MIN_KERNEL_ADDRESS;
	(void) vm_map_find(kernel_map, vm_object_null(), (vm_offset_t) 0,
				&addr, (start - VM_MIN_KERNEL_ADDRESS),
				FALSE);
}

/*
 *	kmem_terminate:		[ debug ]
 *
 *	Terminate the kernel's virtual memory map.
 */
void kmem_terminate()
{
	vm_map_deallocate(kernel_map);
}

/*
 *	kmem_alloc_pageable:
 *
 *	Allocate pageable memory to the kernel's address map.
 *	map must be "kernel_map" below.
 */

vm_offset_t kmem_alloc_pageable(map, size)
	vm_map_t		map;
	register vm_size_t	size;
{
	vm_offset_t		addr;
	register vm_object_t	object;
	register kern_return_t	result;
	vm_object_t	vm_kobject_allocate();

	if (map != kernel_map)
		panic("kmem_alloc_pageable: not called with kernel_map");

	size = round_page(size);
	object = vm_kobject_allocate(size);

	addr = vm_map_min(map);
	result = vm_map_find(map, object, (vm_offset_t) 0, &addr, size, TRUE);
	if (result != KERN_SUCCESS) {
		vm_object_deallocate(object);
		return(NULL);
	}
	return(addr);
}

vm_offset_t kmem_alloc(map, size, wait)
	vm_map_t	map;
	vm_size_t	size;
	boolean_t	wait;
{
	vm_object_t	object;
	vm_offset_t	addr;
	kern_return_t	result;
	int		npgs, s;
	vm_page_t	m;
	vm_offset_t	vaddr, offset;
	vm_object_t	vm_kobject_allocate();

	size = round_page(size);
	if (map == kernel_map)		/* zone_map perhaps? */
		object = vm_kobject_allocate(size);
	else
		object = vm_object_allocate(size);

	addr = vm_map_min(map);
	result = vm_map_find(map, object, (vm_offset_t) 0, &addr, size, TRUE);
	if (result != KERN_SUCCESS) {
		vm_object_deallocate(object);
		return(NULL);
	}

	npgs = atop(size);

	s = splvm();
	vm_page_lock();

	/*
	 *	Since we may not have enough memory, and we may not be able
	 *	to block, we first allocate all the memory up front, pulling
	 *	it off the active queue to prevent pageout.  We then can
	 *	use the standard routines to either "fault" in the wired pages
	 *	or to free whatever we tried to get (but failed).
	 */

	offset = 0;
	while (npgs) {
		m = vm_page_alloc(object, offset);
		if (m == VM_PAGE_NULL) {
			/*
			 *	Not enough free pages.  See if we can
			 *	wait or not.
			 */
			if (wait) {
				sleep_and_unlock(&vm_page_free_count, PSWP+2,
					&vm_page_system_lock);
				vm_page_lock();
				continue;
			}
			else {
				/*
				 *	Not enough pages, and we can't
				 *	wait, so free everything up.
				 */
				vm_page_unlock();
				splx(s);
				(void) vm_map_remove(map, trunc_page(addr),
					round_page(addr + size));
				return(NULL);
			}
		}

		/*
		 *	We want zero-filled memory
		 */

		vm_page_zero_fill(m);

		/*
		 *	Since no other process can see these pages, we don't
		 *	have to bother with the busy bit.
		 */

		m->busy = FALSE;

		npgs--;
		offset += page_size;
	}

	/*
	 *	And finally, mark the data as non-pageable.
	 */

	vm_page_unlock();
	splx(s);

	vm_map_pageable(map, (vm_offset_t) addr, addr + size, FALSE);

	return(addr);
}

/*
 *	kmem_free:
 *
 *	Release a region of kernel virtual memory allocated
 *	with kmem_alloc, and return the physical pages
 *	associated with that region.
 */
void kmem_free(map, addr, size)
	vm_map_t		map;
	register caddr_t	addr;
	vm_size_t		size;
{
	(void) vm_map_remove(map, trunc_page(addr), round_page(addr + size));
}

/*
 *	kmem_suballoc:
 *
 *	Allocates a map to manage a subrange
 *	of the kernel virtual address space.
 *
 *	Arguments are as follows:
 *
 *	parent		Map to take range from
 *	size		Size of range to find
 *	min, max	Returned endpoints of map
 *	pageable	Can the region be paged
 */
vm_map_t kmem_suballoc(parent, min, max, size, pageable)
	vm_map_t	parent;
	vm_offset_t	*min, *max;
	vm_size_t	size;
	boolean_t	pageable;
{
	kern_return_t	ret;

	size = round_page(size);

	*min = (vm_offset_t) vm_map_min(parent);
	ret = vm_map_find(parent, vm_object_allocate(size), (vm_offset_t) 0,
				min, size, TRUE);
	if (ret != KERN_SUCCESS) {
		printf("kmem_suballoc: bad status return of %d.\n", ret);
		panic("kmem_suballoc");
	}
	*max = *min + size;
	pmap_reference(vm_map_pmap(parent));
	return(vm_map_create(vm_map_pmap(parent), *min, *max, pageable));
}
