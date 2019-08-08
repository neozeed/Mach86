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
#
/*
 *	File:	kern/mach_if.c
 *	Author:	Avadis Tevanian, Jr., Michael Wayne Young
 *
 *	Copyright (C) 1986, Avadis Tevanian, Jr., Michael Wayne Young
 *
 *	Ancillary functions to support the Mach user interface.
 *
 * HISTORY
 * 30-Apr-86  Michael Young (mwyoung) at Carnegie-Mellon University
 *	Created.
 *	Temporary, until IPC is rewritten, and some of the
 *	functionality is actually built.
 */


#include "../kern/mach_if.h"
#include "../accent/accent.h"
#include "../h/proc.h"
	
task_t convert_port_to_task(x)
	port_t		x;
{
	if (PortArray[x].Receiver != pKERNEL_PROCESS)
		return(TASK_NULL);

	return(PortArray[x].Owner->task);
}

thread_t convert_port_to_thread(x)
	port_t		x;
{
	uprintf("convert_port_to_thread: not implemented\n");
}

vm_map_t convert_port_to_map(x)
	port_t		x;
{
	task_t		y = convert_port_to_task(x);
	return(y == TASK_NULL ? VM_MAP_NULL : y->map);
}

port_t convert_task_to_port(x)
	task_t		x;
{
	if (convert_port_to_task(aproc[x->proc - proc]->KPorts[KernelPort]) != x)
		panic("convert_task_to_port: not invertible");
	return(aproc[x->proc - proc]->KPorts[KernelPort]);
}

port_t convert_thread_to_port(x)
	thread_t	x;
{
	uprintf("convert_thread_to_port: not implemented\n");
}

port_t	task_self()
{
	return(1);
}
port_t	task_data()
{
	return(2);
}
port_t	task_notify()
{
	return(2);
}
port_t	thread_self()
{
	return(0);
}

port_t	ns_port()
{
	return(3);
}
