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
 *	File:	vm/pmap.h
 *	Author:	Avadis Tevanian, Jr.
 *
 *	Copyright (C) 1985, Avadis Tevanian, Jr.
 *
 *	Machine address mapping definitions -- machine-independent
 *	section.  [For machine-dependent section, see "machine/pmap.h".]
 *
 * HISTORY
 *  6-Jun-85  Avadis Tevanian (avie) at Carnegie-Mellon University
 *	Created.
 */

#ifndef	_PMAP_VM_
#define	_PMAP_VM_

#ifdef	KERNEL
#include "../machine/pmap.h"
#include "../machine/vm_types.h"
#include "../h/types.h"
#else	KERNEL
#include <machine/pmap.h>
#include <machine/vm_types.h>
#include <sys/types.h>
#endif	KERNEL

void		pmap_bootstrap();
void		pmap_init();
vm_offset_t	pmap_map();
pmap_t		pmap_create();
pmap_t		pmap_kernel();
void		pmap_destroy();
void		pmap_reference();
void		pmap_remove();
void		pmap_remove_all();
void		pmap_copy_on_write();
void		pmap_protect();
void		pmap_enter();
vm_offset_t	pmap_extract();
void		pmap_update();
void		pmap_collect();
void		pmap_activate();
void		pmap_deactivate();
void		pmap_copy();
void		pmap_statistics();

void		pmap_redzone();
boolean_t	pmap_access();

#endif	_PMAP_VM_
