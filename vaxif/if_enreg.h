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
 *	@(#)if_enreg.h	6.2 (Berkeley) 6/8/85
 */

/*
 * Xerox experimental ethernet registers.
 *
 * N.B.: status register and device address are read/write,
 * device address is read-only, rest are WRITE ONLY!
 */
struct endevice {
	short	en_owc;		/* output word count (10 bits) */
	short	en_oba;		/* output buffer address */
	short	en_ostat;	/* output control and status */
	short	en_odelay;	/* output start delay, 25usec units  */
	short	en_iwc;		/* input word count */
	short	en_iba;		/* input buffer address */
	short	en_istat;	/* input csr */
	short	en_addr;	/* ~device address (low 8 bits) */
};

/*
 * Control and status bits.
 */
#define EN_IERROR	0x8000		/* CRC error, buf ovflo or overrun */
#define	EN_OERROR	0x8000		/* collision or output underrun */
#define EN_OPDONE	0x0080		/* previous operation completed */
#define EN_IEN		0x0040		/* enable interrupt when DONE */
#define	EN_PROMISCUOUS	0x0002		/* promiscuous, input any packet */
#define EN_GO		0x0001		/* start op bit */

#define	EN_BITS	"\10\20ERR\10OPDONE\7IEN\2PROM\1GO"

#define	spl_enet()	spl5()
