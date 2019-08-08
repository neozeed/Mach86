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
/* $Header: softint.h,v 4.0 85/07/15 00:48:24 ibmacis GAMMA $ */
/* $Source: /ibm/acis/usr/sys_ca/ca/RCS/softint.h,v $ */

#if !defined(lint) && !defined(LOCORE) && defined(RCS_HDRS)
static char *rcsidsoftint = "$Header: softint.h,v 4.0 85/07/15 00:48:24 ibmacis GAMMA $";
#endif

/*	softint.h	*/

/*
 * CPU level 6 is used for software interrupts.
 *
 * The softlevel word contains one bit representing which software
 * interrupt routine is requested.
 *
 * SOFT_ALL is the OR of all the valid bits and
 * is used for defensive programming.
 */

/*
 * Bit positions for softlevel
 */

#define SOFT_CLOCK	1
#define SOFT_NET	2

#define SOFT_ALL	(SOFT_CLOCK | SOFT_NET)

#if defined(KERNEL) && !defined(LOCORE)
int	softlevel;
#endif
