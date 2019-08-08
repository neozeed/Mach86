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
 *	File:	ca/pcb.c
 *	Author:	Avadis Tevanian, Jr., Michael Wayne Young, David Golub, William J. Bolosky
 *
 *	Copyright (C) 1986, Avadis Tevanian, Jr., Michael Wayne Young, David Golub, William J. Bolosky
 *
 *	Romp PCB management
 *
 * HISTORY
 */

#include "mach_tt.h"

#include "../ca/pcb.h"
#include "../h/thread.h"
#include "../vm/vm_param.h"
#include "../ca/pmap.h"	/* for KERNSTACK */
#include "../h/param.h"
#include "../h/systm.h"
#include "../h/dir.h"
#include "../h/user.h"

void pcb_init(pcb, ksp)
	struct pcb	*pcb;
	vm_offset_t	ksp;
{
	bzero((caddr_t)pcb, sizeof(struct pcb));

	pcb->pcb_usp = pcb->pcb_ksp = KERNSTACK;
	pcb->pcb_ssp = -1;
	pcb->pcb_esp = -1;
}

/*
 *	Set up the context for the very first thread to execute
 *	(which is passed in as the parameter).
 */
void initial_context(thread)
	thread_t	thread;
{
	extern	int	after_lc;

	active_threads[cpu_number()] = thread;
	pmap_activate(vm_map_pmap(thread->task->map), thread, cpu_number());
	thread->pcb->pcb_r15 = (int)(&after_lc);
}
