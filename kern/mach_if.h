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
#

/*
 *	File:	kern/mach_if.h
 *	Author:	Avadis Tevanian, Jr., Michael Wayne Young
 *
 *	Copyright (C) 1986, Avadis Tevanian, Jr., Michael Wayne Young
 *
 *	Mach external interface definitions.
 *
 * HISTORY
 * 30-Apr-86  Michael Young (mwyoung) at Carnegie-Mellon University
 *	Created.
 *	Very temporary, until we get IPC rewritten.
 */


#ifndef	_MACH_IF_
#define	_MACH_IF_	1

#ifdef	KERNEL
#include "../accent/accenttype.h"
#include "../vm/vm_user.h"
#include "../h/task.h"
#include "../h/thread.h"
#else	KERNEL
#include <accent/accenttype.h>
#include <vm/vm_user.h>
#include <sys/task.h>
#include <sys/thread.h>
#endif	KERNEL

/*
 */

#define		mach_message_id(x)	(x >= 1000)

/*
 *	Compatibility issues
 */

typedef vm_offset_t vm_address_t;
typedef vm_offset_t pointer_t;
typedef int vm_region_t;
typedef vm_region_t *vm_region_array_t;

#define FastAssign 0

/* typedef task_t vm_task_t; */

/*
 *	Conversion routines, to let Matchmaker do this for
 *	us automagically.
 */

extern task_t convert_port_to_task( /* port_t x */ );
extern thread_t convert_port_to_thread( /* port_t x */ );
extern vm_map_t convert_port_to_map( /* port_t x */ );
extern port_t convert_task_to_port( /* task_t x */ );
extern port_t convert_thread_to_port( /* thread_t x */ );

#endif	_MACH_IF_
