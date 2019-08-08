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
 *	File:	kern/task.c
 *	Author:	Avadis Tevanian, Jr., Michael Wayne Young, David Golub
 *
 *	Copyright (C) 1986, Avadis Tevanian, Jr., Michael Wayne Young, David Golub
 *
 *	Task management primitives implementation.
 *
 * HISTORY
 */

#include "../h/mach_param.h"
#include "../h/task.h"
#include "../h/thread.h"
#include "../h/zalloc.h"
#include "../vm/vm_param.h"

/* static */ struct zone *task_zone;

void task_init()
{
	task_zone = zinit(
			sizeof(struct task),
			TASK_MAX * sizeof(struct task),
			TASK_CHUNK * sizeof(struct task),
			FALSE, "tasks");

}

kern_return_t task_create(parent_task, inherit_memory, child_task, child_data)
	task_t		parent_task;
	boolean_t	inherit_memory;
	task_t		*child_task;		/* OUT */
	port_t		*child_data;		/* OUT */
{
	register task_t	new_task;
#if	MACH_TT
	extern struct zone *u_zone;
#endif	MACH_TT

	new_task = (task_t) zalloc(task_zone);
	if (new_task == TASK_NULL) {
		panic("task_create: no memory for task structure");
	}

#if	MACH_TT
	new_task->u_address = (struct user *) zalloc(u_zone);
#endif	MACH_TT

	if (inherit_memory)
		new_task->map = vm_map_fork(parent_task->map);
	else
		new_task->map = vm_map_create(pmap_create(0),
					round_page(VM_MIN_ADDRESS),
					trunc_page(VM_MAX_ADDRESS), TRUE);

	lock_init(&new_task->lock);
	*child_task = new_task;
	*child_data = PORT_NULL;
	return(KERN_SUCCESS);
}

task_destroy(task)
	task_t	task;
{
	vm_map_deallocate(task->map);
	zfree(task_zone, task);
}

kern_return_t	task_terminate ( ServPort,reason)
	task_t	ServPort;
	int	reason;
{
	uprintf("task_terminate: not implemented\n");
}

kern_return_t	task_suspend ( ServPort)
	task_t	ServPort;
{
	uprintf("task_suspend: not implemented\n");
}

kern_return_t	task_resume ( ServPort)
	task_t	ServPort;
{
	uprintf("task_resume: not implemented\n");
}

kern_return_t	task_threads ( ServPort,thread_list,thread_list_Cnt)
	task_t	ServPort;
	thread_array_t	 * thread_list;
	/* Long */ long * thread_list_Cnt;
{
	uprintf("task_threads: not implemented\n");
}

kern_return_t	task_status ( ServPort,notify_port,resumed)
	task_t	ServPort;
	port_t	 * notify_port;
	boolean_t	 * resumed;
{
	*notify_port = PORT_NULL;
	*resumed = TRUE;
}

kern_return_t	task_set_notify ( ServPort,notify_port)
	task_t	ServPort;
	port_t	notify_port;
{
	uprintf("task_set_notify: not implemented\n");
}

kern_return_t	task_ports ( ServPort,port_list,port_list_Cnt)
	task_t		ServPort;
	port_array_t	*port_list;
	long		*port_list_Cnt;
{
	uprintf("task_ports: not implemented\n");
}
