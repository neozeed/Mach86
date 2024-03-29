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
 *	@(#)if_accreg.h	6.2 (Berkeley) 6/8/85
 */

/*
 * ACC LH/DH-11 interface
 */

struct accdma {
	short	csr;	/* control and status */
	short	db;	/* data buffer */
	u_short	ba;	/* buss address */
	short	wc;	/* word count */
};

struct accdevice {
	struct	accdma input;
	struct	accdma output;
};

#define	icsr	input.csr
#define	iba	input.ba
#define	iwc	input.wc
#define	ocsr	output.csr
#define	oba	output.ba
#define	owc	output.wc

/*
 * Bits Common to both input and out CSR's
 */
#define	ACC_ERR		0x8000		/* error present */
#define	ACC_NXM		0x4000		/* non-existant memory */
#define	ACC_RDY		0x0080		/* ready */
#define	ACC_IE		0x0040		/* interrupt enable */
#define	ACC_RESET	0x0002		/* reset interface */
#define	ACC_GO		0x0001		/* start operation */

/*
 * Input Control Status Register
 */
#define IN_EOM		0x2000		/* end-of-message recieved */
#define IN_HRDY		0x0800		/* host ready */
#define IN_IMPBSY	0x0400		/* IMP not ready */
#define IN_RMR		0x0200		/* receive master ready error */
#define IN_IBF		0x0100		/* input data buffer full */
#define IN_WEN		0x0008		/* write enable */
#define IN_MRDY		0x0004		/* master ready */

#define ACC_INBITS \
"\20\20ERR\17NXM\16EOM\14HRDY\13IMPBSY\12RMR\11IBF\10RDY\7IE\
\4WEN\3MRDY\2RESET\1GO"

/*
 * Output Control Status Register
 */
#define OUT_TMR		0x0200		/* transmit master ready error */
#define OUT_BBACK	0x0008		/* bus back */
#define OUT_ENLB 	0x0004		/* enable last bit */

#define ACC_OUTBITS \
"\20\20ERR\17NXM\12TMR\10RDY\7IE\4BBACK\3ENLB\2RESET\1GO"
