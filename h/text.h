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
 *	@(#)text.h	6.3 (Berkeley) 6/8/85
 */

/*
 * Text structure.
 * One allocated per pure
 * procedure on swap device.
 * Manipulated by text.c
 */
#define	NXDAD	12		/* param.h:MAXTSIZ / dmap.h:DMTEXT */

struct text
{
	swblk_t	x_daddr[NXDAD];	/* disk addresses of DMTEXT-page segments */
	swblk_t	x_ptdaddr;	/* disk address of page table */
	size_t	x_size;		/* size (clicks) */
	struct proc *x_caddr;	/* ptr to linked proc, if loaded */
	struct inode *x_iptr;	/* inode of prototype */
	short	x_rssize;
	short	x_swrss;
	short	x_count;	/* reference count */
	short	x_ccount;	/* number of loaded references */
	char	x_flag;		/* traced, written flags */
	char	x_slptime;
	short	x_poip;		/* page out in progress count */
};

#ifdef	KERNEL
struct	text *text, *textNTEXT;
int	ntext;
#endif

#define	XTRC	01		/* Text may be written, exclusive use */
#define	XWRIT	02		/* Text written into, must swap out */
#define	XLOAD	04		/* Currently being read from file */
#define	XLOCK	010		/* Being swapped in or out */
#define	XWANT	020		/* Wanted for swapping */
#define	XPAGI	040		/* Page in on demand from inode */
