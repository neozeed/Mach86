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
/*	@(#)vaxemul.h	1.2		11/2/84		*/


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
 * 000- This module is used by the emulation code.  It defines most
 *	of the constants that are required by the emulation code.
 *
 ***********************************************************************/

/*
 * VAX program status longword
 */

#define	psl$m_c PSL_C			/* carry bit */
#define psl$m_v	PSL_V			/* overflow bit */
#define	psl$m_z PSL_Z			/* zero bit */
#define psl$m_n	PSL_N			/* negative bit */
#define	psl$m_allcc PSL_ALLCC		/* all cc bits - unlikely */
#define	psl$m_t PSL_T			/* trace enable bit */
#define psl$m_tbit PSL_T		/*  ditto	*/
#define	psl$m_iv PSL_IV			/* integer overflow enable bit */
#define	psl$m_fu PSL_FU			/* floating point underflow enable */
#define	psl$m_dv PSL_DV			/* decimal overflow enable bit */
#define psl$m_ipl PSL_IPL		/* interrupt priority level */
#define psl$m_prvmod PSL_PRVMOD		/* previous mode (all on is user) */
#define psl$m_curmod PSL_CURMOD		/* current mode (all on is user) */
#define psl$m_is PSL_IS			/* interrupt stack */
#define psl$m_fpd PSL_FPD		/* first part done */
#define psl$m_tp PSL_TP			/* trace pending */
#define psl$m_cm PSL_CM			/* compatibility mode */

/*
 *	Bit offsets into psw
 */

# define psl$v_c 0x00
# define psl$v_v 0x01 
# define psl$v_z 0x02 
# define psl$v_n 0x03
# define psl$v_dv 0x07
# define psl$v_iv 0x05
# define psl$v_fpd 0x1b
# define psl$v_tp 0x1e
# define psl$v_cm 0X1f
#define psl$v_curmod 0x18
#define psl$s_curmod 0x02
#define psl$c_user 0x03
#define psl$s_prvmod 0x02
#define psl$v_prvmod 0x016

#define ss$_accvio 0x0c
#define ss$_unwind 0x920
#define ss$_resignal 0x918
#define ss$_opcdec 0x43c
#define ss$_radrmod 0x44c
#define ss$_roprand 0x454
#define vax$_opcdec 0x456
#define vax$_opcdec_fpd 0x458
#define ss$_nopriv 0x024
#define ss$_nohandler 0x08f8
#define ss$_nosignal 0x0900
#define ss$_normal 0x01
#define ss$_insframe 0x012c
#define ss$_unwinding 0x01
#define ss$_tbit 0x0464

/*	Trap definitions */

#define srm$k_int_ovf_t 0x01
#define srm$k_int_div_t 0x02
#define srm$k_int_flt_t	0x03
#define srm$k_flt_div_t 0x04
#define srm$k_flt_und_t 0x05
#define srm$k_dec_ovf_t 0x06
#define srm$k_sub_rng_t 0x07

/*	Fault definitions */

#define srm$k_flt_ovf_f 0x08
#define srm$k_flt_div_f 0x09
#define srm$k_flt_und_f 0x0a

/*	Do it the way VMS does it so that the emulation code can stay the same
 */	

#define ss$_artres 0x0474

#define ss$_intovf   (ss$_artres+(8*srm$k_int_ovf_t))
#define ss$_intdiv   (ss$_artres+(8*srm$k_int_div_t))
#define ss$_fltovf   (ss$_artres+(8*srm$k_flt_ovf_t))
#define ss$_fltdiv   (ss$_artres+(8*srm$k_flt_div_t))
#define ss$_fltund   (ss$_artres+(8*srm$k_flt_und_t))
#define ss$_decovf   (ss$_artres+(8*srm$k_dec_ovf_t))
#define ss$_subrng   (ss$_artres+(8*srm$k_sub_rng_t))
#define ss$_fltovf_f (ss$_artres+(8*srm$k_flt_ovf_f))
#define ss$_fltdiv_f (ss$_artres+(8*srm$k_flt_div_f))
#define ss$_fltund_f (ss$_artres+(8*srm$k_flt_und_f))

#define	    FPE_INTOVF_TRAP	0x1	/* integer overflow */
#define	    FPE_INTDIV_TRAP	0x2	/* integer divide by zero */
#define	    FPE_FLTOVF_TRAP	0x3	/* floating overflow */
#define	    FPE_FLTDIV_TRAP	0x4	/* floating/decimal divide by zero */
#define	    FPE_FLTUND_TRAP	0x5	/* floating underflow */
#define	    FPE_DECOVF_TRAP	0x6	/* decimal overflow */
#define	    FPE_SUBRNG_TRAP	0x7	/* subscript out of range */
#define	    FPE_FLTOVF_FAULT	0x8	/* floating overflow fault */
#define	    FPE_FLTDIV_FAULT	0x9	/* divide by zero floating fault */
#define	    FPE_FLTUND_FAULT	0xa	/* floating underflow fault */

/*	Definition of the VMS signal constants */

#define chf$l_mcharglst 0x08
#define chf$l_mch_args	0x00
#define chf$l_mch_depth 0x08
#define chf$l_mch_frame 0x04
#define chf$l_mch_savr0 0x0c
#define chf$l_mch_savr1 0x010
#define chf$l_sigarglst 0x04
#define chf$l_sig_arg1  0x08
#define chf$l_sig_args  0x00
#define chf$l_sig_name  0x04

