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
 *	File:	vm/vm_kern.h
 *	Author:	Avadis Tevanian, Jr., Michael Wayne Young
 *
 *	Copyright (C) 1985, Avadis Tevanian, Jr., Michael Wayne Young
 *
 *	Kernel memory management definitions.
 *
 * HISTORY
 * 14-Oct-85  Michael Wayne Young (mwyoung) at Carnegie-Mellon University
 *	Created header file.
 */

#ifdef	KERNEL
#include "mach_acc.h"
#include "../h/types.h"
#include "../vm/vm_map.h"
#else	KERNEL
#include <sys/features.h>
#include <sys/types.h>
#include <vm/vm_map.h>
#endif	KERNEL

void		kmem_init();
void		kmem_terminate();
vm_offset_t	kmem_alloc();
vm_offset_t	kmem_alloc_pageable();
void		kmem_free();
vm_map_t	kmem_suballoc();

vm_map_t	kernel_map;
vm_map_t	user_pt_map;
vm_map_t	sh_map;
vm_map_t	mb_map;
#if	MACH_ACC
vm_map_t	ipc_map;
#endif	MACH_ACC
