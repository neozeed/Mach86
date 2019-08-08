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
 *	@(#)if_en.h	6.3 (Berkeley) 6/8/85
 */
#if	CMU
/*
 **********************************************************************
 * HISTORY
 * 26-Jan-86  Avadis Tevanian (avie) at Carnegie-Mellon University
 *	Upgraded to 4.3.
 *
 * 13-Aug-85  Mike Accetta (mja) at Carnegie-Mellon University
 *	CS_INET:  Added ENTYPE_ARP definition.
 *	[V1(1)]
 *
 **********************************************************************
 */
 
#ifdef	KERNEL
#include "cs_inet.h"
#else	KERNEL
#include <sys/features.h>
#endif	KERNEL
#endif	CMU

/*
 * Structure of a Ethernet header.
 */
struct	en_header {
	u_char	en_shost;
	u_char	en_dhost;
	u_short	en_type;
};

#define	ENTYPE_PUP	0x0200		/* PUP protocol */
#define	ENTYPE_IP	0x0201		/* IP protocol */
#if	CS_INET
#define	ENTYPE_ARP	0x0207		/* ARP protocol (CMU) */
#endif	CS_INET

/*
 * The ENTYPE_NTRAILER packet types starting at
 * ENTYPE_TRAIL have (type-ENTYPE_TRAIL)*512 bytes
 * of data followed by an Ethernet type (as given above)
 * and then the (variable-length) header.
 */
#define	ENTYPE_TRAIL	0x1000		/* Trailer type */
#define	ENTYPE_NTRAILER	16

#define EN_BROADCAST	0		/* Hardware broadcast address */
