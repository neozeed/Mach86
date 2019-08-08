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
 *	@(#)mtio.h	6.3 (Berkeley) 6/8/85
 */
#if	CMU
/*
 ************************************************************************
 * HISTORY
 * 22-Mar-86  Avadis Tevanian (avie) at Carnegie-Mellon University
 *	Merged VM and Romp versions.
 *
 * 18-Feb-86  Bill Bolosky (bolosky) at Carnegie-Mellon University
 *	Added defintions of MTERAGE and MTRETENSION for Sailboat under
 *	switch romp.
 *
 * 25-Jan-86  Avadis Tevanian (avie) at Carnegie-Mellon University
 *	Upgraded to 4.3
 *
 * 15-Jun-85  Avadis Tevanian (avie) at Carnegie-Mellon University
 *	NTMS:  Added MT_ISTMSCP for tmscp driver.  NOTE:  It is defined as 8
 *	here but is 6 in Ultrix.
 ************************************************************************
 */

#ifdef	KERNEL
#include "tms.h"
#else	KERNEL
#include <sys/features.h>
#endif	KERNEL
#endif	CMU
/*
 * Structures and definitions for mag tape io control commands
 */

/* structure for MTIOCTOP - mag tape op command */
struct	mtop	{
	short	mt_op;		/* operations defined below */
	daddr_t	mt_count;	/* how many of them */
};

/* operations */
#define MTWEOF	0	/* write an end-of-file record */
#define MTFSF	1	/* forward space file */
#define MTBSF	2	/* backward space file */
#define MTFSR	3	/* forward space record */
#define MTBSR	4	/* backward space record */
#define MTREW	5	/* rewind */
#define MTOFFL	6	/* rewind and put the drive offline */
#define MTNOP	7	/* no operation, sets status only */
#ifdef	romp
#define MTERASE 8	/* for streamer: erase tape */
#define MTRETENSION 9	/* for streamer: retension tape */
#endif	romp

/* structure for MTIOCGET - mag tape get status command */

struct	mtget	{
	short	mt_type;	/* type of magtape device */
/* the following two registers are grossly device dependent */
	short	mt_dsreg;	/* ``drive status'' register */
	short	mt_erreg;	/* ``error'' register */
/* end device-dependent registers */
	short	mt_resid;	/* residual count */
/* the following two are not yet implemented */
	daddr_t	mt_fileno;	/* file number of current position */
	daddr_t	mt_blkno;	/* block number of current position */
/* end not yet implemented */
};

/*
 * Constants for mt_type byte
 */
#define	MT_ISTS		0x01
#define	MT_ISHT		0x02
#define	MT_ISTM		0x03
#define	MT_ISMT		0x04
#define	MT_ISUT		0x05
#define	MT_ISCPC	0x06
#define	MT_ISAR		0x07
#if	NTMS > 0
#define MT_ISTMSCP	0x08
#endif	NTMS > 0
#ifdef	romp
#define	MT_ISST		0x08
#endif	romp

/* mag tape io control commands */
#define	MTIOCTOP	_IOW(m, 1, struct mtop)		/* do a mag tape op */
#define	MTIOCGET	_IOR(m, 2, struct mtget)	/* get tape status */
#define MTIOCIEOT	_IO(m, 3)			/* ignore EOT error */
#define MTIOCEEOT	_IO(m, 4)			/* enable EOT error */

#ifndef KERNEL
#define	DEFTAPE	"/dev/rmt12"
#endif
