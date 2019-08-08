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
 *	@(#)gprof.h	6.2 (Berkeley) 6/8/85
 */

struct phdr {
    char	*lpc;
    char	*hpc;
    int		ncnt;
};

    /*
     *	histogram counters are unsigned shorts (according to the kernel).
     */
#define	HISTCOUNTER	unsigned short

    /*
     *	fraction of text space to allocate for histogram counters
     *	here, 1/2
     */
#define	HISTFRACTION	2

    /*
     *	Fraction of text space to allocate for from hash buckets.
     *	The value of HASHFRACTION is based on the minimum number of bytes
     *	of separation between two subroutine call points in the object code.
     *	Given MIN_SUBR_SEPARATION bytes of separation the value of
     *	HASHFRACTION is calculated as:
     *
     *		HASHFRACTION = MIN_SUBR_SEPARATION / (2 * sizeof(short) - 1);
     *
     *	For the VAX, the shortest two call sequence is:
     *
     *		calls	$0,(r0)
     *		calls	$0,(r0)
     *
     *	which is separated by only three bytes, thus HASHFRACTION is 
     *	calculated as:
     *
     *		HASHFRACTION = 3 / (2 * 2 - 1) = 1
     *
     *	Note that the division above rounds down, thus if MIN_SUBR_FRACTION
     *	is less than three, this algorithm will not work!
     *
     *	NB: for the kernel we assert that the shortest two call sequence is:
     *
     *		calls	$0,_name
     *		calls	$0,_name
     *
     *	which is separated by seven bytes, thus HASHFRACTION is calculated as:
     *
     *		HASHFRACTION = 7 / (2 * 2 - 1) = 2
     */
#define	HASHFRACTION	2

    /*
     *	percent of text space to allocate for tostructs
     *	with a minimum.
     */
#define ARCDENSITY	2
#define MINARCS		50

struct tostruct {
    char		*selfpc;
    long		count;
    unsigned short	link;
};

    /*
     *	a raw arc,
     *	    with pointers to the calling site and the called site
     *	    and a count.
     */
struct rawarc {
    unsigned long	raw_frompc;
    unsigned long	raw_selfpc;
    long		raw_count;
};

    /*
     *	general rounding functions.
     */
#define ROUNDDOWN(x,y)	(((x)/(y))*(y))
#define ROUNDUP(x,y)	((((x)+(y)-1)/(y))*(y))
