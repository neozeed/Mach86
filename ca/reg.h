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
/* $Header: reg.h,v 4.0 85/07/15 00:47:34 ibmacis GAMMA $ */
/* $Source: /ibm/acis/usr/sys_ca/ca/RCS/reg.h,v $ */

#if !defined(lint) && !defined(LOCORE) && defined(RCS_HDRS)
static char *rcsidreg = "$Header: reg.h,v 4.0 85/07/15 00:47:34 ibmacis GAMMA $";
#endif

/*
 * Location of the users' stored
 * registers relative to R0.
 * Usage is u.u_ar0[XX].
 */

#define ICSCS (-1)
#define R0 (0)
#define R1 (1)
#define SP (1)
#define R2 (2)
#define R3 (3)
#define R4 (4)
#define R5 (5)
#define R6 (6)
#define R7 (7)
#define R8 (8)
#define R9 (9)
#define R10 (10)
#define R11 (11)
#define R12 (12)
#define R13 (13)
#define R14 (14)
#define R15 (15)
#define IAR (16)
#define MQ  (17)
