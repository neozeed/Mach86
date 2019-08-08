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
 *	@(#)msgbuf.h	6.3 (Berkeley) 6/8/85
 */
#if CMU

/*
 ************************************************************************
 * HISTORY
 * 18-Feb-86  Bill Bolosky (bolosky) at Carnegie-Mellon University
 *	Added definition of msgbufp for Sailboat under romp.  It is
 *	referenced by romp dependent code in machdep.c and subr_prf.c.
 *
 ************************************************************************
 */
#endif	CMU

#define	MSG_MAGIC	0x063061
#define	MSG_BSIZE	(4096 - 3 * sizeof (long))
struct	msgbuf {
	long	msg_magic;
	long	msg_bufx;
	long	msg_bufr;
	char	msg_bufc[MSG_BSIZE];
};
#ifdef KERNEL
struct	msgbuf msgbuf;
#ifdef	romp
struct	msgbuf *msgbufp;	/* real memory address of msgbuf */
#endif	romp
#endif
