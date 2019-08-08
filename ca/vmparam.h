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
/* $Header: vmparam.h,v 4.1 85/07/24 10:45:18 ruth Exp $ */
/* $Source: /ibm/acis/usr/sys_ca/ca/RCS/vmparam.h,v $ */

#if !defined(lint) && !defined(LOCORE) && defined(RCS_HDRS)
static char *rcsidca_vmparam = "$Header: vmparam.h,v 4.1 85/07/24 10:45:18 ruth Exp $";
#endif

#if CMU
/***********************************************************************
 * HISTORY
 * 26-Feb-86  Bill Bolosky (bolosky) at Carnegie-Mellon University
 *	Added definitions of LOTSFREE, DFLSSIZ, DFLDSIZ, HANDSPREAD,
 *	RATETOSCHEDPAGING, DMMIN, DMMAX and DMTEXT from vax and ibm code.
 *
 */
#ifdef	KERNEL
#include "mach_vm.h"
#else	KERNEL
#include <sys/features.h>
#endif	KERNEL
#endif CMU

#if	MACH_VM
#ifdef	KERNEL
#include "../vm/vm_param.h"
#else	KERNEL
#include <vm/vm_param.h>
#endif	KERNEL
#endif	MACH_VM

/*     vmparam.h       6.1     83/07/29        */

/*
 * Machine dependent constants for ROMP
 */
/*
 * USRTEXT is the start of the user text/data space, while USRSTACK
 * is the top (end) of the user stack.  LOWPAGES and HIGHPAGES are
 * the number of pages from the beginning of the P0 region to the
 * beginning of the text and from the beginning of the P1 region to the
 * beginning of the stack respectively.
 */

#define ENDOFP1         0x20000000  /* first address past P1 */
#define P1PAGES         0x20000     /* number of pages in P1 region */
#define LOWPAGES        0           /* this is not implemented */
#define HIGHPAGES       (UPAGES+1)  /* 1 page for red zone */

#define USRTEXT         0
#if	MACH_VM
#define USRSTACK        (trunc_page(ENDOFP1-HIGHPAGES*NBPG))
#else	MACH_VM
#define USRSTACK        (ENDOFP1-HIGHPAGES*NBPG) /* top of user stack */
#endif	MACH_VM
#define KERNSTACK       (ENDOFP1-sizeof(struct user)) /* top of kernel stack */
#define UAREA           (ENDOFP1-UPAGES*NBPG) /* start of u area */

#define uareabase(u)    ((caddr_t)(u)+sizeof(struct user)-UPAGES*NBPG)

/*
 * Virtual memory related constants ??? JEC ???
 */
#define SLOP    32
#define MAXTSIZ         (6*2048-SLOP)           /* max text size (clicks) */
#ifndef DFLDSIZ
#define	DFLDSIZ		(6*1024*1024/NBPG)	/* initial data size limit */
#endif
#ifndef MAXDSIZ
#define MAXDSIZ         (12*1024-32-SLOP)       /* max data size (clicks       ) */
#endif
#ifndef	DFLSSIZ
#define	DFLSSIZ		(512*1024/NBPG)		/* initial stack size limit */
#endif
#define MAXSSIZ         (12*1024-32-SLOP)       /* max stack size (clicks) */
/*
 * Default sizes of swap allocation chunks (see dmap.h).
 * The actual values may be changed in vminit() based on MAXDSIZ.
 * With MAXDSIZ of 16Mb and NDMAP of 38, dmmax will be 1024.
 */
#ifndef DMMIN
#define	DMMIN	32			/* smallest swap allocation */
#endif DMMIN
#ifndef DMMAX
#define	DMMAX	1024			/* largest potential swap allocation */
#endif DMMAX
#ifndef DMTEXT
#define	DMTEXT	1024			/* swap allocation for text */
#endif DMTEXT

/*
 * Sizes of the system and user portions of the system page table.
 */
/* SYSPTSIZE IS SILLY; IT SHOULD BE COMPUTED AT BOOT TIME */
#define SYSPTSIZE       ((20+MAXUSERS)*NPTEPG)
#define USRPTSIZE       (1*NPTEPG)

/*
 * The size of the clock loop.
 */
#define LOOPPAGES       (maxfree - firstfree)

/*
 * The time for a process to be blocked before being very swappable.
 * This is a number of seconds which the system takes as being a non-trivial
 * amount of real time.  You probably shouldn't change this;
 * it is used in subtle ways (fractions and multiples of it are, that is, like
 * half of a ||long time'', almost a long time, etc.)
 * It is related to human patience and other factors which don't really
 * change over time.
 */
#define MAXSLP          20

/*
 * A swapped in process is given a small amount of core without being bothered
 * by the page replacement algorithm.  Basically this says that if you are
 * swapped in you deserve some resources.  We protect the last SAFERSS
 * pages against paging and will just swap you out rather than paging you.
 * Note that each process has at least UPAGES+CLSIZE pages which are not
 * paged anyways (this is currently 8+2=10 pages or 5k bytes), so this
 * number just means a swapped in process is given around 25k bytes.
 * Just for fun: current memory prices are 4600$ a megabyte on VAX (4/22/81),
 * so we loan each swapped in process memory worth 100$, or just admit
 * that we don't consider it worthwhile and swap it out to disk which costs
 * $30/mb or about $0.75.
 */
#define SAFERSS         32              /* nominal ||small'' resident set size
					  protected against replacement */

/*
 * DISKRPM is used to estimate the number of paging i/o operations
 * which one can expect from a single disk controller.
 */
#define DISKRPM         60

/*
 * Klustering constants.  Klustering is the gathering
 * of pages together for pagein/pageout, while clustering
 * is the treatment of hardware page size as though it were
 * larger than it really is.
 *
 * KLMAX gives maximum cluster size in CLSIZE page (cluster-page)
 * units.  Note that KLMAX*CLSIZE must be <= DMMIN in dmap.h.
 *
 * Modified 5/84 by JEC.  Romp does not do klustering, because it
 * must do disk i/o to physical addresses, while Vax can (evidently)
 * do i/o to logical addresses.
 */

#define KLMAX   CLSIZE
#define KLSEQL  CLSIZE           /* in klust if vadvise(VA_SEQL) */
#define KLIN    CLSIZE           /* default data/stack in klust */
#define KLTXT   CLSIZE           /* default text in klust */
#define KLOUT   CLSIZE

/*
 * KLSDIST is the advance or retard of the fifo reclaim for sequential
 * processes data space.
 */
#define KLSDIST 3               /* klusters advance/retard for seq. fifo */

/*
 * Paging thresholds (see vm_sched.c).
 * Strategy of 4/22/81:
 *     lotsfree is 1/4 of memory free.
 *     desfree is 200k bytes, but at most 1/8 of memory
 *     minfree is 64k bytes, but at most 1/2 of desfree
 */
#define LOTSFREE	(512 * 1024)
#define LOTSFREEFRACT   4
#define DESFREE         (200 * 1024)
#define DESFREEFRACT    8
#define MINFREE         (64 * 1024)
#define MINFREEFRACT    2

/*
 * There are two clock hands, initially separated by HANDSPREAD bytes
 * (but at most all of user memory).  The amount of time to reclaim
 * a page once the pageout process examines it increases with this
 * distance and decreases as the scan rate rises.
 */
#define	HANDSPREAD	(2 * 1024 * 1024)

/*
 * The number of times per second to recompute the desired paging rate
 * and poke the pagedaemon.
 */
#define	RATETOSCHEDPAGING	4

/*
 * Believed threshold (in megabytes) for which interleaved
 * swapping area is desirable.
 */
#define LOTSOFMEM       3

