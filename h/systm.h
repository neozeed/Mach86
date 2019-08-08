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
 *	@(#)systm.h	6.3 (Berkeley) 6/8/85
 */
#if	CMU
/*
 **********************************************************************
 * HISTORY
 * 22-Mar-86  Avadis Tevanian (avie) at Carnegie-Mellon University
 *	Merged VM and Romp versions.
 *
 * 18-Feb-86  Bill Bolosky (bolosky) at Carnegie-Mellon University
 *	Added a few extra definitions for Sailboat under switch ROMP.
 *
 * 25-Jan-86  Avadis Tevanian (avie) at Carnegie-Mellon University
 *	Upgraded to 4.3.
 *
 *  4-Nov-85  Avadis Tevanian (avie) at Carnegie-Mellon University
 *	Add sy_parallel flag to the system call entries to specify
 *	whether or not the system call can be executed in parallel.
 *
 * 03-Aug-85  Mike Accetta (mja) at Carnegie-Mellon University
 *	CS_RPAUSE:  Added rpause() and fspause() declarations.
 *
 * 20-Jun-85  Mike Accetta (mja) at Carnegie-Mellon University
 *	CS_BOOT:  Added bootdev definition.
 *	[V1(1)]
 *
 **********************************************************************
 */
 
#ifdef	KERNEL
#include "cs_generic.h"
#include "cs_boot.h"
#include "cs_rpause.h"
#include "mach_mp.h"
#else	KERNEL
#include <sys/features.h>
#endif	KERNEL
#endif	CMU

#if	CS_GENERIC
#ifdef	KERNEL
#include "../h/types.h"
#endif	KERNEL
#endif	CS_GENERIC

/*
 * Random set of variables
 * used by more than one
 * routine.
 */
extern	char version[];		/* system version */

/*
 * Nblkdev is the number of entries
 * (rows) in the block switch. It is
 * set in binit/bio.c by making
 * a pass over the switch.
 * Used in bounds checking on major
 * device numbers.
 */
int	nblkdev;

/*
 * Number of character switch entries.
 * Set by cinit/prim.c
 */
int	nchrdev;

int	nswdev;			/* number of swap devices */
int	mpid;			/* generic for unique process id's */
char	runin;			/* scheduling flag */
char	runout;			/* scheduling flag */
int	runrun;			/* scheduling flag */
char	kmapwnt;		/* kernel map want flag */
char	curpri;			/* more scheduling */

int	maxmem;			/* actual max memory per process */
int	physmem;		/* physical memory on this CPU */
#ifdef	romp
int	endmem;			/* physical end of memory */
int	holestart;		/* start of hole in address space */
int	holelength;		/* length of hole in address space */
/* ishole is true if page is inside memory hole */
#define ishole(v) ((v) >= holestart && (v) < holestart+holelength)
#endif	romp

int	nswap;			/* size of swap space */
int	updlock;		/* lock for sync */
daddr_t	rablock;		/* block to be read ahead */
int	rasize;			/* size of block in rablock */
extern	int intstack[];		/* stack for interrupts */
dev_t	rootdev;		/* device of the root */
dev_t	dumpdev;		/* device to take dumps on */
long	dumplo;			/* offset into dumpdev */
dev_t	swapdev;		/* swapping device */
dev_t	argdev;			/* device for argument lists */

#ifdef vax
extern	int icode[];		/* user init code */
extern	int szicode;		/* its size */
#endif
#ifdef	romp
extern  short icode[];		/* user init code */
extern  int szicode;		/* its size */
#endif	romp

daddr_t	bmap();
caddr_t	calloc();
unsigned max();
unsigned min();
int	memall();
int	uchar(), schar();
int	vmemall();
caddr_t	wmemall();
swblk_t	vtod();
#if	CS_RPAUSE
bool	rpause();
bool	fspause();
#endif	CS_RPAUSE

/*
 * Structure of the system-entry table
 */
extern struct sysent
{
#if	MACH_MP
	short	sy_narg;		/* total number of arguments */
	short	sy_parallel;		/* can execute in parallel */
#else	MACH_MP
	int	sy_narg;		/* total number of arguments */
#endif	MACH_MP
	int	(*sy_call)();		/* handler */
} sysent[];

int	noproc;			/* no one is running just now */
char	*panicstr;
int	wantin;
int	boothowto;		/* reboot flags, from console subsystem */
#if	CS_GENERIC
int	show_space;
#endif	CS_GENERIC
#if	CS_BOOT
int	bootdev;		/* boot device, from bootstrap subsystem */
#endif	CS_BOOT
int	selwait;

extern	char vmmap[];		/* poor name! */

/* casts to keep lint happy */
#define	insque(q,p)	_insque((caddr_t)q,(caddr_t)p)
#define	remque(q)	_remque((caddr_t)q)
#define	queue(q,p)	_queue((caddr_t)q,(caddr_t)p)
#define	dequeue(q)	_dequeue((caddr_t)q)
