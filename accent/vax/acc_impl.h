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
 * acc_impl.h -- Vax version
 *
 * Constants of the specific Accent implementation.
 *
 * WARNING:
 *	ANY USE OF THE CONSTANTS IN THIS FILE MAY NOT BE TOTALLY PORTABLE.
 *	THEY ARE AVAILABLE ONLY FOR SYSTEM-DEPENDENT APPLICATIONS WHICH
 *	NEED TO KNOW THEM.
 *
 * Michael B. Jones
 *
 * Copyright (C) 1981, 1982, 1983, 1984, 1985
 *	Richard F. Rashid, Michael B. Jones, and Carnegie-Mellon University
 *
 * HISTORY:
 * 01-Jan-86  Mike Accetta (mja) at Carnegie-Mellon University
 *	Raised MaxNumProcs and NumProcs to 512 from 256 and 128,
 *	respectively.
 *
 *
 *  15-Aug-85	Michael B. Jones
 *		Moved page size parameters here from machdep.h.
 *
 *  14-Aug-85	Michael B. Jones
 *		Broke off from accenttype.h, proctypes.h.
 */

#ifndef _Acc_Impl
#define _Acc_Impl


#if ! VAX
Wrong version of acc_impl.h!
#endif

/*
 * Define the page size of the implementation.
 */

#define	PageBits	9		/* Addr bits per page */
#define PageUnitSize	(1 << PageBits)	/* Addr units per page */
#define	PageByteSize	(PageUnitSize * BytesPerUnit)	/* Bytes per page */


/*
 * Special port ranges
 */

#define	FirstNonReservedPort  3L
#define LastInitPort	9L

/*
 * Port queue sizes
 */

#define ActualDefaultBacklog 4
#define	MaxBacklog	7

/*
 * Process management restrictions
 */

#define NumPriorities	16	/* Number of process priorities */
#define NumSleepQs	32	/* Number of process sleep queues */
#define NumQueues	54	/* Number of process queues */

/*
 * Maximum numbers of ports, processes.
 */

#define NumPorts	512
#define MaxNumProcs	512
#define NumProcs	512

/*
 * Maximum size of simple (non-mapped) messages.
 */

#define TotalKMsgSize		8192

/*
 * The following constants are not currently used in the Vax implementation,
 * but are maintained for compatibility with the Accent message interface.
 */

#define MaxPartChars 	8	/* Maximum length for a partition name */
#define MaxDPChars 	25	/* Maximum length for a dev:part name */

#define MaxPartitions	10	/* Maximum partitions on one device */
#define MaxDevices	5	/* Maximum number of devices */

#endif _Acc_Impl
