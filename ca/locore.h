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
/* $Header: locore.h,v 4.0 85/07/15 00:43:31 ibmacis GAMMA $ */
/* $Source: /ibm/acis/usr/sys_ca/ca/RCS/locore.h,v $ */

	.data
rcsidlocore_h:	.asciz "$Header: locore.h,v 4.0 85/07/15 00:43:31 ibmacis GAMMA $"
	.text

#define eye_catcher(name) .ascii "<name>";.align 2; name
real0:	# this had better be the first location in .text!
 #
 #        interrupt vectors locations
 #
.set int0_ps  , 0x100 + real0
.set int1_ps  , 0x110 + real0
.set int2_ps  , 0x120 + real0
.set int3_ps  , 0x130 + real0
.set int4_ps  , 0x140 + real0
.set int5_ps  , 0x150 + real0
.set int6_ps  , 0x160 + real0
.set mck_ps   , 0x170 + real0
.set pck_ps   , 0x180 + real0
.set svc_ps   , 0x190 + real0
 #
 #  System Register equates:
 #
#define scr_ctrsrc	 r6		/* system control reg: counter src */
#define scr_ctr		 r7		/* counter */
#define scr_ts		 r8		/* timer status */
#define scr_mq		 r10		/* multiplier/quotient */
#define scr_mcspcs	 r11		/* machine check/program check stat. */
#define scr_irb		 r12		/* interrupt request buffer */
#define scr_iar		 r13		/* instruction address register */
#define scr_ics		 r14		/* interrupt control status */
#define scr_cs		 r15		/* condition status */

 #  byte offsets into the 16 byte old/new ps pairs kept by Romp at 0x100

.set old_iar    , 0                   # offset for old-iar field
.set old_ics_cs , 4                   # offset for old ics + cs field
.set new_iar    , 8                   # offset for new iar field
.set new_ics    , 12                  # offset for new ics field
.set svc_code   , 14                  # offset for svc code field (svc only)

 # r perie stg-prot prob-state transl-mode int-mask chkstp-mask reg r pri

 #000  0     1          0          1          0         1       000 0 000
.set TRANS_ICS , 0x0A80               # translate mode on
 #000  0     0          0          0          0         1       000 0 000
.set NOTRANS_ICS , 0x0080             # translate mode off
 #000  0     0          0          0          1         0       000 0 000
.set INTMASK_ICS , 0x0100             # interrupts masked
 #000  0     0          1          0          0         0       000 0 000
.set PROBSTATE_ICS	, 0x0400	# problem state, level 0

.set PROBSTATE	, 21		# ICS bit number for problem state
.set INTMASK	, 23		# ICS bit number for interrupt mask
.set INSTSTEP	, 28		# ICS bit number for simulated inst step

 #  Interrupt priorities
#ifdef RDB
.set INT_PRI0	, 0x0001	# use level 1 so that debugger has chance
#else
.set INT_PRI0	, 0x0000
#endif
.set INT_PRI1	, 0x0001
.set INT_PRI2	, 0x0002
.set INT_PRI3	, 0x0003
.set INT_PRI4	, 0x0004
.set INT_PRI5	, 0x0005
.set INT_PRI6	, 0x0006
.set INT_PRI7	, 0x0007

 #  interrupt request buffer bits

.set IRB_IRQ_0 , 16-16
.set IRB_IRQ_1 , 17-16
.set IRB_IRQ_2 , 18-16
.set IRB_IRQ_3 , 19-16
.set IRB_IRQ_4 , 20-16
.set IRB_IRQ_5 , 21-16
.set IRB_IRQ_6 , 22-16

 #	BUS I/O ADDRESSES
.set IOBASE	,	0xF000 ^ 0x8000 - 0x8000
#ifdef SBPROTO
.set IMRA	,	0x002C
.set IMRB	,	0x042C
.set IMRC	,	0x082C
.set IMRD	,	0x0C2C
.set IVRA	,	0x102C
.set IVRB	,	0x142C
#endif

#ifdef SBMODEL
.set A8259	,	0x8880
.set B8259	,	0x88a0
.set NSEOI_CMD	,	0x20	/* non specific end of interrupt */
.set SEOI_CMD	,	0x60	/* specific end of interrupt */
.set POLL_CMD	,	0x0c	/* poll for next interupt */
#endif

 # Equates for saveing REG's on INTERUPT STACK at INTERUPT TIME
.set XIHMASK	,	0  * 4
.set XIHR0	,	1  * 4
.set XIHR1	,	2  * 4
.set XIHR2	,	3  * 4
.set XIHR3	,	4  * 4
.set XIHR4	,	5  * 4
.set XIHR5	,	6  * 4
.set XIHR6	,	7  * 4
.set XIHR7	,	8  * 4
.set XIHR8	,	9  * 4
.set XIHR9	,	10 * 4
.set XIHR10	,	11 * 4
.set XIHR11	,	12 * 4
.set XIHR12	,	13 * 4
.set XIHR13	,	14 * 4
.set XIHR14	,	15 * 4
.set XIHR15	,	16 * 4
.set XIHSR0	,	17 * 4
.set XIHMQ	,	18 * 4
 # WARNING - WARNING - WARNING: If you change anything which affects the value
 # of IOSAVEL here and you are compiling with MACH_VM on (which you should be
 # doing!) then you MUST fix the corresponding definition in ../ca/pmap.h or
 # it will die as soon as the first copy{in,out}{,str} occurs!
 #
.set IOSAVEL ,	XIHMQ + 4


 #	INITIAL IO INT's MAP SETTINGS TO ROMP INT's
.set SETIO	,	0x00000000
.set FLIHICS	,	0x181		# First Level Interrupt's ICS

 #
 #  assume we are relocated at E000XXXX  <======== NOTE: THIS MUST BE CORRECT
 #
 #  Clarification: The segment (E) must match everybody else's idea of which
 #  segment register is used to address the kernel (which in fact occupies
 #  PHYSICAL memory starting at REAL XXXX (0x800 or 0x6800, depending on
 #  whether or not you have the debugger in there or not)). The actual segment
 #  to which the kernel is mapped is 0x200. The 0x6800 is important because it
 #  has to be significantly less than 0x8000 or else you'll have to re-code some
 #  of the data access instructions differently (*sigh* yes, the 16 bit
 #  value in the intruction is SIGN EXTENDED!!!, so values above 7FFF in
 #  untranslated mode refer to REAL ADDRESSES FFFF8000 -> FFFFFFFF which
 #  represent the second half of the IO segment (romp/xt excluded), and
 #  are therefore somewhat less than useful.
 #
.set SYS_ORG , 0xE0000000            # virtual memory address of start of memory

#ifdef SBMODEL
.set LED_ADDR , 0x811000			/* address of LED's */
#define ROS_ADDR	0x800000	/* address of ROS */
#endif
