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
/* $Header: fpfp.h,v 4.0 85/07/15 00:42:21 ibmacis GAMMA $ */
/* $Source: /ibm/acis/usr/sys_ca/ca/RCS/fpfp.h,v $ */

#if !defined(lint) && !defined(LOCORE)  && defined(RCS_HDRS)
static char *rcsidfpfp = "$Header: fpfp.h,v 4.0 85/07/15 00:42:21 ibmacis GAMMA $";
#endif

/* @(#)fpfp.h  5.19 - 85/02/08 */
#ifndef _h_FPFP
#define _h_FPFP

#define TRAP_NaN 0x7ff70000


/* Floating point accelerator/emulator interface definitions */

typedef struct {
	unsigned kill:1;	/* SIGARITH (SIGFPE) on exception */
	unsigned xcp_flag:1;	/* exception occurred		  */
	unsigned io_flag:1;	/* invalid operation occurred	  */
 	unsigned io_xpt:1;	/* exception on invalid operation */
	unsigned dz_flag:1;	/* divide by zero occurred	  */
	unsigned dz_xpt:1;	/* exception on divide by zero	  */
	unsigned of_flag:1;	/* overflow occurred		  */
	unsigned of_xpt:1;	/* exception on overflow	  */
	unsigned uf_flag:1;	/* underflow occurred		  */
	unsigned uf_xpt:1;	/* exception on underflow	  */
	unsigned rsvd11:11;	/* reserved			  */
	unsigned cmp_rslt:2;	/* comparison result		  */
	unsigned rnd_mode:2;	/* rounding mode		  */
	unsigned ir_flag:1;	/* inexact result occurred	  */
	unsigned ir_xpt:1;	/* exception on inexact result	  */
	unsigned rsvd2:2;	/* reserved			  */
	unsigned mc_type:3;	/* machine communications type	  */
} FP_STATUS;

/* initializers for fields of FP_STATUS, as fields and as an unsigned */
#define FP_S_flds {1,0,0,1,0,1,0,1,0,0,0,0,0,0,0,1,0};
#define FP_S_unsgd 0x95000008

typedef unsigned long FP_FLOAT;

typedef double FP_DOUBLE;

/* rounding mode values */
#define FP_NEAR	0		/* round to nearest  */
#define FP_ZERO 1		/* round toward zero */
#define FP_UP	2		/* round toward +inf */
#define FP_DOWN	3		/* round toward -inf */

/* return constants from comparison routines */
#define LESSTHAN -1
#define EQUAL	  0
#define GREATER	  1

extern void (*_fpfpf[])();		/* routines have various types */
extern void (*_fpfaf[])();		/* this vector refs. to hardware fns */

enum FPFPI {
    FP_rdf,   FP_rdd,   FP_i2f,  FP_i2d,
    FP_cpf,   FP_cpfi,  FP_cpd,  FP_cpdi,
    FP_f2d,   FP_f2di,  FP_d2f,  FP_d2fi,
    FP_ngf,   FP_ngfi,  FP_ngd,  FP_ngdi,
    FP_abf,   FP_abfi,  FP_abd,  FP_abdi,
    FP_ntf,   FP_ntfi,  FP_ntd,  FP_ntdi,
    FP_rnf,   FP_rnfi,  FP_rnd,  FP_rndi,
    FP_trf,   FP_trfi,  FP_trd,  FP_trdi,
    FP_flf,   FP_flfi,  FP_fld,  FP_fldi,
    FP_cmf,   FP_cmfi,  FP_cmd,  FP_cmdi,
    FP_adf,   FP_adfi,  FP_add,  FP_addi,
    FP_sbf,   FP_sbfi,  FP_sbd,  FP_sbdi,
    FP_mlf,   FP_mlfi,  FP_mld,  FP_mldi,
    FP_dvf,   FP_dvfi,  FP_dvd,  FP_dvdi,
    FP_rmf,   FP_rmfi,  FP_rmd,  FP_rmdi,
    FP_sqf,   FP_sqfi,  FP_sqd,  FP_sqdi,
/*
    FP_csf,   FP_cfs,   FP_csd,  FP_cds,
    FP_getst, FP_setst};
*/
    FP_csf,   FP_csfi,  FP_csd,  FP_csdi,
    FP_getst, FP_setst,
    FP_lmr,   FP_smr,
    FP_tan,   FP_atan,  FP_2xml, FP_yl2x,  FP_ylp1};

#define FP_num (FP_ylp1 - FP_rdf + 1)

extern  FP_DOUBLE
    ieeeatof(),	ieeeldexp(),	ieeefrexp(),	ieeemodf(),
    _FPrdd(),	_FPi2d(),	_FPcpd(),	_FPcpdi(),
    _FPf2d(),	_FPf2di(),	_FPngd(),	_FPngdi(),
    _FPabd(),	_FPabdi(),	_FPntd(),	_FPntdi(),
    _FPadd(),	_FPaddi(),	_FPsbd(),	_FPsbdi(),
    _FPmld(),	_FPmldi(),	_FPdvd(),	_FPdvdi(),
    _FPrmd(),	_FPrmdi(),	_FPsqd(),	_FPsqdi(),
    _FPntd(),	_FPntdi(),	_FPcsd(),	_FPyl2x(),
    _FPtan(),	_FPatan(),	_FP2xml(),	_FPylp1();

extern FP_FLOAT
    _FPrdf(),	_FPi2f(),	_FPcpf(),	_FPcpfi(),
    _FPd2f(),	_FPd2fi(),	_FPngf(),	_FPngfi(),
    _FPabf(),	_FPabfi(),	_FPadf(),	_FPadfi(),
    _FPsbf(),	_FPsbfi(),	_FPmlf(),	_FPmlfi(),
    _FPdvf(),	_FPdvfi(),	_FPrmf(),	_FPrmfi(),
    _FPntf(),	_FPntfi(),
    _FPsqf(),	_FPsqfi(),	_FPcsf();
#endif
