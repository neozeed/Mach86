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
 *	File:	vm_pager.h
 *	Author:	Avadis Tevanian, Jr., Michael Wayne Young
 *
 *	Copyright (C) 1986, Avadis Tevanian, Jr., Michael Wayne Young
 *	Copyright (C) 1985, Avadis Tevanian, Jr., Michael Wayne Young
 *
 *	Pager routine interface definition
 *
 * HISTORY
 * 30-Jan-86  Michael Young (mwyoung) at Carnegie-Mellon University
 *	Converted vm_pager_t to an integer, in preparation for making it
 *	a port.  A new module (vm_pager.c) handles internal pagers.
 *
 * 27-Oct-85  Avadis Tevanian (avie) at Carnegie-Mellon University
 *	Created.
 */

#ifndef	_VM_PAGER_
#define	_VM_PAGER_

#ifdef	KERNEL
#include "../h/types.h"		/* for boolean_t */
#else	KERNEL
#include <sys/types.h>
#endif	KERNEL

/*
 *	A pager is really a port, but for now it's an integer.
 *	Special internal pagers get special port numbers.
 */

typedef	int		vm_pager_t;
typedef	int		vm_pager_id_t;
#define		VM_PAGER_ID_NULL	((vm_pager_id_t) 0)

extern vm_pager_t	vm_pager_null;
extern vm_pager_t	vm_pager_zero_fill;
extern vm_pager_t	vm_pager_inode;
extern vm_pager_t	vm_pager_text;
extern vm_pager_t	vm_pager_default;

boolean_t	vm_pager_alloc();
boolean_t	vm_pager_dealloc();
boolean_t	vm_pager_get();
boolean_t	vm_pager_put();

#endif	_VM_PAGER_
