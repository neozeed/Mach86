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
 *	@(#)if_ubreg.h	6.2 (Berkeley) 6/8/85
 */

/*
 * Device registers and bit meanings
 */

struct undevice {
	short	wcr;	/* word count */
	short	bar;	/* bus address */
	short	csr;	/* control & status (also error & info) */
	short	dar;	/* input and output data register */
};

/* CSR bits */
#define GO	0000001
#define FCN1	0000002		/* three function bits */
#define FCN2	0000004	
#define FCN3	0000010	
#define XBA	0000060		/* extended bus address (16&17) */
#define IE	0000100		/* interrupt enable */
#define RDY	0000200		/* ready */
#define CYCL	0000400		/* cycle */
#define	STATC	0001000		/* Status bit C */
#define	STATB	0002000		/* Status bit B */
#define	STATA	0004000		/* Status bit A */
#define MNT	0010000		/* maintenance */
#define ATTN	0020000		/* attention (from device) */
#define NEX	0040000		/* non-existent memory */
#define ERR	0100000
#define XADD	4

#define	UNBITS	"\10\20ERR\17NEX\16ATTN\15MNT\14STATA\13STATB\12STATC\
\11CYCL\10RDY\7IE\6XBA17\5XBA16\4FCN3\3FCN2\2FCN1\1GO"

/* EIR bits */
#define RF	0000001		/* register flag 1 - verifies EIR */
#define NBST	0000400		/* N - cycle burst */
#define BDLT	0001000		/* burst data late */
#define PERR	0002000		/* parity error */
#define ACLO	0004000		/* power fail */
#define MCYC	0010000		/* multicycle request */
#define ATTN	0020000		/* attention (from device) */
#define NEX	0040000		/* non-existent memory */
#define ERR	0100000

/* Network interface commands */
#define	UNRESET	0
#define	UNRDHDR	FCN1
#define	UNOUT	FCN2
#define	UNRDDG	(FCN2|FCN1)
#define	UNFLUSH	FCN3
#define	UNIDLE	(FCN3|FCN2)

/* Extended status bits (in dar) */
#define RESETACK	1	/* Reset cmd acknowledged */
