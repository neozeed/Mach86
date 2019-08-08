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
 * Copyright (c) 1982 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)file.h	6.3 (Berkeley) 6/8/85
 */
#if	CMU
/*
 **********************************************************************
 * HISTORY
 * 26-Mar-86  Mike Accetta (mja) at Carnegie-Mellon University
 *	CS_BUGFIX: Fixed KERNEL_FILE symbol to include all
 *	definitions.
 *
 * 29-Jan-86  Mike Accetta (mja) at Carnegie-Mellon University
 *	CS_COMPAT:  Added FDIROK definition.
 *
 * 25-Jan-86  Avadis Tevanian (avie) at Carnegie-Mellon University
 *	Upgraded to 4.3.
 *
 * 25-Nov-85  Mike Accetta (mja) at Carnegie-Mellon University
 *	CS_BUGFIX: Added new KERNEL_FILE symbol to enable struct
 *	file definitions when desired outside the kernel.
 *
 * 20-Jul-85  Mike Accetta (mja) at Carnegie-Mellon University
 *	CS_RFS:  Added DTYPE_RFSINO definition.
 *	[V1(1)]
 *
 * 13-May-85  Mike Accetta (mja) at Carnegie-Mellon University
 *	Upgraded from 4.1BSD.  Carried over change below.
 *	CS_COMPAT: retained pipe definitions for now;
 *	CS_XMOD:  retained exclusive use definitions for now.
 *	[V1(1)]
 *
 **********************************************************************
 */
 
#ifdef	KERNEL
#include "cs_compat.h"
#include "cs_bugfix.h"
#include "cs_rfs.h"
#include "cs_xmod.h"
#else	KERNEL
#include <sys/features.h>
#endif	KERNEL
#endif	CMU

#if	CS_BUGFIX
#ifdef	KERNEL_FILE
#define	KERNEL
#endif	KERNEL_FILE
#endif	CS_BUGFIX
#ifdef KERNEL
/*
 * Descriptor table entry.
 * One for each kernel object.
 */
struct	file {
	int	f_flag;		/* see below */
	short	f_type;		/* descriptor type */
	short	f_count;	/* reference count */
	short	f_msgcount;	/* references from message queue */
	struct	fileops {
		int	(*fo_rw)();
		int	(*fo_ioctl)();
		int	(*fo_select)();
		int	(*fo_close)();
	} *f_ops;
	caddr_t	f_data;		/* inode */
#if	CS_COMPAT
	union {
		off_t	fu_offset;	/* read/write character pointer */
		struct chan *f_chan;	/* mpx channel pointer */
	} f_un;
#define	f_offset	f_un.fu_offset
#else	CS_COMPAT
	off_t	f_offset;
#endif	CS_COMPAT
};

struct	file *file, *fileNFILE;
int	nfile;
struct	file *getf();
struct	file *falloc();
#endif

/*
 * flags- also for fcntl call.
 */
#define	FOPEN		(-1)
#define	FREAD		00001		/* descriptor read/receive'able */
#define	FWRITE		00002		/* descriptor write/send'able */
#ifndef	F_DUPFD
#define	FNDELAY		00004		/* no delay */
#define	FAPPEND		00010		/* append on each write */
#endif
#define	FMARK		00020		/* mark during gc() */
#define	FDEFER		00040		/* defer for next gc pass */
#ifndef	F_DUPFD
#define	FASYNC		00100		/* signal pgrp when data ready */
#endif
#define	FSHLOCK		00200		/* shared lock present */
#define	FEXLOCK		00400		/* exclusive lock present */

/* bits to save after open */
#define	FMASK		00113
#define	FCNTLCANT	(FREAD|FWRITE|FMARK|FDEFER|FSHLOCK|FEXLOCK)

/* open only modes */
#define	FCREAT		01000		/* create if nonexistant */
#define	FTRUNC		02000		/* truncate to zero length */
#define	FEXCL		04000		/* error if already created */
#if	CS_COMPAT
#define	FDIROK		010000
#define	FPIPE		020000
#define	FMPX		040000
#define	FMPY		0100000
#define	FMP		0140000
#endif	CS_COMPAT
#if	CS_XMOD
#define	FXMODE		0200000		/* exclusive use mode set */
#endif	CS_XMOD

#ifndef	F_DUPFD
/* fcntl(2) requests--from <fcntl.h> */
#define	F_DUPFD	0	/* Duplicate fildes */
#define	F_GETFD	1	/* Get fildes flags */
#define	F_SETFD	2	/* Set fildes flags */
#define	F_GETFL	3	/* Get file flags */
#define	F_SETFL	4	/* Set file flags */
#define	F_GETOWN 5	/* Get owner */
#define F_SETOWN 6	/* Set owner */
#endif

/*
 * User definitions.
 */

/*
 * Open call.
 */
#define	O_RDONLY	000		/* open for reading */
#define	O_WRONLY	001		/* open for writing */
#define	O_RDWR		002		/* open for read & write */
#define	O_NDELAY	FNDELAY		/* non-blocking open */
#define	O_APPEND	FAPPEND		/* append on each write */
#define	O_CREAT		FCREAT		/* open with file create */
#define	O_TRUNC		FTRUNC		/* open with truncation */
#define	O_EXCL		FEXCL		/* error on create if file exists */

/*
 * Flock call.
 */
#define	LOCK_SH		1	/* shared lock */
#define	LOCK_EX		2	/* exclusive lock */
#define	LOCK_NB		4	/* don't block when locking */
#define	LOCK_UN		8	/* unlock */

/*
 * Access call.
 */
#define	F_OK		0	/* does file exist */
#define	X_OK		1	/* is it executable by caller */
#define	W_OK		2	/* writable by caller */
#define	R_OK		4	/* readable by caller */

/*
 * Lseek call.
 */
#define	L_SET		0	/* absolute offset */
#define	L_INCR		1	/* relative to current offset */
#define	L_XTND		2	/* relative to end of file */

#ifdef KERNEL
#define	GETF(fp, fd) { \
	if ((unsigned)(fd) >= NOFILE || ((fp) = u.u_ofile[fd]) == NULL) { \
		u.u_error = EBADF; \
		return; \
	} \
}
#define	DTYPE_INODE	1	/* file */
#define	DTYPE_SOCKET	2	/* communications endpoint */
#if	CS_RFS
#define	DTYPE_RFSINO	3	/* remote file system inode */
#define	DTYPE_RFSCTL	4	/* remote file system control */
#endif	CS_RFS
#endif
#if	CS_BUGFIX
#ifdef	KERNEL_FILE
#undef	KERNEL
#endif	KERNEL_FILE
#endif	CS_BUGFIX
