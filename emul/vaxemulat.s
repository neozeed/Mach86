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
/*	@(#)vaxemulat.s	1.3		11/2/84		*/

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
 #	this is the main body of the instruction emulator that supports
 #	the instructions that are not a part of the microvax architecture.
 #	the current design calls for support of the string instructions
 #	(including crc), the decimal instructions, and editpc.
 #
 #	this routine performs the following steps.
 #
 #	o moves operands from the exception stack to registers in an
 #	  instruction-specific manner
 #
 #	o calls an instruction-specific subroutine to do the actual work
 #
 #	if errors occur along the way, those errors are reflected to the
 #	user as exceptions.
 #
 # environment: 
 #
 #	these routines run at any access mode, at any ipl, and are ast 
 #	reentrant. the routine starts execution in the access mode and
 #	at the ipl at which the instruction executed.
 #
 # author: 
 #
 #	lawrence j. kenah	
 #
 # creation date
 #
 #	17 august 1982
 #
 # modified by:
 #
 #	v01-011 ljk0041		Lawrence j. Kehah	16-jul-1984
 #		Clear FPD in saved psl at vax$emulate_fpd entry so that
 #		next instruction can execute correctly
 #
 #	v01-010 ljk0031		Lawrence j. Kenah	 5-jul-1984
 #		Set r2 and r4 unconditionally to zero in editpc routine
 #		to allow the storage of fpd flags and simliar date.
 #
 #	v01-009 ljk0026		Lawrence j. kenah	19-mar-1984
 #		Perform final cleanup pass. eliminate xxx_unpack routine
 #		references. add c-bit optimization to movp.
 #
 #	v01-008	ljk0010		lawrence j. kenah	8-nov-1983
 #		eliminate code in exit_emulator path that unconditionally
 #		clears the t-bit and conditionally sets the tp-bit. the
 #		tp-bit is handled by the base hardware.
 #
 #	v01-007	kdm0088		kathleen d. morse	20-oct-1983
 #		make branches to vax$reflect_to_vms into jumps, so that
 #		the bootstrap emulator will link without truncation errors
 #		until that routine is finished.
 #
 #	v01-006	kdm0003		kathleen d. morse	18-apr-1983
 #		generate abbreviated vax$emulate_fpd for the bootstrap
 #		emulator.
 #
 #	v01-005	ljk0006		lawrence j. kenah	16-mar-1983
 #		generate case tables with macros. allow subset emulator
 #		for bootstrap instruction emulation.
 #
 #	v01-004	kdm0002		kathleen d. morse	16-mar-1983
 #		fix fourth and fifth operand fetches for subp6, addp6,
 #		mulp and divp.
 #
 #	v01-003	kdm0001		kathleen d. morse	04-mar-1983
 #		longword align the exception handler entry points.
 #
 #	v01-002	ljk0005		lawrence j. kenah	15-nov-1982
 #		use hardware aids provided by microvax architecture revision.
 #		exception is now reported in caller's mode. operands are parsed
 #		and placed on the exception stack as exception parameters.
 #
 #	v01-001	ljk0002		lawrence j. kenah	17-aug-1982
 #		original version using kernel mode exception through opcdec
 #		exception vector.
 #--

 # include files:

/*	$opdef				# values for instruction opcodes
 *	$psldef				# define bit fields in psl

 *	pack_def			# stack usage when restarting instructions
 *	stack_def			# stack usage for original exception
 */

 # macro definitions

/*	.macro	init_case_table		size,base,error_exit
 *base:
 *	.rept	size
 *	.word	error_exit-base
 *	.endr
 *	.endm	init_case_table

 *	.macro	case_table_entry	opcode,-
 *					routine,-
 *					fpd_routine,-
 *					boot_flag
 *		sign_extend	op$_'opcode , ...opcode
 *		...offset = ...opcode - opcode_base
 *		.if	not_defined	boot_switch
 *			include_'opcode = 0
 *			.external	vax$'opcode
 *			.external	fpd_routine
 *			. = case_table_base + <2 * ...offset>
 *			.word	routine - case_table_base
 *			. = fpd_case_table_base + <2 * ...offset>
 *			.word	fpd_routine - fpd_case_table_base
 *		.if_false
 *			.if	identical	<boot_flag>,boot
 *			include_'opcode = 0
 *			.external	vax$'opcode
 *			. = case_table_base + <2 * ...offset>
 *			.word	routine - case_table_base
 *			.endc
 *		.endc
 *		.endm	case_table_entry
 */

 # global and external declarations


 # external declarations for exception handling

/*	.if	not_defined	boot_switch
 *	.external	vax$al_delta_pc_table
 *	.external	vax$reflect_to_vms
 *	.endc

 *	.external	vax$_opcdec, -
 *			vax$_opcdec_fpd

 # psect declarations:

 #+
 # functional description:
 #
 #	there are two different entries into this module. when a reserved
 #	instruction is first encountered, its operands are parsed by the
 #	hardware (or microcode, if you will) and placed on the stack as
 #	exception parameters. the code at address vax$emulate is then entered
 #	through the ^xc8(scb) exception vector. that routine dispatches to an
 #	instruction-specific routine called vax$xxxxxx (xxxxxx represents the
 #	name of the reserved instruction) after placing the operands into
 #	registers as required by vax$xxxxxx. 
 #
 #	if an exception occurred during instruction emulation such that a
 #	reserved instruction executed again, this time with fpd set, then a
 #	different exception path is taken. the stack has a different (smaller)
 #	set of parameters for the fpd exception. a different
 #	instruction-specific routine executes to unpack saved intermediate
 #	state before resuming instruction emulation. 
 #
 #	the access mode and ipl are preserved across either exception. 
 #
 # input parameters:
 #
 #	00(sp) - opcode of reserved instruction
 #	04(sp) - pc of reserved instruction (old pc)
 #	08(sp) - first operand specifier
 #	12(sp) - second operand specifier
 #	16(sp) - third operand specifier
 #	20(sp) - fourth operand specifier
 #	24(sp) - fifth operand specifier
 #	28(sp) - sixth operand specifier
 #	32(sp) - seventh operand specifier (currently unused)
 #	36(sp) - eight operand specifier (currently unused)
 #	40(sp) - pc of instruction following reserved instruction (new pc)
 #	44(sp) - psl at time of exception
 #
 # notes on input parameters:
 #
 #   1.	the information that appears on the stack for each operand depends
 #	on the nature of the operand.
 #
 #	.rx - operand value
 #	.ax - operand address
 #	.wx - operand address (register destination is stored in one's
 #	      complement form. see vax$cvtpl for details.)
 #
 #   2.	the old pc value is not used unless an exception such as an access
 #	violation occurs and the instruction has to be backed up.
 #
 #   3.	the seventh and eighth operands are not used for any existing vax-11
 #	instructions. those slots in the exception stach frame are reserved
 #	for future expansion. 
 #
 #   4.	the two pc parameters and the psl are the only data that needs to
 #	be preserved once the instruction-specific routine is entered.
 #
 # output parameters:
 #
 #	the operands are moved from the stack to general registers in a way
 #	that varies from instruction to instruction. control is transferred
 #	to a specific routine for each opcode.
 #
 # notes:
 #
 #	there are several tables in the emulator that use the opcode as an 
 #	index. we choose to interpret the opcode as a signed quantity because 
 #	this reduces the amount of wasted space in the tables. in either case, 
 #	there are 27 useful entries.
 #
 #	unsigned opcode
 #
 #		opcode_base = cvtps (value of 8)
 #		opcode_max = cvtlp (value of f9)
 #
 #		table_size = 241 decimal bytes
 #
 #	signed opcode
 #
 #		opcode_base = ashp (value of f8 or -8)
 #		opcode_max = skpc (value of 3b)
 #
 #		table_size = 67 decimal bytes
 #
 #	the savings of more than 170 entries in each table justifies all
 #	of the machinations that we go through to treat opcodes as signed
 #	quantities.
 #-

 # because the assembler does not understand sign extension of byte and
 # word quantities, we must accomplish this sign extension with macros. the
 # assignment statements that appear as comments illustrate the sense of the
 # macro invocations that immediately follow.

 #	opcode_max = op$_skpc		# largest opcode in this emulator

 #	sign_extend	op$_skpc , opcode_max

 # we further restrict the table size and supported operations when we are 
 # building the bootstrap subset of the emulator. we only allow certain string
 # instructions to contribute to the emulator.

/*	.if	defined		boot_switch

 * #	opcode_base = op$_cmpc3		# smallest (in signed sense) opcode

 *		sign_extend	op$_cmpc3 , opcode_base
 *	.if_false

 *#	opcode_base = op$_ashp		# smallest (in signed sense) opcode

 *		sign_extend	op$_ashp , opcode_base
 *	.endc
 */

 
 # case_table_size = <opcode_max - opcode_base> + 1	# define table size
# define case_table_size 0x44
# define opcode_base 0x0f8
 # define opcode_max-opcode_base 0x43

	.globl	vax$emulate

vax$emulate:

	caseb	opcode(sp),$opcode_base,$(0x43)

 #	init_case_table	case_table_size,case_table_base,10$
case_table_base:
	.word	Lashp-case_table_base		# ashp entry
 	.word	Lcvtlp-case_table_base		# cvtlp entry
	.word	1f-case_table_base
	.word	1f-case_table_base
	.word	1f-case_table_base
	.word	1f-case_table_base
	.word	1f-case_table_base
	.word	1f-case_table_base
	.word	1f-case_table_base
	.word	1f-case_table_base
	.word	1f-case_table_base
	.word	1f-case_table_base
	.word	1f-case_table_base
	.word	1f-case_table_base
	.word	1f-case_table_base
	.word	1f-case_table_base
	.word	Lcvtps-case_table_base		# cvtps entry
	.word	Lcvtsp-case_table_base		# cvtps entry
	.word	1f-case_table_base
	.word	Lcrc-case_table_base		# crc entry
	.word	1f-case_table_base
	.word	1f-case_table_base
	.word	1f-case_table_base
	.word	1f-case_table_base
	.word	1f-case_table_base
	.word	1f-case_table_base
	.word	1f-case_table_base
	.word	1f-case_table_base
	.word	1f-case_table_base
	.word	1f-case_table_base
	.word	1f-case_table_base
	.word	1f-case_table_base
	.word	1f-case_table_base
	.word	1f-case_table_base
	.word	1f-case_table_base
	.word	1f-case_table_base
	.word	1f-case_table_base
	.word	1f-case_table_base
	.word	1f-case_table_base
	.word	1f-case_table_base
	.word	Laddp4-case_table_base		# addp4 entry
	.word	Laddp6-case_table_base		# addp4 entry
	.word	Lsubp4-case_table_base		# subp4 entry
	.word	Lcvtpt-case_table_base		# cvtpt entry
	.word	Lsubp6-case_table_base		# subp6 entry
	.word	Lmulp-case_table_base		# mulp entry
	.word	Lcvttp-case_table_base		# cvttp entry
	.word	Ldivp-case_table_base		# divp entry
	.word	1f-case_table_base
	.word	Lcmpc3-case_table_base		# cmpc3 entry
	.word	Lscanc-case_table_base		# scanc entry
	.word	Lspanc-case_table_base		# spanc entry
	.word	1f-case_table_base
	.word	Lcmpc5-case_table_base		# cmpc5 entry
	.word	Lmovtc-case_table_base		# movtc entry
	.word	Lmovtuc-case_table_base		# movtuc entry
	.word	1f-case_table_base
	.word	1f-case_table_base
	.word	1f-case_table_base
	.word	1f-case_table_base
	.word	Lmovp-case_table_base		# movp entry
	.word	Lcmpp3-case_table_base		# cmpp3 entry
	.word	Lcvtpl-case_table_base		# cvtpl entry
	.word	Lcmpp4-case_table_base		# cmpp4 entry
	.word	Leditpc-case_table_base		# editpc entry
	.word	Lmatchc-case_table_base		# matchc entry
	.word	Llocc-case_table_base		# locc entry
	.word	Lskpc-case_table_base		# skpc entry



 # if we drop through the case dispatcher, then the fault was not caused
 # by executing one of the instructions supported by this emulator. such
 # exceptions will simply be passed through to vms. (in the bootstrap emulator,
 # there is no operating system to reflect the exception. we simply halt.)

1:	pushl	$vax$_opcdec		# store signal name
	pushl	$13			# total of 13 longwords in signal array

	jmp	vax$reflect_to_vms	# use common exit to vms

 #+
 # functional description:
 #
 #	this routine is entered through the ^xcc(scb) exception vector when an
 #	instruction that is not a part of the microvax architecture executes
 #	and the fpd bit is set in the psl. the software state that was
 #	preserved by each instruction must be restored and instruction
 #	execution resumed. access mode and ipl are preserved across the
 #	exception occurrence. 
 #
 #	before the various vax$xxxxxx (or vax$xxxxxx_restart) routines regain
 #	control, this dispatcher must retrieve the delta pc from wherever
 #	it was stored and place the stack in the same state that it is in
 #	when the normal (fpd bit not set) instruction dispatcher passes
 #	control to the various vax$xxxxxx routines. the pictures below explain
 #	this. 
 #
 # input parameters:
 #
 #	00(sp) - pc of reserved instruction 
 #	04(sp) - psl at time of exception
 #
 # output parameters:
 #
 #	the following picture shows the state of the stack after the dispatcher
 #	has executed its preliminary code but before control is passed back to
 #	instruction-specific  execution. note that this routine makes the 
 #	stack look like it does when a reserved instruction executes and fpd
 #	is not yet set. this is done to make the exception exit code independent
 #	of whether a different exception exception occurred while the emulator
 #	was running.
 #
 #	00(sp) - return pc (address of exit routine in this module)
 #	04(sp) - unused placeholder (opcode)
 #	08(sp) - pc of reserved instruction (old pc)
 #	12(sp) - unused placeholder (operand_1)
 #	16(sp) - unused placeholder (operand_2)
 #	20(sp) - unused placeholder (operand_3)
 #	24(sp) - unused placeholder (operand_4)
 #	28(sp) - unused placeholder (operand_5)
 #	32(sp) - unused placeholder (operand_6)
 #	36(sp) - unused placeholder (operand_7)
 #	40(sp) - unused placeholder (operand_8)
 #	44(sp) - pc of instruction following reserved instruction (new pc)
 #	48(sp) - psl at time of exception
 #
 #	before this routine dispatches to opcode-specific code, it calculates
 #	the pc of the next instruction based on the pc of the reserved
 #	instruction and the delta-pc quantity that was stored as part of the
 #	instruction's intermediate state. note that the delta pc quantity
 #
 #		delta pc = new pc - old pc
 #
 #	is stored in the upper bytes of one of the general registers, usually
 #	bits <31:24> of r0 or r2. the registers r0 through r3 are stored on
 #	the stack (in the space used for the first four operands when the
 #	reserved instruction is first encountered) so that the same offsets
 #	that were used to store the delta-pc can be used to retrieve it.
 #-

	.globl	vax$emulate_fpd

vax$emulate_fpd:


	bbcc	$psl$v_fpd,4(sp),5f	# clear fpd in exception psl
5:	subl2	$new_pc,sp		# create extra stack space
	movl	new_pc(sp),old_pc(sp)	# make second copy of old pc
	movq	r0,operand_1(sp) 	# save r0 and r1 in some extra space
	movq	r2,operand_3(sp) 	# do the same for r2 and r3
	cvtbl	*old_pc(sp),r0		# get opcode from instruction stream
 #	movzbl	vax$al_delta_pc_table[r0],r1 # get offset to byte with delta-pc
	 subl3	$0xf8,$vax$al_delta_pc_table,r1 # adjust address for table
	 movzbl	(r1)[r0],r1		     # get offset to byte with delta-pc
	movzbl	operand_1(sp)[r1],r1 	# get delta-pc
	addl2	r1,new_pc(sp)		# convert old pc to new pc
	movl	r0,opcode(sp)		# store opcode in other than a register
	movq	operand_1(sp),r0 	# restore r0 and r1 
					# (r2 and r3 were not changed)
	pushab	vax$exit_emulator	# create return pc to make case like bsb

	caseb	(opcode+4)(sp),$opcode_base,$(0x43)

 #	init_case_table	case_table_size,fpd_case_table_base,10$
fdp_case_table_base:
	.word	Lvax$ashp-fdp_case_table_base	# ashp entry
	.word	Lvax$cvtlp_restart-fdp_case_table_base	# cvtlp entry
	.word	1f-fdp_case_table_base
	.word	1f-fdp_case_table_base
	.word	1f-fdp_case_table_base
	.word	1f-fdp_case_table_base
	.word	1f-fdp_case_table_base
	.word	1f-fdp_case_table_base
	.word	1f-fdp_case_table_base
	.word	1f-fdp_case_table_base
	.word	1f-fdp_case_table_base
	.word	1f-fdp_case_table_base
	.word	1f-fdp_case_table_base
	.word	1f-fdp_case_table_base
	.word	1f-fdp_case_table_base
	.word	1f-fdp_case_table_base
	.word	1f-fdp_case_table_base
	.word	Lvax$cvtsp-fdp_case_table_base		# cvtsp entry
	.word	1f-fdp_case_table_base
	.word	Lvax$crc-fdp_case_table_base		# crc entry
	.word	1f-fdp_case_table_base
	.word	1f-fdp_case_table_base
	.word	1f-fdp_case_table_base
	.word	1f-fdp_case_table_base
	.word	1f-fdp_case_table_base
	.word	1f-fdp_case_table_base
	.word	1f-fdp_case_table_base
	.word	1f-fdp_case_table_base
	.word	1f-fdp_case_table_base
	.word	1f-fdp_case_table_base
	.word	1f-fdp_case_table_base
	.word	1f-fdp_case_table_base
	.word	1f-fdp_case_table_base
	.word	1f-fdp_case_table_base
	.word	1f-fdp_case_table_base
	.word	1f-fdp_case_table_base
	.word	1f-fdp_case_table_base
	.word	1f-fdp_case_table_base
	.word	1f-fdp_case_table_base
	.word	1f-fdp_case_table_base
	.word	Lvax$addp4-fdp_case_table_base	# addp4 entry
	.word	Lvax$addp6-fdp_case_table_base	# addp6 entry
	.word	Lvax$subp4-fdp_case_table_base	# subp4 entry
	.word	Lvax$subp6-fdp_case_table_base	# subp6 entry
	.word	Lvax$cvtpt_restart-fdp_case_table_base	# cvtpt entry
	.word	Lvax$mulp-fdp_case_table_base	# mulp entry
	.word	Lvax$cvttp_restart-fdp_case_table_base	# cvttp entry
	.word	Lvax$divp-fdp_case_table_base	# divp entry
	.word	1f-fdp_case_table_base
	.word	Lvax$cmpc3-fdp_case_table_base		# cmpc3 entry
	.word	Lvax$scanc-fdp_case_table_base		# scanc entry
	.word	Lvax$spanc-fdp_case_table_base		# spanc entry
	.word	1f-fdp_case_table_base
	.word	Lvax$cmpc5-fdp_case_table_base		# cmpc5 entry
	.word	Lvax$movtc-fdp_case_table_base		# movtc entry
	.word	Lvax$movtuc-fdp_case_table_base		# movtuc entry
	.word	1f-fdp_case_table_base
	.word	1f-fdp_case_table_base
	.word	1f-fdp_case_table_base
	.word	1f-fdp_case_table_base
	.word	Lvax$movp-fdp_case_table_base	# movp entry
	.word	Lvax$cmpp3-fdp_case_table_base	# cmpp3 entry
	.word	Lvax$cvtpl_restart-fdp_case_table_base	# cmtpl entry
	.word	Lvax$cmpp4-fdp_case_table_base		# cmpp4 entry
	.word	Lvax$editpc_restart-fdp_case_table_base		# editpc entry
	.word	Lvax$matchc-fdp_case_table_base		# matchc entry
	.word	Lvax$locc-fdp_case_table_base		# locc entry
	.word	Lvax$skpc-fdp_case_table_base		# skpc entry


 # if we drop through the case dispatcher, then the fault was not caused
 # by executing one of the instructions supported by this emulator. the 
 # exception will be passed to vms with the following stack.
 #
 #	00(sp) - signal array size (always 4)
 #	04(sp) - signal name (vax$_opcdec_fpd)
 #	08(sp) - opcode that is not supported
 #	12(sp) - pc of that opcode
 #	16(sp) - psl of exception
 #
 # (in the bootstrap emulator, we simply halt with the stack containing
 # these data.)

1:	addl2	$4,sp			# discard return pc
	movl	old_pc(sp),new_pc(sp)	# use pc of opcode and not new pc
	movl	opcode(sp),operand_8(sp) # include opcode in signal array
	moval	operand_8(sp),sp	# discard rest of stack

	pushl	$vax$_opcdec_fpd	# this is the signal name
	pushl	$4			# signal array has four longwords

	jmp	vax$reflect_to_vms	# use common exit to vms


 #+
 # functional description:
 #
 #	the case tables for the two caseb instructions are built with the
 #	macros that are invoked here. macros are used to guarantee that both
 #	tables contain correct entries for a selected opcode at the same
 #	offset.
 #
 # assumptions:
 #
 #	the case_table_entry macro assumes that the names of the respective
 #	case tables are case_table_base and fpd_case_table_base.
 #
 # notes:
 #
 #	in the following lists, those fpd routines that do not have _fpd in
 #	their names use the same jsb entry point for initial entry and after
 #	restarting the instruction. in most of these cases, the register state
 #	is the same for both starting and restarting. for the remaining cases,
 #	there is not enough difference between the two cases to justify an
 #	additional entry point. (see vax$movtc for an example of this latter
 #	situation.) 
 #
 #	the fpd routines that include _restart in their names have to do a
 #	certain amount of work to restore the intermediate state from the
 #	canonical registers before they can resume instruction execution. 
 #-


 # first generate table entries for the string instructions

/*	case_table_entry	opcode=movtc,-
				routine=movtc,-
				fpd_routine=vax$movtc

	case_table_entry	opcode=movtuc,-
				routine=movtuc,-
				fpd_routine=vax$movtuc

	case_table_entry	opcode=cmpc3,-
				routine=cmpc3,-
				fpd_routine=vax$cmpc3,-
				boot_flag=boot

	case_table_entry	opcode=cmpc5,-
				routine=cmpc5,-
				fpd_routine=vax$cmpc5,-
				boot_flag=boot

	case_table_entry	opcode=locc,-
				routine=locc,-
				fpd_routine=vax$locc,-
				boot_flag=boot

	case_table_entry	opcode=skpc,-
				routine=skpc,-
				fpd_routine=vax$skpc

	case_table_entry	opcode=scanc,-
				routine=scanc,-
				fpd_routine=vax$scanc

	case_table_entry	opcode=spanc,-
				routine=spanc,-
				fpd_routine=vax$spanc

	case_table_entry	opcode=matchc,-
				routine=matchc,-
				fpd_routine=vax$matchc

	case_table_entry	opcode=crc,-
				routine=crc,-
				fpd_routine=vax$crc

 # now generate table entries for the decimal instructions

	case_table_entry	opcode=addp4,-
				routine=addp4,-
				fpd_routine=vax$addp4

	case_table_entry	opcode=addp6,-
				routine=addp6,-
				fpd_routine=vax$addp6

	case_table_entry	opcode=ashp,-
				routine=ashp,-
				fpd_routine=vax$ashp

	case_table_entry	opcode=cmpp3,-
				routine=cmpp3,-
				fpd_routine=vax$cmpp3

	case_table_entry	opcode=cmpp4,-
				routine=cmpp4,-
				fpd_routine=vax$cmpp4

	case_table_entry	opcode=cvtlp,-
				routine=cvtlp,-
				fpd_routine=vax$cvtlp_restart

	case_table_entry	opcode=cvtpl,-
				routine=cvtpl,-
				fpd_routine=vax$cvtpl_restart

	case_table_entry	opcode=cvtps,-
				routine=cvtps,-
				fpd_routine=vax$cvtps

	case_table_entry	opcode=cvtpt,-
				routine=cvtpt,-
				fpd_routine=vax$cvtpt_restart

	case_table_entry	opcode=cvtsp,-
				routine=cvtsp,-
				fpd_routine=vax$cvtsp

	case_table_entry	opcode=cvttp,-
				routine=cvttp,-
				fpd_routine=vax$cvttp_restart

	case_table_entry	opcode=divp,-
				routine=divp,-
				fpd_routine=vax$divp

	case_table_entry	opcode=movp,-
				routine=movp,-
				fpd_routine=vax$movp

	case_table_entry	opcode=mulp,-
				routine=mulp,-
				fpd_routine=vax$mulp

	case_table_entry	opcode=subp4,-
				routine=subp4,-
				fpd_routine=vax$subp4

	case_table_entry	opcode=subp6,-
				routine=subp6,-
				fpd_routine=vax$subp6_unpack

 # editpc always seems to find itself in last place

	case_table_entry	opcode=editpc,-
				routine=editpc,-
				fpd_routine=vax$editpc_restart

	.restore			# reset current location counter

 */


 #++
 # the instruction-specific routines do similar things. rather than clutter up 
 # each routine with the same comments, we will describe the steps that each 
 # routine takes in this section.
 # 
 # the input parameters to each routine are identical.
 # 
 # 		 contents of exception stack
 #		 ---------------------------
 #
 #	opcode(sp)    - opcode of reserved instruction
 #	old_pc(sp)    - pc of reserved instruction 
 #	operand_1(sp) - first operand specifier
 #	operand_2(sp) - second operand specifier
 #	operand_3(sp) - third operand specifier
 #	operand_4(sp) - fourth operand specifier
 #	operand_5(sp) - fifth operand specifier
 #	operand_6(sp) - sixth operand specifier
 #	operand_7(sp) - seventh operand specifier (currently unused)
 #	operand_8(sp) - eight operand specifier (currently unused)
 #	new_pc(sp)    - pc of instruction following reserved instruction
 #	exception_psl(sp) - psl at time of exception
 # 
 #	the routine headers for the instruction-specific routines in this
 #	module will list the input and output parameters in symbolic form
 #	only. the vax$xxxxxx routines in other modules in the emulator contain
 #	the exact meanings of the various operands (parameters) to the
 #	routines.
 #
 # outline of execution:
 #
 #	the operands are loaded into registers as required by the instruction
 #	specific routines. routine headers for each routine contain detailed
 #	descriptions. 
 #
 #	a routine of the form vax$xxxxxx (where xxxxxx is the instruction
 #	name) is called to perform the actual work indicated by each
 #	instruction. 
 #
 #	common exit code executes to allow the condition codes returned by the
 #	vax$xxxxxx routines to be passed back to the code that generated the
 #	original exception. 
 #
 # notes:
 #
 #	the following routines are constructed to be reasonably fast. in 
 #	particular, each instruction has its own separate routine, even though
 #	several instructions differ only in the instruction-specific routine
 #	to which final control is passed. rather than share this common code
 #	at the expense of another dispatch on opcode, we shoose to duplicate
 #	the common code.
 #--




 #+
 # input parameters:
 #
 #	opcode(sp)
 #	old_pc(sp)
 #	operand_1(sp) - srclen.rw
 #	operand_2(sp) - srcaddr.ab
 #	operand_3(sp) - fill.rb
 #	operand_4(sp) - tbladdr.ab
 #	operand_5(sp) - dstlen.rw
 #	operand_6(sp) - dstaddr.ab
 #	operand_7(sp)
 #	operand_8(sp)
 #	new_pc(sp)
 #	exception_psl(sp)
 #
 # output parameters:
 #
 #	r0<15:0> - srclen.rw
 #	r1       - srcaddr.ab
 #	r2<7:0>  - fill.rb
 #	r3       - tbladdr.ab
 #	r4<15:0> - dstlen.rw
 #	r5       - dstaddr.ab
 #
 # implicit output:
 #
 #	r0<31:16> - 0
 #	r2<31:8>  - 0
 #	r4<31:16> - 0
 #-

Lmovtc:
	movzwl	operand_1(sp),r0	# r0<15:0> <- srclen.rw 
	movl	operand_2(sp),r1	# r1       <- srcaddr.ab 
	movzbl	operand_3(sp),r2	# r2<7:0>  <- fill.rb
	movl	operand_4(sp),r3	# r3       <- tbladdr.ab 
	movzwl	operand_5(sp),r4	# r4<15:0> <- dstlen.rw 
	movl	operand_6(sp),r5	# r5       <- dstaddr.ab 

 # now that the operands have been loaded, the only exception parameter
 # other than the pc/psl pair that needs to be saved is the old pc. however,
 # there is no reason why the state of the stack needs to be altered and we
 # save two instructions if we leave the stack alone.

	pushab	vax$exit_emulator	# store the return pc
	jmp	vax$movtc		# do the actual work


 #+
 # input parameters:
 #
 #	opcode(sp)
 #	old_pc(sp)
 #	operand_1(sp) - srclen.rw
 #	operand_2(sp) - srcaddr.ab
 #	operand_3(sp) - esc.rb
 #	operand_4(sp) - tbladdr.ab
 #	operand_5(sp) - dstlen.rw
 #	operand_6(sp) - dstaddr.ab
 #	operand_7(sp)
 #	operand_8(sp)
 #	new_pc(sp)
 #	exception_psl(sp)
 #
 # output parameters:
 #
 #	r0<15:0> - srclen.rw
 #	r1       - srcaddr.ab
 #	r2<7:0>  - esc.rb
 #	r3       - tbladdr.ab
 #	r4<15:0> - dstlen.rw
 #	r5       - dstaddr.ab
 #
 # implicit output:
 #
 #	r0<31:16> - 0
 #	r2<31:8>  - 0
 #	r4<31:16> - 0
 #-

Lmovtuc:

	movzwl	operand_1(sp),r0	# r0<15:0> <- srclen.rw 
	movl	operand_2(sp),r1	# r1       <- srcaddr.ab 
	movzbl	operand_3(sp),r2	# r2<7:0>  <- esc.rb
	movl	operand_4(sp),r3	# r3       <- tbladdr.ab 
	movzwl	operand_5(sp),r4	# r4<15:0> <- dstlen.rw 
	movl	operand_6(sp),r5	# r5       <- dstaddr.ab 

 # now that the operands have been loaded, the only exception parameter
 # other than the pc/psl pair that needs to be saved is the old pc. however,
 # there is no reason why the state of the stack needs to be altered and we
 # save two instructions if we leave the stack alone.

	pushab	vax$exit_emulator	# store the return pc
	jmp	vax$movtuc		# do the actual work


 #+
 # input parameters:
 #
 #	opcode(sp)
 #	old_pc(sp)
 #	operand_1(sp) - len.rw
 #	operand_2(sp) - src1addr.ab
 #	operand_3(sp) - src2addr.ab
 #	operand_4(sp)
 #	operand_5(sp)
 #	operand_6(sp)
 #	operand_7(sp)
 #	operand_8(sp)
 #	new_pc(sp)
 #	exception_psl(sp)
 #
 # output parameters:
 #
 #	r0<15:0> - len.rw
 #	r1       - src1addr.ab
 #	r3       - src2addr.ab
 #
 # implicit output:
 #
 #	r0<31:16> - 0
 #	r2        - unpredictable
 #-

Lcmpc3:

	movzwl	operand_1(sp),r0	# r0<15:0> <- srclen.rw 
	movl	operand_2(sp),r1	# r1       <- src1addr.ab 
	movl	operand_3(sp),r3	# r3       <- src2addr.ab 

 # now that the operands have been loaded, the only exception parameter
 # other than the pc/psl pair that needs to be saved is the old pc. however,
 # there is no reason why the state of the stack needs to be altered and we
 # save two instructions if we leave the stack alone.

	pushab	vax$exit_emulator	# store the return pc
	jmp	vax$cmpc3		# do the actual work


 #+
 # input parameters:
 #
 #	opcode(sp)
 #	old_pc(sp)
 #	operand_1(sp) - src1len.rw
 #	operand_2(sp) - src1addr.ab
 #	operand_3(sp) - fill.rb
 #	operand_4(sp) - src2len.rw
 #	operand_5(sp) - src2addr.ab
 #	operand_6(sp)
 #	operand_7(sp)
 #	operand_8(sp)
 #	new_pc(sp)
 #	exception_psl(sp)
 #
 # output parameters:
 #
 #	r0<15:0>   - srclen.rw
 #	r0<23:16>  - fill.rb
 #	r1         - srcaddr.ab
 #	r2<15:0>   - src2len.rw
 #	r3         - src2addr.ab
 #
 # implicit output:
 #
 #	r0<31:24> - unpredictable
 #	r2<31:16> - 0
 #-

Lcmpc5:

	rotl	$16,operand_3(sp),r0	# r0<23:16> <- fill.rb
	movw	operand_1(sp),r0	# r0<15:0>  <- src1len.rw 
	movl	operand_2(sp),r1	# r1        <- src1addr.ab 
	movzwl	operand_4(sp),r2	# r2<15:0>  <- src2len.rw 
	movl	operand_5(sp),r3	# r3        <sca- src2addr.ab 

 # now that the operands have been loaded, the only exception parameter
 # other than the pc/psl pair that needs to be saved is the old pc. however,
 # there is no reason why the state of the stack needs to be altered and we
 # save two instructions if we leave the stack alone.

	pushab	vax$exit_emulator	# store the return pc
	jmp	vax$cmpc5		# do the actual work

 #+
 # input parameters:
 #
 #	opcode(sp)
 #	old_pc(sp)
 #	operand_1(sp) - len.rw
 #	operand_2(sp) - addr.ab
 #	operand_3(sp) - tbladdr.ab
 #	operand_4(sp) - mask.ab
 #	operand_5(sp)
 #	operand_6(sp)
 #	operand_7(sp)
 #	operand_8(sp)
 #	new_pc(sp)
 #	exception_psl(sp)
 #
 # output parameters:
 #
 #	r0<15:0> - len.rw
 #	r1       - addr.ab
 #	r2<7:0>  - mask.rb
 #	r3       - tbladdr.ab
 #
 # implicit output:
 #
 #	r0<31:16> - 0
 #	r2<31:8>  - 0
 #-

Lscanc:

	movzwl	operand_1(sp),r0	# r0<15:0> <- len.rw 
	movl	operand_2(sp),r1	# r1       <- addr.ab 
	movl	operand_3(sp),r3	# r3       <- tbladdr.ab 
	movzbl	operand_4(sp),r2	# r2<7:0>  <- mask.ab

 # now that the operands have been loaded, the only exception parameter
 # other than the pc/psl pair that needs to be saved is the old pc. however,
 # there is no reason why the state of the stack needs to be altered and we
 # save two instructions if we leave the stack alone.

	pushab	vax$exit_emulator	# store the return pc
	jmp	vax$scanc		# do the actual work


 #+
 # input parameters:
 #
 #	opcode(sp)
 #	old_pc(sp)
 #	operand_1(sp) - len.rw
 #	operand_2(sp) - addr.ab
 #	operand_3(sp) - tbladdr.ab
 #	operand_4(sp) - mask.ab
 #	operand_5(sp)
 #	operand_6(sp)
 #	operand_7(sp)
 #	operand_8(sp)
 #	new_pc(sp)
 #	exception_psl(sp)
 #
 # output parameters:
 #
 #	r0<15:0> - len.rw
 #	r1       - addr.ab
 #	r2<7:0>  - mask.rb
 #	r3       - tbladdr.ab
 #
 # implicit output:
 #
 #	r0<31:16> - 0
 #	r2<31:8>  - 0
 #-

Lspanc:

	movzwl	operand_1(sp),r0	# r0<15:0> <- len.rw 
	movl	operand_2(sp),r1	# r1       <- addr.ab 
	movl	operand_3(sp),r3	# r3       <- tbladdr.ab 
	movzbl	operand_4(sp),r2	# r2<7:0>  <- mask.ab

 # now that the operands have been loaded, the only exception parameter
 # other than the pc/psl pair that needs to be saved is the old pc. however,
 # there is no reason why the state of the stack needs to be altered and we
 # save two instructions if we leave the stack alone.

	pushab	vax$exit_emulator	# store the return pc
	jmp	vax$spanc		# do the actual work


 #+
 # input parameters:
 #
 #	opcode(sp)
 #	old_pc(sp)
 #	operand_1(sp) - char.rb
 #	operand_2(sp) - len.rw
 #	operand_3(sp) - addr.ab
 #	operand_4(sp)
 #	operand_5(sp)
 #	operand_6(sp)
 #	operand_7(sp)
 #	operand_8(sp)
 #	new_pc(sp)
 #	exception_psl(sp)
 #
 # output parameters:
 #
 #	r0<15:0>  - len.rw
 #	r0<23:16> - char.rb
 #	r1        - addr.ab
 #
 # implicit output:
 #
 #	r0<31:24> - unpredictable
 #-

Llocc:

	rotl	$16,operand_1(sp),r0	# r0<23:16> <- char.ab
	movw	operand_2(sp),r0	# r0<15:0>  <- len.rw 
	movl	operand_3(sp),r1	# r1        <- addr.ab 

 # now that the operands have been loaded, the only exception parameter
 # other than the pc/psl pair that needs to be saved is the old pc. however,
 # there is no reason why the state of the stack needs to be altered and we
 # save two instructions if we leave the stack alone.

	pushab	vax$exit_emulator	# store the return pc
	jmp	vax$locc		# do the actual work


 #+
 # input parameters:
 #
 #	opcode(sp)
 #	old_pc(sp)
 #	operand_1(sp) - char.rb
 #	operand_2(sp) - len.rw
 #	operand_3(sp) - addr.ab
 #	operand_4(sp)
 #	operand_5(sp)
 #	operand_6(sp)
 #	operand_7(sp)
 #	operand_8(sp)
 #	new_pc(sp)
 #	exception_psl(sp)
 #
 # output parameters:
 #
 #	r0<15:0>  - len.rw
 #	r0<23:16> - char.rb
 #	r1        - addr.ab
 #
 # implicit output:
 #
 #	r0<31:24> - unpredictable
 #-

Lskpc:

	rotl	$16,operand_1(sp),r0	# r0<23:16> <- char.ab
	movw	operand_2(sp),r0	# r0<15:0>  <- len.rw 
	movl	operand_3(sp),r1	# r1        <- addr.ab 

 # now that the operands have been loaded, the only exception parameter
 # other than the pc/psl pair that needs to be saved is the old pc. however,
 # there is no reason why the state of the stack needs to be altered and we
 # save two instructions if we leave the stack alone.

	pushab	vax$exit_emulator	# store the return pc
	jmp	vax$skpc		# do the actual work


 #+
 # input parameters:
 #
 #	opcode(sp)
 #	old_pc(sp)
 #	operand_1(sp) - objlen.rw
 #	operand_2(sp) - objaddr.ab
 #	operand_3(sp) - srclen.rw
 #	operand_4(sp) - srcaddr.ab
 #	operand_5(sp)
 #	operand_6(sp)
 #	operand_7(sp)
 #	operand_8(sp)
 #	new_pc(sp)
 #	exception_psl(sp)
 #
 # output parameters:
 #
 #	r0<15:0>  - objlen.rw
 #	r1        - objaddr.ab
 #	r2<15:0>  - srclen.rw
 #	r3        - srcaddr.ab
 #
 # implicit output:
 #
 #	r0<31:16> - 0
 #	r2<31:16> - 0
 #-

Lmatchc:

	movzwl	operand_1(sp),r0	# r0<15:0>  <- objlen.rw 
	movl	operand_2(sp),r1	# r1        <- objaddr.ab 
	movzwl	operand_3(sp),r2	# r2<15:0>  <- srclen.rw 
	movl	operand_4(sp),r3	# r3        <- srcaddr.ab 

 # now that the operands have been loaded, the only exception parameter
 # other than the pc/psl pair that needs to be saved is the old pc. however,
 # there is no reason why the state of the stack needs to be altered and we
 # save two instructions if we leave the stack alone.

	pushab	vax$exit_emulator	# store the return pc
	jmp	vax$matchc		# do the actual work


 #+
 # input parameters:
 #
 #	opcode(sp)
 #	old_pc(sp)
 #	operand_1(sp) - tbl.ab
 #	operand_2(sp) - inicrc.rl
 #	operand_3(sp) - strlen.rw
 #	operand_4(sp) - stream.ab
 #	operand_5(sp)
 #	operand_6(sp)
 #	operand_7(sp)
 #	operand_8(sp)
 #	new_pc(sp)
 #	exception_psl(sp)
 #
 # output parameters:
 #
 #	r0        - inicrc.rl
 #	r1        - tbl.ab
 #	r2<15:0>  - strlen.rw
 #	r3        - stream.ab
 #
 # implicit output:
 #
 #	r2<31:16> - 0
 #-

Lcrc:

	movl	operand_1(sp),r1	# r1        <- tbl.ab 
	movl	operand_2(sp),r0	# r0        <- inicrc.rl
	movzwl	operand_3(sp),r2	# r2<15:0>  <- strlen.rw 
	movl	operand_4(sp),r3	# r3        <- stream.ab 

 # now that the operands have been loaded, the only exception parameter
 # other than the pc/psl pair that needs to be saved is the old pc. however,
 # there is no reason why the state of the stack needs to be altered and we
 # save two instructions if we leave the stack alone.

	pushab	vax$exit_emulator	# store the return pc
	jmp	vax$crc			# do the actual work



 #+
 # input parameters:
 #
 #	opcode(sp)
 #	old_pc(sp)
 #	operand_1(sp) - addlen.rw
 #	operand_2(sp) - addaddr.ab
 #	operand_3(sp) - sumlen.rw
 #	operand_4(sp) - sumaddr.ab
 #	operand_5(sp)
 #	operand_6(sp)
 #	operand_7(sp)
 #	operand_8(sp)
 #	new_pc(sp)
 #	exception_psl(sp)
 #
 # output parameters:
 #
 #	r0<15:0> - addlen.rw
 #	r1       - addaddr.ab
 #	r2<15:0> - sumlen.rw
 #	r3       - sumaddr.ab
 #
 # implicit output:
 #
 #	r0<31:16> - 0
 #	r2<31:16> - 0
 #-

Laddp4:

	movzwl	operand_1(sp),r0	# r0<15:0> <- addlen.rw 
	movl	operand_2(sp),r1	# r1       <- addaddr.ab 
	movzwl	operand_3(sp),r2	# r2<15:0> <- sumlen.rw 
	movl	operand_4(sp),r3	# r3       <- sumaddr.ab 

 # now that the operands have been loaded, the only exception parameter
 # other than the pc/psl pair that needs to be saved is the old pc. however,
 # there is no reason why the state of the stack needs to be altered and we
 # save two instructions if we leave the stack alone.

	pushab	vax$exit_emulator	# store the return pc
	jmp	vax$addp4		# do the actual work


 #+
 # input parameters:
 #
 #	opcode(sp)
 #	old_pc(sp)
 #	operand_1(sp) - add1len.rw
 #	operand_2(sp) - add1addr.ab
 #	operand_3(sp) - add2len.rw
 #	operand_4(sp) - add2addr.ab
 #	operand_5(sp) - sumlen.rw
 #	operand_6(sp) - sumaddr.ab
 #	operand_7(sp)
 #	operand_8(sp)
 #	new_pc(sp)
 #	exception_psl(sp)
 #
 # output parameters:
 #
 #	r0<15:0> - add1len.rw
 #	r1       - add1addr.ab
 #	r2<15:0> - add2len.rw
 #	r3       - add2addr.ab
 #	r4<15:0> - sumlen.rw
 #	r5       - sumaddr.ab
 #
 # implicit output:
 #
 #	r0<31:16> - 0
 #	r2<31:16> - 0
 #	r4<31:16> - 0
 #-

Laddp6:

	movzwl	operand_1(sp),r0	# r0<15:0> <- add1len.rw 
	movl	operand_2(sp),r1	# r1       <- add1addr.ab 
	movzwl	operand_3(sp),r2	# r2<15:0> <- add2len.rw 
	movl	operand_4(sp),r3	# r3       <- add2addr.ab 
	movzwl	operand_5(sp),r4	# r4<15:0> <- sumlen.rw 
	movl	operand_6(sp),r5	# r5       <- sumaddr.ab 

 # now that the operands have been loaded, the only exception parameter
 # other than the pc/psl pair that needs to be saved is the old pc. however,
 # there is no reason why the state of the stack needs to be altered and we
 # save two instructions if we leave the stack alone.

	pushab	vax$exit_emulator	# store the return pc
	jmp	vax$addp6		# do the actual work

 #+
 # input parameters:
 #
 #	opcode(sp)
 #	old_pc(sp)
 #	operand_1(sp) - cnt.rb
 #	operand_2(sp) - srclen.rw
 #	operand_3(sp) - srcaddr.ab
 #	operand_4(sp) - round.rb
 #	operand_5(sp) - dstlen.rw
 #	operand_6(sp) - dstaddr.ab
 #	operand_7(sp)
 #	operand_8(sp)
 #	new_pc(sp)
 #	exception_psl(sp)
 #
 # output parameters:
 #
 #	r0<15:0>  - srclen.rw
 #	r0<31:16> - count.rb
 #	r1        - srcaddr.ab
 #	r2<15:0>  - dstlen.rw
 #	r2<31:16> - round.rb
 #	r3        - dstaddr.ab
 #
 # implicit output:
 #
 #	r0<31:24> - unpredictable
 #	r2<31:24> - unpredictable
 #-

Lashp:

	rotl	$16,operand_1(sp),r0	# r0<31:16> <- count.rb
	movw	operand_2(sp),r0	# r0<15:0>  <- srclen.rw 
	movl	operand_3(sp),r1	# r1        <- srcaddr.ab 
	rotl	$16,operand_4(sp),r2	# r2<31:16> <- round.rb
	movw	operand_5(sp),r2	# r2<15:0>  <- dstlen.rw 
	movl	operand_6(sp),r3	# r3        <- dstaddr.ab 

 # now that the operands have been loaded, the only exception parameter
 # other than the pc/psl pair that needs to be saved is the old pc. however,
 # there is no reason why the state of the stack needs to be altered and we
 # save two instructions if we leave the stack alone.

	pushab	vax$exit_emulator	# store the return pc
	jmp	vax$ashp		# do the actual work

 #+
 # input parameters:
 #
 #	opcode(sp)
 #	old_pc(sp)
 #	operand_1(sp) - len.rw
 #	operand_2(sp) - src1addr.ab
 #	operand_3(sp) - src2addr.ab
 #	operand_4(sp)
 #	operand_5(sp)
 #	operand_6(sp)
 #	operand_7(sp)
 #	operand_8(sp)
 #	new_pc(sp)
 #	exception_psl(sp)
 #
 # output parameters:
 #
 #	r0<15:0> - len.rw
 #	r1       - src1addr.ab
 #	r3       - src2addr.ab
 #
 # implicit output:
 #
 #	r0<31:16> - 0
 #	r2        - unpredictable
 #-

Lcmpp3:

	movzwl	operand_1(sp),r0	# r0<15:0> <- len.rw 
	movl	operand_2(sp),r1	# r1       <- src1addr.ab 
	movl	operand_3(sp),r3	# r3       <- src2addr.ab 

 # now that the operands have been loaded, the only exception parameter
 # other than the pc/psl pair that needs to be saved is the old pc. however,
 # there is no reason why the state of the stack needs to be altered and we
 # save two instructions if we leave the stack alone.

	pushab	vax$exit_emulator	# store the return pc
	jmp	vax$cmpp3		# do the actual work




 #+
 # input parameters:
 #
 #	opcode(sp)
 #	old_pc(sp)
 #	operand_1(sp) - src1len.rw
 #	operand_2(sp) - src1addr.ab
 #	operand_3(sp) - src2len.rw
 #	operand_4(sp) - src2addr.ab
 #	operand_5(sp)
 #	operand_6(sp)
 #	operand_7(sp)
 #	operand_8(sp)
 #	new_pc(sp)
 #	exception_psl(sp)
 #
 # output parameters:
 #
 #	r0<15:0> - src1len.rw
 #	r1       - src1addr.ab
 #	r2<15:0> - src2len.rw
 #	r3       - src2addr.ab
 #
 # implicit output:
 #
 #	r0<31:16> - 0
 #	r2<31:16> - 0
 #-

Lcmpp4:

	movzwl	operand_1(sp),r0	# r0<15:0> <- src1len.rw 
	movl	operand_2(sp),r1	# r1       <- src1addr.ab 
	movzwl	operand_3(sp),r2	# r2<15:0> <- src2len.rw 
	movl	operand_4(sp),r3	# r3       <- src2addr.ab 

 # now that the operands have been loaded, the only exception parameter
 # other than the pc/psl pair that needs to be saved is the old pc. however,
 # there is no reason why the state of the stack needs to be altered and we
 # save two instructions if we leave the stack alone.

	pushab	vax$exit_emulator	# store the return pc
	jmp	vax$cmpp4		# do the actual work


 #+
 # input parameters:
 #
 #	opcode(sp)
 #	old_pc(sp)
 #	operand_1(sp) - src.rl
 #	operand_2(sp) - dstlen.rw
 #	operand_3(sp) - dstaddr.ab
 #	operand_4(sp)
 #	operand_5(sp)
 #	operand_6(sp)
 #	operand_7(sp)
 #	operand_8(sp)
 #	new_pc(sp)
 #	exception_psl(sp)
 #
 # output parameters:
 #
 #	r0       - src.rl
 #	r2<15:0> - dstlen.rw
 #	r3       - dstaddr.ab
 #
 # implicit output:
 #
 #	r1        - explicity set to zero
 #	r2<31:16> - 0
 #-

Lcvtlp:

	movl	operand_1(sp),r0	# r0       <- src.rl 
	clrl	r1			# r1	   <- 0
	movzwl	operand_2(sp),r2	# r2<15:0> <- dstlen.rw 
	movl	operand_3(sp),r3	# r3       <- dstaddr.ab 

 # now that the operands have been loaded, the only exception parameter
 # other than the pc/psl pair that needs to be saved is the old pc. however,
 # there is no reason why the state of the stack needs to be altered and we
 # save two instructions if we leave the stack alone.

	pushab	vax$exit_emulator	# store the return pc
	jmp	vax$cvtlp		# do the actual work




 #+
 # input parameters:
 #
 #	opcode(sp)
 #	old_pc(sp)
 #	operand_1(sp) - srclen.rw
 #	operand_2(sp) - srcaddr.ab
 #	operand_3(sp) - dst.wl
 #	operand_4(sp)
 #	operand_5(sp)
 #	operand_6(sp)
 #	operand_7(sp)
 #	operand_8(sp)
 #	new_pc(sp)
 #	exception_psl(sp)
 #
 # output parameters:
 #
 #	r0<15:0> - srclen.rw
 #	r1       - srcaddr.ab
 #	r3       - dst.wl
 #
 # notes:
 #
 #	the routine header for vax$cvtpl describes how the destination is
 #	encoded in a register. basically, operand_3 contains the effective
 #	address of the operand. if the destination is a general register, then
 #	operand_3 contains the ones complement of the register number. 
 #
 # implicit output:
 #
 #	r0<31:16> - 0
 #	r2        - explicitly set to zero
 #-

Lcvtpl:

	movzwl	operand_1(sp),r0	# r0<15:0> <- srclen.rw 
	movl	operand_2(sp),r1	# r1       <- srcaddr.ab 
	clrl	r2			# r2	   <- 0
	movl	operand_3(sp),r3	# r3       <- dst.wl 

 # now that the operands have been loaded, the only exception parameter
 # other than the pc/psl pair that needs to be saved is the old pc. however,
 # there is no reason why the state of the stack needs to be altered and we
 # save two instructions if we leave the stack alone.

	pushab	vax$exit_emulator	# store the return pc
	jmp	vax$cvtpl		# do the actual work


 #+
 # input parameters:
 #
 #	opcode(sp)
 #	old_pc(sp)
 #	operand_1(sp) - srclen.rw
 #	operand_2(sp) - srcaddr.ab
 #	operand_3(sp) - dstlen.rw
 #	operand_4(sp) - dstaddr.ab
 #	operand_5(sp)
 #	operand_6(sp)
 #	operand_7(sp)
 #	operand_8(sp)
 #	new_pc(sp)
 #	exception_psl(sp)
 #
 # output parameters:
 #
 #	r0<15:0> - srclen.rw
 #	r1       - srcaddr.ab
 #	r2<15:0> - dstlen.rw
 #	r3       - dstaddr.ab
 #
 # implicit output:
 #
 #	r0<31:16> - 0
 #	r2<31:16> - 0
 #-

Lcvtps:

	movzwl	operand_1(sp),r0	# r0<15:0> <- srclen.rw 
	movl	operand_2(sp),r1	# r1       <- srcaddr.ab 
	movzwl	operand_3(sp),r2	# r2<15:0> <- dstlen.rw 
	movl	operand_4(sp),r3	# r3       <- dstaddr.ab 

 # now that the operands have been loaded, the only exception parameter
 # other than the pc/psl pair that needs to be saved is the old pc. however,
 # there is no reason why the state of the stack needs to be altered and we
 # save two instructions if we leave the stack alone.

	pushab	vax$exit_emulator	# store the return pc
	jmp	vax$cvtps		# do the actual work


 #+
 # input parameters:
 #
 #	opcode(sp)
 #	old_pc(sp)
 #	operand_1(sp) - srclen.rw
 #	operand_2(sp) - srcaddr.ab
 #	operand_3(sp) - tbladdr.ab
 #	operand_4(sp) - dstlen.rw
 #	operand_5(sp) - dstaddr.ab
 #	operand_6(sp)
 #	operand_7(sp)
 #	operand_8(sp)
 #	new_pc(sp)
 #	exception_psl(sp)
 #
 # output parameters:
 #
 #	r0<15:0>  - srclen.rw
 #	r0<31:16> - dstlen.rw
 #	r1        - srcaddr.ab
 #	r2        - tbladdr.ab
 #	r3        - dstaddr.ab
 #-

Lcvtpt:

	rotl	$16,operand_4(sp),r0	# r0<31:16> <- dstlen.rw 
	movw	operand_1(sp),r0	# r0<15:0>  <- srclen.rw 
	movl	operand_2(sp),r1	# r1        <- srcaddr.ab 
	movl	operand_3(sp),r2	# r2        <- tbladdr.ab 
	movl	operand_5(sp),r3	# r3        <- dstaddr.ab 

 # now that the operands have been loaded, the only exception parameter
 # other than the pc/psl pair that needs to be saved is the old pc. however,
 # there is no reason why the state of the stack needs to be altered and we
 # save two instructions if we leave the stack alone.

	pushab	vax$exit_emulator	# store the return pc
	jmp	vax$cvtpt		# do the actual work



 #+
 # input parameters:
 #
 #	opcode(sp)
 #	old_pc(sp)
 #	operand_1(sp) - srclen.rw
 #	operand_2(sp) - srcaddr.ab
 #	operand_3(sp) - dstlen.rw
 #	operand_4(sp) - dstaddr.ab
 #	operand_5(sp)
 #	operand_6(sp)
 #	operand_7(sp)
 #	operand_8(sp)
 #	new_pc(sp)
 #	exception_psl(sp)
 #
 # output parameters:
 #
 #	r0<15:0> - srclen.rw
 #	r1       - srcaddr.ab
 #	r2<15:0> - dstlen.rw
 #	r3       - dstaddr.ab
 #
 # implicit output:
 #
 #	r0<31:16> - 0
 #	r2<31:16> - 0
 #-

Lcvtsp:

	movzwl	operand_1(sp),r0	# r0<15:0> <- srclen.rw 
	movl	operand_2(sp),r1	# r1       <- srcaddr.ab 
	movzwl	operand_3(sp),r2	# r2<15:0> <- dstlen.rw 
	movl	operand_4(sp),r3	# r3       <- dstaddr.ab 

 # now that the operands have been loaded, the only exception parameter
 # other than the pc/psl pair that needs to be saved is the old pc. however,
 # there is no reason why the state of the stack needs to be altered and we
 # save two instructions if we leave the stack alone.

	pushab	vax$exit_emulator	# store the return pc
	jmp	vax$cvtsp		# do the actual work


 #+
 # input parameters:
 #
 #	opcode(sp)
 #	old_pc(sp)
 #	operand_1(sp) - srclen.rw
 #	operand_2(sp) - srcaddr.ab
 #	operand_3(sp) - tbladdr.ab
 #	operand_4(sp) - dstlen.rw
 #	operand_5(sp) - dstaddr.ab
 #	operand_6(sp)
 #	operand_7(sp)
 #	operand_8(sp)
 #	new_pc(sp)
 #	exception_psl(sp)
 #
 # output parameters:
 #
 #	r0<15:0>  - srclen.rw
 #	r0<31:16> - dstlen.rw
 #	r1        - srcaddr.ab
 #	r2        - tbladdr.ab
 #	r3        - dstaddr.ab
 #-

Lcvttp:

	rotl	$16,operand_4(sp),r0	# r0<31:16> <- dstlen.rw 
	movw	operand_1(sp),r0	# r0<15:0>  <- srclen.rw 
	movl	operand_2(sp),r1	# r1        <- srcaddr.ab 
	movl	operand_3(sp),r2	# r2        <- tbladdr.ab 
	movl	operand_5(sp),r3	# r3        <- dstaddr.ab 

 # now that the operands have been loaded, the only exception parameter
 # other than the pc/psl pair that needs to be saved is the old pc. however,
 # there is no reason why the state of the stack needs to be altered and we
 # save two instructions if we leave the stack alone.

	pushab	vax$exit_emulator	# store the return pc
	jmp	vax$cvttp		# do the actual work





 #+
 # input parameters:
 #
 #	opcode(sp)
 #	old_pc(sp)
 #	operand_1(sp) - divrlen.rw
 #	operand_2(sp) - divraddr.ab
 #	operand_3(sp) - divdlen.rw
 #	operand_4(sp) - divdaddr.ab
 #	operand_5(sp) - quolen.rw
 #	operand_6(sp) - quoaddr.ab
 #	operand_7(sp)
 #	operand_8(sp)
 #	new_pc(sp)
 #	exception_psl(sp)
 #
 # output parameters:
 #
 #	r0<15:0> - divrlen.rw
 #	r1       - divraddr.ab
 #	r2<15:0> - divdlen.rw
 #	r3       - divdaddr.ab
 #	r4<15:0> - quolen.rw
 #	r5       - quoaddr.ab
 #
 # implicit output:
 #
 #	r0<31:16> - 0
 #	r2<31:16> - 0
 #	r4<31:16> - 0
 #-

Ldivp:

	movzwl	operand_1(sp),r0	# r0<15:0> <- divrlen.rw 
	movl	operand_2(sp),r1	# r1       <- divraddr.ab 
	movzwl	operand_3(sp),r2	# r2<15:0> <- divdlen.rw 
	movl	operand_4(sp),r3	# r3       <- divdaddr.ab 
	movzwl	operand_5(sp),r4	# r4<15:0> <- quolen.rw 
	movl	operand_6(sp),r5	# r5       <- quoaddr.ab 

 # now that the operands have been loaded, the only exception parameter
 # other than the pc/psl pair that needs to be saved is the old pc. however,
 # there is no reason why the state of the stack needs to be altered and we
 # save two instructions if we leave the stack alone.

	pushab	vax$exit_emulator	# store the return pc
	jmp	vax$divp		# do the actual work


 #+
 # input parameters:
 #
 #	opcode(sp)
 #	old_pc(sp)
 #	operand_1(sp) - len.rw
 #	operand_2(sp) - srcaddr.ab
 #	operand_3(sp) - dstaddr.ab
 #	operand_4(sp)
 #	operand_5(sp)
 #	operand_6(sp)
 #	operand_7(sp)
 #	operand_8(sp)
 #	new_pc(sp)
 #	exception_psl(sp)
 #
 # output parameters:
 #
 #	r0<15:0> - len.rw
 #	r1       - srcaddr.ab
 #	r3       - dstaddr.ab
 #
 # implicit output:
 #
 #	r0<31:16> - 0
 #	r2        - unpredictable
 #-

Lmovp:

	movzwl	operand_1(sp),r0	# r0<15:0> <- len.rw 
	movl	operand_2(sp),r1	# r1       <- srcaddr.ab 
	movl	operand_3(sp),r3	# r3       <- dstaddr.ab 

 # now that the operands have been loaded, the only exception parameter
 # other than the pc/psl pair that needs to be saved is the old pc. however,
 # there is no reason why the state of the stack needs to be altered and we
 # save two instructions if we leave the stack alone.

 # the movp instruction is the only instruction in this entire set that
 # preserves the setting of the c-bit. the c-bit setting in the saved psl
 # is propogated into the current psl because the current psl forms the
 # initial setting for the final settings of the condition codes.

	bicpsw	$psl$m_c		# assume c bit is clear
	bitb	$psl$m_c,exception_psl(sp) # is the saved c-bit set?
	beql	1f			# skip next if saved c-bit is clear
	bispsw	$psl$m_c		# otherwise, set the c-bit

 # note that it is crucial that no instructions that alter the c-bit can
 # execute until the psl is saved in vax$movp. pushab preserves the c-bit.

1:	pushab	vax$exit_emulator	# store the return pc
	jmp	vax$movp		# do the actual work


 #+
 # input parameters:
 #
 #	opcode(sp)
 #	old_pc(sp)
 #	operand_1(sp) - mulrlen.rw
 #	operand_2(sp) - mulraddr.ab
 #	operand_3(sp) - muldlen.rw
 #	operand_4(sp) - muldaddr.ab
 #	operand_5(sp) - prodlen.rw
 #	operand_6(sp) - prodaddr.ab
 #	operand_7(sp)
 #	operand_8(sp)
 #	new_pc(sp)
 #	exception_psl(sp)
 #
 # output parameters:
 #
 #	r0<15:0> - mulrlen.rw
 #	r1       - mulraddr.ab
 #	r2<15:0> - muldlen.rw
 #	r3       - muldaddr.ab
 #	r4<15:0> - prodlen.rw
 #	r5       - prodaddr.ab
 #
 # implicit output:
 #
 #	r0<31:16> - 0
 #	r2<31:16> - 0
 #	r4<31:16> - 0
 #-

Lmulp:

	movzwl	operand_1(sp),r0	# r0<15:0> <- mulrlen.rw 
	movl	operand_2(sp),r1	# r1       <- mulraddr.ab 
	movzwl	operand_3(sp),r2	# r2<15:0> <- muldlen.rw 
	movl	operand_4(sp),r3	# r3       <- muldaddr.ab 
	movzwl	operand_5(sp),r4	# r4<15:0> <- prodlen.rw 
	movl	operand_6(sp),r5	# r5       <- prodaddr.ab 

 # now that the operands have been loaded, the only exception parameter
 # other than the pc/psl pair that needs to be saved is the old pc. however,
 # there is no reason why the state of the stack needs to be altered and we
 # save two instructions if we leave the stack alone.

	pushab	vax$exit_emulator	# store the return pc
	jmp	vax$mulp		# do the actual work



 #+
 # input parameters:
 #
 #	opcode(sp)
 #	old_pc(sp)
 #	operand_1(sp) - sublen.rw
 #	operand_2(sp) - subaddr.ab
 #	operand_3(sp) - diflen.rw
 #	operand_4(sp) - difaddr.ab
 #	operand_5(sp)
 #	operand_6(sp)
 #	operand_7(sp)
 #	operand_8(sp)
 #	new_pc(sp)
 #	exception_psl(sp)
 #
 # output parameters:
 #
 #	r0<15:0> - sublen.rw
 #	r1       - subaddr.ab
 #	r2<15:0> - diflen.rw
 #	r3       - difaddr.ab
 #
 # implicit output:
 #
 #	r0<31:16> - 0
 #	r2<31:16> - 0
 #-

Lsubp4:

	movzwl	operand_1(sp),r0	# r0<15:0> <- sublen.rw 
	movl	operand_2(sp),r1	# r1       <- subaddr.ab 
	movzwl	operand_3(sp),r2	# r2<15:0> <- diflen.rw 
	movl	operand_4(sp),r3	# r3       <- difaddr.ab 

 # now that the operands have been loaded, the only exception parameter
 # other than the pc/psl pair that needs to be saved is the old pc. however,
 # there is no reason why the state of the stack needs to be altered and we
 # save two instructions if we leave the stack alone.

	pushab	vax$exit_emulator	# store the return pc
	jmp	vax$subp4		# do the actual work


 #+
 # input parameters:
 #
 #	opcode(sp)
 #	old_pc(sp)
 #	operand_1(sp) - sublen.rw
 #	operand_2(sp) - subaddr.ab
 #	operand_3(sp) - minlen.rw
 #	operand_4(sp) - minaddr.ab
 #	operand_5(sp) - diflen.rw
 #	operand_6(sp) - difaddr.ab
 #	operand_7(sp)
 #	operand_8(sp)
 #	new_pc(sp)
 #	exception_psl(sp)
 #
 # output parameters:
 #
 #	r0<15:0> - sublen.rw
 #	r1       - subaddr.ab
 #	r2<15:0> - minlen.rw
 #	r3       - minaddr.ab
 #	r4<15:0> - diflen.rw
 #	r5       - difaddr.ab
 #
 # implicit output:
 #
 #	r0<31:16> - 0
 #	r2<31:16> - 0
 #	r4<31:16> - 0
 #-

Lsubp6:

	movzwl	operand_1(sp),r0	# r0<15:0> <- sublen.rw 
	movl	operand_2(sp),r1	# r1       <- subaddr.ab 
	movzwl	operand_3(sp),r2	# r2<15:0> <- minlen.rw 
	movl	operand_4(sp),r3	# r3       <- minaddr.ab 
	movzwl	operand_5(sp),r4	# r4<15:0> <- diflen.rw 
	movl	operand_6(sp),r5	# r5       <- difaddr.ab 

 # now that the operands have been loaded, the only exception parameter
 # other than the pc/psl pair that needs to be saved is the old pc. however,
 # there is no reason why the state of the stack needs to be altered and we
 # save two instructions if we leave the stack alone.

	pushab	vax$exit_emulator	# store the return pc
	jmp	vax$subp6		# do the actual work


 #+
 # input parameters:
 #
 #	opcode(sp)
 #	old_pc(sp)
 #	operand_1(sp) - srclen.rw
 #	operand_2(sp) - srcaddr.ab
 #	operand_3(sp) - pattern.ab
 #	operand_4(sp) - dstaddr.ab
 #	operand_5(sp)
 #	operand_6(sp)
 #	operand_7(sp)
 #	operand_8(sp)
 #	new_pc(sp)
 #	exception_psl(sp)
 #
 # output parameters:
 #
 #	r0<15:0> - srclen.rw
 #	r1       - srcaddr.ab
 #	r3       - pattern.ab
 #	r5       - dstaddr.ab
 #
 # implicit output:
 #
 #	r0<31:16> - 0
 #	r2        - explicitly set to zero
 #	r4        - explicitly set to zero
 #-

Leditpc:

	movzwl	operand_1(sp),r0	# r0<15:0> <- srclen.rw 
	movl	operand_2(sp),r1	# r1       <- srcaddr.ab 
	clrl	r2			# r2	   <- 0
	movl	operand_3(sp),r3	# r3       <- pattern.ab 
	clrl	r4			# r4	   <- 0
	movl	operand_4(sp),r5	# r5       <- dstaddr.ab 

 # now that the operands have been loaded, the only exception parameter
 # other than the pc/psl pair that needs to be saved is the old pc. however,
 # there is no reason why the state of the stack needs to be altered and we
 # save two instructions if we leave the stack alone.

	pushab	vax$exit_emulator	# store the return pc
	jmp	vax$editpc		# do the actual work


 #+
 # functional description:
 #
 #	this is the common exit path for all instruction-specific routines.
 #	the condition codes returned by the vax$xxxxxx routine are stored in
 #	the exception psl and control is passed back to the instruction stream
 #	that executed the reserved instruction. 
 #
 # input parameters:
 #
 #	psl contains condition code settings from vax$xxxxxx routine.
 #
 #	opcode(sp)    - opcode of reserved instruction
 #	old_pc(sp)    - pc of reserved instruction 
 #	operand_1(sp) - first operand specifier
 #	operand_2(sp) - second operand specifier
 #	operand_3(sp) - third operand specifier
 #	operand_4(sp) - fourth operand specifier
 #	operand_5(sp) - fifth operand specifier
 #	operand_6(sp) - sixth operand specifier
 #	operand_7(sp) - seventh operand specifier (currently unused)
 #	operand_8(sp) - eight operand specifier (currently unused)
 #	new_pc(sp)    - pc of instruction following reserved instruction
 #	exception_psl(sp) - psl at time of exception
 #
 # implicit input:
 #
 #	general registers contain architecturally specified values according
 #	to specific instruction that was emulated.
 #
 # implicit output:
 #
 #	control is passed to the location designated by "new pc" with the
 #	condition codes as determined by vax$xxxxxx. the exit routine also
 #	preserves general registers.
 #-

	.globl	vax$exit_emulator

vax$exit_emulator:
	movpsl	-(sp)			# save the new psl on the stack

 # note that the next instruction makes no assumptions about the condition 
 # codes in the saved psl. 

	insv	(sp)+,$0,$4,exception_psl(sp)	# replace saved condition codes
	addl2	$new_pc,sp		# adjust stack pointer (discard old pc)
	rei				# return


Lvax$ashp:		jmp	vax$ashp		# ashp entry
Lvax$cvtlp_restart:	jmp	vax$cvtlp_restart	# cvtlp entry
Lvax$cvtsp:		jmp	vax$cvtsp		# cvtsp entry
Lvax$crc:		jmp	vax$crc			# crc entry
Lvax$addp4:		jmp	vax$addp4	 	# addp4 entry
Lvax$addp6:		jmp	vax$addp6		# addp6 entry
Lvax$subp4:		jmp	vax$subp4		# subp4 entry
Lvax$subp6:		jmp	vax$subp6		# subp6 entry
Lvax$cvtpt_restart:	jmp	vax$cvtpt_restart	# cvtpt entry
Lvax$mulp:		jmp	vax$mulp		# mulp entry
Lvax$cvttp_restart:	jmp	vax$cvttp_restart	# cvttp entry
Lvax$divp:		jmp	vax$divp		# divp entry
Lvax$cmpc3:		jmp	vax$cmpc3		# cmpc3 entry
Lvax$scanc:		jmp	vax$scanc		# scanc entry
Lvax$spanc:		jmp	vax$spanc		# spanc entry
Lvax$cmpc5:		jmp	vax$cmpc5		# cmpc5 entry
Lvax$movtc:		jmp	vax$movtc		# movtc entry
Lvax$movtuc:		jmp	vax$movtuc		# movtuc entry
Lvax$movp:		jmp	vax$movp		# movp entry
Lvax$cmpp3:		jmp	vax$cmpp3		# cmpp3 entry
Lvax$cvtpl_restart:	jmp	vax$cvtpl_restart	# cmtpl entry
Lvax$cmpp4:		jmp	vax$cmpp4		# cmpp4 entry
Lvax$editpc_restart:	jmp	vax$editpc		# editpc entry
Lvax$matchc:		jmp	vax$matchc		# matchc entry
Lvax$locc:		jmp	vax$locc		# locc entry
Lvax$skpc:		jmp	vax$skpc		# skpc entry
