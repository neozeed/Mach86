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
/* $Header: lointpsw.s,v 4.1 85/08/28 21:34:31 webb Exp $ */
/* $Source: /ibm/acis/usr/sys_ca/ca/RCS/lointpsw.s,v $ */

	.data
rcsidlointpsw:	.asciz	"$Header: lointpsw.s,v 4.1 85/08/28 21:34:31 webb Exp $"
	.text

 # Move up to the NPS interrupt setup area
	.org real0 + 0x100

 #***************************************************************
 #*	LEVEL 0 INTERRUPT OLD/NEW PROGRAM STATUS AREA. 0x100   *
 #***************************************************************
LV0:	.long	0xB0B0B0B0		# Old IAR
	.short	0xB0B0			# Old ICS (SCR 14)
	.short	0xB0B0			# Old CS  (SCR 15)
	.long	int0 - real0		# New IAR
	.short	NOTRANS_ICS+INT_PRI0	# New ICS
	.short	0x0000			# Reserved
 #***************************************************************
 #*	LEVEL 1 INTERRUPT OLD/NEW PROGRAM STATUS AREA. 0x110   *
 #***************************************************************
LV1:	.long	0xB0B0B0B0		# Old IAR
	.short	0xB0B0			# Old ICS (SCR 14)
	.short	0xB0B0			# Old CS  (SCR 15)
	.long	int1 - real0		# New IAR
	.short	NOTRANS_ICS+INT_PRI1	# New ICS
	.short	0x0000			# Reserved
 #***************************************************************
 #*	LEVEL 2 INTERRUPT OLD/NEW PROGRAM STATUS AREA. 0x120   *
 #***************************************************************
LV2:	.long	0xB0B0B0B0			# Old IAR
	.short	0xB0B0				# Old ICS (SCR 14)
	.short	0xB0B0				# Old CS  (SCR 15)
	.long	int2 - real0			# New IAR
	.short	NOTRANS_ICS+INTMASK_ICS+INT_PRI2# New ICS
	.short	0x0000				# Reserved
 #***************************************************************
 #*	LEVEL 3 INTERRUPT OLD/NEW PROGRAM STATUS AREA. 0x130   *
 #***************************************************************
LV3:	.long	0xB0B0B0B0		# Old IAR
	.short	0xB0B0			# Old ICS (SCR 14)
	.short	0xB0B0			# Old CS  (SCR 15)
	.long	int3 - real0		# New IAR
	.short	FLIHICS			# New ICS
	.short	0x0000			# Reserved
 #***************************************************************
 #*	LEVEL 4 INTERRUPT OLD/NEW PROGRAM STATUS AREA. 0x140   *
 #***************************************************************
LV4:	.long	0xB0B0B0B0		# Old IAR
	.short	0xB0B0			# Old ICS (SCR 14)
	.short	0xB0B0			# Old CS  (SCR 15)
	.long	int4 - real0		# New IAR
	.short	FLIHICS			# New ICS
	.short	0x0000			# Reserved
 #***************************************************************
 #*	LEVEL 5 INTERRUPT OLD/NEW PROGRAM STATUS AREA. 0x150   *
 #***************************************************************
LV5:	.long	0xB0B0B0B0		# Old IAR
	.short	0xB0B0			# Old ICS (SCR 14)
	.short	0xB0B0			# Old CS  (SCR 15)
	.long	int5 - real0		# New IAR
	.short	FLIHICS			# New ICS
	.short	0x0000			# Reserved
 #***************************************************************
 #*	LEVEL 6 INTERRUPT OLD/NEW PROGRAM STATUS AREA. 0x160   *
 #***************************************************************
LV6:	.long	0xB0B0B0B0		# Old IAR
	.short	0xB0B0			# Old ICS (SCR 14)
	.short	0xB0B0			# Old CS  (SCR 15)
	.long	int6 - real0		# New IAR
	.short	FLIHICS			# New ICS
	.short	0x0000			# Reserved
 #*********************************************************************
 #*	MACHINE CHECK INTERRUPT OLD/NEW PROGRAM STATUS AREA. 0x170   *
 #*********************************************************************
MACHK:	.long	0xB0B0B0B0		# Old IAR
	.short	0xB0B0			# Old ICS (SCR 14)
	.short	0xB0B0			# Old CS  (SCR 15)
	.long	mck0 - real0		# New IAR
	.short	FLIHICS			# New ICS
	.short	0x0000			# Reserved
 #*********************************************************************
 #*	PROGRAM CHECK INTERRUPT OLD/NEW PROGRAM STATUS AREA. 0x180   *
 #*********************************************************************
PGMCHK:	.long	0xB0B0B0B0		# Old IAR
	.short	0xB0B0			# Old ICS (SCR 14)
	.short	0xB0B0			# Old CS  (SCR 15)
	.long	pck0 -real0		# New IAR
	.short	FLIHICS			# New ICS
	.short	0x0000			# Reserved
 #*********************************************************************
 #*	SVC INTERRUPT OLD/NEW PROGRAM STATUS AREA. 0x190	     *
 #*********************************************************************
SVC:	.long	0xB0B0B0B0		# Old IAR
	.short	0xB0B0			# Old ICS (SCR 14)
	.short	0xB0B0			# Old CS  (SCR 15)
	.long	svc0 - real0		# New IAR
	.short	NOTRANS_ICS+INTMASK_ICS+INT_PRI0	# New ICS
SVCODE:	.short	0x0000			# SVC CODE

 #
 # the Model has a POST (Power On Self Test) table at 0x800 that must
 # be intact for the 'control-alt-pause/break'  
 # to work. The loaders protect this area if one leaves room (zeros)
 # in that area.
 #
 # on the Prototype the debugger lives from 0x800 - 0x6800.
 # on the Model the kernel ownes 0x1000-0x8000 and the debugger
 # owns 0x8000-0x12000.
 #
#ifdef SBMODEL
 # Move past the POST
	.org real0 + 0x1000
#else
#ifdef RDB
 # Move past the debugger area
	.org real0 + 0x6800
#else
 # Move to a nice starting address
	.org real0 + 0x800
#endif RDB
#endif
