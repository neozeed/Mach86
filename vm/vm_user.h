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
 *	File:	vm/vm_user.h
 *	Author:	Avadis Tevanian, Jr., Michael Wayne Young
 *
 *	Copyright (C) 1986, Avadis Tevanian, Jr., Michael Wayne Young
 *
 *	Kernel memory management definitions.
 *
 * HISTORY
 * 14-Oct-85  Michael Wayne Young (mwyoung) at Carnegie-Mellon University
 *	Created header file.
 */

#ifndef	_VM_USER_
#define	_VM_USER_

#ifdef	KERNEL
#include "../h/types.h"
#include "../vm/vm_map.h"
#include "../vm/vm_pager.h"
#include "../h/kern_return.h"
#else	KERNEL
#include <sys/types.h>
#include <vm/vm_map.h>
#include <vm/vm_pager.h>
#include <sys/kern_return.h>
#endif	KERNEL

kern_return_t	vm_allocate();
kern_return_t	vm_deallocate();
kern_return_t	vm_inherit();
kern_return_t	vm_protect();
kern_return_t	vm_statistics();

#endif	_VM_USER_
