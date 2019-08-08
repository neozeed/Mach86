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
 *	File:	vm_object.h
 *	Author:	Avadis Tevanian, Jr., Michael Wayne Young
 *
 *	Copyright (C) 1985, Avadis Tevanian, Jr., Michael Wayne Young
 *
 *	Virtual memory object module definitions.
 *
 * HISTORY
 * 21-May-86  David Golub (dbg) at Carnegie-Mellon University
 *	Added vm_object_cache_clear.
 *
 *  2-May-86  David Golub (dbg) at Carnegie-Mellon University
 *	Removed already_locked parameter from vm_object_deallocate.
 *
 *  6-Jun-85  Avadis Tevanian (avie) at Carnegie-Mellon University
 *	Created.
 */

#ifndef	_VM_OBJECT_
#define	_VM_OBJECT_

#ifdef	KERNEL
#include "../h/queue.h"
#include "../sync/lock.h"
#include "../h/types.h"		/* for boolean_t */
#include "../vm/vm_pager.h"
#else	KERNEL
#include <sys/queue.h>
#include <sync/lock.h>
#include <sys/types.h>		/* for boolean_t */
#include <vm/vm_pager.h>
#endif	KERNEL

/*
 *	Types defined:
 *
 *	vm_object_t		Virtual memory object.
 */

struct vm_object {
	queue_chain_t		memq;		/* Resident memory */
	queue_chain_t		object_list;	/* list of all objects */
	simple_lock_data_t	lock;		/* Synchronization */
	int			ref_count;	/* How many refs?? */
	vm_size_t		size;		/* Object size */
	boolean_t		is_kernel_object;
	int			cluster_size;	/* Cluster size */
	vm_pager_t		pager;		/* My pager */
	vm_pager_id_t		paging_space;	/* My paging space */
	vm_offset_t		paging_offset;	/* Offset into paging space */
	struct vm_object	*shadow;	/* My shadow */
	vm_offset_t		shadow_offset;	/* Offset in shadow */
	boolean_t		pageout_in_progress;
						/* Pageout daemon using object */
	boolean_t		read_only;	/* Object may not be modified */
};

typedef struct vm_object	*vm_object_t;

struct vm_object_hash_entry {
	vm_object_t		object;
	vm_pager_t		pager;
	vm_pager_id_t		paging_space;
};

typedef struct vm_object_hash_entry	*vm_object_hash_entry_t;

queue_head_t	vm_object_list;		/* list of allocated objects in system */

#define	VM_OBJECT_NULL		((vm_object_t) NULL)

#define MAX_CLUSTER_SIZE	16

/*
 *	Declare procedures that operate on bs objects.
 */

void		vm_object_init ();
void		vm_object_terminate();
vm_object_t	vm_object_allocate();
void		vm_object_reference();
void		vm_object_deallocate();
vm_object_t	vm_object_null();
void		vm_object_pmap_copy();
void		vm_object_pmap_remove();
void		vm_object_page_remove();
void		vm_object_shadow();
void		vm_object_setpager();
vm_object_t	vm_object_lookup();
void		vm_object_cache();
void		vm_object_uncache();
void		vm_object_cache_clear();
void		vm_object_print();

#endif	_VM_OBJECT_
