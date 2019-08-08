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
/* @(#)if_qereg.h	1.2 (ULTRIX) 1/3/85 */

/************************************************************************
 *									*
 *			Copyright (c) 1983 by				*
 *		Digital Equipment Corporation, Maynard, MA		*
 *			All rights reserved.				*
 *									*
 *   This software is furnished under a license and may be used and	*
 *   copied  only  in accordance with the terms of such license and	*
 *   with the  inclusion  of  the  above  copyright  notice.   This	*
 *   software  or  any  other copies thereof may not be provided or	*
 *   otherwise made available to any other person.  No title to and	*
 *   ownership of the software is hereby transferred.			*
 *									*
 *   This software is  derived  from  software  received  from  the	*
 *   University    of   California,   Berkeley,   and   from   Bell	*
 *   Laboratories.  Use, duplication, or disclosure is  subject  to	*
 *   restrictions  under  license  agreements  with  University  of	*
 *   California and with AT&T.						*
 *									*
 *   The information in this software is subject to change  without	*
 *   notice  and should not be construed as a commitment by Digital	*
 *   Equipment Corporation.						*
 *									*
 *   Digital assumes no responsibility for the use  or  reliability	*
 *   of its software on equipment which is not supplied by Digital.	*
 *									*
 ************************************************************************/

/* ---------------------------------------------------------------------
 * Modification History 
 *
 *  13 Feb. 84 -- rjl
 *
 *	Initial version of driver. derived from IL driver.
 * 
 * ---------------------------------------------------------------------
 */

/*
 * Digital Q-BUS to NI Adapter 
 */
struct qedevice {
	u_short	qe_sta_addr[2]; 	/* Station address (actually 6 	*/
	u_short	qe_rcvlist_lo; 		/* Recieve list lo address 	*/
	u_short	qe_rcvlist_hi; 		/* Recieve list hi address 	*/
	u_short	qe_xmtlist_lo;		/* Transmit list lo address 	*/
	u_short	qe_xmtlist_hi;		/* Transmit list hi address 	*/
	u_short	qe_vector;		/* Interrupt vector 		*/
	u_short	qe_csr;			/* Command and Status Register 	*/
};

/*
 * Command and status bits (csr)
 */
#define QE_RCV_ENABLE	0x0001		/* Receiver enable		*/
#define QE_RESET	0x0002		/* Software reset		*/
#define QE_NEX_MEM_INT	0x0004		/* Non existant mem interrupt	*/
#define QE_LOAD_ROM	0x0008		/* Load boot/diag from rom	*/
#define QE_XL_INVALID	0x0010		/* Transmit list invalid	*/
#define QE_RL_INVALID	0x0020		/* Receive list invalid		*/
#define QE_INT_ENABLE	0x0040		/* Interrupt enable		*/
#define QE_XMIT_INT	0x0080		/* Transmit interrupt		*/
#define QE_ILOOP 	0x0100		/* Internal loopback		*/
#define QE_ELOOP	0x0200		/* External loopback		*/
#define QE_STIM_ENABLE	0x0400		/* Sanity timer enable		*/
#define QE_POWERUP	0x1000		/* Tranceiver power on		*/
#define QE_CARRIER	0x2000		/* Carrier detect		*/
#define QE_RCV_INT	0x8000		/* Receiver interrupt		*/

/*
 * Transmit and receive ring discriptor ---------------------------
 *
 * The QNA uses the flag, status1 and the valid bit as a handshake/semiphore
 * mechinism. 
 * 
 * The flag word is written on ( bits 15,15 set to 1 ) when it reads the
 * descriptor. If the valid bit is set it considers the address to be valid.
 * When it uses the buffer pointed to by the valid address it sets status word
 * one.
 */
struct qe_ring	{
	u_short qe_flag;		/* Buffer utilization flags	*/
	u_short qe_addr_hi:6,		/* Hi order bits of buffer addr	*/
	      qe_odd_begin:1,		/* Odd byte begin and end (xmit)*/
	      qe_odd_end:1,
	      qe_fill1:4,
	      qe_setup:1,		/* Setup packet			*/
	      qe_eomsg:1,		/* End of message flag		*/
	      qe_chain:1,		/* Chain address instead of buf */
	      qe_valid:1;		/* Address field is valid	*/
	u_short qe_addr_lo;		/* Low order bits of address	*/
	short qe_buf_len;		/* Negative buffer length	*/
	u_short qe_status1;		/* Status word one		*/
	u_short qe_status2;		/* Status word two		*/
};

/*
 * Status word definations (receive)
 *	word1
 */
#define QE_OVF			0x0001	/* Receiver overflow		*/
#define QE_CRCERR		0x0002	/* CRC error			*/
#define QE_FRAME		0x0004	/* Framing alignment error	*/
#define QE_SHORT		0x0008	/* Packet size < 10 bytes	*/
#define QE_RBL_HI		0x0700	/* Hi bits of receive len	*/
#define QE_RUNT			0x0800	/* Runt packet			*/
#define QE_DISCARD		0x1000	/* Discard the packet		*/
#define QE_ESETUP		0x2000	/* Looped back setup or eloop	*/
#define QE_ERROR		0x4000	/* Receiver error		*/
#define QE_LASTNOT		0x8000	/* Not the last in the packet	*/
/*	word2								*/
#define QE_RBL_LO		0x00ff	/* Low bits of receive len	*/

/*
 * Status word definations (transmit)
 *	word1
 */
#define QE_CCNT			0x00f0	/* Collision count this packet	*/
#define QE_FAIL			0x0100	/* Heart beat check failure	*/
#define QE_ABORT		0x0200	/* Transmission abort		*/
#define QE_STE16		0x0400	/* Sanity timer default on	*/
#define QE_NOCAR		0x0800	/* No carrier			*/
#define QE_LOSS			0x1000	/* Loss of carrier while xmit	*/
/*	word2								*/
#define QE_TDR			0x3fff	/* Time domain reflectometry	*/

/*
 * General constant definations
 */
#define QEALLOC 		0	/* Allocate an mbuf		*/
#define QENOALLOC		1	/* No mbuf allocation		*/
#define QEDEALLOC		2	/* Release an mbuf chain	*/

#define QE_NOTYET		0x8000	/* Descriptor not in use yet	*/
#define QE_INUSE		0x4000	/* Descriptor being used by QNA	*/
#define QE_MASK			0xc000	/* Lastnot/error/used mask	*/
