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
 * Copyright (c) 1982 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)vadvise.h	6.2 (Berkeley) 6/8/85
 */

/*
 * Parameters to vadvise() to tell system of particular paging
 * behaviour:
 *	VA_NORM		Normal strategy
 *	VA_ANOM		Sampling page behaviour is not a win, don't bother
 *			Suitable during GCs in LISP, or sequential or random
 *			page referencing.
 *	VA_SEQL		Sequential behaviour expected.
 *	VA_FLUSH	Invalidate all page table entries.
 */
#define	VA_NORM	0
#define	VA_ANOM	1
#define	VA_SEQL	2
#define	VA_FLUSH 3
