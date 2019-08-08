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
 * 5799-CGZ (C) COPYRIGHT IBM CORPORATION  1986
 * LICENSED MATERIALS - PROPERTY OF IBM
 * REFER TO COPYRIGHT INSTRUCTIONS FORM NUMBER G120-2083
 */
/* $Header: asyregs.h,v 5.0 86/01/31 18:07:10 ibmacis ibm42a $ */
/* $Source: /ibm/acis/usr/sys_ca/caio/RCS/asyregs.h,v $ */

#if !defined(lint) && !defined(LOCORE) && defined(RCS_HDRS)
static char *rcsidasydefs = "$Header: asyregs.h,v 5.0 86/01/31 18:07:10 ibmacis ibm42a $";
#endif

/* Async Register definition and address locations */
struct asydevice {
	unsigned char txb;	/* Transmit Buffer at 0xf00003f8 */
#define ASY_RXB txb			/* Receive Buffer */
#define ASY_DLLSB txb		/* Divisor Latch Least Significant Byte */
#define ASY_TXB txb
	unsigned char ier;	/* Interupt Enable Register at 0xf00003f9 */
#define ASY_DLMSB ier		/* Divisor Latch Most Significant Byte */
#define ASY_IER ier
	unsigned char iir;	/* Interupt Identificaton Reg. at 0xf00003fa */
#define ASY_IIR iir
	unsigned char lcr;	/* Line Control Register at 0xf00003fb */
#define ASY_LCR lcr
	unsigned char mcr;	/* Modem Control Register at 0xf00003fc */
#define ASY_MCR mcr
	unsigned char lsr;	/* Line Status Register at 0xf00003fd */
#define ASY_LSR lsr
	unsigned char msr;	/* Modem Status Register at 0xf00003fe */
#define ASY_MSR msr
	unsigned char scr;	/* Scratch register */
#define ASY_SCR scr
};
typedef struct asydevice *ASYDEVICE;

/* Line Control Register bit defintions */
#define LCR_WLS		0x03	/* Word Length Select Mask */
#define LCR_5BIT	0x00	/* 5 data bits */
#define LCR_6BIT	0x01	/* 6 data bits */
#define LCR_7BIT	0x02	/* 7 data bits */
#define LCR_8BIT	0x03	/* 8 data bits */
#define LCR_STB		0x04	/* Select Number of Stop Bits */
#define LCR_PEN		0x08	/* Parity Enable */
#define LCR_EPS		0x10	/* Even Parity Select */
#define LCR_STP		0x20	/* Stick Parity */
#define LCR_SETB	0x40	/* Set Break */
#define LCR_DLAB	0x80	/* Divisor Latch Access Bit */


/* Line Status Registers bit definitions */
#define LSR_DR		0x01	/* Data Ready */
#define LSR_OR		0x02	/* Overrun Error */
#define LSR_PE		0x04	/* Parity Error */
#define LSR_FE		0x08	/* Framing Error */
#define LSR_BI		0x10	/* Break Interrupt */
#define LSR_THRE	0x20	/* Transmitter Holding Register Empty */
#define LSR_TSRE	0x40	/* Tx Shift Register Empty */

/* Interrupt Identification register bit definitions */
#define IIR_PEND	0x01	/* This Bit is 0 when an interupt is pending */
#define IIR_MASK	0x07
/* Interupt types and priorities */
#define IIR_LSR		0x06	/* Highest priority (Some error condition) */
#define IIR_RXB		0x04	/* Second pri. (Received data available) */
#define IIR_TXB		0x02	/* Third pri. (Transmitter Holding reg empty) */
#define IIR_MSR		0x00	/* Fourth pri. (Some Modem status change) */

/* Interupt Enable Register bit definitions */
#define IER_DR		0x01	/* Enable Data Read (available) int. */
#define IER_THRE	0x02	/* Enable TX Holding Reg Empty int. */
#define IER_LSR		0x04	/* Enable Receive line status  int. */
#define IER_MSR		0x08	/* Enable Modem status  int. */

/* Modem Control Register bit definitions */
#define MCR_OFF		0x00	/* Turn off all modem reg (drop line) */
#define MCR_DTR		0x01	/* Data Terminal Ready */
#define MCR_RTS		0x02	/* Request To Send */
#define MCR_OUT1	0x04	/* User-designed output 1 */
#define MCR_OUT2	0x08	/* User-designed output 2 */
#define MCR_LOOP	0x10	/* Loopback diagnostic */

/* Modem Status Register bit definitions */
#define MSR_DCTS	0x01	/* Delta Clear To Send */
#define MSR_DDSR	0x02	/* Delta Data Set Ready */
#define MSR_TERI	0x04	/* Trailing Edge Ring Indicator */
#define MSR_DRLSD	0x08	/* Delta RX Line Signal Detect */
#define MSR_CTS		0x10	/* Clear To Send */
#define MSR_DSD		0x20	/* Data Set Ready */
#define MSR_RI		0x40	/* Ring Indicator */
#define MSR_RLSD	0x80	/* Receive Line Signal Detect */

/* SET TO 2 TO MATCH TTY MODULES FOR NOW... (mjb) */
#define ASYSPL()	spl5()
#define PORT_A	0x01
#define PORT_B	0x02
#define PORT_C	0x04
#define PORT_D	0x08
