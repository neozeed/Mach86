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
/*
 *	File:	ca/pmap.h
 *
 *	Vax Version:
 *	Copyright (C) 1985, Avadis Tevanian, Jr., Michael Wayne Young
 *
 *	RT PC Version:
 *	Copyright (c) 1986, William J. Bolosky
 *
 *	Machine-dependent structures for the physical map module.
 *
 * HISTORY
 *  8-Apr-86  Bill Bolosky (bolosky) at Carnegie-Mellon University
 *	Created.
 *
 */

#ifndef	_PMAP_MACHINE_
#define	_PMAP_MACHINE_	1

#define	NUSR_SEGS	14
#define IOSAVEL		19 * 4

#ifndef	ASSEMBLER

#ifdef	KERNEL
#include "../h/zalloc.h"
#include "../sync/lock.h"
#include "../ca/vm_param.h"
#include "../vm/vm_statistics.h"
#else	KERNEL
#include <sys/zalloc.h>
#include <sync/lock.h>
#include <machine/vm_param.h>
#include <vm/vm_statistics.h>
#endif	KERNEL

/*
 * WARNING: load_context in ca/loutil.s relies on the fact that sidtab is
 * 	    an array of shorts.  If you change this, you must also change
 *	    load_context or all hell will break loose.
 */
struct pmap {
	short			sidtab[NUSR_SEGS];	/* seg id's */
	int			ref_count;	/* reference count */
	simple_lock_data_t	lock;		/* lock on map */
	struct pmap_statistics	stats;		/* map statistics */
};

typedef struct pmap	*pmap_t;

#define	PMAP_NULL	((pmap_t) 0)
#define atoseg(a)	((unsigned)a>>28)
#define segoffset(a)	(a & 0x0fffffff)
#define	md_ptob(x)		((caddr_t)((x) << rose_page_shift))
#define	virt_eq_real()	{int tcr = ior(ROSEBASE+0x15 /* 0x15 is tcr */); \
			 iow(ROSEBASE+0x15,tcr |= TCR_V); }
#define virt_neq_real()	{int tcr = ior(ROSEBASE+0x15 /* 0x15 is tcr */); \
			 iow(ROSEBASE+0x15,tcr &= ~TCR_V);}
#define invalidate_tlb(vpage) iow(RTA_INV_ADDR, md_ptob(vpage))
#define	btop(x)		(((unsigned)(x)) >> PGSHIFT)
extern	loadpt;
#define KERNSTACK       (ENDOFP1-sizeof(struct user)) /* top of kernel stack */
#define UAREA           (ENDOFP1-UPAGES*NBPG) /* start of u area */

#define uareabase(u)    ((caddr_t)(u)+sizeof(struct user)-UPAGES*NBPG)
#define ENDOFP1         0x20000000  /* first address past P1 */
#define P1PAGES         0x20000     /* number of pages in P1 region */
#endif	ASSEMBLER

#define	SYS_SEG 0xe		/* System segment register. */
#define	MER_L	0x00001000	/* Memory Exception Register L bit */
#define TCR_V	0x00008000	/* Translation Control Register V bit */

#define HIGHPAGES       (UPAGES+1)  /* 1 page for red zone */

#define	PMAP_ACTIVATE(pmap, th, cpu) \
{\
	extern pmap_t active_pmap; \
	active_pmap = pmap; \
}

#define PMAP_DEACTIVATE(pmap,th,cpu) \
{\
	extern pmap_t active_pmap; \
	active_pmap = PMAP_NULL; \
}

#endif	_PMAP_MACHINE_
