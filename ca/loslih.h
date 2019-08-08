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
/* $Header: loslih.h,v 4.0 85/07/15 00:44:59 ibmacis GAMMA $ */
/* $Source: /ibm/acis/usr/sys_ca/ca/RCS/loslih.h,v $ */

	.data
rcsidloslih:	.asciz "$Header"
	.text

 #
 # table defining the SLIH's for each interrupt level
 # in some cases the SLIH will determine the source of an interrupt
 # and then call the appropriate SLIH for that particular source
 # this allows the SLIH to service serveral requests at that level before
 # returning to the FLIH.
 #
#define IOSLICS	4	/* ICS value */
#define DEVSLIH 0	/* the SLIH address */
#define DEVDEP	8	/* the INFO */


#define SLIH_TABLE(level,routine,ics,info) .globl slih_/**/level; \
	.align	2; \
slih_/**/level: .int routine; \
	.short ics, 0; \
	.int	info


SLIH_TABLE(0,_panic,TRANS_ICS+INT_PRI0,INT0MSG)			/* level 0 */
SLIH_TABLE(1,_hardclock,TRANS_ICS+INT_PRI1,0)			/* level 1 */
SLIH_TABLE(2,_panic,TRANS_ICS+INT_PRI2,INT2MSG)			/* level 2 */
SLIH_TABLE(3,_int_8259,TRANS_ICS+INT_PRI3,_int3table)		/* level 3 */
SLIH_TABLE(4,_int_8259,TRANS_ICS+INT_PRI4,_int4table)		/* level 4 */
SLIH_TABLE(5,_devunk,TRANS_ICS+INT_PRI5,0)			/* level 5 */
SLIH_TABLE(6,_panic,TRANS_ICS+INT_PRI6,INT6MSG)			/* level 6 */
SLIH_TABLE(6soft,_slih6,TRANS_ICS+INT_PRI6,0)			/* level 6 */
SLIH_TABLE(mc,_panic,TRANS_ICS+INT_PRI0,INTMCMSG)		/* machine check */

 # INT0MSG: .ascil "Illegal External Interrupt ZERO"
 # INT2MSG: .ascil "Illegal Interrupt two"
 # INT6MSG: .ascil "Illegal External Interrupt SIX"
 # INTMCMSG: .ascil "Illegal Machine Check routine"
INT0MSG: .ascii "Illegal External Interrupt ZERO\0"
INT2MSG: .ascii "Illegal Interrupt two\0"
INT6MSG: .ascii "Illegal External Interrupt SIX\0"
INTMCMSG: .ascii "Illegal Machine Check routine\0"
