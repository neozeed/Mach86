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
 *	@(#)if_cssreg.h	6.2 (Berkeley) 6/8/85
 */

/* 
 * DEC/CSS IMP11-A ARPAnet interface
 */

struct cssdma {
	short	wc;		/* word count */
	u_short	ba;		/* bus address (low 16 bits) */
	short	csr;		/* status register */
	short	db;		/* data buffer*/
};

struct cssdevice {
	struct	cssdma	css_output;	/* transmit DR11-B */
	struct	cssdma	css_hole;	/* unclever gap */
	struct	cssdma	css_input;	/* receive DR11-B */
};

#define css_icsr        css_input.csr
#define css_iba         css_input.ba
#define css_iwc         css_input.wc
#define css_ocsr        css_output.csr
#define css_oba         css_output.ba
#define css_owc         css_output.wc

/*
 * Bits Common to both input and out CSR's
 */
#define CSS_ERR         0x8000          /* error present */
#define CSS_NXM         0x4000          /* non-existant memory */
#define	CSS_ATTN	0x2000		/* attention */
#define	CSS_MAINT	0x1000		/* maintenance mode */
#define	CSS_CYCLE	0x0100		/* force bus cycle */
#define CSS_RDY         0x0080          /* ready */
#define CSS_IE          0x0040          /* interrupt enable */
#define	CSS_XA		0x0030		/* extended address bits */
#define	CSS_CLR		0x0020		/* clear status (reset) */
#define CSS_GO          0x0001          /* start operation */

/*
 * Input Control Status Register
 */
#define IN_EOM          0x0800          /* end-of-message recieved */
#define IN_IMPNR	0x0400          /* IMP not ready */
#define IN_RLE          0x0200          /* ready line error */
#define IN_WEN          0x0008          /* write enable */
#define IN_HRDY         0x0004          /* host ready */

#define CSS_INBITS \
"\20\20ERR\17NXM\16ATTN\15MAINT\14EOM\13IMPNR\12RLE\11CYCLE\10RDY\7IE\6XBA17\5XBA16\4WE\3HRDY\2CLR\1GO"


/*
 * Output Control Status Register
 */
#define OUT_TXEC	0x0008          /* tx error clear */
#define OUT_ENLB	0x0004          /* enable last bit */

#define CSS_OUTBITS \
"\20\20ERR\17NXM\16ATTN\15MAINT\11CYCLE\10RDY\7IE\6XBA17\5XBA16\4TXEC\3ENLB\2CLR\1GO"
