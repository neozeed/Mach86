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
/* $Header: lopckrt0.s,v 4.0 85/07/15 00:44:39 ibmacis GAMMA $ */
/* $Source: /ibm/acis/usr/sys_ca/ca/RCS/lopckrt0.s,v $ */

	.text

 #
 #  program check level 0 interrupt service routine
 #
 #
 #  assumptions:  All the interesting (ones passed to trap.c) bits in the
 #       storage exception register are included in the lower order 11 (the
 #       number of bits required for hardware page offsets).
 #
 # r10 = mcs_pcs	value with (possibly) simulated bit values for certain
 #		conditions.
 # r11 = exception information
 #
	.align	2
pck_vec1:
	.long	pck1 -real0		# New IAR
	.short	NOTRANS_ICS+INTMASK_ICS+INT_PRI0,0	# New ICS
        .using real0,r0
eye_catcher(pck0):
#ifdef 	notdef
#if	ROMP_CACHE
        stm   r10,low_save10    # copy interrupt time rx into low_save var
	cau   r15,(ROSEBASE)>>16(r0) # i/o base address of Rosetta
	oil   r15,r15,(ROSEBASE)&0xffff
        ior   r14,ROSE_SER(r15) # r14 = storage exception register
	nilz  r14,r14,RTA_EX_FAULT
	jz    fallthrough
	ior   r14,ROSE_SEAR(r15)
	nilz  r10,r14,~RTA_VPAGE_BITS	#r10 is shifted seg reg #
	sri   r10,
	ior   r13,ROSE_TCR(r15)
	nilz  r13,r13,TCR_S
	jz    shift2:
	sri   r14,1	# one extra bit to shift for 4k pages
shift2: sri   r14,0x8   #shift three bits less than necessary due to 8 byte
			#table entry size (since this will be an offset)
	nilz  r13,r14,(0xf)<<3 #r13 is low 4 bits of page number << 3
	sri   r14,RTA_VPAGE_BITS - 4
	nilz  r14,r14,0xf0 #r14 is low 4 bits of seg number << 7
	o     r14,r13	#r14 is hash displacement
 #1 cycle (can't touch r13 for a while)
	l   r13,cache(r14) #since in absoulte mode, use base as displacement
 #1 cycle (can't touch r12 for a while)
	l   r12,cache+1(r14) # load physaddr (can't hurt)
 #1 cycle (can't touch r11 for a while)
	l   r11,hatipt	#physical address of hat/ipt
 #1 cycle 
	sli r10,1	#r10 is now sid
 #1 cycle
	sri r14,3	#shift off 8byte displacement zeroes.
 #r13 is now completely loaded
 #1 cycle 
	c   r14,r13	#compare addrtags
 #r12 is now completely loaded
 #1 cycle if not hash fault
	jne fallthrough #addrtags don't match, handle as normal fault
 #r11 is now completely loaded
	sri 
	

	
fallthrough:
	lm    
#endif	ROMP_CACHE
#endif	notdef
	b	0f	#this instruction is necessary for certain page-faults
			#that will result in a "death-by-88" without it.
0:
        stm   r10,low_save10    # copy interrupt time rx into low_save var
        mfs   scr_mcspcs,r10    # r10 = program check status
	lps   1,pck_vec1	# get off program check level ASAP
pck1:
        st    r1,low_save1      # copy interrupt time r1 into low_save var

        nilz  r11,r10,PCS_UNKNOWN+PCS_TRAP+PCS_PRIV_I+PCS_BAD_I+PCS_RESERVED
        bnz   pck_notstg        # branch if any of those bad guys

	cau   r15,(ROSEBASE)>>16(r0) # i/o base address of Rosetta
	oil   r15,r15,(ROSEBASE)&0xffff
        ior   r14,ROSE_SER(r15) # r14 = storage exception register
#if	ROMP_DEBUG
	st	r14,_ser	# save ser
#endif	ROMP_DEBUG
        niuo  r13,r14,RTA_EX_OHOH_UPPER  # unexpected upper bits
        nilo  r13,r13,RTA_EX_OHOH_LOWER  # unexpected lower bits
        bnz   pck_debug          # branch if any unexpected bits are on

#if	MACH_VM
/*
 *	Code to set the RTA_EX_HACK bit in r14 if the RTA_EX_FAULT bit is
 *	set.  This is because the fault bit is overwritten by the page number
 *	and never makes it to the trap routine where it is needed.  This
 *	should be done in some better way...
 */
	mttbil r14,19-16	#get RTA_EX_LOAD bit
	mftbil r14,29-16	#copy it into RTA_EX_HACK
#endif	MACH_VM

	nilo   r14,r14,~RTA_EX_CECC # forget about correctable ecc errors

    # Following added by mjb to deal with lm and stm data faults.
    # Maybe I even got it right this time.
	nilz  r13,r14,RTA_EX_MULTX  # Multiple exceptions indicated?
	jz    pck05		# Nope.
        mttbil r10,30-16        # d-fetch vs i-fetch bit
        bntb  pck_debug          # jump if i-fetch -- this is not normal
	nilo  r14,r14,0xFFFF-RTA_EX_MULTX # Let's not get confused
pck05:
    # End of lines added by mjb to deal with lm and stm exceptions.

        ior   r12,ROSE_SEAR(r15)# r12 = storage exception address register
#if	ROMP_DEBUG
	st	r12,_sear	# save for debugging
#endif 	ROMP_DEBUG
        mttbil r10,30-16        # d-fetch vs i-fetch bit
        jtb   pck10             # jump if d-fetch (SEAR correct)
        l     r12,pck_ps+old_iar# r12 = iar at time of fault
        nilz  r13,r12,PAGESIZE-1# r13 = page offset
        ci    r13,PAGESIZE-6    # if inst (or subject of bx) w/in page
        jl    pck10             # then jump (iar in faulting page)

        iow   r12,ROSE_CRA(r15) # compute real addr of lower page
        ior   r13,ROSE_TRAR(r15)# r13 = translated real addr register
        cis   r13,0             # if high bit is on, translation failed
        jl    pck10             # then jump (lower page faulted)
        ais   r12,6             # else put iar into higher page
pck10:
 #***    nilo  r12,0-PAGESIZE    | zero page offset bits in exception address
        nilo  r12,r12,0-PAGESIZE    # zero page offset bits in exception address
        nilz  r11,r14,PAGESIZE-1# zero all but page offset bits in exception reg
        o     r11,r12           # fold interesting exception bits into pg offset
	s	r14,r14		# zippo
	iow   r14,ROSE_SER(r15) # Clear the exception register
        b     pck_fault         # go handle page fault
 #
 #  program check, but not a storage fault
 #  if debugger present and in kernel mode pass to the debugger
 #  otherwise pass to 'trap'.
 #
pck_notstg:
#ifdef RDB
	l	r11,pck_ps+old_ics_cs # get the ics into r11
	mttbiu	r11,PROBSTATE-16	#copy problem state bit from old ics
	jntb	pck_debug	#transfer control to debugger if not user mode
#endif
         mttbil r10,26-16        # if not a trap inst (bit 26 from PCS)
         jntb  pck_real          # then jump
	 cau   r10,(BKPT)>>16(r0)  # mcs_pcs value for breakpoint inst "check"
	 oil   r10,r10,(BKPT)&0xffff
 #
 #  real program checks - call fault to handle
 #
pck_real:
	lis r11,0             # no additional information required
	b	pck_fault	# handle as a fault
 #
 # pass control to debugger if it is present
 #
pck_debug:
#ifndef RDB
	j	pck_real
#endif
         lm    r10,low_save10    # restore regs
pck_jump_to_dan: b real0         # jump to original owner of program check
