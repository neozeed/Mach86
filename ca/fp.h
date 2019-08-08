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
/* $Header: fp.h,v 4.0 85/07/15 00:42:11 ibmacis GAMMA $ */
/* $Source: /ibm/acis/usr/sys_ca/ca/RCS/fp.h,v $ */

#if !defined(lint) && !defined(LOCORE)  && defined(RCS_HDRS)
static char *rcsidfp = "$Header: fp.h,v 4.0 85/07/15 00:42:11 ibmacis GAMMA $";
#endif

/* @(#)FP.h	5.7 - 84/08/28 */
#ifndef _h_FP
#define _h_FP
/***********************  ---- TYPEDEFS ----  *****************************/
 
#ifdef KERNEL
#include "../machine/fpfp.h"
#else
#include <machine/fpfp.h>
#endif
 
/* These are internal definitions that reflect how the fpfp routines
    view float and double objects */
 
typedef unsigned long FLOAT;
typedef struct {unsigned long dfracth,dfractl;} DOUBLE;
 
extern  FLOAT   FLT_NaN;
extern  DOUBLE  DBL_NaN;
 
typedef struct {
	DOUBLE dreg[8];        /* FP_DOUBLE regs 0-7 */
	FP_STATUS status;         /* fp status reg */
	unsigned long fsear;      /* fp addr reg */
    } FP_MACH;
 
extern	FP_MACH _fpfpr;
#define machine _fpfpr
 
#define _Double(x) (*(FP_DOUBLE *)&(x))
 
FP_FLOAT _fADDSUB(), _fFPDIV();
FP_DOUBLE _dADDSUB(), _dFPDIV();
 
/***********************  ---- MACROS -----  *****************************/
 
#define RNDMODE (machine.status.rnd_mode)
 
#define fFP_ret(reg) \
return(((reg) & NORETBIT) ? FLT_NaN : machine.dreg[(reg) & OKREGBITS].dfracth)
 
#define dFP_ret(reg) \
	return(((reg) & NORETBIT) ? _Double(DBL_NaN) \
		: _Double(machine.dreg[(reg) & OKREGBITS]))
 
#define fNaN_ret(reg) \
	return(machine.dreg[(reg)&OKREGBITS].dfracth=FLT_NaN, FLT_NaN)
 
#define dNaN_ret(reg) \
	return(machine.dreg[(reg)&OKREGBITS]=DBL_NaN, _Double(DBL_NaN))

#define QfNaN_ret(reg) \
	return(machine.dreg[(reg)&OKREGBITS].dfracth |= BIT22, \
	((reg) & NORETBIT) ? FLT_NaN : machine.dreg[(reg)].dfracth)
 
#define QdNaN_ret(reg) \
	return(machine.dreg[(reg)&OKREGBITS].dfracth |= BIT19, \
	((reg) & NORETBIT) ? _Double(DBL_NaN) : _Double(machine.dreg[(reg)]))
 
#define dTRAP_NaN(val) \
	(((val.dfracth & DEXPBITS) == DEXPBITS) \
	&& (((val.dfracth & LO20BITS) != 0) || (val.dfractl != 0)) \
	&& !(val.dfracth & BIT19))
 
#define fTRAP_NaN(val) \
	(((val & FEXPBITS) == FEXPBITS) \
	&& ((val & LO23BITS) != 0) && !(val & BIT22))

/***********************  ---- CONSTANTS ----  *****************************/
 
	/* machine communications type */
 
#define FP_INV_CMD 4            /* invalid FPA command */
 
	/* status constants */
 
#define INV_OPER 0
#define OVERFLOW 1
#define UNDERFLOW 2
#define DIVIDE 3
#define INEXACT 4
 
	/* exponent constants */
 
#define MAX_FEXPON      0x00ff
#define MAX_DEXPON      0x07ff
#define FEXPBIAS        0x7f
#define DEXPBIAS        0x3ff
#define FEXPHI          FEXPBIAS+1
#define DEXPHI          DEXPBIAS+1
#define FEXPLO          -(FEXPBIAS)
#define DEXPLO          -(DEXPBIAS)
 
	/* register mask constants */
 
#define OKREGBITS       0x07
#define NORETBIT        0x08
#define ALLBITS         0xffffffff
#define DEXPBITS        0x7ff00000
#define FEXPBITS        0x7f800000
#define DLDGBIT         0x00100000
#define FLDGBIT         0x00800000
#define HI25BITS        0xffffff80
#define HI16BITS        0xffff0000
#define HI12BITS        0xfff00000
#define HI3BITS         0xe0000000
#define LO31BITS        0x7fffffff
#define LO30BITS        0x3fffffff
#define LO29BITS        0x1fffffff
#define LO28BITS        0x0fffffff
#define LO27BITS        0x07ffffff
#define LO26BITS        0x03ffffff
#define LO24BITS        0x00ffffff
#define LO23BITS        0x007fffff
#define LO21BITS        0x001fffff
#define LO20BITS        0x000fffff
#define LO16BITS        0x0000ffff
#define LO13BITS        0x00001fff
#define LO12BITS        0x00000fff
#define LO11BITS        0x000007ff
#define LO10BITS        0x000003ff
#define LO9BITS         0x000001ff
#define LO8BITS         0x000000ff
#define LO7BITS         0x0000007f
#define LO6BITS         0x0000003f
#define LO5BITS         0x0000001f
#define LO4BITS         0x0000000f
#define LO3BITS         0x00000007
#define BIT31           0x80000000
#define BIT30           0x40000000
#define BIT29           0x20000000
#define BIT28           0x10000000
#define BIT27           0x08000000
#define BIT26           0x04000000
#define BIT25           0x02000000
#define BIT24           0x01000000
#define BIT23           0x00800000
#define BIT22           0x00400000
#define BIT20           0x00100000
#define BIT19           0x00080000
#define BIT16           0x00010000
#define BIT6            0x00000040
#define BIT5            0x00000020
#define BIT0            0x00000001
 
	/* return constants from comparison routines */
 
#define NORETURN        0
#define MININT          (1 << 31)
 
/**********************************************************************/
#endif
