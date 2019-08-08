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
 *	File:	romp_init.c
 *	
 *	Unlike vax_init on the vax, this DOES NOT run in physical mode.  Thus,
 *	the hat/ipt is already setup and active, so we can't randomly munge it
 *	anymore.
 */

#include "../h/types.h"

#include "../ca/machparam.h"
#include "../ca/pmap.h"
#include "../ca/rosetta.h"
#include "../h/msgbuf.h"
#include "../vm/vm_param.h"
#include "../vm/vm_prot.h"

extern long	loadpt;
vm_size_t	mem_size;
vm_offset_t	first_addr;
vm_offset_t	last_addr;

vm_offset_t	avail_start, avail_end;
vm_offset_t	virtual_avail, virtual_end;

romp_init()
{
	extern	char	end;
	extern	vm_size_t endmem;	/* Set by lohatipt */
	extern	int	rose_page_size;
	extern	caddr_t	is_in;

	cnatch(0);

#ifdef	notdef
/* DEBUGGING: Turn on the "terminate long search" bit in the TCR. */
	iow(ROSEBASE + 0x15 /*TCR*/, (ior(ROSEBASE + 0x15) | 0x1000)); /*BJB*/
#endif	notdef

	mem_size = endmem * rose_page_size;
	first_addr  = (vm_offset_t)(&end) & 0x0fffffff; /* Make it physical */
	first_addr  = round_page(first_addr);
	last_addr   = (((int)RTA_HATIPT) & 0x0fffffff) - 
			(((sizeof (struct msgbuf) + 2047) >> 11) << 11) - 
			((((mem_size / (rose_page_size * 8)) + 2047) >> 11)
				<< 11);
			/* Assumption1: hat/ipt is at top of memory */
			/* Assumption2: machdep will allocate msgbuf here */
			/* Leave space for the is_in table here, too. */
	last_addr   = trunc_page(last_addr);
	/*Plunk the is_in table right above available memory. */
	is_in = (caddr_t)((int)last_addr + (int)loadpt);  /*Make it virtual.*/

	avail_start = first_addr;
	avail_end   = last_addr;
	printf("RT PC Mach with VM, avail_start = 0x%x, avail_end = 0x%x.\n",
		avail_start, avail_end);
	pmap_bootstrap(loadpt, &avail_start, 
			   &avail_end, &virtual_avail,
			   &virtual_end);

}
