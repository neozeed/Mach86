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
/*	@(#)vaxcvtlp.s	1.3		11/2/84		*/

#include "../emul/vaxemul.h"
#include "../machine/psl.h"
#include "../emul/vaxregdef.h"


/************************************************************************
 *									*
 *			Copyright (c) 1984 by				*
 *		Digital Equipment Corporation, Maynard, MA		*
 *			All rights reserved.				*
 *									*
 *   This software is furnished under a license and may be used and	*
 *   copied  only  in accordance with the terms of such license and	*
 *   with the  inclusion  of  the  above  copyright  notice.   This	*
 *   software  or  any  other copies thereof may not be provided or	*
 *   otherwise made available to any other person.  No title to and	*
 *   ownership of the software is hereby transferred.			*
 *									*
 *   The information in this software is subject to change  without	*
 *   notice  and should not be construed as a commitment by Digital	*
 *   Equipment Corporation.						*
 *									*
 *   Digital assumes no responsibility for the use  or  reliability	*
 *   of its software on equipment which is not supplied by Digital.	*
 *									*
 ************************************************************************/

/************************************************************************
 *
 *			Modification History
 *
 *	Stephen Reilly, 20-Mar-84
 * 000- This code is the modification of the VMS emulation codethat 
 *	was written by Larry Kenah.  It has been modified to run
 *	on Ultrix.
 *
 ***********************************************************************/

 #++
 # facility: 
 #
 #       vax-11 instruction emulator
 #
 # abstract:
 #       the routine in this module emulates the vax-11 packed decimal 
 #       cvtlp instruction. this procedure can be a part of an emulator 
 #       package or can be called directly after the input parameters 
 #       have been loaded into the architectural registers.
 #
 #       the input parameters to this routine are the registers that
 #       contain the intermediate instruction state. 
 #
 # environment: 
 #
 #       this routine runs at any access mode, at any ipl, and is ast 
 #       reentrant.
 #
 # author: 
 #
 #       lawrence j. kenah       
 #
 # creation date
 #
 #       18 october 1983
 #
 # modified by:
 #
 #	 v01-004 ljk0040	 Lawrence j. Kenah	 24-jul-1984
 #		 Do not use INCL instruction to modify the contents of
 #		 the sign byte of the output string.
 #
 #	 v01-003 ljk0032	 Lawrence j. Kenah	  5-jul-1984
 #		 Fix restart routine to take into account the fact that
 #		 restart codes are based at one when computing restart PC.
 #
 # 	 v01-002 ljk0024	 lawrence j. kenah	 22-Feb-1984
 #		 Add code to handle access violation. Perform minor cleanup.
 #
 #       v01-001 ljk0008         lawrence j. kenah       18-oct-1983
 #               the emulation code for cvtlp was moved into a separate module.
 #--

 # include files:

/*      $psldef                         # define bit fields in psl

 *      cvtlp_def                       # bit fields in cvtlp registers
 *      stack_def                       # stack usage for original exception
 */

 # psect declarations:

 #		begin_mark_point	restart
	.text
	.text	2
	.set	table_size,0
pc_table_base:
	.text	3
handler_table_base:
	.text	1
	.set	restart_table_size,0
restart_pc_table_base:
	.text
module_base:
 #+
 # functional description:
 #
 #       the source operand is converted to  a  packed  decimal  string  and  the
 #       destination  string  operand  specified  by  the  destination length and
 #       destination address operands is replaced by the result.
 #
 # input parameters:
 #
 #       r0 - src.rl             input longword to be converted
 #       r2 - dstlen.rw          length of output decimal string
 #       r3 - dstaddr.ab         address of output packed decimal string
 #
 # output parameters:
 #
 #       r0 = 0
 #       r1 = 0
 #       r2 = 0
 #       r3 = address of byte containing most significant digit of
 #            the destination string
 #
 # condition codes:
 #
 #       n <- destination string lss 0
 #       z <- destination string eql 0
 #       v <- decimal overflow
 #       c <- 0
 #
 # register usage:
 #
 #       this routine uses r0 through r5 and r11 as scratch registers. r10
 #	 serves its usual function as an access violation routine pointer. the
 #	 condition codes are stored in r11 as the routine executes.
 #
 # notes:
 #
 ###     the following comment needs to be updated to reflect the revised
 ###     algorithm.
 #
 #       the algorithm used in this routine builds the packed decimal from 
 #	least significant digit to most significant digit. the least
 #	significant digit is obtained by dividing the input longword by 10 and
 #	storing the remainder as the least significant digit. the rest of the
 # 	result is obtained by taking the quotient from the first step,
 #	repeatedly dividing by 100, and converting the resulting remainder
 #	into a pair of packed decimal digits. this process continues until the
 #	quotient goes to zero. 
 #
 #	no special processing is observed for an input longword of zero. the
 #	correct results for this case drops out of normal processing. 
 #-

	.globl	vax$cvtlp

L502:	jmp	vax$cvtlp_restart	# restart somewhere else
vax$cvtlp:
	bbs	$(cvtlp_v_fpd+24),r1,L502	# Br is this is a restart
 #      pushr   $^m<r4,r5,r10,r11>      # save some registers
	 pushr	$0x0c30
 #	establish_handler	cvtlp_accvio	# Store address of access
						# violation handler
	 movab	cvtlp_accvio,r10

 # get initial settings for condition codes. the initial settings for v and c
 # will be zero. the initial setting of n depends on the sign of the source
 # operand. the z-bit starts off set and remains set until a nonzero digit is
 # stored in the output string. note that the final z-bit may be set for
 # nonzero input if the output string is not large enough. (the v-bit is set
 # in this case.) in this case, the saved dv bit will determine whether to
 # reflect an exception or merely report the result to the caller. 

        movpsl  r11                     # get dv bit from psl on input
        insv    $psl$m_z,$0,$4,r11      # start with z-bit set, others clear
 #      roprand_check   r2              # insure that r2 lequ 31
	 cmpw	r2,$31
	 blequ	1f
	 brw	decimal_roprand
1:
	 movzwl	r2,r2
        ashl    $-1,r2,r1               # convert digit count to byte count
        addl2   r1,r3                   # get address of sign byte
 #	mark_point	cvtlp_1,restart
	 .text	2
	 .set	table_size,table_size + 1
	 .word	Lcvtlp_1 - module_base
	 .text	3
	 .word	cvtlp_1 - module_base
	 .text	1
	 .set	restart_table_size,restart_table_size+1
	 .set	cvtlp_1_restart,restart_table_size
	 .word	Lcvtlp_1 - module_base
	 .text
Lcvtlp_1:
        movb    $12,(r3)                # assume that sign is plus
 #	clrl     r1                      # prepare r1 for input to ediv
        tstl    r0                      # check sign of source operand
        bgeq    1f                      # start getting digits if not negative

 # source operand is minus. we remember that by setting the saved n-bit but work
 # with the absolute value of the input operand from this point on.

 #	mark_point	cvtlp_2,restart
	 .text	2
	 .set	table_size,table_size + 1
	 .word	Lcvtlp_2 - module_base
	 .text	3
	 .word	cvtlp_2 - module_base
	 .text	1
	 .set	restart_table_size,restart_table_size+1
	 .set	cvtlp_2_restart,restart_table_size
	 .word	Lcvtlp_2 - module_base
	 .text
Lcvtlp_2:
        incb    (r3)                    # convert "+" to "-" (12 -> 13)
        mnegl   r0,r0                   # normalize source operand
        bisb2   $psl$m_n,r11            # set n-bit in saved psw

 #+ 
 # the first (least significant) digit is obtained by dividing the source 
 # longword by ten and storing the remainder in the high order nibble of the
 # sign byte. note that at this point, the upper four bits of the sign byte
 # contain zero.
 #-

1:	clrl	r1			# prepare r1 for input to ediv
        movl    r2,r4                   # special exit if zero source length
        beql    9f                      # only overflow check remains
        ediv    $10,r0,r0,r5            # r5 gets remainder, first digit
        ashl    $4,r5,r5                # shift digit to high nibble position
        beql    2f                      # leave z-bit alone if digit is zero
        bicb2   $psl$m_z,r11            # turn off z-bit if nonzero
 #	mark_point	cvtlp_3,restart
	 .text	2
	 .set	table_size,table_size + 1
	 .word	Lcvtlp_3 - module_base
	 .text	3
	 .word	cvtlp_3 - module_base
	 .text	1
	 .set	restart_table_size,restart_table_size+1
	 .set	cvtlp_3_restart,restart_table_size
	 .word	Lcvtlp_3 - module_base
	 .text
Lcvtlp_3:
        addb2   r5,(r3)                 # merge this digit with low nibble
2:      decl    r4                      # one less output digit
        beql    9f                      # no more room in output string
        ashl    $-1,r4,r4               # number of complete bytes remaining
        beql    8f                      # check for last digit if none
        tstl    r0                      # is source exhausted?
        bneq    3f                      # go get next digits if not
 #	mark_point	cvtlp_4,restart
	 .text	2
	 .set	table_size,table_size + 1
	 .word	Lcvtlp_4 - module_base
	 .text	3
	 .word	cvtlp_4 - module_base
	 .text	1
	 .set	restart_table_size,restart_table_size+1
	 .set	cvtlp_4_restart,restart_table_size
	 .word	Lcvtlp_4 - module_base
	 .text
Lcvtlp_4:
        clrb    -(r3)                   # store a pair of zeros
        brb     5f                      # fill rest of output with zeros

 #+
 # the following loop obtains two digits at a time from the source longword. it
 # accomplishes this by dividing the current value of r0 by 100 and converting
 # the remainder to a pair of decimal digits using the table that converts
 # binary numbers in the range from 0 to 99 to their packed decimal equivalents.
 # note that this technique may cause nonzero to be stored in the upper nibble
 # of the most significant byte of an even length string. this condition will
 # be tested for at the end of the loop.
 #-

3:      ediv    $100,r0,r0,r5           # r5 gets remainder, next digit
 #	mark_point	cvtlp_5, restart
	 .text	2
	 .set	table_size,table_size + 1
	 .word	Lcvtlp_5 - module_base
	 .text	3
	 .word	cvtlp_5 - module_base
	 .text	1
	 .set	restart_table_size,restart_table_size+1
	 .set	cvtlp_5_restart,restart_table_size
	 .word	Lcvtlp_5 - module_base
	 .text
Lcvtlp_5:
        movb    decimal$binary_to_packed_table[r5],-(r3)
                                        # store converted remainder
        beql    4f                      # leave z-bit alone if digit is zero
        bicb2   $psl$m_z,r11            # turn off z-bit if nonzero
4:      tstl    r0                      # is source exhausted?
        beql    5f                      # exit loop is no more source
        sobgtr  r4,3b                   # check for end of loop
        
        brb     8f                      # check for remaining digit

 # the following code executes if the source longword is exhausted. if there
 # are any remaining digits in the destination string, they must be filled
 # with zeros. note that one more byte is cleared if the original input length
 # was odd. this includes the most significant digit and the unused nibble.

5:      blbs    r2,L65                  # one less byte to zero if odd input length

 #	mark_point	cvtlp_6,restart
	 .text	2
	 .set	table_size,table_size + 1
	 .word	Lcvtlp_6 - module_base
	 .text	3
	 .word	cvtlp_6 - module_base
	 .text	1
	 .set	restart_table_size,restart_table_size+1
	 .set	cvtlp_6_restart,restart_table_size
	 .word	Lcvtlp_6 - module_base
	 .text
Lcvtlp_6:
6:      clrb    -(r3)                  # set a pair of digits to zero
L65:    sobgtr  r4,6b                  # any more digits to zero?

 # the following code is the exit path for this routine. note that all code
 # paths that arrive here do so with r0 containing zero. r1 and r2, however,
 # must be cleared on exit. 

7:      clrq    r1                      # comform to architecture
        bicpsw  $(psl$m_n|psl$m_z|psl$m_v|psl$m_c)      # clear condition codes
        bispsw  r11                     # set appropriate condition codes
 #      popr    $^m<r4,r5,r10,r11>      # restore registers, preserving psw
	 popr	$0x0c30
        rsb

 #+
 # the following code executes when there is no more room in the destination
 # string. we first test for the parity of the output length and, if even, 
 # determine whether a nonzero digit was stored in the upper nibble of the 
 # most significant byte. such a nonzero store causes an overflow condition.
 #
 # if the source operand is not yet exhausted, then decimal overflow occurs.
 # if decimal overflow exceptions are enabled, an exception is signalled.
 # otherwise, the v-bit in the psw is set and a normal exit is issued. note
 # that negative zero is only an issue for this instruction when overflow
 # occurs. in the no overflow case, the entire converted longword is stored in
 # the output string and there is only one form of binary zero. 
 #-

8:      blbs    r2,9f                   # no last digit if odd output length
        ediv    $10,r0,r0,r5            # get next input digit
 #	mark_point	cvtlp_7,restart
	 .text	2
	 .set	table_size,table_size + 1
	 .word	Lcvtlp_7 - module_base
	 .text	3
	 .word	cvtlp_7 - module_base
	 .text	1
	 .set	restart_table_size,restart_table_size+1
	 .set	cvtlp_7_restart,restart_table_size
	 .word	Lcvtlp_7 - module_base
	 .text
Lcvtlp_7:
        movb    r5,-(r3)                # store in last output byte
        beql    9f                      # leave z-bit alone if zero
        bicb2   $psl$m_z,r11

9:      tstl    r0                      # is source also all used up?
        beql    7b                     # yes, continue with exit processing

 # an overflow has occurred. if the z-bit is still set, then the n-bit is cleared. 
 # note that, because all negative zero situations occur simultaneously with
 # overflow, the output sign is left as minus. 

0:      clrl    r0                      # r0 must be zero on exit
        bbc     $psl$v_z,r11,L110       # z-bit and n-bit cannot both be set
        bicb2   $psl$m_n,r11            # clear n-bit if z-bit still set
L110:   bisb2   $psl$m_v,r11            # set v-bit in saved psw

 # if the v-bit is set and decimal traps are enabled (dv-bit is set), then
 # a decimal overflow trap is generated. note that the dv-bit can be set in
 # the current psl or, if this routine was entered as the result of an emulated
 # instruction exception, in the saved psl on the stack.

        bbs     $psl$v_dv,r11,L120      # report exception if current dv-bit set
        movab   vax$exit_emulator,r4    # set up r4 for pic address comparison
        cmpl    r4,(4*4)(sp)            # is return pc eqlu vax$exit_emulator ?
        bnequ   7b                      # no. simply return v-bit set
        bbc     $psl$v_dv,((4*(4+1))+exception_psl)(sp),7b
                                        # only return v-bit if dv-bit is clear

 # restore the saved registers and transfer control to decimal_overflow

L120:   clrq    r1                      # comform to architecture
        bicpsw  $(psl$m_n|psl$m_z|psl$m_v|psl$m_c)      # clear condition codes
        bispsw  r11                     # set appropriate condition codes
 #      popr    $^m<r4,r5,r10,r11>      # restore registers, preserving psw
	 popr	$0x0c30
        jmp     vax$decimal_overflow    # report overflow exception


 #-
 # functional description:
 #
 #       this routine receives control when a digit count larger than 31
 #       is detected. the exception is architecturally defined as an
 #       abort so there is no need to store intermediate state. the digit
 #       count is made after registers are saved. these registers must be
 #       restored before reporting the exception.
 #
 # input parameters:
 #
 #       00(sp) - saved r4
 #       04(sp) - saved r5
 #       08(sp) - saved r10
 #       12(sp) - saved r11
 #       16(sp) - return pc from vax$cvtlp routine
 #
 # output parameters:
 #
 #       00(sp) - offset in packed register array to delta pc byte
 #       04(sp) - return pc from vax$cvtlp routine
 #
 # implicit output:
 #
 #       this routine passes control to vax$roprand where further
 #       exception processing takes place.
 #-

decimal_roprand:
 #      popr    $^m<r4,r5,r10,r11>      # restore registers
	 popr	$0x0c30
        pushl   $cvtlp_b_delta_pc       # store offset to delta pc byte
        jmp     vax$roprand             # pass control along

 #
 #+
 # functional description:
 #
 #	this routine receives control when an access violation occurs while
 #	executing within the vax$cvtlp emulator routine.
 #
 #	the routine header for ashp_accvio in module vax$ashp contains a
 #	detailed description of access violation handling for the decimal
 #	string instructions. this routine differs from most decimal 
 #	instruction emulation routines in that it preserves intermediate 
 #	results if an access violation occurs. this is accomplished by 
 #	storing the number of the exception point, as well as intermediate 
 #	arithmetic results, in the registers r0 through r3.
 #
 # input parameters:
 #
 #	see routine ashp_accvio in module vax$ashp
 #
 # output parameters:
 #
 #	see routine ashp_accvio in module vax$ashp
 #-

cvtlp_accvio:
	clrl	r2			# initialize the counter
	pushab	module_base		# store base address of this module
	subl2	(sp)+,r1		# get pc relative to this base

1:	cmpw	r1,pc_table_base[r2]	# is this the right pc?
	beql	3f			# exit loop if true
	aoblss	$table_size,r2,1b	# do the entire table

 # if we drop through the dispatching based on pc, then the exception is not 
 # one that we want to back up. we simply reflect the exception to the user.

2:
 #	popr	#^m<r0,r1,r2,r3>	# restore saved registers
	 popr	$0x0f
	rsb				# return to exception dispatcher

 # the exception pc matched one of the entries in our pc table. r2 contains
 # the index into both the pc table and the handler table. r1 has served
 # its purpose and can be used as a scratch register.

3:	movzwl	handler_table_base[r2],r1	# get the offset to the handler
	jmp	module_base[r1]		# pass control to the handler
	
 # in all of the instruction-specific routines, the state of the stack
 # will be shown as it was when the exception occurred. all offsets will
 # be pictured relative to r0. 



 #+
 # functional description:
 #
 #	the intermediate state of the instruction is packed into registers r0 
 #	through r3 and control is passed to vax$reflect_fault that will, in 
 #	turn, reflect the access violation back to the user. the intermediate 
 #	state reflects the point at which the routine was executing when the 
 #	access violation occurred.
 #
 # input parameters:
 #
 #	r0 - address of top of stack when access violation occurred
 #
 #	00(r0) - saved r4 on entry to vax$cvtlp
 #	04(r0) - saved r5
 #	08(r0) - saved r10
 #	12(r0) - saved r11
 #	16(r0) - return pc from vax$cvtlp routine
 #
 #	00(sp) - saved r0 (restored by vax$handler)
 #	04(sp) - saved r1
 #	08(sp) - saved r2
 #	12(sp) - saved r3
 #
 # output parameters:
 #
 #	r0 - address of return pc from vax$cvtlp
 #	r1 - byte offset to delta-pc in saved register array
 #		(pack_v_fpd and pack_m_accvio set to identify exception)
 #
 #	see list of input parameters for cvtlp_restart for a description of the
 #	contents of the packed register array.
 #
 # implicit output:
 #
 #	r4, r5, r10, and r11 are restored to the values that they had
 #	when vax$cvtlp was entered.
 #-


 #+
 # cvtlp_1 or cvtlp_2
 #
 # an access violation occurred while storing the initial sign in the output
 # string. r1, r4, and r5 contain junk at this point.
 #
 #	r0  - input source longword
 #	r2  - digit count of destination string
 #	r3  - address of sign byte in destination string
 #	r11 - current psw (with z-bit set and all others clear)
 #
 #	r1 - not important
 #	r4 - scratch but saved anyway
 #	r5 - scratch but saved anyway
 #-

cvtlp_1:
 #	movb	#<cvtlp_1_restart!-	# store code that locates exception pc
 #		cvtlp_m_fpd>,-
 #		cvtlp_b_state(sp)	
	 movb	$(cvtlp_1_restart|cvtlp_m_fpd),cvtlp_b_state(sp)
	brb	1f			# join common code

cvtlp_2:
 #	movb	#<cvtlp_2_restart!-	# store code that locates exception pc
 #		cvtlp_m_fpd>,-
 #		cvtlp_b_state(sp)	
	 movb	$(cvtlp_2_restart|cvtlp_m_fpd),cvtlp_b_state(sp)
	brb	1f			# join common code

 #+
 # cvtlp_3 through cvtlp_7
 #
 # an access violation occurred while storing a digit or digit pair in the 
 # output string. 
 #
 #	r0  - input source longword (updated)
 #	r1  - zero (so that r0/r1 can be used as input quadword to ediv)
 #	r2  - digit count of destination string
 #	r3  - address of current byte in destination string
 #	r4  - updated digit or byte count
 #	r5  - most recent remainder from ediv
 #	r11 - current psw (condition codes reflect results so far)
 #-

cvtlp_3:
 #	movb	#<cvtlp_3_restart!-	# store code that locates exception pc
 #		cvtlp_m_fpd>,-
 #		cvtlp_b_state(sp)	
	 movb	$(cvtlp_3_restart|cvtlp_m_fpd),cvtlp_b_state(sp)
	brb	1f			# join common code

cvtlp_4:
 #	movb	#<cvtlp_4_restart!-	# store code that locates exception pc
 #		cvtlp_m_fpd>,-
 #		cvtlp_b_state(sp)	
	 movb	$(cvtlp_4_restart|cvtlp_m_fpd),cvtlp_b_state(sp)
	brb	1f			# join common code

cvtlp_5:
 #	movb	#<cvtlp_5_restart!-	# store code that locates exception pc
 #		cvtlp_m_fpd>,-
 #		cvtlp_b_state(sp)	
	 movb	$(cvtlp_5_restart|cvtlp_m_fpd),cvtlp_b_state(sp)
	brb	1f			# join common code

cvtlp_6:
 #	movb	#<cvtlp_6_restart!-	# store code that locates exception pc
 #		cvtlp_m_fpd>,-
 #		cvtlp_b_state(sp)	
	 movb	$(cvtlp_6_restart|cvtlp_m_fpd),cvtlp_b_state(sp)
	brb	1f			# join common code

cvtlp_7:
 #	movb	#<cvtlp_7_restart!-	# store code that locates exception pc
 #		cvtlp_m_fpd>,-
 #		cvtlp_b_state(sp)	
	 movb	$(cvtlp_7_restart|cvtlp_m_fpd),cvtlp_b_state(sp)

1:	movb	r4,cvtlp_b_saved_r4(sp)	# store current digit/byte count
	movb	r5,cvtlp_b_saved_r5(sp)	# store latest ediv remainder
	movb	r11,cvtlp_b_saved_psw(sp) # store current condition codes

 # at this point, all intermediate state has been preserved in the register
 # array on the stack. we now restore the registers that were saved on entry 
 # to vax$cvtlp and pass control to vax$reflect_fault where further exception
 # dispatching takes place.

	movq	(r0)+,r4		# restore r4 and r5
	movq	(r0)+,r10		# ... and r10 and r11

 #	movl	#<cvtlp_b_delta_pc!-	# indicate offset for delta pc
 #		pack_m_fpd!-		# fpd bit should be set
 #		pack_m_accvio>,r1	# this is an access violation
	 movl	$(cvtlp_b_delta_pc|pack_m_fpd|pack_m_accvio),r1
	jmp	vax$reflect_fault	# continue exception handling


 #+
 # functional description:
 #
 #	this routine receives control when a cvtlp instruction is restarted. 
 #	the instruction state (stack and general registers) is restored to the 
 #	state that it was in when the instruction (routine) was interrupted and 
 #	control is passed to the pc at which the exception occurred.
 #
 # input parameters:
 #
 #	 31		  23		   15		    07         	  00
 #	+----------------+----------------+----------------+----------------+
 #	|    state       |    saved_psw   |   saved_r5     |   saved_r4     |
 #	+----------------+----------------+----------------+----------------+
 #	|   delta-pc     |      XXXX      |              dstlen             |
 #	+----------------+----------------+----------------+----------------+
 #      |			       dstaddr				    |
 #	+----------------+----------------+----------------+----------------+
 #
 #	depending on where the exception occurred, some of these parameters 
 #	may not be relevant. they are nevertheless stored as if they were 
 #	valid to make this restart code as simple as possible.
 #
 #
 #	r0        - updated source longword
 #	r1<07:00> - latest digit or byte count (loaded into r4)
 #	r1<15:08> - most recent remainder from ediv (loaded into r5)
 #	r1<23:16> - saved condition codes (loaded into r11)
 #	r1<26:24> - restart code (identifies point where routine will resume)
 #	r1<27>	  - Internal FPD flag
 #	r2<15:00> - initial value of "dstlen"
 #	r2<23:16> - spare
 #	r2<31:24> - Size of instruction in instruction stream
 #	r3        - address of current byte in destination string
 #
 #	00(sp) - return pc from vax$cvtlp routine
 #
 # output parameters:
 #
 #	r0  - updated source longword (unchanged from input)
 #	r1  - scratch
 #	r2  - initial value of "dstlen"
 #	r3  - address of current byte in output string (unchanged from input)
 #	r4  - latest digit or byte count 
 #	r5  - most recent remainder from ediv 
 #	r10 - address of cvtlp_accvio, this module's "condition handler"
 #	r11 - condition codes
 #
 #	00(sp) - saved r4
 #	04(sp) - saved r5
 #	08(sp) - saved r10
 #	12(sp) - saved r11
 #	16(sp) - return pc from vax$cvtlp routine
 #
 # implicit output:
 #
 #	control is passed to the instruction that was executing when the
 #	access violation occurred.
 #-

	.globl	vax$cvtlp_restart
vax$cvtlp_restart:
 #	pushr	#^m<r0,r1,r4,r5,r10,r11>	# save some registers
	 pushr	$0x0c33
 #	establish_handler	cvtlp_accvio	# reload r10 with handler address
	 movab	cvtlp_accvio,r10
	extzv	$cvtlp_v_state,$cvtlp_s_state,cvtlp_b_state(sp),r1
						# put restart code into r1
	movzbl	cvtlp_b_saved_r4(sp),r4		# restore digit/byte count
	movzbl	cvtlp_b_saved_r5(sp),r5		# restore latest ediv remainder
	movzbl	cvtlp_b_saved_psw(sp),r11	# restore condition codes
	movzbl	r2,r2				# clear out r2<31:8>
	addl2	$8,sp				# discard saved r0 and r1
	movzwl	restart_pc_table_base-2[r1],r1	# convert code to pc offset

 # in order to get back to the restart point with r1 containing zero, we cannot
 # use r1 to transfer control as we did in other routines like vax$cvtpl.

	pushab	module_base[r1]		# store "return pc"
	clrl	r1			# restart with r1 set to zero
	rsb				# get back to work

 #	end_mark_point		cvtlp_m_state

module_end:
