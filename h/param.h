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
 *	@(#)param.h	6.14 (Berkeley) 9/18/85
 */
#if	CMU
/*
 **********************************************************************
 * HISTORY
 * 22-Mar-86  Avadis Tevanian (avie) at Carnegie-Mellon University
 *	Merged VM and Romp versions.
 *
 * 22-Mar-86  Avadis Tevanian (avie) at Carnegie-Mellon University
 *	Merged VM and Romp versions.
 *
 * 19-Mar-86  Bill Bolosky (bolosky) at Carnegie-Mellon University
 *	Fixed definitions of the bit manipulation macros (setbit, etc.)
 *	so that their arguments may be of any type (as opposed to having
 *	to be of a single byte length type).
 *
 * 28-Feb-86  Bill Bolosky (bolosky) at Carnegie-Mellon University
 *	Added defintions of _bit_{set,clear,tst} for romp.
 *
 * 20-Feb-86  Bill Bolosky (bolosky) at Carnegie-Mellon University
 *	Spiffed up and included IBM code for Sailboat which handles the
 *	inclusion of signal.h when we are running on the romp
 *	(specifically, not to include it when building locore, or all
 *	hell will break loose).
 *
 * 25-Jan-86  Avadis Tevanian (avie) at Carnegie-Mellon University
 *	Upgraded to 4.3 and increased MAXHOSTNAMELEN to 256 [CS_GENERIC].
 *	Also removed CS_NOFILE.
 *
 * 23-May-85  Glenn Marcy (gm0w) at Carnegie-Mellon University
 *	CS_COMPAT:  Added tsleep() return values from 4.1BSD [V1(1)].
 *
 * 10-Nov-84  Avadis Tevanian (avie) at Carnegie-Mellon University
 *	MACH_MP:  Define PSEM for semaphore sleep priority.
 *
 * 31-Oct-84  Avadis Tevanian (avie) at Carnegie-Mellon University
 *	MACH_MP:  Include reference to ../mp/mp_param.h to include multiprocessor
 *	related parameters.
 *
 * 29-Sep-84  Robert V Baron (rvb) at Carnegie-Mellon University
 *	MACH_ACC:  Define a Sleep priority for IPC (PIPC == PZERO+1).  
 *
 **********************************************************************
 */
 
#ifdef	KERNEL
#include "cs_compat.h"
#include "cs_generic.h"
#include "mach_mp.h"
#include "mach_acc.h"
#include "cs_bugfix.h"
#else	KERNEL
#include <sys/features.h>
#endif	KERNEL
#endif	CMU

#define	BSD	4.3
#define BSD4_3

/*
 * Machine type dependent parameters.
 */
#ifdef KERNEL
#include "../machine/machparam.h"
#else
#include <machine/machparam.h>
#endif

#if	MACH_MP
/*
 *	Include multiprocessor parameters.
 */
#ifdef	KERNEL
#include "../mp/mp_param.h"
#else	KERNEL
#include <mp/mp_param.h>
#endif	KERNEL
#endif	MACH_MP

#define	NPTEPG		(NBPG/(sizeof (struct pte)))

/*
 * Machine-independent constants
 */
#define	NMOUNT	20		/* number of mountable file systems */
/* NMOUNT must be <= 255 unless c_mdev (cmap.h) is expanded */
#define	MSWAPX	NMOUNT		/* pseudo mount table index for swapdev */
#define	MAXUPRC	25		/* max processes per user */
#define	NOFILE	64		/* max open files per process */
#define	CANBSIZ	256		/* max size of typewriter line */
#define	NCARGS	20480		/* # characters in exec arglist */
#define	NGROUPS	16		/* max number groups */

#define	NOGROUP	-1		/* marker for empty group set member */

/*
 * Priorities
 */
#define	PSWP	0
#define	PINOD	10
#define	PRIBIO	20
#define	PRIUBA	24
#define	PZERO	25
#if	MACH_ACC || MACH_MP
#define	PIPC	26
#define	PSEM	26
#endif	MACH_ACC || MACH_MP
#define	PPIPE	26
#define	PWAIT	30
#define	PLOCK	35
#define	PSLEP	40
#define	PUSER	50

#define	NZERO	0

/*
 * Signals
 */
#if defined(KERNEL) && !(defined(LOCORE) && defined(ROMP))
#include "signal.h"
#else KERNEL && !(LOCORE && ROMP)
#if !(defined(ROMP) && defined(LOCORE))
#include <signal.h>
#endif !(ROMP && LOCORE)
#endif KERNEL && !(etc...)

#if	CS_COMPAT
/*
 * Return values from tsleep().
 */
#define	TS_OK	0	/* normal wakeup */
#define	TS_TIME	1	/* timed-out wakeup */
#define	TS_SIG	2	/* asynchronous signal wakeup */

#endif	CS_COMPAT
#define	ISSIG(p) \
	((p)->p_sig && ((p)->p_flag&STRC || \
	 ((p)->p_sig &~ ((p)->p_sigignore | (p)->p_sigmask))) && issig())

#define	NBPW	sizeof(int)	/* number of bytes in an integer */

#define	NULL	0
#define	CMASK	022		/* default mask for file creation */
#define	NODEV	(dev_t)(-1)

/*
 * Clustering of hardware pages on machines with ridiculously small
 * page sizes is done here.  The paging subsystem deals with units of
 * CLSIZE pte's describing NBPG (from vm.h) pages each.
 *
 * NOTE: SSIZE, SINCR and UPAGES must be multiples of CLSIZE
 */
#define	CLBYTES		(CLSIZE*NBPG)
#define	CLOFSET		(CLSIZE*NBPG-1)	/* for clusters, like PGOFSET */
#define	claligned(x)	((((int)(x))&CLOFSET)==0)
#define	CLOFF		CLOFSET
#define	CLSHIFT		(PGSHIFT+CLSIZELOG2)

#if CLSIZE==1
#define	clbase(i)	(i)
#define	clrnd(i)	(i)
#else
/* give the base virtual address (first of CLSIZE) */
#define	clbase(i)	((i) &~ (CLSIZE-1))
/* round a number of clicks up to a whole cluster */
#define	clrnd(i)	(((i) + (CLSIZE-1)) &~ (CLSIZE-1))
#endif

/* CBLOCK is the size of a clist block, must be power of 2 */
#define	CBLOCK	64
#define	CBSIZE	(CBLOCK - sizeof(struct cblock *))	/* data chars/clist */
#define	CROUND	(CBLOCK - 1)				/* clist rounding */

#ifndef KERNEL
#include	<sys/types.h>
#else
#ifndef LOCORE
#include	"types.h"
#endif
#endif

/*
 * File system parameters and macros.
 *
 * The file system is made out of blocks of at most MAXBSIZE units,
 * with smaller units (fragments) only in the last direct block.
 * MAXBSIZE primarily determines the size of buffers in the buffer
 * pool. It may be made larger without any effect on existing
 * file systems; however making it smaller make make some file
 * systems unmountable.
 *
 * Note that the blocked devices are assumed to have DEV_BSIZE
 * "sectors" and that fragments must be some multiple of this size.
 * Block devices are read in BLKDEV_IOSIZE units. This number must
 * be a power of two and in the range of
 *	DEV_BSIZE <= BLKDEV_IOSIZE <= MAXBSIZE
 * This size has no effect upon the file system, but is usually set
 * to the block size of the root file system, so as to maximize the
 * speed of ``fsck''.
 */
#define	MAXBSIZE	8192
#define	DEV_BSIZE	512
#define	DEV_BSHIFT	9		/* log2(DEV_BSIZE) */
#define BLKDEV_IOSIZE	2048
#define MAXFRAG 	8

#define	btodb(bytes)	 		/* calculates (bytes / DEV_BSIZE) */ \
	((unsigned)(bytes) >> DEV_BSHIFT)
#define	dbtob(db)			/* calculates (db * DEV_BSIZE) */ \
	((unsigned)(db) << DEV_BSHIFT)

/*
 * Map a ``block device block'' to a file system block.
 * This should be device dependent, and will be after we
 * add an entry to cdevsw for that purpose.  For now though
 * just use DEV_BSIZE.
 */
#define	bdbtofsb(bn)	((bn) / (BLKDEV_IOSIZE/DEV_BSIZE))

/*
 * MAXPATHLEN defines the longest permissable path length
 * after expanding symbolic links. It is used to allocate
 * a temporary buffer from the buffer pool in which to do the
 * name expansion, hence should be a power of two, and must
 * be less than or equal to MAXBSIZE.
 * MAXSYMLINKS defines the maximum number of symbolic links
 * that may be expanded in a path name. It should be set high
 * enough to allow all legitimate uses, but halt infinite loops
 * reasonably quickly.
 */
#define MAXPATHLEN	1024
#define MAXSYMLINKS	8

/*
 * bit map related macros
 */
#if	CS_BUGFIX
#define	setbit(a,i)	(*(((char *)(a)) + ((i)/NBBY)) |= 1<<((i)%NBBY))
#define	clrbit(a,i)	(*(((char *)(a)) + ((i)/NBBY)) &= ~(1<<((i)%NBBY)))
#define	isset(a,i)	(*(((char *)(a)) + ((i)/NBBY)) & (1<<((i)%NBBY)))
#define	isclr(a,i)      ((*(((char *)(a)) + ((i)/NBBY)) & (1<<((i)%NBBY))) == 0)
#else	CS_BUGFIX
#define	setbit(a,i)	((a)[(i)/NBBY] |= 1<<((i)%NBBY))
#define	clrbit(a,i)	((a)[(i)/NBBY] &= ~(1<<((i)%NBBY)))
#define	isset(a,i)	((a)[(i)/NBBY] & (1<<((i)%NBBY)))
#define	isclr(a,i)	(((a)[(i)/NBBY] & (1<<((i)%NBBY))) == 0)
#endif	CS_BUGFIX
#if	CS_GENERIC
#ifndef vax
#define _bit_set(i,a)   setbit(a,i)
#define _bit_clear(i,a)	clrbit(a,i)
#define _bit_tst(i,a)	isset(a,i)
#endif vax
#endif	CS_GENERIC

/*
 * Macros for fast min/max.
 */
#define	MIN(a,b) (((a)<(b))?(a):(b))
#define	MAX(a,b) (((a)>(b))?(a):(b))

/*
 * Macros for counting and rounding.
 */
#ifndef howmany
#define	howmany(x, y)	(((x)+((y)-1))/(y))
#endif
#define	roundup(x, y)	((((x)+((y)-1))/(y))*(y))

/*
 * Maximum size of hostname recognized and stored in the kernel.
 */
#if	CS_GENERIC
#define MAXHOSTNAMELEN	256
#else	CS_GENERIC
#define MAXHOSTNAMELEN	64
#endif	CS_GENERIC
