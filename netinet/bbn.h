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
#define MINPORT		0x200	/* minimum non-privileged port */
#define	SMTPPORT	25	/* SMTP socket */

#define	MINGENPORT	0x400	/* minimum generated port */
#define MGPMASK		(MINGENPORT-1)	/* must be a power of 2 to work */
#define MGPSHIFT	10	/* see uniqport for code */

#define ENETSTAT	35     /* user status available (not an error) */
#define ENETUNAVAIL	36     /* net i/f not available */
#define ETABLE		37     /* table full */
#define ENETCON		ETABLE
#define ENETBUF 	38     /* no more net buffer space */
#define ENETPARM	39     /* bad mode in net open or request in net ioctl */
#define ENETRNG 	40     /* bad local net addr, port, raw link or proto range */
#define ENETDED 	41     /* can't open: destination dead */
#define ERAWBAD		42	/* unable to send on raw connection */
#define ENETERR		43	/* attempt to read or write on closed connection */
#define ENETREF		44	/* can't open: destination refused (reset) */
#define ENETUNR 	45	/* can't open: destination unreachable */
#define ENETTIM		46	/* can't open: destination not responding (timeout) */
#define EBLOCK		47	/* interface blocked */
#define EFLUSHED	48	/* network flushed */


struct con {			/* user connection structure */
	u_short c_mode;			/* mode (see below) */
	u_char c_sbufs;			/* # send buffers to use */
	u_char c_rbufs;			/* # rcv buffers to use */
	u_short c_proto;		/* init raw proto number */
	u_char c_timeo;			/* tcp open timeout (seconds) */
	u_char c_noact;			/* no activity timeout (minutes) */
	u_short c_lport;		/* local port */
	u_short c_fport;		/* foreign port */
	struct in_addr c_fcon;			/* foreign net address */
	struct in_addr c_lcon;			/* local net address */
};

struct netstate {		/* network status structure */
	u_char n_snd;			/* # send bufs allocated */
	u_char n_rcv;			/* # receive bufs allocated */
	u_char n_ssize;			/* # bufs on send buffer */
	u_char n_rsize;			/* # bufs on receive buffer */
	u_short n_xstat;		/* network status word */
	u_short n_state;		/* state of this connection */
	u_short n_flags;		/* misc. flags (see below) */
	u_short n_lport;		/* local port */
	u_short n_fport;		/* foreign port */
	struct in_addr n_fcon;			/* foreign socket */
	struct in_addr n_lcon;			/* local socket */
};

				/* c_mode field definitions */
#define CONACT		0000001		/* active connection */
#define CONTCP		0000002		/* open a tcp connection */
#define CONIP		0000004		/* open a raw ip connection */
#define CONRAW		0000010		/* open a raw local net connection */
#define CONCTL		0000020		/* open a control port (no conn) */
#define CONUDP		0000040		/* open a udp connection */
#define CONPUSH		0000100		/* set TCP PUSH flag */
#define CONDEBUG 	0000200		/* turn on debugging info */
#define CONURG		0000400		/* set TCP urgent */
#define CONRAWCOMP	0001000		/* system supplies raw leaders */
#define CONRAWVER	0002000		/* system supplies cksum only */
#define CONRAWASIS	0004000		/* user supplies raw leaders */
#define CONRAWERR	0010000		/* user wants raw ICMP error msgs */
#define CONCWAIT	0020000		/* block on TCP close */
#define CONOBLOK	0040000		/* don't block on TCP open */

				/* n_flags field definitions */
#define ULISTEN		CONACT		/* awaiting a connection */
#define UTCP		CONTCP		/* this is a TCP connection */
#define UIP		CONIP		/* this is a raw IP connection */
#define URAW		CONRAW		/* this is a raw 1822 connection */
#define UCTL		CONCTL		/* this is a control port only */
#define UUDP		CONUDP		/* this is a UDP connecetion */
#define UPUSH		CONPUSH		/* PUSH sent */
#define UDEBUG		CONDEBUG	/* turn on debugging info recording */
#define UURG		CONURG		/* urgent data sent */
#define RAWCOMP		CONRAWCOMP	/* system supplies raw headers */
#define RAWVER		CONRAWVER	/* system supplies raw ip cksum only */
#define RAWASIS		CONRAWASIS	/* user supplies raw headers */
#define RAWERR		CONRAWERR	/* give user ICMP error messages */
#define UCWAIT		CONCWAIT	/* wait for TCP close */
#define UNOBLOK		CONOBLOK	/* don't block on TCP open */
#define RAWMASK		(RAWCOMP+RAWVER+RAWASIS)
#define CONMASK		(UTCP+UIP+URAW+UCTL+UUDP)

				/* n_state field definitions */
#define UCLOSED		0000		/* connection closed */
#define UCLSERR		0001		/* error -- connection closing */
#define UABORT		0002		/* connection aborted */
#define UINTIMO		0004		/* open failed -- init timeout */
#define URXTIMO		0010		/* retransmit too long timeout */
#define URESET		0020		/* connection aborted due to reset */
#define UDEAD		0040		/* destination dead */
#define UURGENT		0100		/* urgent data received */
#define UNRCH		0200		/* destination unreachable */
#define UDROP		0400		/* raw message dropped */
#define UNOACT		01000		/* no activity on connection */
			
				/* net ioctl definitions */
#define NETGETS 	_IOR(N, 1, struct netstate)		/* get status */
#define NETSETD 	_IO (N, 2)		/* set debugging info */
#define NETSETU 	_IO (N, 3)		/* set urgent mode */
#define NETRSETU 	_IO (N, 4)		/* reset urgent mode */
#define NETSETE 	_IO (N, 5)		/* set EOL mode */
#define NETRSETE 	_IO (N, 6)		/* reset EOL mode */
#define NETCLOSE 	_IO (N, 7)		/* initiate tcp close */
#define NETABORT 	_IO (N, 8)		/* initiate tcp abort */
#define NETRESET	_IO (N, 9)		/* forced tcp connection reset */
#define NETDEBUG	_IO (N, 10)		/* toggle debugging flag */
#define NETGINIT	_IO (N, 11)		/* re-read gateway file */
#define NETTCPOPT	_IO (N, 12)		/* set tcp option string */
#define NETPRADD	_IO (N, 13)		/* add to raw proto list */
#define NETPRDEL	_IO (N, 14)		/* delete from raw proto list */
#define NETPRSTAT	_IO (N, 15)		/* return raw proto list */
#define NETROUTE	_IOW(N, 16, long)	/* override IP routing info */
#define NETOWAIT	_IO (N, 17)		/* wait for tcp connection estab */
#define NETINIT		_IO (N, 18)		/* initialize net i/f */
#define NETDISAB	_IO (N, 19)		/* disable net i/f */
#define NETFLUSH	_IO (N, 20)		/* flush user input and output buffs */
#define NETRESERVE	_IO (N, 21)		/* reserve net i/f */
#define NETAVAIL	_IO (N, 22)		/* unreserve net i/f */
#define NETECHO		_IO (N, 23)		/* set echo address */
#define NETENBS		_IOW(N, 24, int)	/* enable signal for net input */
#define NETINHS		_IO (N, 25)		/* disable signal for net input */
#define NETRDNOBLK	_IO (N, 26)		/* don't block on reads */
