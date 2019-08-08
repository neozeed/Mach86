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
/*CMU:	smalloc.h	7.1	5/29/84	*/

/*
 *	File:	kern_smalloc.c
 *	Author:	Avadis Tevanian, Jr.
 *
 * HISTORY
 * 30-Jan-84  Avadis Tevanian (avie) at Carnegie-Mellon University
 *	Created.
 *
 */

/*
 *	MINSIZE is the minimum size block that will be allocated.
 *	If it is a power of 2 and is greater than or equal to 8, then
 *	all allocations will be quad word aligned.  This is important because
 *	use of the interlocked queue instructions require quad word aligned
 *	queues.  Therefore, it must be a power of 2 and be greater than or
 *	equal to 8.
 */

#define MINSIZE 8
#define NQUEUES 8+CLSIZE

caddr_t kalloc();
