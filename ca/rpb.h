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
/* $Header: rpb.h,v 4.0 85/07/15 00:47:56 ibmacis GAMMA $ */
/* $Source: /ibm/acis/usr/sys_ca/ca/RCS/rpb.h,v $ */

#if !defined(lint) && !defined(LOCORE) && defined(RCS_HDRS)
static char *rcsidrpb = "$Header: rpb.h,v 4.0 85/07/15 00:47:56 ibmacis GAMMA $";
#endif

/*     rpb.h   6.1     83/07/29        */

/*
 * The restart parameter block, which is a page in (very) low
 * core which runs after a crash.  Currently, the restart
 * procedure takes a dump.
 */
struct rpb {
       struct  rpb *rp_selfref;        /* self-reference */
       int     (*rp_dumprout)();       /* routine to be called */
       long    rp_checksum;            /* checksum of 31 words of dumprout */
       long    rp_flag;                /* set to 1 when dumprout runs */
/* the dump stack grows from the end of the rpb page not to reach here */
};
#ifdef KERNEL
extern struct rpb rpb;
#endif
