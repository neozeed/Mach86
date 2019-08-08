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
/* $Header: pcb.h,v 4.1 85/08/22 11:38:44 webb Exp $ */
/* $Source: /ibm/acis/usr/sys_ca/ca/RCS/pcb.h,v $ */

#if !defined(lint) && !defined(LOCORE) && defined(RCS_HDRS)
static char *rcsidpcb = "$Header: pcb.h,v 4.1 85/08/22 11:38:44 webb Exp $";
#endif

#if	CMU
/***********************************************************************
 * HISTORY
 * 22-Mar-86  Bill Bolosky (bolosky) at Carnegie-Mellon University
 *	Fixed for recursive includes.
 *
 ***********************************************************************
 */
#endif	CMU

#ifndef	_PCB_
#define	_PCB_

/*     pcb.h   6.1     83/07/29        */

/*
 * VAX process control block
 */

#ifndef LOCORE     /* use c version */
struct pcb
{
       int     pcb_ksp;        /* kernel stack pointer */
       int     pcb_esp;        /* exec stack pointer */
       int     pcb_ssp;        /* supervisor stack pointer */
#define pcb_usp pcb_r1         /* user stack pointer */
       int     pcb_r0;
       int     pcb_r1;
       int     pcb_r2;
       int     pcb_r3;
       int     pcb_r4;
       int     pcb_r5;
       int     pcb_r6;
       int     pcb_r7;
       int     pcb_r8;
       int     pcb_r9;
       int     pcb_r10;
       int     pcb_r11;
       int     pcb_r12;
       int     pcb_r13;
       int     pcb_r14;
       int     pcb_r15;
       int     pcb_iar;        /* instruction address */
       int     pcb_icscs;      /* program status longword */
       struct  pte *pcb_p0br;  /* seg 0 base register */
       int     pcb_p0lr;       /* seg 0 length register and astlevel */
       struct  pte *pcb_p1br;  /* seg 1 base register */
       int     pcb_p1lr;       /* seg 1 length register and pme */
/*
 * Software pcb (extension)
 */
       int     pcb_szpt;       /* number of pages of user page table */
       int     pcb_cmap2;
       int     *pcb_sswap;
       int     pcb_sigc[3];
       int	pcb_ccr;	/* value for CCR for this process */
       char	pcb_consdev;	/* which console devices are open for HW access */
       char    pcb_fill[2];	/* Alignment filler. */
};

#define AST_USER 0x80000000    /* VAX AST simulation - changed WEW*/

#define aston() u.u_pcb.pcb_icscs |= AST_USER

#define astoff() u.u_pcb.pcb_icscs &= ~AST_USER
#else  /* for locore use assembler version */
	  PCB_KSP =        0
	  PCB_ESP =        4
	  PCB_SSP =        8
	  PCB_USP =        16
	  PCB_R0 =         12
	  PCB_R1 =         16
	  PCB_R2 =         20
	  PCB_R3 =         24
	  PCB_R4 =         28
	  PCB_R5 =         32
	  PCB_R6 =         36
	  PCB_R7 =         40
	  PCB_R8 =         44
	  PCB_R9 =         48
	  PCB_R10 =        52
	  PCB_R11 =        56
	  PCB_R12 =        60
	  PCB_R13 =        64
	  PCB_R14 =        68
	  PCB_R15 =        72
	  PCB_IAR =        76
	  PCB_ICSCS =      80
	  PCB_P0BR =       84
	  PCB_P0LR =       88
	  PCB_P1BR =       92
	  PCB_P1LR =       96
/*
 * Software pcb extension
 */
	  PCB_SZPT =      100      /* number of user page table pages */
	  PCB_CMAP2 =     104      /* saved cmap2 across cswitch (ick) */
	  PCB_SSWAP =     108      /* flag for non-local goto */
	  PCB_SIGC =      112      /* signal trampoline code */
	  PCB_CCR =	  124      /* ccr value */

AST_USER  = 0x80000000             /* VAX AST simulation for user mode in ics*/
AST_USER_BIT = 0                   /* VAX AST simulation for user mode in ics*/
#endif
#endif	_PCB_
