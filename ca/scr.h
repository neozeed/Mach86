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
/* $Header: scr.h,v 4.3 85/08/28 21:35:56 webb Exp $ */
/* $Source: /ibm/acis/usr/sys_ca/ca/RCS/scr.h,v $ */

#if !defined(lint) && !defined(LOCORE) && defined(RCS_HDRS)
static char *rcsidscr = "$Header: scr.h,v 4.3 85/08/28 21:35:56 webb Exp $";
#endif

/* CPU's system comtrol registers */

#define SCR_TIMER_RESET 6
#define SCR_TIMER 7
#define SCR_TIMER_STATUS 8
#define SCR_MQ 10
#define SCR_MCSPCS 11
#define SCR_IRB 12
#define SCR_IAR 13
#define SCR_ICS 14
#define SCR_CS 15

/* machine check status and program check status as in scr 11 */

#define MCS_RSC_CK     0x8000		/* Storage Channel check */
#define MCS_PARITY_CK  0x2000		/* Parity Check */
#define MCS_INS_TO     0x1000		/* Instruction timeout */
#define MCS_DATA_TO    0x0800		/* Data timeout */
#define MCS_RSC_TO     0x0400		/* Storage Channel timeout */
#define MCS_IO_TRAP    0x0200		/* I/O TRAP */
#define MCS_CHECK      0x0100		/* SIMULATED: machine check (added by locore ) */

#define PCS_KNOWN      0x0080		/* IAR is known */
#define PCS_UNKNOWN    0x0040		/* IAR is not known */
#define PCS_TRAP       0x0020		/* TRAP instruction */
#define PCS_PRIV_I     0x0010		/* Privileged instruction */
#define PCS_BAD_I      0x0008		/* bad instruction */
#define PCS_IADDR      0x0004		/* instruction address problem */
#define PCS_DADDR      0x0002		/* data address problem */
#define PCS_RESERVED   0x0001

#define STEP 0x010000  /* instruction step (added to mcs_pcs by locore) */
#define BKPT 0x020000  /* breakpoint inst (added to mcs_pcs by locore) */
#define VAST 0x040000  /* VAX AST (added to mcs_pcs by locore) */
#define USER 0x800000  /* user mode (added to mcs_pcs by trap.c) */

/* format printing MCPC */
#define MCPCFMT "\20\1RESERVED\2DATA-ADDR\3INSTN-ADDR\4BAD-OPCD\5PRIV-OPCD\6TRAP\7UNKNOWN\10KNOWN\11MACHINE-CHECK\12IO-TRAP\13RSC-TO\14DATA-TO\15INS-TO\16PARITY-CHK\20RSC-CHK\21STEP\22BKPT\23VAST\30USER"

#define ICSCS_ASTON     0x80000000  /* simulated, unpredictable in hardware */
#define ICSCS_RESERVED  0x70000000
#define ICSCS_STGPROT   0x08000000
#define ICSCS_PROBSTATE 0x04000000
#define ICSCS_TRANSLATE 0x02000000
#define ICSCS_INTERRUPT 0x01000000
#define ICSCS_CHECKSTOP 0x00800000
#define ICSCS_REGSETNO  0x00700000
#define ICSCS_INSTSTEP  0x00080000  /* simulated, unpredictable in hardware */
#define ICSCS_PRIORITY  0x00070000
#define ICSCS_HOKEY 0x00000070  /* ad hoc convention used by wait and execve */
#define ICSCS_TESTBIT 0x00000001

#define ICSCS_USERSET 0x0E870000  /* user ICSCS must have these bits set */
#define ICSCS_USERCLR 0xF1780000  /* user ICSCS must have thsts bits cleared */

/* format for printing ics_cs */
#define ICSFMT "\20\20INST-STEP\30CHKSTOP-MASK\31INTR-MASK\32XLATE\33USER\34STG-PROT\35PERIE"
/* format for printing cs also in ics_cs */
#define ICSFMT2 "\20\1TB\2OV\4C-ZERO\5GT\6EQ\7LT\10P-ZERO\20INST-STEP\30CHECK-STOP-MASK\31INTR-MASK\32XLATE\33PROB-STATE\34STG-PROT\35PERIE";	/* full blown-version */

#define TS_ENABLE 0x40
#define TS_ILEVEL 1 /* interrupt level for timer */

#ifdef SBPROTO
#define ROMPHZ 1000 /* # of decrements per second for scr 7 */
#endif

#ifdef SBMODEL
#define ROMPHZ 1024 /* # of decrements per second for scr 7 */
#endif


/* definitions for the CCR (Channel Control Register */

#define CCR	 0xf0008c20	/* the address of the CCR */

/*
 * in order to allow user programs to directly access the Memory 
 * map (at f4xxxxxx) turn on CCR_SMP
 * to access the I/O registers turn on CCR_IMP
 */
#define CCR_DCA	0x80	/* DMA Control Register Access */
#define CCR_DNA 0x40	/* DMA Native Adapter Access */
#define CCR_DKA	0x20	/* DMA Keyboard Access */
#define CCR_IMP	0x10	/* I/O Map Problem state access */
#define CCR_SMP 0x08	/* Storage Map Problem state access */
#define CCR_RFT	0x04	/* Refresh at 1=6.5, 0=14.8 microseconds/ */
#define CCR_RFE	0x02	/* Refresh enable */
#define CCR_RES	0x01	/* Reserved */

#define CCR_DEFAULT	CCR_RFE+CCR_RFT	/* no user access etc. */


#define    CSR        0xF0010800         /* Channel Status Reg.   */
#define CSR_OHOH	0x04ffff00	/* bad bits to have on in CSR */

/*
 * the reason the order is so screwed up is the Volume 1 table is in
 * this order.
  */
#define CSR_PIOERR	0x01000000	/* 24 Programmed I/O error */
/*			0x02000000	/* 25 reserved */
#define CSR_SYSATTN	0x04000000	/* 26 system attention */
#define CSR_SOFTRESET	0x08000000	/* 27 soft reset */

#define CSR_EPOW	0x10000000	/* 28 power fail */
/*			0x20000000	/* 29 reserved */
#define CSR_INTPEND	0x40000000	/* 30 interrupt pending */
#define CSR_EXREP	0x80000000	/* 31 exception reported */

#define CSR_DMA8ERR	0x00010000	/* 16 DMA ERROR ch 8 */
#define CSR_DMA7ERR	0x00020000	/* 17 DMA ERROR ch 7 */
#define CSR_DMA6ERR	0x00040000	/* 18 DMA ERROR ch 6 */
#define CSR_DMA5ERR	0x00080000	/* 19 DMA ERROR ch 5 */

#define CSR_DMA3ERR	0x00100000	/* 20 DMA ERROR ch 3 */
#define CSR_DMA2ERR	0x00200000	/* 21 DMA ERROR ch 2 */
#define CSR_DMA1ERR	0x00400000	/* 22 DMA ERROR ch 1 */
#define CSR_DMA0ERR	0x00800000	/* 23 DMA ERROR ch 0 */

#define CSR_PIORP	0x00000100	/* 8 PIO Request Pending */
#define CSR_PBUSY	0x00000200	/* 9 Planar Busy */
#define CSR_CHRST	0x00000400	/* 10 Channel Reset */
#define CSR_DMAEX	0x00000800	/* 11 DMA Exception */

#define CSR_IOCHK	0x00001000	/* 12 I/O Channel Check */
#define CSR_INVOP	0x00002000	/* 13 Invalid Operation */
#define CSR_PROTECT	0x00004000	/* 14 Protection violation */
#define CSR_PIODMA	0x00008000	/* 15 PIO DMA */

