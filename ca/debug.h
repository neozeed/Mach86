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
/* $Header: debug.h,v 4.0 85/07/15 00:41:50 ibmacis GAMMA $ */
/* $Source: /ibm/acis/usr/sys_ca/ca/RCS/debug.h,v $ */

#ifdef	CMU
/*
 ***********************************************************************
 * HISTORY
 *  4-May-86  Bill Bolosky (bolosky) at Carnegie-Mellon University
 *	Switched DEBUG tom ROMP_DEBUG and made it an OPTIONS style
 *	option rather than a command line one.
 *
 ***********************************************************************
 */
#include "romp_debug.h"
#endif	CMU

#ifndef CONF
extern char vmdebug;  /* virtual memory */
extern char trdebug;  /* trap traces */
extern char svdebug;  /* svc traces */
extern char ttydebug; /* TTY (async) debug */
extern char cndebug;  /* Console (mono/keybrd) debug */
extern char iodebug;  /* low level i/o */
extern char fsdebug;  /* Unix file system */
extern char swdebug;  /* swapping system */
extern char padebug;  /* paging system */
extern char sydebug;  /* syncronization */
extern char prt_scr;  /* Which screen for printf */
#endif CONF

char	indebug;	/* interrupt system */
int	hddebug;	/* hard disk debug flag */
char	autodebug;	/* for autoconf debugging */
int	fddebug;	/* for floppy driver */

#define PRT_CONSOLE	0x1
#define PRT_MONO	0x2

#ifdef ROMP_DEBUG
#define DEBUGF(cond,stmt) if (cond) stmt	/* do the stmt (printf) */
#else
#define DEBUGF(cond,stmt) 		/* do nothing if not debugging */
#endif ROMP_DEBUG
