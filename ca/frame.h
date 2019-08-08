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
/* $Header: frame.h,v 4.0 85/07/20 14:05:03 schuller GAMMA $ */
/* $Source: /ibm/acis/usr/sys_ca/ca/RCS/frame.h,v $ */

#if !defined(lint) && !defined(LOCORE)  && defined(RCS_HDRS)
static char *rcsidframe = "$Header: frame.h,v 4.0 85/07/20 14:05:03 schuller GAMMA $";
#endif

#define FRMARG1  -16	/* locations for first four arg words, IF stored. */
#define FRMARG2  -12	/* they're passed in r2-r5, not storage. */
#define FRMARG3  -8
#define FRMARG4  -4
#define FRMARG5   0	/* first arg word that's always passed in storage. */
#define FRMLINK  -36	/* start of linkage area */
#define FRMREGS  -100	/* start of register save area */
#define FRMFREGS -164	/* start of floating register save area */
#define FRMPROTECT -164	/* kernel, etc. must protect this far below sp */
