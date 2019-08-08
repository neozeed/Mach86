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
 *	File:	h/kern_error.h
 *	Author:	Avadis Tevanian, Jr.
 *
 *	Copyright (C) 1985, Avadis Tevanian, Jr.
 *
 *	Kernel error codes.
 *
 * HISTORY
 * 17-Sep-85  Avadis Tevanian (avie) at Carnegie-Mellon University
 *	Created.
 */

#ifndef	_KERN_RETURN_
#define	_KERN_RETURN_

#include "../machine/kern_return.h"

#define KERN_SUCCESS			((kern_return_t) 0)
#define	KERN_INVALID_ADDRESS		((kern_return_t) 1)
#define KERN_PROTECTION_FAILURE		((kern_return_t) 2)
#define	KERN_NO_SPACE			((kern_return_t) 3)
#define	KERN_INVALID_ARGUMENT		((kern_return_t) 4)
#define	KERN_FAILURE			((kern_return_t) 5)
#define	KERN_RESOURCE_SHORTAGE		((kern_return_t) 6)
#define	KERN_NOT_RECEIVER		((kern_return_t) 7)
#define KERN_NO_ACCESS			((kern_return_t) 8)

#endif	_KERN_RETURN_
