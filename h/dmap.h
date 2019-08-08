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
 *	@(#)dmap.h	6.3 (Berkeley) 6/8/85
 */
#if	CMU
/*
 **********************************************************************
 * HISTORY
 * 25-Jan-86  Avadis Tevanian (avie) at Carnegie-Mellon University
 *	Upgraded to 4.3.
 *
 * 07-May-85  Mike Accetta (mja) at Carnegie-Mellon University
 *	Upgraded from 4.1BSD.  Carried over changes below:
 *	[V1(1)]
 *
 * 12-Apr-82  Mike Accetta (mja) at Carnegie-Mellon University
 *	CS_GENERIC:  Increased NDMAP from 48 to 65 to support 32Mb.
 *
 **********************************************************************
 */
 
#ifdef	KERNEL
#include "cs_generic.h"
#else	KERNEL
#include <sys/features.h>
#endif	KERNEL
#endif	CMU

/*
 * Definitions for the mapping of vitual swap
 * space to the physical swap area - the disk map.
 */

#if	CS_GENERIC
#define	NDMAP 		65	/* size of the swap area map */
#else	CS_GENERIC
#define	NDMAP 		38	/* size of the swap area map */
#endif	CS_GENERIC

struct	dmap
{
	swblk_t	dm_size;	/* current size used by process */
	swblk_t	dm_alloc;	/* amount of physical swap space allocated */
	swblk_t	dm_map[NDMAP];	/* first disk block number in each chunk */
};
#ifdef KERNEL
struct	dmap zdmap;
int	dmmin, dmmax, dmtext;
#endif

/*
 * The following structure is that ``returned''
 * from a call to vstodb().
 */
struct	dblock
{
	swblk_t	db_base;	/* base of physical contig drum block */
	swblk_t	db_size;	/* size of block */
};
