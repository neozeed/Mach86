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
/* $Header: async.h,v 4.0 85/07/15 00:39:24 ibmacis GAMMA $ */
/* $Source: /ibm/acis/usr/sys_ca/ca/RCS/async.h,v $ */

#if !defined(lint) && !defined(LOCORE) && defined(RCS_HDRS)
static char *rcsidasync = "$Header: async.h,v 4.0 85/07/15 00:39:24 ibmacis GAMMA $";
#endif

/* Async Register definition and address locations */

struct asydevice {       /*  Async Card ------>    COM1         COM2   */
	u_char txb;      /* Transmit Buffer     0xf00003f8  0xf00002f8 */
	u_char ier;      /* Interupt Enable Reg 0xf00003f9  0xf00002f9 */
	u_char iir;      /* Interupt Ident Reg  0xf00003fa  0xf00002fa */
	u_char lcr;      /* Line Control Reg    0xf00003fb  0xf00002fb */
	u_char mcr;      /* Modem Control Reg   0xf00003fc  0xf00002fc */
	u_char lsr;      /* Line Status Reg     0xf00003fd  0xf00002fd */
	u_char msr;      /* Modem Status Reg    0xf00003fe  0xf00002fe */
};

#define TXB     txb             /* Transmit buffer */
#define RXB     txb             /* Receive Buffer */
#define DLLSB   txb             /* Divisor Latch Least Significant Byte */
#define DLMSB   ier             /* Divisor Latch Most Significant Byte */
#define IER     ier             /* Interrupt Enable Register */
#define IIR     iir             /* Interrupt Identification Register */
#define LCR     lcr             /* Line Control Register */
#define MCR     mcr             /* Modem Control Register */
#define LSR     lsr             /* Line status Register */
#define MSR     msr             /* Modem Status Register */

#ifdef SBPROTO
struct asydevice *card[] = {
	(struct asydevice *) 0xf00003f8,     /* COM1 */
	(struct asydevice *) 0xf00002f8      /* COM2 */
};
#endif

#ifdef SBMODEL
struct asydevice *card[] = {
	(struct asydevice *) 0xf0001230,     /* COM1 */
	(struct asydevice *) 0xf0001234,     /* COM2 */
	(struct asydevice *) 0xf0001238,     /* COM3 */
	(struct asydevice *) 0xf000123c,     /* COM4 */
};
#endif


#define COM1    0               /* Async COM1 */
#define COM2    1               /* Async COM2 */

#define ASYNC_CARD      COM2

struct  asydevice       *async;

/* Line Control Register bit defintions */
#define LCR_WLS		0x03	/* Word Length Select Mask */
#define LCR_5BIT	0x00	/* 5 data bits */
#define LCR_6BIT	0x01	/* 6 data bits */
#define LCR_7BIT	0x02	/* 7 data bits */
#define LCR_8BIT	0x03	/* 8 data bits */
#define LCR_STB		0x04	/* Number of Stop Bits */
#define LCR_PEN		0x08	/* Parity Enable */
#define LCR_EPS		0x10	/* Even Parity Select */
#define LCR_STP		0x20	/* Stick Parity */
#define LCR_SETB	0x40	/* Set Break */
#define LCR_DLAB	0x80	/* Divisor Latch Access Bit */

/* Indexes to baud rate table */
#define B0      0
#define B50     1
#define B75     2
#define B110    3
#define B134    4
#define B150    5
#define B200    6
#define B300    7
#define B600    8
#define B1200   9
#define B1800   10
#define B2400   11
#define B4800   12
#define B7200   13
#define B9600   14
#define B19200  15

/*
   Baud Rate Generator Selection Macro (stored in DLLSB & DLMSB while DLAB
   is set in the LCR)
*/

#define SETBAUD(r, b)   r->LCR |= LCR_DLAB;\
			r->DLLSB = asybaudtbl[b].lsb;\
			r->DLMSB = asybaudtbl[b].msb;\
			r->LCR &= ~LCR_DLAB;

struct asybaudtbl {
	unsigned char msb;
	unsigned char lsb;
}

asybaudtbl [] = {
	/* B0 */        0x00, 0x00,
	/* B50 */       0x09, 0x00,
	/* B75 */       0x06, 0x00,
	/* B110 */      0x04, 0x17,
	/* B134 */      0x03, 0x5B,
	/* B150 */	0x03, 0x00,
	/* B200 */	0x02, 0x40,
	/* B300 */	0x01, 0x80,
	/* B600 */	0x00, 0xC0,
	/* B1200 */	0x00, 0x60,
	/* B1800 */	0x00, 0x40,
	/* B2400 */	0x00, 0x30,
	/* B4800 */	0x00, 0x18,
	/* B7200 */     0x00, 0x10,
	/* B9600 */	0x00, 0x0C,
	/* B19200 */    0x00, 0x06
};

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
#define IIR_MASK        0x07

/* Interupt types and priorities */
#define IIR_RLS         0x06    /* Highest priority (Some error condition) */
#define IIR_RXB         0x04    /* Second pri. (Received data available) */
#define IIR_TXB         0x02    /* Third pri. (Transmitter Holding reg empty) */
#define IIR_MS          0x00    /* Fourth pri. (Some Modem status change) */

/* Interupt Enable Register bit definitions */
#define IER_RDA         0x01    /* Enable Recieved Data available int. */
#define IER_THRE	0x02	/* Enable TX Holding Reg Empty int. */
#define IER_RLS         0x04    /* Enable Receiver line status  int. */
#define IER_MS          0x08    /* Enable Modem status  int. */

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

