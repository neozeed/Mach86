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
/*	@(#)vaxexception.s	1.4		4/1/85		*/

#if	CMU
#include "mach_time.h"

#if	MACH_TIME > 0
#include "../vax/tmac.h"
#endif	MACH_TIME > 0
#endif	CMU

#include "../emul/vaxemul.h"
#include "../machine/psl.h"
#include "../emul/vaxregdef.h"
#include "../machine/mtpr.h"

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
/************************************************************************
 *
 *			Modification History
 *
 *	David L. Black at CMU, 7-Aug-85
 * dlb- Added code to do user mode timing on kernel exit, (copyargs and
 *	exe$srchandler)
 *
 *	Stephen Reilly, 25-Mar-85
 * 002- Fixed the problem in the exception handler that occurs during
 *	the emulation of a floationg point instruction which will cause the
 *	system to point to the emulated instruction rather than point into
 *	the emulation code that caused the exception.
 *
 *	Stephen Reilly, 11-Jul-84
 * 001- Changes were made so that in the event of a stack overflow the
 *	exception handling will die gracefully.
 *
 *	Stephen Reilly, 20-Mar-84
 * 000- This code is the modification of the VMS emulation code that 
 *	was written by Dave Cutler. This routine includes sysunwind.mar,
 *	exception.mar and exeadjstk.mar. The reason was so that all of the
 *      handling for the emulation code could be found in one module.
 *      These routines have been modified to run on Ultrix
 *
 ***********************************************************************/
 #
 # d. n. cutler 6-jul-76
 #
 # modified by:
 #
 #	v03-005 ly00b2		larry yetto		10-feb-1984 09:56
 #		fix truncation error
 #
 #	v03-005	ljk0260		lawrence j. kenah	5-feb-1984
 #		allow exception dispatching to take a detour through an
 #		instruction emulator so that the exception parameters can
 #		be modified to describe an exception at the site of the
 #		emulated instruction rather than inside the emulator.
 #		correct errors that appeared in comments.
 #
 #	v03-004	acg0393		andrew c. goldstein,	20-jan-1984  1:38
 #		fix fp validation in condition handler search# also
 #		call exe$unwind directly to avoid p1 vectors
 #
 #	v03-003	wmc0001		wayne cardoza		28-oct-1983
 #		change mode to user or supervisor handlers should not get 
 #		control in privileged modes.
 #
 #	v03-002	acg0348		andrew c. goldstein,	4-aug-1983  17:01
 #		fix unwinding to frame of exception
 #
 #	v03-001	acg0310		andrew c. goldstein,	31-jan-1983  13:44
 #		fix probing of stack after expansion
 #
 #	v02-016	msh0001		maryann hinden		09-feb-1982
 #		fix probe problem.
 #
 #	v02-015	acg0261		andrew c. goldstein,	4-feb-1982  13:50
 #		fix skipping over vectored handler invocations
 #		in nested exceptions.
 #
 #	v02-014	acg0260		andrew c. goldstein,	1-feb-1982  16:25
 #		make failure to create stack space non-fatal for user
 #		and super modes
 #
 #	v02-013	acg0242		andrew c. goldstein,	30-dec-1981  19:12
 #		catch exceptions at ast call site
 #
 #	v02-012	acg0224		andrew c. goldstein,	23-nov-1981  14:31
 #		fix access mode used to report ast faults
 #
 #	v02-011	acg0222		andrew c. goldstein,	17-nov-1981  16:52
 #		fix context bug in compatibility mode check
 #
 #	v02-010 dwt0003		david w. thiel		11-nov-1981
 #		allow t-bit traps during call to a condition handler.
 #		fix search's condition handler.
 #		define global entry (exe$sigtoret) for a condition handler
 #		that turns an exception into a return with status.
 #
 #	v02-009	kdm0064		kathleen d. morse	25-aug-1981
 #		change label of exception to exe$exception, for the use of
 #		the multi-processing code.
 #
 #	v02-008	ljk0036		lawrence j. kenah	23-jun-1981
 #		convert fatal exception for executive mode into a
 #		nonfatal bugcheck.
 #
 #	v02-007	kta0022		kerbey t. altmann	10-jun-1981
 #		add some code for inhibited system services. change
 #		stack range check to use new top limit array.
 #
 #	v02-006	kdm0037		kathleen d. morse	12-feb-1981
 #		change non-kernel mode references to sch$gl_curpcb
 #		to use ctl$gl_pcb instead.
 #
 #	v02-005	acg0183		andrew c. goldstein,	29-dec-1980  20:01
 #		fix condition handler search bugs, add entry point for
 #		lib$signal, add support for lib$stop, add checks to catch
 #		call failures to handlers.
 #
 #
 # hardware exception handling
 #
 #****************************************************************************
 #
 # fair warning!! the exception reflection and condition handling code in
 # this module crawls with assumptions about the format of the stack and
 # argument lists, as documented in various comments throughout. since
 # the stack pointer moves frequently, no attempt has been made to use
 # symbolic offsets for stack relative references. changes to the stack
 # format should be made only after thorough inspection and understanding
 # of the code (not to mention appendix c of the architecture handbook).
 # note also that lib$signal must track the stack formats used here.
 #
 #****************************************************************************
 #
 
 #
 # local symbols
 #
 # call frame offset definitions
 #
 
#define handler 0
					#condition handler address
#define savpsw 4
					#saved psw from call
#define savmsk 6
					#register save mask
#define savap 8
					#saved ap register image
#define savfp 12
					#saved fp register image
#define savpc 16
					#saved pc register image
#define savrg 20
					#other saved register images
 
 #
 # local data
 #
 

#define final_idx 0
					#indices to fetch message addresses
#define attconsto_idx 1
#define badhandler_idx 2
#define badast_idx 3
 #
 #+
 # exe$acviolat - access violate fault
 #
 # this routine is automatically vectored to when an access violation is
 # detected. the state of the stack on entry is:
 #
 #	00(sp) = access violation reason mask.
 #	04(sp) = access violation virtual address.
 #	08(sp) = exception pc.
 #	12(sp) = exception psl.
 #
 # access violation reason mask format is:
 #
 #	bit 0 = type of access violation.
 #		0 = pte access code did not permit intented access.
 #		1 = p0lr, p1lr, or s0lr length violation.
 #	bit 1 = pte reference.
 #		0 = specified virtual address not accessible.
 #		1 = associated page table entry not accessible.
 #	bit 2 = intended access type.
 #		0 = read.
 #		1 = modify.
 #
 # the exception name followed by the number of exception arguments are
 # pushed on the stack. final processing is accomplished in common code.
 #-
 
	.align	2

	.globl	exe$acviolat
 exe$acviolat:				#access violation faults

/*
 *	The following code emulates what is done with a SEGFLT in the
 *	module trap.c.  The reason for this is that if a length violation
 *	has occured we attempt to extend the stack. If we fail then continue
 *	where the code will had an actual stack overflow.  The only problem with
 *	this is the address the user sees causing the violation is not the
 *	emulated instruction, instead it is the instruction in the emulation
 *	code.
 */
	blbc	(sp),acviolat		#001 is this a lenght violation?
	cmpzv	$psl$v_curmod,$psl$s_curmod,12(sp),$psl$c_user # check for user 
 	bneq	acviolat		#001 no, really an access violation
	pushr	$0x03f			#001 save r0-r1 because C uses these
					# as temp registers and never saves 
					# them
	mfpr	$USP,-(sp)		#001 get user stack
	calls	$1,_grow 		#001 see if we can grow
	blbs	r0,1f			#001 br if we did grow
	pushl	(4+(6*4))(sp)		#001 get address that caused the
					# violation
	calls	$1,_grow		#001 try to grow
	blbc	r0,2f			#001 br if we fail
1: 	popr	$0x3f			#001 restore r0-r1
 	addl2	$8,sp			#clean exception parameters from stack
 	rei				#and return to retry instruction
2:	popr	$0x03f			#restore registers r0-r1
 
acviolat:				#
	movzwl	$ss$_accvio,-(sp)	#set exception name
ex5arg:	pushl	$5			#set number of signal arguments
	jmp	exe$exception		#finish in common code


 #+
 # exe$reflect - reflect exception from mode other than kernel
 #
 # this routine is jumped to reflect an exception from a mode other than kernel.
 # the signal arguments are assumed to be set up properly on the stack.
 # note that the previous mode field of the psl contains the access mode
 # of the exception.
 #-
 
	.globl	exe$reflect
exe$reflect:				#reflect exception
	pushl	$1			#save code indicating signal
 #	pushr	$^m<r0,r1>		#save registers r0 and r1
	 pushr	$0x03
	mnegl	$1,-(sp)		#002 set initial frame depth
	pushl	fp			#set initial handler establisher frame
	pushl	$4			#set number of mechanism arguments
 
 #
 # at this point the stack has the following format:
 #
 #	00(sp) = number of mechanism arguments (always 4).
 #	04(sp) = fp of handler establisher frame (tentative).
 #	08(sp) = frame depth (always -3).
 #	12(sp) = saved r0.
 #	16(sp) = saved r1.
 #	20(sp) = flags longword
 #	24(sp) = number of signal arguments.
 #	28(sp) = exception name (integer value).
 #	32(sp) = first exception parameter (if any).
 #	36(sp) = second exception parameter (if any).
 #	      .
 #	      .
 #	      .
 #	28+n*4(sp) = n'th exception parameter (if any).
 #	28+n*4+4(sp) = exception pc.
 #	28+n*4+8(sp) = exception psl.
 #
 
	movpsl	r0			#read current psl
	extzv	$psl$v_curmod,$psl$s_curmod,r0,r1 #current mode kernel?
	beql	3f			#if eql yes
	cmpzv	$psl$v_prvmod,$psl$s_prvmod,r0,r1 #is current eql previous?
	beql	4f			#if eql yes
 
 #
 # adjust previous mode stack pointer using system service
 #
 
 #	pushr	$^m<r2,r3,r4>		#save registers r2, r3, and r4
	 pushr	$0x01c
	addl3	$7,36(sp),r3		#calculate number of longwords to move
	pushab	normal			#assume information can be copied
	clrl	-(sp)			#set to use current stack pointer value
	pushab	(sp)			#push address to store updated stack value
	ashl	$2,r3,-(sp)		#calculate stack adjustment value
	mnegl	(sp),(sp)		#set negative adjustment value
	extzv	$psl$v_prvmod,$psl$s_prvmod,r0,-(sp) #push access mode of stack
	pushl	$3			#push number of arguments
	callg	(sp),*$sys$adjstk	#adjust previous mode stack pointer
	blbs	r0,2f			#if lbs successful completion

	addl2	$((6*4)+(3*4)),sp	#001 strip of the stuff associated
					#001 with the sys$adjstk call
	movab	28(sp),sp		#001 set sp to the exception name
	jmp	start_handling		#001 handle the exception	

/*
 *1:	movab	w^badstack,20(sp)	#set bad stack return
 *	movl	4(sp),r2		#retrieve previous access mode
 *	movl	*$ctl$al_stack[r2],16(sp) #set to use specifed stack pointer value
 *	callg	(sp),sys$adjstk		#reload previous mode stack pointer
 *	blbs	r0,20$			#if lbs successful completion
 *	subl3	$4,*$ctl$al_stack[r2],-(sp) #calculate top address of stack range
 *	addl3	12(sp),(sp),-(sp)	#calculate bottom address of stack range
 *	bsbw	crestack		#re-create virtual space under stack
 *	addl	$8,sp			#remove virtual address descriptor
 *	brb	10$			#and try again
 */

2:	addl2	$16,sp			#remove argument list from stack
 #	popr	$^m<r1>			#get new previous mode stack pointer value
	 popr	$0x02
	brw	copyargs		#
3:	jmp	reflect			#
4:	brw	normal			#


 #
 #
 # all exceptions converge to this point with:
 #
 #	00(sp) = number of signal arguments.
 #	04(sp) = exception name (integer value).
 #	08(sp) = first exception parameter (if any).
 #	12(sp) = second exception parameter (if any).
 #	      .
 #	      .
 #	      .
 #	04+n*4(sp) = n'th exception parameter (if any).
 #	04+n*4+4(sp) = exception pc.
 #	04+n*4+8(sp) = exception psl.
 #
 # note that the previous mode field of the psl contains the access mode
 # of the exception.
 #
 
		.globl	exe$exception
exe$exception:				#this label must be global for mp code
	pushl	$1			#set code indicating signal
 #	pushr	$^m<r0,r1>		#save registers r0 and r1
	 pushr	$0x03
	mnegl	$3,-(sp)		#set initial frame depth
	pushl	fp			#set initial handler establisher frame
	pushl	$4			#set number of mechanism arguments
 
 #
 # at this point the stack has the following format:
 #
 #	00(sp) = number of mechanism arguments (always 4).
 #	04(sp) = fp of handler establisher frame (tentative).
 #	08(sp) = frame depth (always -3).
 #	12(sp) = saved r0.
 #	16(sp) = saved r1.
 #	20(sp) = flags longword
 #	24(sp) = number of signal arguments.
 #	28(sp) = exception name (integer value).
 #	32(sp) = first exception parameter (if any).
 #	36(sp) = second exception parameter (if any).
 #	      .
 #	      .
 #	      .
 #	28+n*4(sp) = n'th exception parameter (if any).
 #	28+n*4+4(sp) = exception pc.
 #	28+n*4+8(sp) = exception psl.
 #
 
reflect:				#reflect exception to proper access mode
/*	addl3	$6,24(sp),r0		#calculate longword offset to saved psl
 *	assume	psl$v_cm eq 31
 *	tstl	(sp)[r0]		#previously in compatibility mode?
 *	bgeq 	10$			#branch if not
 *	moval	*$ctl$al_cmcntx,r1	#get address of compatibility context area
 *	movq	12(sp),(r1)+		#save r0 and r1
 *	movq	r2,(r1)+		#save r2 and r3
 *	movq	r4,(r1)+		#save r4 and r5
 *	movl	r6,(r1)+		#save r6
 *	movzbl	$8,(r1)+		#set cm exception type
 *	movl	-4(sp)[r0],(r1)+	#save pc
 *	movl	(sp)[r0],(r1)		#save psl
 */
1:	movpsl	r1			#read current psl
/*
 *	mfpr	$pr$_ipl,r0		#read current ipl
 *	cmpl	$ipl$_astdel,r0		#invalid priority level?
 *	blss	20$			#if lss yes
 *	bbc	$psl$v_is,r1,30$	#if clr, then not on interrupt stack
 *20$:	bug_check invexceptn,fatal	#invalid exception
 *30$:	ifnord	$4,*$ctl$al_stack,20$	#is there a control region?
 */					# (not swapper,nullproc)
	extzv	$psl$v_prvmod,$psl$s_prvmod,r1,r0 #extract previous mode field
	bneq	8f			#if eql previous mode was kernel
	jmp	normal			#
8:	jmp	9f			#goto paged code
9:
 #	pushr	$^m<r2,r3,r4>		#save registers r2, r3, r4
	 pushr	$0x01c
	pushab	normal			#assume information can be copied
	movl	r0,r2			#save previous mode
	addl3	$7,40(sp),r3		#calculate number of longwords to move
	mfpr	r2,r1			#read previous mode stack pointer

 8:	bsbw	check_stack		#check stack range
	bneq	4f			#continue if stack ok
	cmpl	$psl$c_user,r2		#is it user mode stack fault?
 	bneq	5f			#no, then bad stack
 # 	pushr	$^m<r1,r2,r3,r4,r5>	#save registers
	 pushr	$0x03e
 # 	movl	r0,r2			#get lowest stack address from check_st *ack
	pushl	r0			#001 push address to grow
	calls	$1,_grow		#001 try to grow
 # 	bsbw	exe$expandstk		#and call to expand stack
 # 	popr	$^m<r1,r2,r3,r4,r5>	#restore registers
	 popr	$0x3e
 	blbc	r0,5f			#br if any error to declare bad stack
 	brb	8b			#check the stack again
 					#note- check_stack prevents us from loping
5:	movab	44(sp),sp		#001 pop everthing except the exception
					# stuff
	jmp	start_handling		#001 go to general handler

4:
/*	cmpl	r1,*$ctl$al_stack[r2]	#top address of stack in range?
 *	bgtru	50$			#if gtru no
 */
	cmpl	$psl$c_user,r2		#previous mode user?
	beql	7f			#if eql yes
/*	cmpl	r0,*$ctl$al_stacklim[r2]#bottom address of stack in range?
 *	bgequ	70$			#if gequ yes
 *50$:	movab	w^badstack,(sp)		#set bad stack return
 *	movl	*$ctl$al_stack[r2],r1	#get starting address of previous mode stack
 *	bsbw	check_stack		#check stack range
 *	bneq	70$			#if neq successful range check
 *	pushal	-(r1)			#push top address of stack range
 *	movab	-1024+4(r1),-(sp)	#push bottom address of stack range
 *	bsbb	crestack		#re-create stack space
 *	addl	$8,sp			#remove virtual address limits from stack
 *	movl	*$ctl$al_stack[r2],r1	#get starting address of previous mode stack
 */
7:	ashl	$2,r3,r0		#calculate number of bytes to move
	subl2	r0,r1			#calculate new top of stack address
	mtpr	r1,r2			#set new previous mode stack pointer
	brw	copyargs		#

 #
 #
 # push argument list on stack
 #
 
normal:					#normal exit from stack copy
	pushal	(sp)			#push address of mechanism arguments
	pushal	28(sp)			#push address of signal arguments
	pushl	$2			#push number of arguments
 
 # check if this exception should be modified by an instruction emulator

 #	movl	exe$gl_vaxexcvec,r1	#modification routine supplied?
 #	beql	exe$srchandler		#branch if none

/*
 *	No need to check because we only come throught here if we have
 *	an error in the Ultrix emulation code. So jump to the code that
 *	will handle the emulation signals
 */
	 jsb	vax$modify_exception
 

 #
 # check the pc of the exception. if it is in the condition handler call
 # vector, then an exception has occurred attempting to call a condition
 # handler (e.g., due to bad address or entry mask). if this is the case,
 # exit the image to avoid an exception loop. we do the same for calls
 # to ast routines. while this is not strictly a bad stack, reporting the
 # exception with the ast context recorded in the stack is such a pain
 # that it is not worth it.  the special case of a t-bit pending exception
 # is allowed since this case cannot result in a loop.
 #
		.globl	exe$srchandler

exe$srchandler:				#entry point for external use
1:	movb	36(sp),35(sp)		#save signal vector length
	cmpl	40(sp),$ss$_tbit	#check for t-bit pending exception
	beql	1f			#branch if yes - skip checks
	addl3	$8,36(sp),r1		#compute longword offset to saved pc
/*	cmpl	(sp)[r1],$sys$call_handl #compare to handler call site
 *	beql	bad_handler		#branch if yes
 *	cmpl	(sp)[r1],$exe$astdel	#compare to ast call site
 *	beql	bad_ast			#branch if yes
 */

 #
 # search for condition handler
 #
1:	callg	(sp),search		#search for condition handler
	blbc	r0,2f			#if lbc fatal error
 
	bbc	$1,32(sp),6f		#branch if this is not a stop
/*	insv	$sts$k_severe,$sts$v_severity,$sts$s_severity,40(sp)
 * 					#for stop, force severity to fatal
 */
 #
 # call condition handler
 #
 
6:	jsb	*$sys$call_handl	#call handler via system vector
	blbc	r0,1b			#if lbc resignal
/*	bbs	$1,32(sp),cont_from_stop #branch if attempting to continue fro stop
 */
	movzbl	35(sp),(sp)		#get original signal arg count
	addl2	$8,(sp)			#calculate longword offset to saved pc
	mull2	$4,(sp)			#calculate number of bytes to remove
	movq	24(sp),r0		#restore r0 and r1
	addl2	(sp),sp			#remove argument list from stack
#if	MACH_TIME > 0
	TSREI				#
#else
	rei
#endif
 
 #
 # to here on attempt to continue form a call to stop
 #
/*
 *cont_from_stop:				#set final status and message
 *	movl	$<lib$_attconsto&^csts$m_severity>!attconsto_idx,r0
 *	cmpl	20(sp),$-3		#see if just called last chance handler
 *	brb	7f			#and flow into exit code
 *
 *#
 *# to here if an exception occurred attempting to call a handler
 *#
 *bad_handler:
 *	movl	40(sp),r0		#set condition as final status
 *					#set message string
 *	insv	$badhandler_idx,$sts$v_severity,$sts$s_severity,r0
 *	cmpl	32(sp)[r1],$-3		#see if trying to call last chance handler
 *7:	bneq	2f			#if not, go to call it
 *	movl	r0,32(sp)		#save condition and message
 *	brb	3f			#if yes, don't call it again
 *
 *#
 *# to here if an exception occurred attempting to call an ast
 *#
 *bad_ast:
 *	movl	40(sp),r0		#set condition as final status
 *					#set message string
 *	insv	$badast_idx,$sts$v_severity,$sts$s_severity,r0
 *	brb	20$
 *#
 *# bad stack when trying to copy exception arguments
 *#
 *
 *badstack:				#bad stack exit from stack copy
 *	pushal	(sp)			#push address of mechanism arguments
 *	pushal	28(sp)			#push address of signal arguments
 *	pushl	$2			#push number of arguments
 *					#set bad stack status
 *	movzwl	$<ss$_badstack&^csts$m_severity>!final_idx,r0
 */
2:	movl	r0,32(sp)		#save final status and message
/*	$setsfm_s	$0		# clear sys. service failure excep. mode
 */
	movpsl	r0			#read current psl
	extzv	$psl$v_curmod,$psl$s_curmod,r0,r0 #extract current mode
/*	movl	*$ctl$al_finalexc[r0],r1 #get address of last chance handler
 */
	movl	$default_handler,r1	# store the Ultrix default handler
/*	beql	3f			#if eql none
 */
	mnegl	$3,20(sp)		#set frame depth to minus three
	jsb	*$sys$call_handl	#call last chance condition handler
/*	brb	8f
 *
 *
 *3:	movpsl	r0			#read current psl
 *	extzv	$psl$v_curmod,$psl$s_curmod,r0,r0 #extract current mode
 *	cmpl	$psl$c_exec,r0		#executive or kernel mode?
 *	bgequ	9f			#if gequ yes
 *8:	extzv	$sts$v_severity,$sts$s_severity,32(sp),r0 #get message index
 *	tstb	*$ctl$gb_ssfilter	#are system services inhibited?
 *	bneq	4f			#yes, don't try to print anything
 *	pushab	(sp)			#push address of condition argument list
 *	pushl	msg_vector[r0]		#push address of final exception message
 *	calls	$2,exe$excmsg		#print final exception message
 *4:	movl	32(sp),r0		#retrieve final status
 *	insv	$sts$k_severe,$sts$v_severity,$sts$s_severity,r0
 *	$exit_s	r0			#exit process
 *
 *9:	bgtru	5f			#branch if kernel mode
 *	bug_check fatalexcpt		#fatal executive mode exception
 *	brb	4b			#go delete the process	
 *
 *5:	bug_check fatalexcpt,fatal	#fatal kernel mode exception
 */

 #
 # copy arguments to previous mode stack and exit to previous mode
 #
 
copyargs:				#copy argument lists to previous mode stack
	movab	16(sp),r0		#get address of arguments to copy
1:	movl	(r0)+,(r1)+		#copy exception arguments to previous stack
	sobgtr	r3,1b			#any more longwords to copy?
 #	bicl2	$psl$m_cm|psl$m_tbit|psl$m_fpd|psl$m_tp,-(r0)
 	bicl2	$0xC8000010,-(r0)	#clear compatibility, t-bit, t pending,and
					#first part done
 #	popl	-(r0)			#set return address
	 movl	(sp)+,-(r0)
 #	popr	$^m<r2,r3,r4>		#restore registers r2, r3, r4
	 popr	$0x1c
	movl	r0,sp			#remove arguments from kernal stack
#if 	MACH_TIME > 0
	TSREI				#
#else
	rei
#endif

 #
 # subroutine to check accessibility of stack address range
 #
 # inputs:
 #	r1 - stack pointer
 #	r3 - partial longword count
 #
 # outputs:
 #	r0 - bottom address of range
 #	z condition code - 0 if accessible, else 1	
 #
 #	r1,r2,r3 are preserved.
 #	

check_stack:				#check stack address range
 #	pushr	$^m<r1,r2,r3>		#save registers
	 pushr	$0x0e
	addl3	$3+1+17,r3,r1		#calculate total longwords in range
	mull2	$4,r1			#calculate number of bytes in range
	subl3	r1,(sp),r0		#calculate bottom address of range
	pushl	r0			#save this quantity
	clrl	r3			#access mode to maximize with psl<prvmod>
 #	jsb	exe$probew		#check write access
 	probew	r3,r1,(r0)		#001 check write access
	beql	1f			#001 br if we can't
	movl	$1,r0			#001 indicate success
	brb	2f			#001
1:	clrl	r0 			#001 indicate failure
2:	bitl	$1,r0			#set condition code
 #	popr	$^m<r0,r1,r2,r3>	#restore registers (note: cond. codes
 	 popr	$0x0f
 					# preserved
 	rsb				#return
 

 #
 # search - search for condition handler
 #
 # this is a special internal routine that is called in the initial search
 # for a condition handler and on resignal from a previously signalled
 # condition.
 #
 # inputs:
 #
 #	00(ap) = number of condition arguments.
 #	04(ap) = address of signal argument list.
 #	08(ap) = address of mechanism argument list.
 #	12(ap) = number of mechanism arguments.
 #	16(ap) = fp of handler establisher frame.
 #	20(ap) = frame depth.
 #	24(ap) = saved r0.
 #	28(ap) = saved r1.
 #	32(ap) = flags longword
 #	36(ap) = number of signal arguments.
 #	40(ap) = exception name (integer value).
 #	44(ap) = first exception parameter (if any).
 #	48(ap) = second exception parameter (if any).
 #	      .
 #	      .
 #	      .
 #	40+n*4(ap) = n'th exception parameter (if any).
 #	40+n*4+4(ap) = exception pc.
 #	40+n*4+8(ap) = exception psl.
 #
 # outputs:
 #
 #	r0 low bit clear indicates failure to locate condition handler.
 #
 #		r0 = ss$_accvio - stack cannot be read from current mode.
 #
 #		r0 = ss$_nohandler - no condition handler could be found.
 #
 #	r0 low bit set indicates successful completion.
 #
 #		r1 = address of condition handler.
 #
 
search:					#search for condition handler
	.word	0			#entry mask
/*	movab	exe$sigtoret,(fp)	#set address of condition handler
 */
1:	movl	16(ap),r0		#get previous frame address
2:	movpsl	r1			#read current psl
	extzv	$psl$v_curmod,$psl$s_curmod,r1,r1 #extract current mode
	incl	20(ap)			#increment frame depth
	beql	5f			#if eql first stack frame
	bgtr	4f			#if gtr other stack frame
/*	movaq	*$ctl$aq_excvec[r1],r0	#get address of exception vector quadword
 *	cmpl	$-2,20(ap)		#examine primary vector?
 *	beql	3f			#if eql yes
 *	tstl	(r0)+			#adjust to secondary vector
 *3:	movl	(r0),r1			#get address of condition handler
 *	bneq	6f			#if neq condition handler found
 *	brb	1b			#
 */
4:	blbs	22(ap),0f		#if lbs search count overflow
L5:	movpsl	r1			#read current psl
	extzv	$psl$v_curmod,$psl$s_curmod,r1,r1 #extract current mode
					#range check fp to make sure we are in
					#the right stack. this is crucial, since
					#there is no other mechanism to prevent
					#following the fp linkage into call
					#frames belonging to an outer mode.
/*	cmpl	r0,*$ctl$al_stack[r1]	#frame pointer within stack range?
	bgtru	0f			#if gtru no
 */
	cmpl	$psl$c_user,r1		#if in user mode
	beql	L6			#skip top range check
/*	cmpl	r0,*$ctl$al_stacklim[r1]#frame pointer within stack range?
	blssu	0f			#if lssu no
 */
L6:	cmpl	$sys$call_handl+4,savpc(r0) #call from condition dispatcher?
	beql	7f			#branch if yes - must skip frames
	movl	savfp(r0),r0		#get address of previous frame
	beql	0f			#if eql none
L8:	movl	r0,16(ap)		#save address of establisher frame
5:	
 #	bsbb	check_fp		#check if this frame is valid
	movl	(r0),r1			#get address of condition handler
	beql	1b			#if eql none
6:	bisl2	$1,r0			#indicate successful completion
	ret				#

7:	extzv	$0,$12,savmsk(r0),r1	#get register save mask
	extzv	$14,$2,savmsk(r0),-(sp)	#get stack alignment bias
	addl2	$savrg,r0		#add offset to register save area
	addl2	(sp)+,r0		#add stack alignment bias
8:	blbc	r1,9f			#if lbc corresponding register not saved
	addl2	$4,r0			#adjust for saved register
9:	ashl	$-1,r1,r1		#any more registers saved?
	bneq	8b			#if neq yes
	movl	chf$l_mcharglst+4(r0),r1 #get address of mechanism arguments
	movl	chf$l_mch_frame(r1),r0	#get address of establisher frame
	tstl	chf$l_mch_depth(r1)	#check if this is a vectored handler
	blss	L8			#if so, don't skip "establisher"
	brb	L5			#

0:	movzwl	$ss$_nohandler,r0	#set no handler found
	ret				#

 #
 # subroutine to validate the current frame address. this is done with
 # a range check against the stack limit registers in the p1 vector page.
 # since fp linkages extend across access modes, there is no other check
 # possible to prevent chasing an inner mode exception out to an outer
 # access mode.
 #
/*check_fp:
 *	movpsl	r1			#read current psl
 *	extzv	$psl$v_curmod,$psl$s_curmod,r1,r1 #extract current mode
 *	cmpl	r0,*$ctl$al_stack[r1]	#frame pointer within stack range?
 *	bgtru	100$			#if gtru no
 *	cmpl	$psl$c_user,r1		#if in user mode
 *	beql	95$			#skip top range check
 *	cmpl	r0,*$ctl$al_stacklim[r1]#frame pointer within stack range?
 *	blssu	100$			#if lssu no
 *95$:	rsb				#stack frame ok
 *100$:	movzwl	$ss$_nohandler,r0	#set no handler found
 *	ret				#
 */

 #
		.text

 #
 # d. n. cutler 16-dec-76
 #
 # modified by:
 #
 #	v02-006	acg0261		andrew c. goldstein,	4-feb-1982  14:11
 #		fix skipping over vectored handler invocations
 #		in nested exceptions.
 #
 #	v02-005	acg0252		andrew c. goldstein,	11-jan-1982  17:02
 #		fix return status when newpc is specified
 #
 #	v02-004	acg0242		andrew c. goldstein,	16-dec-1981  18:21
 #		fix unwinding to caller of establisher in nested exceptions,
 #		allow unwinding out of ast's.
 #
 #	v02-003 dwt0002		david w. thiel		10-nov-1981  11:10
 #		remove sys_call_handl+5 rsb.
 #		use common condition handler.
 #
 #	v02-002	acg0183		andrew c. goldstein,	31-dec-1980  11:23
 #		fix bug in unwinding to establisher frame
 #
 #**
 #
 # system service unwind procedure call stack
 #
 # macro library calls
 #
/*
 *	$chfdef				#define condition handling arglist offsets
 *	$ssdef				#define system status values
 */
 
 #
 # local symbols
 #
 # argument list offset definitions
 #
 
# define depadr 4
					#address of number of frames to unwind
# define newpc 8
					#change of flow final return address
 

 #+
 # exe$unwind - unwind procedure call stack
 #
 # this service provides the capability to unwind the procedure call stack
 # to a specified depth after a hardware- or software-detected exception
 # condition has been signalled. optionally a change of flow return address
 # may also be specified. the actual unwind is not performed immediately by
 # the service, but rather the return addresses in the call stack are modified
 # such that when the condition handler returns the unwind occurs.
 #
 # inputs:
 #
 #	depadr(ap) = address of number of frames to unwind.
 #	newpc(ap) = change of flow final return address.
 #
 #	r4 = current process pcb address.
 #
 # outputs:
 #
 #	r0 low bit clear indicates failure to fully unwind call stack.
 #
 #		r0 = ss$_accvio - call stack not accessible to calling access
 #			mode.
 #
 #		r0 = ss$_insframe - insufficient call frames to unwind to
 #			specified depth.
 #
 #		r0 = ss$_nosignal - no signal is currently active to unwind.
 #
 #		r0 = ss$_unwinding - unwind already in progress.
 #
 #	r0 low bit set indicates successful completion.
 #
 #		r0 = ss$_normal - normal completion.
 #-
 
		.text
 #	.entry	exe$unwind,^m<r2,r3,r4,r5>
		.globl	exe$unwind
exe$unwind:
	.word	0x03c
/*	movab	exe$sigtoret,(fp)	#establish condition handler
 */
	movl	fp,r4			#set address of first frame to examine
 
 #
 # search call stack for a frame that was created by a call from the signal
 # dispatch vector or by a call from the unwind signal dispatcher.
 #
 
	movzwl	$ss$_nosignal,r0	#assume no signal active
1:	movl	savfp(r4),r4		#get address of previous frame
	beql	2f			#if eql end of call stack
	cmpl	$sys$call_handl+4,savpc(r4) #call from condition handler dispatcher?
	beql	3f			#if eql yes
	cmpl	$callunwind+4,savpc(r4)	#call from unwind signal dispatcher?
	bneq	1b			#if neq no
	movzwl	$ss$_unwinding,r0	#set already unwinding
2:	ret				#
 
 #
 # set to unwind procedure call stack to specified depth
 #
 
3:	movl	depadr(ap),r3		#get address of number of frames to unwind
	beql	4f			#if eql none specified
	movl	(r3),r3			#get number of frames to unwind
	brb	5f			#
4:	movl	r4,r2			#copy current frame address
	bsbw	oldsp			#calculate value of sp before call
	movl	chf$l_mcharglst+4(r2),r2 #get address of mechanism argument list
	addl3	$1,chf$l_mch_depth(r2),r3 #calculate depth of establisher's caller
5:	bleq	9f			#if leq no frames to remove
	moval	startunwind,r0		#set condition handler unwind address
	bsbb	setpc			#
 
 #
 # scan through specified number of frames setting each frame to unwind on return
 #
 
6:	movl	savfp(r4),r4		#get address of previous frame
	beql	0f			#if eql insufficient frames
	decl	r3			#any more frames to consider?
	bgtr	L10			#branch if yes
	tstl	depadr(ap)		#are we unwinding to caller of establisher?
	beql	9f			#branch if yes - don't touch handler frames
L10:	cmpl	$sys$call_handl+4,savpc(r4) #call from condition dispatcher?
	bneq	8f			#if neq no
	movl	r4,r2			#copy address of current frame
	bsbw	oldsp			#calculate value of sp before call
	movl	chf$l_mcharglst+4(r2),r2 #get address of mechanism argument list
	tstl	chf$l_mch_depth(r2)	#check if this is a vectored handler
	blss	8f			#if so, don't skip any frames
7:	cmpl	r4,chf$l_mch_frame(r2)	#unwound to establisher frame?
	beql	L10			#if eql yes
	moval	loopunwind,savpc(r4)	#set frame unwind address
	movl	savfp(r4),r4		#get address of previous frame
	brb	7b			#

8:	tstl	r3			#any more frames to consider?
	bleq	9f			#if leq no
	moval	loopunwind,r0		#set frame unwind address
	bsbb	setpc			#
	brb	6b			#
 
 #
 # modify change of flow return if new address specified
 #
 
9:	movl	newpc(ap),r0		#get change of flow return address
	beql	L15			#if eql none specified
	bsbb	setpc			#set new final return address
L15:	movzwl	$ss$_normal,r0		#set normal completion
	ret				#
0:	movzwl	$ss$_insframe,r0	#set insufficient frames
	ret				#
 
 #
 # subroutine to store unwind pc. it checks if the frame being altered
 # is an ast call frame. rather than plug its return pc, we let
 # it return to the ast dispatcher, who will dismiss the ast. instead,
 # we plug the interrupt pc of the ast, so the rei goes back to
 # loopunwind to continue with the ast dismissed.
 #

setpc:
/*	cmpl	savpc(r4),$exe$astret	#check if frame is an ast
 *	beql	1f			#branch if yes
 */
	movl	r0,savpc(r4)		#set frame unwind address
	rsb
/*
 *1:	movl	r4,r2
 *	bsbw	oldsp			#find the start of the ast arg list
 *	moval	loopunwind,16(r2)	#and stuff the ast pc
 *	bicl2	$psl$m_cm|psl$m_fpd,20(r2) #clean out cm and fpd bits
 *	rsb
 */

 #
 # unwind handler frame
 #
 
startunwind:				#start of actual unwind
	movl	chf$l_mcharglst+4(sp),r0 #get address of mechanism argument list
	movq	chf$l_mch_savr0(r0),r0	#restore registers r0 and r1
 
 #
 # unwind call frame signaling condition handler if one is specified
 #
 
loopunwind:				#unwind call frame
	tstl	(fp)			#condition handler specified?
	beql	L20			#if eql no
	movzwl	$ss$_unwind,-(sp)	#push unwind signal condition
	pushl	$1			#push number of signal arguments
 #	pushr	$^m<r0,r1>		#push registers r0 and r1
	 pushr	$0x03
	pushl	$0			#push frame depth
	pushl	fp			#push frame address
	pushl	$4			#push number of mechanism arguments
	pushal	(sp)			#push address of mechanism arguments
	pushal	24(sp)			#push address of signal arguments
callunwind:				#signal unwind
	calls	$2,*(fp)		#call condition handler
	movq	chf$l_mch_savr0(sp),r0	#retrieve new values for r0 and r1
L20:	moval	savrg(fp),ap		#get address of register save area
	blbc	savmsk(fp),2f		#if lbc r0 not saved
	movl	r0,(ap)+		#save r0 for subsequent restoration
2:	bbc	$1,savmsk(fp),4f	#if clr, r1 not saved
	movl	r1,(ap)			#save r1 for subsequent restoration
/*
 *3:	cmpl	savpc(fp),$exe$astret	#about to unwind an ast?
 *	bneq	4f			#branch if not
 *	pushr	$0x06			#save r1 and r2
 *	movl	fp,r2
 *	bsbb	oldsp			#find the ast parameter list
 *#	popl	r1			#get back r1
 *	 movl	r1,(sp)+
 *	movq	r0,8(r2)		#stuff r0 and r1 so they will pass through
 *#	popl	r2			#restore r2
 *	 movl	r2,(sp)+
 */

4:	ret				#

 #
 # subroutine to calculate value of sp before call
 #
 
oldsp:	extzv	$14,$2,savmsk(r2),-(sp)	#get stack alignment bias
	extzv	$0,$12,savmsk(r2),r1	#get register save mask
	addl2	$savrg,r2		#add offset to register save area
	addl2	(sp)+,r2		#add stack alignment bias
1:	blbc	r1,2f			#if lbc corresponding register not saved
	addl2	$4,r2			#adjust for saved register
2:	ashl	$-1,r1,r1		#any more registers saved?
	bneq	1b			#if neq yes
	rsb				#


 #
/*
 *
 *	This routine is used call any of the conditional handler during
 *	the process of unwinding
 */
	.globl	sys$call_handl

sys$call_handl:
	callg	4(sp),(r1)
	rsb


 
#define acmode 4
				# access mode to adjust stack pointer for
#define adjust 8
				# 16-bit signed adjustment value
#define newadr 12
				# address of longword to store updated value
 #+
 # sys$adjstk - adjust outer mode stack pointer
 #
 # this service provides the capability to adjust the stack pointer for
 # a mode that is less privileged than the calling access mode. it can be
 # used to load an initial value into the specified mode's stack pointer or
 # to adjust its current value.
 #
 # inputs:
 #
 #		acmode(ap) = access mode to adjust stack pointer for.
 #		adjust(ap) = 16-bit signed adjustment value.
 #		newadr(ap) = address of longword to store updated value.
 #			if the initial contents of @newadr(ap) are nonzero,
 #			then the value is taken as the current top of stack.
 #			else the current stack pointer for the specified mode
 #			is used.
 #
 # outputs:
 #
 #	r0 low bit clear indicates failure to adjust stack pointer.
 #
 #		r0 = ss$_accvio - longword to store updated stack pointer
 #			or part of new stack segment cannot be written by
 #			calling access mode.
 #
 #		r0 = ss$_nopriv - specifed access mode is equal or more
 #			privileged than calling access mode.
 #
 #	r0 low bit set indicates successful completion.
 #
 #		r0 = ss$_normal - normal completion.
 #-
 
 #	.entry	exe$adjstk,^m<r2,r3,r4,r5,r6>
sys$adjstk:
	.word	0x07c
	movl	newadr(ap),r5		#get address to store new stack value
	extzv	$0,$2,acmode(ap),r3	#get access mode to modify stack pointer for
	movpsl	r2			#read current psl
	cmpzv	$psl$v_prvmod,$psl$s_prvmod,r2,r3 #previous mode more privileged?
	bgeq	6f			#if geq no
 #	ifnowrt	#4,(r5),40$		#can new stack value be written?
	 probew	$0,$4,(r5)		#001 can new stack value be written?
	 beql	4f			#001 br if it can't

	movl	(r5),r6			#get specified stack value
	bneq	1f			#if neq value specified
	mfpr	r3,r6			#
1:	cvtwl	adjust(ap),r0		#get adjustment value
	addl2	r0,r6			#calculate new top of stack
	mnegl	r0,r0			#allocation of stack space?
	bleq	3f			#if leq no
	movl	r6,r1			#copy new stack value
	cvtwl	$-0x200,r2		#set addition constant
2:
 #	ifnowrt	r0,(r1),40$,r3		#can allocated stack segment be written?
	 probew	r3,r0,(r1)		#001 can new stack value be written
	 beql	4f			#001 br if it can't

	subl2	r2,r1			#update address in stack
	movaw	(r0)[r2],r0		#update remaining length
	bgeq	2b			#if geq more to check
3:	mtpr	r6,r3			#
	movl	r6,(r5)			#store new stack value
	movzwl	$ss$_normal,r0		#set normal completion
	ret				#
4:	cmpl	$psl$c_user,r3		#is this for user mode stack?
	bneq	5f			#br if not
 #	pushr	#^m<r1,r2,r3,r4,r5>	#save registers
	 pushr	$0x03e
 #	movl	r1,r2			#stack base address
	pushl	r1			#001 push address to grow
	calls	$1,_grow		#001 try to grow
 #	bsbw	exe$expandstk		#augment stack to make accessible
 #	popr	#^m<r1,r2,r3,r4,r5>	#restore registers
	 popr	$0x03e
 	blbs	r0,1f			#repeat checks
 	ret				#return error code
 
5:	movzwl	$ss$_accvio,r0		#set access violation
	ret				#
6:	movzwl	$ss$_nopriv,r0		#set no privilege
	ret				#

 #
/*
 *
 *	This routine is called when there are no handlers set. This should
 *	always be true since Ultrix does not use the conditional
 *	handler mechanism.  The only case this is not true is in the
 *	float point emulation code where it does set a conditional handler.
 *	
 */

default_handler:
	.word	0
	pushab	handle_exception	# push the default handler
	pushl	$0			# set number of frames to unwind
	calls	$2,exe$unwind		# call the system unwind routine
	movl	$ss$_normal,r0		# indicate normal
	ret

 #
/*
 *	This routine will interface the VMS exception handling that is
 *	used by the emulation package to the Ultrix signal facility
 *
 * Input:
 *	(sp) - return address of sys$call_handl
 *    04(sp) - number of condtion arg
 *    08(sp) - addr of signal arg list
 *    12(sp) - addr of mechanism arf list
 *    16(sp) - number of mechanism arg.
 *    20(sp) - fp of handler establisher frame
 *    24(sp) - frame depth
 *    28(sp) - saved r0
 *    32(sp) - saved r1
 *    36(sp) - flags longword
 *    40(sp) - number of signal args
 *    44(sp) - exception name
 *    48(sp) - first exception parameter (if any )
 *    52(sp) - second exception parameter (if any)
 *	 .
 *	 .
 *	 .
 *	(sp) - exception pc
 *	(sp) - exception psl
 */ 

handle_exception:

/*
 *	Get everthing off the stack up to the number of signal args
 */
	tstl	(sp)+			# pop return address
	movq	24(sp),r0		# restore r0 and r1
	movab	36(sp),sp		# adjust to the signal number

/*
 *		(sp) - number of signal args
 *   	     N*4(sp) - exception name
 *       	 .   - exception specific information
 *	 	 .
 *  	   N*4+4(sp) - exception pc
 *	   N*4+8(sp) - exception psl
 */

	
	movpsl	-(sp)					# get the current psl
	extzv	$psl$v_curmod,$psl$s_curmod,(sp),(sp)+	# are we in kernel mode
	beql	kern_handling				# br if yes

/*
 *	We are in user mode but to do any of the ultrix signaling
 *	we need to get into kernel mode so that we can use the Utlrix
 *	signal facility.
 *
 * 	The halt instruction is used to change our
 *	mode to kernel so that we can use the signal code. This works
 *	because in the privilege instruction code, in locore.s, will
 *	check the PC of the exception. If the exception address
 *	equals vax$special_halt it will jump to vax$special_cont,
 *	otherwise it will proceed normally.
 */
	.globl	vax$special_halt

vax$special_halt:
        halt
	
/*
 *	This routine is called if the pervious mode and the current mode
 *	of the psl are not the same. The routine purpose is to copy
 *	the exception information from the previous mode stack to the
 *	kernel stack.
 */
	.globl	vax$special_cont
vax$special_cont:
	
/*
 *	At most we need type 5 location for the exception information and
 *	one location for the address that points to the stuff that was copied
 *      to the kernel stack
 */
	movab	-6*4(sp),sp			# at most  new entries
						# are going on the stack
	pushr	$0x0f				# save r0 r1 r2 r3

/*
 *	Stack looks as follows
 *
 *	0(sp)	saved r3
 *	4(sp)	saved r2
 *	8(sp)	saved r1
 *     12(sp)	saved r0
 *     16(sp)	address of the copied exception information
 *     20(sp)
 *	 .
 *	 .	exception information area
 *	 .
 *     36(sp)
 */
	movpsl	r2				# get the psl
	extzv	$psl$v_prvmod,$psl$s_prvmod,r2,r2 # get the previous mode
	mfpr	r2,r0				# get user stack pointer
	movab	20(sp),r3			# r3 = address of scratch
						# area
	movl	(r0),r1				# get number of user stack
						# entries we have
	subl3	r1,$5,-(sp)			# subtract from the max no.
	ashl	$2,(sp),(sp)			# longword aligned
	addl2	(sp)+,r3			# r3 = start address of 
						# exception area
	movl	r3,16(sp)			# set the address

/*
 *	The previous stack look as follows:
 *
 *	0(r0)	signal number
 *	4(r0)	exception name
 *	 .	
 *	 .	exception specific information
 *	 (r0)	pc
 *	 (r0)	psl
 */


	tstl	(r0)+				# skip by num of signal args
1:	movl	(r0)+,(r3)+			# move user stack to kernel 
						# stack
	sobgtr	r1,1b				# cont until done
	mtpr	r0,r2				# reset user stack
	popr	$0x0f				# restore r0 r1 r2 r3
	movl	(sp),sp				# set the address of the
						# exception information
	brb	start_handling			# handle the exception

/*
 *	If we were already in kernal there is no need to copy the
 *	stack, but we do need to pop the signal arg count off.
 */
kern_handling:
	tstl	(sp)+				# pop of the signal count

/*
 *	Start handling the excpetion
 *
 *		(sp) - exception name
 *		 .
 *	 	 .	exception parameters
 * 	 	 .
 *         N*4+4(sp) - exception pc
 * 	   N*4+*(sp) - exception psl
 */

start_handling:

/*
 *	Check for access violation
 */
	cmpl	$ss$_accvio,(sp)	# Is this a access violation?
	bneq	1f			# br if no
	tstl	(sp)+			# pop of exception name
	jmp	_Xprotflt1		# jump handler

/*
 *	Check for reserved address
 */
1:	cmpl	$ss$_radrmod,(sp)	# Is this a reserved address
	bneq	2f			# br if no
	tstl	(sp)+			# pop of exception name
	jmp	_Xresadflt		# jump to handler
/*
 *	Check for reserved operand
 */
2:	cmpl	$ss$_roprand,(sp)	# Is this reserved operand
	bneq	3f			# br if no
	tstl	(sp)+			# pop of exception name
	jmp	_Xresopflt		# jump to handler

/*
 *	Reserved opcode
 */
 3:	cmpl	$vax$_opcdec,(sp)
	bneq	4f
	tstl	(sp)+
	jmp	_Xprivinflt

/*
 *	Reserved opcode with fpd
 */
 4:	cmpl	$vax$_opcdec_fpd,(sp)
	bneq	5f
	tstl	(sp)+
	jmp	_Xprivinflt

/*
 *	Check for the all the arithmetic traps and faults
 */
5:	cmpl	(sp),$ss$_intovf		# Is it in ..
	blss	6f				# between the ..
	cmpl	(sp),$ss$_fltund_f		# arithtrap ..
	bgtr	6f				# trap, if true then
/*
 *	We must subtract ss$_artress and divide by 8 to get the 
 *	hardware values of the trap and fault numbers. The reason
 *	for this is the variable constants were defined as
 *	(ss$_artress + 8 * <hardware constants value > ).  This is
 *	done to be consistent with the way VMS does it in the emulation
 *	code.
 */
	subl2	$ss$_artres,(sp)		
	divl2	$8,(sp)

	jmp	_Xarithtrap			# jmp to the Ultrix
						# routine that handles these
						# traps
/*
 *	Something is wrong
 */
6:	movpsl	r0			# *slr001* get psl
	extzv	$psl$v_curmod,$psl$s_curmod,r0,r0	
					# *slr001* get current mode
	beql	1f			# br if zero ( kernel )
	halt				# indicate that something is wrong
1:	pushab	pnstr2			# slr001 get address of string to print
	calls	$1,_panic		# *slr001* call panic routine
pnstr2:	.ascii	"vaxexception: fatal emulation error handling error type\n"
