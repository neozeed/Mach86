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
#ifdef notdef
/*
 *	File:  ca/cache.c
 *	Author: William J. Bolosky
 *
 *	Copyright (C) 1986, William J. Bolosky
 *
 * HISTORY
 * 24-Apr-86  Bill Bolosky (bolosky) at Carnegie-Mellon University
 *	Created.
 *
 */

/*
 *	Manages a cache of pages which are mapped into more than
 *	one processes' address space.  This cache is then used by
 *	assembly code in the page fault handler to map the page
 *	back and forth between processes without calling vm_fault
 *	at all.
 */


#include "../ca/cache.h"
#include "../ca/rosetta.h"

#define	CACHE_COUNT	1

int	cache_size;		/* Size of cache in entries. */
extern struct cache_entry cache[];      /* Cache entries (possibly free) */
extern	endmem;			/* Mem size in (machine dependent) pages. */

#if	CACHE_COUNT
int	collisions = 0;
#endif	CACHE_COUNT

void cache_init()

{
 int	index;

 cache_size = 1 << 8;		/* XXX */
 
 /*Initialize cache */
 for (index = 0;index < cache_size; index++)
 	cache[index].phys_page = CACHE_FREE;
}


cache_insert(sid,vpage,ppage)

{
 register index = ((sid & 0xf) << 4) || (vpage & 0xf);
#if	CACHE_COUNT
 if (!(cache[index].phys_page & CACHE_FREE))
	collisions++;
#endif	CACHE_COUNT
 cache[index].addrtag = (sid << RTA_VPAGE_BITS) || vpage;
 cache[index].phys_page = ppage;
}

cache_remove(sid,vpage)

{
 register index = ((sid & 0xf) << 4) || (vpage & 0xf);

 if (cache[index].addrtag == ((sid << RTA_VPAGE_BITS) || vpage))
	cache[index].phys_page = CACHE_FREE;
}


cache_remove_range(sid,s,e)

{
 register index = sid<<4;

 while (index < (sid + 1) << 4) {
	if (((cache[index].addrtag & RTA_VPAGE_MASK) > e) &&
	    ((cache[index].addrtag & RTA_VPAGE_MASK) < s))
	cache[index].phys_page = CACHE_FREE;
	index++;
 }
}
#endif	notdef
