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
/* $Header: pte.h,v 4.0 85/07/15 00:47:25 ibmacis GAMMA $ */
/* $Source: /ibm/acis/usr/sys_ca/ca/RCS/pte.h,v $ */

#if !defined(lint) && !defined(LOCORE) && defined(RCS_HDRS)
static char *rcsidpte = "$Header: pte.h,v 4.0 85/07/15 00:47:25 ibmacis GAMMA $";
#endif
#if 	CMU
/***********************************************************************
 * HISTORY
 * 14-Mar-86  Bill Bolosky (bolosky) at Carnegie-Mellon University
 *	Changed definition of fpte to have 1 bit for pg_fileno (which,
 *	incidentially no longer contains a fileno at all!) and 24 to
 *	pg_blkno to correspond to 4.3 changes.  Also changed definitions
 *	of PG_FZERO, PG_FMAX and PG_FTEXT.
 *
 ***********************************************************************
 */
#ifdef	KERNEL
#include "mach_vm.h"
#else	KERNEL
#include <sys/features.h>
#endif	KERNEL
#endif 	CMU

#if	MACH_VM
#ifndef	ASSEMBLER
struct	pte { int foo;} ;
#endif	ASSEMBLER
#else	MACH_VM

/*     pte.h   6.1     83/07/29        */

/*
 * VAX page table entry
 *
 * There are two major kinds of pte's: those which have ever existed (and are
 * thus either now in core or on the swap device), and those which have
 * never existed, but which will be filled on demand at first reference.
 * There is a structure describing each.  There is also an ancillary
 * structure used in page clustering.
 */

#ifndef LOCORE
struct pte
{
unsigned int
               pg_v:1,                 /* valid bit */
               pg_prot:4,              /* access control */
               pg_m:1,                 /* hardware maintained modified bit */
               pg_fod:1,               /* is fill on demand (=0) */
               pg_swapm:1,             /* have to write back to swap */
               pg_vreadm:1,            /* modified since vread (or with _m) */
               :2,
               pg_pfnum:21;            /* core page frame number or 0 */
};
struct hpte
{
unsigned int
               pg_high:9,              /* special for clustering */
               :2,
               pg_pfnum:21;
};
struct fpte
{
unsigned int
               pg_v:1,
               pg_prot:4,
               :1,
               pg_fod:1,               /* is fill on demand (=1) */
               pg_fileno:1,            /* TEXT or ZERO */
               pg_blkno:24;            /* file system block number */
};
#endif

#define        PG_V            0x80000000
#define        PG_PROT         0x78000000
#define        PG_M            0x04000000
#define        PG_FOD          0x02000000
#define        PG_VREADM       0x00800000
#define        PG_PFNUM        0x001fffff

#define        PG_FZERO        0
#define        PG_FTEXT        1
#define        PG_FMAX         (PG_FTEXT)

#define        PG_NOACC        0
#define        PG_KW           0x10000000
#define        PG_KR           0x18000000
#define        PG_UW           0x20000000
#define        PG_URKW         0x70000000
#define        PG_URKR         0x78000000

/*
 * Pte related macros
 */
#define dirty(pte) ((pte)->pg_fod == 0 && (pte)->pg_pfnum && \
 ((pte)->pg_m || (pte)->pg_swapm || get_mod_bit((pte)->pg_pfnum)))

#define referenced(pte) get_ref_bit((pte)->pg_pfnum)

#ifndef LOCORE
#ifdef KERNEL
struct pte *vtopte();

/* utilities defined in locore.s */
extern struct pte Sysmap[];
extern struct pte Usrptmap[];
extern struct pte usrpt[];
extern struct pte Swapmap[];
extern struct pte Forkmap[];
extern struct pte Xswapmap[];
extern struct pte Xswap2map[];
extern struct pte Pushmap[];
extern struct pte Vfmap[];
extern struct pte mmap[];
extern struct pte msgbufmap[];
extern struct pte camap[];
extern struct pte Nexmap[][16];
#endif
#endif

#endif	MACH_VM
