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
/*	@(#)vaxregdef.h	1.3		11/2/84		*/



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
 * 000- This moudule is used by the emulation code. It defines most
 *	most of the constatns that are required by the emulation code
 *
 ***********************************************************************/
 
 #	.macro	movtc_def,..equ=<=>,..col=<:>
 #  define register usage for the movtc instruction
 # 
 #      31               23               15               07            00
 #     +----------------+----------------+----------------+----------------+
 #     |         initial srclen          |             srclen              | : r0
 #     +----------------+----------------+----------------+----------------+
 #     |                              srcaddr                              | : r1
 #     +----------------+----------------+----------------+----------------+
 #     |    delta-pc    |     xxxx       |     flags      |      fill      | : r2
 #     +----------------+----------------+----------------+----------------+
 #     |                              tbladdr                              | : r3
 #     +----------------+----------------+----------------+----------------+
 #     |         initial dstlen          |             dstlen              | : r4
 #     +----------------+----------------+----------------+----------------+
 #     |                              dstaddr                              | : r5
 #     +----------------+----------------+----------------+----------------+
# define movtc_m_fpd 1
# define movtc_s_movtc 24
 # define movtc 0
# define movtc_w_srclen 0                  
					#  srclen.rw
# define movtc_w_inisrclen 2
# define movtc_a_srcaddr 4                 
					#  srcaddr.ab
# define movtc_b_fill 8                    
					#  fill.rb
# define movtc_b_flags 9
# define movtc_v_fpd 0
# define movtc_b_delta_pc 11
# define movtc_a_tbladdr 12                
					#  tbladdr.ab
# define movtc_w_dstlen 16                 
					#  dstlen.rw
# define movtc_w_inidstlen 18
# define movtc_a_dstaddr 20                
					#  dstaddr.ab
 #	.endm
 
 #	.macro	movtuc_def,..equ=<=>,..col=<:>
 #  define register usage for the movtuc instruction
 # 
 #      31               23               15               07            00
 #     +----------------+----------------+----------------+----------------+
 #     |         initial srclen          |             srclen              | : r0
 #     +----------------+----------------+----------------+----------------+
 #     |                              srcaddr                              | : r1
 #     +----------------+----------------+----------------+----------------+
 #     |    delta-pc    |     xxxx       |     flags      |      esc       | : r2
 #     +----------------+----------------+----------------+----------------+
 #     |                              tbladdr                              | : r3
 #     +----------------+----------------+----------------+----------------+
 #     |         initial dstlen          |             dstlen              | : r4
 #     +----------------+----------------+----------------+----------------+
 #     |                              dstaddr                              | : r5
 #     +----------------+----------------+----------------+----------------+
# define movtuc_m_fpd 1
# define movtuc_s_movtuc 24
 # define movtuc 0
# define movtuc_w_srclen 0                 
					#  srclen.rw
# define movtuc_w_inisrclen 2
# define movtuc_a_srcaddr 4                
					#  srcaddr.ab
# define movtuc_b_esc 8                    
					#  esc.rb
# define movtuc_b_flags 9
# define movtuc_v_fpd 0
# define movtuc_b_delta_pc 11
# define movtuc_a_tbladdr 12               
					#  tbladdr.ab
# define movtuc_w_dstlen 16                
					#  dstlen.rw
# define movtuc_w_inidstlen 18
# define movtuc_a_dstaddr 20               
					#  dstaddr.ab
 #	.endm
 
 #	.macro	cmpc3_def,..equ=<=>,..col=<:>
 #  define register usage for the cmpc3 instruction
 # 
 #      31               23               15               07            00
 #     +----------------+----------------+----------------+----------------+
 #     |    delta-pc    |      xxxx      |               len               | : r0
 #     +----------------+----------------+----------------+----------------+
 #     |                             src1addr                              | : r1
 #     +----------------+----------------+----------------+----------------+
 #     |                               xxxxx                               | : r2
 #     +----------------+----------------+----------------+----------------+
 #     |                             src2addr                              | : r3
 #     +----------------+----------------+----------------+----------------+
# define cmpc3_s_cmpc3 16
 # define cmpc3 0
# define cmpc3_w_len 0                     
					#  len.rw
# define cmpc3_b_delta_pc 3
# define cmpc3_a_src1addr 4                
					#  src1addr.ab
# define cmpc3_a_src2addr 12               
					#  src2addr.ab
 #	.endm
 
 #	.macro	cmpc5_def,..equ=<=>,..col=<:>
 #  define register usage for the cmpc5 instruction
 # 
 # 
 #      31               23               15               07            00
 #     +----------------+----------------+----------------+----------------+
 #     |    delta-pc    |      fill      |             src1len             | : r0
 #     +----------------+----------------+----------------+----------------+
 #     |                             src1addr                              | : r1
 #     +----------------+----------------+----------------+----------------+
 #     |              xxxxx              |             src2len             | : r2
 #     +----------------+----------------+----------------+----------------+
 #     |                             src2addr                              | : r3
 #     +----------------+----------------+----------------+----------------+
# define cmpc5_s_cmpc5 16
 # define cmpc5 0
# define cmpc5_w_src1len 0                 
					#  src1len.rw
# define cmpc5_b_fill 2                    
					#  fill.rb
# define cmpc5_b_delta_pc 3
# define cmpc5_a_src1addr 4                
					#  src1addr.ab
# define cmpc5_w_src2len 8                 
					#  src2len.rw
# define cmpc5_a_src2addr 12               
					#  src2addr.ab
 #	.endm
 
 #	.macro	scanc_def,..equ=<=>,..col=<:>
 #  define register usage for the scanc instruction
 # 
 #      31               23               15               07            00
 #     +----------------+----------------+----------------+----------------+
 #     |    delta-pc    |      xxxx      |               len               | : r0
 #     +----------------+----------------+----------------+----------------+
 #     |                               addr                                | : r1
 #     +----------------+----------------+----------------+----------------+
 #     |              xxxxx              |      xxxx      |      mask      | : r2
 #     +----------------+----------------+----------------+----------------+
 #     |                              tbladdr                              | : r3
 #     +----------------+----------------+----------------+----------------+
# define scanc_s_scanc 16
 # define scanc 0
# define scanc_w_len 0                     
					#  len.rw
# define scanc_b_delta_pc 3
# define scanc_a_addr 4                    
					#  addr.ab
# define scanc_b_mask 8                    
					#  mask.rb
# define scanc_a_tbladdr 12                
					#  tbladdr.ab
 #	.endm
 
 #	.macro	spanc_def,..equ=<=>,..col=<:>
 #  define register usage for the spanc instruction
 # 
 #      31               23               15               07            00
 #     +----------------+----------------+----------------+----------------+
 #     |    delta-pc    |      xxxx      |               len               | : r0
 #     +----------------+----------------+----------------+----------------+
 #     |                               addr                                | : r1
 #     +----------------+----------------+----------------+----------------+
 #     |              xxxxx              |      xxxx      |      mask      | : r2
 #     +----------------+----------------+----------------+----------------+
 #     |                              tbladdr                              | : r3
 #     +----------------+----------------+----------------+----------------+
# define spanc_s_spanc 16
 # define spanc 0
# define spanc_w_len 0                     
					#  len.rw
# define spanc_b_delta_pc 3
# define spanc_a_addr 4                    
					#  addr.ab
# define spanc_b_mask 8                    
					#  mask.rb
# define spanc_a_tbladdr 12                
					#  tbladdr.ab
 #	.endm
 
 #	.macro	locc_def,..equ=<=>,..col=<:>
 #  define register usage for the locc instruction
 # 
 #      31               23               15               07            00
 #     +----------------+----------------+----------------+----------------+
 #     |    delta-pc    |      char      |               len               | : r0
 #     +----------------+----------------+----------------+----------------+
 #     |                               addr                                | : r1
 #     +----------------+----------------+----------------+----------------+
# define locc_s_locc 8
 # define locc 0
# define locc_w_len 0                      
					#  len.rw
# define locc_b_char 2                     
					#  char.rb
# define locc_b_delta_pc 3
# define locc_a_addr 4                     
					#  addr.ab
 #	.endm
 
 #	.macro	skpc_def,..equ=<=>,..col=<:>
 #  define register usage for the skpc instruction
 # 
 #      31               23               15               07            00
 #     +----------------+----------------+----------------+----------------+
 #     |    delta-pc    |      char      |               len               | : r0
 #     +----------------+----------------+----------------+----------------+
 #     |                               addr                                | : r1
 #     +----------------+----------------+----------------+----------------+
# define skpc_s_skpc 8
 # define skpc 0
# define skpc_w_len 0                      
					#  len.rw
# define skpc_b_char 2                     
					#  char.rb
# define skpc_b_delta_pc 3
# define skpc_a_addr 4                     
					#  addr.ab
 #	.endm
 
 #	.macro	matchc_def,..equ=<=>,..col=<:>
 #  define register usage for the matchc instruction
 # 
 #      31               23               15               07            00
 #     +----------------+----------------+----------------+----------------+
 #     |    delta-pc    |      xxxx      |             objlen              | : r0
 #     +----------------+----------------+----------------+----------------+
 #     |                              objaddr                              | : r1
 #     +----------------+----------------+----------------+----------------+
 #     |              xxxxx              |             srclen              | : r2
 #     +----------------+----------------+----------------+----------------+
 #     |                              srcaddr                              | : r3
 #     +----------------+----------------+----------------+----------------+
# define matchc_s_matchc 16
 # define matchc 0
# define matchc_w_objlen 0                 
					#  objlen.rw
# define matchc_b_delta_pc 3
# define matchc_a_objaddr 4                
					#  objaddr.ab
# define matchc_w_srclen 8                 
					#  srclen.rw
# define matchc_a_srcaddr 12               
					#  srcaddr.ab
 #	.endm
 
 #	.macro	crc_def,..equ=<=>,..col=<:>
 #  define register usage for the crc instruction
 # 
 #      31               23               15               07            00
 #     +----------------+----------------+----------------+----------------+
 #     |                              inicrc                               | : r0
 #     +----------------+----------------+----------------+----------------+
 #     |                                tbl                                | : r1
 #     +----------------+----------------+----------------+----------------+
 #     |    delta-pc    |      xxxx      |             strlen              | : r2
 #     +----------------+----------------+----------------+----------------+
 #     |                              stream                               | : r3
 #     +----------------+----------------+----------------+----------------+
# define crc_s_crc 16
 # define crc 0
# define crc_l_inicrc 0                    
					#  inicrc.rl
# define crc_a_tbl 4                       
					#  tbl.ab
# define crc_w_strlen 8                    
					#  strlen.rw
# define crc_b_delta_pc 11
# define crc_a_stream 12                   
					#  stream.ab
 #	.endm
 
 #	.macro	stack_def,..equ=<=>,..col=<:>
 #	Define exception parameters for vax$_opcdec exception, the exception
 #	that occurs when an unimplemented instructions on a microvax processor.
 # 
 #      31           23           15           07        00
 #     +------------+------------+------------+------------+
 #     |             opcode of reserved instruction        | : 00(sp)
 #     +------------+------------+------------+------------+
 #     |             pc of reserved instruction (old pc)   | : 04(sp)
 #     +------------+------------+------------+------------+
 #     |             first operand specifier               | : 08(sp)
 #     +------------+------------+------------+------------+
 #     |             second operand specifier              | : 12(sp)
 #     +------------+------------+------------+------------+
 #     |             third operand specifier               | : 16(sp)
 #     +------------+------------+------------+------------+
 #     |             fourth operand specifier              | : 20(sp)
 #     +------------+------------+------------+------------+
 #     |             fifth operand specifier               | : 24(sp)
 #     +------------+------------+------------+------------+
 #     |             sixth operand specifier               | : 28(sp)
 #     +------------+------------+------------+------------+
 #     |             seventh operand specifier             | : 32(sp)
 #     +------------+------------+------------+------------+
 #     |             eighth operand specifier              | : 36(sp)
 #     +------------+------------+------------+------------+
 #     |             pc of next instruction (new pc)       | : 40(sp)
 #     +------------+------------+------------+------------+
 #     |             psl at time of exception              | : 44(sp)
 #     +------------+------------+------------+------------+
# define s_stack 48
# define stack 0
# define opcode 0                          
					#  opcode of reserved instruction
# define old_pc 4                          
					#  pc of reserved instruction (old pc)
# define operand_1 8                       
					#  first operand specifier
# define operand_2 12                      
					#  second operand specifier
# define operand_3 16                      
					#  third operand specifier
# define operand_4 20                      
					#  fourth operand specifier
# define operand_5 24                      
					#  fifth operand specifier
# define operand_6 28                      
					#  sixth operand specifier
# define operand_7 32                      
					#  seventh operand specifier 
# define operand_8 36                      
					#  eight operand specifier 
# define new_pc 40                         
					#  pc of instruction following 
 #   reserved instruction (new pc)
# define exception_psl 44                  
					#  psl at time of exception
 #	.endm
 
 #	.macro	pack_def,..equ=<=>,..col=<:>
 #  define stack usage when packing registers to back up instruction
 # 
 #      31           23           15           07        00
 #     +------------+------------+------------+------------+
 #     |                     saved r0                      | : 00(sp)
 #     +------------+------------+------------+------------+
 #     |                     saved r1                      | : 04(sp)
 #     +------------+------------+------------+------------+
 #     |                     saved r2                      | : 08(sp)
 #     +------------+------------+------------+------------+
 #     |                     saved r3                      | : 12(sp)
 #     +------------+------------+------------+------------+
 # 
 #  the next longword after the saved r3 depends on the context. 
 # 
 #  in the case of software generated exceptions, a signal array sits
 #  immediately underneath the saved register array. 
 # 
 #     +------------+------------+------------+------------+
 #     |           first longword of signal array          | : 16(sp)
 #     +------------+------------+------------+------------+
 # 
 #  in the case of an access 
 #  violation, there is a return pc followed by an argument list.
 # 
 #     +------------+------------+------------+------------+
 #     |                     return pc                     | : 16(sp)
 #     +------------+------------+------------+------------+
 #     |             argument count (always 2)             | : 20(sp)
 #     +------------+------------+------------+------------+
 #     |              pointer to signal array              | : 24(sp)
 #     +------------+------------+------------+------------+
 #     |             pointer to mechanism array            | : 28(sp)
 #     +------------+------------+------------+------------+
 # 
 #  after the stack has been modified, a new value of sp is inserted here.
 # 
 #     +------------+------------+------------+------------+
 #     |                     saved sp                      | : 16(sp)
 #     +------------+------------+------------+------------+
#define pack_m_fpd 256
#define pack_m_accvio 512
#define pack_s_pack 36
 #define pack 0
#define pack_l_saved_r0 0
#define pack_l_saved_r1 4
#define pack_l_saved_r2 8
#define pack_l_saved_r3 12
#define pack_l_return_pc 16
#define pack_l_signal_array 16
#define pack_l_saved_sp 16
#define pack_l_argument_count 20
#define pack_l_signal_array_pointer 24
#define pack_l_mechanism_array_pointer 28
#define pack_s_delta_pc 8
#define pack_v_delta_pc 0
#define pack_v_fpd 8
	                      #  set fpd bit in psl
#define pack_v_accvio 9 
		              #  stack contains extra data
 #	.endm
 
 #	.macro	addp4_def,..equ=<=>,..col=<:>
 #  define register usage for the addp4 instruction
 # 
 #      31               23               15               07            00
 #     +----------------+----------------+----------------+----------------+
 #     |    delta-pc    |      xxxx      |             addlen              | : r0
 #     +----------------+----------------+----------------+----------------+
 #     |                              addaddr                              | : r1
 #     +----------------+----------------+----------------+----------------+
 #     |              xxxxx              |             sumlen              | : r2
 #     +----------------+----------------+----------------+----------------+
 #     |                              sumaddr                              | : r3
 #     +----------------+----------------+----------------+----------------+
# define addp4_s_addp4 16
 # define addp4 0
# define addp4_w_addlen 0
# define addp4_s_addlen 5
# define addp4_v_addlen 0
# define addp4_b_delta_pc 3
# define addp4_a_addaddr 4                 
					#  addaddr.ab
# define addp4_w_sumlen 8
# define addp4_s_sumlen 5
# define addp4_v_sumlen 0
# define addp4_a_sumaddr 12                
					#  sumaddr.ab
 #	.endm
 
 #	.macro	addp6_def,..equ=<=>,..col=<:>
 #  define register usage for the addp6 instruction
 # 
 #      31               23               15               07            00
 #     +----------------+----------------+----------------+----------------+
 #     |    delta-pc    |      xxxx      |             add1len             | : r0
 #     +----------------+----------------+----------------+----------------+
 #     |                             add1addr                              | : r1
 #     +----------------+----------------+----------------+----------------+
 #     |              xxxxx              |             add2len             | : r2
 #     +----------------+----------------+----------------+----------------+
 #     |                             add2addr                              | : r3
 #     +----------------+----------------+----------------+----------------+
 #     |              xxxxx              |             sumlen              | : r4
 #     +----------------+----------------+----------------+----------------+
 #     |                              sumaddr                              | : r5
 #     +----------------+----------------+----------------+----------------+
# define addp6_s_addp6 24
 # define addp6 0
# define addp6_w_add1len 0
# define addp6_s_add1len 5
# define addp6_v_add1len 0
# define addp6_b_delta_pc 3
# define addp6_a_add1addr 4                
					#  add1addr.ab
# define addp6_w_add2len 8
# define addp6_s_add2len 5
# define addp6_v_add2len 0
# define addp6_a_add2addr 12               
					#  add2addr.ab
# define addp6_w_sumlen 16
# define addp6_s_sumlen 5
# define addp6_v_sumlen 0
# define addp6_a_sumaddr 20                
					#  sumaddr.ab
 #	.endm
 
 #	.macro	ashp_def,..equ=<=>,..col=<:>
 #  define register usage for the ashp instruction
 # 
 #      31               23               15               07            00
 #     +----------------+----------------+----------------+----------------+
 #     |    delta-pc    |      cnt       |             srclen              | : r0
 #     +----------------+----------------+----------------+----------------+
 #     |                              srcaddr                              | : r1
 #     +----------------+----------------+----------------+----------------+
 #     |      xxxx      |     round      |             dstlen              | : r2
 #     +----------------+----------------+----------------+----------------+
 #     |                              dstaddr                              | : r3
 #     +----------------+----------------+----------------+----------------+
# define ashp_s_ashp 16
 # define ashp 0
# define ashp_w_srclen 0
# define ashp_s_srclen 5
# define ashp_v_srclen 0
# define ashp_b_cnt 2
					#  cnt.rb
# define ashp_b_delta_pc 3
# define ashp_a_srcaddr 4 
					#  srcaddr.ab
# define ashp_w_dstlen 8
# define ashp_s_dstlen 5
# define ashp_v_dstlen 0
# define ashp_b_round 10
# define ashp_s_round 4
# define ashp_v_round 0
# define ashp_a_dstaddr 12
					#  dstaddr.ab
 #	.endm
 
 #	.macro	cmpp3_def,..equ=<=>,..col=<:>
 #  define register usage for the cmpp3 instruction
 # 
 #      31               23               15               07            00
 #     +----------------+----------------+----------------+----------------+
 #     |    delta-pc    |      xxxx      |               len               | : r0
 #     +----------------+----------------+----------------+----------------+
 #     |                             src1addr                              | : r1
 #     +----------------+----------------+----------------+----------------+
 #     |                               xxxxx                               | : r2
 #     +----------------+----------------+----------------+----------------+
 #     |                             src2addr                              | : r3
 #     +----------------+----------------+----------------+----------------+
# define cmpp3_s_cmpp3 16
 # define cmpp3 0
# define cmpp3_w_len 0
# define cmpp3_s_len 5
# define cmpp3_v_len 0
# define cmpp3_b_delta_pc 3
# define cmpp3_a_src1addr 4                
					#  src1addr.ab
# define cmpp3_a_src2addr 12               
					#  src2addr.ab
 #	.endm
 
 #	.macro	cmpp4_def,..equ=<=>,..col=<:>
 #  define register usage for the cmpp4 instruction
 # 
 #      31               23               15               07            00
 #     +----------------+----------------+----------------+----------------+
 #     |    delta-pc    |      xxxx      |             src1len             | : r0
 #     +----------------+----------------+----------------+----------------+
 #     |                             src1addr                              | : r1
 #     +----------------+----------------+----------------+----------------+
 #     |              xxxxx              |             src2len             | : r2
 #     +----------------+----------------+----------------+----------------+
 #     |                             src2addr                              | : r3
 #     +----------------+----------------+----------------+----------------+
# define cmpp4_s_cmpp4 16
 # define cmpp4 0
# define cmpp4_w_src1len 0
# define cmpp4_s_src1len 5
# define cmpp4_v_src1len 0
# define cmpp4_b_delta_pc 3
# define cmpp4_a_src1addr 4                
					#  src1addr.ab
# define cmpp4_w_src2len 8
# define cmpp4_s_src2len 5
# define cmpp4_v_src2len 0
# define cmpp4_a_src2addr 12               
					#  src2addr.ab
 #	.endm
 
 #	.macro	cvtlp_def,..equ=<=>,..col=<:>
 #  define register usage for the cvtlp instruction
 # 
 #      31               23               15               07            00
 #     +----------------+----------------+----------------+----------------+
 #     |                                src                                | : r0
 #     +----------------+----------------+----------------+----------------+
 #     |    state          saved_psw        saved_r5          saved_r4     | : r1
 #     +----------------+----------------+----------------+----------------+
 #     |    delta-pc    |      xxxx      |             dstlen              | : r2
 #     +----------------+----------------+----------------+----------------+
 #     |                              dstaddr                              | : r3
 #     +----------------+----------------+----------------+----------------+
#define cvtlp_m_state 7
#define cvtlp_m_fpd 8
#define cvtlp_s_cvtlp16
# define cvtlp_s_cvtlp 16
 # define cvtlp 0
# define cvtlp_l_src 0                     
					#  src.rl
#define cvtlp_b_saved_r4 4
#define cvtlp_b_saved_r5 5
#define cvtlp_b_saved_psw 6
#define cvtlp_b_state 7
#define cvtlp_s_state 3
#define cvtlp_v_state 0
#define cvtlp_v_fpd 3
# define cvtlp_w_dstlen 8
# define cvtlp_s_dstlen 5
# define cvtlp_v_dstlen 0
# define cvtlp_b_delta_pc 11
# define cvtlp_a_dstaddr 12                
					#  dstaddr.ab
 #	.endm
 
 #	.macro	cvtpl_def,..equ=<=>,..col=<:>
 #  define register usage for the cvtpl instruction
 # 
 #      31               23               15               07            00
 #     +----------------+----------------+----------------+----------------+
 #     |    delta-pc    |      state     |             srclen              | : r0
 #     +----------------+----------------+----------------+----------------+
 #     |                              srcaddr                              | : r1
 #     +----------------+----------------+----------------+----------------+
 #     |                               result                              | : r2
 #     +----------------+----------------+----------------+----------------+
 #     |                                dst                                | : r3
 #     +----------------+----------------+----------------+----------------+
#define cvtpl_m_saved 15
#define cvtpl_m_state 112
#define cvtpl_m_fpd 128
# define cvtpl_s_cvtpl 16
 # define cvtpl 0
# define cvtpl_w_srclen 0
# define cvtpl_b_srclen 0
# define cvtpl_s_srclen 5
# define cvtpl_v_srclen 0
# define cvtpl_b_delta_srcaddr 1
# define cvtpl_b_state 2
# define cvtpl_s_saved_psw 4
# define cvtpl_v_saved_psw 0
# define cvtpl_s_state 3
# define cvtpl_v_state 4
# define cvtpl_v_fpd 7
# define cvtpl_b_delta_pc 3
# define cvtpl_a_srcaddr 4                 
					#  srcaddr.ab
# define cvtpl_l_result 8
# define cvtpl_a_dst 12                    
					#  dst.wl
 #	.endm
 
 #	.macro	cvtps_def,..equ=<=>,..col=<:>
 #  define register usage for the cvtps instruction
 # 
 #      31               23               15               07            00
 #     +----------------+----------------+----------------+----------------+
 #     |    delta-pc    |      xxxx      |             srclen              | : r0
 #     +----------------+----------------+----------------+----------------+
 #     |                              srcaddr                              | : r1
 #     +----------------+----------------+----------------+----------------+
 #     |              xxxxx              |             dstlen              | : r2
 #     +----------------+----------------+----------------+----------------+
 #     |                              dstaddr                              | : r3
 #     +----------------+----------------+----------------+----------------+
# define cvtps_s_cvtps 16
 # define cvtps 0
# define cvtps_w_srclen 0
# define cvtps_s_srclen 5
# define cvtps_v_srclen 0
# define cvtps_b_delta_pc 3
# define cvtps_a_srcaddr 4                 
					#  srcaddr.ab
# define cvtps_w_dstlen 8
# define cvtps_s_dstlen 5
# define cvtps_v_dstlen 0
# define cvtps_a_dstaddr 12                
					#  dstaddr.ab
 #	.endm
 
 #	.macro	cvtpt_def,..equ=<=>,..col=<:>
 #  define register usage for the cvtpt instruction
 # 
 #      31               23               15               07            00
 #     +----------------+----------------+----------------+----------------+
 #     |             dstlen              |             srclen              | : r0
 #     +----------------+----------------+----------------+----------------+
 #     |                              srcaddr                              | : r1
 #     +----------------+----------------+----------------+----------------+
 #     |                              tbladdr                              | : r2
 #     +----------------+----------------+----------------+----------------+
 #     |                              dstaddr                              | : r3
 #     +----------------+----------------+----------------+----------------+
 # 
 #     there is no spare byte to store the delta-pc quantity with three
 #     addresses to be stored in four registers. the delta-pc quantity is
 #     stored in the upper byte of the dstlen cell. once the destination length
 #     has been checked for a legal range (a decimal string length larger than
 #     31 causes a reserved operand abort), that length can be easily stored in
 #     five bytes, leaving the upper byte free. 
 # 
 # 
 #                       31            24       20      16
 #                      +----------------+-----+----------+
 #                      |             dstlen              |
 #                      +----------------+-----+----------+
 #                     /                /       \          \
 #                    /                /         \          \
 #                   +----------------+           +----------+
 #                   |    delta-pc    |           |  dstlen  |
 #                   +----------------+           +----------+
 #                    31            24             20      16
# define cvtpt_s_cvtpt 16
 # define cvtpt 0
# define cvtpt_w_srclen 0
# define cvtpt_s_srclen 5
# define cvtpt_v_srclen 0
# define cvtpt_v_fpd 15
# define cvtpt_w_dstlen 2
# define cvtpt_b_dstlen 2
# define cvtpt_s_dstlen 5
# define cvtpt_v_dstlen 0
# define cvtpt_b_delta_pc 3
# define cvtpt_a_srcaddr 4                 
					#  srcaddr.ab
# define cvtpt_a_tbladdr 8                 
					#  tbladdr.ab
# define cvtpt_a_dstaddr 12                
					#  dstaddr.ab
 #	.endm
 
 #	.macro	cvtsp_def,..equ=<=>,..col=<:>
 #  define register usage for the cvtsp instruction
 # 
 #      31               23               15               07            00
 #     +----------------+----------------+----------------+----------------+
 #     |    delta-pc    |      xxxx      |             srclen              | : r0
 #     +----------------+----------------+----------------+----------------+
 #     |                              srcaddr                              | : r1
 #     +----------------+----------------+----------------+----------------+
 #     |              xxxxx              |             dstlen              | : r2
 #     +----------------+----------------+----------------+----------------+
 #     |                              dstaddr                              | : r3
 #     +----------------+----------------+----------------+----------------+
# define cvtsp_s_cvtsp 16
 # define cvtsp 0
# define cvtsp_w_srclen 0
# define cvtsp_s_srclen 5
# define cvtsp_v_srclen 0
# define cvtsp_b_delta_pc 3
# define cvtsp_a_srcaddr 4                 
					#  srcaddr.ab
# define cvtsp_w_dstlen 8
# define cvtsp_s_dstlen 5
# define cvtsp_v_dstlen 0
# define cvtsp_a_dstaddr 12                
					#  dstaddr.ab
 #	.endm
 
 #	.macro	cvttp_def,..equ=<=>,..col=<:>
 #  define register usage for the cvttp instruction
 # 
 #      31               23               15               07            00
 #     +----------------+----------------+----------------+----------------+
 #     |             dstlen              |             srclen              | : r0
 #     +----------------+----------------+----------------+----------------+
 #     |                              srcaddr                              | : r1
 #     +----------------+----------------+----------------+----------------+
 #     |                              tbladdr                              | : r2
 #     +----------------+----------------+----------------+----------------+
 #     |                              dstaddr                              | : r3
 #     +----------------+----------------+----------------+----------------+
 # 
 #     there is no spare byte to store the delta-pc quantity with three
 #     addresses to be stored in four registers. the delta-pc quantity is
 #     stored in the upper byte of the dstlen cell. once the destination length
 #     has been checked for a legal range (a decimal string length larger than
 #     31 causes a reserved operand abort), that length can be easily stored in
 #     five bytes, leaving the upper byte free. 
 # 
 # 
 #                       31            24       20      16
 #                      +----------------+-----+----------+
 #                      |             dstlen              |
 #                      +----------------+-----+----------+
 #                     /                /       \          \
 #                    /                /         \          \
 #                   +----------------+           +----------+
 #                   |    delta-pc    |           |  dstlen  |
 #                   +----------------+           +----------+
 #                    31            24             20      16
# define cvttp_s_cvttp 16
 # define cvttp 0
# define cvttp_w_srclen 0
# define cvttp_s_srclen 5
# define cvttp_v_srclen 0
# define cvttp_v_fpd 15
# define cvttp_w_dstlen 2
# define cvttp_b_dstlen 2
# define cvttp_s_dstlen 5
# define cvttp_v_dstlen 0
# define cvttp_b_delta_pc 3
# define cvttp_a_srcaddr 4                 
					#  srcaddr.ab
# define cvttp_a_tbladdr 8                 
					#  tbladdr.ab
# define cvttp_a_dstaddr 12                
					#  dstaddr.ab
 #	.endm
 
 #	.macro	divp_def,..equ=<=>,..col=<:>
 #  define register usage for the divp instruction
 # 
 #      31               23               15               07            00
 #     +----------------+----------------+----------------+----------------+
 #     |    delta-pc    |      xxxx      |             divrlen             | : r0
 #     +----------------+----------------+----------------+----------------+
 #     |                             divraddr                              | : r1
 #     +----------------+----------------+----------------+----------------+
 #     |              xxxxx              |             divdlen             | : r2
 #     +----------------+----------------+----------------+----------------+
 #     |                             divdaddr                              | : r3
 #     +----------------+----------------+----------------+----------------+
 #     |              xxxxx              |             quolen              | : r4
 #     +----------------+----------------+----------------+----------------+
 #     |                              quoaddr                              | : r5
 #     +----------------+----------------+----------------+----------------+
# define divp_s_divp 24
 # define divp 0
# define divp_w_divrlen 0
# define divp_s_divrlen 5
# define divp_v_divrlen 0
# define divp_b_delta_pc 3
# define divp_a_divraddr 4                 
					#  divraddr.ab
# define divp_w_divdlen 8
# define divp_s_divdlen 5
# define divp_v_divdlen 0
# define divp_a_divdaddr 12                
					#  divdaddr.ab
# define divp_w_quolen 16
# define divp_s_quolen 5
# define divp_v_quolen 0
# define divp_a_quoaddr 20                 
					#  quoaddr.ab
 #	.endm
 
 #	.macro	movp_def,..equ=<=>,..col=<:>
 #  define register usage for the movp instruction
 # 
 #      31               23               15               07            00
 #     +----------------+----------------+----------------+----------------+
 #     |    delta-pc    |      state     |               len               | : r0
 #     +----------------+----------------+----------------+----------------+
 #     |                              srcaddr                              | : r1
 #     +----------------+----------------+----------------+----------------+
 #     |                               xxxxx                               | : r2
 #     +----------------+----------------+----------------+----------------+
 #     |                              dstaddr                              | : r3
 #     +----------------+----------------+----------------+----------------+
# define movp_s_movp 16
 # define movp 0
# define movp_w_len 0
# define movp_s_len 5
# define movp_v_len 0
# define movp_b_state 2
# define movp_s_saved_psw 4
# define movp_v_saved_psw 0
# define movp_m_fpd 16
# define movp_v_fpd 4
# define movp_b_delta_pc 3
# define movp_a_srcaddr 4                  
					#  srcaddr.ab
# define movp_a_dstaddr 12                 
					#  dstaddr.ab
 #	.endm
 
 #	.macro	mulp_def,..equ=<=>,..col=<:>
 #  define register usage for the mulp instruction
 # 
 #      31               23               15               07            00
 #     +----------------+----------------+----------------+----------------+
 #     |    delta-pc    |      xxxx      |             mulrlen             | : r0
 #     +----------------+----------------+----------------+----------------+
 #     |                             mulraddr                              | : r1
 #     +----------------+----------------+----------------+----------------+
 #     |              xxxxx              |             muldlen             | : r2
 #     +----------------+----------------+----------------+----------------+
 #     |                             muldaddr                              | : r3
 #     +----------------+----------------+----------------+----------------+
 #     |              xxxxx              |             prodlen             | : r4
 #     +----------------+----------------+----------------+----------------+
 #     |                             prodaddr                              | : r5
 #     +----------------+----------------+----------------+----------------+
# define mulp_s_mulp 24
 # define mulp 0
# define mulp_w_mulrlen 0
# define mulp_s_mulrlen 5
# define mulp_v_mulrlen 0
# define mulp_b_delta_pc 3
# define mulp_a_mulraddr 4                 
					#  mulraddr.ab
# define mulp_w_muldlen 8
# define mulp_s_muldlen 5
# define mulp_v_muldlen 0
# define mulp_a_muldaddr 12                
					#  muldaddr.ab
# define mulp_w_prodlen 16
# define mulp_s_prodlen 5
# define mulp_v_prodlen 0
# define mulp_a_prodaddr 20                
					#  prodaddr.ab
 #	.endm
 
 #	.macro	subp4_def,..equ=<=>,..col=<:>
 #  define register usage for the subp4 instruction
 # 
 #      31               23               15               07            00
 #     +----------------+----------------+----------------+----------------+
 #     |    delta-pc    |      xxxx      |             sublen              | : r0
 #     +----------------+----------------+----------------+----------------+
 #     |                              subaddr                              | : r1
 #     +----------------+----------------+----------------+----------------+
 #     |              xxxxx              |             diflen              | : r2
 #     +----------------+----------------+----------------+----------------+
 #     |                              difaddr                              | : r3
 #     +----------------+----------------+----------------+----------------+
# define subp4_s_subp4 16
 # define subp4 0
# define subp4_w_sublen 0
# define subp4_s_sublen 5
# define subp4_v_sublen 0
# define subp4_b_delta_pc 3
# define subp4_a_subaddr 4                 
					#  subaddr.ab
# define subp4_w_diflen 8
# define subp4_s_diflen 5
# define subp4_v_diflen 0
# define subp4_a_difaddr 12                
					#  difaddr.ab
 #	.endm
 
 #	.macro	subp6_def,..equ=<=>,..col=<:>
 #  define register usage for the subp6 instruction
 # 
 #      31               23               15               07            00
 #     +----------------+----------------+----------------+----------------+
 #     |    delta-pc    |      xxxx      |             sublen              | : r0
 #     +----------------+----------------+----------------+----------------+
 #     |                              subaddr                              | : r1
 #     +----------------+----------------+----------------+----------------+
 #     |              xxxxx              |             minlen              | : r2
 #     +----------------+----------------+----------------+----------------+
 #     |                              minaddr                              | : r3
 #     +----------------+----------------+----------------+----------------+
 #     |              xxxxx              |             diflen              | : r4
 #     +----------------+----------------+----------------+----------------+
 #     |                              difaddr                              | : r5
 #     +----------------+----------------+----------------+----------------+
# define subp6_s_subp6 24
 # define subp6 0
# define subp6_w_sublen 0
# define subp6_s_sublen 5
# define subp6_v_sublen 0
# define subp6_b_delta_pc 3
# define subp6_a_subaddr 4                 
					#  subaddr.ab
# define subp6_w_minlen 8
# define subp6_s_minlen 5
# define subp6_v_minlen 0

# define subp6_a_minaddr 12                
					#  minaddr.ab
# define subp6_w_diflen 16
# define subp6_s_diflen 5
# define subp6_v_diflen 0
# define subp6_a_difaddr 20                
					#  difaddr.ab
 #	.endm
 
 #	.macro	editpc_def,..equ=<=>,..col=<:>
 #  define register usage for the editpc instruction
 # 
 #      31               23               15               07            00
 #     +----------------+----------------+----------------+----------------+
 #     |           zero count            |             srclen              | : r0
 #     +----------------+----------------+----------------+----------------+
 #     |                              srcaddr                              | : r1
 #     +----------------+----------------+----------------+----------------+
 #     |    delta-pc    |  delta-pc      |      sign      |      fill      | : r0
 #     +----------------+----------------+----------------+----------------+
 #     |                              pattern                              | : r3
 #     +----------------+----------------+----------------+----------------+
 #     |  loop-count    |  state         |  saved_psw     | inisrclen      | : r4
 #     +----------------+----------------+----------------+----------------+
 #     |                              dstaddr                              | : r5
 #     +----------------+----------------+----------------+----------------+
# define editpc_m_state 15
# define editpc_m_fpd 16
# define editpc_s_editpc 24
 # define editpc 0
# define editpc_w_srclen 0
# define editpc_b_srclen 0
# define editpc_s_srclen 5
# define editpc_v_srclen 0
# define editpc_b_eo_read_char 1
# define editpc_w_zero_count 2
# define editpc_a_srcaddr 4                
					#  srcaddr.ab
# define editpc_b_fill 8
# define editpc_b_sign 9
# define editpc_b_delta_pc 10
# define editpc_b_delta_srcaddr 11         
					#  current srcaddr minus initial srcaddr
# define editpc_a_pattern 12               
					#  pattern.ab
# define editpc_b_inisrclen 16
# define editpc_b_saved_psw 17
# define editpc_b_state 18
# define editpc_s_state 4
# define editpc_v_state 1
# define editpc_v_fpd 4
# define editpc_b_loop_count 19
# define editpc_a_dstaddr 20               
					#  dstaddr.ab
# define editpc_l_saved_r0 16              
					#  initial srclen.rw
# define editpc_l_saved_r1 20              
					#  initial srcaddr.ab
/*
 *	Misc offsets
 */

# define cmppx_m_src1_minus 1
# define cmppx_m_src2_minus 2
# define cmppx_v_src1_minus 0
# define cmppx_v_src2_minus 1

 #	.endm
