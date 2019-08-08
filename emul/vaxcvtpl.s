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
/*	@(#)vaxcvtpl.s	1.3		11/2/84		*/

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
 #	vax-11 instruction emulator
 #
 # abstract:
 #	the routine in this module emulates the vax-11 packed decimal 
 #	cvtpl instruction. this procedure can be a part of an emulator 
 #	package or can be called directly after the input parameters 
 #	have been loaded into the architectural registers.
 #
 #	the input parameters to this routine are the registers that
 #	contain the intermediate instruction state. 
 #
 # environment: 
 #
 #	this routine runs at any access mode, at any ipl, and is ast 
 #	reentrant.
 #
 # author: 
 #
 #	lawrence j. kenah	
 #
 # creation date
 #
 #	18 october 1983
 #
 # modified by:
 #
 #	v01-009 ljk0034		Lawrence j. Kenah	08-Jul--1984
 #		Fix several bugs in restart logic.
 #
 #		Use r4 instead of r7 as dispatch register for restart routine.
 #		there is a single code path where r7 contains useful data.
 #		Insure that the contents of r7 are preserved across the
 #		occurrence of the cvtpl_5 access violation.
 #		Use special restart path for cvtpl_6.
 #		fix recalculation of srcaddr.
 #		Use saved r2 when saving condition codes.
 #
 #	v01-008 ljk0033		lawrence j. kenah	 06-jul-1984
 #		Add r10 to register mask used along error path when the
 #		digit count is larger than 31.
 #
 #	v01-007 ljk0032		Lawrence j. kenah	05-jul-1984
 #		Fix restart routine to take into account the fact that
 #		restart codes are based at one when computing restart PC.
 #
 #	v01-006 ljk0030		lawrence j. kenah	20-jun-1984
 #		Load access violation handler address into r10 before
 #		any useful work (like memory accesses) gets done.
 #
 #	v01-005 ljk0029		lawrence j. kenah	24-may-1984
 #		Fix stack offset calculation in exit code when v-bit is
 #		set to reflect in fact the seven registers (not six) have
 #		been saved on the stack.
 #
 #	v01-004	ljk0024		lawrence j. kenah	22-Feb-1984
 #		Add code to handle access violation. Perform minor cleanup.
 #
 #	v01-003	ljk0023		lawrence j. kenah	10-Feb-1984
 #		Make a write to PC generate a reserved addressing mode fault.
 #		Temporarily do the same thing for a SP destination operand
 #		until a better solution can be figured out.
 #
 #	v01-002	ljk0016		lawrence j. kenah	28-nov-1983
 #		algorithm was revised to work with digit pairs. overflow check
 #		was modified to account for -2,147,483,648.
 #
 #	v01-001	ljk0008		lawrence j. kenah	17-nov-1983
 #		the emulation code for cvtpl was moved into a separate module.
 #--



 # include files:


/*	$psldef				# define bit fields in psl
 *	$srmdef				# define arithmetic trap codes

 *	cvtpl_def			# bit fields in cvtpl registers
 *	stack_def			# stack usage for original exception
 */

 # psect declarations:


 #	begin_mark_point	restart
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
 #	the source string specified by the  source  length  and  source  address
 #	operands  is  converted  to  a  longword  and the destination operand is
 #	replaced by the result.
 #
 # input parameters:
 #
 #	r0 - srclen.rw		length of input decimal string
 #	r1 - srcaddr.ab		address of input packed decimal string
 #	r3 - dst.wl		address of longword to receive converted string
 #
 #	note that the cvtpl instruction is the only instruction in the
 #	emulator package that has an operand type of .wx. this operand type
 #	needs special treatment if the operand is to be written into a general
 #	register. the following convention is established. if the destination
 #	is anything other than a general register (addressing mode 5), then r3
 #	contains the address of the destination. if the destination is a
 #	general register, then r3 contains the ones complement of the register
 #	number. note that if this is interpreted as an address, then r3 points
 #	to the last page of so-called s1 space, the reserved half of system
 #	virtual address space. the algorithmic specification of this
 #	convention is as follows. 
 #
 #		if r3 <31:04> nequ ^xfffffff
 #		  then
 #		    r3 contains the address of the destination operand
 #		  else
 #		    r3 contains the ones complement of the register number
 #		    of the single register to be loaded with the result 
 #		    of the conversion
 #
 #		that is, 
 #
 #			r3 = ffffffff  ==>   r0 <- result
 #			r3 = fffffffe  ==>   r1 <- result
 #				.
 #				.
 #			r3 = fffffff4  ==>  r11 <- result
 #				.
 #				.
 #
 #		note that any "s1 address" in r3 on input other than
 #		ffffffff through fffffff0 will cause a length access
 #		violation. 
 #
 # output parameters:
 #
 #	r0 = 0
 #	r1 = address of byte containing most significant digit of
 #	     the source string
 #	r2 = 0
 #	r3 = 0
 #
 # condition codes:
 #
 #	n <- output longword lss 0
 #	z <- output longword eql 0
 #	v <- integer overflow
 #	c <- 0
 #
 # register usage:
 #
 #	this routine uses r0 through r7. the condition codes are recorded
 #	in r2 as the routine executes. In addition, r10 serves its usual
 #	purpose by pointing to the access violation handler.
 #-

	.globl	vax$cvtpl

L502:	jmp	vax$cvtpl_restart	# Restart somewhere else
vax$cvtpl:
	bbs	$(cvtpl_v_fpd+16),r0,L502# Branch if this is a restart
 #	pushr	$^m<r1,r4,r5,r6,r7,r10>	# save some registers
	 pushr	$0x04f2
 #	establish_handler cvtpl_accvio
	 movab	cvtpl_accvio,r10
	movpsl	r2			# get current psl
	bicb2	$(psl$m_n|psl$m_z|psl$m_v|psl$m_c),r2	# clear condition codes
	clrl	r6			# assume result is zero
 #	roprand_check	r0		# insure that r0 lequ 31
	 cmpw	r0,$31
	 blequ	1f
	 brw	decimal_roprand
1:
	 movzwl	r0,r0
	beql	6f			# all done if string has zero length
 #	mark_point	cvtpl_1,restart
	 .text	2
	 .set	table_size,table_size + 1
	 .word	Lcvtpl_1 - module_base
	 .text	3
	 .word	cvtpl_1 - module_base
	 .text	1
	 .set	restart_table_size,restart_table_size+1
	 .set	cvtpl_1_restart,restart_table_size
	 .word	Lcvtpl_1 - module_base
	 .text
Lcvtpl_1:
	jsb	decimal$strip_zeros_r0_r1	# eliminate leading zeros from input
	ashl	$-1,r0,r0		# convert digit count to byte count
	beql	3f			# skip loop if single digit

 # the first digit pair sets up the initial value of the result.

 #	mark_point	cvtpl_2,restart
	 .text	2
	 .set	table_size,table_size + 1
	 .word	Lcvtpl_2 - module_base
	 .text	3
	 .word	cvtpl_2 - module_base
	 .text	1
	 .set	restart_table_size,restart_table_size+1
	 .set	cvtpl_2_restart,restart_table_size
	 .word	Lcvtpl_2 - module_base
	 .text
Lcvtpl_2:
	movzbl	(r1)+,r5		# get first digit pair
	movzbl	decimal$packed_to_binary_table[r5],r6
					# convert to binary number

 # the sobgtr instruction at the bottom of the loop can be used to decrement
 # the byte count and test whether this is the special case of an initial
 # digit count of two or three. note that this loop does not attempt to
 # optimize the case where the v-bit is already set.

	brb	2f			# join the loop at the bottom

 #	mark_point	cvtpl_3,restart
	 .text	2
	 .set	table_size,table_size + 1
	 .word	Lcvtpl_3 - module_base
	 .text	3
	 .word	cvtpl_3 - module_base
	 .text	1
	 .set	restart_table_size,restart_table_size+1
	 .set	cvtpl_3_restart,restart_table_size
	 .word	Lcvtpl_3 - module_base
	 .text
Lcvtpl_3:
1:	movzbl	(r1)+,r5		# get next digit pair
	movzbl	decimal$packed_to_binary_table[r5],r5
					# convert to binary number
	emul	$100,r6,r5,r6		# blend this latest with previous result

 # check all of r7 and r6<31> for nonzero. unconditionally clear r6<31>.

	bbsc	$31,r6,L15		# branch if overflow into r6<31>
	tstl	r7			# anything into upper longword
	beql	2f			# branch if ok
L15:	bisb2	$psl$m_v,r2		# set saved v-bit
2:	sobgtr	r0,1b			# continue for rest of whole digit pairs

 # the final (least significant) digit is handled in a slightly different 
 # fashion. this has an advantage in that the final overflow check is different
 # from the check that is made inside the loop. that check can be made quickly
 # without concern for the final digit special cases.

 #	mark_point	cvtpl_4,restart
	 .text	2
	 .set	table_size,table_size + 1
	 .word	Lcvtpl_4 - module_base
	 .text	3
	 .word	cvtpl_4 - module_base
	 .text	1
	 .set	restart_table_size,restart_table_size+1
	 .set	cvtpl_4_restart,restart_table_size
	 .word	Lcvtpl_4 - module_base
	 .text
Lcvtpl_4:
3:	extzv	$4,$4,(r1),r5		# get least significant digit
	emul	$10,r6,r5,r6		# blend in with previous result

 # this overflow check differs from the one inside the loop in three ways.
 #
 #	the check for nonzero r7 precedes the test of r6<31>. 
 #
 #     o the high order bit of r6 is left alone. (if overflow occurs, the 
 #	complete 32-bit contents of r6 need to be preserved.)
 #
 #     o a special check is made to see if the 64-bit result is identically 
 #	equal to 
 #
 #		r6 = 80000000
 #		r7 = 00000000
 #
 #     o if this is true and the input sign is minus, then the overflow bit 
 #	needs to be turned off. this unusual result is passed to the following
 #	code by means of a zero in r7. all other results cause nonzero r7
 #	(including the case where the v-bit was already set).
 #
 # note that the check for v-bit previously set is the single additional 
 # instruction that must execute in the normal (v-bit clear) case to test
 # for the extraordinarily rare case of -2147483648.

	tstl	r7			# overflow into second longword?
	bneq	L36			# branch if overflow
	bbs	$psl$v_v,r2,L33		# set r7 to nonzero if v-bit already set
	cmpl	r6,$0x80000000		# peculiar check for r6<31> neq zero
	blssu	4f			# branch if no overflow at all
	beql	L36			# leave r7 alone in special case
L33:	incl	r7			# set r7 to nonzero in all other cases
L36:	bisb2	$psl$m_v,r2		# set saved v-bit

 # all of the input digits have been processed, get the sign of the input
 # string and complete the instruction processing.

 #	mark_point	cvtpl_5,restart
	 .text	2
	 .set	table_size,table_size + 1
	 .word	Lcvtpl_5 - module_base
	 .text	3
	 .word	cvtpl_5 - module_base
	 .text	1
	 .set	restart_table_size,restart_table_size+1
	 .set	cvtpl_5_restart,restart_table_size
	 .word	Lcvtpl_5 - module_base
	 .text
Lcvtpl_5:
4:	bicb3	$0x0f0,(r1),r5		# get sign "digit"

	caseb	r5,$10,$15-10		# dispatch on sign 
L1:
	.word	6f-L1			# 10 => +
	.word	5f-L1			# 11 => -
	.word	6f-L1			# 12 => +
	.word	5f-L1			# 13 => -
	.word	6f-L1			# 14 => +
	.word	6f-L1			# 15 => +

 # note that negative zero is not a problem in this instruction because the
 # longword result will simply be zero, independent of the input sign.

5:	mnegl	r6,r6			# change sign on negative input
	tstl	r7			# was input -2,147,483,648?
	bneq	6f			# nope, leave v-bit alone
	bicb2	$psl$m_v,r2		# clear saved v-bit
6:	movl	(sp)+,r1		# restore original value of r1
	clrq	-(sp)			# set saved r2 and r3 to zero

 # if r3 contains the ones complement of a number between 0 and 15, then the
 # destination is a general register. special processing is required to 
 # correctly restore registers, store the result in a register, and set the
 # condition codes. 

	mcoml	r3,r7			# set up r7 for limit check with case
	casel	r7,$0,$15 - 0		# see if r7 contains a register number
L2:
	.word	L0-L2			# r0  --  store into r0 via popr
	.word	L0-L2			# r1  --  store into r1 via popr

	.word	L110-L2			# r2  --  store in saved r2 on stack
	.word	L110-L2			# r3  --  store in saved r3 on stack
	.word	L110-L2			# r4  --  store in saved r4 on stack
	.word	L110-L2			# r5  --  store in saved r5 on stack
	.word	L110-L2			# r6  --  store in saved r5 on stack
	.word	L110-L2			# r7  --  store in saved r5 on stack

	.word	L0-L2			# r8  --  store into r8 via popr
	.word	L0-L2			# r9  --  store into r8 via popr
	.word	L120-L2			# r10 --  store into r8 via popr
	.word	L0-L2			# r11 --  store into r8 via popr
	.word	L0-L2			# ap  --  store into r8 via popr
	.word	L0-L2			# fp  --  store into r8 via popr

 # the result of specifying pc as a destination operand is defined to be
 # unpredictable in the vax architecture. in addition, it is difficult (but
 # not impossible) for this emulator to modify sp because it is using the
 # stack for local storage. we will generate a reserved addressing mode fault
 # if pc is specified as the destination operand. we will also temporarily
 # generate a reserved addressing mode fault if sp is specified as the
 # destination operand. 

	.word	L0-cvtpl_radrmod	# sp -- reserved addressing mode
	.word	L0-cvtpl_radrmod	# pc -- reserved addressing mode

 # if we drop through the case instruction, then r3 contains the address of
 # the destination operand. this includes system space addresses in the range
 # c0000000 to ffffffff other than the ones complements of 0 through 15
 # (fffffff0 to ffffffff). the next instruction will cause an access violation
 # for all such illegal system space addresses. 

 #	mark_point	cvtpl_6, restart
	 .text	2
	 .set	table_size,table_size + 1
	 .word	Lcvtpl_6 - module_base
	 .text	3
	 .word	cvtpl_6 - module_base
	 .text	1
	 .set	restart_table_size,restart_table_size+1
	 .set	cvtpl_6_restart,restart_table_size
	 .word	Lcvtpl_6 - module_base
	 .text
Lcvtpl_6:
	movl	r6,(r3)			# store result and set condition codes

 # this is the exit path for this routine. the result has already been stored.
 # the condition codes are set and the saved registers restored. the bicpsw
 # instruction is necessary because the various instructions that stored the 
 # result (movl, pushl, etc.) do not affect the c-bit and the c-bit must be
 # clear on exit from this routine.

7:	bicpsw	$psl$m_c		# insure that c-bit is clear on exit
	bispsw	r2			# set saved v-bit
	bbs	$psl$v_v,r2,L75		# step out of line for overflow check
 #	popr	$^m<r2,r3,r4,r5,r6,r7,r10> # restore saved registers and clear 
	 popr	$0x04fc			#  r2 and r3

	rsb

L72:	bicpsw	$(psl$m_n|psl$m_z|psl$m_v|psl$m_c)	# clear condition codes
	bispsw	r3			# set relevant condition codes
 #	popr	$^m<r2,r3,r4,r5,r6,r7,r10>	# restore saved registers 
	 popr	$0x04fc
	rsb

 # if the v-bit is set and decimal traps are enabled (iv-bit is set), then
 # a decimal overflow trap is generated. note that the iv-bit can be set in
 # the current psl or, if this routine was entered as the result of an emulated
 # instruction exception, in the saved psl on the stack.

L75:	movpsl	r3			# save current condition codes
	bbs	$psl$v_iv,r2,L78	# report exception if current iv-bit set
	movab	vax$exit_emulator,r2	# set up r2 for pic address comparison
	cmpl	r2,(4*7)(sp)		# is return pc eqlu vax$exit_emulator ?
	bnequ	L72			# no. simply return v-bit set
	bbc	$psl$v_iv,((4*(7+1))+exception_psl)(sp),L72
					# only return v-bit if iv-bit is clear

	bicpsw	$(psl$m_n|psl$m_z|psl$m_v|psl$m_c)	# clear condition codes
	bispsw	r3			# set relevant condition codes

L78:
 #	popr	$^m<r2,r3,r4,r5,r6,r7,r10># otherwise, restore the registers
	 popr	$0x04fc

 # ... drop into integer_overflow

 #+
 # this code path is entered if the result is too large to fit into a longword
 # and integer overflow exceptions are enabled. the final state of the
 # instruction, including the condition codes, is entirely in place. 
 #
 # input parameter:
 #
 #	(sp) - return pc
 #
 # output parameters:
 #
 #	0(sp) - srm$k_int_ovf_t (arithmetic trap code)
 #	4(sp) - final state psl
 #	8(sp) - return pc
 #
 # implicit output:
 #
 #	control passes through this code to vax$reflect_trap.
 #-

integer_overflow:
	movpsl	-(sp)			# save final psl on stack
	pushl	$srm$k_int_ovf_t	# store arithmetic trap code
	jmp	vax$reflect_trap	# report exception


 #+
 # the destination address is a general register. r3 contains the ones
 # complement of the register number of the general register that is to be
 # loaded with the result. note that the result must be stored in such a way
 # that restoring the saved registers does not overwrite the destination. 
 #
 # the algorithm that accomplishes a correct store of the result with the
 # accompanying setting of the condition codes is as follows.
 #
 #	if the register is in the range r2 through r7 or r10
 #	  then
 #	    store the result on the stack over that saved register
 #	    (note that this store sets condition codes, except the c-bit)
 #	  else
 #	    construct a register save mask from the register number
 #	    store result on the top of the stack
 #	    (note that this store sets condition codes, except the c-bit)
 #	    popr the result using the mask in r3
 #	endif
 #	restore saved registers
 #-

 # r7 contains 0, 1, 8, 9, 10, 11, 12, 13, or 14. we will use the bit number 
 # to create a register save mask for the appropriate register. note that $1
 # is the source operand and r7 is the shift count in the next instruction.

L0:	ashl	r7,$1,r3		# r3 contains mask for single register
	pushl	r6			# store result and set condition codes
	popr	r3			# restore result into correct register
	brb	7b			# restore r2 through r7 and return      

 # r7 contains 2, 3, 4, 5, 6, or 7

L110:	movl	r6,-8(sp)[r7]		# store result over saved register
	brb	7b			# restore r2 through r7 and return      
 # r7 contains a 10

L120:	movl	r6,24(sp)		# Store result over saved register
	brb	7b			# Restore register and return

 #-
 # functional description:
 #
 #	this routine receives control when a digit count larger than 31
 #	is detected. the exception is architecturally defined as an
 #	abort so there is no need to store intermediate state. the digit
 #	count is made after registers are saved. these registers must be
 #	restored before reporting the exception.
 #
 # input parameters:
 #
 #	00(sp) - saved r1
 #	04(sp) - saved r4
 #	08(sp) - saved r5
 #	12(sp) - saved r6
 #	16(sp) - saved r7
 #	20(sp) - saved r10
 #	24(sp) - return pc from vax$cvtpl routine
 #
 # output parameters:
 #
 #	00(sp) - offset in packed register array to delta pc byte
 #	04(sp) - return pc from vax$cvtpl routine
 #
 # implicit output:
 #
 #	this routine passes control to vax$roprand where further
 #	exception processing takes place.
 #-

decimal_roprand:
 #	popr	#^m<r1,r4,r5,r6,r7,r10>	# restore saved registers
	 popr	$0x04f2
	pushl	$cvtpl_b_delta_pc	# store offset to delta pc byte
	jmp	vax$roprand		# pass control along

 #-
 # functional description:
 #
 #	this routine receives control when pc or sp is used as the destination
 #	of a cvtpl instruction. the reaction to this is not architecturally
 #	defined so we are somewhat free in how we handle it. we currently
 #	generate a radrmod abort with r0 containing the correct 32-bit result.
 #	in the future, we may make this instruction restartable following this
 #	exception. 
 #
 # input parameters:
 #
 #	r0 - zero
 #	r1 - address of source decimal string
 #	r2 - contains overflow indication in r2<psl$v_v>
 #	r3 - register number in ones complement form
 #		r3 = -15 => pc was destination operand
 #		r3 = -14 => sp was destination operand
 #	r4 - scratch
 #	r5 - scratch
 #	r6 - correct 32-bit result
 #	r7 - scratch
 #
 #	00(sp) - saved r2 (contains zero)
 #	04(sp) - saved r3 (contains zero)
 #	08(sp) - saved r4
 #	12(sp) - saved r5
 #	16(sp) - saved r6
 #	20(sp) - saved r7
 #	24(sp) - return pc from vax$xxxxxx routine
 #
 # output parameters:
 #
 #	r0 - correct 32-bit result
 #
 #	r1, r2, and r3 are unchanged from their input values.
 #
 #	r4 through r7 are restored from the stack.
 #
 #	00(sp) - offset in packed register array to delta pc byte
 #	04(sp) - return pc from vax$xxxxxx routine
 #
 # implicit output:
 #
 #	this routine passes control to vax$radrmod where further
 #	exception processing takes place.
 #-

cvtpl_radrmod:
	addl2	$8,sp			# discard "saved" r2 and r3
	movl	r6,r0			# remember final result
 #	popr	#^m<r4,r5,r6,r7>	# restore saved registers
	 popr	$0x0f0
	pushl	$cvtpl_b_delta_pc	# store offset to delta pc byte
	jmp	vax$radrmod		# pass control along

 #+
 # functional description:
 #
 #	this routine receives control when an access violation occurs while
 #	executing within the vax$cvtpl emulator routine.
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

cvtpl_accvio:
	clrl	r2			# initialize the counter
	pushab	module_base		# store base address of this module
	pushab	module_end		# store module end address
	jsb	decimal$bounds_check	# check if pc is inside the module
	addl2	$4,sp			# discard end address
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
 #	00(sp) - saved r0 (restored by vax$handler)
 #	04(sp) - saved r1
 #	08(sp) - saved r2
 #	12(sp) - saved r3
 #
 #	see individual entry points for details
 #
 # output parameters:
 #
 #	r0 - address of return pc from vax$cvtpl
 #	r1 - byte offset to delta-pc in saved register array
 #		(pack_v_fpd and pack_m_accvio set to identify exception)
 #
 #	see list of input parameters for cvtpl_restart for a description of the
 #	contents of the packed register array.
 #
 # implicit output:
 #
 #	r4, r5, r6, r7, and r10 are restored to the values that they had
 #	when vax$cvtpl was entered.
 #-


 #+
 # cvtpl_1
 #
 # an access violation occurred in subroutine strip_zeros while scanning the
 # source string for leading zeros.
 #
 #	r0  - updated digit or byte count in source string
 #	r1  - address of current byte in source string
 #	r2  - condition codes reflecting result
 #	r3  - address of destination (unchanged from input value)
 #	r6  - intermediate (or final) longword result
 #
 #	00(r0) - return pc from strip_zeros
 #	04(r0) - original value of r1 (scraddr)
 #	08(r0) - saved r4
 #	12(r0) - saved r5
 #	16(r0) - saved r6
 #	20(r0) - saved r7
 #	24(r0) - saved r10
 #	28(r0) - return pc from vax$cvtpl routine
 #-

cvtpl_1:
	addl2	$4,r0			# discard return pc from strip_zeros
	movb	$cvtpl_1_restart,r4	# store code that locates exception pc
	brb	1f			# join common code

 #+
 # cvtpl_2 through cvtpl_5
 #
 #	r0  - updated digit or byte count in source string
 #	r1  - address of current byte in source string
 #	r2  - condition codes reflecting result
 #	r3  - address of destination (unchanged from input value)
 #	r6  - intermediate (or final) longword result
 #
 #	00(r0) - original value of r1 (scraddr)
 #	04(r0) - saved r4
 #	08(r0) - saved r5
 #	12(r0) - saved r6
 #	16(r0) - saved r7
 #	20(r0) - saved r10
 #	24(r0) - return pc from vax$cvtpl routine
 #-

cvtpl_2:
	movb	$cvtpl_2_restart,r4	# store code that locates exception pc
	brb	1f			# join common code

cvtpl_3:
	movb	$cvtpl_3_restart,r4	# store code that locates exception pc
	brb	1f			# join common code

cvtpl_4:
	movb	$cvtpl_4_restart,r4	# store code that locates exception pc
	brb	1f			# join common code

cvtpl_5:
	movb	$cvtpl_5_restart,r4	# store code that locates exception pc
	brb	1f			# join common code

 #+
 # cvtpl_6 
 #
 #	r0  - updated digit or byte count in source string
 #	r1  - address of most significant byte in source string (original srcaddr)
 #	r2  - condition codes reflecting result
 #	r3  - address of destination (unchanged from input value)
 #	r6  - intermediate (or final) longword result
 #
 #	00(r0) - zero (will be restored to r2)
 #	04(r0) - zero (will be restored to r3)
 #	08(r0) - saved r4
 #	12(r0) - saved r5
 #	16(r0) - saved r6
 #	20(r0) - saved r7
 #	24(r0) - saved r10
 #	28(r0) - return pc from vax$cvtpl routine
 #-

cvtpl_6:
	addl2	$4,r0			# discard extra longword on the stack
	movb	$cvtpl_6_restart,r4	# store code that locates exception pc
	movl	pack_l_saved_r1(sp),(r0)# put "current" r1 on top of stack

1:	subl3	(r0)+,cvtpl_a_srcaddr(sp),r1	# current minus initial srcaddr
	movb	r1,cvtpl_b_delta_srcaddr(sp)	# remember it for restart


	bisb3	$cvtpl_m_fpd,pack_l_saved_r2(sp),cvtpl_b_state(sp)
						# save current condition codes
						#  and set internal fpd bit
 #	insv	r7,#cvtpl_v_state,-		# store code that identifies
 #		#cvtpl_s_state,-		#  exception pc so that we
 #		cvtpl_b_state(sp)		#  restart at correct place
	 insv	r7,$cvtpl_v_state,$cvtpl_s_state,cvtpl_b_state(sp)
	movl	r6,cvtpl_l_result(sp)		# save intermediate result

 # at this point, all intermediate state has been preserved in the register
 # array on the stack. we now restore the registers that were saved on entry 
 # to vax$cvtpl and pass control to vax$reflect_fault where further exception
 # dispatching takes place.

	movq	(r0)+,r4		# restore r4 and r6
	movq	(r0)+,r6		# ... and r6 and r7
	movl	(r0)+,r10		# ... and r10

 #	movl	#<cvtpl_b_delta_pc!-	# indicate offset for delta pc
 #		pack_m_fpd!-		# fpd bit should be set
 #		pack_m_accvio>,r1	# this is an access violation
	 movl	$(cvtpl_b_delta_pc|pack_m_fpd|pack_m_accvio),r1
	jmp	vax$reflect_fault	# continue exception handling

 #	end_mark_point	cvtpl_m_state















 #+
 # functional description:
 #
 #	this routine receives control when a cvtpl instruction is restarted. 
 #	the instruction state (stack and general registers) is restored to the 
 #	state that it was in when the instruction (routine) was interrupted and 
 #	control is passed to the pc at which the exception occurred.
 #
 # input parameters:
 #
 #	 31		  23		   15		   07		 00
 #	+----------------+----------------+---------------+----------------+
 #	|   delta-pc	 |    state	  |            strlen		   |
 #	+----------------+----------------+---------------+----------------+
 #	|			     srcaddr				   |
 #	+----------------+----------------+---------------+----------------+
 #	|			     result				   |
 #	+----------------+----------------+---------------+----------------+
 #	|			        dst				   |
 #	+----------------+----------------+---------------+----------------+
 #
 #	depending on where the exception occurred, some of these parameters 
 #	may not be relevant. they are nevertheless stored as if they were 
 #	valid to make this restart code as simple as possible.
 #
 #	r0<04:00> - remaining digit/byte count in source string
 #	r0<07:05> - spare
 #	r0<15:08> - "srcaddr" difference (current - initial)
 #	r0<19:16> - saved condition codes
 #	r0<22:20> - restart code (identifies point where routine will resume)
 #	r0<23>	  - internal FPD flag
 #      r0<31:24> - size of instruction in instruction stream ( delta pc )
 #	r1	  - address of current byte in source string
 #	r2	  - value of intermediate or final result
 #	r3	  - address of destination ( unchanged from input value of r3 )
 #
 #	00(sp) - return pc from vax$cvtpl routine
 #
 # implicit input:
 #
 #	note that the initial "srclen" is checked for legality before any
 #	restartable exception can occur. this means that r0 lequ 31, which
 #	leaves bits <15:5> free for storing intermediate state. in the case of
 #	an access violation, r0<15:8> is used to store the difference between
 #	the original and current addresses in the source string. 
 #
 # output parameters:
 #
 #	r0  - updated digit or byte count in source string
 #	r2  - condition codes reflecting result
 #	r3  - address of destination (unchanged from input value)
 #	r6  - intermediate (or final) longword result
 #	r10 - address of cvtpl_accvio, this module's "condition handler"
 #
 #	if the instruction was interrupted at mark point 6, the stack and r1
 #	contain different values than they do if the instruction was interrupted
 #	at any of the intermediate restart points.
 #
 #	access violation occurred at restart points 1 through 5
 #
 #		r1 - address of current byte in source string
 #
 #		00(sp) - original value of r1 (scraddr)
 #		04(sp) - saved r4
 #		08(sp) - saved r5
 #		12(sp) - saved r6
 #		16(sp) - saved r7
 #		20(sp) - saved r10
 #		24(sp) - return pc from vax$cvtpl routine
 #
 #	access violation occurred at restart points 1 through 5
 #
 #		r1 - address of most significant byte in source string 
 #			(original srcaddr)
 #
 #		00(sp) - zero (will be restored to r2)
 #		04(sp) - zero (will be restored to r3)
 #		08(sp) - saved r4
 #		12(sp) - saved r5
 #		16(sp) - saved r6
 #		20(sp) - saved r7
 #		24(sp) - saved r10
 #		28(sp) - return pc from vax$cvtpl routine
 #
 # implicit output:
 #
 #	r4, r5, and r7 are used as scratch registers
 #-

		.globl	vax$cvtpl_restart
vax$cvtpl_restart:
 #	pushr	#^m<r0,r1,r4,r5,r6,r7,r10>	# save some registers
	 pushr	$0x04f3
 #	establish_handler	cvtpl_accvio	# reload r10 with handler address
	 movab	cvtpl_accvio,r10
 # make sure that the cvtpl_b_state byte is now on the stack (in r0 or r1)

 #	assume cvtpl_b_state le 7

	extzv	$cvtpl_v_state,$cvtpl_s_state,cvtpl_b_state(sp),r4
						# put restart code into r4

 # the next two instructions reconstruct the initial value of "srcaddr" that
 # is stored on the stack just above the saved r4. this value will be loaded
 # into r1 when the instruction completes execution. 

	movzbl	cvtpl_b_delta_srcaddr(sp),r5	# get the difference
	subl2	r5,cvtpl_a_srcaddr(sp)		# recreate the original r1
	movzbl	r0,r0				# clear out r0<31:8>

 # make sure that the intermediate result is stored in r2

 #	assume cvtpl_l_result eq 8

	movl	r2,r6
	movpsl	r2				# get clean copy of psl
 #	extzv	#cvtpl_v_saved_psw,-		# retrieve saved copy of
 #		#cvtpl_s_saved_psw,-		#  condition codes
 #		cvtpl_b_state(sp),r5
	 extzv	$cvtpl_v_saved_psw,$cvtpl_s_saved_psw,cvtpl_b_state(sp),r5
	bicb2	$(psl$m_n|psl$m_z|psl$m_v|psl$m_c),r2	# clear condition codes
	bisb2	r5,r2				# restore saved codes to r2

 # a check is made to determine whether the access violation occurred at 
 # restart point number 6, where the stack is slightly different from its
 # state at the other exception points.

	addl2	$4,sp			# discard saved r0 place holder
	cmpl	r4,$cvtpl_6_restart	# check for restart at the bitter end
	blssu	1f			# branch if somewhere else
	movl	(sp)+,r1		# restore saved " current" r1
	clrq	-(sp)			# store final values of r2 and r3
1:	movzwl	restart_pc_table_base-2[r4],r4	# convert code to pc offset
	jmp	module_base[r4]		# get back to work

 #	end_mark_point		cvtpl_m_state
module_end:
