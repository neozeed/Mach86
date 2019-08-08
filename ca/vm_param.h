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
 *	File:	ca/vm_param.h
 *	Author:	Avadis Tevanian, Jr.
 *
 *	Copyright (C) 1985, Avadis Tevanian, Jr.
 *
 *	VAX machine dependent virtual memory parameters.
 *	Most of the declarations are preceeded by VAX_ (or vax_)
 *	which is OK because only Vax specific code will be using
 *	them.
 *
 * HISTORY
 * 20-Jun-85  Avadis Tevanian (avie) at Carnegie-Mellon University
 *	Created.
 */

#define BYTE_SIZE	8	/* byte size in bits */

#define	VM_MIN_ADDRESS	((vm_offset_t) 0)
#define	VM_MAX_ADDRESS	((vm_offset_t) 0xe0000000)

#define VM_MIN_KERNEL_ADDRESS	((vm_offset_t) 0xe0000000)
#define VM_MAX_KERNEL_ADDRESS	((vm_offset_t) 0xf0000000)

#define	KERNEL_STACK_SIZE	(8*512)	/* XXX we don't use the stack yet */

#define	splvm()	_spl0()

