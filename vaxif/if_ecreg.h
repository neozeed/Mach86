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
 *	@(#)if_ecreg.h	6.4 (Berkeley) 8/9/85
 */

/*
 * 3Com Ethernet controller registers.
 */
struct ecdevice {
	short	ec_rcr;		/* Receive Control Register */
	short	ec_xcr;		/* Transmit Control Register */
};

/*
 * Control and status bits -- rcr
 */
#define	EC_SPIE		0x8000		/* set parity interrupt enable */
#define	EC_ASTEP	0x4000		/* increment address counter */
#define	EC_AROM		0x2000		/* 1: Use address ROM, 0: use RAM */
#define	EC_PE		0x2000		/* Parity error */
#define	EC_AWCLK	0x1000		/* address write clock bit */
#define	EC_PIE		0x1000		/* Parity interrupt enable (read) */
#define	EC_ADATA	0x0f00		/* address/filtering */
#define	EC_RDONE	0x0080		/* receive done */
#define	EC_MDISAB	0x0080		/* memory disable */
#define	EC_RINTEN	0x0040		/* receive interrupt enable */
#define	EC_RCLR		0x0020		/* clear RDONE bit */
#define	EC_RWBN		0x0010		/* submit buffer for receive */
#define	EC_RBN		0x000f		/* buffer number */

#define	EC_RBITS	"\10\16PE\15PIE\10RDONE\7RINTEN"

/*
 * Control and status bits -- xcr
 */
#define	EC_JAM		0x8000		/* collision dectected */
#define	EC_JINTEN	0x4000		/* collision interrupt enable */
#define	EC_JCLR		0x2000		/* clear collision detect */
#define	EC_UECLR	0x0100		/* reset controller */
#define	EC_XDONE	0x0080		/* transmit done */
#define	EC_XINTEN	0x0040		/* transmit interrupt enable */
#define	EC_XCLR		0x0020		/* clear XDONE bit */
#define	EC_XWBN		0x0010		/* submit buffer for transmit */
#define	EC_XBN		0x000f		/* buffer number */

#define	EC_XBITS	"\10\20JAM\17JINTEN\10XDONE\7XINTEN"

/*
 * Useful combinations
 */
#define	EC_READ		(0x600|EC_RINTEN|EC_RWBN)
#define	EC_WRITE	(EC_JINTEN|EC_XINTEN|EC_XWBN)
#define	EC_CLEAR	(EC_JINTEN|EC_XINTEN|EC_JCLR)

/*
 * Buffer number definitions
 */
#define	ECTBF		0		/* Buffer for transmit */
#define	ECRLBF		1		/* First buffer for receive */
#define	ECRHBF		15		/* Last buffer for receive */

#define	ECRDOFF		528		/* Packet offset in read buffer */
