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
 *	@(#)ttychars.h	6.2 (Berkeley) 6/8/85
 */

#if CMU
/***********************************************************************
 * HISTORY
 * 18-Feb-86  Bill Bolosky (bolosky) at Carnegie-Mellon University
 *	Added different definition of CERASE for Sailboat under switch
 *	romp from IBM code.
 *
 */
#endif CMU

/*
 * User visible structures and constants
 * related to terminal handling.
 */
#ifndef _TTYCHARS_
#define	_TTYCHARS_
struct ttychars {
	char	tc_erase;	/* erase last character */
	char	tc_kill;	/* erase entire line */
	char	tc_intrc;	/* interrupt */
	char	tc_quitc;	/* quit */
	char	tc_startc;	/* start output */
	char	tc_stopc;	/* stop output */
	char	tc_eofc;	/* end-of-file */
	char	tc_brkc;	/* input delimiter (like nl) */
	char	tc_suspc;	/* stop process signal */
	char	tc_dsuspc;	/* delayed stop process signal */
	char	tc_rprntc;	/* reprint line */
	char	tc_flushc;	/* flush output (toggles) */
	char	tc_werasc;	/* word erase */
	char	tc_lnextc;	/* literal next character */
};

#define	CTRL(c)	('c'&037)

/* default special characters */
#ifdef romp
#define	CERASE	010
#else romp
#define	CERASE	0177
#endif romp
#define	CKILL	CTRL(u)
#define	CINTR	CTRL(c)
#define	CQUIT	034		/* FS, ^\ */
#define	CSTART	CTRL(q)
#define	CSTOP	CTRL(s)
#define	CEOF	CTRL(d)
#define	CEOT	CEOF
#define	CBRK	0377
#define	CSUSP	CTRL(z)
#define	CDSUSP	CTRL(y)
#define	CRPRNT	CTRL(r)
#define	CFLUSH	CTRL(o)
#define	CWERASE	CTRL(w)
#define	CLNEXT	CTRL(v)
#endif
