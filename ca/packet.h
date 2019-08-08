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
/* $Header: packet.h,v 4.0 85/07/15 00:46:28 ibmacis GAMMA $ */
/* $Source: /ibm/acis/usr/sys_ca/ca/RCS/packet.h,v $ */

#if !defined(lint) && !defined(LOCORE) && defined(RCS_HDRS)
static char *rcsidpacket = "$Header: packet.h,v 4.0 85/07/15 00:46:28 ibmacis GAMMA $";
#endif

struct  rq_packet {			/* REQUEST PACKET */
	u_short pktno;			/* Up to pktnochk must match data */
	u_short pktnochk;		/*   packet format		  */
	u_long  blkno;			/* Starting read/write position  */
					/*   for disk arm (block number) */
	u_short npkts;			/* Number of data packets to be    */
	u_char  flags;			/* Read/write flags */
	u_char  fs;			/* File system index */
}rqp;


/* defines for rqp.flags */

#define READ_RQ         0x01            /* Read Request */
#define WRITE_RQ        0x02            /* Write Request */


/* Packet size definitions */

#define PACKET_SIZE     128		/* Current packet size */
#define PACKET_SHIFT	7		/* Log2(PACKET_SIZE) */

/* Constants for rs232 protocol */

#define SEC		100		/* Number of timmer tics per second */
#define CHAR_TIME_OUT   (SEC * 1)     	/* 1 second */
#define PURGE_TIME_OUT  (SEC * 1)	/* 1 second */
#define ACK_TIME_OUT    (SEC * 8)      	/* 8 seconds */
#define SOH_TIME_OUT    (SEC * 4)      	/* 4 seconds */
#define PKT_TIME_OUT	(SEC * 3)      	/* 3 seconds */


#define TIME_OUT        -1
#define SOH             0x01
#define ACK             0x06
#define NAK             0x15
#define DUP             0x00
#define CAN             0x18
#define ESC             0x1b

#define RETRY_LIMIT	30
#define POLL_TIME	250

/* Maximum number of packets allowed in one I/O request */

#define MAXPKTNO	0xffff

/* Misc constants */

#define	ENABLE	1
#define	DISABLE	0
#define	TRUE	1
#define	FALSE	0
#define EOF	0
