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
 ************************************************************************
 * HISTORY
 * 31-May-86  Avadis Tevanian (avie) at Carnegie-Mellon University
 *	Fixed arguments to be "task" instead of "ServPort."
 ************************************************************************
 */

#include "../kern/mach_if.h"
#include "../accent/accent.h"

kern_return_t port_allocate(task, my_port)
	task_t	task;
	port_t	*my_port;
{ 
	*my_port = NewPort(PortArray[convert_task_to_port(task)].Owner,
			ActualDefaultBacklog);
	if (*my_port == PORT_NULL)
		return(KERN_RESOURCE_SHORTAGE);
	else
		return(KERN_SUCCESS);
} 

kern_return_t port_deallocate(task, my_port, reason)
	task_t	task;
	port_t	my_port;
	int	reason;
{ 
	RemPort(PortArray[convert_task_to_port(task)].Owner, my_port);
	return(KERN_SUCCESS);
} 

kern_return_t port_enable(task, my_port)
	task_t	task;
	port_t	 my_port;
{
	if (PortArray[my_port].Receiver->task != task)
		return(KERN_NOT_RECEIVER);

	PortArray[my_port].Locked = FALSE; /* Locked is inverse of enabled */
	return(KERN_SUCCESS);
}

kern_return_t port_disable(task, my_port)
	task_t	task;
	port_t	 my_port;
{
	if (PortArray[my_port].Receiver->task != task)
		return(KERN_NOT_RECEIVER);

	PortArray[my_port].Locked = TRUE;
	return(KERN_SUCCESS);
}

kern_return_t port_select(task, port_list, count)
	task_t		task;
	port_array_t	*port_list;
	long		*count;
{
	uprintf("port_select: not implemented\n");
}

kern_return_t port_set_backlog(task, my_port, backlog)
	task_t	task;
	port_t	my_port;
	int	backlog;
{
	if (PortArray[my_port].Receiver->task != task)
		return(KERN_NOT_RECEIVER);

	/* XXX mwyoung thinks it's bogus to accept this Backlog without
	 * checking, so does Avie */
	PortArray[my_port].Backlog = backlog;
	return(KERN_SUCCESS);
}

kern_return_t port_status(task, my_port, enabled, num_msgs, backlog)
	task_t		task;
	port_t		my_port;
	boolean_t	*enabled;
	int	 	*num_msgs;
	int	 	*backlog;
{
	uprintf("port_status: not implemented\n");
}
