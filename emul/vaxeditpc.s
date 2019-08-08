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
/*	@(#)vaxeditpc.s	1.3		11/2/84		*/

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
 #
 #	the routines in this module emulate the vax-11 editpc instruction.
 #	these routines can be a part of an emulator package or can be
 #	called directly after the input parameters have been loaded into
 #	the architectural registers.
 #
 #	the input parameters to these routines are the registers that
 #	contain the intermediate instruction state. 
 #
 # environment: 
 #
 #	these routines run at any access mode, at any ipl, and are ast 
 #	reentrant.
 #
 # author: 
 #
 #	lawrence j. kenah	
 #
 # creation date
 #
 #	20 september 1982
 #
 # modified by:
 #
 #	v01-008 ljk0035		lawrence j. kenah	16-jul-1984
 #		Fix bugs in restart logic.
 #
 #		R6 cannot be used as both the exception dispatch register and
 #		a scratch register in the main editpc routine.  Use r7 as the
 #		scratch register.
 #		Add code to the editpc_1 restart routine to restore r7 as the
 #		address of the sign byte.
 #		clear c-bit in saved psw in end_float_1 routine.
 #		restore r9 (count of zeros) with cvtwl instruction.
 #		fix calculation of initial srcaddr parameter.
 #		preserve r8 in read_1 and read_2 routines.
 #		preserve r7 in float_2 routine.
 #
 #	v01-007 ljk0032		lawrence j. kenah	 5-jul-1984
 #		fix restart routine to take into account the fact that
 #		restart codes are based at one when computing restart pc.
 #		Load state cell with nonzero restart code in roprand_fault
 #		routine.
 #
 #	v01-006	ljk0026		lawrence j. kenah	19-mar-1984
 #		final cleanup, especially in access violation handling. make
 #		all of the comments in exception handling accurately describe
 #		what the code is really doing.
 #
 #	v01-005	ljk0018		lawrence j. kenah	23-jan-1984
 #		add restart logic for illegal pattern operator. add access
 #		violation handling.
 #
 #	v01-004	ljk0014		lawrence j. kenah	21-nov-1983
 #		clean up rest of exception handling. remove reference
 #		to lib$signal.
 #
 #	v01-003	ljk0012		lawrence j. kenah	8-nov-1983
 #		start out with r9 containing zero so that pattern streams
 #		that do not contain eo$adjust_input will work correctly.
 #
 #	v01-002	ljk0009		lawrence j. kenah	20-oct-1983
 #		add exception handling. fix bug in size of count field.
 #
 #	v01-001	original	lawrence j. kenah	20-sep-1982
 #--


 # include files:

/*	editpc_def			# define intermediaie instruction state
 *
 *	$psldef				# define bit fields in psl
 */

 # equated symbols

# define blank 0x20
# define minus 0x2d
# define zero  0x30

 # psect declarations:

	.text

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
 #	the destination  string  specified  by  the  pattern  and  destination
 #	address  operands  is  replaced  by  the editted version of the source
 #	string specified by the source length  and  source  address  operands.
 #	the  editing  is performed according to the pattern string starting at
 #	the address pattern and extending until a pattern end (eo$end) pattern
 #	operator  is  encountered.   the  pattern  string consists of one byte
 #	pattern operators.  some pattern operators  take  no  operands.   some
 #	take  a repeat count which is contained in the rightmost nibble of the
 #	pattern operator itself.  the rest  take  a  one  byte  operand  which
 #	follows  the  pattern operator immediately.  this operand is either an
 #	unsigned integer length or a byte character.  the  individual  pattern
 #	operators are described on the following pages.
 #
 # input parameters:
 #
 #	r0 - srclen.rw		length of input packed decimal string
 #	r1 - srcaddr.ab		address of input packed decimal string
 #	r3 - pattern.ab		address of table of editing pattern operators
 #	r5 - dstaddr.ab		address of output character string
 #
 # intermediate state:
 #
 #     31               23               15               07            00
 #    +----------------+----------------+----------------+----------------+
 #     |            zero count             |              srclen               |  : r0
 #    +----------------+----------------+----------------+----------------+
 #     |                               srcaddr                               |  : r1
 #    +----------------+----------------+----------------+----------------+
 #     |     delta-srcaddr     |       xxxx       |       sign       |       fill       |  : r0
 #    +----------------+----------------+----------------+----------------+
 #     |                               pattern                               |  : r3
 #    +----------------+----------------+----------------+----------------+
 #     |    loop-count      state            saved-PSW       inisrclen                                |  : r4
 #    +----------------+----------------+----------------+----------------+
 #     |                               dstaddr                               |  : r5
 #    +----------------+----------------+----------------+----------------+
 #
 # output parameters:
 #
 #	r0 - length of input decimal string
 #	r1 - address of most significant byte of input decimal string
 #	r2 - 0
 #	r3 - address of byte containing eo$end pattern operator
 #	r4 - 0
 #	r5 - address of one byte beyond destination character string
 #
 # condition codes:
 #
 #	n <- source string lss 0	(src = -0 => n = 0)
 #	z <- source string eql 0
 #	v <- decimal overflow		(nonzero digits lost)
 #	c <- significance
 #-

	.globl	vax$editpc

L502:	jmp	vax$editpc_restart	# Make sure we test the right FPD bit

L5:	brw	editpc_roprand_abort	# time to quit if illegal length

vax$editpc:
	bbs	$(editpc_v_fpd+16),r4,L502
 #	pushr	$^m<r0,r1,r6,r7,r8,r9,r10,r11>	# save lots of registers
	 pushr	$0x0fc3
	cmpw	r0,$31			# check for r0 gtru 31
	bgtru	L5			# signal roprand if r0 gtru 31
	movzwl	r0,r0			# clear any junk from high-order word
	movzbl	$blank,r2		# set fill to blank, stored in r2
	clrl	r9			# start with "zero count" of zero
 #	establish_handler	editpc_accvio
	 movab	editpc_accvio,r10
	movpsl	r11			# get current psl
	bicb2	$( psl$m_n | psl$m_v | psl$m_c ),r11	# clear n-, v-, and c-bits
	bisb2	$psl$m_z,r11		# set z-bit. 

 # we need to determine the sign in the input decimal string to choose
 # the initial setting of the n-bit in the saved psw.

	extzv	$1,$4,r0,r7		# get byte offset to end of string
	addl2	r1,r7			# get address of byte containing sign
 #	mark_point	editpc_1, restart
	 .text	2
	 .set	table_size,table_size + 1
	 .word	Leditpc_1 - module_base
	 .text	3
	 .word	editpc_1 - module_base
	 .text	1
	 .set	restart_table_size,restart_table_size+1
	 .set	editpc_1_restart,restart_table_size
	 .word	Leditpc_1 - module_base
	 .text
Leditpc_1:
	extzv	$0,$4,(r7),r7		# get sign "digit" into r6

	caseb	r7,$10,$15-10		# dispatch on sign 
L1:
	.word	2f-L1			# 10 => +
	.word	1f-L1			# 11 => -
	.word	2f-L1			# 12 => +
	.word	1f-L1			# 13 => -
	.word	2f-L1			# 14 => +
	.word	2f-L1			# 15 => +

 # sign is minus

1:	bisb2	$psl$m_n,r11		# set n-bit in saved psw
	movzbl	$minus,r4		# set sign to minus, stored in r4
	brb	top_of_loop		# join common code

 # sign is plus (but initial content of sign register is blank)

2:	movzbl	$blank,r4		# set sign to blank, stored in r4

 # the architectural description of the editpc instruction uses an exit flag
 # to determine whether to continue reading edit operators from the input
 # stream. this implementation does not use an explicit exit flag. rather, all
 # of the end processing is contained in the routine that handles the eo$end
 # operator.

 # the next several instructions are the main routine in this module. each
 # pattern is used to dispatch to a pattern-specific routine that performs
 # its designated action. these routines (except for eo$end) return control
 # to top_of_loop to allow the next pattern operator to be processed.

top_of_loop:
	pushab	top_of_loop			# store "return pc"

 # the following instructions pick up the next byte in the pattern stream and
 # dispatch to a pattern specific subroutine that performs the designated
 # action. control is passed back to the main editpc loop by the rsb
 # instructions located in each pattern-specific subroutine. 

 # note that the seemingly infinite loop actually terminates when the eo$end
 # pattern operator is detected. that routine insures that we do not return
 # to this loop but rather to the caller of vax$editpc.

 # 	mark_point	editpc_2, restart
	 .text	2
	 .set	table_size,table_size + 1
	 .word	Leditpc_2 - module_base
	 .text	3
	 .word	editpc_2 - module_base
	 .text	1
	 .set	restart_table_size,restart_table_size+1
	 .set	editpc_2_restart,restart_table_size
	 .word	Leditpc_2 - module_base
	 .text
Leditpc_2:
	caseb	(r3)+,$0,$4-0
L2:
	.word	eo$end_routine-L2		# 00 - eo$end
	.word	eo$end_float_routine-L2		# 01 - eo$end_float
	.word	eo$clear_signif_routine-L2	# 02 - eo$clear_signif
	.word	eo$set_signif_routine-L2	# 03 - eo$set_signif
	.word	eo$store_sign_routine-L2	# 04 - eo$store_sign

 #	mark_point	editpc_3
	.text
	.text	2
	.set	table_size, table_size + 1
	.word	Leditpc_3 - module_base
	.text	3
	.word	editpc_3 - module_base
	.text
Leditpc_3:
	caseb	-1(r3 ),$0x40,$0x47-0x40
L3:
	.word	eo$load_fill_routine-L3		# 40 - eo$load_fill
	.word	eo$load_sign_routine-L3		# 41 - eo$load_sign
	.word	eo$load_plus_routine-L3		# 42 - eo$load_plus
	.word	eo$load_minus_routine-L3	# 43 - eo$load_minus
	.word	eo$insert_routine-L3		# 44 - eo$insert
	.word	eo$blank_zero_routine-L3	# 45 - eo$blank_zero
	.word	eo$replace_sign_routine-L3	# 46 - eo$replace_sign
	.word	eo$adjust_input_routine-L3	# 47 - eo$adjust_input

 #	mark_point	editpc_4
	.text
	.text	2
	.set	table_size, table_size + 1
	.word	Leditpc_4 - module_base
	.text	3
	.word	editpc_4 - module_base
	.text
Leditpc_4:
 #	bitb	$^b1111,-1(r3)		# check for 80, 90, or a0
	 bitb	$0x0f,-1(r3)
	beql	3f			# reserved operand on repeat of zero
 #	mark_point	editpc_5
	.text
	.text	2
	.set	table_size, table_size + 1
	.word	Leditpc_5 - module_base
	.text	3
	.word	editpc_5 - module_base
	.text
Leditpc_5:
	extzv	$4,$4,-1(r3 ),r7		# ignore repeat count in dispatch

	caseb	r7,$8,$10-8
L4:
	.word	eo$fill_routine-L4		# 81 to 8f - eo$fill
	.word	eo$move_routine-L4		# 91 to 9f - eo$move
	.word	eo$float_routine-L4		# a1 to af - eo$float

 # if we drop through all three case instructions, the pattern operator is
 # unimplemented or reserved. r3 is backed up to point to the illegal
 # pattern operator and a reserved operand fault is signalled.

3:	decl	r3			# point r3 to illegal operator
	addl2	$4,sp			# discard return pc
	brw	editpc_roprand_fault	# initiate exception processing

 #+
 # functional description:
 #
 #	there is a separate action routine for each pattern operator. these 
 #	routines are entered with specific register contents and several 
 #	scratch registers at their disposal. they perform their designated 
 #	action and return to the main vax$editpc routine.
 #
 #	there are several words used in the architectural description of this
 #	instruction that are carried over into comments in this module. these
 #	words are briefly mentioned here.
 #
 #	char	character in byte following pattern operator (used by
 #		eo$load_fill, eo$load_sign, eo$load_plus, eo$load_minus,
 #		and eo$insert)
 #
 #	length	length in byte following pattern operator (used by
 #		eo$blank_zero, eo$replace_sign, and eo$adjust_input)
 #
 #	repeat	repeat count in bits <3:0> of pattern operator (used by
 #		eo$fill, eo$move, and eo$float)
 #
 #	the architecture makes use of two character registers, described
 #	as appearing in different bytes of r2. for simplicity, we use an
 #	additional register.
 #
 #	fill	stored in r2<7:0>
 #
 #	sign	stored in r4<7:0> 
 #
 #	finally, the architecture describes two subroutines, one that obtains
 #	the next digit from the input string and the other that stores a 
 #	character in the output string. 
 #
 #	read	subroutine eo_read provides this functionality
 #
 #	store	a single instruction of the form
 #
 #			movb	xxx,(r5)+
 #
 #		or
 #
 #			addb3	$zero,r7,(r5)+
 #
 #		stores a single character and advances the pointer.
 #
 # input parameters:
 #
 #	r0 - updated length of input decimal string
 #	r1 - address of next byte of input decimal string
 #	r2 - fill character
 #	r3 - address of one byte beyond current pattern operator
 #	r4 - sign character 
 #	r5 - address of next character to be stored in output character string
 #
 # implicit input:
 #
 #	several registers are used to contain intermediate state, passed
 #	from one action routine to the next.
 #
 #	r7  - contains lastest digit from input stream (output from eo_read)
 #	r11 - pseudo-psw that contains the saved condition codes
 #
 # side effects:
 #
 #	the remaining registers are used as scratch by the action routines.
 #	
 #	r6 - scratch register used only by access violation handler
 #	r7 - output parameter of eo_read routine
 #	r8 - scratch register used by pattern-specific routines
 #
 # output parameters:
 #
 #	the actual output depends on the pattern operator that is currently
 #	executing. the routine headers for each routine will describe the
 #	specific output parameters.
 #-


 #+
 # functional description:
 #
 #	this routine reads the next digit from the input packed decimal
 #	string and passes it back to the caller.
 #
 # input parameters:
 #
 #	r0 - updated length of input decimal string
 #	r1 - address of next byte of input decimal string
 #	r9 - count of extra zeros (see eo$adjust_input)
 #
 #	(sp) - return address to caller of this routine
 #
 #	note that r9<15:0> contains the data described by the architecture as
 #	appearing in r0<31:16>. in the event of an restartable exception
 #	(access violation or reserved operand fault due to an illegal pattern
 #	operator), the contents of r9<15:0> will be stored in r0<31:16>. in
 #	order for the instruction to be restarted, the "zero count" (the
 #	contents of r9) must be preserved. while any available field will do
 #	in the event of an access violation, the use of r0<31:16> is clearly
 #	specified for a reserved operand fault. 
 #
 # output parameters:
 #
 #	The behavior of this routine depends on the contents of r9
 #
 #	r9 is zero on input
 #
 #		r0 - updated by one 
 #		r1 - updated by one if r0<0> is clear on input
 #		r7 - next decimal digit in input string
 #		r9 - unchanged
 #
 #		psw<z> is set if the digit is zero, clear otherwise
 #
 #	r9 is nonzero (lss 0) on input
 #
 #		r0 - unchanged
 #		r1 - unchanged
 #		r7 - zero
 #		r9 - incremented by one (toward zero)
 #
 #		psw<z> is set
 #
 # notes:
 #
 #-

eo_read:
	tstl	r9			# check for "r0" lss 0
	bneq	2f			# special code if nonzero
	decl	r0			# insure that digits still remain
	blss	3f			# reserved operand if none
	blbc	r0,1f			# next code path is flip flop

 # r0 was even on input (and is now odd), indicating that we want the low
 # order nibble in the input stream. the input pointer r1 must be advanced 
 # to point to the next byte. 

 #	mark_point	read_1
	.text
	.text	2
	.set	table_size, table_size + 1
	.word	read_1 - module_base
	.text	3
	.word	read_1 - module_base
	.text
Lread_1:
	extzv	$0,$4,(r1)+,r7		# load low order nibble into r7
	rsb				# Return with information in Z-bit

 # r0 was odd on input (and is now even), indicating that we want the high
 # order nibble in the input stream. the next pass through this routine will
 # pick up the low order nibble of the same input byte. 

 #	mark_point	read_2
	.text
	.text	2
	.set	table_size, table_size + 1
	.word	Lread_2 - module_base
	.text	3
	.word	read_2 - module_base
	.text
Lread_2:
1:	extzv	$4,$4,(r1 ),r7		# load high order nibble into r7
	rsb				# return with information in z-bit

 # r9 was nonzero on input, indicating that zeros should replace the original
 # input digits.

2:	incl	r9			# advance r9 toward zero
	clrl	r7			# behave as if we read a zero digit
	rsb				# return with z-bit set

 # the input decimal string ran out of digits before its time. the architecture
 # dictates that r3 points to the pattern operator that requested the input
 # digit and r0 contains a -1 when the reserved operand abort is reported.
 # it is not necessary to load r0 here. r0 already contains -1 because it
 # just turned negative

3:	decl	r3			# back up r3 to current pattern operator
	addl2	$8,sp			# discard two return pcs
	brw	editpc_roprand_abort	# branch aid for reserved operand abort

 #+
 # functional description:
 #
 #	insert a fixed character, substituting the fill character if
 #	not significant.
 #
 # input parameters:
 #
 #	r2 - fill character
 #	r3 - address of character to be inserted if significance is set
 #	r5 - address of next character to be stored in output character string
 #	r11<c> - current setting of significance
 #
 # output parameters:
 #
 #	character in pattern stream (or fill character if no significance)
 #	is stored in the the output string.
 #
 #	r3 - advanced beyond character in pattern stream
 #	r5 - advanced one byte as a result of the store operation
 #-

eo$insert_routine:
	bbc	$psl$v_c,r11,1f		# skip next if no significance
 #	mark_point	insert_1
	.text
	.text	2
	.set	table_size, table_size + 1
	.word	Linsert_1 - module_base
	.text	3
	.word	insert_1 - module_base
	.text
Linsert_1:
	movb	(r3)+,(r5)+		# store "ch" in output string
	rsb

 #	mark_point	insert_2
	.text
	.text	2
	.set	table_size, table_size + 1
	.word	Linsert_2 - module_base
	.text	3
	.word	insert_2 - module_base
	.text
Linsert_2:
1:	movb	r2,(r5)+		# store fill character
	incl	r3			# skip over unused character
	rsb
 #+
 # functional description:
 #
 #	the contents of the sign register are placed into the output string.
 #
 # input parameters:
 #
 #	r4 - sign character
 #	r5 - address of next character to be stored in output character string
 #
 # output parameters:
 #
 #	sign character is stored in the the output string.
 #
 #	r5 - advanced one byte as a result of the store operation
 #-

eo$store_sign_routine:
 #	mark_point	store_sign_1
	.text
	.text	2
	.set	table_size, table_size + 1
	.word	Lstore_sign_1 - module_base
	.text	3
	.word	store_sign_1 - module_base
	.text
Lstore_sign_1:
	movb	r4,(r5)+		# store sign character
	rsb


 #+
 # functional description:
 #
 #	the contents of the fill register are placed into the output string
 #	a total of "repeat" times. 
 #
 # input parameters:
 #
 #	r2 - fill character
 #	r5 - address of next character to be stored in output character string
 #
 #	-1(r3)<3:0> - repeat count is stored in right nibble of pattern operator
 #
 # output parameters:
 #
 #	fill character is stored in the output string "repeat" times
 #
 #	r5 - advanced "repeat" bytes as a result of the store operations
 #-

eo$fill_routine:
 #	mark_point	fill_1
	.text
	.text	2
	.set	table_size, table_size + 1
	.word	Lfill_1 - module_base
	.text	3
	.word	fill_1 - module_base
	.text
Lfill_1:
	extzv	$0,$4,-1(r3 ),r8		# get repeat count from pattern operator
 # 	mark_point	fill_2, restart
	 .text	2
	 .set	table_size,table_size + 1
	 .word	Lfill_2 - module_base
	 .text	3
	 .word	fill_2 - module_base
	 .text	1
	 .set	restart_table_size,restart_table_size+1
	 .set	fill_2_restart,restart_table_size
	 .word	Lfill_2 - module_base
	 .text
Lfill_2:
1:	movb	r2,(r5)+		# store fill character
	sobgtr	r8,1b			# test for end of loop
	rsb

 #+
 # functional description:
 #
 #	the right nibble of the pattern operator is  the  repeat  count.   for
 #	repeat  times, the following algorithm is executed.  the next digit is
 #	moved from the source to the destination.  if the digit  is  non-zero,
 #	significance  is  set  and  zero  is  cleared.   if  the  digit is not
 #	significant (i.e., is a leading zero) it is replaced by  the  contents
 #	of the fill register in the destination.
 #-

eo$move_routine:
 #	mark_point	move_1
	.text
	.text	2
	.set	table_size, table_size + 1
	.word	Lmove_1 - module_base
	.text	3
	.word	move_1 - module_base
	.text
Lmove_1:
	extzv	$0,$4,-1(r3 ),r8		# get repeat count

1:
 #	eo_read
	 .text	1
	 .set	restart_table_size,restart_table_size+1
	 .word	0f - module_base
	 .text
0:  	 bsbw	eo_read			# get next input digit

 #	cmpb	$zero,r7		# is this digit zero?
	beql	3f			# branch if yes
	bisb2	$psl$m_c,r11		# indicate significance 
	bicb2	$psl$m_z,r11		# also indicate nonzero

 # 	mark_point	move_2, restart
	 .text	2
	 .set	table_size,table_size + 1
	 .word	Lmove_2 - module_base
	 .text	3
	 .word	move_2 - module_base
	 .text	1
	 .set	restart_table_size,restart_table_size+1
	 .set	move_2_restart,restart_table_size
	 .word	Lmove_2 - module_base
	 .text
Lmove_2:
2:	addb3	$zero,r7,(r5)+		# store digit in output stream
	sobgtr	r8,1b			# test for end of loop
	rsb

3:	bbs	$psl$v_c,r11,2b		# if significance, then store digit

 # 	mark_point	move_3, restart
	 .text	2
	 .set	table_size,table_size + 1
	 .word	Lmove_3 - module_base
	 .text	3
	 .word	move_3 - module_base
	 .text	1
	 .set	restart_table_size,restart_table_size+1
	 .set	move_3_restart,restart_table_size
	 .word	Lmove_3 - module_base
	 .text
Lmove_3:
	movb	r2,(r5)+		# otherwise, store fill character
	sobgtr	r8,1b			# test for end of loop
	rsb

 #+
 # functional description:
 #
 #	the right nibble of the pattern operator is  the  repeat  count.   for
 #	repeat  times,  the  following  algorithm is executed.  the next digit
 #	from the source is examined.  if it is non-zero  and  significance  is
 #	not  yet  set, then the contents of the sign register is stored in the
 #	destination, significance is set, and zero is cleared.  if  the  digit
 #	is  significant,  it  is  stored  in  the  destination,  otherwise the
 #	contents of the fill register is stored in the destination.
 #-

eo$float_routine:
 #	mark_point	float_1
	.text
	.text	2
	.set	table_size, table_size + 1
	.word	Lfloat_1 - module_base
	.text	3
	.word	float_1 - module_base
	.text
Lfloat_1:
	extzv	$0,$4,-1(r3 ),r8		# get repeat count

1:
 #	eo_read
	 .text	1
	 .set	restart_table_size,restart_table_size+1
	 .word	0f - module_base
	 .text
0:  	 bsbw	eo_read			# get next input digit
	bbs	$psl$v_c,r11,2f		# if significance, then store digit
 #	cmpb	$zero,r7		# is this digit zero?
	beql	3f			# branch if yes. store fill character.
 # 	mark_point	float_2,restart
	 .text	2
	 .set	table_size,table_size + 1
	 .word	Lfloat_2 - module_base
	 .text	3
	 .word	float_2 - module_base
	 .text	1
	 .set	restart_table_size,restart_table_size+1
	 .set	float_2_restart,restart_table_size
	 .word	Lfloat_2 - module_base
	 .text
Lfloat_2:
	movb	r4,(r5)+		# store sign
	bisb2	$psl$m_c,r11		# indicate significance 
	bicb2	$psl$m_z,r11		# also indicate nonzero

 # 	mark_point	float_3,restart
	 .text	2
	 .set	table_size,table_size + 1
	 .word	Lfloat_3 - module_base
	 .text	3
	 .word	float_3 - module_base
	 .text	1
	 .set	restart_table_size,restart_table_size+1
	 .set	float_3_restart,restart_table_size
	 .word	Lfloat_3 - module_base
	 .text
Lfloat_3:
2:	addb3	$zero,r7,(r5)+		# store digit in output stream
	sobgtr	r8,1b			# test for end of loop
	rsb

3:
 # 	mark_point	float_4,restart
	 .text	2
	 .set	table_size,table_size + 1
	 .word	Lfloat_4 - module_base
	 .text	3
	 .word	float_4 - module_base
	 .text	1
	 .set	restart_table_size,restart_table_size+1
	 .set	float_4_restart,restart_table_size
	 .word	Lfloat_4 - module_base
	 .text
Lfloat_4:
	movb	r2,(r5)+		# otherwise, store fill character
	sobgtr	r8,1b			# test for end of loop
	rsb

 #+
 # functional description:
 #
 #	if the floating sign has not yet been placed into the destination
 #	string (that is, if significance is not yet set ), then the contents
 #	of the sign register are stored in the output string and significance 
 #	is set.
 #
 # input parameters:
 #
 #	r4 - sign character
 #	r5 - address of next character to be stored in output character string
 #	r11<c> - current setting of significance
 #
 # output parameters:
 #
 #	sign character is optionally stored in the output string (if 
 #	significance was not yet set).
 #
 #	r5 - optionally advanced one byte as a result of the store operation
 #	r11<c> - (significance) is unconditionally set
 #-

eo$end_float_routine:
	bbss	$psl$v_c,r11,1f		# test and set significance
 #	mark_point	end_float_1
	.text
	.text	2
	.set	table_size, table_size + 1
	.word	Lend_float_1 - module_base
	.text	3
	.word	end_float_1 - module_base
	.text
Lend_float_1:
	movb	r4,(r5)+		# store sign character
1:	rsb

 #+
 # functional description:
 #
 #	the pattern operator is followed by an unsigned byte  integer  length.
 #	if  the  value  of the source string is zero, then the contents of the
 #	fill register are stored into the last length bytes of the destination
 #	string.
 #
 # input parameters:
 #
 #	r2 - fill character
 #	r3 - address of "length", number of characters to blank
 #	r5 - address of next character to be stored in output character string
 #	r11<z> - set if input string is zero
 #
 # output parameters:
 #
 #	contents of fill register are stored in last "length" characters
 #	of output string if input string is zero.
 #
 #	r3 - advanced one byte over "length"
 #	r5 - unchanged
 #
 # side effects:
 #
 #	r8 is destroyed
 #-

eo$blank_zero_routine:
 #	mark_point	blank_zero_1
	.text
	.text	2
	.set	table_size, table_size + 1
	.word	Lblank_zero_1 - module_base
	.text	3
	.word	blank_zero_1 - module_base
	.text
Lblank_zero_1:
	movzbl	(r3)+,r8		# get length
	bbc	$psl$v_z,r11,2f		# skip rest if source string is zero
	subl2	r8,r5			# back up destination pointer
 # 	mark_point	blank_zero_2, restart
	 .text	2
	 .set	table_size,table_size + 1
	 .word	Lblank_zero_2 - module_base
	 .text	3
	 .word	blank_zero_2 - module_base
	 .text	1
	 .set	restart_table_size,restart_table_size+1
	 .set	blank_zero_2_restart,restart_table_size
	 .word	Lblank_zero_2 - module_base
	 .text
Lblank_zero_2:
1:	movb	r2,(r5)+		# store fill character
	sobgtr	r8,1b			# check for end of loop
2:	rsb

 #+
 # functional description:
 #
 #	if the value of the source string is zero, then the contents of the
 #	fill register are stored into the byte of the destination string
 #	that is "length" bytes before the current position.
 #
 # input parameters:
 #
 #	r2 - fill character
 #	r3 - address of "length", number of characters to blank
 #	r5 - address of next character to be stored in output character string
 #	r11<z> - set if input string is zero
 #
 # output parameters:
 #
 #	contents of fill register are stored in byte of output string
 #	"length" bytes before current position if input string is zero.
 #
 #	r3 - advanced one byte over "length"
 #	r5 - unchanged
 #
 # side effects:
 #
 #	r8 is destroyed
 #-

eo$replace_sign_routine:
 #	mark_point	replace_sign_1
	.text
	.text	2
	.set	table_size, table_size + 1
	.word	Lreplace_sign_1 - module_base
	.text	3
	.word	replace_sign_1 - module_base
	.text
Lreplace_sign_1:
	movzbl	(r3)+,r8		# get length
	bbc	$psl$v_z,r11,1f		# skip rest if source string is zero
	subl3	r8,r5,r8		# get address of indicated byte
 #	mark_point	replace_sign_2
	.text
	.text	2
	.set	table_size, table_size + 1
	.word	Lreplace_sign_2 - module_base
	.text	3
	.word	replace_sign_2 - module_base
	.text
Lreplace_sign_2:
	movb	r2,(r8)			# store fill character
1:	rsb

 #+
 # functional description:
 #
 #	the contents of the fill or sign register are replaced with the
 #	character that follows the pattern operator in the pattern stream.
 #
 #	eo$load_fill	load fill register
 #
 #	eo$load_sign	load sign register
 #
 #	eo$load_plus	load sign register if source string is positive (or zero)
 #
 #	eo$load_minus	load sign register if source string is negative
 #
 # input parameters:
 #
 #	r3 - address of character to be loaded
 #	r11<n> - set if input string is lss zero (negative)
 #
 # output parameters:
 #
 #	if entry is at eo$load_fill, the fill register contents (r2<7:0>) are 
 #	replaced with the next character in the pattern stream. 
 # 
 #	if one of the other entry points is used (and the appropriate conditions
 #	obtain ), the contents of the sign register are replaced with the next
 #	character in the pattern stream. for simplicity of implementation, the
 #	sign character is stored in r4<7:0> while this routine executes. 
 #
 #	in the event of an exception, the contents of r4<7:0> will be stored
 #	in r2<15:8>, either to conform to the architectural specification of
 #	register contents in the event of a reserved operand fault, or to
 #	allow the instruction to be restarted in the event of an access
 #	violation. 
 #
 #	r3 - advanced one byte over new fill or sign character
 #-

eo$load_fill_routine:
 #	mark_point	load_xxxx_1
	.text
	.text	2
	.set	table_size, table_size + 1
	.word	Lload_xxxx_1 - module_base
	.text	3
	.word	load_xxxx_1 - module_base
	.text
Lload_xxxx_1:
	movb	(r3)+,r2		# load new fill character
	rsb

eo$load_sign_routine:
 #	mark_point	load_xxxx_2
	.text
	.text	2
	.set	table_size, table_size + 1
	.word	Lload_xxxx_2 - module_base
	.text	3
	.word	load_xxxx_2 - module_base
	.text
Lload_xxxx_2:
	movb	(r3)+,r4		# load new sign character into r4
	rsb

eo$load_plus_routine:
 	bbc	$psl$v_n,r11,eo$load_sign_routine # use common code if plus
	incl	r3			# otherwise, skip unused character
	rsb

eo$load_minus_routine:
 	bbs	$psl$v_n,r11,eo$load_sign_routine # use common code if minus
	incl	r3			# otherwise, skip unused character
	rsb

 #+
 # functional description:
 #
 #	the significance indicator (c-bit in auxiliary psw) is set or
 #	cleared according to the entry point.
 #
 # input parameters:
 #
 #	none
 #
 # output parameters:
 #
 #	eo$clear_signif		r11<c> is cleared
 #
 #	eo$set_signif		r11<c> is set 
 #-

eo$clear_signif_routine:
	bicb2	$psl$m_c,r11		# clear significance
	rsb

eo$set_signif_routine:
	bisb2	$psl$m_c,r11		# set significance
	rsb

 #+
 # functional description:
 #
 #	the pattern operator is followed by an unsigned byte integer length in
 #	the  range  1  through  31.  if the source string has more digits than
 #	this length, the excess leading digits are read and discarded.  if any
 #	discarded  digits  are  non-zero then overflow is set, significance is
 #	set, and zero is cleared.  if the source string has fewer digits  than
 #	this  length,  a  counter  is  set  of  the number of leading zeros to
 #	supply.  this counter is stored as a negative number in r0<31:16>.
 #-

eo$adjust_input_routine:
 #	mark_point	adjust_input_1
	.text
	.text	2
	.set	table_size, table_size + 1
	.word	Ladjust_input_1 - module_base
	.text	3
	.word	adjust_input_1 - module_base
	.text
Ladjust_input_1:
	movzbl	(r3)+,r8		# get "length" from pattern stream
	subl3	r8,r0,r8		# is length larger than input length?
	blequ	3f			# branch if yes
	clrl	r9			# clear count of zeros ("r0<31:16>")

1:
 #	eo_read
	 .text	1
	 .set	restart_table_size,restart_table_size+1
	 .word	0f - module_base
	 .text
0:  	 bsbw	eo_read			# get next input digit
 #	cmpb	$zero,r7		# is it zero?
	beql	2f			# skip to end of loop if zero
	bicb2	$psl$m_z,r11		# otherwise, indicate nonzero
	bisb2	$( psl$m_c | psl$m_v ),r11	# indicate significance and overflow
2:	sobgtr	r8,1b			# test for end of loop
	rsb

3:	movl	r8,r9			# store difference into "r0<31:16>"
	rsb

 #+
 # functional description:
 #
 #	the edit operation is terminated.
 #
 #	the architectural description of editpc divides end processing between
 #	the eo$end routine and code at the end of the main loop. this 
 #	implementation performs all of the work in a single place.
 #
 #	the edit operation is terminated. there are several details that this
 #	routine must take care of.
 #
 #	1.  the return pc to the main dispatch loop is discarded.
 #
 #	2.  r3 is backed up to point to the eo$end pattern operator.
 #
 #	3.  a special check must be made for negative zero to insure that
 #	    the n-bit is cleared.
 #
 #	4.  if any digits still remain in the input string, a reserved
 #	    operand abort is taken.
 #
 #	5.  r2 and r4 are set to zero according to the architecture.
 #
 # input parameters:
 #
 #	r0 - number of digits remaining in input string
 #	r3 - address of one byte beyond the eo$end operator
 #
 #	00(sp)  - return address in dispatch loop in this module (discarded)
 #	04(sp) - saved r0
 #	08(sp) - saved r1
 #	12(sp) - saved r6
 #	16(sp) - saved r7
 #	20(sp) - saved r8
 #	24(sp) - saved r9
 #	28(sp) - saved r10
 #	32(sp) - saved r11
 #	36(sp) - return pc to caller of vax$editpc
 #
 # output parameters:
 #
 #	these register contents are dictated by the vax architecture
 #
 #	if no overflow has occured, then this routine exits through the rsb
 #	instruction with the following output parameters:
 #
 #	r0 - length in digits of input decimal string
 #	r1 - address of most significant byte of input decimal string
 #	r2 - set to zero to conform to architecture
 #	r3 - backed up one byte to point to eo$end operator
 #	r4 - set to zero to conform to architecture
 #	r5 - address of one byte beyond destination character string
 #
 #	psl<v> is clear
 #
 #	if the v-bit is set, then control is transferred to vax$editpc_overflow
 #	where a check for decimal overflow exceptions is made
 #
 #	the registers are loaded with their correct contents and then saved on
 #	the stack as follows:
 #
 #
 #	00(sp) - saved r0
 #	04(sp) - saved r1
 #	08(sp) - saved r2
 #	12(sp) - saved r3
 #	16(sp) - saved r4
 #	20(sp) - saved r5
 #	24(sp) - saved r6
 #	28(sp) - saved r7
 #	32(sp) - saved r8
 #	36(sp) - saved r9
 #	40(sp) - saved r10
 #	44(sp) - saved r11
 #	48(sp) - return pc to caller of vax$editpc
 #
 #	psl<v> is set
 #-

eo$end_routine:
	addl2	$4,sp			# discard return pc to main loop
	decl	r3			# back up pattern pointer one byte
	bbc	$psl$v_z,r11,1f		# check for negative zero
	bicb2	$psl$m_n,r11		# turn off n-bit if zero
1:	tstl	r0			# any digits remaining?
	bneq	editpc_roprand_abort	# error if yes
	tstl	r9			# any zeros (r0<31:16>) remaining?
	bneq	editpc_roprand_abort	# error if yes
	clrl	r2			# architecture specifies that r2
	clrl	r4			#  and r4 are zero on exit
	bicpsw	$( psl$m_n | psl$m_z | psl$m_v | psl$m_c )	# clear condition codes
	bispsw	r11			# set codes according to saved psw
	bbs	$psl$v_v,r11,2f		# get out of line if overflow
 #	popr	$^m<r0,r1,r6,r7,r8,r9,r10,r11>	# restore saved registers
	 popr	$0x0fc3
	rsb				# return to caller's caller

 # at this point, we must determine whether the dv bit is set. the tests that 
 # must be performed are identical to the tests performed by the overflow
 # checking code for the packed decimal routines. in order to make use of
 # that code, we need to set up the saved registers on the stack to match
 # the input to that routine. note also that the decimal routines specify
 # that r0 is zero on completion while editpc dictates that r0 contains the
 # initial value of "srclen". for this reason, we cannot simply branch to
 # vax$decimal_exit but must use a special entry point.

2:
 #	popr	$^m<r0,r1>		# restore r0 and r1
	 popr	$0x03			#  but preserve condition codes
 #	pushr	$^m<r0,r1,r2,r3,r4,r5>	# ... only to save them again
	 pushr	$0x03f

 # the condition codes were not changed by the previous two instructions.

	jmp	vax$editpc_overflow	# join exit code

 #+
 # functional description:
 #
 #	this routine stores the intermediate state of an editpc instruction
 #	that has been prematurely terminated by an illegal pattern operator.
 #	these exceptions and access violations are the only exceptions from
 #	which execution can continue after the exceptional condition has been
 #	cleared up. after the state is stored in the registers r0 through r5,
 #	control is transferred through vax$roprand to vax$reflect_fault, where
 #	the appropriate backup method is determined, based on the return pc
 #	from the vax$editpc routine. 
 #
 # input parameters:
 #
 #	r0  - current digit count in input string
 #	r1  - address of next digit in input string
 #	r2  - fill character
 #	r3  - address of illegal pattern operator
 #	r4  - sign character (stored in r2<15:8>)
 #	r5  - address of next character to be stored in output character string
 #	r9  - zero count (stored in r0<31:16>)
 #	r11 - condition codes
 #
 #	00(sp) - saved r0
 #	04(sp) - saved r1
 #	08(sp) - saved r6
 #	12(sp) - saved r7
 #	16(sp) - saved r8
 #	20(sp) - saved r9
 #	24(sp) - saved r10
 #	28(sp) - saved r11
 #	32(sp) - return pc from vax$editpc routine
 #
 # output parameters:
 #
 #	00(sp) - offset in packed register array to delta pc byte
 #	04(sp) - return pc from vax$editpc routine
 #
 #	some of the register contents are dictated by the vax architecture.
 #	other register contents are architecturally described as "implementation
 #	dependent" and are used to store the instruction state that enables it
 #	to be restarted successfully and complete according to specifications.
 #
 #	the following register contents are architecturally specified
 #
 #		r0<15:00> - current digit count in input string
 #		r0<31:16> - current zero count (from r9)
 #		r1        - address of next digit in input string
 #		r2<07:00> - fill character
 #		r2<15:08> - sign character (from r4)
 #		r3        - address of next pattern operator
 #		r5        - address of next character in output character string
 #
 #	the following register contents are peculiar to this implementation
 #
 #		r2<23:16> - delta-pc (if initiated by exception)
 #		r2<31:24> - delta srcaddr (current srcaddr minus initial srcaddr)
 #		r4<07:00> - initial digit count (from saved r0)
 #		r4<15:08> - saved condition codes (for easy retrieval)
 #		r4<23:16> - state flags
 #				state = editpc_2_restart
 #				fpd bit is set
 #				accvio bit is clear
 #		r4<31:24> - unused for this exception (see access violations)
 #
 #		editpc_2_restart is the restart code that causes the 
 #		instruction to be restarted at the top of the main loop.
 #		It is the simplest point at which to resume execution after
 #		an illegal pattern operator fault.
 #
 #	the condition codes reported in the exception psl are also defined
 #	by the vax architecture.
 #
 #		psl<n> - source string has a minus sign
 #		psl<z> - all digits are zero so far
 #		psl<v> - nonzero digits have been lost
 #		psl<c> - significance
 #-

 #	assume editpc_l_saved_r1 eq <editpc_l_saved_r0 + 4>

editpc_roprand_fault:
 #	pushr	$^m<r0,r1,r2,r3>		# save current r0..r3
	 pushr	$0x0f
	movq	editpc_l_saved_r0(sp),r0	# retrieve original r0 and r1
	movq	r4,16(sp)			# save r4 and r5 in right placeon stack

 # now start stuffing the various registers 

	movw	r9,editpc_w_zero_count(sp)	# save r9 in r0<31:16>
	movb	r4,editpc_b_sign(sp)		# save r4 in r2<15:8>
	movb	r0,editpc_b_inisrclen(sp)	# save initial value of r0
	subl3	r1,editpc_a_srcaddr(sp),r1	# calculate srcaddr difference
	movb	r1,editpc_b_delta_srcaddr(sp)	# store it in r4<15:8>
	movb	r11,editpc_b_saved_psw(sp)	# save condition codes
	movb	$(editpc_m_fpd|editpc_2_restart),editpc_b_state(sp)
						# set the fpd bit

 #	popr	$^m<r0,r1,r2,r3,r4,r5,r6,r7,r8,r9,r10,r11> # load registers
 	 popr	$0x0fff
 #	pushl	$<editpc_b_delta_pc!-	# store delta-pc offset
 #		pack_m_fpd>		# indicate that fpd should be set
	 pushl	$(editpc_b_delta_pc|pack_m_fpd)

 # the following is admittedly gross. this is the only code path into
 # vax$roprand where the condition codes are significant. all other paths can
 # store the delta-pc offset without concern for its affect on condition
 # codes. fortunately, the popr instruction does not affect condition codes.


	pushl	r0			# get a scratch register
	extzv	$8,$4,r4,r0		# get codes from r4<11:8>
	bicpsw	$(psl$m_n | psl$m_z | psl$m_v | psl$m_c )# clear the codes
	bispsw	r0			# set relevant condition codes
 #	popr	$^m<r0>			# restore r0, preserving psw
	 popr	$0x01
	jmp	vax$roprand		# continue exception handling


 #-
 # functional description:
 #
 #	this routine reports a reserved operand abort back to the caller.
 #
 #	reserved operand aborts are trivial to handle because they cannot be
 #	continued. there is no need to pack intermediate state into the
 #	general registers. those registers that should not be modified by the
 #	editpc instruction have their contents restored. control is then
 #	passed to vax$roprand, which takes the necessary steps to eventually
 #	reflect the exception back to the caller. 
 #
 #	the following conditions cause a reserved operand abort
 #
 #	    1.	input digit count gtru 31
 #		(this condition is detected by the editpc initialization code.)
 #
 #	    2.	not enough digits in source string to satisfy pattern operators
 #		(this condition is detected by the eo_read routine.)
 #
 #	    3.	too many digits in source string (digits left over)
 #		(this condition is detected by the eo$end routine.)
 #
 #	    4.	an eo$end operator was encountered while zero count was nonzero
 #		(this condition is also detected by the eo$end routine.)
 #
 # input parameters:
 #
 #	00(sp) - saved r0
 #	04(sp) - saved r1
 #	08(sp) - saved r6
 #	12(sp) - saved r7
 #	16(sp) - saved r8
 #	20(sp) - saved r9
 #	24(sp) - saved r10
 #	28(sp) - saved r11
 #	32(sp) - return pc from vax$editpc routine
 #
 # output parameters:
 #
 #	the contents of r0 through r5 are not important because the 
 #	architecture states that they are unpredictable if a reserved
 #	operand abort occurs. no effort is made to put these registers
 #	into a consistent state.
 #
 #	r6 through r11 are restored to their values when the editpc 
 #	instruction began executing.
 #
 #	00(sp) - offset in packed register array to delta pc byte
 #	04(sp) - return pc from vax$editpc routine
 #
 # implicit output:
 #
 #	this routine passes control to vax$roprand where further
 #	exception processing takes place.
 #-

editpc_roprand_abort:
 #	popr	$^m<r0,r1,r6,r7,r8,r9,r10,r11>	# restore saved registers
	 popr	$0x0fc3
	pushl	$editpc_b_delta_pc		# store delta-pc offset
	jmp	vax$roprand			# continue exception handling






 #+
 # functional description:
 #
 #	this routine receives control when an access violation occurs while
 #	executing within the editpc emulator. this routine determines whether
 #	the exception occurred while accessing the source decimal string, the
 #	pattern stream, or the output character string. (this check is made
 #	based on the pc of the exception.) 
 #
 #	if the pc is one that is recognized by this routine, then the state of
 #	the instruction (character counts, string addresses, and the like) are
 #	restored to a state where the instruction/routine can be restarted
 #	after (if) the cause for the exception is eliminated. control is then
 #	passed to a common routine that sets up the stack and the exception
 #	parameters in such a way that the instruction or routine can restart
 #	transparently. 
 #
 #	if the exception occurs at some unrecognized pc, then the exception is
 #	reflected to the user as an exception that occurred within the
 #	emulator. 
 #
 #	there are two exceptions that can occur that are not backed up to
 #	appear as if they occurred at the site of the original emulated
 #	instruction. these exceptions will appear to the user as if they
 #	occurred inside the emulator itself. 
 #
 #	    1.	if stack overflow occurs due to use of the stack by one of 
 #		the routines, it is unlikely that this routine will even
 #		execute because the code that transfers control here must
 #		first copy the parameters to the exception stack and that
 #		operation would fail. (the failure causes control to be
 #		transferred to vms, where the stack expansion logic is
 #		invoked and the routine resumed transparently.) 
 #
 #	    2.	if assumptions about the address space change out from under 
 #		these routines (because an ast deleted a portion of the 
 #		address space or a similar silly thing), the handling of the 
 #		exception is unpredictable.
 #
 # input parameters:
 #
 #	r0  - value of sp when exception occurred
 #	r1  - pc at which exception occurred
 #	r2  - scratch
 #	r3  - scratch
 #	r10 - address of this routine (no longer needed)
 #
 #	00(sp) - value of r0 when exception occurred
 #	04(sp) - value of r1 when exception occurred
 #	08(sp) - value of r2 when exception occurred
 #	12(sp) - value of r3 when exception occurred
 #	16(sp) - return pc in exception dispatcher in operating system
 #
 #	20(sp) - first longword of system-specific exception data
 #	  .
 #	  .
 #	xx(sp) - last longword of system-specific exception data
 #
 #	the address of the next longword is the position of the stack when
 #	the exception occurred. r0 locates this address.
 #
 # r0 ->	xx+4(sp)     - instruction-specific data
 #	  .	     - optional instruction-specific data
 #	  .          - optional instruction-specific data
 #	xx+<4*m>(sp) - return pc from vax$editpc routine (m is the number
 #		       of instruction-specific longwords)
 #
 # implicit input:
 #
 #	it is assumed that the contents of all registers coming into this
 #	routine are unchanged from their contents when the exception occurred.
 #	(for r0 through r3, this assumption applies to the saved register
 #	contents on the top of the stack. any modification to these four
 #	registers must be made to their saved copies and not to the registers
 #	themselves.) 
 #
 #	it is further assumed that the exception pc is within the bounds of 
 #	this module. (violation of this assumption is simply an inefficiency.)
 #
 #	finally, the macro begin_mark_point should have been invoked at the
 #	beginning of this module to define the symbols 
 #
 #		module_base
 #		pc_table_base
 #		handler_table_base
 #		table_size
 #
 # output parameters:
 #
 #	if the exception is recognized (that is, if the exception pc is 
 #	associated with one of the mark points), control is passed to the 
 #	context-specific routine that restores the instruction state to a 
 #	uniform point from which the editpc instruction can be restarted.
 #
 #		r0  - value of sp when exception occurred
 #		r1  - scratch
 #		r2  - scratch
 #		r3  - scratch
 #		r10 - scratch
 #
 #	vax$editpc is different from the other emulated instructions in that 
 #	it requires intermediate state to be stored in r4 and r5 as well as r0 
 #	through r3. this requires that r4 and r5 also be saved on the stack so 
 #	that they can be manipulated in a consistent fashion. 
 #
 #	00(sp) - value of r0 when exception occurred
 #	04(sp) - value of r1 when exception occurred
 #	08(sp) - value of r2 when exception occurred
 #	12(sp) - value of r3 when exception occurred
 #	16(sp) - value of r4 when exception occurred
 #	20(sp) - value of r5 when exception occurred
 #	24(sp) - value of r0 when exception occurred
 #	28(sp) - value of r1 when exception occurred
 #	32(sp) - value of r2 when exception occurred
 #	36(sp) - value of r3 when exception occurred
 #	40(sp) - return pc in exception dispatcher in operating system
 #	 etc.
 #
 # r0 ->	zz(sp) - instruction-specific data begins here
 #
 #	if the exception pc occurred somewhere else (such as a stack access), 
 #	the saved registers are restored and control is passed back to the 
 #	host system with an rsb instruction.
 #
 # implicit output:
 #
 #	the register contents are modified to put the intermediate state of
 #	the instruction into a consistent state from which it can be
 #	continued. any registers saved by the vax$editpc routine are
 #	restored. 
 #-


editpc_accvio:
	movq	r4,-(sp)		# store r5 and r4 on the stack
	movq	16(sp),-(sp)		# ... and another copy of r3 and r2
	movq	16(sp),-(sp)		# ... and another copy of r1 and r0

	clrl	r2			# initialize the counter
	pushab	module_base		# store base address of this module
	subl2	(sp)+,r1		# get pc relative to this base

1:	cmpw	r1,pc_table_base[r2]	# is this the right pc?
	beql	3f			# exit loop if true
	aoblss	$table_size,r2,1b	# do the entire table

 # if we drop through the dispatching based on pc, then the exception is not 
 # one that we want to back up. we simply reflect the exception to the user.

2:	addl2	$16,sp			# discard duplicate saved r0 .. r3
	movq	(sp)+,r4		# restore r4 and r5
 #	popr	$^m<r0,r1,r2,r3>	# restore saved registers
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
 # eo_read packing routine
 #
 # functional description:
 #
 #	this routine executes if an access violation occurred in the eo_read
 #	subroutine while accessing the input packed decimal string. 
 #
 # input parameters:
 #
 #	r0 - address of top of stack when access violation occurred
 #
 #	00(r0) - return pc to caller of eo_read
 #	04(r0) - return pc to main vax$editpc control loop
 #	08(r0) - saved r0
 #	12(r0) - saved r1
 #	  etc.
 #
 # output parameters:
 #
 #	if the caller of this routine a recognized restart point, the restart
 #	code is stored in editpc_b_state in the saved register array, the
 #	psuedo stack pointer r0 is advanced by one, and control is passed to
 #	the general editpc_pack routine for final exception processing. 
 #
 #		r0 is advanced by one longword
 #
 #		00(r0) - return pc to main vax$editpc control loop
 #		04(r0) - saved r0
 #		08(r0) - saved r1
 #		  etc.
 #
 #		editpc_b_state(sp) - code that uniquely determines the caller
 #			of eo_read when the access violation was detected.
 #
 #	if the caller's pc is not recognized, the exception is dismissed from
 #	further modification.
 #-

read_1:
read_2:
	clrl	r2			# set table index to zero
	pushab	module_base		# prepare for pic arithmetic
	subl3	(sp)+,(r0)+,r1		# r1 contains relative pc
	subl2	$3,r1			# back up over bsbw instruction

4:	cmpw	r1,restart_pc_table_base[r2]	# check next pc offset
	beql	5f				# exit loop if match
	aoblss	$restart_table_size,r2,4b	# check for end of loop

 # if we drop through this loop, we got into the eo_read subroutine from
 # other than one of the three known call sites. we pass control back to
 # the general exception dispatcher.

	brb	2b			# join common code to dismiss exception

 # store the restart code appropriate to the return pc and join common code to
 # store the rest of the instruction state into the saved register array.


5:	addb3	$1,r2,editpc_b_state(sp)	# restart code base is 1, not 0
	incw	editpc_w_srclen(sp)		# digit never got read
	brb	7f				# join common code


 #+
 # packing routine for storage loops
 #
 # functional description:
 #
 #	all of the following labels are associated with exceptions that occur
 #	inside a loop that is reading digits from the input stream and
 #	optionally storing these or other characters in the output string.
 #	while it is a trivial matter to back up the output pointer and restart
 #	the loop from the beginning, it is somewhat more difficult to handle
 #	all of the cases that can occur with the input packed decimal string
 #	(because a byte can contain two digits). to avoid this complication,
 #	we add the ability to restart the various loops where they left off.
 #	in order to accomplish this, we need to store the loop count and,
 #	optionally, the latest input digit in the intermediate state array. 
 #
 #	the two entry points where the contents of r7 (the last digit read
 #	from the input stream) are significant are move_2 and float_3. all
 #	other entry points ignore the contents of r7. (note that these two
 #	entry points exit through label 60$ to store r7 in the saved register
 #	array.) 
 #
 # input parameters:
 #
 #	r0 - address of top of stack when access violation occurred
 #	r7 - latest digit read from input stream (move_2 and float_3 only)
 #	r8 - remaining loop count
 #
 #	00(r0) - return pc to main vax$editpc control loop
 #	04(r0) - saved r0
 #	08(r0) - saved r1
 #	  etc.
 #
 # output parameters:
 #
 #	a restart code that is unique for each entry is stored in the saved
 #	register array. the loop count (and the latest input digit, if
 #	appropriate) is also stored before passing control to editpc_pack. 
 #
 #	editpc_b_state(sp) - code that uniquely determines the code that
 #		was executing when the access violation was detected.
 #
 #	editpc_b_eo_read_char(sp) - latest digit read from the input string
 #
 #	editpc_b_loop_count(sp) - remaining loop count
 #
 # side effects:
 #
 #	r0 is unchanged by this code path
 #-

 #	assume editpc_v_state eq 0

fill_2:
	movb	$fill_2_restart,editpc_b_state(sp)
	brb	7f

move_2:
	movb	$move_2_restart,editpc_b_state(sp)
	brb	6f

move_3:
	movb	$move_3_restart,editpc_b_state(sp)
	brb	7f

float_2:
	movb	$float_2_restart,editpc_b_state(sp)
	brb	6f

float_3:
	movb	$float_3_restart,editpc_b_state(sp)
	brb	6f

float_4:
	movb	$float_4_restart,editpc_b_state(sp)
	brb	7f

blank_zero_2:
	movb	$blank_zero_2_restart,editpc_b_state(sp)
	brb	7f

6:	movb	r7,editpc_b_eo_read_char(sp)	# save result of latest read
7:	movb	r8,editpc_b_loop_count(sp)	# save loop counter
	brb	8f


 #+ 
 # functional description:
 #
 #	an access violation at editpc_1 indicates that the byte containing the
 #	sign of the input packed decimal string could not be read. there is
 #	little state to preserve. the key step here is to store a restart code
 #	that differentiates this exception from the large number that can be
 #	restarted at the top of the command loop. 
 #
 # input parameters:
 #
 #	00(r0) - saved r0
 #	04(r0) - saved r1
 #	  etc.
 #
 # output parameter:
 #
 #	editpc_b_state(sp) - code that indicates that instruction should
 #		be restarted at point where sign "digit" is fetched.
 #-

editpc_1:
	movb	$editpc_1_restart,editpc_b_state(sp)
	brb	editpc_pack




 #+
 # functional description:
 #
 #	this routine handles all of the simple access violations, those that
 #	can be backed up to the same intermediate state. in general, an access
 #	violation occurred in one of the simpler routines or at some other
 #	point where it is not difficult to back up the editpc operation to the
 #	top of the main dispatch loop. 
 #
 # input parameters:
 #
 #	r3 - points one byte beyond current pattern operator (except for 
 #		replace_sign_2 where it is one byte further along)
 #
 #	00(r0) - top_of_loop (return pc to main vax$editpc control loop)
 #	04(r0) - saved r0
 #	08(r0) - saved r1
 #	  etc.
 #
 # output parameters:
 #
 #	r3 must be decremented to point to the pattern operator that was being
 #	processed when the exception occurred. the return pc must be
 #	"discarded" to allow the registers to be restored and the return pc
 #	from vax$editpc to be located. 
 #
 #		r3 - points to current pattern operator
 #
 #		00(r0) - saved r0
 #		04(r0) - saved r1
 #		  etc.
 #
 # output parameter:
 #
 #	editpc_b_state(sp) - the restart point called editpc_2 is the place
 #		from which all "simple" access violations are restarted.
 #		this is essentially the location top_of_loop.
 #-

end_float_1:
	bbsc	$psl$v_c,r11,9f
	brb	9f
replace_sign_2:
	decl	editpc_a_pattern(sp)	# back up to "length" byte

editpc_3:
editpc_4:
editpc_5:

insert_1:
insert_2:

store_sign_1:

fill_1:

move_1:

float_1:

blank_zero_1:

replace_sign_1:

load_xxxx_1:
load_xxxx_2:

adjust_input_1:
9:	decl	editpc_a_pattern(sp)	# back up to current pattern operator
editpc_2:
	movb	$editpc_2_restart,editpc_b_state(sp)	# store special restart code
8:	addl2	$4,r0			# discard return pc
				# ... and drop through to editpc_pack



 #+
 # functional description:
 #
 #	this routine stores the intermediate state of an editpc instruction
 #	that has been prematurely terminated by an access violation. these
 #	exceptions and illegal pattern operators are the only exceptions from
 #	which execution can continue after the exceptional condition has been
 #	cleared up. after the state is stored in the registers r0 through r5,
 #	control is transferred to vax$reflect_fault, where the appropriate
 #	backup method is determined, based on the return pc from the
 #	vax$editpc routine. 
 #
 # input parameters:
 #
 #	r0  - current digit count in input string
 #	r1  - address of next digit in input string
 #	r2  - fill character
 #	r3  - address of current pattern operator
 #	r4  - sign character (stored in r2<15:8>)
 #	r5  - address of next character to be stored in output character string
 #	r9  - zero count (stored in r0<31:16>)
 #	r11 - condition codes
 #
 #	00(r0) - saved r0
 #	04(r0) - saved r1
 #	08(r0) - saved r6
 #	12(r0) - saved r7
 #	16(r0) - saved r8
 #	20(r0) - saved r9
 #	24(r0) - saved r10
 #	28(r0) - saved r11
 #	32(r0) - return pc from vax$editpc routine
 #
 # output parameters:
 #
 #	r0 - address of return pc from vax$editpc routine
 #
 #	00(r0) - return pc from vax$editpc routine
 #
 #	some of the register contents are dictated by the vax architecture.
 #	other register contents are architecturally described as "implementation
 #	dependent" and are used to store the instruction state that enables it
 #	to be restarted successfully and complete according to specifications.
 #
 #	the following register contents are architecturally specified
 #
 #		r0<15:00> - current digit count in input string
 #		r0<31:16> - current zero count (from r9)
 #		r1        - address of next digit in input string
 #		r2<07:00> - fill character
 #		r2<15:08> - sign character (from r4)
 #		r3        - address of current pattern operator
 #		r5        - address of next character in output character string
 #
 #	the following register contents are peculiar to this implementation
 #
 #		r2<23:16> - delta-pc (if initiated by exception)
 #		r2<31:24> - delta srcaddr (current srcaddr minus initial srcaddr)
 #		r4<07:00> - initial digit count (from saved r0)
 #		r4<15:08> - saved condition codes (for easy retrieval)
 #		r4<23:16> - state flags
 #				state field determines the restart point
 #				fpd bit is set
 #				accvio bit is set
 #		r4<31:24> - unused for this exception (see access violations)
 #
 #	the condition codes are not architecturally specified by the vax
 #	architecture for an access violation. the following list applies to
 #	some but not all of the points where an access violation can occur. 
 #
 #		psl<n> - source string has a minus sign
 #		psl<z> - all digits are zero so far
 #		psl<v> - nonzero digits have been lost
 #		psl<c> - significance
 #-


editpc_pack:

 # now start stuffing the various registers 

	movw	r9,editpc_w_zero_count(sp)	# save r9 in r0<31:16>
	movb	r4,editpc_b_sign(sp)		# save r4 in r2<15:8>
	movq	(r0)+,r2			# get initial r0/r1 to r2/r3
	movb	r2,editpc_b_inisrclen(sp)	# save initial value of r0
	subl3	r3,editpc_a_srcaddr(sp),r3	# calculate srcaddr difference
	movb	r3,editpc_b_delta_srcaddr(sp)	# store it in r4<15:8>
	movb	r11,editpc_b_saved_psw(sp)	# save condition codes
	bisb2	$editpc_m_fpd,editpc_b_state(sp)	# set the fpd bit

 # restore the remaining registers 

	movq	(r0)+,r6			# restore r6 and r7
	movq	(r0)+,r8			# ... and r8 and r9
	movq	(r0)+,r10			# ... and r10 and r11

 # get rid of the extra copy of saved registers on the stack

	movq	(sp)+,16(sp)		# copy the saved r0/r1 pair
	movq	(sp)+,16(sp)		# ... and the saved r2/r3 pair
	movq	(sp)+,r4		# r4 and r5 can be themselves

 # r1 contains delta-pc offset and indicates that fpd gets set

 #	movl	$<editpc_b_delta_pc!-	# locate delta-pc offset
 #		pack_m_fpd!-		# set fpd bit in exception psl
 #		pack_m_accvio>,r1	# indicate an access violation
	 movl	$(editpc_b_delta_pc|pack_m_fpd|pack_m_accvio),r1
	jmp	vax$reflect_fault	# reflect fault to caller



 #+
 # functional description:
 #
 #	this routine receives control when an editpc instruction is restarted. 
 #	the instruction state (stack and general registers) is restored to the 
 #	point where it was when the instruction (routine) was interrupted and 
 #	control is passed back to the top of the control loop or to another
 #	restart point.
 #
 # input parameters:
 #
 #	 31		  23 		   15		    07		  00
 #	+----------------+----------------+----------------+----------------+
 #	|       zero count		  |		srclen		    |
 #	+----------------+----------------+----------------+----------------+
 #	| delta-srcaddr  |  delta-pc      |    sign        |      fill      |
 #	+----------------+----------------+----------------+----------------+
 #      | loop-count     |   state        |   saved-psw    |     inisrclen  |
 #	+----------------+----------------+----------------+----------------+
 #	|			       dstaddr				    |
 #      +----------------+----------------+----------------+----------------+
 #
 #	depending on where the exception occurred, some of these parameters 
 #	may not be relevant. they are nevertheless stored as if they were 
 #	valid to make this restart code as simple as possible.
 #
 #	these register fields are more or less architecturally defined. they 
 #	are strictly specified for a reserved operand fault (illegal pattern 
 #	operator) and it makes sense to use the same register fields for 
 #	access violations as well.
 #
 #		r0<07:00> - current digit count in input string 
 #			(see eo_read_char below)
 #		r0<31:16> - current zero count (loaded into r9)
 #		r1        - address of next digit in input string
 #		r2<07:00> - fill character
 #		r2<15:08> - sign character (loaded into r4)
 #		r3        - address of next pattern operator
 #		r5        - address of next character in output character string
 #
 #	these register fields are specific to this implementation. 
 #
 #		r0<15:08> - latest digit from input string (loaded into r7)
 #		r2<23:16> - size of instruction (unused by this routine)
 #		r2<31:24> - delta srcaddr (used to compute saved r1)
 #		r4<07:00> - initial digit count (stored in saved r0)
 #		r4<15:08> - saved condition codes (stored in r11)
 #			psl<n> - source string has a minus sign
 #			psl<z> - all digits are zero so far
 #			psl<v> - nonzero digits have been lost
 #			psl<c> - significance
 #		r4<23:16> - state flags
 #			state field determines the restart point
 #		r4<31:24> - loop count (loaded into r8)
 #
 #	00(sp) - return pc from vax$editpc routine
 #
 # implicit input:
 #
 #	note that the initial "srclen" is checked for legality before any
 #	restartable exception can occur. this means that r0 lequ 31, which
 #	leaves bits <15:5> free for storing intermediate state. in the case of
 #	an access violation, r0<15:8> is used to store the latest digit read
 #	from the input stream. in the case of an illegal pattern operator,
 #	r0<15:5> are not used so that the architectural requirement that
 #	r0<15:0> contain the current byte count is adhered to. 
 #
 # output parameters:
 #
 #	all of the registers are loaded, even if some of their contents are 
 #	not relevant to the particular point at which the instruction will be 
 #	restarted. this makes the output of this routine conditional on a 
 #	single thing, namely on whether the restart point is in one of the 
 #	pattern-specific routines or in the outer vax$editpc routine. this
 #	comment applies especially to r7 and r8.
 #
 #	r0  - current digit count in input string
 #	r1  - address of next digit in input string
 #	r2  - fill character
 #	r3  - address of next pattern operator
 #	r4  - sign character (stored in r2<15:8>)
 #	r5  - address of next character to be stored in output character string
 #	r6  - scratch
 #	r7  - latest digit read from input packed decimal string
 #	r8  - loop count
 #	r9  - zero count (stored in r0<31:16>)
 #	r10 - address of editpc_accvio, this module's "condition handler"
 #	r11 - condition codes
 #
 #	00(sp) - saved r0
 #	04(sp) - saved r1
 #	08(sp) - saved r6
 #	12(sp) - saved r7
 #	16(sp) - saved r8
 #	20(sp) - saved r9
 #	24(sp) - saved r10
 #	28(sp) - saved r11
 #	32(sp) - return pc from vax$editpc routine
 #
 # side effects:
 #
 #	r6 is assumed unimportant and is used as a scratch register by this 
 #	routine as soon as it is saved.
 #-

		.globl	vax$editpc_restart
vax$editpc_restart:
 #	pushr	$^m<r0,r1,r2,r3,r4,r5,r6,r7,r8,r9,r10,r11>	# save them all
	 pushr	$0x0fff
 #	establish_handler	editpc_accvio	# reload r10 with handler address
	 movab	editpc_accvio,r10
	movzbl	r0,r0				# clear out r0<31:8>
	movzbl	editpc_b_sign(sp),r4		# put "sign" back into r4
 #	extzv	$editpc_v_state,-
 #		$editpc_s_state,-
 #		editpc_b_state(sp),r6		# put restart code into r6
	 extzv	$editpc_v_state,$editpc_s_state,editpc_b_state(sp),r6

 # the following two values are not used on all restart paths but r7 and r8
 # are loaded unconditionally to make this routine simpler. The most extreme
 # example is that r7 gets recalculated below for the editpc_1 restart point.

	movzbl	editpc_b_eo_read_char(sp),r7	# get latest input digit
	movzbl	editpc_b_loop_count(sp),r8	# restore loop count
	cvtwl	editpc_w_zero_count(sp),r9	# reset zero count (r9 lss 0)
	movzbl	editpc_b_saved_psw(sp),r11	# restore saved condition codes

 # the next three instructions reconstruct the initial values of "srclen" and
 # "srcaddr" and store them on the stack just above the saved r6. these values
 # will be loaded into r0 and r1 when the instruction completes execution.
 # note that these two instructions destroy information in the saved copy of
 # r4 so all of that information must be removed before these instructions
 # execute. 

	movzbl	editpc_b_delta_srcaddr(sp),editpc_l_saved_r1(sp)
	subl3	editpc_l_saved_r1(sp),editpc_a_srcaddr(sp),editpc_l_saved_r1(sp)
	addl2	editpc_a_srcaddr(sp),editpc_l_saved_r1(sp)
	movzbl	editpc_b_inisrclen(sp),editpc_l_saved_r0(sp)

 # the top four longwords are discarded and control is passed to the restart
 # point obtained from the restart pc table. note that there is an assumption
 # here that the first two restart points are different from the others in that
 # they do not have an additional return pc (top_of_loop) on the stack.

	addl2	$editpc_l_saved_r0,sp	# make saved registers r0, r1, r6, ...
	cmpl	r6,$editpc_1_restart	# check for restart in main routine
	blequ	1f			# branch if no return pc
	pushab	top_of_loop		# restart in some subroutine
	brb	2f			# use common code to resume execution

 # editpc_1 is restart point where r7 must contain the address of the byte
 # that containts the sign "digit". This address must be recalculated. Note
 # that this calculation overwrites the previous r7 restoration.
1:	extzv	$1,$4,r0,r7		# get byte offset to end of string
	addl2	r1,r7			# get address of byte containing sign

2:	movzwl	restart_pc_table_base-2[r6],r6	# convert code to pc offset
	jmp	module_base[r6]		# get back to work

 #	end_mark_point		editpc_m_state

module_end:
