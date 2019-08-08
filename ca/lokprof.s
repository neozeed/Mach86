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
/* $Header: lokprof.s,v 4.0 85/07/15 00:44:28 ibmacis GAMMA $ */
/* $Source: /ibm/acis/usr/sys_ca/ca/RCS/lokprof.s,v $ */

	.data
rcsidkprof:	.asciz	"$Header: lokprof.s,v 4.0 85/07/15 00:44:28 ibmacis GAMMA $"
	.text

        #=====> begin kernel profiling code <=====|

        l       r13,kpf_onoff           # kernel profiling on/off switch
        cis     r13,0                   # if kernel profiling not active
        jz      kpf30                   # then jump

 # ????  mfs     scr_ts,r13              | get timer status
 # ????  mttbil  r13,26-16               | check for pending clock interrupt
 # ????  jntb    kpf30                   | jump if interrupt not from clock
	clrsb	scr_ts,26-16		# reset clock interrupt pending bit

	cal16	r13,kpf_base		# base address of kernel profile vector

        lm      r14,LV0+old_iar		# get old ps
	mttbiu	r15,PROBSTATE-16	# if interrupted problem state
	jtb	kpf10			# then jump
	l	r15,_origin$		# base address of locore
	s	r14,r15			# iar -= base of locore
	sri	r14,4			# iar /= 16  (select 64-byte bin)
	nilz	r14,r14,0x3FFC		# iar %= 0x4000
	a	r13,r14			# index into kernel profile vector
kpf10:
        l       r15,0(r13)              # get vector element
        ais     r15,1                   # increment by 1
        st      r15,0(r13)              # update vector element

        l       r13,kpf_ticks           # get ticks to Unix tick
        sis     r13,1                   # decrement by one
        st      r13,kpf_ticks           # update counter
        jle     kpf20                   # jump if time for a Unix tick

        lm      r13,low_save13          # restore registers
        lps     0,LV0+old_iar		# return from interrupt
kpf20:
	# Save Registers and things
	# push registers onto the interrupt stack
	st	r6,iosave6
	l	r6,iosavep
	cal	r6,-IOSAVEL(r6)
	ti	4,r6,iosavea-real0
	st	r6,iosavep
	stm	r0,XIHR0(r6)

	cau     r13,(kpf_hz)>>16(r0)
	oil     r13,r13,(kpf_hz)&0xffff
	st	r13,kpf_ticks		# reset the counter

	# Get the old IAR and ICS/CS
	l	r10,LV1
	l	r11,LV1+old_ics_cs

        j       int1tick                # handle like a level 1 clock interrupt
kpf30:
        #=====> end kernel profiling code <=====|
