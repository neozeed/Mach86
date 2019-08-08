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
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#ifndef lint
char copyright[] =
"@(#) Copyright (c) 1980 Regents of the University of California.\n\
 All rights reserved.\n";
#endif not lint

#if	CMU
/*
 ************************************************************************
 *
 * HISTORY
 * 20-Mar-86  Avadis Tevanian (avie) at Carnegie-Mellon University
 *	If compiling a system with new VM, then increase NINODE by a
 *	factor of 4 (since the new VM code pages out to inodes).
 *
 ************************************************************************
 */

#include "mach_vm.h"
#endif	CMU

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/socket.h"
#include "../h/dir.h"
#include "../h/user.h"
#include "../h/proc.h"
#include "../h/text.h"
#include "../h/inode.h"
#include "../h/file.h"
#include "../h/callout.h"
#include "../h/clist.h"
#include "../h/cmap.h"
#include "../h/mbuf.h"
#include "../h/quota.h"
#include "../h/kernel.h"
/*
 * System parameter formulae.
 *
 * This file is copied into each directory where we compile
 * the kernel; it should be modified there to suit local taste
 * if necessary.
 *
 * Compiled with -DHZ=xx -DTIMEZONE=x -DDST=x -DMAXUSERS=xx
 */

#define	HZ 100
int	hz = HZ;
int	tick = 1000000 / HZ;
int	tickadj = 1000000 / HZ / 10;
struct	timezone tz = { TIMEZONE, DST };
#define	NPROC (20 + 8 * MAXUSERS)
int	nproc = NPROC;
int	ntext = 36 + MAXUSERS;
#if	MACH_VM
#define NINODE (((NPROC + 16 + MAXUSERS) + 32)*4)
#else	MACH_VM
#define NINODE ((NPROC + 16 + MAXUSERS) + 32)
#endif	MACH_VM
int	ninode = NINODE;
int	nchsize = NINODE * 11 / 10;
int	nfile = 16 * (NPROC + 16 + MAXUSERS) / 10 + 32;
int	ncallout = 16 + NPROC;
int	nclist = 100 + 16 * MAXUSERS;
int	nport = NPROC / 2;
int     nmbclusters = NMBCLUSTERS;
#ifdef QUOTA
int	nquota = (MAXUSERS * 9) / 7 + 3;
int	ndquot = NINODE + (MAXUSERS * NMOUNT) / 4;
#endif

/*
 * These are initialized at bootstrap time
 * to values dependent on memory size
 */
int	nbuf, nswbuf;

/*
 * These have to be allocated somewhere; allocating
 * them here forces loader errors if this file is omitted.
 */
struct	proc *proc, *procNPROC;
struct	text *text, *textNTEXT;
struct	inode *inode, *inodeNINODE;
struct	file *file, *fileNFILE;
struct 	callout *callout;
struct	cblock *cfree;
struct	buf *buf, *swbuf;
short	*swsize;
int	*swpf;
char	*buffers;
struct	cmap *cmap, *ecmap;
struct	nch *nch;
#ifdef QUOTA
struct	quota *quota, *quotaNQUOTA;
struct	dquot *dquot, *dquotNDQUOT;
#endif
