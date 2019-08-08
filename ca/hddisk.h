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
/* $Header: hddisk.h,v 4.0 85/07/15 00:42:30 ibmacis GAMMA $ */
/* $Source: /ibm/acis/usr/sys_ca/ca/RCS/hddisk.h,v $ */

#if !defined(lint) && !defined(LOCORE) && defined(RCS_HDRS)
static char *rcsidhddisk = "$Header: hddisk.h,v 4.0 85/07/15 00:42:30 ibmacis GAMMA $";
#endif

/* Disk controler commands */
#define DISKRSTR        0x10    /* Restore command */
#define DISKSEEK        0x70    /* Seek command */
#define DISKREAD        0x20    /* Disk Read */
#define DISKNORE        0x01    /* No retries (rd & wrt) */
#define DISKWRIT        0x30    /* Write command */
#define DISKFRMT        0x50    /* Format command */
#define DISKRDVR        0x40    /* Read/Verify command */
#define DISKDIAG        0x90    /* Diagnostic mode */
#define DISKSETP        0x91    /* Set parameters command */

/* Disk status */
#define DISKBUSY        0x80    /* Adapter busy */
#define DISKRDY         0x40    /* drive ready */
#define DISKWFLT        0x20    /* Write fault */
#define DISKDONE        0x10    /* Seek complete */
#define DISKDRQ         0x08    /* Data ReQuest */
#define DISKCORR        0x04    /* ECC correction */
#define DISKINDX        0x02    /* Index marker error */
#define DISKERR         0x01    /* Error, read errstat */

/* Interrupt mask */
#define DISKIMASK	(DISKERR|DISKWFLT|DISKDRQ|DISKBUSY|DISKRDY|DISKDONE)

/* Disk errors */
#define DISKBBLK        0x80    /* Bad block detect */
#define DISKCRC         0x40    /* Data CRC error */
#define DISKIDNF        0x10    /* ID not found */
#define DISKABRT        0x04    /* Command aborted */
#define DISKTR00        0x02    /* TR000 error */
#define DISKMAM         0x01    /* Missing Data/address/mark */


#define DISKHD3         0x08    /* Head select 3 = 1 */
#define DISKRWRT        0x08    /* Reduced write = 0 */
#define DISKRSET        0x04    /* Reset WD1002 */
#define DISKDISI        0x02    /* Disable Interrupts */
