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
 *	@(#)vtimes.h	6.3 (Berkeley) 6/9/85
 */

/*
 * Structure returned by vtimes() and in vwait().
 * In vtimes() two of these are returned, one for the process itself
 * and one for all its children.  In vwait() these are combined
 * by adding componentwise (except for maxrss, which is max'ed).
 */
struct vtimes {
	int	vm_utime;		/* user time (60'ths) */
	int	vm_stime;		/* system time (60'ths) */
	/* divide next two by utime+stime to get averages */
	unsigned vm_idsrss;		/* integral of d+s rss */
	unsigned vm_ixrss;		/* integral of text rss */
	int	vm_maxrss;		/* maximum rss */
	int	vm_majflt;		/* major page faults */
	int	vm_minflt;		/* minor page faults */
	int	vm_nswap;		/* number of swaps */
	int	vm_inblk;		/* block reads */
	int	vm_oublk;		/* block writes */
};

#ifdef KERNEL
#endif
