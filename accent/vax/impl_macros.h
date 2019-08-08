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
 * Package:	IPC
 *
 * File:	impl_stuff.h
 *
 * Abstract:
 *	Highly machine specific definitions
 *
 * Author:	Robert V. Baron
 *		Copyright (c) 1984 by Robert V. Baron
 *
 * HISTORY
 * 12-May-86  Rick Rashid (rfr) at Carnegie-Mellon University
 *	Removed IPCWakeup macro (now a routine in vmipc.c). All old
 *	shared memory code is now obsolete.
 *
 * 11-Oct-85  Robert V Baron (rvb) at Carnegie-Mellon University
 *	fixed to use new shared memory primitives
 *
 */

#ifndef	_IMPL_MACROS_
#define	_IMPL_MACROS_
#include "../accent/acc_macro.h"
#endif	_IMPL_MACROS_

