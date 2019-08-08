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
/*	@(#)vaxdeciml.s	1.3		11/2/84		*/

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
 #
 #       the routines in this module emulate the vax-11 packed decimal 
 #       instructions. these procedures can be a part of an emulator 
 #       package or can be called directly after the input parameters 
 #       have been loaded into the architectural registers.
 #
 #       the input parameters to these routines are the registers that
 #       contain the intermediate instruction state. 
 #
 # environment: 
 #
 #       these routines run at any access mode, at any ipl, and are ast 
 #       reentrant.
 #
 # author: 
 #
 #       lawrence j. kenah       
 #
 # creation date
 #
 #       24 september 1982
 #
 # modified by:
 #
 #	 v01-007 ljk0043	 lawrence j. kenah	 26-jul-1984
 #		 change strip_zeros routines so that they are more forgiving
 #		 when confronted with poorly formed packed decimal strings.
 #		 specifically, do not allow string lengths smaller than zero.
 #
 #	 v01-006 ljk0038	 lawrence j. kenah	 19-jul-1984
 #		 Insure that initial setting of c-bit is preserved when
 #		 movp is restarted after an access violation.
 #	
 #	 v01-005 ljk0024	 lawrence j. Kenah	 20-Feb-1984
 #		 Add coe to handle access violations.
 #
 #       v01-004 ljk0013         lawrence j. kenah       17-nov-1983
 #               move cvtpl to a separate module.
 #
 #       v01-003 ljk0008         lawrence j. kenah       18-oct-1983
 #               move decimal arithmetic and numeric string routines to
 #               separate modules.
 #
 #       v01-002 ljk0006         lawrence j. kenah       14-oct-1983
 #               fix code that handles arithmetic traps. add reserved operand
 #               processing. add probes and other code to handle access
 #               violations.
 #
 #       v01-001 original        lawrence j. kenah       24-sep-82
 #
 #--

 #+
 # there are several techniques that are used throughout the routines in this 
 # module that are worth a comment somewhere. rather than duplicate near 
 # identical commentary in several places, we will describe these general 
 # techniques in a single place.
 #
 # 1.    the vax-11 architecture specifies that several kinds of input produce 
 #       unpredictable results. they are:
 #
 #        o  illegal decimal digit in packed decimal string
 #
 #        o  illegal sign specifier (other than 10 through 15) in low nibble of 
 #           highest addressed byte of packed decimal string
 #
 #        o  packed decimal string with even number of digits that contains 
 #           other than a zero in the high nibble of the lowest addressed byte
 #
 #       these routines take full advantage of the meaning of unpredictable. 
 #       in general, the code assumes that all input is correct. the operation 
 #       of the code for illegal input is not even consistent but is simply 
 #       whatever happens to be convenient in a particular place.
 #
 # 2.    all of these routines accumulate information about condition codes at
 #       several key places in a routine. this information is kept in a
 #       register (usually r11) that is used to set the final condition codes
 #       in the psw. in order to allow the register to obtain its correct
 #       contents when the routine exits (without further affecting the
 #       condition codes), the condition codes are set from the register
 #       (bispsw reg) and the register is then restored with a popr
 #       instruction, which does not affect condition codes. 
 #
 # 3.    there are several instances in these routines where it is necessary to
 #       determine the difference in length between an input and an output
 #       string and perform special processing on the excess digits. when the
 #       longer string is a packed decimal string (it does not matter if the
 #       packed decimal string is an input string or an output string), it is
 #       sometimes useful to convert the difference in digits to a byte count. 
 #
 #       there are four different cases that exist. we will divide these cases
 #       into two sets of two cases, depending on whether the shorter length is
 #       even or odd. 
 #
 #       in the pictures that appear below, a blank box indicates a digit in 
 #       the shorter string. a string of three dots in a box indicates a digit 
 #       in the longer string. a string of three stars indicates an unused 
 #       digit in a decimal string. the box that contains +/- obviously 
 #       indicates the sign nibble in a packed decimal string.
 #
 # (cont.)


 #                                               +-------+-------+
 #                                               |       |       |
 #                                               |  ***  |  ...  |
 #                                               |       |       |
 #          +-------+-------+                    +-------+ - - - +
 #          |               |                    |               |
 #          |  ...  |  ...  |                    |  ...  |  ...  |
 #          |               |                    |               |
 #          + - - - +-------+                    + - - - +-------+
 #          |       |       |                    |       |       |
 #          |  ...  |       |                    |  ...  |       |
 #          |       |       |                    |       |       |
 #          +-------+ - - - +                    +-------+ - - - +
 #          |               |                    |               |
 #          |       |       |                    |       |       |
 #          |               |                    |               |
 #          + - - - + - - - +                    + - - - + - - - +
 #          |               |                    |               |
 #          |       |  +/-  |                    |       |  +/-  |
 #          |               |                    |               |
 #          +-------+-------+                    +-------+-------+
 #
 #        a  longer string odd                 b  longer string even
 #           difference odd                       difference even
 #
 #
 #             case 1  shorter string has even number of digits
 #
 #
 #
 #          +-------+-------+                    +-------+-------+
 #          |               |                    |       |       |
 #          |  ...  |  ...  |                    |  ***  |  ...  |
 #          |               |                    |       |       |
 #          + - - - + - - - +                    +-------+ - - - +
 #          |               |                    |               |
 #          |  ...  |  ...  |                    |  ...  |  ...  |
 #          |               |                    |               |
 #          +-------+-------+                    +-------+-------+
 #          |               |                    |               |
 #          |       |       |                    |       |       |
 #          |               |                    |               |
 #          + - - - + - - - +                    + - - - + - - - +
 #          |               |                    |               |
 #          |       |  +/-  |                    |       |  +/-  |
 #          |               |                    |               |
 #          +-------+-------+                    +-------+-------+
 #
 #        a  longer string odd                 b  longer string even
 #           difference even                      difference odd
 #
 #
 #              case 2  shorter string has odd number of digits
 #
 # (cont.)


 #
 #       in general, the code must calculate the number of bytes that contain 
 #       the excess digits. most of the time, the interesting number includes 
 #       complete excess bytes. the excess digit in the high nibble of the 
 #       highest addressed byte (both parts of case 1) is ignored. 
 #
 #       in three out of four cases, the difference (called r5 from this point 
 #       on) can be simply divided by two to obtain a byte count. in one case 
 #       (case 2 b), this is not correct. (for example, 3/2 = 1 and we want to 
 #       get a result of 2.) note, however, that in both parts of case 2, we 
 #       can add 1 to r5 before we divide by two. in case 2 b, this causes the 
 #       result to be increased by 1, which is what we want. in case 2 a, 
 #       because the original difference is even, an increment of one before we 
 #       divide by two has no effect on the final result. 
 #
 #       the correct code sequence to distinguish case 2 b from the other three
 #       cases involves two blbx instructions. a simpler sequence that
 #       accomplishes correct results in all four cases when converting a digit
 #       count to a byte count is something like 
 #
 #                       blbc    length-of-shorter,10$
 #                       incl    r5
 #               10$:    ashl    $-1,r5,r5
 #
 #       where the length of the shorter string will typically be contained in 
 #       either r0 or r2.
 #
 #       note that we could also look at both b parts, performing the extra 
 #       incl instruction when the longer string is even. in case 1 b, this 
 #       increment transforms an even difference to an odd number but does not 
 #       affect the division by two. in case 2 b, the extra increment produces 
 #       the correct result. this option is not used in these routines.
 #
 #       the two routines for cvtsp and cvttp need a slightly different number. 
 #       they want the number of bytes including the byte containing the excess 
 #       high nibble. for case 2, the above calculation is still valid. for 
 #       case 1, it is necessary to add one to r5 after the r5 is divided by 
 #       two to obtain the correct byte count.
 #
 # 4.    there is a routine called strip_zeros that removes high order zeros
 #       from decimal strings. this routine is not used by all of the routines
 #       in this module but only by those routines that perform complicated
 #       calculations on each byte of the input string. for these routines, the
 #       overhead of testing for and discarding leading zeros is less than the
 #       more costly per byte overhead of these routines. 
 #-



 # include files:

 #        $psldef                         # define bit fields in psl

 #        cmpp3_def                       # bit fields in cmpp3 registers
 #       cmpp4_def                       # bit fields in cmpp4 registers
 #        movp_def                        # bit fields in movp registers
 #       stack_def                       # stack usage of original exception

 #	begin_mark_point
		.text
		.text	2
	.set	table_size,0
pc_table_base:
		.text	3
handler_table_base:
		.text		
module_base:

 # psect declarations:
 #+
 # the following tables are designed to perform fast conversions between
 # numbers in the range 0 to 99 and their decimal equivalents. the tables
 # are used by placing the input parameter into a register and then using
 # the contents of that register as an index into the table.
 #-

 #+
 #       decimal digits to binary number
 #
 # the following table is used to convert a packed decimal byte to its binary
 # equivalent. 
 #
 # packed decimal numbers that contain illegal digits in the low nibble
 # convert as if the low nibble contained a zero. that is, the binary number
 # will be a multiple of ten. this is done so that this table can be used to
 # convert the least significant (highest addressed) byte of a decimal string
 # without first masking off the sign "digit". 
 #
 # illegal digits in the high nibble produce unpredictable results because the
 # table does not contain entries to handle these illegal constructs. 
 #-

 #               binary equivalent                 decimal digits
 #               -----------------                 --------------

	.globl	decimal$packed_to_binary_table

decimal$packed_to_binary_table:

        .byte   00 , 01 , 02 , 03 , 04 		# index  ^x00
        .byte   05 , 06 , 07 , 08 , 09          #    to  ^x09

        .byte   00 , 00 , 00 , 00 , 00 , 00     # illegal decimal digits

        .byte   10 , 11 , 12 , 13 , 14	        # index  ^x10
        .byte   15 , 16 , 17 , 18 , 19          #    to  ^x19

        .byte   10 , 10 , 10 , 10 , 10 , 10     # illegal decimal digits

        .byte   20 , 21 , 22 , 23 , 24          # index  ^x20
        .byte   25 , 26 , 27 , 28 , 29          #    to  ^x29

        .byte   20 , 20 , 20 , 20 , 20 , 20     # illegal decimal digits

        .byte   30 , 31 , 32 , 33 , 34          # index  ^x30
        .byte   35 , 36 , 37 , 38 , 39          #    to  ^x39

        .byte   30 , 30 , 30 , 30 , 30 , 30     # illegal decimal digits

        .byte   40 , 41 , 42 , 43 , 44	        # index  ^x40
        .byte   45 , 46 , 47 , 48 , 49          #    to  ^x49

        .byte   40 , 40 , 40 , 40 , 40 , 40     # illegal decimal digits

        .byte   50 , 51 , 52 , 53 , 54          # index  ^x50
        .byte   55 , 56 , 57 , 58 , 59          #    to  ^x59

        .byte   50 , 50 , 50 , 50 , 50 , 50     # illegal decimal digits

        .byte   60 , 61 , 62 , 63 , 64          # index  ^x60
        .byte   65 , 66 , 67 , 68 , 69          #    to  ^x69

        .byte   60 , 60 , 60 , 60 , 60 , 60     # illegal decimal digits

        .byte   70 , 71 , 72 , 73 , 74          # index  ^x70
        .byte   75 , 76 , 77 , 78 , 79          #    to  ^x79

        .byte   70 , 70 , 70 , 70 , 70 , 70     # illegal decimal digits

        .byte   80 , 81 , 82 , 83 , 84          # index  ^x80
        .byte   85 , 86 , 87 , 88 , 89          #    to  ^x89

        .byte   80 , 80 , 80 , 80 , 80 , 80     # illegal decimal digits

        .byte   90 , 91 , 92 , 93 , 94      	# index  ^x90
        .byte   95 , 96 , 97 , 98 , 99          #    to  ^x99

        .byte   90 , 90 , 90 , 90 , 90 , 90     # illegal decimal digits

 #+
 #               binary number to decimal equivalent
 #
 # the following table is used to do a fast conversion from a binary number
 # stored in a byte to its decimal representation. the table structure assumes
 # that the number lies in the range 0 to 99. numbers that lie outside this
 # range produce unpredictable resutls.
 #-

 #               decimal equivalents                          binary
 #               -------------------                          ------

	.globl	decimal$binary_to_packed_table

decimal$binary_to_packed_table:

        .byte   0x00 , 0x01 , 0x02 , 0x03 , 0x04        #  0 through  9
        .byte   0x05 , 0x06 , 0x07 , 0x08 , 0x09        # 

        .byte   0x10 , 0x11 , 0x12 , 0x13 , 0x14        # 10 through 19
        .byte   0x15 , 0x16 , 0x17 , 0x18 , 0x19        # 

        .byte   0x20 , 0x21 , 0x22 , 0x23 , 0x24        # 20 through 29
        .byte   0x25 , 0x26 , 0x27 , 0x28 , 0x29        # 

        .byte   0x30 , 0x31 , 0x32 , 0x33 , 0x34        # 30 through 39
        .byte   0x35 , 0x36 , 0x37 , 0x38 , 0x39        # 

        .byte   0x40 , 0x41 , 0x42 , 0x43 , 0x44        # 40 through 49
        .byte   0x45 , 0x46 , 0x47 , 0x48 , 0x49        # 

        .byte   0x50 , 0x51 , 0x52 , 0x53 , 0x54        # 50 through 59
        .byte   0x55 , 0x56 , 0x57 , 0x58 , 0x59        # 

        .byte   0x60 , 0x61 , 0x62 , 0x63 , 0x64        # 60 through 69
        .byte   0x65 , 0x66 , 0x67 , 0x68 , 0x69        # 

        .byte   0x70 , 0x71 , 0x72 , 0x73 , 0x74        # 70 through 79
        .byte   0x75 , 0x76 , 0x77 , 0x78 , 0x79        # 

        .byte   0x80 , 0x81 , 0x82 , 0x83 , 0x84        # 80 through 89
        .byte   0x85 , 0x86 , 0x87 , 0x88 , 0x89        # 

        .byte   0x90 , 0x91 , 0x92 , 0x93 , 0x94        # 90 through 99
        .byte   0x95 , 0x96 , 0x97 , 0x98 , 0x99        # 

 #+
 # functional description:
 #
 #       in 3 operand format, the source 1 string specified  by  the  length  and
 #       source  1  address operands is compared to the source 2 string specified
 #       by the length and source 2 address operands.   the  only  action  is  to
 #       affect the condition codes.
 #
 #       in 4 operand format, the source 1  string  specified  by  the  source  1
 #       length  and source 1 address operands is compared to the source 2 string
 #       specified by the source 2 length and source  2  address  operands.   the
 #       only action is to affect the condition codes.
 #
 # input parameters:
 #
 #       entry at vax$cmpp3
 #
 #               r0 - len.rw             length of either decimal string
 #               r1 - src1addr.ab        address of first packed decimal string
 #               r3 - src2addr.ab        address of second packed decimal string
 #
 #       entry at vax$cmpp4
 #
 #               r0 - src1len.rw         length of first packed decimal string
 #               r1 - src1addr.ab        address of first packed decimal string
 #               r2 - src2len.rw         length of second packed decimal string
 #               r3 - src2addr.ab        address of second packed decimal string
 #
 # output parameters:
 #
 #       r0 = 0
 #       r1 = address of the byte containing the most significant digit of
 #            the first source string
 #       r2 = 0
 #       r3 = address of the byte containing the most significant digit of
 #            the second source string
 #
 # condition codes:
 #
 #       n <- first source string lss second source string
 #       z <- first source string eql second source string
 #       v <- 0
 #       c <- 0
 #
 # register usage:
 #
 #       this routine uses r0 through r5. the condition codes are recorded
 #       in r2 as the routine executes.
 #
 # algorithm:
 #
 #       tbs
 #-


 #+
 # define some bit fields that allow recording the presence of minus signs
 # in either or both of the source strings.
 #-

/*        $defini cmppx_flags
 *
 *       _vield  cmppx,0,<-
 *               <src1_minus,,m>,-
 *               <src2_minus,,m>,-
 *                       >
 *
 *       $defend cmppx_flags
 */
   
	.globl	vax$cmpp3

vax$cmpp3:
        movzwl  r0,r2                   # make two source lengths equal
        brb     L110                    # only make one length check    

	.globl	vax$cmpp4
vax$cmpp4:

 #      roprand_check   r2              # insure that r2 lequ 31
	 cmpw	r2,$31
	 blequ	1f
	 brw	decimal_roprand
1:
	 movzwl	r2,r2

L110:
 #      roprand_check   r0              # insure that r0 lequ 31
	 cmpw	r0,$31
	 blequ	1f
	 brw	decimal_roprand
1:
	 movzwl	r0,r0
 #      pushr   $^m<r0,r1,r2,r3,r4,r5>  # save some registers
	 pushr	$0x03f 
 #	establish_handler	decimal_accvio	# Store address of access
					# violation handler
	 movab	decimal_accvio,r10
 # get sign of first input string

        clrl    r4                      # assume both strings contain "+"
        extzv   $1,$4,r0,r5             # convert digit count to byte count
 #	mark_point	cmppx_accvio
	 .text	2
	 .set	table_size,table_size + 1
	 .word	Lcmppx_accvio - module_base
	 .text	3
	 .word	cmppx_accvio - module_base
	 .text
Lcmppx_accvio:
        bicb3   $0x0f0,(r1)[r5],r5 	# r5 contains "sign" digit
        caseb   r5,$10,$15-10		# dispatch on sign digit
L1:
        .word   3f-L1                   # 10 => sign is "+"
        .word   2f-L1                   # 11 => sign is "-"
        .word   3f-L1                   # 12 => sign is "+"
        .word   2f-L1                   # 13 => sign is "-"
        .word   3f-L1                   # 14 => sign is "+"
        .word   3f-L1                   # 15 => sign is "+"

2:      bisl2   $cmppx_m_src1_minus,r4  # remember that src1 contains "-"

 # now get sign of second input string

3:      extzv   $1,$4,r2,r5             # convert digit count to byte count
 #	mark_point	cmppx_accvio_a
	 .text	2
	 .set	table_size,table_size + 1
	 .word	Lcmppx_accvio_a - module_base
	 .text	3
	 .word	cmppx_accvio - module_base
	 .text
Lcmppx_accvio_a:
        bicb3   $0x0f0,(r3)[r5],r5	# r5 contains "sign" digit
        caseb   r5,$10,$15-10		# dispatch on sign digit
L2:
        .word   5f-L2                    # 10 => sign is "+"
        .word   4f-L2                    # 11 => sign is "-"
        .word   5f-L2                    # 12 => sign is "+"
        .word   4f-L2                    # 13 => sign is "-"
        .word   5f-L2                    # 14 => sign is "+"
        .word   5f-L2                    # 15 => sign is "+"

4:      bisl2   $cmppx_m_src2_minus,r4  # remember that src2 contains "-"

 # at this point, we have determined the signs of both input strings. if the
 # strings have different signs, then the comparison is done except for the
 # extraordinary case of comparing a minus zero to a plus zero. if both signs
 # are the same, then a digit-by-digit comparison is required.

5:      caseb   r4,$0,$3-0		# dispatch on combination of signs
L3:
        .word   6f-L3                   # both signs are "+"
        .word   minus_zero_check-L3      # signs are different
        .word   minus_zero_check-L3      # signs are different
        .word   6f-L3                   # both signs are "-"

 # both strings have the same sign. if the strings have different lengths, then
 # the excess digits in the longer string are checked for nonzero because that
 # eliminates the need for further comparison.

6:      subl3   r0,r2,r5                # get difference in lengths
        beql    equal_length            # strings have the same size
        blss    src2_shorter            # src2 is shorter than src1

 # this code executes when src1 is shorter than src2. that is, r0 lssu r2.
 # the large comment at the beginning of this module explains the need for the
 # incl r5 instruction when r0, the length of the shorter string, is odd.

src1_shorter:
        blbc    r0,7f                   # skip adjustment if r0 is even
        incl    r5                      # adjust digit difference if r0 is odd
7:      extzv   $1,$4,r5,r5             # convert digit count to byte count
        beql    equal_length            # skip loop if no entire bytes in excess
 #	mark_point	cmppx_accvio_b
	 .text	2
	 .set	table_size,table_size + 1
	 .word	Lcmppx_accvio_b - module_base
	 .text	3
	 .word	cmppx_accvio - module_base
	 .text
Lcmppx_accvio_b:
8:      tstb    (r3)+                   # test excess src2 digits for nonzero
        bneq    src1_smaller            # all done if nonzero. src1 lss src2
        sobgtr  r5,8b                    # test for end of loop

        brb     equal_length            # enter loop that performs comparison

 # this code executes when src2 is shorter than src1. that is, r2 lssu r0.
 # the large comment at the beginning of this module explains the need for the
 # incl r5 instruction when r2, the length of the shorter string, is odd.

src2_shorter:
        movl    r2,r0                   # r0 contains number of remaining digits
        mnegl   r5,r5                   # make difference positive
        blbc    r2,9f                   # skip adjustment if r2 is even
        incl    r5                      # adjust digit difference if r2 is odd
9:      extzv   $1,$4,r5,r5             # convert digit count to byte count
        beql    equal_length            # skip loop if no entire bytes in excess
 #	mark_point	cmppx_accvio_c
	 .text	2
	 .set	table_size,table_size + 1
	 .word	Lcmppx_accvio_c - module_base
	 .text	3
	 .word	cmppx_accvio - module_base
	 .text
Lcmppx_accvio_c:
0:      tstb    (r1)+                   # test excess src1 digits for nonzero
        bneq    src2_smaller            # all done if nonzero. src2 lss src1
        sobgtr  r5,0b	                # test for end of loop

 # all excess digits are zero. we must now perform a digit-by-digit comparison
 # of the remaining digits in the two strings. r0 contains the remaining number
 # of digits in either string.

equal_length:
        extzv   $1,$4,r0,r0             # convert digit count to byte count
        beql    2f                      # all done if no digits remain
 #	mark_point	cmppx_accvio_d
	 .text	2
	 .set	table_size,table_size + 1
	 .word	Lcmppx_accvio_d - module_base
	 .text	3
	 .word	cmppx_accvio - module_base
	 .text
Lcmppx_accvio_d:
1:      cmpb    (r1)+,(r3)+             # compare next two digits
        bneq    not_equal               # comparison complete if not equal
        sobgtr  r0,1b                   # test for end of loop

 # compare least significant digit in source and destination strings

 #	mark_point	cmppx_accvio_e
	 .text	2
	 .set	table_size,table_size + 1
	 .word	Lcmppx_accvio_e - module_base
	 .text	3
	 .word	cmppx_accvio - module_base
	 .text
Lcmppx_accvio_e:
2:      bicb3   $0x0f,(r1),r1           # strip sign from last src1 digit
 #	mark_point	cmppx_accvio_f
	 .text	2
	 .set	table_size,table_size + 1
	 .word	Lcmppx_accvio_f - module_base
	 .text	3
	 .word	cmppx_accvio - module_base
	 .text
Lcmppx_accvio_f:
        bicb3   $0x0f,(r3),r3           # strip sign from last src2 digit
        cmpb    r1,r3                   # compare least significant digits
        bneq    not_equal

 # at this point, all tests have been exhausted and the two strings have
 # been shown to be equal. set the z-bit, clear the remaining condition
 # codes, and restore saved registers.

src1_eql_src2:
        movzbl  $psl$m_z,r2             # set condition codes for src1 eql src2

 # this is the common exit path. r2 contains the appropriate settings for the
 # n- and z-bits. there is no other expected input at this point.

cmppx_exit:
        clrl    (sp)                    # set saved r0 to 0
        clrl    8(sp)                   # set saved r2 to 0
        bicpsw  $(psl$m_n|psl$m_z|psl$m_v|psl$m_c)      # start with clean slate
        bispsw  r2                      # set n- and z-bits as appropriate
 #      popr    $^m<r0,r1,r2,r3,r4,r5,r10>  # restore saved registers
	 popr	$0x043f
        rsb                             # return

 # the following code executes if specific digits in the two strings have
 # tested not equal. separate pieces of code are selected for the two
 # different cases of not equal. note that unsigned comparisons are required
 # here because the decimal digits "8" and "9", when appearing in the high
 # nibble, can cause the sign bit to be set.

not_equal:
        blssu   src1_smaller            # branch if src1 is smaller than src2

 # the src2 string has a smaller magnitude than the src1 string. the setting
 # of the signs determines how this transforms to a signed comparison. that is,
 # if both input signs are minus, then reverse the sense of the comparison.

src2_smaller:
        bbs     $cmppx_v_src2_minus,r4,src1_smaller_really

 # the src2 string has been determined to be smaller that the src1 string

src2_smaller_really:
        clrl    r2                      # clear both n- and z-bits
        brb     cmppx_exit              # join common exit code

 # the src1 string has a smaller magnitude than the src2 string. the setting
 # of the signs determines how this transforms to a signed comparison. that is,
 # if both input signs are minus, then reverse the sense of the comparison.

src1_smaller:
        bbs     $cmppx_v_src1_minus,r4,src2_smaller_really

 # the src1 string has been determined to be smaller that the src2 string

src1_smaller_really:
        movb    $psl$m_n,r2             # clear both n- and z-bits
        brb     cmppx_exit              # join common exit code

 # the following code executes if the two input strings have different
 # signs. we need to determine if a comparison between plus zero and minus
 # zero is being made, because such a comparison should test equal. we scan
 # first one string and then the other. if we find a nonzero digit anywhere
 # along the way, we immediately exit this test and set the final condition
 # codes such that the "-" string is smaller than the "+" string. if we
 # exhaust both strings without finding a nonzero digit, then we report
 # that the two strings are equal.

minus_zero_check:
        extzv   $1,$4,r0,r0             # convert src1 digit count to byte count
        beql    7f                      # skip loop if only single digit
 #	mark_point	cmppx_accvio_g
	 .text	2
	 .set	table_size,table_size + 1
	 .word	Lcmppx_accvio_g - module_base
	 .text	3
	 .word	cmppx_accvio - module_base
	 .text
Lcmppx_accvio_g:
6:      tstb    (r1)+                   # test next byte for nonzero
        bneq    cmppx_not_zero          # exit loop if nonzero
        sobgtr  r0,6b                   # test for end of loop

 #	mark_point	cmppx_accvio_h
	 .text	2
	 .set	table_size,table_size + 1
	 .word	Lcmppx_accvio_h - module_base
	 .text	3
	 .word	cmppx_accvio - module_base
	 .text
Lcmppx_accvio_h:
7:      bitb    $0x0f0,(r1)             # test least significant digit
        bneq    cmppx_not_zero          # exit if this digit is not zero

 # all digits in src1 are zero. now we must look for nonzero digits in src2.

        extzv   $1,$4,r2,r2             # convert src2 digit count to byte count
        beql    9f                      # skip loop if only single digit
 #	mark_point	cmppx_accvio_i
	 .text	2
	 .set	table_size,table_size + 1
	 .word	Lcmppx_accvio_i - module_base
	 .text	3
	 .word	cmppx_accvio - module_base
	 .text
Lcmppx_accvio_i:
8:      tstb    (r3)+                   # test next byte for nonzero
        bneq    cmppx_not_zero          # exit loop if nonzero
        sobgtr  r2,8b                   # test for end of loop

 #	mark_point	cmppx_accvio_j
	 .text	2
	 .set	table_size,table_size + 1
	 .word	Lcmppx_accvio_j - module_base
	 .text	3
	 .word	cmppx_accvio - module_base
	 .text
Lcmppx_accvio_j:
9:      bitb    $0x0f0,(r3)             # test least significant digit
        beql    src1_eql_src2           # branch if two strings are equal 

 # at least one of the two input strings contains at least one nonzero digit.
 # that knowledge is sufficient to determine the result of the comparison
 # based simply on the two (necessarily different) signs of the input strings.

cmppx_not_zero:
        bbs     $cmppx_v_src2_minus,r4,src2_smaller_really
        brb     src1_smaller_really


 #+
 # functional description:
 #
 #       the destination string specified by the length and  destination  address
 #       operands  is  replaced  by the source string specified by the length and
 #       source address operands.
 #
 # input parameters:
 #
 #       r0 - len.rw             length of input and output decimal strings
 #       r1 - srcaddr.ab         address of input packed decimal string
 #       r3 - dstaddr.ab         address of output packed decimal string
 #
 #       psl<c>                  contains setting of c-bit when movp executed
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
 #       v <- 0
 #       c <- c                          # note that c-bit is preserved!
 #
 # register usage:
 #
 #       this routine uses r0 through r3. the condition codes are recorded
 #       in r2 as the routine executes.
 #
 # notes:
 #
 #       the initial value of the c-bit must be captured (saved in r2) before
 #       any instructions execute that alter the c-bit.
 #-

	.globl 	vax$movp

vax$movp:
        movpsl  r2                      # save initial psl (to preserve c-bit)
	bbc	$(movp_v_fpd + 16),r0,0f # branch if first time
 	extzv	$(movp_v_saved_psw + 16),$movp_s_saved_psw,r0,r2
					# otherwise, replace condition
					# codes with previous settings

 #      roprand_check   r0              # insure that r0 lequ 31
0:	 cmpw	r0,$31
	 blequ	1f
	 brw	decimal_roprand
1:
  	 movzwl	r0,r0
 
 #       pushl   r3                      # save starting addresses of output
 #       pushl   r2                      # ... and input strings. store a
 #       pushl   r1                      # place holder for saved r2.

 # Save the starting addressess of the input and output strings in addition to
 # the digit count operand (initial R0 contents.) Store a place holder for
 # saved R2.

 #	pushr	$^m<r0,r1,r2,r3,r10>	# Save initial register contents
 	 pushr	$0x040f
 #	establish_handler	decimal_accvio
	 movab	decimal_accvio,r10

        insv    $(psl$m_z>>-1),$1,$3,r2  # set z-bit. clear n- and v-bits.
        extzv   $1,$4,r0,r0             # convert digit count to byte count
        beql    3f                     # skip loop if zero or one digit

 #	mark_point	movp_accvio
	 .text	2
	 .set	table_size,table_size + 1
	 .word	Lmovp_accvio - module_base
	 .text	3
	 .word	movp_accvio - module_base
	 .text
Lmovp_accvio:
1:      movb    (r1)+,(r3)+             # move next two digits
        beql    2f                      # leave z-bit alone if both zero
        bicb2   $psl$m_z,r2             # otherwise, clear saved z-bit
2:      sobgtr  r0,1b                   # check for end of loop

 # the last byte must be processed in a special way. the digit must be checked
 # for nonzero because that affects the condition codes. the sign must be
 # transformed into the preferred form. the n-bit must be set if the input
 # is negative, but cleared in the case of negative zero.

 #	mark_point	movp_accvio_a
	 .text	2
	 .set	table_size,table_size + 1
	 .word	Lmovp_accvio_a - module_base
	 .text	3
	 .word	movp_accvio - module_base
	 .text
Lmovp_accvio_a:
3:      movb    (r1),r0                 # get last input byte (r1 now scratch)
        bitb    $0x0f0,r0               # is digit nonzero?
        beql    4f                      # branch if zero
        bicb2    $psl$m_z,r2             # otherwise, clear saved z-bit  
4:      bicb3   $0x0f0,r0,r1            # sign "digit" to r1

 # assume that the sign is "+". if the input sign is minus, one of the several
 # fixups that must be done is to change the output sign from "+" to "-".

        insv    $12,$0,$4,r0            # 12 is preferred plus sign
        caseb   r1,$10,$15-10		# dispatch on sign type
L4:
        .word   6f-L4                   # 10 => +
        .word   5f-L4                   # 11 => -
        .word   6f-L4                   # 12 => +
        .word   5f-L4                   # 13 => -
        .word   6f-L4                   # 14 => +
        .word   6f-L4                   # 15 => +

 # input sign is "-"

5:      bbs     $psl$v_z,r2,6f          # treat as "+" if negative zero
        incl    r0                      # 13 is preferred minus sign
        bisb2    $psl$m_n,r2             # set n-bit

 # input sign is "+" or input is negative zero. nothing special to do.

 #	mark_point	movp_accvio_b
	 .text	2
	 .set	table_size,table_size + 1
	 .word	Lmovp_accvio_b - module_base
	 .text	3
	 .word	movp_accvio - module_base
	 .text
Lmovp_accvio_b:
6:      movb    r0,(r3)                 # move modified final digit
        clrl    (sp)                    # r0 and r2 must be zero on output
        clrl    8(sp)                   # ... but need r2 so clear saved r2
        bicpsw  $(psl$m_n|psl$m_z|psl$m_v|psl$m_c)      # clear all codes
        bispsw  r2                      # reset codes as appropriate
 #      popr    $^m<r0,r1,r2,r3,r10>    # restore saved registers
	 popr	$0x40f
        rsb                             # return

 #+
 # functional description:
 #
 #       this routine strips leading (high-order) zeros from a packed decimal 
 #       string. the routine exists based on two assumptions.
 #
 #       1.  many of the decimal strings that are used in packed decimal 
 #           operations have several leading zeros.
 #
 #       2.  the operations that are performed on a byte containing packed 
 #           decimal digits are more complicated that the combination of this 
 #           routine and any special end processing that occurs in the various 
 #           vax$xxxxxx routines when a string is exhausted.
 #
 #       this routine exists as a performance enhancement. as such, it can only
 #       succeed if it is extremely efficient. it does not attempt to be
 #       rigorous in squeezing every last zero out of a string. it eliminates
 #       only entire bytes that contain two zero digits. it does not look for a
 #       leading zero in the high order nibble of a string of odd length. 
 #
 #       the routine also assumes that the input decimal strings are well 
 #       formed. if an even-length decimal string does not have a zero in its 
 #       unused high order nibble, then no stripping takes place, even though 
 #       the underlying vax$xxxxxx routine may work correctly. 
 #
 #	 (The comment in the next four lines is preserved for its historical
 #	  content.)
 #
 #       finally, there is no explicit test for the end of the string. the 
 #       routine assumes that the low order byte, the one that contains the 
 #       sign, is not equal to zero. This can cause rather strange behavior
 #	 ( read UNPREDICTABLE ) for poorly formed decimal strings.
 #
 #	 (The following comment describes the revised treatment of certain
 #	  froms of illegal packed decimal strings.)
 #
 #	 although an end-of-string test is not required for well formed packed
 #	 decimal strings, it turns out that some layered products create packed
 #	 decimal data on the fly consisting of so many bytes containing zero. 
 #	 in other words, the sign nibble contains zero.  Previous 
 #	 implementations of packed decimal zero.
 #
 #	 The bleq 30$ instructions that exist in the following two loops detect
 #	 these strings and treat them as strings with a digit count of one.
 #	 (The digit itself is zero.)  Whether this string is treated as +0 or
 #	 -0 is determine by the caller of this routine.  That much 
 #	 unpredictable behavior remains in the treatment of these illegal
 #	 strings.
 #
 # input and output parameters:
 #
 #       there are really two identical but separate routines here. one is
 #       used when the input decimal string descriptor is in r0 and r1. the
 #       other is used when r2 and r3 describe the decimal string. note that
 #       we have already performed the reserved operand checks so that r0 (or
 #       r2) is guaranteed lequ 31.
 #
 #       if the high order digit of an initially even length string is zero,
 #       then the digit count (r0 or r2) is reduced by one. for all other
 #       cases, the digit count is reduced by two as an entire byte of zeros
 #       is skipped.
 #
 # input parameters (for entry at decimal$strip_zeros_r0_r1):
 #
 #       r0<4:0> - len.rw        length of input decimal string
 #       r1      - addr.ab       address of input packed decimal string
 #
 # output parameters (for entry at decimal$strip_zeros_r0_r1):
 #
 #       r1      advanced to first nonzero byte in string
 #       r0      reduced accordingly (note that if r0 is altered at all,
 #               then r0 is always odd on exit.)
 #
 # input parameters (for entry at decimal$strip_zeros_r2_r3):
 #
 #       r2<4:0> - len.rw        length of input decimal string
 #       r3      - addr.ab       address of input packed decimal string
 #
 # output parameters (for entry at decimal$strip_zeros_r2_r3):
 #
 #       r3      advanced to first nonzero byte in string
 #       r2      reduced accordingly (note that if r2 is altered at all,
 #               then r2 is always odd on exit.)
 # note:
 #
 #	although these routines can generate access violations, there is no
 #	mark_point here because these routines can be called from other
 #	modules (and are not called by the routines in this module). the pc
 #	check is made based on the return pc from this subroutine rather than
 #	on the pc of the instruction that accessed the inaccessible address. 
 #-

 # this routine is used when the decimal string is described by r0 (digit
 # count) and r1 (string address).

	.globl	decimal$strip_zeros_r0_r1

decimal$strip_zeros_r0_r1:
        blbs    r0,1f                   # skip first check if r0 starts out odd
        tstb    (r1)+                   # is first byte zero?
        bneq    2f                      # all done if not
        decl    r0                      # skip leading zero digit (r0 nequ 0)

1:      tstb    (r1)+                   # is next byte zero?
        bneq    2f                      # all done if not       
        subl2   $2,r0                   # decrease digit count by 2
	bleq	3f			# We passed the end of the string
        brb     1b                      # ... and charge on

2:      decl    r1                      # back up r1 to last nonzero byte
        rsb

3:	addl2	$2,r0			# undo last r0 modification
	brb	2b			# ... and take common exit

 # this routine is used when the decimal string is described by r2 (digit
 # count) and r3 (string address).

	.globl	decimal$strip_zeros_r2_r3

decimal$strip_zeros_r2_r3:
        blbs    r2,1f                   # skip first check if r2 starts out odd
        tstb    (r3)+                   # is first byte zero?
        bneq    2f                      # all done if not
        decl    r2                      # skip leading zero digit (r2 nequ 0)

1:      tstb    (r3)+                   # is next byte zero?
        bneq    2f                      # all done if not       
        subl2    $2,r2                   # decrease digit count by 2
	bleq	3f			# We passed the end of the string
        brb     1b                      # ... and charge on

2:      decl    r3                      # back up r3 to last nonzero byte
        rsb

3:	addl2	$2,r2			# undo last r2 modification
	brb	2b			# ... and take common exit
 #
 #-
 # functional description:
 #
 #	this routine receives control when a digit count larger than 31
 #	is detected. the exception is architecturally defined as an
 #	abort so there is no need to store intermediate state. because
 #	all of the routines in this module check for legal digit counts
 #	before saving any registers, this routine simply passes control
 #	to vax$roprand.
 #
 # input parameters:
 #
 #	0(sp) - return pc from vax$xxxxxx routine
 #
 # output parameters:
 #
 #	0(sp) - offset in packed register array to delta pc byte
 #	4(sp) - return pc from vax$xxxxxx routine
 #
 # implicit output:
 #
 #	this routine passes control to vax$roprand where further
 #	exception processing takes place.
 #-


decimal_roprand:
	pushl	$movp_b_delta_pc	# store offset to delta pc byte
	jmp	vax$roprand		# pass control along


 #+
 # functional description:
 #
 #	this routine receives control when an access violation occurs while
 #	executing within the emulator routines for cmpp3, cmpp4 or movp.
 #
 #	the routine header for ashp_accvio in module vax$ashp contains a
 #	detailed description of access violation handling for the decimal
 #	string instructions. 
 #
 # input parameters:
 #
 #	see routine ashp_accvio in module vax$ashp
 #
 # output parameters:
 #
 #	see routine ashp_accvio in module vax$ashp
 #-

decimal_accvio:
	clrl	r2			# initialize the counter
	pushab	module_base		# store base address of this module
	subl2	(sp)+,r1		# get pc relative to this base

1:	cmpw	r1,pc_table_base[r2]	# is this the right pc?
	beql	3f			# exit loop if true
	aoblss	$table_size,r2,1b	# do the entire table

 # if we drop through the dispatching based on pc, then the exception is not 
 # one that we want to back up. we simply reflect the exception to the user.

2:
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
 # functional description:
 #
 #	it is trivial to back out cmpp3 and cmpp4 because neither of these 
 #	routines uses any stack space (other than saved register space). the 
 #	only reason that this routine does not use the common 
 #	vax$decimal_accvio exit path is that fewer registers are saved by 
 #	these two routines than are saved by the typical packed decimal 
 #	emulation routine.
 #
 # input parameters:
 #
 #	r0 - address of top of stack when access violation occurred
 #
 #	00(r0) - saved r0 on entry to vax$cmppx
 #	04(r0) - saved r1
 #	08(r0) - saved r2
 #	12(r0) - saved r3
 #	16(r0) - saved r4
 #	20(r0) - saved r5
 #	24(r0) - saved r10
 #	28(r0) - return pc from vax$cmppx routine
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
 #	00(r0) - return pc from vax$cmppx routine
 #	
 #	00(sp) - value of r0 on entry to vax$cmppx
 #	04(sp) - value of r1 on entry to vax$cmppx
 #	08(sp) - value of r2 on entry to vax$cmppx
 #	12(sp) - value of r3 on entry to vax$cmppx
 #
 #	r4, r5, and r10 are restored to their values on entry to vax$cmppx.
 #-

	.globl	cmppx_accvio

cmppx_accvio:
	movq	(r0)+,pack_l_saved_r0(sp)	# "restore" r0 and r1
	movq	(r0)+,pack_l_saved_r2(sp)	# "restore" r2 and r3
	movq	(r0)+,r4			# really restore r4 and r5

 # the last two instructions can be shared with movp_accvio, provided that
 # the following assumptions hold.


	brb	1f			# share remainder with movp_accvio

 #+
 # functional description:
 #
 #	it is almost too trivial to back out vax$movp to its starting point. 
 #	if time permits, we will add restart points to this routine. this will 
 #	illustrate how one could go about adding restart capability to other 
 #	decimal instructions, allowing the routines to pick up where they left 
 #	off if an access violation occurs. this will also point out the 
 #	magnitude of the task by showing the amount of intermediate state that 
 #	must be saved for even so simple a routine as vax$movp.
 #
 #	the vax$movp routine, like vax$cmppx, uses no stack space. it also 
 #	saves only a subset of the registers and so a special exit path must 
 #	be taken to vax$reflect_fault.
 #
 # input parameters:
 #
 #	r0 - address of top of stack when access violation occurred
 #
 #	00(r0) - saved r0 on entry to vax$movp
 #	04(r0) - saved r1
 #	08(r0) - saved r2
 #	12(r0) - saved r3
 #	16(r0) - saved r10
 #	20(r0) - return pc from vax$movp routine
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
 #	00(r0) - return pc from vax$movp routine
 #	
 #	00(sp) - value of r0 on entry to vax$movp
 #	04(sp) - value of r1 on entry to vax$movp
 #	08(sp) - value of r2 on entry to vax$movp
 #	12(sp) - value of r3 on entry to vax$movp
 #
 #	r10 is restored to its value on entry to vax$movp.
 #-

	.globl	movp_accvio
movp_accvio:
	movq	(r0)+,pack_l_saved_r0(sp)	# "restore" r0 and r1
	movq	(r0)+,pack_l_saved_r2(sp)	# "restore" r2 and r3
	bisb3	$movp_m_fpd,-8(r0),movp_b_state(sp)	# preserved saved c-bit
1:	movl	(r0)+,r10			# really restore r10 

 #	movl	$<movp_b_delta_pc!-	# indicate offset for delta pc
 #		pack_m_fpd!-		# fpd bit should be set
 #		pack_m_accvio>,r1	# this is an access violation
	 movl	$(movp_b_delta_pc|pack_m_fpd|pack_m_accvio),r1
	jmp	vax$reflect_fault	# continue exception handling

