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
/* $Header: mono_tcap.h,v 4.4 85/08/31 13:02:10 webb Exp $ */
/* $Source: /ibm/acis/usr/sys_ca/ca/RCS/mono_tcap.h,v $ */

#if !defined(lint) && !defined(LOCORE) && defined(RCS_HDRS)
static char *rcsidsari = "$Header: mono_tcap.h,v 4.4 85/08/31 13:02:10 webb Exp $";
#endif

#if CMU
/*
 ***********************************************************************
 * HISTORY
 * 27-Feb-86  Bill Bolosky (bolosky) at Carnegie-Mellon University
 *	Modified to redefine LOCAL if it has been previously defined,
 *	rather than gagging.
 *
 ***********************************************************************
 */
#include "romp_debug.h"
#endif CMU

/* sari.h -- standalone constants for the IBM RI */

#define CTL(x) ('x'&037)	/* get a control character */
#define min(x,y) x < y ? x : y
#define max(x,y) x > y ? x : y

#define ROMP_BASE 0xf0000000	       /* I/O base address */

#define in(port) * (( char *) (ROMP_BASE + (port)))
#define out(port,value) in(port) = value
 /* output a PC word (= short) */
#define inw(port) * (( short *) (ROMP_BASE + (port)))
#define outw(port,value) inw(port) = value

#define CRT_1 0x3b8		       /* crt port 1 */
#define PUT_SCR_REG(reg,value) out(0x3b4,reg); out(0x3b5,value);

#ifdef LOCAL
#undef LOCAL
#endif LOCAL
#if	ROMP_DEBUG 
#define LOCAL			       /* make it external */
#else	ROMP_DEBUG
#define LOCAL	static		       /* make it local */
#endif	ROMP_DEBUG
#ifdef SBPROTO
#define    _CDB        0x082A	       /* Hex Display Register  */
#endif

#ifdef SBMODEL
#define    _CDB        0x8ce0	       /* Hex Display Register  */
#endif
#ifndef lint
static char rcsidputchar[] = "$Header: mono_tcap.h,v 4.4 85/08/31 13:02:10 webb Exp $";
#endif
 /* $Source: /ibm/acis/usr/sys_ca/ca/RCS/mono_tcap.h,v $ */
#if defined(ACT4)
#define UP_CHAR	CTL(Z)		/* \32 cursor up (and scroll) */
#define CD_CHAR CTL(_)		/* \37 clear to end of display */
#define ND_CHAR CTL(X)		/* \26 non destructive space */
#define HO_CHAR	CTL(])		/* \35 home */
#define CL_CHAR	CTL(L)		/* \14 erase screen */
#define CE_CHAR	CTL(^)		/* \36 erase to end of line */
#define CM_CHAR	CTL(T)		/* \24 CURSOR MOTION */
#define CM_CHAR2 CTL(U)		/* \25 CURSOR MOTION alternate */
#define SO_CHAR	CTL(A)		/* \01 stand out */
#define SE_CHAR	CTL(B)		/* \02 stand out end */
#define SH_CHAR CTL(C)		/* \03 status line on/off */
static char term_type[] = "act4";
#else
#define ESC_FOUND 0x100		       /* flag for escape found */
#define ESC_(x) ESC_FOUND+'x'		/* ESC + x */
#define UP_CHAR ESC_(A)			/* ESC A cursor up */
#define CL_CHAR	ESC_(K)			/* ESC K clear screen */
#define ND_CHAR ESC_(C)			/* ESC C non-destructive space */
#define CD_CHAR ESC_(J)			/* ESC J clear to end of screen */
#define CE_CHAR ESC_(I)			/* ESC I clear to end of line */
#define HO_CHAR ESC_(H)			/* ESC H home cursor */
#define CM_CHAR2 ESC_(Y)		/* ESC Y x y */
#define SO_CHAR	ESC_(p)			/* ESC p stand out */
#define SE_CHAR	ESC_(q)			/* ESC q stand out end */
#define HI_CHAR ESC_(Z)			/* ESC Z hi intensity */
#define LO_CHAR ESC_(z)			/* ESC z lo intensity */
#define US_CHAR ESC_(W)			/* ESC W start underline */
#define UE_CHAR ESC_(w)			/* ESC w stop underline */
#define SH_CHAR ESC_(s)			/* ESC s status line on/off */
#define SAVE_CM ESC_(j)			/* ESC j save cursor position */
#define RESTORE_CM ESC_(k)		/* ESC k restore cursor position */
#define AL_CHAR ESC_(L)			/* ESC L insert line */
#define DL_CHAR ESC_(M)			/* ESC M delete line */
#define IGN_CHAR	ESC_([)		/* ESC [ ==> ESC */
static char term_type[] = "ibm3101";
#endif
#define CURSOR_OFFSET	' '	       /* blank is normal offset value */
