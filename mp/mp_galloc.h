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
/* ************************************************************************ *\
 *									    *
 *									    *
 * File:	mp_galloc.h						    *
 *									    *
 * Abstract:								    *
 *	A heap package to allocate space in shared memory for strs, et all  *
 *									    *
 * Author:	Robert V. Baron						    *
 *		Copyright (c) 1984 by Robert V. Baron			    *
 *									    *
 * History:	Created Feb/22/85					    *
 *									    *
\* ************************************************************************ */


struct mp_heap {
	int	mh_size;
	int	mh_limit;
	char 	*mh_end;
	char	*mh_tough;
	char	mh_pool[1];
};

extern char *mh_stralloc();
