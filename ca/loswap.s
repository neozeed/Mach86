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
/* $Header: loswap.s,v 4.2 85/09/16 16:42:59 webb Exp $ */
/* $Source: /ibm/acis/usr/sys_ca/ca/RCS/loswap.s,v $ */

	.data
rcsidloswap:	.asciz	"$Header: loswap.s,v 4.2 85/09/16 16:42:59 webb Exp $"
	.text

 #
 #  swap_sids( x, y )  swaps the sids of procs x and y
 #    exchange virtual memory spaces, except for uareas
 #
         .globl _swap_sids
eye_catcher(_swap_sids):
1:       .using 1b                     # tell assembler no base register
         setsb scr_ics,INTMASK-16      # no interrupts, please
         stm   r6,6-16*4(sp)           # save caller's registers

         lh    r4,P_SID1(r2)           # r4 = x->p_sid1
         ls    r5,P_ADDR(r2)           # r5-> pte for 1st upage of x
	 cau   r6,(UAREA_PAGE)>>16(r0)
	 oil   r6,r6,(UAREA_PAGE)&0xffff
         bali  r15,zzipte              # find ipte pointing to ipte on chain
         cas   r8,r0,r0                # r8-> ipte link field to ipte

         lh    r4,P_SID1(r3)           # r4 = y->p_sid1
         ls    r5,P_ADDR(r3)           # r5-> pte for 1st upage of y
	 cau   r6,(UAREA_PAGE)>>16(r0)
	 oil   r6,r6,(UAREA_PAGE)&0xffff
         bali  r15,zzipte              # find ipte pointing to ipte on chain
         cas   r9,r0,r0                # r9-> ipte link field to ipte

         bali  r15,zzswap              # swap ipte entries

         lh    r4,P_SID1(r2)           # r4 = x->p_sid1
         ls    r5,P_ADDR(r2)           # r5-> pte for 1st upage of x
         ais   r5,4                    # r5-> pte for 2nd upage of x
	 cau   r6,(UAREA_PAGE+1)>>16(r0)
	 oil   r6,r6,(UAREA_PAGE+1)&0xffff
         bali  r15,zzipte              # find ipte pointing to ipte on chain
         cas   r8,r0,r0                # r8-> ipte link field to ipte

         lh    r4,P_SID1(r3)           # r4 = y->p_sid1
         ls    r5,P_ADDR(r3)           # r5-> pte for 1st upage of y
         ais   r5,4                    # r5-> pte for 2nd upage of y
	 cau   r6,(UAREA_PAGE+1)>>16(r0)
	 oil   r6,r6,(UAREA_PAGE+1)&0xffff
         bali  r15,zzipte              # find ipte pointing to ipte on chain
         cas   r9,r0,r0                # r9-> ipte link field to ipte

         bali  r15,zzswap              # swap ipte entries

	 cau   r6,(RTA_SEGREG0)>>16(r0)
	 oil   r6,r6,(RTA_SEGREG0)&0xffff

         lh    r4,P_SID0(r2)           # r4 = x->p_sid0
         lh    r5,P_SID0(r3)           # r5 = y->p_sid0
         sth   r5,P_SID0(r2)           # x->p_sid0 = r5
         sth   r4,P_SID0(r3)           # y->p_sid0 = r4
         sli   r5,2                    # (Special,Key) = (0,0)
#ifndef VTL_RELOCATE
         oiu   r5,r5,1                    # (Present) = (1)
#endif
         iow   r5,0(r6)                # reload seg reg 0

         lh    r4,P_SID1(r2)           # r4 = x->p_sid1
         lh    r5,P_SID1(r3)           # r5 = y->p_sid1
         sth   r5,P_SID1(r2)           # x->p_sid1 = r5
         sth   r4,P_SID1(r3)           # y->p_sid1 = r4
#ifndef VTL_RELOCATE
	 cau   r7,(UAREA)>>16(r0)
	 oil   r7,r7,(UAREA)&0xffff
	 cau   r8,(RTA_INV_ADDR)>>16(r0)
	 oil   r8,r8,(RTA_INV_ADDR)&0xffff
         iow   r7,0(r8)                # blast tlb entry for current sid 1st pg
         ai    r7,r7,PAGESIZE             # r7-> 2nd page in u area
         iow   r7,0(r8)                # blast tlb entry for current sid 2nd pg
#endif
         sli   r5,2                    # (Special,Key) = (0,0)
#ifndef VTL_RELOCATE
         oiu   r5,r5,1                    # (Present) = (1)
#endif
         iow   r5,RTA_SEGREGSTEP(r6)   # reload seg reg 1
#ifdef VTL_RELOCATE
	 cau   r4,(RTA_TLBUNUSED)>>16(r0)
	 oil   r4,r4,(RTA_TLBUNUSED)&0xffff
	 cau   r5,(UAREA_TLB * RTA_TLBSTEP + RTA_TLBAW1)>>16(r0)
	 oil   r5,r5,(UAREA_TLB * RTA_TLBSTEP + RTA_TLBAW1)&0xffff
                                       # tlb for 1st page of uarea (A side)
         iow   r4,0(r5)                # invalidate tlb for 1st page
         iow   r4,RTA_TLBSTEP(r5)      # invalidate tlb for 2nd page
	 cau   r5,(UAREA_TLB * RTA_TLBSTEP + RTA_TLBBW1)>>16(r0)
	 oil   r5,r5,(UAREA_TLB * RTA_TLBSTEP + RTA_TLBBW1)&0xffff
                                       # tlb for 1st page of uarea (B side)
         iow   r4,0(r5)                # invalidate tlb for 1st page
         iow   r4,RTA_TLBSTEP(r5)      # invalidate tlb for 2nd page
#else
         iow   r7,0(r8)                # blast tlb entry for new sid 2nd page
         ai    r7,r7,-PAGESIZE             # r7-> 1st page in u area
         iow   r7,0(r8)                # blast tlb entry for new sid 1st page
#endif
         lm    r6,6-16*4(sp)           # restore caller's registers
         brx   r15                     # return to caller
         clrsb scr_ics,INTMASK-16      # interrupts ok now

 #  Routine to find the link in the hatipt to a specified page
 #
 #   r4 = sid fields from proc struct
 #   r5-> pte for specified page
 #   r6 = virtual page number for specified page

zzipte:
         lh    r0,2(r5)                # r0 = page frame number from pte
         x     r4,r6                   # r4 = r4 xor r6 (hash function)
	bali	r14,1f			# r14-> next instruction
1:	.using	1b,r14			# tell assembler
	l	r5,_RTA_HASHMASK	# get significant bits mask
	n	r4,r5			# r4 = hash anchor value
	sli	r4,LOG2HATIPTSIZE	# convert to offset
	l	r6,_RTA_HATIPT		# get address of HATIPT table
1:	.using	1b			# done with this
         a     r4,r6                   # r4-> hatipt entry heading hash chain

         lh    r5,IPTHATPTR(r4)        # get 1st frame index
#ifdef ROSETTA_0
         nilo  r5,r5,RTA_HATIPT_PTRUNBUG  # clear bogus bits
#endif
         mttbil r5,16-16               # get index invalid bit
         jtb   zzpanic                 # jump if not valid
         c     r0,r5                   # if not frame wanted
         jne   zznext                  # then jump
         ai    r4,r4,IPTHATPTR         # r4-> hatipt link to ipte
         brx   r15                     # return to caller...
         cas   r0,r4,r0                # with r0-> hatipt link to ipte
zznext:
         sli   r5,4                    # r5 *= 16 (offset into hatipt)
         cas   r4,r5,r6                # r4-> next hatipt entry on chain
         lh    r5,IPTIPTPTR(r4)        # get next frame index
#ifdef ROSETTA_0
         nilo  r5,r5,RTA_HATIPT_PTRUNBUG  # clear bogus bits
#endif
         mttbil r5,16-16               # get index invalid bit
         jtb   zzpanic                 # branch if page fault
         cl    r0,r5                   # if 1st frame index not it
         jne   zznext                  # then jump
         ai    r4,r4,IPTIPTPTR            # r4-> hatipt link to ipte
         brx   r15                     # return to caller...
         cas   r0,r4,r0                # with r0-> hatipt link to ipte
zzpanic:
         b     zzpanic                 # do something better here

 #  Routine to swap two uarea page entries in the hatipt
 #   r8-> link field in ipte entry that locates a uarea page
 #   r9-> link field in ipte entry that locates a uarea page

zzswap:
         bali  r14,1f                  # r14-> next instruction
1:	 .using 1b,r14                 # tell assembler
         lps   0,zzswun_ps             # goto zzswun with translate off
zzswun:
         .using real0,r0
	 cau   r0,(SYS_ORG)>>16(r0)
	 oil   r0,r0,(SYS_ORG)&0xffff
         s     r8,r0                   # convert virtual to real address
         s     r9,r0                   # convert virtual to real address
         lh    r4,0(r8)                # frame index for page A
         lh    r5,0(r9)                # frame index for page B
         sths  r5,0(r8)                # change link from A to B
         sths  r4,0(r9)                # change link from B to A
#ifdef ROSETTA_0
         nilo  r4,r4,RTA_HATIPT_PTRUNBUG  # clear bogus bits
         nilo  r5,r5,RTA_HATIPT_PTRUNBUG  # clear bogus bits
#endif
         sli   r4,4                    # r4 *= 16 (offset into hatipt)
         sli   r5,4                    # r5 *= 16 (offset into hatipt)
         l     r0,hatipt               # r10-> hatipt
         a     r4,r0                   # r4-> hatipt entry for A page
         a     r5,r0                   # r5-> hatipt entry for B page

         ls    r6,IPTADDRTAG(r4)       # r6 = addrtag for A page
         ls    r7,IPTADDRTAG(r5)       # r7 = addrtag for B page
         sts   r7,IPTADDRTAG(r4)       # change addrtag for A page
         sts   r6,IPTADDRTAG(r5)       # change addrtag for B page

         lh    r6,IPTIPTPTR(r4)       # r6 = ipt chain ptr for A page
         lh    r7,IPTIPTPTR(r5)       # r7 = ipt chain ptr for B page
         sths  r7,IPTIPTPTR(r4)       # change ipt chain ptr for A page
         sths  r6,IPTIPTPTR(r5)       # change ipt chain ptr for B page

         lps   0,zzswxl_ps             # goto zzswun with translate on
zzswxl:
         br    r15                     # return to caller
1:       .using 1b                     # tell assembler no base register
