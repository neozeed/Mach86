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
/*	@(#)vaxashp.s	1.4	Ultrix	12/10/84 */

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
 *	Stephen Reilly, 27-Apr-84
 * 001- An instruction was taken out that should have
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
 #
 #       the routine in this module emulates the vax-11 packed decimal 
 #       ashp instruction. this procedure can be a part of an emulator 
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
 #	V01-002	LJK0024		Lawrence J. Kenah	20-Feb-1984
 #		Add code to handle access violations. Perform minor cleanup.
 #
 #       v01-001 ljk0008         lawrence j. kenah       18-oct-1983
 #               the emulation code for ashp was moved into a separate module.
 #--


 # include files:

/*      $psldef                         # define bit fields in psl
 *      $srmdef                         # define arithmetic trap codes

 *      ashp_def                        # bit fields in ashp registers
 *      stack_def                       # stack usage for original exception
 */

 # macro definitions

 # psect declarations:

 #        BEGIN_MARK_POINT
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
 #       the source string specified by the  source  length  and  source  address
 #       operands is scaled by a power of 10 specified by the count operand.  the
 #       destination string specified by the destination length  and  destination
 #       address operands is replaced by the result.
 #
 #       a positive count  operand  effectively  multiplies#   a  negative  count
 #       effectively  divides#  and a zero count just moves and affects condition
 #       codes.  when a negative count is specified, the result is rounded  using
 #       the round operand.
 #
 # input parameters:
 #
 #       r0<15:0>  = srclen.rw   number of digits in source character string
 #       r0<23:16> = cnt.rb      shift count
 #       r1        = srcaddr.ab  address of input character string
 #       r2<15:0>  = dstlen.rw   length in digits of output decimal string
 #       r2<23:16> = round.rb    round operand used with negative shift count 
 #       r3        = dstaddr.ab  address of destination packed decimal string
 #
 # output parameters:
 #
 #       r0 = 0
 #       r1 = address of byte containing most significant digit of
 #            the source string
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
 # algorithm:
 #
 #       the routine tries as much as possible to work with entire bytes. this 
 #       makes the case of an odd shift count more difficult that of an even 
 #       shift count. the first part of the routine reduces the case of an odd 
 #       shift count to an equivalent operation with an even shift count.
 #
 #       the instruction proceeds in several stages. in the first stage, after 
 #       the input parameters have been verified and stored, the operation is 
 #       broken up into four cases, based on the sign and parity (odd or even) 
 #       of the shift count. these four cases are treated as follows, in order 
 #       of increasing complexity.
 #
 #       case 1. shift count is negative and even
 #
 #           the actual shift operation can work with the source string in
 #           place. there is no need to move the source string to an
 #           intermediate work area. 
 #
 #       case 2. shift count is positive and even
 #
 #           the source string is moved to an intermediate work area and the
 #           sign "digit" is cleared before the actual shift operation takes
 #           place. if the source is worked on in place, then a spurious sign
 #	     digit would be moved to the middle of the output string instead
 #	     of a zero. The alternative is to keep track of where, in the
 #	     several special cases of shifting, the sign digit is looked at.
 #	     We chose to use the work area to simplify the later stages of
 #	     this routine. 
 #
 #       cases 3 and 4. shift count is odd
 #
 #           the case of an odd shift count is considerably more difficult
 #           than an even shift count, which is only slightly more complicated
 #           than movp. in the case of an even shift count, various digits
 #           remain in the same place (high nibble or low nibble) in a byte.
 #           for odd shift counts, high nibbles become low nibbles and vice
 #           versa. in addition, digits that were adjacent when viewing the
 #           decimal string as a string of bits proceeding from low address to
 #           high are now separated by a full byte. 
 #
 #           we proceed in two steps. the source string is first moved to a
 #           work area. the string is then shifted by one. this shift reduces
 #           the operation to one of the two even shift counts already
 #           mentioned, where the source to the shift operation is the
 #           modified source string residing in the work area. the details of
 #           the shift-by-one are described below near the code that performs
 #           the actual shift. 
 #-

# define  ashp_shift_mask 0xf0f0f0f0
					# mask used to shift string by one

	.globl	vax$ashp

vax$ashp:
 #      pushr   $^m<r0,r1,r2,r3,r4,r5,r6,r7,r8,r9,r10,r11>      # save the lot
	 pushr	$0x0fff
        movpsl  r11                     # get initial psl
        insv    $psl$m_z,$0,$4,r11      # set z-bit, clear the rest
 #	ESTABLISH_HANDLER	-	; Store address of access
 #		ASHP_ACCVIO		;  violation handler
	movab	ashp_accvio,r10
 #      roprand_check   r2              # insure that r2 lequ 31
	 cmpw	r2,$31
	 blequ	1f
	 brw	decimal_roprand
1:
	 movzwl	r2,r2

 #      roprand_check   r0              # insure that r0 lequ 31
	 cmpw	r0,$31
	 blequ	1f
	 brw	decimal_roprand
1:
	 movzwl	r0,r0

        jsb    decimal$strip_zeros_r0_r1       # eliminate any high order zeros
        movl    sp,r8                   # remember current top of stack
        subl2   $20,sp                  # allocate work area on stack
 #	MARK_POINT	ASHP_BSBW_20_a
	 .text	2
	 .set	table_size,table_size + 1
	 .word	Lashp_bsbw_20_a - module_base
	 .text	3
	 .word	ashp_bsbw_20 - module_base
	 .text
Lashp_bsbw_20_a:
	jsb	decimal$strip_zeros_r0_r1	# Eliminate any high order zeros
        extzv   $1,$4,r2,r2             # convert output digit count to bytes
        incl    r2                      # make room for sign as well
        extzv   $1,$4,r0,r0             # same for input string
        addl3   r0,r1,r6                # get address of sign digit
        incl    r0                      # include byte containing sign
 #	MARK_POINT	ASHP_20_a
	 .text	2
	 .set	table_size,table_size + 1
	 .word	Lashp_20_a - module_base
	 .text	3
	 .word	ashp_20 - module_base
	 .text
Lashp_20_a:
        bicb3   $0x0f0,(r6),r6          # extract sign digit

 # Form sign of output string in r9 in preferred from (12 for "+" and 13 for 
 # "-")
        movzbl  $12,r9                  # assume that input sign is plus
        caseb   r6,$10,$15-10		# dispatch on sign 
L1:
        .word   4f-L1                   # 10 => +
        .word   3f-L1                   # 11 => -
        .word   4f-L1                   # 12 => +
        .word   3f-L1                   # 13 => -
        .word   4f-L1                   # 14 => +
        .word   4f-L1                   # 15 => +

3:      incl    r9                      # change preferred plus to minus
        bisb2   $psl$m_n,r11            # set n-bit in saved psw

 # we now retrieve the shift count from the saved r0 and perform the next set
 # of steps based on the parity and sign of the shift count. note that the
 # round operand is ignored unless the shift count is strictly less than zero.

4:      cvtbl   ashp_b_cnt(r8),r4       # extract sign-extended shift count
        blss    5f                      # branch if shift count negative
        clrl    r5                      # ignore "round" for positive shift
        bsbw    ashp_copy_source        # move source string to work area
        blbs    r4,6f                   # do shift by one for odd shift count
        bicb2   $0x0f,-(r8)             # drop sign in saved source string
        brb     ashp_shift_positive     # go do the actual shift

 # the "round" operand is important for negative shifts. if the shift count
 # is even, the source can be shifted directly into the destination. for odd
 # shift counts, the source must be moved into the work area on the stack and
 # shifted by one before the rest of the shift operation takes place.

5:      extzv   $ashp_v_round,$ashp_s_round,ashp_b_round(r8),r5
					# store "round" in a safe place
        blbc    r4,ashp_shift_negative  # get right to it for even shift count
        bsbw    ashp_copy_source        # move source string to work area

 # for odd shift counts, the saved source string is shifted by one in place.
 # this is equivalent to a shift of -1 so the shift count (r4) is adjusted
 # accordingly. the least significant digit is moved to the place occupied by
 # the sign, the tens digit becomes the units digit, and so on. because the
 # work area was padded with zeros, this shift moves a zero into the high
 # order digit of a source string of even length. 

6:      pushl   r0                      # we need a scratch register to count
        decl    r0                      # want to map {1..16} onto {0..3}
        ashl    $-2,r0,r0               # convert a byte count to longwords

 # the following loop executes from one to four times such that the entire
 # source, taken as a collection of longwords, is shifted by one. note that 
 # the two pieces of the source are shifted (rotated) in opposite directions. 
 # note also that the shift mask is applied to one string before the shift and 
 # to the other string after the shift. (this points up the arbitrary choice 
 # of shift mask. we just as well could have chosen the one's complement of 
 # the shift mask and reversed the order of the shift and mask operations for 
 # the two pieces of the source string.)

7:      rotl    $-4,-(r8),r6            # shift left one digit
        bicl2   $ashp_shift_mask,r6     # clear out old low order digits
        bicl3   $ashp_shift_mask,-1(r8),r7      # clear out high order digits
        rotl    $4,r7,r7                # shift these digits right one digit
        bisl3   r6,r7,(r8)              # combine the two sets of digits
        sobgeq  r0,7b                   # keep going if more

        movl    (sp)+,r0                # restore source string byte count
        incl    r4                      # count the shift we did
        blss    ashp_shift_negative     # join common code at the right place
                                        # drop through to ashp_shift_positive
 #+
 # functional description:
 #
 #       this routine completes the work of the ashp instruction in the case of
 #       an even shift count. (if the original shift count was odd, the source
 #       string has already been shifted by one and the shift count adjusted by
 #       one.) a portion (from none to all) of the source string is moved to
 #       the destination string. pieces of the destination string at either end
 #       may be filled with zeros. if excess digits of the source are not
 #       moved, they must be tested for nonzero to determine the correct
 #       setting of the v-bit. 
 #
 # input parameters:
 #
 #       r0<3:0> - number of bytes in source string 
 #       r1      - address of source string
 #       r2<3:0> - number of bytes in destination string 
 #       r3      - address of destination string
 #       r4<7:0> - count operand (signed longword of digit count)
 #       r5<3:0> - round operand in case of negative shift
 #       r9<3:0> - sign of source string in preferred form
 #
 # implicit input:
 #
 #       r4 is presumed (guaranteed) even on input to this routine
 #
 #       the top of the stack is assumed to contain a 20-byte work area (that 
 #       may or may not have been used). the space must be allocated for this 
 #       work area in all cases so that the exit code works correctly for all 
 #       cases without the need for lots of extra conditional code.
 #
 # output parameters:
 #
 #       this routine completes the operation of vax$ashp. see the routine
 #       header for vax$ashp for details on output registers and conditon codes.
 #
 # details:
 #
 #       put some of the stuff from ashp.txt here.
 #-


ashp_shift_positive:
        divl2   $2,r4                   # convert digit count to byte count
        subl3   r4,r2,r7                # modify the destination count
        blss    L130                     # branch if simply moving zeros

        movl    r4,r6                   # number of zeros at low order end
L110:   subl3   r0,r7,r8                # are there any excess high order digits?
        blss    L160                    # no, excess is in source.

 # we only move "srclen" source bytes. the rest of the destination string is
 # filled with zeros.

        movl    r0,r7                   # get number of bytes to actually move
        brb     L100                    # ... and go move them

 # the count argument is larger than the destination length. all of the source
 # is checked for nonzero (overflow check). all of the destination is filled
 # with zeros.

L130:   movl    r2,r6                   # number of low order zeros
        clrl    r7                      # the source string is untouched
        movl    r0,r8                   # number of source bytes to check
        brb     L180                    # go do the actual work

 # if the count is negative, then there is no need to fill in low order zeros
 # (r6 is zero). the following code is similar to the above cases, differing
 # in the roles played by source length (r0) and destination length (r2) and
 # also in the first loop (zero fill or overflow check) that executes.

ashp_shift_negative:
        clrl    r6                      # no zero fill at low end of destination
        mnegl   r4,r4                   # get absolute value of count
        divl2   $2,r4                   # convert digit count to byte count
        subl3   r4,r0,r7                # get modified source length
        blss    L170                    # branch if count is larger 

        subl3   r7,r2,r8                # are there zeros at high end?
        bgeq    L100                    # exit to zero fill loop if yes

 # the modified source length is larger than the destination length. part
 # of the source is moved. the rest is checked for nonzero.

        movl    r2,r7                   # only move "dstlen" bytes

 # in these cases, some digits in the source string will not be moved. if any
 # of these digits is nonzero, then the v-bit must be set.

L160:   mnegl   r8,r8                   # number of bytes in source to check
        brb     L180                    # exit to overflow check loop

 # the count argument is larger than the source length. all of the destination 
 # is filled with zeros. the source is ignored.

L170:   clrl    r7                      # no source bytes get moved
        movl    r2,r8                   # all of the destination is filled
        brb     L100                    # join the zero fill loop

 #+
 # at this point, the three separate counts have all been calculated. each
 # loop is executed in turn, stepping through the source and destination
 # strings, either alone or in step as appropriate.
 #
 #       r6 - number of low order digits to fill with zero
 #       r7 - number of bytes to move intact from source to destination
 #       r8 - number of excess digits in one or the other string. 
 #
 #       if excess source digits, they must be tested for nonzero to
 #       correctly set the v-bit.
 #
 #       if excess destination bytes, they must be filled with zero.
 #-

 # test excess source digits for nonzero

 #	MARK_POINT	ASHP_20_b
	 .text	2
	 .set	table_size,table_size + 1
	 .word	Lashp_20_b - module_base
	 .text	3
	 .word	ashp_20 - module_base
	 .text
Lashp_20_b:
L175:   tstb    (r1)+                   # is next byte nonzero
        bneq    L190                    # handle overflow out of line
L180:   sobgeq  r8,L175                 # otherwise, keep on looking

        brb     L220                    # join top of second loop

L190:   bisb2   $psl$m_v,r11            # set saved v-bit
        addl2   r8,r1                   # skip past rest of excess
        brb     L220                    # join top of second loop

 # in this case, the excess digits are found in the destination string. they
 # must be filled with zero.

L100:   tstl    r8                      # is there really something to do?
        beql    L220                    # skip first loop if nothing

 #	mark_point	ashp_20
	 .text	2
	 .set	table_size,table_size + 1
	 .word	Lashp_20 - module_base
	 .text	3
	 .word	ashp_20 - module_base
	 .text
Lashp_20:

L210:   clrb    (r3)+                   # store another zero
        sobgtr  r8,L210                 # ... and keep on looping

 # the next loop is where something interesting happens, namely that parts of
 # the source string are moved to the destination string. note that the use of
 # bytes rather than digits in this operation makes the detection of nonzero 
 # digits difficult because the presence of a nonzero digit in the place 
 # occupied by the sign or in the hign order nibble of an even length output
 # string and nowhere else would cause the Z-bit to be incorrectly cleared.
 # For this reason, we ignore the Z-bit here and make a special pass over the
 # output string after all of the special cases have been dealt with. The
 # extra overhead of a second trip to memory is offset by the simplicity in
 # other places in this routine.

L220:   tstl    r7                      # something to do here?
        beql    L240                    # skip this loop if nothing

 #	mark_point	ashp_20_d
	 .text	2
	 .set	table_size,table_size + 1
	 .word	Lashp_20_d - module_base
	 .text	3
	 .word	ashp_20 - module_base
	 .text
Lashp_20_d:
L230:   movb    (r1)+,(r3)+             # move the next byte
        sobgtr  r7,L230                 # ... and keep on looping

 # the final loop occurs in some cases of positive shift count where the low
 # order digits of the destination must be filled with zeros.

L240:   tstl    r6                      # something to do here?
        beql    L260                    # skip if loop count is zero

 #	mark_point	ashp_20_e
	 .text	2
	 .set	table_size,table_size + 1
	 .word	Lashp_20_e - module_base
	 .text	3
	 .word	ashp_20 - module_base
	 .text
Lashp_20_e:

L250:   clrb    (r3)+                   # store another zero
        sobgtr  r6,L250                 # ... until we're done

 #+
 # at this point, the destination string is complete except for the sign.
 # if there is a round operand, that must be added to the destination string.
 #
 #       r3 - address one byte beyond destination string
 #       r5 - round operand
 #-

L260:   addl2   $20,sp                  # deallocate work area
        extzv   $1,$4,ashp_w_dstlen(sp),r2      # get original destination byte count
        movq    r2,-(sp)                # save address and count for z-bit loop
        movzbl  r5,r8                   # load round into carry register
        beql    L280                    # skip next mess unless "round" exists

        movl    r3,r5                   # r5 tracks the addition output
        clrl    r6                      # we only need one term and carry in sum

 #	mark_point	ashp_8
	 .text	2
	 .set	table_size,table_size + 1
	 .word	Lashp_8 - module_base
	 .text	3
	 .word	ashp_8 - module_base
	 .text
Lashp_8:

L270:   movzbl  -(r3),r7                # get next digit
 #	mark_point	ashp_bsbw_8
	 .text	2
	 .set	table_size,table_size + 1
	 .word	Lashp_bsbw_8 - module_base
	 .text	3
	 .word	ashp_bsbw_8 - module_base
	 .text
Lashp_bsbw_8:

        jsb    vax$add_packed_byte_r6_r7       # perform the addition
        tstl    r8                      # see if this add produced a carry
        beql    L280                    # all done if no more carry
        sobgeq  r2,L270                 # back for the next byte

 # if we drop through the end of the loop, then the final add produced a carry.
 # this must be reflected by setting the v-bit in the saved psw.

        bisb2   $psl$m_v,r11            # set the saved v-bit

 # all of the digits are now loaded into the destination string. the condition
 # codes, except for the z-bit, have their correct settings. the sign must be
 # set, a check must be made for even digit count in the output string, and
 # the various special cases (negative zero, decimal overflow trap, ans so on)
 # must be checked before completing the routine. 

 # this entire routine worked with entire bytes, ignoring whether digit counts
 # were odd or even. an illegal digit in the upper nibble of an even input string
 # is ignored. a nonzero digit in the upper nibble of an even output string is
 # not allowed but must be checked for. if one exists, it indicates overflow.

L280:   blbs    (8+ashp_w_dstlen)(sp),L285
				        # skip next if output digit count is odd
        bitb    $0x0f0,*(8+ashp_a_dstaddr)(sp)
				        # is most significant digit nonzero?
        beql    L285                    # nothing to worry about if zero
 #	mark_point	ashp_8_c
	 .text	2
	 .set	table_size,table_size + 1
	 .word	Lashp_8_c - module_base
	 .text	3
	 .word	ashp_8 - module_base
	 .text
Lashp_8_c:

        bicb2   $0x0f0,*(8+ashp_a_dstaddr)(sp)      # make the digit zero
        bisb2   $psl$m_v,r11            # ... and set the overflow bit

 # we have not tested for nonzero digits in the output string. this test is
 # made by making another pass over the ouptut string. note that the low
 # order digit is unconditionally checked.

L285:   movq    (sp),r2                 # get address and count
 #	mark_point	ashp_8_a
	 .text	2
	 .set	table_size,table_size + 1
	 .word	Lashp_8_a - module_base
	 .text	3
	 .word	ashp_8 - module_base
	 .text
Lashp_8_a:

        bitb    $0x0f0,-(r3)       	# do not test sign in low order byte
        bneq    L287                    # skip loop if nonzero
        brb     L286                    # start at bottom of loop

 #	mark_point	ashp_8_b
	 .text	2
	 .set	table_size,table_size + 1
	 .word	Lashp_8_b - module_base
	 .text	3
	 .word	ashp_8 - module_base
	 .text
Lashp_8_b:

L283:   tstb    -(r3)                   # is next higher byte nonzero?
        bneq    L287                    # exit loop if yes
L286:   sobgeq  r2,L283                 # keep looking for nonzero if more bytes

 # the entire output string has been scanned and contains no nonzero
 # digits. the z-bit retains its original setting, which is set. if the
 # n-bit is also set, then the negative zero must be changed to positive
 # zero (unless the v-bit is also set). note that in the case of overflow,
 # the n-bit is cleared but the output string retrins the minus sign.

        bbc     $psl$v_n,r11,L290       # n-bit is off already
        bicb2   $psl$m_n,r11            # turn off saved n-bit unconditionally
        bbs     $psl$v_v,r11,L290       # no fixup if v-bit is also set 
        movb    $12,r9                  # use preferred plus as sign of output
        brb     L290                    # ... and rejoin the exit code

 # the following instruction is the exit point for all of the nonzero byte
 # checks. its direct effect is to clear the saved z-bit. it also bypasses
 # whatever other zero checks have not yet been performed.

L287:   bicb2    $psl$m_z,r11            # clear saved z-bit

 # the following code executes in all cases. it is the common exit path for
 # all of the ashp routines when the count is even.

L290:   movq    (sp)+,r2                # get address of end of output string
 #	mark_point	ashp_0
	 .text	2
	 .set	table_size,table_size + 1
	 .word	Lashp_0 - module_base
	 .text	3
	 .word	ashp_0 - module_base
	 .text
Lashp_0:
        insv    r9,$0,$4,-1(r3)         # store sign that we have been saving


 #+
 # this is the common exit path for many of the routines in this module. this
 # exit path can only be used for instructions that conform to the following
 # restrictions.
 #
 # 1.  registers r0 through r11 were saved on entry.
 #
 # 2.  the architecture requires that r0 and r2 are zero on exit.
 #
 # 3.  all other registers that have instruction-specific values on exit are 
 #     correctly stored in the appropriate locations on the stack.
 #
 # 4.  the saved psw is contained in r11
 #
 # 5.  this instruction/routine should generate a decimal overflow trap if 
 #     both the v-bit and the dv-bit are set on exit.
 #-

	.globl	vax$decimal_exit

vax$decimal_exit:
        clrl    (sp)                    # r0 must be zero on exit
        clrl    8(sp)                   # r2 must also be zero
        bbs     $psl$v_v,r11,L320       # see if exceptions are enabled
L310:   bicpsw  $( psl$m_n | psl$m_z | psl$m_v | psl$m_c )      
					# clear condition codes
        bispsw  r11                     # set appropriate condition codes
 #      popr    $^m<r0,r1,r2,r3,r4,r5,r6,r7,r8,r9,r10,r11>      # restore 
	 popr	$0x0fff
        rsb                             # ... and return                

 # if the v-bit is set and decimal traps are enabled (dv-bit is set), then
 # a decimal overflow trap is generated. note that the dv-bit can be set in
 # the current psl or, if this routine was entered as the result of an emulated
 # instruction exception, in the saved psl on the stack. note that the final
 # condition codes in the psw have not yet been set. this means that all exit
 # pathe out of this code must set the condition codes to their correct values.

	.globl	vax$editpc_overflow

vax$editpc_overflow:
L320:   bbs     $psl$v_dv,r11,L330       # report exception if current dv-bit set
        movab   vax$exit_emulator,r0    # set up r0 for pic address comparison
        cmpl    r0,(4*12)(sp)           # is return pc eqlu vax$exit_emulator ?
        bnequ   L310                    # no. simply return v-bit set
        bbc     $psl$v_dv,((4*(12+1))+exception_psl)(sp),L310
                                        # only return v-bit if dv-bit is clear

 # restore all of the saved registers, reset the condition codes, and drop 
 # into decimal_overflow.

L330:   bicpsw  $( psl$m_n | psl$m_z | psl$m_v | psl$m_c )      
					# clear condition codes
        bispsw  r11                     # set appropriate condition codes
 #      popr    $^m<r0,r1,r2,r3,r4,r5,r6,r7,r8,r9,r10,r11>
	 popr	$0x0fff
 #+
 # this code path is entered if the decimal string result is too large to
 # fit into the output string and decimal overflow exceptions are enabled.
 # the final state of the instruction, including the condition codes, is
 # entirely in place.
 #
 # input parameter:
 #
 #       (sp) - return pc
 #
 # output parameters:
 #
 #       0(sp) - srm$k_dec_ovf_t (arithmetic trap code)
 #       4(sp) - final state psl
 #       8(sp) - return pc
 #
 # implicit output:
 #
 #       control passes through this code to vax$reflect_trap.
 #-

	.globl	vax$decimal_overflow

vax$decimal_overflow:
        movpsl  -(sp)                   # save final psl on stack
        pushl   $srm$k_dec_ovf_t        # store arithmetic trap code
        jmp     vax$reflect_trap        # report exception


 #+
 # functional description:
 #
 #       for certain cases (three out of four), it is necessary to put the
 #       source string in a work area so that later portions of vax$ashp can
 #       proceed in a straightforward manner. in one case (positive even shift
 #       count), the sign must be eliminated before the least significant
 #       byte of the source is moved to its appropriate place (not the least
 #       significant byte) in the destination string. for odd shift counts,
 #       the source string in the work area is shifted by one to reduce the
 #       complicated case of an odd shift count to an equivalent but simpler
 #       case with an even shift count.
 #
 #       this routine moves the source string to a 20-byte work area already
 #       allocated on the stack. note that the work area is zeroed by this
 #       routine so that, if the work area is used, it consists of either
 #       valid bytes from the source string or bytes containing zero. if the
 #       work area is not needed (shift count is even and not positive), the
 #       overhead of zeroing the work area is avoided. 
 #
 # input parameters:
 #
 #       r0 - byte count of source string (preserved)
 #       r1 - address of most significant byte in source string
 #       r8 - address one byte beyond end of work area (preserved)
 #
 # output parameters:
 #
 #       r1 - address of most significant byte of source string in
 #            work area
 #
 # side effects:
 #
 #       r6 and r7 are modified by this routine.
 #-

ashp_copy_source:
        clrq    -8(r8)                  # insure that the work area 
        clrq    -16(r8)                 # ... is entirely filled
        clrl    -20(r8)                 # ... with zeros
        addl3   r0,r1,r7                # r7 points one byte beyond source
        movl    r8,r1                   # r1 will step through work area
        movl    r0,r6                   # use r6 as the loop counter

 #	mark_point	ashp_bsbw_20
	 .text	2
	 .set	table_size,table_size + 1
	 .word	Lashp_bsbw_20 - module_base
	 .text	3
	 .word	ashp_bsbw_20 - module_base
	 .text
Lashp_bsbw_20:

1:      movb    -(r7),-(r1)             # move the next source byte
        sobgtr  r6,1b                  # check for end of loop

        rsb                             # return with r1 properly loaded
 #-
 # functional description:
 #
 #       this routine receives control when a digit count larger than 31 is
 #       detected. the exception is architecturally defined as an abort so
 #       there is no need to store intermediate state. the vax$ashp routine
 #       saves all registers r0 through r11 before performing the digit check.
 #       these registers must be restored before control is passed to
 #       vax$roprand. 
 #
 # input parameters:
 #
 #       00(sp) - saved r0
 #         .
 #         .
 #       44(sp) - saved r11
 #       48(sp) - return pc from vax$xxxxxx routine
 #
 # output parameters:
 #
 #       00(sp) - offset in packed register array to delta pc byte
 #       04(sp) - return pc from vax$xxxxxx routine
 #
 # implicit output:
 #
 #       this routine passes control to vax$roprand where further
 #       exception processing takes place.
 #-

decimal_roprand:
 #      popr    $^m<r0,r1,r2,r3,r4,r5,r6,r7,r8,r9,r10,r11>
	 popr	$0x0fff
        pushl   $ashp_b_delta_pc        # store offset to delta pc byte
        jmp     vax$roprand             # pass control along

 #+
 #	the general approach to handling access violations that occur while
 #	emulating the packed decimal instructions is described here. we take
 #	advantage of the fact that there are no architectural constraints on
 #	the way that access violations must be handled. in general, we back
 #	the instruction up to the beginning, to the point where initial state
 #	is stored in the set of general registers modified by each
 #	instruction. thus, the only step that is avoided when an instruction
 #	is restarted is operand evaluation. 
 #
 # functional description:
 #
 #	this routine (or its counterpart in other decimal emulation modules)
 #	receives control when an access violation occurs while executing
 #	within one of the vax$xxxxxx routines that emulated a decimal string
 #	instruction. this routine determines whether the exception occurred
 #	while accessing a source or destination string or whether the access
 #	violation is peculiar to emulation, such as stack overflow. (this
 #	check is made based on the pc of the exception.) 
 #
 #	if the pc is one that is recognized by this routine, then the state of
 #	the instruction (digit counts, string addresses, and the like) are
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
 #	xx+<4*m>(sp) - return pc from vax$xxxxxx routine (m is the number
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
 #	finally, the macro begin_mark_point should have been invoked at the
 #	beginning of this module to define the symbols 
 #
 #		module_base
 #		pc_table_base
 #		handler_table_base
 #		table_size
 #
 #	pc_table_base is the base of a word array with one entry for each 
 #		pc (relative to module_base) that can cause an access 
 #		violation that is capable of being backed up.
 #
 #	handler_table_base is the base of a corresponding word array with an
 #		entry that locates the context specific code to handle each
 #		of the recognized access violations.
 #
 # output parameters (exit via jmp instruction):
 #
 #	if the exception is recognized (that is, if the exception pc is 
 #	associated with one of the mark points), control is passed to the 
 #	context-specific routine that restores the instruction state to 
 #	its initial state.
 #
 #	these are the register values and stack state when the context
 #	specific code begins execution.
 #
 #		r0  - value of sp when exception occurred
 #		r1  - scratch
 #		r2  - scratch
 #		r3  - scratch
 #		r10 - scratch
 #
 # r0 ->	zz(sp) - instruction-specific data begins here
 #
 # implicit output:
 #
 #	the context-specific code accomplishes essentially the same thing for 
 #	all of the emulated instructions.
 #
 #	the register contents are restored to the values that they had on 
 #	entry to the vax$xxxxxx routine. this causes the instruction to be 
 #	backed up almost (but not quite) to its starting point. (the operand 
 #	evaluation is not lost. the operands are saved in registers.) any
 #	registers saved on entry are restored.
 #
 # output parameters (exit via rsb instruction):
 #
 #	if the exception pc occurred somewhere else (such as a stack access), 
 #	the saved registers are restored and control is passed back to the 
 #	host system with an rsb instruction.
 #
 #-

ashp_accvio:
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
 # functional decsription:
 #
 #	this routine is called by the exception handlers for the various 
 #	decimal string instruction emulation routines to perform a bounds 
 #	check on the exception pc. the real reason for performing this check 
 #	is that certain exceptions can occur in subroutines that are outside a 
 #	given module. in this case, it is not the exception pc but rather the 
 #	return pc on the top of the stack that determines the context of the 
 #	exception (and therefore, the code necessary to back up the 
 #	instruction state). 
 #
 #	the basic mode of operation is that, if the exception pc is outside 
 #	the current module boundarise, then r1 (the exception pc) is replaced 
 #	by the return pc (presumed pointed to by r0).
 #
 # input parameters:
 #
 #	r0 - top of stack when exception occurred
 #	r1 - pc at time of exception
 #
 #	(r0) - return pc from subroutine in which access violation occurred
 #
 #	00(sp) - return pc from caller of this routine
 #	04(sp) - end address of module
 #	08(sp) - start address of module
 #
 # output parameters:
 #
 #	if the exception pc is outside the bounds of the module (as defined by
 #	the two longwords on the stack, then r1 is replaced by the "return
 #	pc", the contents of the longword located by r0. 
 #
 #	if the exception pc is inside the module, nothing is changed by the
 #	execution of this module. 
 #
 # assumptions:
 #
 #	there are two assumptions that must hold for these subroutines
 #	in which access violations can occur.
 #
 #		they must not use the stack. this keeps the return pc on the 
 #		top of the stack, located by r0.
 #
 #		they must be called with a bsbw instruction. this causes the 
 #		return pc to be exactly three bytes beyond instruction that 
 #		transferred control to the subroutine.
 #-

		.globl	decimal$bounds_check
decimal$bounds_check:
	cmpl	r1,4(sp)		# beyond upper end?
	bgequ	1f			# branch if out of bounds
	cmpl	r1,8(sp)		# within lower limit?
	blssu	1f			# branch if out of bounds
	rsb				# return with r1 intact

 # r1 is out of bounds. replace it with the return pc from the routine that
 # was executing when the access violation occurred. note that the pc is
 # backed up over the jsb instruction because the pc offset that appears in
 # the pc_table will be the pc of the jsb instruction and not the pc of the
 # next instruction. 

1:	subl3	$6,(r0),r1		# get new "exception" pc
	rsb


 #+
 # functional description:
 #
 #	the only difference among the various entry points is the number of
 #	longwords on the stack. r0 is advanced beyond these longwords to point
 #	to the list of saved registers. these registers are then restored,
 #	effectively backing the routine up to its initial state. 
 #
 # input parameters:
 #
 #	r0 - address of top of stack when access violation occurred
 #
 #	see specific entry points for details
 #
 # output parameters:
 #
 #	see input parameter list for vax$decimal_accvio
 #-

 #+
 # ashp_bsbw_20
 #
 # an access violation occurred in subroutine strip_zeros or in subroutine
 # ashp_copy_source while the source string was being copied to the work space
 # on the stack. in addition to the five longwords of work space on the stack,
 # this routine has an additional longword, the return pc, on the stack. 
 #
 #	00(r0) - return pc in mainline of vax$ashp
 #	04(r0) - first longword of scratch space
 #	 etc.
 #-

ashp_bsbw_20:
	addl2	$4,r0			# skip over return pc and drop into ...

 #+
 # ashp_20
 #
 # there are five longwords of workspace on the stack for this entry point.
 #
 #	00(r0) - first longword of scratch space
 #	  .
 #	  .
 #	16(r0) - fifth longword of scratch space
 #	20(sp) - saved r0
 #	24(sp) - saved r1
 #	 etc.
 #-

ashp_20:
	addl2	$20,r0			# discard scratch space on stack
	jmp	vax$decimal_accvio	# join common code to restore registers

 #+
 # ashp_bsbw_8
 #
 # an access violation occurred in subroutine add_packed_byte while the round
 # operand was being propogated. in addition to the saved r2/r3 pair of
 # longwords on the stack, this routine has an additional longword, the return
 # pc, on the stack. 
 #
 #	00(r0) - return pc in mainline of vax$ashp
 #	04(r0) - saved intermediate value of r2
 #	 etc.
 #-

ashp_bsbw_8:
	addl2	$4,r0			# skip over return pc and drop into ...

 #+
 # ashp_8
 #
 # there is a saved register pair (two longwords) on the stack for these entry
 # points.
 #
 #	00(r0) - saved intermediate value of r2
 #	04(r0) - saved intermediate value of r3
 #	08(sp) - saved r0
 #	12(sp) - saved r1
 #	16(sp) - saved r2
 #	20(sp) - saved r3
 #	 etc.
 #-

ashp_8:
	addl2	$8,r0			# discard saved register pair

 # drop into vax$decimal_accvio to restore saved registers

 #+
 # ashp_0
 #
 # the stack is empty. this label is merely a synonym for vax$decimal_accvio
 # because there is no context-specific work to do. 
 #
 #	00(sp) - saved r0
 #	04(sp) - saved r1
 #	08(sp) - saved r2
 #	12(sp) - saved r3
 #	 etc.
 #-

ashp_0:

 # drop into vax$decimal_accvio to restore saved registers
 #+
 # functional description:
 #
 #	this code is the final access violation processing for those 
 #	exceptions that have two things in common. 
 #
 #	the instruction/routine is to be backed up to its initial state.
 #
 #	all registers from r0 to r11 were saved on entry to vax$xxxxxx.
 #
 # input parameters:
 #
 #	00(r0) - saved r0 on entry to vax$xxxxxx
 #	04(r0) - saved r1
 #	  .
 #	  .
 #	44(r0) - saved r11 on entry to vax$xxxxxx
 #	48(r0) - return pc from vax$xxxxxx routine
 #
 #	00(sp) - saved r0 (restored by vax$handler)
 #	04(sp) - saved r1
 #	08(sp) - saved r2
 #	12(sp) - saved r3
 #
 # output parameters:
 #
 #	r0 is advanced over saved register array as the registers are restored.
 #	r0 ends up pointing at the return pc.
 #
 #	r1 contains the value of delta pc for all of the routines that
 #	use this common code path. the fpd and accvio bits are both set
 #	in r1.
 #
 #	00(r0) - return pc from vax$xxxxxx routine
 #	
 #	00(sp) - value of r0 on entry to vax$xxxxxx
 #	04(sp) - value of r1 on entry to vax$xxxxxx
 #	08(sp) - value of r2 on entry to vax$xxxxxx
 #	12(sp) - value of r3 on entry to vax$xxxxxx
 #
 #	r4 through r11 are restored to their values on entry to vax$xxxxxx.
 #-

		.globl	vax$decimal_accvio
vax$decimal_accvio:
	movq	(r0)+,pack_l_saved_r0(sp)	# "restore" r0 and r1
	movq	(r0)+,pack_l_saved_r2(sp)	# "restore" r2 and r3
	movq	(r0)+,r4			# really restore r4 and r5
	movq	(r0)+,r6			# ... and r6 and r7
	movq	(r0)+,r8			# ... and r8 and r8
	movq	(r0)+,r10			# ... and r10 and r11


 #	movl	#<ashp_b_delta_pc!-	# indicate offset for delta pc
 #		pack_m_fpd!-		# fpd bit should be set
 #		pack_m_accvio>,r1	# this is an access violation
	 movl	$( ashp_b_delta_pc | pack_m_fpd | pack_m_accvio ),r1
	jmp	vax$reflect_fault	# continue exception handling

 #	end_mark_point

module_end:
