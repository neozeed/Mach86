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
 *	File:	zalloc.h
 *	Author:	Avadis Tevanian, Jr.
 *
 *	Copyright (C) 1985, Avadis Tevanian, Jr.
 *
 * HISTORY
 *  9-Jun-85  Avadis Tevanian (avie) at Carnegie-Mellon University
 *	Created.
 */

#ifndef	_ZALLOC_
#define	_ZALLOC_

#ifdef	KERNEL
#include "../h/types.h"
#include "../sync/lock.h"
#include "../sync/mp_queue.h"
#else	KERNEL
#include <sys/types.h>
#include <sync/lock.h>
#include <sync/mp_queue.h>
#endif	KERNEL

/*
 *	A zone is a collection of fixed size blocks for which there
 *	is fast allocation/deallocation access.  Kernel routines can
 *	use zones to manage data structures dynamically, creating a zone
 *	for each type of data structure to be managed.
 */

typedef struct zone {
	struct mpqueue	freeq;		/* list of free blocks */
	vm_offset_t	base_addr;	/* base address of zone memory */
	vm_size_t	cur_size;	/* current memory utilization */
	vm_size_t	max_size;	/* how large can this zone grow */
	vm_size_t	elem_size;	/* size of an element */
	vm_size_t	alloc_size;	/* size used for more memory */
	simple_lock_t	lock;		/* generic lock */
	boolean_t	pageable;	/* zone pageable? */
	char		*zone_name;	/* a name for the zone */
} *zone_t;

caddr_t		zalloc();
zone_t		zinit();

#endif	_ZALLOC_
