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
/* $Header: io.h,v 4.0 85/07/15 00:42:49 ibmacis GAMMA $ */
/* $Source: /ibm/acis/usr/sys_ca/ca/RCS/io.h,v $ */

#if !defined(lint) && !defined(LOCORE) && defined(RCS_HDRS)
static char *rcsidrompio = "$Header: io.h,v 4.0 85/07/15 00:42:49 ibmacis GAMMA $";
#endif

#ifdef SBPROTO
#define MASK_ROMP_INT	0x00
#define ROMP_INT_3	0x01
#define ROMP_INT_4	0x02
#define ROMP_INT_5	0x03
#define DISPLAY(value) *(char *) LED_ADDR = value

#endif

#ifdef SBMODEL

/* IRQ0 - IRQ15 BUS Levels */
#define IRQ(n)	n

/* Bit fields for ROMP IO MAP IVRB */
#define IO_INT_7	0x80
#define IO_INT_6	0x40
#define IO_INT_5	0x20
#define IO_INT_4	0x10
#define IO_INT_3	0x08
#define IO_INT_2	0x04
#define IO_INT_1	0x02
#define IO_INT_0	0x01

#define Adr_8259A	0xF0008880	/* first 8259 address (ROMP IRPT 3) */
#define Adr_8259B	0XF00088A0	/* second 8259 address (ROMP IRPT 4) */
#define INIT_8259(adr)	* (char *) adr = 0x12; \
	* (char *) (adr+1) = 0x00	/* send init sequence */
#define INIT_INTR() INIT_8259(Adr_8259A);	\
		INIT_8259(Adr_8259B)	/* initialize both 8259's */
#define POLL_CMD	0x0c		/* poll interrupts */
#define GET_IMR		0x00		/* get interrupt mask register */
#define SET_IMR		0x00		/* set interrupt mask register */
#define GET_ISR		0x0b		/* get In Service Register */
#define GET_IRR		0x0a		/* get Int Request Register */
#define NSEOI_CMD 	0x20	/* non specific end of interrupt */
#define SEOI_CMD 	0x60	/* specific end of interrupt */
#define DISPLAY(value)	display(value)	/* put into front panel display */
#endif	SBMODEL

/* Enable/Disable Interrupts from a Bus I/O level */
#define ENABLE	1
#define DISABLE	0
