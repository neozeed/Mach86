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
 *	@(#)cmap.h	6.6 (Berkeley) 6/8/85
 */
#if	CMU
/*
 **********************************************************************
 * HISTORY
 * 18-Feb-86  Bill Bolosky (bolosky) at Carnegie-Mellon University
 *	Added defn of CUSTRUCT for Sailboat under switch romp.  This
 *	definition should be dropped when we switch in the new vm code,
 *	as it is only used in the romp-vm.
 *
 * 25-Jan-86  Avadis Tevanian (avie) at Carnegie-Mellon University
 *	Upgraded to 4.3.
 *	Berkeley finally made their field big enough, so our changes
 *	could be deleted.
 *
 * 12-Mar-85  Robert V Baron (rvb) at Carnegie-Mellon University
 *	MACH_LOAD:  We have tried to modularize the change to load Unix at an
 *	arbitrary point in physical memory.  The best way to do this seems
 *	to be to modify the pgtocm and cmtopg macros to subtract/add the
 *	loadpg from the pf_num that they deal with.  Nonetheless there are
 *	occasional references to loadpg in open code.
 *
 **********************************************************************
 */
 
#ifdef	KERNEL
#include "cs_generic.h"
#include "mach_load.h"
#else	KERNEL
#include <sys/features.h>
#endif	KERNEL
#endif	CMU

/*
 * core map entry
 *
 * Limits imposed by this structure:
 *
 *		limit		     cur. size		fields
 *	Physical memory+		64 Mb	c_next, c_prev, c_hlink
 *	Mounted filesystems		255	c_mdev
 *	size of a process segment	1 Gb	c_page
 *	filesystem size			8 Gb	c_blkno
 *	proc, text table size		64K	c_ndx
 *
 *	+ memory can be expanded by converting first three entries
 *	to bit fields, shrinking c_ndx, and increasing MAXMEM below.
 */
#ifndef	LOCORE
struct cmap
{
unsigned short 	c_next,		/* index of next free list entry */
		c_prev,		/* index of previous free list entry */
		c_hlink;	/* hash link for <blkno,mdev> */
unsigned short	c_ndx;		/* index of owner proc or text */
unsigned int	c_page:21,	/* virtual page number in segment */
		c_lock:1,	/* locked for raw i/o or pagein */
		c_want:1,	/* wanted */
		c_intrans:1,	/* intransit bit */
		c_free:1,	/* on the free list */
		c_gone:1,	/* associated page has been released */
		c_type:2,	/* type CSYS or CTEXT or CSTACK or CDATA */
		:4,		/* to longword boundary */
		c_blkno:24,	/* disk block this is a copy of */
		c_mdev:8;	/* which mounted dev this is from */
};
#else	LOCORE
/*
 * bit offsets of elements in cmap
 */
#define	C_INTRANS	87
#define	C_FREE		88
#define	SZ_CMAP		16		/* sizeof(struct cmap) */

#define	MAXMEM		64*1024		/* maximum memory, in Kbytes */
#endif	LOCORE

#define	CMHEAD	0

/*
 * Shared text pages are not totally abandoned when a process
 * exits, but are remembered while in the free list hashed by <mdev,blkno>
 * off the cmhash structure so that they can be reattached
 * if another instance of the program runs again soon.
 */
#define	CMHSIZ	512		/* SHOULD BE DYNAMIC */
#define	CMHASH(bn)	((bn)&(CMHSIZ-1))

#ifndef	LOCORE
#ifdef	KERNEL
struct	cmap *cmap;
struct	cmap *ecmap;
int	ncmap;
struct	cmap *mfind();
int	firstfree, maxfree;
int	ecmx;			/* cmap index of ecmap */
u_short	cmhash[CMHSIZ];
#endif

/* bits defined in c_type */

#define	CSYS		0		/* none of below */
#define	CTEXT		1		/* belongs to shared text segment */
#define	CDATA		2		/* belongs to data segment */
#define	CSTACK		3		/* belongs to stack segment */
#ifdef	romp
#define CUSTRUCT	4		/* is a u struct (really CSYS) */
/*
 * This define is only used by the romp vm code.  When we put in the
 * new VM, it should be dropped.
 */
#endif	romp

#if	MACH_LOAD
extern	int loadpg;
extern	int loadpt;
#define	pgtocm(x)	(((((x) - loadpg) - firstfree) / CLSIZE) + 1)
#define	cmtopg(x)	(((((x)-1) * CLSIZE) + firstfree) + loadpg)
#else	MACH_LOAD
#define	pgtocm(x)	(((int) ((x)-firstfree) / CLSIZE) + 1)
#define	cmtopg(x)	((((x)-1) * CLSIZE) + firstfree)
#endif	MACH_LOAD
#endif	LOCORE
