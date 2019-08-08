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
 *	File:	vm/vm_init.c
 *	Author:	Avadis Tevanian, Jr., Michael Wayne Young
 *
 *	Copyright (C) 1985, Avadis Tevanian, Jr., Michael Wayne Young
 *
 *	Initialize the Virtual Memory subsystem.
 *
 * HISTORY
 * 18-Sep-85  Avadis Tevanian (avie) at Carnegie-Mellon University
 *	Created.
 */

#include "../machine/vm_types.h"
#include "../sync/lock.h"
#include "../vm/vm_object.h"
#include "../vm/vm_map.h"
#include "../vm/vm_page.h"
#include "../vm/vm_kern.h"

/*
 *	vm_init initializes the virtual memory system.
 *	This is done only by the first cpu up.
 *
 *	The start and end address of physical memory is passed in.
 */

void vm_mem_init()
{
	extern vm_offset_t	avail_start, avail_end, virtual_avail, virtual_end;
	/*
	 *	Initializes resident memory structures.
	 *	From here on, all physical memory is accounted for,
	 *	and we use only virtual addresses.
	 */

	virtual_avail = vm_page_init(avail_start, avail_end, virtual_avail);

	/*
	 *	Initialize other VM packages
	 */

	zone_init();
	vm_object_init();
	vm_map_init();
	kmem_init(virtual_avail, virtual_end);
	pmap_init(avail_start, avail_end);
	
	vm_pager_init();
}
