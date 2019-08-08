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
 *	File:	vm/vm_pager.c
 *	Author:	Avadis Tevanian, Jr., Michael Wayne Young
 *
 *	Copyright (C) 1986, Avadis Tevanian, Jr., Michael Wayne Young
 *	Copyright (C) 1985, Avadis Tevanian, Jr., Michael Wayne Young
 *
 *	Paging space routine stubs.  Emulates a matchmaker-like interface
 *	for builtin pagers.
 *
 * HISTORY
 * 23-May-86  David Golub (dbg) at Carnegie-Mellon University
 *	Restored the text pager so that text files can be protected from
 *	writing (the rest of unix looks at the ITEXT bit to keep from
 *	overwriting text files in use).  Header file inode_pager.h defines
 *	the pagein/pageout routines to be the same for inode and text
 *	pagers.
 *
 * 19-May-86  Avadis Tevanian (avie) at Carnegie-Mellon University
 *	Purge "text" pager since "inode" pager can now handle text.
 *
 */

#include "mach_mach.h"
#include "mach_pager.h"

#include "../h/types.h"
#include "../h/kern_return.h"
#include "../vm/vm_pager.h"
#include "../vm/vm_page.h"
#include "../vm/vm_prot.h"

#include "../vm/pmap.h"

#include "../vm/inode_pager.h"

extern
boolean_t	vm_page_zero_fill();

struct {
	boolean_t	(*pagein)();		/**/
	boolean_t	(*pageout)();
	boolean_t	(*space_alloc)();
	boolean_t	(*space_dealloc)();
} builtin_pagers[] = {
			{ NULL, NULL, NULL, NULL },			/* vm_pager_null */
			{ vm_page_zero_fill, NULL, NULL, NULL },	/* vm_pager_zero_fill */
			{ inode_pagein, inode_pageout, inode_alloc, inode_dealloc },
			{ text_pagein, text_pageout, NULL, text_dealloc },
			};

vm_pager_t	vm_pager_null = (-1);
vm_pager_t	vm_pager_zero_fill = (-2);
vm_pager_t	vm_pager_inode = (-3);
vm_pager_t	vm_pager_text = (-4);
vm_pager_t	vm_pager_default;

#define	builtin(p)	(p < 0)
#define	stuff(p)	(builtin_pagers[-p-1])

/*
 *	Initialize the built-in pagers.  Kinda kludgy now.
 */
void vm_pager_init()
{
	inode_pager_init();
#if	MACH_PAGER
	init_pager();
#endif	MACH_PAGER
	vm_pager_default = vm_pager_inode;
}

boolean_t vm_pager_get(pager, id, m)
	vm_pager_t	pager;
	vm_pager_id_t	id;
	vm_page_t	m;
{
	if (builtin(pager)) {
		if (stuff(pager).pagein != NULL)
			return (stuff(pager).pagein(m));
	}
#if	MACH_PAGER
	else {
		vm_offset_t	addr;
		vm_size_t	count;

		if (pager_read(pager, id, m->offset, page_size, &addr, &count)) {
			if (count != page_size)
				panic("bad count from pager_read");
			copy_to_phys(addr, VM_PAGE_TO_PHYS(m), page_size);
			vm_deallocate(kernel_map, addr, page_size);
			return(TRUE);
		}
		return(FALSE);
	}
#else	MACH_PAGER
	else
	 	panic("vm_pager_get: unknown pager!");
#endif	MACH_PAGER
}

boolean_t vm_pager_put(pager, id, m)
	vm_pager_t	pager;
	vm_pager_id_t	id;
	vm_page_t	m;
{
	if (builtin(pager)) {
		if (stuff(pager).pageout != NULL) {
			return (stuff(pager).pageout(m));
		}
	}
#if	MACH_PAGER
	else {
		vm_offset_t	addr;
		boolean_t	ret;

		vm_allocate(kernel_map, &addr, page_size, TRUE);
		pmap_enter(vm_map_pmap(kernel_map), addr, VM_PAGE_TO_PHYS(m),
				VM_PROT_READ|VM_PROT_WRITE, FALSE);
		ret = pager_write(pager, id, m->offset, addr, page_size);
		vm_deallocate(kernel_map, addr, page_size);
		return(ret);
	}
#else	MACH_PAGER
 	panic("vm_pager_put: unknown pager!");
#endif	MACH_PAGER
}

boolean_t vm_pager_deallocate(pager, id)
	vm_pager_t	pager;
	vm_pager_id_t	id;
{
	if (builtin(pager)) {
		if (stuff(pager).space_dealloc != NULL)
			return (stuff(pager).space_dealloc(id));
	}
#if	MACH_PAGER
	else {
		return (deallocate_pager_space(pager, id));
	}
#else	MACH_PAGER
	else
	 	panic("vm_pager_deallocate: unknown pager!");
#endif	MACH_PAGER
}

vm_pager_id_t vm_pager_allocate(pager, size)
	vm_pager_t	pager;
{
	vm_pager_id_t	id;

	if (builtin(pager)) {
		if (stuff(pager).space_alloc != NULL)
			id = stuff(pager).space_alloc(size);
		else
			panic("no allocation routine for pager!");
	}
	else
	 	panic("vm_pager_allocate: unknown pager!");
	return(id);
}

#if	MACH_MACH
kern_return_t pager_clean(pager, id, offset, size)
	vm_pager_t	pager;
	vm_pager_id_t	id;
	vm_offset_t	offset;
	vm_size_t	size;
{
	uprintf("pager_clean not implemented\n");
}

kern_return_t pager_fetch(pager, id, offset, size)
	vm_pager_t	pager;
	vm_pager_id_t	id;
	vm_offset_t	offset;
	vm_size_t	size;
{
	uprintf("pager_fetch not implemented\n");
}
#endif	MACH_MACH
