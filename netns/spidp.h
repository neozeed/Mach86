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
 *	@(#)spidp.h	6.2 (Berkeley) 6/8/85
 */

/*
 * Definitions for NS(tm) Internet Datagram Protocol
 * containing a Sequenced Packet Protocol packet.
 */
struct spidp {
	struct idp	si_i;
	struct sphdr 	si_s;
};
struct spidp_q {
	struct spidp_q	*si_next;
	struct spidp_q	*si_prev;
};
#define SI(x)	((struct spidp *)x)
#define si_sum	si_i.idp_sum
#define si_len	si_i.idp_len
#define si_tc	si_i.idp_tc
#define si_pt	si_i.idp_pt
#define si_dna	si_i.idp_dna
#define si_sna	si_i.idp_sna
#define si_sport	si_i.idp_sna.x_port
#define si_cc	si_s.sp_cc
#define si_dt	si_s.sp_dt
#define si_sid	si_s.sp_sid
#define si_did	si_s.sp_did
#define si_seq	si_s.sp_seq
#define si_ack	si_s.sp_ack
#define si_alo	si_s.sp_alo
