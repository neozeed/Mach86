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
 *	File:	task.h
 *	Author:	Avadis Tevanian, Jr.
 *
 *	Copyright (C) 1985, Avadis Tevanian, Jr.
 *
 *	This file contains the structure definitions for tasks.
 *
 * HISTORY
 *  8-May-86  Michael Young (mwyoung) at Carnegie-Mellon University
 *	Changed calling sequence for task_* calls to be exported.
 *
 * 10-Dec-85  Avadis Tevanian (avie) at Carnegie-Mellon University
 *	Created.
 */

#ifndef	_TASK_
#define	_TASK_

#ifdef	KERNEL
#include "mach_tt.h"
#include "mach_vm.h"
#else	KERNEL
#include <sys/features.h>
#endif	KERNEL

#ifdef	KERNEL
#include "../sync/lock.h"
#if	MACH_VM
#include "../vm/vm_map.h"
#include "../h/port.h"
#endif	MACH_VM
#else	KERNEL
#include <sync/lock.h>
#if	MACH_VM
#include <vm/vm_map.h>
#include <sys/port.h>
#endif	MACH_VM
#endif	KERNEL

struct task {
	struct lock	lock;		/* task's lock */
#if	MACH_VM
	vm_map_t	map;		/* task's map */
#endif	MACH_VM
	struct proc	*proc;		/* corresponding process */
#if	MACH_TT
	struct user	*u_address;
#endif	MACH_TT
};

typedef struct task *task_t;

task_t		*task_table;		/* the actual task table */

#define TASK_NULL	((task_t) 0)

/*
 *	Exported routines/macros
 */

kern_return_t	task_create();
kern_return_t	task_terminate();
kern_return_t	task_suspend();
kern_return_t	task_resume();
kern_return_t	task_threads();
kern_return_t	task_set_notify();
kern_return_t	task_status();

/*
 *	Internal only routines
 */

task_t		current_task();

#endif	_TASK_
