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
 *	File:	kern/thread.c
 *	Author:	Avadis Tevanian, Jr., Michael Wayne Young, David Golub
 *
 *	Copyright (C) 1986, Avadis Tevanian, Jr., Michael Wayne Young, David Golub
 *
 *	Thread management primitives implementation.
 *
 * HISTORY
 * 31-May-86  Avadis Tevanian (avie) at Carnegie-Mellon University
 *	Initialize thread state field to THREAD_WAITING.  Some general
 *	cleanup.
 *
 */

#include "cpus.h"
#include "mach_tt.h"

#include "../h/mach_param.h"
#include "../h/thread.h"
#include "../h/zalloc.h"
#include "../vm/vm_param.h"
#include "../vm/vm_kern.h"

thread_t active_threads[NCPUS];
struct zone *thread_zone, *stack_zone, *u_zone;

void thread_init()
{
	thread_zone = zinit(
			sizeof(struct thread),
			THREAD_MAX * sizeof(struct thread),
			THREAD_CHUNK * sizeof(struct thread),
			FALSE, "threads");

#if	MACH_TT
	stack_zone = zinit(
			KERNEL_STACK_SIZE,
			THREAD_MAX * KERNEL_STACK_SIZE,
			KERNEL_STACK_SIZE /* one at a time */,
			FALSE, "kernel stacks");
#else	MACH_TT
	/*
	 *	As long as we're not using the kernel stack,
	 *	make it only long enough for a pcb.
	 */
	stack_zone = zinit(
			sizeof(struct pcb),
			THREAD_MAX * sizeof(struct pcb),
			PAGE_SIZE,	/* can't allocate less */
			FALSE, "pcbs");
#endif	MACH_TT

}

kern_return_t thread_create(parent_task, child_thread, child_data)
	task_t		parent_task;
	thread_t	*child_thread;		/* OUT */
	port_t		*child_data;		/* OUT */
{
	thread_t	new_thread;

	new_thread = (thread_t) zalloc(thread_zone);

	new_thread->runq = NULL;
	new_thread->whichq = NULL;
	lock_init(&new_thread->lock);
	new_thread->task = parent_task;
	new_thread->state = THREAD_WAITING;
	new_thread->pri = 0;
	new_thread->recover = NULL;

#if	MACH_TT
	/*
	 * Setup the u-address pointers.
	 */
	new_thread->u_address.uthread = (struct user *) zalloc(u_zone);
	new_thread->u_address.utask = parent_task->u_address;
#endif	MACH_TT

	/*
	 *	Create a kernel stack, and put the PCB at the front.
	 */

	new_thread->kernel_stack = (vm_offset_t) zalloc(stack_zone);
	if (new_thread->kernel_stack == NULL) {
		panic("thread create: no space for kernel stack");
	}

	new_thread->pcb = (struct pcb *) new_thread->kernel_stack;
#ifdef	vax
	new_thread->pcb_addr = pmap_extract(vm_map_pmap(kernel_map), (vm_offset_t) new_thread->pcb);
#endif	vax

	pcb_init(new_thread->pcb, new_thread->kernel_stack);

	*child_thread = new_thread;
	*child_data = PORT_NULL;
	return(KERN_SUCCESS);
}

thread_destroy(thread)
	thread_t	thread;
{
	zfree(stack_zone, thread->kernel_stack);
	zfree(thread_zone, thread);
}

kern_return_t thread_terminate(thread)
	thread_t	thread;
{
	uprintf("thread_terminate: not implemented\n");
}

kern_return_t thread_suspend(thread)
	thread_t	thread;
{
	uprintf("thread_suspend: not implemented\n");
}

kern_return_t thread_resume(thread)
	thread_t	thread;
{
	uprintf("thread_resume: not implemented\n");
}

kern_return_t thread_status(thread, resumed, old_state)
	thread_t	thread;
	boolean_t	*resumed;
	thread_status_t	*old_state;
{
	uprintf("thread_status_get: not implemented\n");
}

kern_return_t thread_mutate(thread, new_state)
	thread_t	thread;
	thread_status_t	new_state;
{
	uprintf("thread_mutate: not implemented\n");
}
