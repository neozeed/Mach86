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
 *	File:	vm/vm_statistics.h
 *	Author:	Avadis Tevanian, Jr., Michael Wayne Young, David Golub
 *
 *	Copyright (C) 1986, Avadis Tevanian, Jr., Michael Wayne Young,
 *				David Golub
 *
 *	Virtual memory statistics structure.
 *
 ************************************************************************
 * HISTORY
 * 22-May-86  Avadis Tevanian (avie) at Carnegie-Mellon University
 *	Defined vm_statistics_data_t as a real typedef so that
 *	MatchMaker can deal with it.
 *
 * 14-Feb-86  Avadis Tevanian (avie) at Carnegie-Mellon University
 *	Created.
 *
 ************************************************************************
 */

#ifndef	_VM_STATISTICS_
#define	_VM_STATISTICS_

struct vm_statistics {
	long	pagesize;		/* page size in bytes */
	long	free_count;		/* # of pages free */
	long	active_count;		/* # of pages active */
	long	reclaim_count;		/* # of pages reclaimable */
	long	wire_count;		/* # of pages wired down */
	long	zero_fill_count;	/* # of zero fill pages */
	long	reclaims;		/* # of pages reclaimed */
	long	pageins;		/* # of pageins */
	long	pageouts;		/* # of pageouts */
	long	faults;			/* # of faults */
	long	cow_faults;		/* # of copy-on-writes */
	long	lookups;		/* object cache lookups */
	long	hits;			/* object cache hits */
};

typedef struct vm_statistics	*vm_statistics_t;
typedef struct vm_statistics	vm_statistics_data_t;

struct vm_statistics	vm_stat;

/*
 *	Each machine dependent implementation is expected to
 *	keep certain statistics.  They may do this anyway they
 *	so choose, but are expected to return the statistics
 *	in the following structure.
 */

struct pmap_statistics {
	long		resident_count;	/* # of pages mapped (total)*/
	long		wired_count;	/* # of pages wired */
};

typedef struct pmap_statistics	*pmap_statistics_t;
#endif	_VM_STATISTICS_
