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
 *	@(#)if_ub.h	6.2 (Berkeley) 6/8/85
 */

/*
 * Structure of a Ungermann-Bass datagram header.
 */

struct un_header {
	u_short	un_length;
	u_char	un_type;
	u_char	un_control;
	u_short	un_dnet;
	u_short	un_dniu;
	u_char	un_dport;
	u_char	un_dtype;
	u_short	un_snet;
	u_short	un_sniu;
	u_char	un_sport;
	u_char	un_stype;
	u_char	un_option;
	u_char	un_bcc;
	u_short	un_ptype;	/* protocol type */
};

/*
 * Protocol types
 */

#define	UNTYPE_INQUIRE		1	/* inquire - "Who am I?" */
#define	UNTYPE_IP		2	/* Internet Protocol */
