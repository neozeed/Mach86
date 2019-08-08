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
 *	File:	vm/vm_user.c
 *	Author:	Avadis Tevanian, Jr., Michael Wayne Young
 *
 *	Copyright (C) 1985, Avadis Tevanian, Jr., Michael Wayne Young
 *
 *	User-exported virtual memory functions.
 *
 * HISTORY
 *  1-Jun-86  Avadis Tevanian (avie) at Carnegie-Mellon University
 *	Changed vm_allocate to not automatically allocate an object.
 *	The VM code will generate a new object at fault time, if
 *	necessary.  However, hopefully the map code will just attach the
 *	new space onto the end of some pre-existing object.
 *
 * 30-Apr-86  Avadis Tevanian (avie) at Carnegie-Mellon University
 *	Added object cache functions.
 *
 * 18-Jun-85  Avadis Tevanian (avie) at Carnegie-Mellon University
 *	Created.
 */

#include "../h/types.h"

#include "../vm/vm_param.h"
#include "../vm/vm_object.h"
#include "../vm/vm_map.h"
#include "../vm/vm_page.h"
#include "../vm/vm_statistics.h"

#include "../h/kern_return.h"
#include "../h/task.h"

#include "../kern/mach_if.h"

/*
 *	vm_allocate allocates virtual memory in the specified address
 *	map.  Newly allocated memory is not yet validated, but will be
 *	when page faults occur.
 */

kern_return_t vm_allocate_with_pager(map, addr, size, find_space, pager, paging_space,
		pager_offset)
	register vm_map_t	map;
	register vm_offset_t	*addr;
	register vm_size_t	size;
	boolean_t		find_space;
	vm_pager_t		pager;
	vm_pager_id_t		paging_space;
	vm_offset_t		pager_offset;
{
	register vm_object_t	object;
	register kern_return_t	result;

	if (map == VM_MAP_NULL)
		return(KERN_INVALID_ARGUMENT);

	*addr = trunc_page(*addr);
	size = round_page(size);

	/*
	 *	Lookup the pager/paging-space in the object cache.
	 *	If it's not there, then create a new object and cache
	 *	it.
	 */
	object = vm_object_lookup(pager, paging_space);
	if (object == VM_OBJECT_NULL) {
		object = vm_object_allocate(size);
		vm_object_cache(object, pager, paging_space);
	}
	else
		vm_object_reference(object);

	if ((result = vm_map_find(map, object, pager_offset, addr, size,
				find_space)) != KERN_SUCCESS)
		vm_object_deallocate(object);
	else if (pager != vm_pager_null) {
		/*
		 *	Assume external pagers are read-only
		 */
		vm_object_setpager(object, pager, paging_space,
					(vm_offset_t) 0, TRUE);
	}

	return(result);
}

/*
 *	vm_allocate allocates "zero fill" memory in the specfied
 *	map.
 */
kern_return_t vm_allocate(map, addr, size, anywhere)
	register vm_map_t	map;
	register vm_offset_t	*addr;
	register vm_size_t	size;
	boolean_t		anywhere;
{
	kern_return_t	result;
	vm_object_t	object;

	if (map == VM_MAP_NULL)
		return(KERN_INVALID_ARGUMENT);
	if (size == 0) {
		*addr = 0;
		return(KERN_SUCCESS);
	}

	if (anywhere)
		*addr = vm_map_min(map);
	else
		*addr = trunc_page(*addr);
	size = round_page(size);

	result = vm_map_find(map, VM_OBJECT_NULL, (vm_offset_t) 0, addr,
			size, anywhere);

	return(result);
}

/*
 *	vm_deallocate deallocates the specified range of addresses in the
 *	specified address map.
 */
kern_return_t vm_deallocate(map, start, size)
	register vm_map_t	map;
	vm_offset_t		start;
	vm_size_t		size;
{
	if (map == VM_MAP_NULL)
		return(KERN_INVALID_ARGUMENT);

	if (size == (vm_offset_t) 0)
		return(KERN_SUCCESS);

	return(vm_map_remove(map, trunc_page(start), round_page(start+size)));
}

/*
 *	vm_inherit sets the inheritence of the specified range in the
 *	specified map.
 */
kern_return_t vm_inherit(map, start, size, new_inheritance)
	register vm_map_t	map;
	vm_offset_t		start;
	vm_size_t		size;
	vm_inherit_t		new_inheritance;
{
	if (map == VM_MAP_NULL)
		return(KERN_INVALID_ARGUMENT);

	return(vm_map_inherit(map, trunc_page(start), round_page(start+size), new_inheritance));
}

/*
 *	vm_protect sets the protection of the specified range in the
 *	specified map.
 */

kern_return_t vm_protect(map, start, size, set_maximum, new_protection)
	register vm_map_t	map;
	vm_offset_t		start;
	vm_size_t		size;
	boolean_t		set_maximum;
	vm_prot_t		new_protection;
{
	if (map == VM_MAP_NULL)
		return(KERN_INVALID_ARGUMENT);

	return(vm_map_protect(map, trunc_page(start), round_page(start+size), new_protection, set_maximum));
}

kern_return_t vm_statistics(map, stat)
	vm_map_t	map;
	vm_statistics_data_t	*stat;
{
	vm_stat.pagesize = page_size;
	vm_stat.free_count = vm_page_free_count;
	vm_stat.active_count = vm_page_active_count;
	vm_stat.reclaim_count = vm_page_reclaim_count;
	vm_stat.wire_count = vm_page_wire_count;
	
	*stat = vm_stat;

	return(KERN_SUCCESS);
}


extern
vm_map_t	ipc_soft_map;

kern_return_t vm_read(map, address, size, data)
	vm_map_t	map;
	vm_address_t	address;
	vm_size_t	size;
	pointer_t	data;
{
	kern_return_t	error;
	vm_offset_t	ipc_address;

	if ((round_page(address) != address) || (round_page(size) != size))
		return(KERN_INVALID_ARGUMENT);

	if ((error = vm_allocate(ipc_soft_map, &ipc_address, size, TRUE)) != KERN_SUCCESS) {
		printf("vm_read: kernel error %d\n", error);
		return(KERN_RESOURCE_SHORTAGE);
	}

	return(vm_map_copy(ipc_soft_map, map, ipc_address, size, address, FALSE, FALSE));
}

kern_return_t vm_write(map, address, data, size)
	vm_map_t	map;
	vm_address_t	address;
	pointer_t	data;
	vm_size_t	size;
{
	if ((round_page(address) != address) || (round_page(size) != size))
		return(KERN_INVALID_ARGUMENT);

	return(vm_map_copy(map, ipc_soft_map, address, size, data, FALSE, FALSE));
}

kern_return_t vm_copy(map, source_address, size, dest_address)
	vm_map_t	map;
	vm_address_t	source_address;
	vm_size_t	size;
	vm_address_t	dest_address;
{
	uprintf("vm_copy: not implemented\n");
	return(KERN_FAILURE);
}

kern_return_t vm_regions(map, regions, size)
	vm_map_t		map;
	vm_region_array_t	*regions;
	long			size;
{
	uprintf("vm_regions: not implemented\n");
	return(KERN_FAILURE);
}
