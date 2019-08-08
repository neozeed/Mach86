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
 * 5799-CGZ (C) COPYRIGHT IBM CORPORATION 1986
 * LICENSED MATERIALS - PROPERTY OF IBM
 * REFER TO COPYRIGHT INSTRUCTIONS FORM NUMBER G120-2083
 */
/* $Header: apio.h,v 5.1 86/02/12 18:10:02 katherin Exp $ */
/* $Source: /ibm/acis/usr/sys_ca/caio/RCS/apio.h,v $ */

#if !defined(lint) && !defined(LOCORE)  && defined(RCS_HDRS)
static char *rcsidapio = "$Header: apio.h,v 5.1 86/02/12 18:10:02 katherin Exp $";
#endif


struct apioinfo
   {
   int apstatus;	/* returned status code */
   char apcmd;		/* expedited command */
   };

/*
 * ioctl command values
 */
#define APTEST   _IOR(a,0,struct apioinfo) /* test status */
#define APTESTRD _IOR(a,1,struct apioinfo) /* test status/read cmd */
#define APRDCMD  _IOR(a,2,struct apioinfo) /* read command */
#define APWRTCMD _IOW(a,3,struct apioinfo) /* write command */

/*
 * status codes that can be returned from APTEST, APTESTRD or APRDCMD
 */
#define APNOINFO 0	/* no status info or expedited cmd */
#define APDOWN 1 	/* sync or i/o error - please close the device */
#define APGOTDATA 2	/* read data is available - please read it */
#define APGOTCMD 3	/* expedited command is in apcmd */
#define APSYNCING 4	/* syncing in progress, line not up yet */
/* APRDCMD cannot get APNOINFO or APSYNCING as a status */

/*
 * Other ioctl codes allowed by the async data mode protocol are:
 * TIOCGETD, TIOCSETD, TIOCGETP, TIOCEXCL, TIOCNXCL
 */
