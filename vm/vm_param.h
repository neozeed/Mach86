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
 *	File:	vm/vm_param.h
 *	Author:	Avadis Tevanian, Jr., Michael Wayne Young
 *
 *	Copyright (C) 1985, Avadis Tevanian, Jr., Michael Wayne Young
 *
 *	Machine independent virtual memory parameters.
 *
 * HISTORY
 * 21-May-86  David Golub (dbg) at Carnegie-Mellon University
 *	Added page_mask.  Changed types of page_size and page_mask
 *	to vm_size_t.
 *
 *  9-Jun-85  Avadis Tevanian (avie) at Carnegie-Mellon University
 *	Created.
 *
 */

#ifndef	_VM_PARAM_
#define	_VM_PARAM_

#ifdef	KERNEL
#include "../machine/vm_param.h"
#include "../machine/vm_types.h"
#else	KERNEL
#include <machine/vm_param.h>
#include <machine/vm_types.h>
#endif	KERNEL

/*
 *	The machine independent pages are refered to as PAGES.  A page
 *	is some number of hardware pages, depending on the target machine.
 */

/*
 *	For compatibility with old code.
 */

#define	PAGE_SIZE	page_size	/* size of page in addressible units */
#define PAGE_SHIFT	page_shift	/* number of bits to shift for pages */

#ifdef	ASSEMBLER
#else	ASSEMBLER
/*
 *	Convert addresses to pages and vice versa.
 *	No rounding is used.
 */

#define	atop(x)		(((unsigned)(x)) >> page_shift)
#define	ptoa(x)		((vm_offset_t)((x) << page_shift))

/*
 *	Round off or truncate to the nearest page.  These will work
 *	for either addresses or counts.  (i.e. 1 byte rounds to 1 page
 *	bytes.
 */

#define round_page(x)	((vm_offset_t)((((vm_offset_t)(x)) + page_mask) & ~page_mask))
#define trunc_page(x)	((vm_offset_t)(((vm_offset_t)(x)) & ~page_mask))

#ifdef	KERNEL
extern vm_size_t	page_size;		/* machine independent page size */
extern vm_size_t	page_mask;		/* page_size - 1; mask for
						   offset within page */
extern int		page_shift;		/* shift to use for page size */

extern vm_size_t	mem_size;		/* size of physical memory (bytes) */
extern vm_offset_t	first_addr;		/* first physical page */
extern vm_offset_t	last_addr;		/* last physical page */
#endif	KERNEL

#endif	ASSEMBLER
#endif	_VM_PARAM_
