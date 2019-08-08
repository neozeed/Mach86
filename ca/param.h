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
/* $Header: param.h,v 4.1 85/07/26 22:39:09 webb Exp $ */
/* $Source: /ibm/acis/usr/sys_ca/ca/RCS/param.h,v $ */

#if !defined(lint) && !defined(LOCORE) && defined(RCS_HDRS)
static char *rcsidca_param = "$Header: param.h,v 4.1 85/07/26 22:39:09 webb Exp $";
#endif
#ifdef CMU
/*
 ***********************************************************************
 * HISTORY
 * 28-Feb-86  Bill Bolosky (bolosky) at Carnegie-Mellon University
 *	added macros for splsoftclock, splhigh and spltty.
 *
 ***********************************************************************
 */

#ifdef	KERNEL
#include "mach_vm.h"
#else	KERNEL
#include <sys/features.h>
#endif	KERNEL
#endif CMU


/*     param.h 6.1     83/07/29        */

/*
 * Machine dependent constants for Romp.
 */
#define        NBPG    2048            /* bytes/page */
#define        PGOFSET (NBPG-1)        /* byte offset into page */
#define        PGSHIFT 11              /* LOG2(NBPG) */
#define        LOG2NBPG 11             /* LOG2(NBPG) */

#define        CLSIZE          1
#define        CLSIZELOG2      0

#define        SSIZE   1               /* initial stack size/NBPG */
#define        SINCR   1               /* increment of stack/NBPG */

#if	MACH_VM
#define        UPAGES  4               /* pages of u-area */
#else	MACH_VM
#define        UPAGES  2               /* pages of u-area */
#endif	MACH_VM

#define SEGMENT_OFFSET 0x0FFFFFFF      /* Romp hardware segment offset */
#define DATABASE       0x10000000      /* virtual address of data origin */
/* IF YOU CHANGE THIS, CHANGE SYS_ORG IN LOCORE.H ALSO! */
/* Better yet, have the system virtual origin defined in only one place. */
#define SYSBASE  ((u_long)0xe0000000)  /* virt addr of system origin */
/*
 * Some macros for units conversion
 */
/* Core clicks (2048 bytes) to segments and vice versa */
#define        ctos(x) (x)
#define        stoc(x) (x)

/* Core clicks (2048 bytes) to disk blocks */
#define        ctod(x) (x<<2)
#define        dtoc(x) ((x+3)>>2)
#define        dtob(x) ((x)<<9)

/* clicks to bytes */
#define        ctob(x) ((x)<<11)

/* bytes to clicks */
#define        btoc(x) ((((unsigned)(x)+2047)>>11))

/*
 * Macros to decode interrupt control status.
 */
#define        USERMODE(icscs)   ((icscs) & ICSCS_PROBSTATE)
#define        BASEPRI(icscs)    (((icscs) & ICSCS_PRIORITY) == ICSCS_PRIORITY)

#ifdef SBPROTO
#define        DELAY(n)        { register int N = (n); while (--N > 0); }
#endif

#ifdef SBMODEL
#define DELAY_ADDR	0xf00080E0		/* IOCC delay location */
#define DELAY(n)	{ register int N = (n); while (--N >= 0) \
				* (char *) DELAY_ADDR = 0xff;	}
#endif

#define spl0()	_spl7()		/* use CPU priority level 7 */
#define spl1()	_spl6()		/* use CPU priority level 6 */
#define spl2()	_spl5()		/* use CPU priority level 5 */
#define spl3()	_spl4()		/* use CPU priority level 4 */
#define spl4()	_spl3()		/* use CPU priority level 3 */
#define spl5()	_spl2()		/* use CPU priority level 2 */
#define spl6()	_spl1()		/* use CPU priority level 1 */
#define spl7()	_spl0()		/* use CPU priority level 0 */
#define splsoftclock() 	_spl1()	
#define splhigh()	_spl1()
#define spltty()	_spl2()

#ifndef LOCORE
short rorootdev;		/* for read-only root device */
#endif

/*
 * following is temporary until config is operational
 */

#ifndef ROOTMAJ
#define ROOTMAJ 1
#endif

#ifndef ROOTMIN
#define ROOTMIN 8
#endif
