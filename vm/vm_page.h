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
 *	File:	vm/vm_page.h
 *	Author:	Avadis Tevanian, Jr., Michael Wayne Young
 *
 *	Copyright (C) 1985, Avadis Tevanian, Jr., Michael Wayne Young
 *
 *	Resident memory system definitions.
 *
 * HISTORY
 * 10-Jun-85  Avadis Tevanian (avie) at Carnegie-Mellon University
 *	Created.
 */

#ifndef	_VM_PAGE_
#define	_VM_PAGE_

#ifdef	KERNEL
#include "../h/types.h"
#include "../h/queue.h"
#include "../vm/vm_param.h"
#include "../vm/vm_object.h"
#include "../sync/lock.h"
#else	KERNEL
#include <sys/types.h>
#include <sys/queue.h>
#include <vm/vm_param.h>
#include <vm/vm_object.h>
#include <sync/lock.h>
#endif	KERNEL

/*
 *	Management of resident (logical) pages.
 *
 *	A small structure is kept for each resident
 *	page, indexed by page number.  Each structure
 *	is an element of several lists:
 *
 *		A hash table bucket used to quickly
 *		perform object/offset lookups
 *
 *		A list of all pages for a given object,
 *		so they can be quickly reclaimed at
 *		time of deallocation.
 *
 *		An ordered list of pages due for pageout.
 *
 *	In addition, the structure contains the object
 *	and offset to which this page belongs (for pageout),
 *	and sundry status bits.
 */

struct vm_page {
	queue_chain_t	pageq;		/* queue info for FIFO
					 * queue or free list */
	queue_chain_t	hashq;		/* hash table links */
	queue_chain_t	listq;		/* all pages in same object */

	vm_object_t	object;		/* which object am I in */
	vm_offset_t	offset;		/* offset into that object */

	int		wire_count:16,	/* how many wired down maps use me? */
	/* boolean_t */	reclaimable:1,	/* page is in reclaim list */
			active:1,	/* page is in active list */
			clean:1,	/* page is clean */
			laundry:1,	/* page is being cleaned now */
			busy:1;		/* page is in transit */
};

typedef struct vm_page	*vm_page_t;

#define	VM_PAGE_NULL		((vm_page_t) 0)

/*
 *	Each pageable resident page falls into one of three lists:
 *
 *	free	
 *		Available for allocation now.
 *	reclaim	
 *		Not in any physical map, but still has an
 *		object/offset-page mapping, and may be dirty.
 *		This is an intermediate step during pageout.
 *	active
 *		A list of pages which have been placed in
 *		at least one physical map.  This list is
 *		ordered, in LRU-like fashion.
 */

queue_head_t	vm_page_queue_free;	/* memory free queue */
queue_head_t	vm_page_queue_active;	/* active memory queue */
queue_head_t	vm_page_queue_reclaim;	/* reclaimable queue */

vm_page_t	vm_page_array;		/* First resident page in table */
long		first_page;		/* first physical page number */
					/* ... represented in vm_page_array */

long	vm_page_free_count;	/* How many pages are free? */
long	vm_page_active_count;	/* How many pages are active? */
long	vm_page_reclaim_count;	/* How many pages are reclaimable? */
long	vm_page_wire_count;	/* How many pages are wired? */
long	vm_page_free_target;	/* How many do we want free? */
long	vm_page_free_min;	/* When to wakeup pageout */
long	vm_page_reclaim_target;	/* How many do we want reclaimable? */

#define	VM_PAGE_TO_PHYS(entry) \
		(ptoa(((entry) - vm_page_array) + first_page))

#define PHYS_TO_VM_PAGE(pa) \
		(&vm_page_array[atop(pa) - first_page ])

simple_lock_data_t	vm_page_system_lock;	/* vm_page module lock */

vm_offset_t	vm_page_init();
vm_page_t	vm_page_lookup();
vm_page_t	vm_page_alloc();
void		vm_page_free();
void		vm_page_reclaim();
void		vm_page_rename();

boolean_t	vm_page_zero_fill();
void		vm_page_copy();

/*
 *	Functions implemented as macros
 */

#define	vm_page_lock()		simple_lock(&vm_page_system_lock)
#define	vm_page_unlock()	simple_unlock(&vm_page_system_lock)

#define vm_page_set_modified(m)	{ (m)->clean = FALSE; }
#endif	_VM_PAGE_
