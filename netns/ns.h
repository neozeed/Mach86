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
 *	@(#)ns.h	6.7 (Berkeley) 9/6/85
 */

/*
 * Constants and Structures defined by the Xerox Network Software
 * per "Internet Transport Protocols", XSIS 028112, December 1981
 */

/*
 * Protocols
 */
#define NSPROTO_RI	1		/* Routing Information */
#define NSPROTO_ECHO	2		/* Echo Protocol */
#define NSPROTO_ERROR	3		/* Error Protocol */
#define NSPROTO_PE	4		/* Packet Exchange */
#define NSPROTO_SPP	5		/* Sequenced Packet */
#define NSPROTO_RAW	255		/* Placemarker*/
#define NSPROTO_MAX	256		/* Placemarker*/


/*
 * Port/Socket numbers: network standard functions
 */

#define NSPORT_RI	1		/* Routing Information */
#define NSPORT_ECHO	2		/* Echo */
#define NSPORT_RE	3		/* Router Error */

/*
 * Ports < NSPORT_RESERVED are reserved for priveleged
 * processes (e.g. root).
 */
#define NSPORT_RESERVED		3000

/* flags passed to ns_output as last parameter */

#define	NS_FORWARDING		0x1	/* most of idp header exists */
#define	NS_ROUTETOIF		0x10	/* same as SO_DONTROUTE */
#define	NS_ALLOWBROADCAST	SO_BROADCAST	/* can send broadcast packets */

#define NS_MAXHOPS		15

/* flags passed to get/set socket option */
#define	SO_HEADERS_ON_INPUT	1
#define	SO_HEADERS_ON_OUTPUT	2
#define	SO_DEFAULT_HEADERS	3
#define	SO_LAST_HEADER		4
#define	SO_NSIP_ROUTE		5
#define SO_SEQNO		6
#define	SO_ALL_PACKETS		7
#define SO_MTU			8


/*
 * NS addressing
 */
union ns_host {
	u_char	c_host[6];
	u_short	s_host[3];
};

union ns_net {
	u_char	c_net[4];
	u_short	s_net[2];
};

struct ns_addr {
	union ns_net	x_net;
	union ns_host	x_host;
	u_short	x_port;
};

/*
 * Socket address, Xerox style
 */
struct sockaddr_ns {
	u_short		sns_family;
	struct ns_addr	sns_addr;
	char		sns_zero[2];
};
#define sns_port sns_addr.x_port

#define ns_netof(a) (*(long *) & ((a).x_net))
#define satons_addr(sa)	(((struct sockaddr_ns *)&(sa))->sns_addr)
#define ns_hosteqnh(s,t) ((s).s_host[0] == (t).s_host[0] && \
	(s).s_host[1] == (t).s_host[1] && (s).s_host[2] == (t).s_host[2])
#define ns_hosteq(s,t) (ns_hosteqnh((s).x_host,(t).x_host))
#define ns_nullhost(x) (((x).x_host.s_host[0]==0) && \
	((x).x_host.s_host[1]==0) && ((x).x_host.s_host[2]==0))

#if !defined(vax)
#if !defined(INET)
/*
 * Macros for number representation conversion.
 */
#define	ntohl(x)	(x)
#define	ntohs(x)	(x)
#define	htonl(x)	(x)
#define	htons(x)	(x)
#endif
#endif

#ifdef KERNEL
extern struct domain nsdomain;
extern union ns_host ns_thishost;
extern union ns_host ns_zerohost;
extern union ns_host ns_broadhost;
u_short ns_cksum();
#endif
