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
/*	@(#)vaxstring.s	1.5		%G		*/

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
 *	Stephen Reilly, 15-Nov-84
 * 001- Fix bugs that were revieved from Larry Kenah.
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
 #	the routines in this module emulate the vax-11 string instructions.
 #	these procedures can be a part of an emulator package or can be
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
 #	16 august 1982
 #
 # modified by:
 #
 #	v04-001 ljk004		lawrence j. kenah	06-sep-1984
 #		The backup code for movtc when moving in the forward direction
 #		also needs to be changed (see ljk0039) based on the relative
 #		sizes of the source and destination strings.
 #
 #	v01-005	kdm0107		kathleen d. morse	21-aug-1984
 #		Fix bug in cmpc3.  Return C clear if string length is 0.
 #
 #	v01-004 ljk0039		lawrence j. kenah	20-jul-1984
 #		modify movtc backup code to reflect differences in register
 #		contents when traversing strings backwards.  There are two
 #		cases based on the relative sizes of source and destination.
 #
 #	v01-003	ljk0026		lawrence j. kenah	19-Mar-1984
 #		Final cleanup pass. Access violation handler is now
 #		called string_accvio. Set pack_m_accvio bit in r1
 #		before passing control to vax$reflect_fault
 #
 #	v01-002	ljk0011		lawrence j. kenah	8-nov-1983
 #		fix three minor bugs in movtc and movtuc. 
 #
 #	v01-001	original	lawrence j. kenah	16-aug-1982
 #--


 #+
 #	the following notes apply to most or all of the routines that appear in 
 #	this module. the comments appear here to avoid duplication in each routine.
 #
 #  1.	the vax architecture standard (dec std 032) is the ultimate authority on
 #	the functional behavior of these routines. a summary of each instruction
 #	that is emulated appears in the functional description section of each
 #	routine header. 
 #
 #  2.	one design goal that affects the algorithms used is that these instructions
 #	can incur exceptions such as access violations that will be reported to
 #	users in such a way that the exception appears to have originated at the
 #	site of the reserved instruction rather than within the emulator. this
 #	constraint affects the algorithms available and dictates specific
 #	implementation decisions. 
 #
 #  3.	each routine header contains a picture of the register usage when it is
 #	necessary to store the intermediate state of an instruction (routine) while
 #	servicing an exception. 
 #
 #	the delta-pc field is used by the condition handler jacket to these
 #	routines when it determines that an exception such as an access violation
 #	occurred in response to an explicit use of one of the reserved
 #	instructions. these routines can also be called directly with the input
 #	parameters correctly placed in registers. the delta-pc field is not used in
 #	this case. 
 #
 #	note that the input parameters to any routine are a subset of the
 #	intermediate state picture. 
 #
 #	fields that are not used either as input parameters or to store
 #	intermediate state are indicated thus, xxxxx. 
 #
 #  4.	in the input parameter list for each routine, certain register fields that
 #	are not used may be explicitly listed for one reason or another. these
 #	unused input parameters are described as irrelevant. 
 #
 #  5.	in general, the final condition code settings are determined as the side
 #	effect of one of the last instructions that executes before control is
 #	passed back to the caller with an rsb. it is seldom necessary to explicitly
 #	manipulate condition codes with a bixpsw instruction or similar means. 
 #
 #  6.	there is only a small set of exceptions that are reflected to the user in an
 #	altered fashion, with the exception pc changed from within the emulator to
 #	the site of the original entry into these routines. the instructions that
 #	generate these exceptions are all immediately preceded by a 
 #
 #		mark_point	yyyy_n
 #
 #	where yyyy is the instruction name and n is a small integer. these names
 #	map directly into instruction- and context-specific routines (located at
 #	the end of this module) that put each instruction (routine) into a
 #	consistent state before passing control to a more general exception handler
 #	in a different module. 
 #-






 # include files:

 #	$psldef				# define bit fields in psl

 #	pack_def			# stack usage for exception handling


 # macro definitions

 #	.macro	_include	opcode , boot_flag
 #	.if	not_defined	boot_switch
 #		opcode'_def
 #		include_'opcode = 0
 #	.if_false
 #		.if	identical	<boot_flag> , boot 
 #			opcode'_def
 #			include_'opcode = 0
 #		.endc
 #	.endc
 #	.endm	_include


 # global and external declarations

 #	.disable	global

 #	.if	not_defined	boot_switch
 #	.external	vax$reflect_fault,-	# reflect access violation
 #			vax$reflect_to_vms	# reflect unrecognized exception
 #	.endc

 # psect declarations:

 #		begin_mark_point
	.text
	.text	2
	.set	table_size,0
pc_table_base:
	.text	3
handler_table_base:
	.text
module_base:

 #+
 # functional description:
 #
 #	the source string specified by the  source  length  and  source  address
 #	operands  is translated and replaces the destination string specified by
 #	the destination length and destination address operands.  translation is
 #	accomplished  by using each byte of the source string as an index into a
 #	256 byte table whose zeroth entry address  is  specified  by  the  table
 #	address operand.  the byte selected replaces the byte of the destination
 #	string.  if the destination string is longer than the source string, the
 #	highest  addressed  bytes  of the destination string are replaced by the
 #	fill operand.  if the destination string  is  shorter  than  the  source
 #	string,  the  highest  addressed  bytes  of  the  source  string are not
 #	translated and moved.  the operation of the  instruction  is  such  that
 #	overlap  of  the  source  and  destination  strings  does not affect the
 #	result.  if the destination string overlaps the translation  table,  the
 #	destination string is unpredictable.
 #
 # input parameters:
 #
 #	the following register fields contain the same information that
 #	exists in the operands to the movtc instruction.
 #
 #		r0<15:0> = srclen	length of source string
 #		r1       = srcaddr	address of source string
 #		r2<7:0>  = fill		fill character
 #		r3       = tbladdr	address of 256-byte table
 #		r4<15:0> = dstlen	length of destination string
 #		r5       = dstaddr	address of destination string
 #
 #	in addition to the input parameters that correspond directly to
 #	operands to the movtc instruction, there are other input parameters 
 #	to this routine. note that the two inixxxlen parameters are only
 #	used when the movtc_v_fpd bit is set in the flags byte.
 #
 #		r2<15:8>  = flags	instruction-specific status
 #
 #	the contents of the flags byte must be zero (mbz) on entry to this
 #	routine from the outside world (through the emulator jacket or by
 #	a jsb call). if the initial contents of flags are not zero, the
 #	actions of this routine are unpredictable. 
 #
 #	there are two other input parameters whose contents depend on
 #	the settings of the flags byte.
 #
 #	movtc_v_fpd bit in flags is clear
 #
 #		r0<31:16> = irrelevant
 #		r4<31:16> = irrelevant
 #
 #	movtc_v_fpd bit in flags is set
 #
 #		r0<31:16> = inisrclen	initial length of source string
 #		r4<31:16> = inidstlen	initial length of destination string
 #
 # intermediate state:
 #
 #     31               23               15               07            00
 #    +----------------+----------------+----------------+----------------+
 #    |         initial srclen          |             srclen              | : r0
 #    +----------------+----------------+----------------+----------------+
 #    |                              srcaddr                              | : r1
 #    +----------------+----------------+----------------+----------------+
 #    |    delta-pc    |     xxxx       |     flags      |      fill      | : r2
 #    +----------------+----------------+----------------+----------------+
 #    |                              tbladdr                              | : r3
 #    +----------------+----------------+----------------+----------------+
 #    |         initial dstlen          |             dstlen              | : r4
 #    +----------------+----------------+----------------+----------------+
 #    |                              dstaddr                              | : r5
 #    +----------------+----------------+----------------+----------------+
 #
 # output parameters:
 #
 #	source string longer than destination string
 #
 #		r0 = number of bytes remaining in the source string 
 #		r1 = address of one byte beyond last byte in source string
 #			that was translated (the first untranslated byte)
 #		r2 = 0
 #		r3 = tbladdr	address of 256-byte table
 #		r4 = 0 (number of bytes remaining in the destination string)
 #		r5 = address of one byte beyond end of destination string
 #
 #	source string same size as or smaller than destination string
 #
 #		r0 = 0 (number of bytes remaining in the source string)
 #		r1 = address of one byte beyond end of source string
 #		r2 = 0
 #		r3 = tbladdr	address of 256-byte table
 #		r4 = 0 (number of bytes remaining in the destination string)
 #		r5 = address of one byte beyond end of destination string
 #
 # condition codes:
 #
 #	n <- srclen lss dstlen
 #	z <- srclen eql dstlen
 #	v <- 0
 #	c <- srclen lssu dstlen
 #
 # side effects:
 #
 #	this routine uses up to four longwords of stack space.
 #-

		.globl	vax$movtc
vax$movtc:
	pushl	r4			# store dstlen on stack
	pushl	r0			# store srclen on stack


	bbs	$(movtc_v_fpd+8),r2,L5	# branch if instruction was interrupted
	movw	(sp),2(sp)		# set the initial srclen on stack
	movw	4(sp),6(sp)		# set the initial dstlen on stack
L5:	pushl	r10			# save r10 so it can hold handler 
 #	establish_handler	string_accvio
	 movab	string_accvio,r10
	movzwl	r4,r4			# clear unused bits of dstlen
	beql	L40			# all done if zero
	movzwl	r0,r0			# clear unused bits of srclen
	beql	L20			# add fill character to destination
	cmpl	r1,r5			# check relative position of strings
	blssu	move_backward		# perform move from end of strings

 # this code executes if the source string is at a larger virtual address
 # than the destination string. the movement takes place from the front
 # (small address end) of each string to the back (high address end).

move_forward:
	pushl	r2			# allow r2 (fill) to be used as scratch
	subl2	r0,r4			# get difference between strings
	bgequ	L10			# branch if fill work to do eventually
	movzwl	12(sp),r0		# use dstlen (saved r4) as srclen (r0)

 #	MARK_POINT	MOVTC_1
	.text	2
	.set	table_size, table_size + 1
	.word	Lmovtc_1 - module_base
	.text	3
	.word	movtc_1 - module_base
	.text
Lmovtc_1:
L10:	movzbl	(r1)+,r2		# get next character from source
 #	MARK_POINT	MOVTC_2
	.text	2
	.set	table_size, table_size + 1
	.word	Lmovtc_2 - module_base
	.text	3
	.word	movtc_2 - module_base
	.text
Lmovtc_2:
	movb	(r3)[r2],(r5)+		# move translated character
	sobgtr	r0,L10			# source all done?

	movl	(sp)+,r2		# retrieve fill character from stack
	tstl	r4			# do we need to fill anything?
	bleq	L80			# skip to exit code if no fill work

 #	MARK_POINT	MOVTC_3
	.text	2
	.set	table_size, table_size + 1
	.word	Lmovtc_3 - module_base
	.text	3
	.word	movtc_3 - module_base
	.text
Lmovtc_3:
L20:	movb	r2,(r5)+		# fill next character
	sobgtr	r4,L20			# destination all done?

 # this is the common exit path. r2 is cleared to conform to its output
 # setting. the condition codes are determined by the original lengths
 # of the source and destination strings that were saved on the stack.

L30:	clrl	r2			# r2 is zero on return
	movl	(sp)+,r10		# restore saved r10
	ashl	$-16,(sp),(sp)		# get initial srclen
	ashl	$-16,4(sp),4(sp)	# get initial dstlen
	cmpl	(sp)+,(sp)+		# set condition codes
	rsb

 # the following instruction is the exit path when the destination string
 # has zero length on input.

L40:	movzwl	r0,r0			# clear unused bits of srclen
	brb	L30			# exit through common code

 # this code executes if the source string is at a smaller virtual address
 # than the destination string. the movement takes place from the back
 # (high address end) of each string to the front (low address end).

move_backward:
	addl2	r4,r5			# point r5 one byte beyond destination
	subl2	r0,r4			# get amount of fill work to do
	bgtru	L50			# branch to fill loop if work to do
	movzwl	8(sp),r0		# use dstlen (saved r4) as srclen (r0)
	brb	L60			# skip loop that does fill characters

 #	MARK_POINT	MOVTC_4
	.text	2
	.set	table_size, table_size + 1
	.word	Lmovtc_4 - module_base
	.text	3
	.word	movtc_4 - module_base
	.text
Lmovtc_4:
L50:	movb	r2,-(r5)		# load fill characters from the back
	sobgtr	r4,L50			# continue until excess all done

L60:	addl2	r0,r1			# point r1 to "modified end" of source

 # move transtaled characters from the high-address end toward the low-address 
 # end. note that the fill character is no longer needed so that r2 is 
 # available as a scratch register.

 #	MARK_POINT	MOVTC_5
	.text	2
	.set	table_size, table_size + 1
	.word	Lmovtc_5 - module_base
	.text	3
	.word	movtc_5 - module_base
	.text
Lmovtc_5:
L70:	movzbl	-(r1),r2		# get next character
 #	MARK_POINT	MOVTC_6
	.text	2
	.set	table_size, table_size + 1
	.word	Lmovtc_6 - module_base
	.text	3
	.word	movtc_6 - module_base
	.text
Lmovtc_6:
	movb	(r3)[r2],-(r5)		# move translated character
	sobgtr	r0,L70			# continue until source is exhausted

 # at this point, r1 points to the first character in the source string and r5
 # points to the first character in the destination string. this is the result
 # of operating on the strings from back to front (high-address end to
 # low-address end). these registers must be modified to point to the ends of
 # their respective strings. this is accomplished by using the saved original
 # lengths of the two strings. note that at this stage of the routine, r2 is
 # no longer needed and so can be used as a scratch register. 

	movzwl	6(sp),r2		# get original source length
	addl2	r2,r1			# point r1 to end of source string
	movzwl	10(sp),r2		# get original destination length
	addl2	r2,r5			# point r5 to end of destination string

 # if r1 is negative, this indicates that the source string is smaller than the
 # destination. r1 must be readjusted to point to the first byte that was not
 # translated. r0, which contains zero, must be loaded with the number of bytes
 # that were not translated (the negative of the contents of r4).

	tstl	r4			# any more work to do?
	beql	L30			# exit through common code
	addl2	r4,r1			# back up r1 (r4 is negative)

 # the exit code for move_forward also comes here is the source is longer than
 # (or equal to) the destination. note that in the case of r4 containing zero,
 # some extra work that accomplishes nothing must be done. this extra work in
 # the case of equal strings avoids two extra instructions in all cases.

L80:	mnegl	r4,r0			# remaining source length to r0
	clrl	r4			# r4 is always zero on exit
	brb	L30			# exit through common code

 #+
 # functional description:
 #
 #	the source string specified by the  source  length  and  source  address
 #	operands  is translated and replaces the destination string specified by
 #	the destination length and destination address operands.  translation is
 #	accomplished by using each byte of the source string as index into a 256
 #	byte table whose zeroth entry address is specified by the table  address
 #	operand.  the byte selected replaces the byte of the destination string.
 #	translation continues until a translated byte is  equal  to  the  escape
 #	byte  or until the source string or destination string is exhausted.  if
 #	translation is terminated because of escape the condition code v-bit  is
 #	set#   otherwise  it is cleared.  if the destination string overlaps the
 #	table,  the  destination  string  and  registers  r0  through   r5   are
 #	unpredictable.   if the source and destination strings overlap and their
 #	addresses are not identical, the destination  string  and  registers  r0
 #	through  r5  are  unpredictable.   if  the source and destination string
 #	addresses are identical, the translation is performed correctly.
 #
 # input parameters:
 #
 #	the following register fields contain the same information that
 #	exists in the operands to the movtuc instruction.
 #
 #		r0<15:0> = srclen	length of source string
 #		r1       = srcaddr	address of source string
 #		r2<7:0>  = fill		escape character
 #		r3       = tbladdr	address of 256-byte table
 #		r4<15:0> = dstlen	length of destination string
 #		r5       = dstaddr	address of destination string
 #
 #	in addition to the input parameters that correspond directly to
 #	operands to the movtuc instruction, there are other input parameters 
 #	to this routine. note that the two inixxxlen parameters are only
 #	used when the movtuc_v_fpd bit is set in the flags byte.
 #
 #		r2<15:8>  = flags	instruction-specific status
 #
 #	the contents of the flags byte must be zero (mbz) on entry to this
 #	routine from the outside world (through the emulator jacket or by
 #	a jsb call). if the initial contents of flags are not zero, the
 #	actions of this routine are unpredictable. 
 #
 #	there are two other input parameters whose contents depend on
 #	the settings of the flags byte.
 #
 #	movtuc_v_fpd bit in flags is clear
 #
 #		r0<31:16> = irrelevant
 #		r4<31:16> = irrelevant
 #
 #	movtuc_v_fpd bit in flags is set
 #
 #		r0<31:16> = inisrclen	initial length of source string
 #		r4<31:16> = inidstlen	initial length of destination string
 #
 # intermediate state:
 #
 #     31               23               15               07            00
 #    +----------------+----------------+----------------+----------------+
 #    |         initial srclen          |             srclen              | : r0
 #    +----------------+----------------+----------------+----------------+
 #    |                              srcaddr                              | : r1
 #    +----------------+----------------+----------------+----------------+
 #    |    delta-pc    |     xxxx       |     flags      |      esc       | : r2
 #    +----------------+----------------+----------------+----------------+
 #    |                              tbladdr                              | : r3
 #    +----------------+----------------+----------------+----------------+
 #    |         initial dstlen          |             dstlen              | : r4
 #    +----------------+----------------+----------------+----------------+
 #    |                              dstaddr                              | : r5
 #    +----------------+----------------+----------------+----------------+
 #
 # output parameters:
 #
 #	the final state of this instruction (routine) can exist in one of 
 #	three forms, depending on the relative lengths of the source and
 #	destination strings and whether a translated character matched the
 #	escape character.
 #
 #    1.	some byte matched escape character 
 #
 #		r0 = number of bytes remaining in the source string (including 
 #			the byte that caused the escape)
 #		r1 = address of the byte that caused the escape
 #		r2 = 0
 #		r3 = tbladdr	address of 256-byte table
 #		r4 = number of bytes remaining in the destination string 
 #		r5 = address of byte that would have received the translated byte
 #
 #    2.	destination string exhausted
 #
 #		r0 = number of bytes remaining in the source string 
 #		r1 = address of the byte that resulted in exhaustion
 #		r2 = 0
 #		r3 = tbladdr	address of 256-byte table
 #		r4 = 0 (number of bytes remaining in the destination string)
 #		r5 = address of one byte beyond end of destination string
 #
 #    3.	source string exhausted
 #
 #		r0 = 0 (number of bytes remaining in the source string)
 #		r5 = address of one byte beyond end of source string
 #		r2 = 0
 #		r3 = tbladdr	address of 256-byte table
 #		r4 = number of bytes remaining in the destination string 
 #		r5 = address of byte that would have received the translated byte
 #
 # condition codes:
 #
 #	n <- srclen lss dstlen
 #	z <- srclen eql dstlen
 #	v <- set if terminated by escape
 #	c <- srclen lssu dstlen
 #
 # side effects:
 #
 #	this routine uses five longwords of stack.
 #-

	.globl	vax$movtuc

vax$movtuc:
	pushl	r4			# store dstlen on stack
	pushl	r0			# store srclen on stack


	bbs	$(movtuc_v_fpd+8),r2,L15	# branch if instruction was interrupted
	movw	(sp),2(sp)		# set the initial srclen on stack
	movw	4(sp),6(sp)		# set the initial dstlen on stack
L15:	movzwl	r4,r4			# clear unused bits of dstlen
	beql	L150			# almost done if zero length
	movzwl	r0,r0			# clear unused bits of srclen
	beql	L140			# done if zero length
	pushl	r10			# save r10 so it can hold handler 
 #	establish_handler	string_accvio
	 movab	string_accvio,r10
	pushl	r7			# we need some scratch registers
	pushl	r6			# 

 # note that all code must now exit through a code path that restores r6
 # r7, and r10 to insure that the stack is correctly aligned and that these 
 # register contents are preserved across execution of this routine.

 # the following initialization routine is designed to make the main loop
 # execute faster. it performs three actions.
 #
 #	r7 <- smaller of r0 and r4 (srclen and dstlen)
 #
 #	larger of r0 and r4 is replaced by the difference between r0 and r4.
 #
 #	smaller of r0 and r4 is replaced by zero.
 #
 # this initializes r0 and r4 to their final states if either the source 
 # string or the destination string is exhausted. in the event that the loop
 # is terminated through the escape path, these two registers are readjusted
 # to contain the proper values as if they had each been advanced one byte
 # for each trip through the loop.

	subl2	r0,r4			# replace r4 with (r4-r0)
	blssu	L110			# branch if srclen gtru dstlen

 # code path for srclen (r0) lequ dstlen (r4). r4 is already correctly loaded.

	movl	r0,r7			# load r7 with smaller (r0)
	clrl	r0			# load smaller (r0) with zero
	brb	L120			# merge with common code at top of loop

 # code path for srclen (r0) gtru dstlen (r4). 

L110:	movzwl	16(sp),r7		# load r7 with smaller (use saved r4)
	mnegl	r4,r0			# load larger (r0) with abs(r4-r0)
	clrl	r4			# load smaller (r4) with zero

 # the following is the main loop in this routine.

 #	MARK_POINT	MOVTUC_1
	.text	2
	.set	table_size, table_size + 1
	.word	Lmovtuc_1 - module_base
	.text	3
	.word	movtuc_1 - module_base
	.text
Lmovtuc_1:
L120:	movzbl	(r1)+,r6		# get next character from source string
 #	MARK_POINT	MOVTUC_2
	.text	2
	.set	table_size, table_size + 1
	.word	Lmovtuc_2 - module_base
	.text	3
	.word	movtuc_2 - module_base
	.text
Lmovtuc_2:
	movzbl	(r3)[r6],r6		# convert to translated character
	cmpb	r2,r6			# does it match escape character?
	beql	escape			# exit loop if yes
 #	MARK_POINT	MOVTUC_3
	.text	2
	.set	table_size, table_size + 1
	.word	Lmovtuc_3 - module_base
	.text	3
	.word	movtuc_3 - module_base
	.text
Lmovtuc_3:
	movb	r6,(r5)+		# move translated character to 
					#  destination string
	sobgtr	r7,L120			# shorter string exhausted?

 # the following exit path is taken when the shorter of the source string and
 # the destination string is exhausted

L130:	movq	(sp)+,r6		# restore contents of scratch register
	movl	(sp)+,r10		# restore saved r10
L140:	clrl	r2			# r2 must be zero on output
	ashl	$-16,(sp),(sp)		# get initial srclen
	ashl	$-16,4(sp),4(sp)	# get initial dstlen
	cmpl	(sp)+,(sp)+		# set condition codes (v-bit always 0)
	rsb				# return

 # this code executes if the destination string has zero length. the source
 # length is set to a known state so that the common exit path can be taken.

L150:	movzwl	r0,r0			# clear unused bits of srclen
	brb	L140			# exit through common code

 # this code executes if the escape character matches the entry in the
 # 256-byte table indexed by the character in the source string. registers
 # r0 and r4 must be adjusted to indicate that neither string was exhausted.
 # the last step taken before return sets the v-bit.

escape:
	decl	r1			# reset r1 to correct byte in source 
	clrl	r2			# r2 must be zero on output
	addl2	r7,r0			# adjust saved srclen
	addl2	r7,r4			# adjust saved dstlen
	movq	(sp)+,r6		# restore contents of scratch registers
	movl	(sp)+,r10		# restore saved r10
	ashl	$-16,(sp),(sp)		# get initial srclen
	ashl	$-16,4(sp),4(sp)	# get initial dstlen
	cmpl	(sp)+,(sp)+		# set condition codes (v-bit always 0)
	bispsw	$psl$m_v		# set v-bit to indicate escape
	rsb				# return

 #+
 # functional description:
 #
 #	the bytes of string 1 specified by the length and address 1 operands are
 #	compared  with the bytes of string 2 specified by the length and address
 #	2 operands.  comparison proceeds until inequality is detected or all the
 #	bytes  of  the strings have been examined.  condition codes are affected
 #	by the result of the last byte  comparison.   two  zero  length  strings
 #	compare equal (i.e.  z is set and n, v, and c are cleared).
 #
 # input parameters:
 #
 #	r0<15:0> = len		length of character strings
 #	r1       = src1addr	address of first character string (called s1)
 #	r3       = src2addr	address of second character string (called s2)
 #
 # intermediate state:
 #
 #     31               23               15               07            00
 #    +----------------+----------------+----------------+----------------+
 #    |    delta-pc    |      xxxx      |               len               | : r0
 #    +----------------+----------------+----------------+----------------+
 #    |                             src1addr                              | : r1
 #    +----------------+----------------+----------------+----------------+
 #    |                               xxxxx                               | : r2
 #    +----------------+----------------+----------------+----------------+
 #    |                             src2addr                              | : r3
 #    +----------------+----------------+----------------+----------------+
 #
 # output parameters:
 #
 #	strings are identical
 #
 #		r0 = 0
 #		r1 = address of one byte beyond end of s1
 #		r2 = 0 (same as r0)
 #		r1 = address of one byte beyond end of s2
 #
 #	strings do not match
 #
 #		r0 = number of bytes left in strings (including first byte
 #			that did not match)
 #		r1 = address of nonmatching byte in s1
 #		r2 = r0
 #		r3 = address of nonmatching byte in s2
 #
 # condition codes:
 #
 #	in general, the condition codes reflect whether or not the strings
 #	are considered the same or different. in the case of different
 #	strings, the condition codes reflect the result of the comparison
 #	that indicated that the strings are not equal.
 #
 #	strings are identical
 #
 #		n <- 0
 #		z <- 1			# (byte in s1) eql (byte in s2)
 #		v <- 0
 #		c <- 0
 #
 #	strings do not match
 #
 #		n <- (byte in s1) lss (byte in s2)
 #		z <- 0			# (byte in s1) neq (byte in s2)
 #		v <- 0
 #		c <- (byte in s1) lssu (byte in s2)
 #
 #	where "byte in s1" or "byte in s2" may indicate the fill character
 #
 # side effects:
 #
 #	this routine uses one longword of stack.
 #-

	.globl	vax$cmpc3

vax$cmpc3:
	movzwl	r0,r0			# clear unused bits & check for zero
	beql	2f			# simply return if zero length string

	pushl	r10			# save r10 so it can hold handler 
 #	establish_handler	string_accvio
	 movab	string_accvio,r10
 #	MARK_POINT	CMPC3_1
	.text	2
	.set	table_size, table_size + 1
	.word	Lcmpc3_1 - module_base
	.text	3
	.word	cmpc3_1 - module_base
	.text
Lcmpc3_1:
1:	cmpb	(r3)+,(r1)+		# character match?
	bneq	3f			# exit loop if different
	sobgtr	r0,1b

 # exit path for strings identical (r0 = 0, either on input or after loop)

	movl	(sp)+,r10		# restore saved r10
2:	clrl	r2			# set r2 for output value of 0
	tstl	r0			# set condition codes
	rsb				# return point for identical strings

 # exit path when strings do not match

3:	movl	(sp)+,r10		# restore saved r10
	movl	r0,r2			# r0 and r2 are the same on exit
	cmpb	-(r1),-(r3)		# reset r1 and r3 and set condition codes
	rsb				# return point when strings do not match

 #+
 # functional description:
 #
 #	the bytes of the string 1 specified  by  the  length  1  and  address  1
 #	operands  are  compared  with the bytes of the string 2 specified by the
 #	length 2 and address 2 operands.  if  one  string  is  longer  than  the
 #	other,  the shorter string is conceptually extended to the length of the
 #	longer by appending (at  higher  addresses)  bytes  equal  to  the  fill
 #	operand.   comparison  proceeds  until inequality is detected or all the
 #	bytes of the strings have been examined.  condition codes  are  affected
 #	by  the  result  of  the  last byte comparison.  two zero length strings
 #	compare equal (i.e.  z is set and n, v, and c are cleared).
 #
 # input parameters:
 #
 #	r0<15:0>  = len		length of first character string (called s1)
 #	r0<23:16> = fill	fill character that is used when strings have
 #				   different lengths
 #	r1        = addr	address of first character string 
 #	r2<15:0>  = len		length of second character string (called s2)
 #	r3        = addr	address of second character string 
 #
 # intermediate state:
 #
 #     31               23               15               07            00
 #    +----------------+----------------+----------------+----------------+
 #    |    delta-pc    |      fill      |             src1len             | : r0
 #    +----------------+----------------+----------------+----------------+
 #    |                             src1addr                              | : r1
 #    +----------------+----------------+----------------+----------------+
 #    |              xxxxx              |             src2len             | : r2
 #    +----------------+----------------+----------------+----------------+
 #    |                             src2addr                              | : r3
 #    +----------------+----------------+----------------+----------------+
 #
 # output parameters:
 #
 #	strings are identical
 #
 #		r0 = 0
 #		r1 = address of one byte beyond end of s1
 #		r2 = 0 (same as r0)
 #		r1 = address of one byte beyond end of s2
 #
 #	strings do not match
 #
 #		r0 = number of bytes remaining in s1 when mismatch detected
 #			(or zero if s1 exhausted before mismatch detected)
 #		r1 = address of nonmatching byte in s1
 #		r2 = number of bytes remaining in s2 when mismatch detected
 #			(or zero if s2 exhausted before mismatch detected)
 #		r3 = address of nonmatching byte in s2
 #
 # condition codes:
 #
 #	in general, the condition codes reflect whether or not the strings
 #	are considered the same or different. in the case of different
 #	strings, the condition codes reflect the result of the comparison
 #	that indicated that the strings are not equal.
 #
 #	strings are identical
 #
 #		n <- 0
 #		z <- 1			# (byte in s1) eql (byte in s2)
 #		v <- 0
 #		c <- 0
 #
 #	strings do not match
 #
 #		n <- (byte in s1) lss (byte in s2)
 #		z <- 0			# (byte in s1) neq (byte in s2)
 #		v <- 0
 #		c <- (byte in s1) lssu (byte in s2)
 #
 #	where "byte in s1" or "byte in s2" may indicate the fill character
 #
 # side effects:
 #
 #	this routine uses two longwords of stack.
 #-

	.globl	vax$cmpc5

vax$cmpc5:
	pushl	r10			# save r10 so it can hold handler 
 #	establish_handler	string_accvio
	 movab	string_accvio,r10
	pushl	r4			# save register
	ashl	$-16,r0,r4		# get escape character
	movzwl	r0,r0			# clear unused bits & is s1 length zero?
	beql	L250			# branch if yes
	movzwl	r2,r2			# clear unused bits & is s2 length zero?
	beql	L230

 # main loop. the following loop executes when both strings have characters
 # remaining and inequality has not yet been detected.

 # the following loop is a target for further optimization in that the
 # loop should not require two sobgtr instructions. note, though, that
 # the current unoptimized loop is easier to back up.

 #	MARK_POINT	CMPC5_1
	.text	2
	.set	table_size, table_size + 1
	.word	Lcmpc5_1 - module_base
	.text	3
	.word	cmpc5_1 - module_base
	.text
Lcmpc5_1:
L210:	cmpb	(r1)+,(r3)+		# characters match?
	bneq	L280			# exit loop if bytes different
	sobgtr	r0,L220			# check for s1 exhausted

 # the next test determines whether s2 is also exhausted.

	decl	r2			# put r2 in step with r0
	bneq	L260			# branch if bytes remaining in s2

 # this is the exit path for identical strings. if we get here, then both
 # r0 and r2 are zero. the condition codes are correctly set (by the ashl
 # instruction) so that registers are restored with a popr to aviod changing
 # the condition codes.

identical:
 #	popr	$^m<r4,r10>		# restore saved registers
	 popr	$0x410
	rsb				# exit indicating identical strings

L220:	sobgtr	r2,L210			# check for s2 exhausted

 # the following loop is entered when all of s2 has been processed but
 # there are characters remaining in s1. in other words, 
 #
 #	r0 gtru 0
 #	r2 eql 0
 #
 # the remaining characters in s1 are compared to the fill character.

 #	MARK_POINT	CMPC5_2
	.text	2
	.set	table_size, table_size + 1
	.word	Lcmpc5_2 - module_base
	.text	3
	.word	cmpc5_2 - module_base
	.text
Lcmpc5_2:
L230:	cmpb	(r1)+,r4		# characters match?
	bneq	L240			# exit loop if no match
	sobgtr	r0,L230			# any more bytes in s1?

	brb	identical		# exit indicating identical strings

L240:	cmpb	-(r1),r4		# reset r1 and set condition codes
	brb	no_match		# exit indicating strings do not match

 # the following code executes if s1 has zero length on input. if s2 also
 # has zero length, the routine smply returns, indicating equal strings.

L250:	movzwl	r2,r2			# clear unused bits. is s2 len also zero?
	beql	identical		# exit indicating identical strings

 # the following loop is entered when all of s1 has been processed but
 # there are characters remaining in s2. in other words, 
 #
 #	r0 eql 0
 #	r2 gtru 0
 #
 # the remaining characters in s2 are compared to the fill character.

 #	MARK_POINT	CMPC5_3
	.text	2
	.set	table_size, table_size + 1
	.word	Lcmpc5_3 - module_base
	.text	3
	.word	cmpc5_3 - module_base
	.text
Lcmpc5_3:
L260:	cmpb	r4,(r3)+		# characters match?
	bneq	L270			# exit loop if no match
	sobgtr	r2,L260			# any more bytes in s2?

	brb	identical		# exit indicating identical strings

L270:	cmpb	r4,-(r3)		# reset r3 and set condition codes
	brb	no_match		# exit indicating strings do not match

 # the following exit path is taken if both strings have characters
 # remaining and a character pair that did not match was detected.

L280:	cmpb	-(r1),-(r3)		# reset r1 and r3 and set condition codes
no_match:				# restore r4 and r10
 #	popr	$^m<r4,r10>		#  without changing condition codes
	 popr	$0x410
	rsb				# exit indicating strings do not match

 #+
 # functional description:
 #
 #	the bytes of the string specified by the length and address operands are
 #	successively  used  to  index  into  a 256 byte table whose zeroth entry
 #	address is specified by the table address operand.   the  byte  selected
 #	from  the table is anded with the mask operand.  the operation continues
 #	until the result of the and is non-zero or all the bytes of  the  string
 #	have  been  exhausted.   if  a  non-zero  and  result  is  detected, the
 #	condition code z-bit is cleared#  otherwise, the z-bit is set.
 #
 # input parameters:
 #
 #	r0<15:0> = len		length of character string
 #	r1       = addr		address of character string
 #	r2<7:0>  = mask		mask that is anded with successive characters
 #	r3       = tbladdr	address of 256-byte table
 #
 # intermediate state:
 #
 #     31               23               15               07            00
 #    +----------------+----------------+----------------+----------------+
 #    |    delta-pc    |      xxxx      |               len               | : r0
 #    +----------------+----------------+----------------+----------------+
 #    |                               addr                                | : r1
 #    +----------------+----------------+----------------+----------------+
 #    |              xxxxx              |      xxxx      |      mask      | : r2
 #    +----------------+----------------+----------------+----------------+
 #    |                              tbladdr                              | : r3
 #    +----------------+----------------+----------------+----------------+
 #
 # output parameters:
 #
 #	nonzero and result
 #
 #		r0 = number of bytes remaining in the string (including the byte
 #			that produced the nonzero and result)
 #		r1 = address of the byte that produced the nonzero and result
 #		r2 = 0
 #		r3 = tbladdr	address of 256-byte table
 #
 #	and result always zero (string exhausted)
 #
 #		r0 = 0
 #		r1 = address of one byte beyond end of string
 #		r2 = 0
 #		r3 = tbladdr	address of 256-byte table
 #
 # condition codes:
 #
 #	n <- 0
 #	z <- r0 eql 0
 #	v <- 0
 #	c <- 0
 #
 #	the z bit is clear if there was a nonzero and result.
 #	the z bit is set if the input string is exhausted.
 #
 # side effects:
 #
 #	this routine uses two longwords of stack.
 #-

	.globl vax$scanc

vax$scanc:
	movzwl	r0,r0			# zero length string?
	beql	3f			# simply return if yes
	pushl	r10			# save r10 so it can hold handler 
 #	establish_handler	string_accvio
	 movab	string_accvio,r10
	pushl	r4			# we need a scratch register

1:	movzbl	(r1)+,r4		# get next character in string
	bitb	r2,(r3)[r4]		# index into table and and with mask
	bneq	4f			# exit loop if nonzero
	sobgtr	r0,1b

 # if we drop through the end of the loop into the following code, then
 # the input string was exhausted with no nonzero result.

2:
 #	popr	$^m<r4,r10>		# restore saved registers
	 popr	$0x410
3:	clrl	r2			# set r2 for output value of 0
	tstl	r0			# set condition codes 
	rsb				# return

 # exit path from loop if and produced nonzero result

4:	decl	r1			# point r1 to located character
	brb	2b			# merge with common exit 


 #+
 # functional description:
 #
 #	the bytes of the string specified by the length and address operands are
 #	successively  used  to  index  into  a 256 byte table whose zeroth entry
 #	address is specified by the table address operand.   the  byte  selected
 #	from  the table is anded with the mask operand.  the operation continues
 #	until the result of the and is zero or all the bytes of the string  have
 #	been  exhausted.   if  a zero and result is detected, the condition code
 #	z-bit is cleared#  otherwise, the z-bit is set.
 #
 # input parameters:
 #
 #	r0<15:0> = len		length of character string
 #	r1       = addr		address of character string
 #	r2<7:0>  = mask		mask that is anded with successive characters
 #	r3       = tbladdr	address of 256-byte table
 #
 # intermediate state:
 #
 #     31               23               15               07            00
 #    +----------------+----------------+----------------+----------------+
 #    |    delta-pc    |      xxxx      |               len               | : r0
 #    +----------------+----------------+----------------+----------------+
 #    |                               addr                                | : r1
 #    +----------------+----------------+----------------+----------------+
 #    |              xxxxx              |      xxxx      |      mask      | : r2
 #    +----------------+----------------+----------------+----------------+
 #    |                              tbladdr                              | : r3
 #    +----------------+----------------+----------------+----------------+
 #
 # output parameters:
 #
 #	zero and result
 #
 #		r0 = number of bytes remaining in the string (including the byte
 #			that produced the zero and result)
 #		r1 = address of the byte that produced the zero and result
 #		r2 = 0
 #		r3 = tbladdr	address of 256-byte table
 #
 #	and result always nonzero (string exhausted)
 #
 #		r0 = 0
 #		r1 = address of one byte beyond end of string
 #		r2 = 0
 #		r3 = tbladdr	address of 256-byte table
 #
 # condition codes:
 #
 #	n <- 0
 #	z <- r0 eql 0
 #	v <- 0
 #	c <- 0
 #
 #	the z bit is clear if there was a zero and result.
 #	the z bit is set if the input string is exhausted.
 #
 # side effects:
 #
 #	this routine uses two longwords of stack.
 #-

	.globl	vax$spanc

vax$spanc:
	movzwl	r0,r0			# clear unused bits & check for 0 length
	beql	3f			# simply return if length is zero
	pushl	r10			# save r10 so it can hold handler 
 #	establish_handler	string_accvio
	 movab	string_accvio,r10
	pushl	r4			# we need a scratch register

 #	MARK_POINT	SCANC_1
	.text	2
	.set	table_size, table_size + 1
	.word	Lscanc_1 - module_base
	.text	3
	.word	scanc_1 - module_base
	.text
Lscanc_1:
1:	movzbl	(r1)+,r4		# get next character in string
 #	MARK_POINT	SCANC_2
	.text	2
	.set	table_size, table_size + 1
	.word	Lscanc_2 - module_base
	.text	3
	.word	scanc_2 - module_base
	.text
Lscanc_2:
	bitb	r2,(r3)[r4]		# index into table and and with mask
	beql	4f			# exit loop if nonzero
	sobgtr	r0,1b

 # if we drop through the end of the loop into the following code, then
 # the input string was exhausted with no zero result.

2:
 #	popr	$^m<r4,r10>		# restore saved registers
	 popr	$0x410
3:	clrl	r2			# set r2 for output value of 0
	tstl	r0			# set condition codes 
	rsb				# return

 # exit path from loop if and produced zero result

4:	decl	r1			# point r1 to located character
	brb	2b			# merge with common exit 


 #+
 # functional description:
 #
 #	the character operand is compared with the bytes of the string specified
 #	by the length and address operands.  comparison continues until equality
 #	is detected or all bytes of the string have been compared.  if  equality
 #	is  detected#  the condition code z-bit is cleared#  otherwise the z-bit
 #	is set.
 #
 # input parameters:
 #
 #	r0<15:0>  = len		length of character string
 #	r0<23:16> = char	character to be located
 #	r1        = addr	address of character string
 #
 # intermediate state:
 #
 #     31               23               15               07            00
 #    +----------------+----------------+----------------+----------------+
 #    |    delta-pc    |      char      |               len               | : r0
 #    +----------------+----------------+----------------+----------------+
 #    |                               addr                                | : r1
 #    +----------------+----------------+----------------+----------------+
 #
 # output parameters:
 #
 #	character found
 #
 #		r0 = number of bytes remaining in the string (including located one)
 #		r1 = address of the located byte
 #
 #	character not found
 #
 #		r0 = 0
 #		r1 = address of one byte beyond end of string
 #
 # condition codes:
 #
 #	n <- 0
 #	z <- r0 eql 0
 #	v <- 0
 #	c <- 0
 #
 #	the z bit is clear if the character is located.
 #	the z bit is set if the character is not located.
 #
 # side effects:
 #
 #	this routine uses two longwords of stack.
 #-

	.globl	vax$locc
vax$locc:
	pushl	r10			# save r10 so it can hold handler 
 #	establish_handler	string_accvio
	 movab	string_accvio,r10
	pushl	r2			# save register
	ashl	$-16,r0,r2		# get character to be located
	movzwl	r0,r0			# clear unused bits & check for 0 length
	beql	2f			# simply return if length is 0

 #	MARK_POINT	LOCC_1
	.text	2
	.set	table_size, table_size + 1
	.word	Llocc_1 - module_base
	.text	3
	.word	locc_1 - module_base
	.text
Llocc_1:
1:	cmpb	r2,(r1)+		# character match?
	beql	3f			# exit loop if yes
	sobgtr	r0,1b

 # if we drop through the end of the loop into the following code, then
 # the input string was exhausted with the character not found.

2:
 #	popr	$^m<r2,r10>		# restore saved r2 and r10
	 popr	$0x404
	tstl	r0			# insure that c-bit is clear
	rsb				# return with z-bit set

 # exit path when character located

3:	decl	r1			# point r1 to located character
	brb	2b			# join common code



 #+
 # functional description:
 #
 #	the character operand is compared with the bytes of the string specified
 #	by   the  length  and  address  operands.   comparison  continues  until
 #	inequality is detected or all bytes of the string  have  been  compared.
 #	if  inequality  is  detected#   the  condition  code  z-bit  is cleared#
 #	otherwise the z-bit is set.
 #
 # input parameters:
 #
 #	r0<15:0>  = len		length of character string
 #	r0<23:16> = char	character to be skipped
 #	r1        = addr	address of character string
 #
 # intermediate state:
 #
 #     31               23               15               07            00
 #    +----------------+----------------+----------------+----------------+
 #    |    delta-pc    |      char      |               len               | : r0
 #    +----------------+----------------+----------------+----------------+
 #    |                               addr                                | : r1
 #    +----------------+----------------+----------------+----------------+
 #
 # output parameters:
 #
 #	different character found
 #
 #		r0 = number of bytes remaining in the string (including 
 #			unequal one)
 #		r1 = address of the unequal byte
 #
 #	all characters in string match "char"
 #
 #		r0 = 0
 #		r1 = address of one byte beyond end of string
 #
 # condition codes:
 #
 #	n <- 0
 #	z <- r0 eql 0
 #	v <- 0
 #	c <- 0
 #
 #	the z bit is clear if a character different from "char" is located.
 #	the z bit is set if the entire string is equal to "char".
 #
 # side effects:
 #
 #	this routine uses two longwords of stack.
 #-

	.globl	vax$skpc

vax$skpc:
	pushl	r10			# save r10 so it can hold handler 
 #	establish_handler	string_accvio
	 movab	string_accvio,r10
	pushl	r2			# save register
	ashl	$-16,r0,r2		# get character to be skipped
	movzwl	r0,r0			# clear unused bits & check for 0 length
	beql	2f			# simply return if yes

 #	mark_point	skpc_1
	.text	2
	.set	table_size, table_size + 1
	.word	Lskpc_1 - module_base
	.text	3
	.word	skpc_1 - module_base
	.text
Lskpc_1:
1:	cmpb	r2,(r1)+		# character match?
	bneq	3f			# exit loop if no
	sobgtr	r0,1b

 # if we drop through the end of the loop into the following code, then
 # the input string was exhausted with all of string equal to "char".

2:
 #	popr	$^m<r2,r10>		# restore saved r2 and r10
	 popr	$0x404
	tstl	r0			# insure that c-bit is clear
	rsb				# return with z-bit set

 # exit path when nonmatching character located

3:	decl	r1			# point r1 to located character
	brb	2b			# join common code

 #+
 # functional description:
 #
 #	the source string specified by the  source  length  and  source  address
 #	operands  is  searched  for  a substring which matches the object string
 #	specified by the object length and  object  address  operands.   if  the
 #	substring  is  found, the condition code z-bit is set#  otherwise, it is
 #	cleared.
 #
 # input parameters:
 #
 #	r0<15:0> = objlen	length of object string
 #	r1       = objaddr	address of object string 
 #	r2<15:0> = srclen	length of source string
 #	r3       = srcaddr	address of source string 
 #
 # intermediate state:
 #
 #     31               23               15               07            00
 #    +----------------+----------------+----------------+----------------+
 #    |    delta-pc    |      xxxx      |             objlen              | : r0
 #    +----------------+----------------+----------------+----------------+
 #    |                              objaddr                              | : r1
 #    +----------------+----------------+----------------+----------------+
 #    |              xxxxx              |             srclen              | : r2
 #    +----------------+----------------+----------------+----------------+
 #    |                              srcaddr                              | : r3
 #    +----------------+----------------+----------------+----------------+
 #
 # output parameters:
 #
 #	match occurred
 #
 #		r0 = 0
 #		r1 = address of one byte beyond end of object string
 #		r2 = number of bytes remaining in the source string
 #		r3 = address of one byte beyond last byte matched
 #
 #	strings do not match
 #
 #		r0 = objlen	length of object string
 #		r1 = objaddr	address of object string 
 #		r2 = 0
 #		r3 = address of one byte beyond end of source string
 #
 # condition codes:
 #
 #	n <- 0
 #	z <- r0 eql 0
 #	v <- 0
 #	c <- 0
 #
 #	the z bit is clear if the object does not match the source
 #	the z bit is set if a match occurred
 #
 # side effects:
 #
 #	this routine uses five longwords of stack for saved registers.
 #-

	.globl	vax$matchc

vax$matchc:
	movzwl	r0,r0			# clear unused bits & check for 0 length
	beql	L340			# simply return if length is 0
	movzwl	r2,r2			# clear unused bits & check for 0 length
	beql	L330			# return with condition codes set
					#  based on r0 gtru 0
	pushl	r10			# save r10 so it can hold handler 
 #	establish_handler	string_accvio
	 movab	string_accvio,r10

 # the next set of instructions saves r4..r7 and copy r0..r3 to r4..r7

	pushl	r7			#
	pushl	r6			#
	pushl	r5			#
	pushl	r4			#
	movq	r0,r4			#
	movq	r2,r6			#

	brb	top_of_loop		# skip reset code on first pass

 # the following code resets the object string parameters (r0,r1) and
 # points the source string parameters (r2,r3) to the next byte. (note 
 # that there is no explicit test for r6 going to zero. that test is
 # implicit in the cmpl r0,r2 at top_of_loop.)

 # in fact, this piece of code is really two nested loops. the object string
 # is traversed for each substring in the source string. if no match occurs,
 # then the source string is advanced by one character and the inner loop is
 # traversed again.

reset_strings:
	decl	r6			# one less byte in source string
	incl	r7			# ... at address one byte larger
	movq	r4,r0			# reset object string descriptor
	movq	r6,r2			# load new source string descriptor

top_of_loop:
	cmpl	r0,r2			# compare sizes of source and object
	bgtru	L350			# object larger than source => no match 
 #	MARK_POINT	MATCHC_1
	.text	2
	.set	table_size, table_size + 1
	.word	Lmatchc_1 - module_base
	.text	3
	.word	matchc_1 - module_base
	.text
Lmatchc_1:
L310:	cmpb	(r1)+,(r3)+		# does next character match?
	bneq	reset_strings		# exit inner loop if no match
	sobgtr	r0,L310			# object exhausted?

 # if we drop through the loop, then a match occurred. set the correct
 # output parameters and exit. note that r0 is equal to zero, which
 # will cause the condition codes (namely the z-bit) to indicate a match.

	subl2	r4,r2			# subtract objlen from srclen

L320:
 #	popr	$^m<r4,r5,r6,r7,r10>	# restore scratch registers and r10
	 popr	$0x04f0

L330:	tstl	r0			# set condition codes
	rsb				# return

 # this code executes if the object string is zero length.  the upper
 # 16 bits have to be cleared in r2 and then the condition codes are set
 # to indicate that a match occurred.

L340:	movzwl	r2,r2			# clear unused bits
	brb	L330			# join common code

 # this code executes if the strings do not match. the actual code state
 # that brings us here is that the object string is now larger than the
 # remaining piece of the source string, making a match impossible.

L350:	clrl	r2			# r2 contains zero in no match case
	addl3	r6,r7,r3		# point r3 to end of source string
	brb	L320			# join common exit code


 #+
 # functional description:
 #
 #	the crc of the  data  stream  described  by  the  string  descriptor  is
 #	calculated.   the initial crc is given by inicrc and is normally 0 or -1
 #	unless the crc is calculated in several steps.  the result  is  left  in
 #	r0.   if  the  polynomial  is  less  than  order-32,  the result must be
 #	extracted from the result.  the  crc  polynomial  is  expressed  by  the
 #	contents of the 16-longword table.  
 #
 # input parameters:
 #
 #	r0       = inicrc	initial crc
 #	r1       = tbl		address of 16-longword table
 #	r2<15:0> = strlen	length of data stream
 #	r3       = stream	address of data stream
 #
 # intermediate state:
 #
 #     31               23               15               07            00
 #    +----------------+----------------+----------------+----------------+
 #    |                              inicrc                               | : r0
 #    +----------------+----------------+----------------+----------------+
 #    |                                tbl                                | : r1
 #    +----------------+----------------+----------------+----------------+
 #    |    delta-pc    |      xxxx      |             strlen              | : r2
 #    +----------------+----------------+----------------+----------------+
 #    |                              stream                               | : r3
 #    +----------------+----------------+----------------+----------------+
 #
 # output parameters:
 #
 #	r0 = final crc value
 #	r1 = 0
 #	r2 = 0
 #	r3 = address of one byte beyond end of data stream
 #
 # condition codes:
 #
 #	n <- r0 lss 0
 #	z <- r0 eql 0
 #	v <- 0
 #	c <- 0
 #
 #	the condition codes simply reflect the final crc value.
 #
 # side effects:
 #
 #	this routine uses three longwords of stack.
 #
 # notes:
 #
 #	note that the main loop of this routine is slightly complicated
 #	by the need to allow the routine to be interrupted and restarted
 #	from its entry point. this requirement prevents r0 from being
 #	partially updates several times during each trip through the loop.
 #	instead, r5 is used to record the partial modifications and r5 is
 #	copied into r0 at the last step (with the extra movl r5,r0).
 #-

	.globl	vax$crc

vax$crc:
	movzwl	r2,r2			# clear unused bits & check for 0 length
	beql	2f			# all done if zero
	pushl	r10			# save r10 so it can hold handler 
 #	establish_handler	string_accvio
	 movab	string_accvio,r10
	pushl	r5			# save contents of scratch register
	movl	r0,r5			# copy inicrc to r5
	pushl	r4			# save contents of scratch register
	clrl	r4			# clear it out (we only use r4<7:0>)

 # this is the main loop that operates on each byte in the input stream

 #	MARK_POINT	CRC_1
	.text	2
	.set	table_size, table_size + 1
	.word	Lcrc_1 - module_base
	.text	3
	.word	crc_1 - module_base
	.text
Lcrc_1:
1:	xorb2	(r3)+,r5		# include next byte

 # the next three instructions are really the body of a loop that executes
 # twice on each pass through the outer loop. rather than incur additional
 # overhead, this inner loop is expanded in line.

	bicb3	$0x0f0,r5,r4		# get right 4 bits
	extzv	$4,$28,r5,r5		# shift result right 4
 #	MARK_POINT	CRC_2
	.text	2
	.set	table_size, table_size + 1
	.word	Lcrc_2 - module_base
	.text	3
	.word	crc_2 - module_base
	.text
Lcrc_2:
	xorl2	(r1)[r4],r5		# include table entry

	bicb3	$0x0f0,r5,r4		# get right 4 bits
	extzv	$4,$28,r5,r5		# shift result right 4
 #	MARK_POINT	CRC_3
	.text	2
	.set	table_size, table_size + 1
	.word	Lcrc_3 - module_base
	.text	3
	.word	crc_3 - module_base
	.text
Lcrc_3:

	xorl2	(r1)[r4],r5		# include table entry

	movl	r5,r0			# preserve latest complete result

	sobgtr	r2,1b			# count down loop

 #	popr	$^m<r4,r5,r10>		# restore saved r4, r5, and r10
	 popr	$0x430	

2:	clrl	r1			# r1 must be zero on exit
	tstl	r0			# determine n- and z-bits 
					# (note that tstl clears v- and c-bits)
	rsb				# return to caller


 #+
 # functional description:
 #
 #	this routine receives control when an access violation occurs while
 #	executing within the emulator. this routine determines whether the
 #	exception occurred while accessing a source or destination string.
 #	(this check is made based on the pc of the exception.) 
 #
 #	if the pc is one that is recognized by this routine, then the state of
 #	the instruction (character counts, string addresses, and the like) are
 #	restored to a state where the instruction/routine can be restarted
 #	after the cause for the exception is eliminated. control is then
 #	passed to a common routine that sets up the stack and the exception
 #	parameters in such a way that the instruction or routine can restart
 #	transparently. 
 #
 #	if the exception occurs at some unrecognized pc, then the exception is
 #	reflected to the user as an exception that occurred within the
 #	emulator. 
 #
 #	there are two exceptions that can occur that are not backed up to a 
 #	consistent state. 
 #
 #	    1.	if stack overflow occurs due to use of the stack by one of 
 #		the vax$xxxxxx routines, it is unlikely that this routine 
 #		will even execute because the code that transfers control 
 #		here must first copy the parameters to the exception stack 
 #		and that operation would fail. (the failure causes control 
 #		to be transferred to vms, where the stack expansion logic is 
 #		invoked and the routine resumed transparently.)
 #
 #	    2.	if assumptions about the address space change out from under 
 #		these routines (because an ast deleted a portion of the 
 #		address space or a similar silly thing), the handling of the 
 #		exception is unpredictable.
 #
 # input parameters:
 #
 #	r0  - value of sp when the exception occurred
 #	r1  - pc of exception
 #	r2  - scratch
 #	r3  - scratch
 #	r10 - address of this routine (but that was already used so r10
 #	      can be used for a scratch register if needed)
 #
 #	00(sp) - saved r0 (contents of r0 when exception occurred)
 #	04(sp) - saved r1 (contents of r1 when exception occurred)
 #	08(sp) - saved r2 (contents of r2 when exception occurred)
 #	12(sp) - saved r3 (contents of r3 when exception occurred)
 #
 #	16(sp) - return pc in exception dispatcher in operating system
 #
 #	20(sp) - first longword of system-specific exception data
 #	xx(sp) - first longword of system-specific exception data
 #
 #	the address of the next longword is the position of the stack when
 #	the exception occurred. this address is contained in r0 on entry
 #	to this routine.
 #
 # r0 ->	<4*<n+1> + 16>(sp) - instruction-specific data
 #	  .                - optional instruction-specific data
 #	  .                - saved r10
 #	<4*<n+m> + 16>(sp) - return pc from vax$xxxxxx routine (m is the number
 #		             of instruction-specific longwords, including the   
 #		             saved r10. m is guaranteed greater than zero.)
 #
 # implicit input:
 #
 #	it is assumed that the contents of all registers (except r0 to r3)
 #	coming into this routine are unchanged from their contents when the
 #	exception occurred. (for r0 through r3, this assumption applies to the
 #	saved register contents on the top of the stack. any modification to
 #	these registers must be made to their saved copies and not to the
 #	registers themselves.) 
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
 #	uniform point from which it can be restarted.
 #
 #		r0  - value of sp when exception occurred
 #		r1  - scratch
 #		r2  - scratch
 #		r3  - scratch
 #		r10 - scratch
 #
 #	 r0 ->	zz(sp) - instruction-specific data begins here
 #
 #	the instruction-specific routines eventually pass control back to the
 #	host system with the following register contents.
 #
 #		r0  - address of return pc from vax$xxxxxx routine
 #		r1  - byte offset from top of stack (into saved r0 through r3) 
 #		      to indicate where to store the delta-pc (if so required)
 #		r10 - restored to its value on entry to vax$xxxxxx
 #
 #	if the exception pc occurred somewhere else (such as a stack access), 
 #	the saved registers are restored and control is passed back to the 
 #	host system with an rsb instruction.
 #
 # implicit output:
 #
 #	the register contents are modified to put the intermediate state of
 #	the instruction into a consistent state from which it can be
 #	continued. any changes to r0 through r3 are made in their saved state
 #	on the top of the stack. any scratch registers saved by each
 #	vax$xxxxxx routine are restored. 
 #-

string_accvio:
	clrl	r2			# initialize the counter
	pushab	module_base		# store base address of this module
	subl2	(sp)+,r1		# get pc relative to this base

1:	cmpw	r1,pc_table_base[r2]	# is this the right pc?
	beql	2f			# exit loop if true
	aoblss	$table_size,r2,1b	# do the entire table

 # if we drop through the dispatching based on pc, then the exception is not 
 # one that we want to back up. we simply reflect the exception to the user.

 #	popr	#^m<r0,r1,r2,r3>	# restore saved registers
	 popr	$0x0f
	rsb				# let vms reflect the exception	

 # the exception pc matched one of the entries in our pc table. r2 contains
 # the index into both the pc table and the handler table. r1 has served
 # its purpose and can be used as a scratch register.

2:	movzwl	handler_table_base[r2],r1	# get the offset to the handler
	jmp	module_base[r1]		# pass control to the handler





 #+
 # functional description:
 #
 #	these routines are used to store the intermediate state of the state
 #	of the string instructions (except movtc and movtuc) into the registers
 #	that are altered by a given instruction.
 #
 # input parameters:
 #
 #	r0 - points to top of stack when exception occurred
 #
 #	see each routine- and context-specific entry point for more details.
 #
 #	in general, register contents for counters and string pointers that
 #	are naturally tracking through a string are not listed. register
 #	contents that are out of the ordinary (different from those listed
 #	in the intermediate state pictures in each routine header) are listed.
 #
 # output parameters:
 #
 #	r0 - points to return pc from vax$xxxxxx
 #	r1 - locates specific byte in r0..r3 that will contain the delta-pc
 #
 #	all scratch registers (including r10) that are not supposed to be
 #	altered by the routine are restored to their contents when the 
 #	routine was originally entered.
 #
 # notes:
 #
 #	in all of the instruction-specific routines, the state of the stack
 #	will be shown as it was when the exception occurred. all offsets will
 #	be pictured relative to r0. in addition, relevant contents of r0
 #	through r3 will be listed as located in the registers themselves, even
 #	though the actual code will manipulate the saved values of these
 #	registers located on the top of the stack. 
 #
 #	the apparent arbitrary order of the instruction-specific routines is
 #	dictated by the amount of code that they can share. the most sharing
 #	occurs at the middle of the code, for instructions like cmpc5 and
 #	scanc. the crc routines, because they are the only routines that store
 #	the delta-pc in r2 appear first. the cmpc3 instruction has no
 #	instruction-specific code that cannot be shared with all of the other
 #	routines so it appears at the end. 
 #-


 #+
 # crc packing routine
 #
 #	r4 - scratch
 #	r5 - scratch
 #
 #	00(r0) - saved r4
 #	04(r0) - saved r5
 #	08(r0) - saved r10
 #	12(r0) - return pc
 #
 # if entry is at crc_2 or crc_3, the exception occurred after the string
 # pointer, r3, was advanced. that pointer must be backed up to achieve a
 # consistent state. 
 #-

crc_2:
crc_3:
	decl	pack_l_saved_r3(sp)	# back up string pointer
crc_1:
	movq	(r0)+,r4		# restore r4 and r5
	movzbl	$crc_b_delta_pc,r1	# indicate offset used to store delta-pc
	brb	L430			# not much common code left but use it

 #+
 # matchc packing routine
 #
 #	r4<15:0> - number of characters in object string
 #	r5       - address of object string
 #	r6<15:0> - number of characters remaining in source string
 #	r7       - updated pointer into source string
 #
 #	00(r0) - saved r4
 #	04(r0) - saved r5
 #	08(r0) - saved r6
 #	12(r0) - saved r7
 #	16(r0) - saved r10
 #	20(r0) - return pc
 #
 # note that the matchc instruction is backed up to the top of its inner loop. 
 # that is, when the instruction restarts, it will begin looking for a match
 # between the first character of the object string and the latest starting
 # character in the source string.
 #-

matchc_1:
	movq	r4,pack_l_saved_r0(sp)	# reset object string to its beginning
	movq	r6,pack_l_saved_r2(sp)	# reset to updated start of source string
	movq	(r0)+,r4		# restore r4 and r5
	movq	(r0)+,r6		# ... and r6 and r7
	brb	L420			# exit through common code path

 #+
 # cmpc5 packing routine
 #
 #	r4<7:0> - fill character operand
 #
 #	00(r0) - saved r4
 #	04(r0) - saved r10
 #	08(r0) - return pc
 #-

cmpc5_1:
cmpc5_2:
cmpc5_3:
	movb	r4,cmpc5_b_fill(sp)	# pack "fill" into r0<23:16>
	brb	L410			# merge with code to restore r4

 #+
 # scanc and spanc  packing routine
 #
 #	r4 - scratch
 #
 #	00(r0) - saved r4
 #	04(r0) - saved r10
 #	08(r0) - return pc
 # 
 # if entry is at scanc_2 or spanc_2, the exception occurred after the string
 # pointer, r1, was advanced. that pointer must be backed up to achieve a
 # consistent state. 
 #-

scanc_2:
spanc_2:
	decl	pack_l_saved_r1(sp)	# back up string pointer
scanc_1:
spanc_1:
L410:	movl	(r0)+,r4		# restore r4
	brb	L420			# exit through common code path

 #+
 # locc and skpc packing routine
 #
 #	r2<7:0> - character operand
 #
 #	00(r0) - saved r2
 #	04(r0) - saved r10
 #	08(r0) - return pc
 #-

locc_1:
skpc_1:

 #	assume locc_b_char eq skpc_b_char 

	movb	pack_l_saved_r2(sp),locc_b_char(sp) # pack "char" into r0<23:16>
	movl	(r0)+,pack_l_saved_r2(sp) # restore saved r2

 #+
 # cmpc3 packing routine
 #
 #	00(r0) - saved r10
 #	04(r0) - return pc
 #-


cmpc3_1:
L420:	movzbl	$cmpc3_b_delta_pc,r1	# indicate that r0 gets delta pc
L430:	movl	(r0)+,r10		# restore saved r10
	bisw2	$(pack_m_fpd|pack_m_accvio),r1
	jmp	vax$reflect_fault

 #+
 # functional description:
 #
 #	these routines are used to store the intermediate state of the state
 #	of the movtc and movtuc instructions into the registers r0 through r5.
 #	the main reason for keeping these two routines separate from the rest
 #	of the string instructions is that r10 is not stored directly adjacent
 #	to the return pc. this means that there is no code that can be shared
 #	with the rest of the instructions.
 #
 # input parameters:
 #
 #	r0 - points to top of stack when exception occurred
 #
 #	see the context-specific entry point for more details.
 #
 # output parameters:
 #
 #	r0 - points to return pc from vax$xxxxxx
 #	r1 - locates specific byte in r0..r3 that will contain the delta-pc
 #
 #	all scratch registers (including r10) that are not supposed to be
 #	altered by the routine are restored to their contents when the 
 #	routine was originally entered.
 #
 # notes:
 #
 #	see the notes in the routine header for the storage routines for 
 #	the rest of the string instructions.
 #-

 #+
 # movtc packing routine (if moving in the forward direction)
 #
 # the entry points movtc_1, movtc_2, and movtc_3 are used when moving the
 # string in the forward direction. if the entry is at movtc_2, then the
 # source and destination strings are out of synch and r1 must be adjusted
 # (decremented) to keep the two strings in step.
 #
 # in the move_forward routine, there is a need for a scratch register before
 # the fill character is used. r2 is used as this scratch and its original
 # contents, the fill character, are saved on the stack. the entry points 
 # movtc_1 and movtc_2 have the stack in this state.
 #
 #	r2 - scratch
 #
 #	00(r0) - saved r2
 #	04(r0) - saved r10
 #	08(r0) - saved r0 
 #		 <31:16> - initial contents of r0
 #		 <15:00> - contents of r0 at time of lastest entry to vax$movtc
 #	12(r0) - saved r4
 #		 <31:16> - initial contents of r4
 #		 <15:00> - contents of r4 at time of lastest entry to vax$movtc
 #	16(r0) - return pc
 #
 # if entry is at movtc_3, then there are no registers other than r0 and r4 
 # (and of course r10) that are saved on the stack.
 #
 #	00(r0) - saved r10
 #	04(r0) - saved r0 
 #		 <31:16> - initial contents of r0
 #		 <15:00> - contents of r0 at time of lastest entry to vax$movtc
 #	08(r0) - saved r4
 #		 <31:16> - initial contents of r4
 #		 <15:00> - contents of r4 at time of lastest entry to vax$movtc
 #	12(r0) - return pc
 #
 # The following are register contents at the time that the exceptino occured.
 #
 #	r0 - number if bytes remaining to be modified in source string.
 #	r1 - address of current byte in source string (except at movtc_2)
 #	r2 - junk or fill character (if entry at movtc_3)
 #	r3 - address of translation table (unchanged during execution)
 #	r4 - signed difference between current lengths of source and destination
 #	r5 - address of current byte in destination string
 #	
 #	r10 - access violation handler address (so can be used as scratch)
 #
 # Note that if r4 lssu 0, then the value of r0 represents the number of bytes
 # in the source string remaining to be modified.  There are also excess bytes
 # of the source string that will be untouched by the complete execution of
 # this instruction. (In fact, at completion, r0 will contain the number of
 # unmodified bytes.)
 #
 # Note further that entry at movtc_3 is impossible with r4 lssu 0 because
 # movtc_3 indicates that an access violation occured while sotring the 
 # fill character in the destination and that can only happen when the
 # output string is longer than the input string.
 #
 # The state that must be modified before stored depends on the sign of
 # r4, which in turn depends on which of source and destination is longer
 #
 #	r4 gequ 0 => srclen lequ dstlen
 #
 #		r0 - unchanged
 #		r4 - increased by r0 (r4 <- r4 + r0)
 #
 #	r4 lssu 0 => srclen gtru dstlen
 #
 #		r0 - increased by negative of r4 (r0 <- r0 + abs(r4))
 #		r4 - replaced with input value of r0 (r4 <- r0)
 #-


movtc_2:
	decl	pack_l_saved_r1(sp)	# back up source string 

movtc_1:
	movl	(r0)+,pack_l_saved_r2(sp) # restore contents of saved r2
	tstl	r4			# 001 r4 lssu 0 => srclen gtru dstlen
	bgeq	L505			# 001 branch if srclen lequ dstlen
	mnegl	r4,r10			# 001 save absolute value of diff
	movl	pack_l_saved_r0(sp),r4	# 001 get updated dsrlen (r4 <- r0)
	addl2	r10,pack_l_saved_r0(sp)	# 001 ... and updated srclen (r0<-r0-r4)
	brb	L510

L505:	addl2	pack_l_saved_r0(sp),r4	# reset correct count of destination

movtc_3:
L510:	movl	(r0)+,r10		# restore saved r10


	movw	2(r0),movtc_w_inisrclen(sp) # save high-order word of r0
	addl2	$4,r0			# point r0 to saved r4
	movw	r4,(r0)			# store low order r4 in saved r4
	movl	(r0)+,r4		# restore all of r4


 # indicate that r2<31:24> gets delta-pc and cause the fpd bit to be set

	movl	$(movtc_b_delta_pc|psl$m_fpd|pack_m_accvio),r1	


	bisb2	$movtc_m_fpd,movtc_b_flags(sp) # set internal fpd bit
	jmp	vax$reflect_fault	# reflect exception to user

 #+
 # movtc packing routine (if moving in the backward direction)
 #
 # the entry points movtc_4, movtc_5, and movtc_6 are used when moving the
 # string in the backward direction. if the entry is at movtc_6, then the
 # source and destination strings are out of synch and r1 must be adjusted
 # (incremented) to keep the two strings in step.
 #
 # at entry points movtc_5 and movtc_6, we must reset the source string
 # pointer, r1, to the beginning of the string because it is currently
 # set up to traverse the string from its high-address end. The details
 # of this reset operation depend on the relative lengths of the source
 # and destination strings as described below.
 #
 # at all three entry points, we must reset the destination string
 # pointer, r5, to the beginning of the string because it is currently
 # set up to traverse the string from its high-address end.
 #
 #	00(r0) - saved r10
 #	04(r0) - saved r0 
 #		 <31:16> - initial contents of r0
 #		 <15:00> - contents of r0 at time of lastest entry to vax$movtc
 #	08(r0) - saved r4
 #		 <31:16> - initial contents of r4
 #		 <15:00> - contents of r4 at time of lastest entry to vax$movtc
 #	12(r0) - return pc
 #
 # The following are register contents at the time the exception occurred.
 #
 #	r0 - number of bytes remaining to be modified in source string
 #	r1 - address of current byte in source string (except at movtc_6)
 #	r2 - scratch
 #	r3 - address of translation table (unchanged during execution)
 #	r4 - signed difference between current lengths of source and 
 #	     destination
 #	r5 - Address of current byte in destination string
 #
 #	r10 - access violation handler address (so can be used as scratch)
 #
 # Note that is r4 lssu 0, then that vaule of r0 represent the number of bytes
 # in the source string remaining to be modified.  There are also excess bytes
 # of the source string that will be untouuched by the complete execution of
 # this instruction. (In fact, at completion, r0 will contain the number of
 # unmodified bytes.)
 #
 # Note further that entry at movtc_4 is impossible with r4 lssu 0 because
 # movtc_4 indicates that and access violation occured while storing the
 # fill character in the destination and that can only happen when the output
 # string is longer than the input string.
 #
 # The state that must be modified before being stored depends on the sign
 # of r4, which in turn depends on which of source and destination is longer.
 #
 #	r4 gequ 0 => srclen lequ dstlen
 #
 #		r0 - unchanged
 #		r1 - backed up by r0 (r1 <- r1 - r0)
 #		r4 - increased by r0 (r4 <- r4 + r0)
 #		r5 - backed up by new value of r4 (r5 <- r5 - r4)
 #
 #	r4 lssu 0 => srclen gtru dstlen
 #	
 #		r0 - increased by negative of r4 (r0 <- r0 + abs(r4))
 #		r1 - backed up by input value of r0 (r1 <- r1 - r0 )
 #		r4 - replaced with input value of r0 (r4 <- r0)
 #		r5 - backed up by new value of r4 (r5 <- r5 - r4)
 #
 #-

movtc_6:
	incl	pack_l_saved_r1(sp)	# undo last fetch from source string

movtc_5:
	subl2	pack_l_saved_r0(sp),pack_l_saved_r1(sp)
					# point r1 to start of source string
	tstl	r4			# r4 lssu 0 => srclen lequ dstlen
	bgeq	2f			# branch iif srclen lequ dstlen
	mnegl	r4,r10			# save absolute value difference
	movl	pack_l_saved_r0(sp),r4	# get updated dstlen	(r4 <- r0)
	addl2	r10,pack_l_saved_r0(sp)	# ... and updated srclen (r0 <- r0 -r4)
	brb	3f

movtc_4:
2:	addl2	pack_l_saved_r0(sp),r4	# treat two strings as having same len
3:	subl2	r4,r5			# point r5 to start of destination string
	brb	L510			# join common code

 #+
 # movtuc packing routine 
 #
 # note that r7 is used to count the number of remaining characters in the
 # strings. the other two counts, r0 and r4, are set to contain their final
 # values.
 #
 # if r0 was initially smaller than r4,
 #
 #	r0 - 0
 #	r4 - difference between r4 and r0 (r4-r0)
 #	r7 - number of characters remaining in source (shorter) string
 #
 # if r0 was initially larger than r4,
 #
 #	r0 - difference between r0 and r4 (r0-r4)
 #	r4 - 0
 #	r7 - number of characters remaining in destination (shorter) string
 #
 # in either case, the stack when the exception occurred looks like this.
 #
 #	r6 - scratch
 #	r7 - number of characters remaining in two strings
 #
 #	00(r0) - saved r6
 #	04(r0) - saved r7 
 #	08(r0) - saved r10
 #	12(r0) - saved r0 
 #		 <31:16> - initial contents of r0
 #		 <15:00> - contents of r0 at time of lastest entry to vax$movtuc
 #	16(r0) - saved r4
 #		 <31:16> - initial contents of r4
 #		 <15:00> - contents of r4 at time of lastest entry to vax$movtuc
 #	20(r0) - return pc
 #
 # if the entry is at movtuc_2 or movtuc_3, then the source and
 # destination strings are out of synch and r1 must be adjusted
 # (decremented) to keep the two strings in step. 
 #-

movtuc_2:
movtuc_3:
	decl	pack_l_saved_r1(sp)	# back up source string pointer

movtuc_1:
	addl2	r7,pack_l_saved_r0(sp)	# readjust source string count
	addl2	r7,r4			# ... and destination string count
	movq	(r0)+,r6		# restore saved r6 and r7
	brb	L510			# join exit path shared with movtc

 #	end_mark_point
module_end:
