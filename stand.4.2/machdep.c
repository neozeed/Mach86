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
/*	machdep.c	6.1	83/07/29	*/

#include "../h/param.h"

#include "../vax/mtpr.h"

/*ARGSUSED*/
/*VARARGS1*/
mtpr(regno, value)
{

	asm("	mtpr	8(ap),4(ap)");
}

/*ARGSUSED*/
mfpr(regno)
{

	asm("	mfpr	4(ap),r0");
#ifdef lint
	return (0);
#endif
}

/*
 * Copy bytes within kernel
 */
/*ARGSUSED*/
bcopy(from, to, count)
	caddr_t from, to;
	unsigned count;
{

	asm("	movc3	12(ap),*4(ap),*8(ap)");
}
