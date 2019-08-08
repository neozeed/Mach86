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
 *	File:  ca/cache.h
 *	Author: William J. Bolosky
 *
 *	Copyright (C) 1986, William J. Bolosky
 *
 * HISTORY
 * 23-Apr-86  Bill Bolosky (bolosky) at Carnegie-Mellon University
 *	Created.
 *
 */

/* WARNING: Assembly code in lopckrt0 depends on this being an 8 byte
 * record.  If this changes, you should fix ca/lopckrt0.s
 */
struct {long	addrtag;	/* (12-bit)seg no and vpage */
	int	phys_page;
       } cache_entry;

#define	CACHE_FREE	0x10000000 /* Free bit in phys_page field */
