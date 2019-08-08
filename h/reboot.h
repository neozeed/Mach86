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
 *	@(#)reboot.h	6.2 (Berkeley) 6/8/85
 */
#if	CMU
/*
 * HISTORY
 * 25-Jan-86  Avadis Tevanian (avie) at Carnegie-Mellon University
 *	Upgraded to 4.3.
 *
 * 22-Oct-85  Avadis Tevanian (avie) at Carnegie-Mellon University
 *	CS_KDB:  Added RB_KDB flag for kernel debugger.  This is currently
 *	defined at 0x04 (RB_NOSYNC) to be backward compatible, but
 *	should be changed in the future (when it is convenient to
 *	update the boot programs).
 *
 * 15-Jun-85  Avadis Tevanian (avie) at Carnegie-Mellon University
 *	CS_VAXM:  Added flags for MicroVAX II console.
 *
 */
#ifdef	KERNEL
#include "cs_kdb.h"
#include "cs_vaxm.h"
#else	KERNEL
#include <sys/features.h>
#endif	KERNEL
#endif	CMU

/*
 * Arguments to reboot system call.
 * These are passed to boot program in r11,
 * and on to init.
 */
#define	RB_AUTOBOOT	0	/* flags for system auto-booting itself */

#define	RB_ASKNAME	0x01	/* ask for file name to reboot from */
#define	RB_SINGLE	0x02	/* reboot to single user only */
#define	RB_NOSYNC	0x04	/* dont sync before reboot */
#if	CS_KDB
#define RB_KDB		0x04	/* load kernel debugger */
#endif	CS_KDB
#define	RB_HALT		0x08	/* don't reboot, just halt */
#define	RB_INITNAME	0x10	/* name given for /etc/init */

#define	RB_PANIC	0	/* reboot due to panic */
#define	RB_BOOT		1	/* reboot due to boot() */

#if	CS_VAXM
/*
 * Flags for MicroVAX-II console program communication
 */
#define RB_RESTART	0x21	/* Restart, english	*/
#define RB_REBOOT	0x22	/* Reboot, english	*/
#define RB_HALTMD	0x23	/* Halt, english	*/
#endif	CS_VAXM
