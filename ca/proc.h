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
/* $Header: proc.h,v 4.0 85/07/15 00:47:07 ibmacis GAMMA $ */
/* $Source: /ibm/acis/usr/sys_ca/ca/RCS/proc.h,v $ */

#if !defined(lint) && !defined(LOCORE) && defined(RCS_HDRS)
static char *rcsidproc = "$Header: proc.h,v 4.0 85/07/15 00:47:07 ibmacis GAMMA $";
#endif

/* process table offsets for kernel assembler code */
#ifdef LOCORE
P_ADDR = 12
P_CPU = 18
P_LINK = 0
P_PRI = 17
P_P0BR = 100
P_RLINK = 4
P_SID0 = 8
P_SID1 = 10
P_STAT = 19
P_SZPT = 62
P_USRPRI = 16
P_PID = 48
SRUN=3
SZOMB=5
U_PROCP = 128
USIZE = 0x784
#endif
