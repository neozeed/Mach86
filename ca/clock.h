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
/* $Header: clock.h,v 4.0 85/07/15 00:40:48 ibmacis GAMMA $ */
/* $Source: /ibm/acis/usr/sys_ca/ca/RCS/clock.h,v $ */

#if !defined(lint) && !defined(LOCORE) && defined(RCS_HDRS)
static char *rcsidclock = "$Header: clock.h,v 4.0 85/07/15 00:40:48 ibmacis GAMMA $";
#endif

/*     clock.h 6.1     83/07/29        */

#define SECMIN 60			/* seconds per minute */
#define SECHR (60 * 60)			/* seconds per hour */
#define SECDAY ((unsigned)(24*60*60))  /* seconds per day */
#define SECYR  ((unsigned)(365*SECDAY))  /* per common year */

#define YRREF  1970
#define LEAPYEAR(year) ((year)%4==0)  /* good till time becomes negative */

/*
 * Has the time-of-day clock wrapped around?
 */
#define clkwrap() (0)
