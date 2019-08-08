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
 *	@(#)sp.h	6.2 (Berkeley) 6/8/85
 */

/*
 * Definitions for Xerox NS style sequenced packet protocol
 */

struct sphdr {
	u_char	sp_cc;		/* connection control */
	u_char	sp_dt;		/* datastream type */
#define	SP_SP	0x80		/* system packet */
#define	SP_SA	0x40		/* send acknowledgement */
#define	SP_OB	0x20		/* attention (out of band data) */
#define	SP_EM	0x10		/* end of message */
	u_short	sp_sid;		/* source connection identifier */
	u_short	sp_did;		/* destination connection identifier */
	u_short	sp_seq;		/* sequence number */
	u_short	sp_ack;		/* acknowledge number */
	u_short	sp_alo;		/* allocation number */
};
