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
/* $Header: loutil.s,v 4.8 85/09/16 16:54:34 webb Exp $ */
/* $Source: /ibm/acis/usr/sys_ca/ca/RCS/loutil.s,v $ */

/****************************************************************
 * HISTORY
 * 10-May-86  Bill Bolosky (bolosky) at Carnegie-Mellon University
 *	Merged in definition of xadunload from new revision ibm sources.
 *
 *  8-Apr-86  Bill Bolosky (bolosky) at Carnegie-Mellon University
 *	MACH_VM:	Modified save_context and load_context to work with 
 *		thread pointers instead of process pointers.
 *
 * 28-Feb-86  David Golub (dbg) at Carnegie-Mellon University
 *	Added save_context and load_context.
 *****************************************************************
 */
	.data
rcsidloutil:	.asciz	"$Header: loutil.s,v 4.8 85/09/16 16:54:34 webb Exp $"
	.text


 #  Note that these are NOT the 'spln' routines called by C, these
 #  are the routines that the 'spln' macros invoke (e.g. they have
 #  the UNIX spl numbers mapped into the romp CPU numbers.
 #	  e.g. 'spl0()' --> _spl7()		(WEW)
 #
 #  set interrupt level 0 (highest priority) through level 7 (lowest)
 #    also set interrupt level according to argument
 #    always return previous level
 #
 # when the debugger is present INT_PRI0 is actually 1 (instead of 0)
 # so that the debugger can use level zero interrupt requests to
 # single step the kernel.
 #
         .globl __spl0
	 .globl __spl1
	 .globl __spl2
	 .globl __spl3
	 .globl __spl4
	 .globl __spl5
	 .globl __spl6
	 .globl __spl7
	 .globl _splx
         .globl _splimp
         .globl _splnet
eye_catcher(__spl0):    bx spljoin;   lis r2,INT_PRI0
eye_catcher(__spl1):    bx spljoin;   lis r2,1
eye_catcher(__spl2):    bx spljoin;   lis r2,2
eye_catcher(_splimp):  bx spljoin;   lis r2,2
eye_catcher(__spl3):    bx spljoin;   lis r2,3
eye_catcher(__spl4):    bx spljoin;   lis r2,4
eye_catcher(__spl5):    bx spljoin;   lis r2,5
eye_catcher(__spl6):    bx spljoin;   lis r2,6
eye_catcher(_splnet):  bx spljoin;   lis r2,6
eye_catcher(__spl7):    bx spljoin;   lis r2,7
eye_catcher(_splx):         nilz  r2,r2,7 # get only low 3 bits
 #
 #  common spl routine;  on entry, r2 = new level
 #
spljoin:
          mfs   scr_ics,r0       # get the current status
          ni    r3,r0,-8         # get all but priority in r3
          o     r2,r3            # overlay priority bits
          ni    r0,r0,7             # get only old priority bits
          brx   r15              # return and...
          mts   scr_ics,r2       # set new status
 #   Getspl: (dob) Returns the current ics priority bits
 #           without modifiction of current spl
	.globl _getspl
eye_catcher(_getspl):
	mfs   scr_ics,r0       # get the current status
	brx   r15
	ni    r0,r0,7             # Return only priority bits

 #
 #  I/O routines:  ior(port);  iow(port,value);
 #
         .globl _ior
	 .globl _iow
eye_catcher(_ior):
         brx   r15
         ior   r0,0(r2)          # read data from port

eye_catcher(_iow):
         brx   r15
         iow   r3,0(r2)          # write data to port
 #
 #  system reqister access routines
 #
         .globl _mtsr
eye_catcher(_mtsr):
         sli   r2,2              # r2 = parm 1 * 4
         mfs   scr_iar,r5        # set r5 to address of next inst
1:       ais   r5,mts_vector - 1b# set r5 to address of mts_vector
         a     r5,r2             # set r5 to address of mts_vector entry
         balr  r0,r5             # case statement (set r0 for debug)
mts_vector:                      # each entry must be 4 bytes
         brx   r15;   mts r0,r3  # set scr 00 to 2nd parm |***
         brx   r15;   mts r1,r3  # set scr 01 to 2nd parm |***
         brx   r15;   mts r2,r3  # set scr 02 to 2nd parm |***
         brx   r15;   mts r3,r3  # set scr 03 to 2nd parm |***
         brx   r15;   mts r4,r3  # set scr 04 to 2nd parm |***
         brx   r15;   mts r5,r3  # set scr 05 to 2nd parm |***
         brx   r15;   mts r6,r3  # set scr 06 to 2nd parm |***
         brx   r15;   mts r7,r3  # set scr 07 to 2nd parm |***
         brx   r15;   mts r8,r3  # set scr 08 to 2nd parm |***
         brx   r15;   mts r9,r3  # set scr 09 to 2nd parm |***
         brx   r15;   mts r10,r3  # set scr 10 to 2nd parm |***
         brx   r15;   mts r11,r3  # set scr 11 to 2nd parm |***
         brx   r15;   mts r12,r3  # set scr 12 to 2nd parm |***
         brx   r15;   mts r13,r3  # set scr 13 to 2nd parm |***
         brx   r15;   mts r14,r3  # set scr 14 to 2nd parm |***
         brx   r15;   mts r15,r3  # set scr 15 to 2nd parm |***
         .globl _mfsr
eye_catcher(_mfsr):
         sli   r2,2              # r2 = parm 1 * 4
         mfs   scr_iar,r5        # set r5 to address of next inst
1:       ais   r5,mfs_vector - 1b# set r5 to address of mfs_vector
         a     r5,r2             # set r5 to address of mfs_vector entry
         balr  r0,r5             # case statement (set r0 for debug)
mfs_vector:                      # each entry must be 4 bytes
         brx   r15;   mfs r0,r0  # return scr 00 |***
         brx   r15;   mfs r1,r0  # return scr 01 |***
         brx   r15;   mfs r2,r0  # return scr 02 |***
         brx   r15;   mfs r3,r0  # return scr 03 |***
         brx   r15;   mfs r4,r0  # return scr 04 |***
         brx   r15;   mfs r5,r0  # return scr 05 |***
         brx   r15;   mfs r6,r0  # return scr 06 |***
         brx   r15;   mfs r7,r0  # return scr 07 |***
         brx   r15;   mfs r8,r0  # return scr 08 |***
         brx   r15;   mfs r9,r0  # return scr 09 |***
         brx   r15;   mfs r10,r0  # return scr 10 |***
         brx   r15;   mfs r11,r0  # return scr 11 |***
         brx   r15;   mfs r12,r0  # return scr 12 |***
         brx   r15;   mfs r13,r0  # return scr 13 |***
         brx   r15;   mfs r14,r0  # return scr 14 |***
         brx   r15;   mfs r15,r0  # return scr 15 |***

 #  prepare for a core dump by saving the registers (including stack
 #  pointer) and also the important MM registers
 #  as a favour to the user we will cause the TRAR to point to the
 #  real address of the u area

	.globl _dumpregs
	.set _dumpregs,real0 + 0x400
	.globl _dumpmmregs
	.set _dumpmmregs,real0 + 0x400 + 64
	.globl _scb
	.set _scb,_dumpregs+8	# just so adb user can find our stack
.globl _dumpsave; eye_catcher(_dumpsave):
         mfs   scr_iar,r5       # use r5 as base register
1: 	 .using 1b,r5           # tell assembler
	stm	r0,_dumpregs	# save the registers
	cal	r2,0x19(r0)	# count of MM segment registers SEG0...RAS MODE
	get	r3,$RTA_SEGREG0	# point to seg reg 0
 #	cau	r3,(RTA_SEGREG0)>>16(r0) # point to seg reg 0
 #	oil	r3,r3,(RTA_SEGREG0)&0xffff
	get	r4,$USTRUCT	# get address of U area
	iow	r4,ROSE_CRA(r3) # do the translate
	cal	r4,_dumpmmregs
 # loop thru storing the important MMU registers into lowcore
1:
	ior	r0,0(r3)	# get the registers
	put	r0,0(r4)	# store it
	ais	r4,4		# step pointer
	sis	r2,1
	ais	r3,1
	cis	r2,0
	jne	1b
	br	r15		# just return 
 #
 #  fill the specified real page with binary zeros.  Vax does this with 4 insts:
 #    create a pte, invalidate the tlb, block fill, return.
 #
         .globl _clearseg
eye_catcher(_clearseg):
         mfs   scr_ics,r5        # save caller's ics in r5
         ai    sp,sp,-44             # push stack
         stm   r5,0(sp)          # save caller's registers
         mfs   scr_iar,r15       # use r15 as base register
1: 	 .using 1b,r15           # tell assembler
         lps   0,csegreal_ps     # switch to untranslated mode at level 0

csegreal_ps:
         .long  csegreal-real0   # new iar
         .short NOTRANS_ICS      # new ics: xlate off
         .short 0                # new cs

csegreal:
         .using real0,r0         # tell assembler r0 is base reg
         mfs    scr_ics,r0       # get the current status
         ni     r0,r0,-8            # set processor level bits to zero
         ni     r5,r5,7             # isolate old processor level bits
         o      r0,r5            # set processor level bits to old level
         mts    scr_ics,r0       # set old processor level

	 cau    r0,(PAGESIZE-52)>>16(r0) # # bytes to clear less 52
	 oil    r0,r0,(PAGESIZE-52)&0xffff
         sli    r2,LOG2PAGESIZE  # real address of page to clear
         lis    r3,0             # fill constant
         lis    r4,0             # fill constant
         lis    r5,0             # fill constant
         lis    r6,0             # fill constant
         lis    r7,0             # fill constant
         lis    r8,0             # fill constant
         lis    r9,0             # fill constant
         lis    r10,0            # fill constant
         lis    r11,0            # fill constant
         lis    r12,0            # fill constant
         lis    r13,0            # fill constant
         lis    r14,0            # fill constant
         lis    r15,0            # fill constant
cseg10:
         stm    r3,0(r2)         # zero 52 bytes in page
         ai     r0,r0,-52            # decrement residual byte count
         bpx    cseg10           # loop if more to clear
         ai     r2,r2,52            # increment page address

         ai     r0,r0,52            # set residual byte count
         je     cseg30           # jump if all done
cseg20:
         sts    r3,0(r2)         # zero 4 bytes in page
         sis    r0,4             # decrement residual byte count
         bpx    cseg20           # loop if more to clear
         ais    r2,4             # increment page address
cseg30:
         lps    0,csegxlat_ps    # switch back to translated mode at level 0
csegxlat:
1:       .using 1b               # tell assembler no base register
         lm     r5,0(sp)         # restore caller's registers
         mts    scr_ics,r5       # restore caller's ics (processor level)
         brx    r15              # return to caller
         ai     sp,sp,44            # pop stack
1:       .using 1b
 #
 #  copypage(from,to) -- copy one real page to another real page.
 #
 #         .globl _copypage
 #eye_catcher(_copypage):
 #         subi  sp,40             | push stack
 #         stm   r6,0(sp)          | save caller's registers
 #         mfs   scr_iar,r15       | use r15 as base register
 #         .using .,r15            | tell assembler
 #         lps   0,cpagreal_ps     | switch to untranslated mode
 #
 #         .align 4
 #cpagreal_ps:
 #         .long  cpagreal-real0   | new iar
 #         .hword NOTRANS_ICS      | new ics: xlate off
 #         .hword 0                | new cs
 #
 #cpagreal:
 #         .using real0,r0         | tell assembler r0 is base reg
 #         loadi  r0,PAGESIZE-48   | # bytes to copy less 48
 #         sli    r2,RTA_LOG2PAGESIZE  | real address of page to copy from
 #         sli    r3,RTA_LOG2PAGESIZE  | real address of page to copy to
 #cpag10:
 #         lm     r4,0(r2)         | get next 48 bytes in "from" page
 #         stm    r4,0(r3)         | set next 48 byttes in "to" page
 #         addi   r3,48            | increment "to" page address
 #         subi   r0,48            | decrement residual byte count
 #         bpx    cpag10           | loop if more to clear
 #         addi   r2,48            | increment "from" page address
 #
 #         addi   r0,48            | set residual byte count
 #         je     cpag30           | jump if all done
 #cpag20:
 #         ls     r4,0(r2)         | get next 4 bytes in "from" page
 #         sts    r4,0(r3)         | set next 4 byttes in "to" page
 #         addi   r3,4             | increment "to" page address
 #         subi   r0,4             | decrement residual byte count
 #         bpx    cpag20           | loop if more to clear
 #         addi   r2,4             | increment "from" page address
 #cpag30:
 #         lps    0,cpagxlat_ps    | switch back to translated mode
 #cpagxlat:
 #         .using .                | tell assembler no base register
 #         lm     r6,0(sp)         | restore caller's registers
 #         brx    r15              | return to caller
 #         addi   sp,40            | pop stack
 #         .using .                | tell assembler no base register

 #  setrq - add to runable queue for this priority
 #
 #  setrq(p) struct proc *p;
 #
 #     _qs[pri]             Q                 R
 #      +---------------------------------------+
 #      |                   +-----------------+ |
 #      |                   V                 | V
 #    +-+-+---+           +---+---+         +-+-+---+
 # +->| | | --+---------->| | | --+-------->| | | | |       BEFORE
 # |  +---+---+           +-+-+---+         +---+-+-+
 # |    A                   |                     |
 # |    +-------------------+                     |
 # +----------------------------------------------+
 #
 #                    P
 #                  +---+---+
 #           RLINK  |   |   |  LINK
 #                  +---+---+
 #
 #
 #     _qs[pri]             Q                 R
 #                          +-----------------+
 #                          V                 |
 #    +---+---+           +---+---+         +-+-+---+
 # +->| | | --+---------->| | | --+-------->| | | | |       AFTER
 # |  +-+-+---+           +-+-+---+         +---+-+-+
 # |    |   A               |                 A   |
 # |    |   +---------------+                 |   |
 # |    |             +-----------------------+   |
 # |    |             |   +-----------------------+
 # |    |           P |   |
 # |    |             |   V
 # |    |           +-+-+---+
 # |    +---------->| | | | |
 # |                +---+-+-+
 # +----------------------+

         .globl _setrq
eye_catcher(_setrq):
         sis   r1,3*4               # make room on stack
         stm   r13,0(r1)            # save registers we use
         mfs   scr_iar,r13          # get addressability
1:	 .using 1b,r13              # tell the assembler
         ls    r0,P_RLINK(r2)       # make sure not already on a queue
         cis   r0,0                 # ?.rlink zero (not on q)
         bz    set1                 # Y.ok, skip ahead
         cal   r2,set_panic         # N.panic
         l     r15,$.long(_panic)    # locate panic proc
         sis   sp,12                # after making room on the stack
         sts   r2,0(sp)             # save parm on stack
         balr  r15,r15              # go panic

set1:
         setsb scr_ics,INTMASK-16   # no interrupts, please
         lc    r15,P_PRI(r2)        # priority - 0..127
         sri   r15,3                # now 0..15
         nilz  r15, r15,0xf              # make sure
         slpi  r15,3                # times 8 bytes per q hdr (in r14)
         cal   r4,_qs               # queue headers
         a     r4,r14               # add offset for this priority
         ls    r3,P_RLINK(r4)       # r4 -> r (tail of queue)
         sts   r3,P_RLINK(r2)       # set back ptr of p -> r
         sts   r4,P_LINK(r2)        # set forw. ptr of p -> que hdr
         sts   r2,P_LINK(r3)        # set forw ptr of r -> p
         sts   r2,P_RLINK(r4)       # set back ptr of que hdr -> p
 #  mark queue as non-empty
         l     r4,_whichqs          # word with one bit per queue
	 cal16 r14,(0x8000^0x8000-0x8000)(r0) # get a one
         sr    r14,r15              # move the 1 to the bit for this prio
         o     r4,r14               # set the bit
         st    r4,_whichqs          # back in _whichqs
 #  restore regs & return
         clrsb scr_ics,INTMASK-16   # interrupts safe now
         lm    r13,0(r1)            # restore registers
         brx   r15                  # return after
         ais   r1,3*4               # restore stack pointer

set_panic: .asciz "setrq"

 #  remrq - delete from runable queue for this priority
 #
 #  remrq(p) struct proc *p;
 #
 #      Q                   P                 R
 #                          +-----------------+
 #                          V                 |
 #    +---+---+           +---+---+         +-+-+---+
 #    |   | --+---------->| | | --+-------->| | |   |       BEFORE
 #    +---+---+           +-+-+---+         +---+---+
 #      A                   |
 #      +-------------------+
 #
 #
 #      Q                   P                 R
 #          +---------------------------------+
 #          |                                 V
 #    +---+-+-+           +---+---+         +---+---+
 #    |   | | |           |   |   |         | | |   |       AFTER
 #    +---+---+           +---+---+         +-+-+---+
 #      A                                     |
 #      +-------------------------------------+
 #
         .globl _remrq
eye_catcher(_remrq):
         sis   r1,3*4               # make room on stack
         stm   r13,0(r1)            # save registers we use
         mfs   scr_iar,r13          # get addressability
1:	 .using 1b,r13              # tell the assembler
 #  make sure that there is really someone on that queue
         lc    r15,P_PRI(r2)        # get the priority
         sri   r15,3                # now 0..15
         nilz  r15,r15,0xf              # make sure
	 cal16 r14,(0x8000^0x8000-0x8000)(r0) # priority 0 mask
         sr    r14,r15              # shift to position for this priority
         l     r4,_whichqs          # which queues have waiting procs
         n     r14,r4               #?.anything there?
         bnz   rem1                 # Y.ok, skip ahead
         cal   r2,set_panic         # N.panic
         l     r15,$.long(_panic)    # locate panic proc
         sis   sp,12                # after making room on the stack
         sts   r2,0(sp)             # save parm on stack
         balr  r15,r15              # go panic

rem1:
         setsb scr_ics,INTMASK-16   # no interrupts, please
         x     r14,r4               # turn off bit for that priority
         ls    r15,P_RLINK(r2)      # r15 -> Q
         ls    r4,P_LINK(r2)        # r4 -> R
         sts   r4,P_LINK(r15)       # link(q) := r
         sts   r15,P_RLINK(r4)      # rlink(r) := q
         c     r4,r15               # equal?  (queue empty?)
         bne   rem2                 # n.skip
         st    r14,_whichqs         # y.update which queues waiting

rem2:
         lis   r15,0                # zero for firewall
         sts   r15,P_RLINK(r2)      # see check in setrq
 #  restore regs & return
         clrsb scr_ics,INTMASK-16   # interrupts safe now
         lm    r13,0(r1)            # restore registers
         brx   r15                  # return after
         ais   r1,3*4               # restore stack pointer

rem_panic: .asciz "remrq"

 #  swtch - resume hightest priority task in the queue
 #
 #     _qs[pri]             P                 Q
 #                          +-----------------+
 #                          V                 |
 #    +---+---+           +---+---+         +-+-+---+
 # +->| | | --+---------->| | | --+-------->| | | | |       BEFORE
 # |  +-+-+---+           +-+-+---+         +---+-+-+
 # |    |   A               |                 A   |
 # |    |   +---------------+                 |   |
 # |    |             +-----------------------+   |
 # |    |             |   +-----------------------+
 # |    |           R |   |
 # |    |             |   V
 # |    |           +-+-+---+
 # |    +---------->| | | | |
 # |                +---+-+-+
 # +----------------------+
 #
 #  note:  MUST NOT modify the return address in r15, which is passed to
 #         _resume below
 #
swtch_panic: .asciz "swtch"
         .globl _swtch
eye_catcher(_swtch):
         sis   sp,3*4
         stm   r13,0(sp)
         mfs   scr_iar,r13
1:	.using 1b,r13
         lis   r0,1
         st    r0,_noproc
         lis   r0,0
         st    r0,_runrun
sw1:
         mfs   scr_ics,r0            # get current ics
         oil   r0,r0,7                  # set level 7
         j     sw1b                  # stay at current level 1st pass
 #  loop looking for process to run
sw1a:
         mts   scr_ics,r0            # get down to level 7
sw1b:
         l     r2,_whichqs           # get list of non-empty queues
         clz   r3,r2                 # how many
         cis   r3,15                 # all 0's
         jh    sw1a                  # y.loop

 #  _whichqs not zero

         nilo  r0,r0,0xFFF8             # back to level zero
         mts   scr_ics,r0            # into ics
	 cal16 r4,(0x8000^0x8000-0x8000)(r0) # get a 1 bit
         sr    r4,r3                 # move the bit into pos'n for pri
         l     r0,_whichqs           # get bits again
         n     r4,r0                 # make sure bit still on
         bz    sw1                   # not still on...loop back
 #
         x     r0,r4                 # was on, turn just that one off
         sli   r3,3                  # 8 bytes per _qs element
         cal   r4,_qs                # point to que headers
         a     r4,r3                # offset into this queue header
 #  remove the head process
         ls    r2,P_LINK(r4)        # r2 -> P
         ls    r3,P_RLINK(r4)       # r3 -> R
         c     r2,r4                # ?==?  (ie. queue empty)
         bne   sw2                  # n.skip ahead (ok)
 #  come here to panic
sw1c:
         cal   r2,swtch_panic       # "swtch"
         l     r15,$.long(_panic)    # locate panic proc
         sis   sp,12                # after making room on the stack
         sts   r2,0(sp)             # save parm on stack
         balr  r15,r15              # go panic
 #
 #  come here to dequeue first element
sw2:
         ls    r3,P_LINK(r2)        # r3 -> Q
         sts   r3,P_LINK(r4)        # link(_qs[pri]) := Q
         sts   r4,P_RLINK(r3)       # rlink(Q) := _qs[pri]
         c     r3,r4                # ?.== now?
         bne   sw3                  # n.not empty, skip ahead
 #  queue is now empty, record new _whichqs
         st    r0,_whichqs          # record new queue status
 #
 #
sw3:
         lis   r0,0                 # zero
         st    r0,_noproc           # ???
         lc    r3,P_STAT(r2)        # check P_STAT
         cis   r3,SRUN              # ?. running?
         bne   sw1c                 # n.panic!
         sts   r0,P_RLINK(r2)       # y.zero rlink for sanity check later
	st	r2,_masterpaddr		# point to current proc running
 #  resume the new process (addr in r2), return addr in r15
	l	r5,__cnt		# get address of cnt
 # increment cnt.v_swtch
#define V_SWTCH 0		/* should really be in assem.s */
	ls	r0,V_SWTCH(r5)		# get value of cnt
	ais	r0,1			# bump 
	sts	r0,V_SWTCH(r5)		# store it
#
         lm    r13,0(sp)            # restore regs
         bx    _resume              # resume new process
         ais   sp,3*4               # give back stack space

 #  resume - save current state and switch to new process
 #
 #  resume(p) - saves current state and switched to process p
 #    struct proc *p;     points to the process to be resumed
 #
 #    operation:
 #      r2 -> proc struct of the new proc to run
 #      remember the address of the current (soon to be previous) proc
 #      save the registers (r1,r6-r15) of the current proc in the u struct
 #      load seg regs 0 and 1 from the new proc struct (changes to new
 #         process
 #      reload the sp from the (new) u struct
 #      restore the other registers from the (new) u struct
 #      if PCB_SSWAP is zero then
 #         return
 #      else
 #         PCB_SSWAP points to a SETJMP saved context - go to LONGJMP
 #
         .globl _resume
eye_catcher(_resume):
         setsb scr_ics,INTMASK-16      # no interrupts, please
 #  save current state in the current pcb
	 cau   r5,(USTRUCT)>>16(r0) # point to the user structure
	 oil   r5,r5,(USTRUCT)&0xffff
         sts   sp,PCB_USP(r5)          # save sp
         stm   r6,PCB_R6(r5)           # save registers
 #  load seg reg 0 value from new proc struct
         lh    r14,P_SID0(r2)          # load seg reg 0 value
         sli   r14,2                   # (Special,Key) = (0,0)
#ifndef VTL_RELOCATE
         oiu   r14,r14,1                   # (Present) = (1)
#endif
 #  send new value to the seg reg 0
	 cau   r13,(RTA_SEGREG0)>>16(r0) # point to seg reg 0
	 oil   r13,r13,(RTA_SEGREG0)&0xffff
         iow   r14,0(r13)              # set new value for seg reg 0
 #  load seg reg 1 value from new proc struct
         lh    r14,P_SID1(r2)          # load seg reg 1 value
         sli   r14,2                   # (Special,Key) = (0,0)
#ifndef VTL_RELOCATE
         oiu   r14,r14,1                   # (Present) = (1)
#endif
 #  send new value to the seg reg 1
         iow   r14,RTA_SEGREGSTEP(r13) # set new value for seg reg 1
 #  now running as new process - restore registers
         ls    sp,PCB_USP(r5)          # restore sp

 #  BEGIN DEBUGGING CODE
         mfs   scr_iar,r11
1:	.using 1b,r11
         l     r2,$.long(_sydebug)
         lcs   r2,0(r2)
         cis   r2,2
         jl    normsg

         sis   sp,12
         cal   r2,$.ascii ":%d:\0"
	 cau   r3,(USTRUCT)>>16(r0)
	 oil   r3,r3,(USTRUCT)&0xffff
         l     r3,U_PROCP(r3)
         lh    r3,P_PID(r3)
         l     r15,$.long(_printf)
         sts   r2,0(sp)
         sts   r3,4(sp)
         balr  r15,r15
         ais   sp,12
1:      .using 1b
normsg:
 #  END DEBUGGING CODE

 #  restore other registers
	 cau   r5,(USTRUCT)>>16(r0)      # point to the user structure
	 oil   r5,r5,(USTRUCT)&0xffff
 # set CCR from pcb
	 l	r7,PCB_CCR(r5)		# pick up ccr value
	 get	r6,$CCR			# point to CCR
 #	 cau   r6,(CCR)>>16(r0)      # point to the user structure
 #	 oil   r6,r6,(CCR)&0xffff
	 stcs	r7,0(r6)		# put into CCR
 # set CCR end
         lm    r6,PCB_R6(r5)           # restore registers
 #  check for PCB_SSWAP - if nonzero points to longjmp vector
         l     r2,PCB_SSWAP(r5)        # get SSWAP field of pcb
         lis   r0,0                    # new value for SSWAP field
         st    r0,PCB_SSWAP(r5)        # use SSWAP only once
         clrsb scr_ics,INTMASK-16      # now interrupts are safe ???
         cis   r2,0                    # check for zero
         bzr   r15                     # return if zero
         j     _longjmp                # non-zero - pass r2 to longjmp
 #
 #  longjmp(buf) - restore saved context and return (to caller of setjmp)
 #      label_t buf;
 #
         .globl _longjmp
eye_catcher(_longjmp):
         lm    r1,0(r2)                # restore registers
	 ls    r0,4*15(r2)	       # restore ics
	 mts   scr_ics,r0
         brx   r15                     # return, after...
         lis   r0,1                    # return value 1
 #
 #  setjmp(buf) - save context for later
 #
	.globl	_savectx
         .globl _setjmp
eye_catcher(_setjmp):
_savectx:
         stm   r1,0(r2)                # save registers
	 mfs   scr_ics,r0	       # save ics
	 sts   r0,4*15(r2)
         brx   r15                     # return, after...
         lis   r0,0                    # return value == 0



#if	MACH_MP
 #
 # int	save_context()
 #
 #	Saves the current process context, masks interrupts off,
 #	and returns 0.  Sets up the saved context so that resuming
 #	the process returns 1.
 #
 #	Use is:
 #
 # swtch()
 # {
 #	if (save_context()) return;
 #	...
 #	load_context(newproc);
 # }

	.globl	_save_context
_save_context:

         setsb scr_ics,INTMASK-16      # no interrupts, please
 #  save current state in the current pcb
#if	MACH_VM
	 bali  r4,save_ctx_base
save_ctx_base:
	 .using save_ctx_base,r4
	 l     r5,a_active_threads
	 l     r5,0(r5)			# XXX - Fails on multiprocessors
	 l     r5,THREAD_PCB(r5)
#else	MACH_VM
	 cau   r5,(USTRUCT)>>16(r0) # point to the user structure
	 oil   r5,r5,(USTRUCT)&0xffff
#endif	MACH_VM
         sts   sp,PCB_USP(r5)          # save sp
         stm   r6,PCB_R6(r5)           # save registers
 # return 0
	 lis	r0,0
	 br	r15

 #
 #	load_context(p)
 #	struct proc *p; (thread * if MACH_VM)
 #
 #	Loads context for a new process.  Assumes that the process
 #	saved its state with save_context, and makes that save_context
 #	call return 1.
 #
	.globl _load_context
_load_context:
#if	MACH_VM
 # find the address of the sidtab of the pmap for this thread
	 l     r3,THREAD_TASK(r2)	# r3 = th->task (pointer to task)
	 l     r3,TASK_MAP(r3)		# r3 = th->task->map (pointer to map)
	 l     r3,MAP_PMAP(r3)		# r3 = th->task->map->pmap
	 ais   r3,PMAP_SIDTAB		# r3 = &(th->task->map->pmap->sidtab)
 # now (properly) restore the segment registers from this pmap.
	 s     r4,r4
	 cau   r13,(RTA_SEGREG0)>>16(r0)
	 oil   r13,r13,(RTA_SEGREG0)&0xffff
load_segregs:
	 lh    r5,0(r3)			# r5 = sid from sidtab for ser(r4)
	 sli   r5,2			# shift 2 bits for seg reg format
	 oiu   r5,r5,(RTA_SEG_PRESENT)>>16
	 cas   r14,r4,r13		# r14 = &(ser(r4))
	 iow   r5,0(r14)		# write it
	 ais   r4,1
	 ais   r3,2			# assumption: sid's are of length 2
	 cli   r4,NUSR_SEGS
	 bl   load_segregs
	 iow   r5,(RTA_INV_TLB - RTA_SEGREG0)(r13) #invalidate entire TLB
 # we have now switched completely to the new process' memory (and stack)
#else	MACH_VM
 #  load seg reg 0 value from new proc struct
         lh    r14,P_SID0(r2)          # load seg reg 0 value
         sli   r14,2                   # (Special,Key) = (0,0)
#ifndef VTL_RELOCATE
         oiu   r14,r14,1                   # (Present) = (1)
#endif
 #  send new value to the seg reg 0
	 cau   r13,(RTA_SEGREG0)>>16(r0) # point to seg reg 0
	 oil   r13,r13,(RTA_SEGREG0)&0xffff
         iow   r14,0(r13)              # set new value for seg reg 0
 #  load seg reg 1 value from new proc struct
         lh    r14,P_SID1(r2)          # load seg reg 1 value
         sli   r14,2                   # (Special,Key) = (0,0)
#ifndef VTL_RELOCATE
         oiu   r14,r14,1                   # (Present) = (1)
#endif	VTL_RELOCATE
 #  send new value to the seg reg 1
         iow   r14,RTA_SEGREGSTEP(r13) # set new value for seg reg 1
#endif	MACH_VM
 #  now running as new process - restore registers
 #  restore other registers
#if	MACH_VM
	 l     r5,THREAD_PCB(r2)
#else	MACH_VM
	 cau   r5,(USTRUCT)>>16(r0)      # point to the user structure
	 oil   r5,r5,(USTRUCT)&0xffff
#endif	MACH_VM	 
         ls    sp,PCB_USP(r5)          # restore sp

#ifdef notdef
 #  BEGIN DEBUGGING CODE
         mfs   scr_iar,r11
1:	.using 1b,r11
         l     r2,$.long(_sydebug)
         lcs   r2,0(r2)
         cis   r2,2
         jl    2f

         sis   sp,12
         cal   r2,$.ascii ":%d:\0"
	 cau   r3,(USTRUCT)>>16(r0)
	 oil   r3,r3,(USTRUCT)&0xffff
         l     r3,U_PROCP(r3)
         lh    r3,P_PID(r3)
         l     r15,$.long(_printf)
         sts   r2,0(sp)
         sts   r3,4(sp)
         balr  r15,r15
         ais   sp,12
1:      .using 1b
2:
 #  END DEBUGGING CODE
#endif notdef

 # set CCR from pcb
	 l	r7,PCB_CCR(r5)		# pick up ccr value
	 get	r6,$CCR			# point to CCR
 #	 cau   r6,(CCR)>>16(r0)      # point to the user structure
 #	 oil   r6,r6,(CCR)&0xffff
	 stcs	r7,0(r6)		# put into CCR
 # set CCR end
         lm    r6,PCB_R6(r5)           # restore registers
 #  check for PCB_SSWAP - if nonzero points to longjmp vector
         l     r2,PCB_SSWAP(r5)        # get SSWAP field of pcb
         lis   r0,0                    # new value for SSWAP field
         st    r0,PCB_SSWAP(r5)        # use SSWAP only once
         clrsb scr_ics,INTMASK-16      # now interrupts are safe ???
	 lis   r0,1		       # return 1 if normal return
         cis   r2,0                    # check for zero
         bzr   r15                     # return if zero
         j     _longjmp                # non-zero - pass r2 to longjmp

#endif	MACH_MP


	.align	2
	.globl _xadunload
 xadunload_ps:
	.long	xadunload-real0
         .short NOTRANS_ICS+INT_PRI1,0	# new ics: xlate off, PRIO 1

 xadunload_ps2:
	.long	xadunload2
	.short	0-0,0		# will be replaced by actual ics upon call


eye_catcher(_xadunload):
|-----------------| xadunload |-------------------|
#ifdef NCS
	stm	%.LOWREG103,(.LOWREGVAR103-16)*4(sp)	# save registers 
	ai	sp,sp,(.LOWREGVAR103-16)*4		# room for r14...r15
#else NCS
 ai sp,sp,-16-L103-.V103 | prologue
 stm %.R103,.V103(sp) | prologue
 cal fp,.V103-64(sp) | prologue
| cas r2,r2,r0 | bfcode
| cas r3,r3,r0 | bfcode
 get r5,L103+92(fp) | bfcode
#endif NCS
 cas r0,r4,r0 | bfcode
 # go to non-translated mode
	mfs scr_iar,r15
1:	.using	1b,r15
	mfs	scr_ics,r14		# get current ics_cs
	lps	0,xadunload_ps		# turn off translate

	.using	real0,r0
xadunload:
 # following only works because translate & storage protect is off
	sth	r14,xadunload_ps2+old_ics_cs	# for later

	cis r5,0 | 85
	be L105 | EQ	(write)

	cis r2,0 | 85
	be L107 | NE	(no buffer)

 # read: transfer adapter to buffer
 # this code was written by Frank Bartucca, IBM Milford
	cis	r0,0		# compare hdcnt to 0
	ble	L118		# already done
	sis	r0,4
	bx	1f		# branch to read routine
2:	ls	r4,0(r3)	# pick up value 
	sis	r0,4		# decrement hdcnt by 4
	inc	r2,8		# increment buffaddr by 8
	st	r5,-4(r2)	# store r5 at buffaddr-4
1:	ble	3f		# if hdnct < 0 clean up 
	ls	r5,0(r3) 	# start next adapter read
	sis	r0,4		# decrement hdcnt by 4
	ble	0f		# if hdcnt <= 0 clean up 
	bx	2b		# else keep going
0:	sts	r4,0(r2)	# store r4 at buffaddr
	bx	L118		#branch to return code
	sts	r5,4(r2)	# store r5 at buffaddr
3:	bx	L118		# else keep going
	sts	r4,0(r2)	# store r4 at buffaddr

 # read: just reference the buffer to unload it and throw away the results
L107:
0:	si r0,r0,4 | 190
	bl L118 | LT
	bx 0b | branch
	ls r2,0(r3) | 94

 # come here to fill out a record with zero's
L117:
	 get r15,$0 | 92
0:
	si r0,r0,4 | 190
	bl L118 | LT
	bx 0b | branch
	sts r15,0(r3) | 80

 # come here to fill adapter buffer for write
L105:
	cis r2,0 | 85
	be L117 | NE
#ifndef notdef
	cis	r0,0		# compare hdcnt to 0 
	jle	L118		# jump to return code if hdcnt <= 0 
	sis	r0,4		# decrement hdcnt by 4 
	blex	0f		# clean up if hdcnt <= 0 
	ls	r4,0(r2)	# load r4 from buffaddr 
2:	ls	r5,4(r2)	# load r5 from buffaddr 
	sts	r4,0(r3)	# write r4 to adapter 
	sis	r0,4		# decrement hdcnt by 4 
	jle	1f		# clean up if hdcnt <= 0 
	inc	r2,8		# increment buffaddr by 8 
	ls	r4,0(r2)	# load r4 from buffaddr 
	sts	r5,0(r3)	# write r5 to adapter 
	sis	r0,4		# decrement hdcnt by 4 
	jh	2b		# if hdcnt > 0 keep going 
0:	bx	L118		# jump to return code 
	sts	r4,0(r3)	# write r4 to adapter 
1:	bx	L118		# branch to return code 
	sts	r5,0(r3)	# write r5 to adapter 
#else	/* old write code */
 # write case
	sis	r0,4	# compare hdcnt to 4
	ble	1f	# either 0 or 4 to do
0:	ls	r4,0(r2)	# get from buffer
	ls	r5,4(r2)	# start another read
	inc	r2,8		# increment pointer
	sis	r0,8		# decrement count
	sts	r4,0(r3)	# store into adapter
	bhx	0b		# branch and complete 
	sts	r5,0(r3)	# storing of second word
 # we have less than 8 to go
1:	bl	L118	# all done
	ls	r4,0(r2)	# can only be 1 word left to do
	bx	L118
	sts	r4,0(r3)	# complete last store
#endif

L118:
 lps	0,xadunload_ps2
xadunload2:
#ifdef NCS
	lm	%.LOWREG103,0(sp)
	brx	r15
	ai	sp,sp,(16-.LOWREGVAR103)*4
#else NCS
 lm %.R103,.V103(sp) | epilogue
 ai sp,sp,16+L103+.V103 | epilogue
 br r15 | epilogue
#endif NCS
.set .LOWREGVAR103, 14 | not optimized
.set .LOWREG103, r14
.set L103, (12 - .LOWREGVAR103) * 4
.set .R103, .LOWREG103
.set .V103, 0 | eobl2


 #
 #	display(nn) displays two hex digits in the front panel display
 #
display_ps:
	.int 	display_go-real0
	.short	NOTRANS_ICS+INT_PRI0
	.short	0
display_ps2:
	.int 	display_go2
	.short	TRANS_ICS		# previous ics stored here
	.short	0

	.globl	_display
eye_catcher(_display):
	mfs	scr_ics,r0
	mfs	scr_iar,r3
1: 	.using	1b,r3
	lps	0,display_ps
 # now running non-translated
	.using	real0,r0		# locore addressing
display_go:
	sth	r0,display_ps2+old_ics_cs
	bali	r5,display		# call routine
	lps	0,display_ps2
display_go2:
	br	r15			# and return to caller
 # display routine available to kernel in non-translate mode
 # r2 contains the value to display
display:
	cau	r3,ROS_ADDR/UPPER(r0)
	ls	r3,4(r3)		# get the ROS EP table
 # we could call ROS but since we've referenced it that is enough
	cau     r3,(LED_ADDR)>>16(r0)	# get the address base for LED's
	oil     r3,r3,(LED_ADDR)&0xffff
	mc33	r3,r2			# get the complete address
	ior	r0,0(r3)		# read from it
	iow	r0,0(r3)		# write it (lock it)
	br	r5			# and return to caller
1:	.using	1b

