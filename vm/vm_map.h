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
 *	File:	vm/vm_map.h
 *	Author:	Avadis Tevanian, Jr., Michael Wayne Young
 *
 *	Copyright (C) 1985, Avadis Tevanian, Jr., Michael Wayne Young
 *
 *	Virtual memory map module definitions.
 *
 * HISTORY
 *  6-Jun-85  Avadis Tevanian (avie) at Carnegie-Mellon University
 *	Created.
 */

#ifndef	_VM_MAP_
#define	_VM_MAP_

#ifdef	KERNEL
#include "../machine/vm_types.h"
#include "../vm/pmap.h"
#include "../sync/lock.h"
#include "../vm/vm_prot.h"
#include "../vm/vm_inherit.h"
#include "../vm/vm_object.h"
#include "../h/kern_return.h"
#else	KERNEL
#include <machine/vm_types.h>
#include <vm/pmap.h>
#include <sync/lock.h>
#include <vm/vm_prot.h>
#include <vm/vm_inherit.h>
#include <vm/vm_object.h>
#include <sys/kern_return.h>
#endif	KERNEL

/*
 *	Types defined:
 *
 *	vm_map_t		the high-level address map data structure.
 *	vm_map_entry_t		an entry in an address map.
 */

/*
 *	Objects which live in maps may be either VM objects, or
 *	another map (called a "sharing map") which denotes read-write
 *	sharing with other maps.
 */

union vm_map_object {
	struct vm_object	*vm_object;	/* object object */
	struct vm_map		*share_map;	/* share map */
};

typedef union vm_map_object	vm_map_object_t;
/*
 *	Address map entries consist of start and end addresses,
 *	a VM object (or sharing map) and offset into that object,
 *	and user-exported inheritance and protection information.
 *	Also included is control information for virtual copy operations.
 */

struct vm_map_entry {
	struct vm_map_entry	*prev;		/* previous entry */
	struct vm_map_entry	*next;		/* next entry */
	vm_offset_t		start;		/* start address */
	vm_offset_t		end;		/* end address */
	union vm_map_object	object;		/* object I point to */
	vm_offset_t		offset;		/* offset into object */
	boolean_t		is_a_map;	/* Is "object" a map? */
		/* Only in sharing maps: */
	boolean_t		copy_on_write;	/* is data copy-on-write */
	boolean_t		needs_copy;	/* does object need to be copied */
		/* Only in task maps: */
	vm_prot_t		protection;	/* protection code */
	vm_prot_t		max_protection;	/* maximum protection */
	vm_inherit_t		inheritance;	/* inheritance */
	boolean_t		pageable;	/* can be paged */
};

typedef struct vm_map_entry	*vm_map_entry_t;

#define	VM_MAP_ENTRY_NULL	((vm_map_entry_t) 0)

/*
 *	Maps are doubly-linked lists of map entries, kept sorted
 *	by address.  A single hint is provided to start
 *	searches again from the last successful search,
 *	insertion, or removal.
 */
struct vm_map {
	lock_data_t		lock;		/* Lock for map data */
	struct vm_map_entry	header;		/* List of entries */
	int			nentries;	/* Number of entries */
	pmap_t			pmap;		/* Physical map */
	boolean_t		is_main_map;	/* Am I a main map? */
	int			ref_count;	/* Reference count */
	simple_lock_data_t	ref_lock;	/* Lock for ref_count field */
	vm_map_entry_t		hint;		/* hint for quick lookups */
	simple_lock_data_t	hint_lock;	/* lock for hint storage */
	simple_lock_data_t	pageable_lock;	/* lock for (un)wiring down */
#define	min_offset		header.start
#define max_offset		header.end
#define entries_pageable	header.pageable
};

typedef	struct vm_map	*vm_map_t;

#define		VM_MAP_NULL	((vm_map_t) 0)

/*
 *	Macros:		vm_map_lock, etc.
 *	Function:
 *		Perform locking on the data portion of a map.
 */

#define		vm_map_lock(map)	lock_write(&(map)->lock)
#define		vm_map_unlock(map)	lock_write_done(&(map)->lock)
#define		vm_map_lock_read(map)	lock_read(&(map)->lock)
#define		vm_map_unlock_read(map)	lock_read_done(&(map)->lock)

/*
 *	Exported procedures that operate on vm_map_t.
 */

void		vm_map_init();
vm_map_t	vm_map_create();
void		vm_map_deallocate();
void		vm_map_reference();
kern_return_t	vm_map_find();
kern_return_t	vm_map_remove();
kern_return_t	vm_map_lookup();
void		vm_map_lookup_done();
kern_return_t	vm_map_protect();
kern_return_t	vm_map_inherit();
kern_return_t	vm_map_copy();
vm_map_t	vm_map_fork();
void		vm_map_print();

/*
 *	Functions implemented as macros
 */
#define		vm_map_min(map)		((map)->min_offset)
#define		vm_map_max(map)		((map)->max_offset)
#define		vm_map_pmap(map)	((map)->pmap)

#endif	_VM_MAP_
