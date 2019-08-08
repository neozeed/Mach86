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
 *	File:	thread.h
 *	Author:	Avadis Tevanian, Jr.
 *
 *	Copyright (C) 1985, Avadis Tevanian, Jr.
 *
 *	This file contains the structure definitions for threads.
 *
 * HISTORY
 *  1-Jun-86  Avadis Tevanian (avie) at Carnegie-Mellon University
 *	Change the thread lock to a simple lock.  Add thread_lock and
 *	thread_unlock macros for abstract thread locking.  Also
 *	add thread state and event fields.
 *
 *  8-May-86  Michael Young (mwyoung) at Carnegie-Mellon University
 *	Converted calling sequences for exported interface.
 *
 * 25-Aug-85  Avadis Tevanian (avie) at Carnegie-Mellon University
 *	Created.
 */

#ifndef	_THREAD_
#define	_THREAD_

#ifdef	KERNEL
#include "mach_tt.h"
#include "mach_vm.h"
#include "../h/queue.h"
#include "../h/task.h"
#include "../machine/pcb.h"
#include "../sync/lock.h"
#include "../h/sched.h"
#include "../machine/vm_types.h"
#else	KERNEL
#include <sys/features.h>
#include <sys/queue.h>
#include <sys/task.h>
#include <machine/pcb.h>
#include <sync/lock.h>
#include <h/sched.h>
#include <machine/vm_types.h>
#endif	KERNEL

struct thread {
	queue_chain_t	links;		/* current run queue links */
	run_queue_t	runq;		/* run queue p is on */
	run_queue_t	whichq;		/* which queue p must be on */
	simple_lock_data_t lock;	/* lock */
	int		state;		/* thread state */
	int		wait_event;	/* event we are waiting on */
#ifdef	vax
	vm_offset_t	pcb_addr;	/* pcb physical address */
#endif	vax
	struct pcb	*pcb;		/* (kernel virtual) hardware pcb */
	task_t		task;		/* Task I'm a member of */
	vm_offset_t	kernel_stack;	/* Where kernel stack was allocated */
	int		pri;		/* thread's priority */
	vm_offset_t	recover;	/* page fault recovery (copyin/out) */
#if	MACH_TT
	struct {
		struct user	*uthread;
		struct user	*utask;
	} u_address;
#endif	MACH_TT
};

typedef struct thread *thread_t;

typedef	thread_t	*thread_array_t;

/*
 *	Thread states (state field).
 */

#define	THREAD_RUNNING		1	/* thread is running normally */
#define THREAD_WAITING		2	/* thread is waiting normally */
#define THREAD_WILL_WAIT	3	/* thread is on a sleep queue, but is
					   still running */

#if	MACH_VM
thread_t 	*thread_table;		/* the actual thread table */
thread_t	active_threads[];	/* active threads */
#else	MACH_VM
struct thread	*thread;
#endif	MACH_VM

/*
 *	Exported routines
 */

kern_return_t	thread_create();
kern_return_t	thread_terminate();
kern_return_t	thread_suspend();
kern_return_t	thread_resume();
kern_return_t	thread_status();
kern_return_t	thread_mutate();

#define THREAD_NULL	((thread_t) 0)

/*
 *	Exported structures
 */

typedef	int		thread_status_t; /* XXX */

/*
 *	Macro-defined routines
 */

#define	thread_pcb(th)		((th)->pcb)

#define	thread_lock(th)		simple_lock(&(th)->lock)
#define	thread_unlock(th)	simple_unlock(&(th)->lock)

#define	current_thread()	(active_threads[cpu_number()])

#endif	_THREAD_
