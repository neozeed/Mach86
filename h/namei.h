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
 *	@(#)namei.h	6.10 (Berkeley) 6/8/85
 */
#if	CMU
/*
 **********************************************************************
 * HISTORY
 * 24-Jan-86  Avadis Tevanian (avie) at Carnegie-Mellon University
 *	Updraged to 4.3.
 *
 * 20-Jul-85  Mike Accetta (mja) at Carnegie-Mellon University
 *	CS_RFS:  Added OKREMOTE flag defintion.
 *	[V1(1)]
 **********************************************************************
 */
 
#ifdef	KERNEL
#include "cs_rfs.h"
#else	KERNEL
#include <sys/features.h>
#endif	KERNEL
#endif	CMU

#ifndef _NAMEI_
#define	_NAMEI_

#ifdef KERNEL
#include "uio.h"
#else
#include <sys/uio.h>
#endif

/*
 * Encapsulation of namei parameters.
 * One of these is located in the u. area to
 * minimize space allocated on the kernel stack.
 */
struct nameidata {
	caddr_t	ni_dirp;		/* pathname pointer */
	short	ni_nameiop;		/* see below */
	short	ni_error;		/* error return if any */
	off_t	ni_endoff;		/* end of useful stuff in directory */
	struct	inode *ni_pdir;		/* inode of parent directory of dirp */
	struct	iovec ni_iovec;		/* MUST be pointed to by ni_iov */
	struct	uio ni_uio;		/* directory I/O parameters */
	struct	direct ni_dent;		/* current directory entry */
};

#define	ni_base		ni_iovec.iov_base
#define	ni_count	ni_iovec.iov_len
#define	ni_iov		ni_uio.uio_iov
#define	ni_iovcnt	ni_uio.uio_iovcnt
#define	ni_offset	ni_uio.uio_offset
#define	ni_segflg	ni_uio.uio_segflg
#define	ni_resid	ni_uio.uio_resid

/*
 * namei opertions
 */
#define	LOOKUP		0	/* perform name lookup only */
#define	CREATE		1	/* setup for file creation */
#define	DELETE		2	/* setup for file deletion */
#define	LOCKPARENT	0x10	/* see the top of namei */
#define NOCACHE		0x20	/* name must not be left in cache */
#define FOLLOW		0x40	/* follow symbolic links */
#define	NOFOLLOW	0x0	/* don't follow symbolic links (pseudo) */
#if	CS_RFS
#define	OKREMOTE	0x80	/* allow remote name */

/*
 *  Default namei() defintion for routines which do not wish to
 *  handle remote names.
 */

#define	namei	cnamei

/*
 *  Cover namei() definition to allow remote name processing.
 */
#define	rnamei(ndp)	\
	cnamei(((ndp->ni_nameiop |= OKREMOTE), ndp))

#endif	CS_RFS

/*
 * This structure describes the elements in the cache of recent
 * names looked up by namei.
 */
struct	nch {
	struct	nch *nc_forw, *nc_back;	/* hash chain, MUST BE FIRST */
	struct	nch *nc_nxt, **nc_prev;	/* LRU chain */
	struct	inode *nc_ip;		/* inode the name refers to */
	ino_t	nc_ino;			/* ino of parent of name */
	dev_t	nc_dev;			/* dev of parent of name */
	dev_t	nc_idev;		/* dev of the name ref'd */
	long	nc_id;			/* referenced inode's id */
	char	nc_nlen;		/* length of name */
#define	NCHNAMLEN	15	/* maximum name segment length we bother with */
	char	nc_name[NCHNAMLEN];	/* segment name */
};
struct	nch *nch;
int	nchsize;

/*
 * Stats on usefulness of namei caches.
 */
struct	nchstats {
	long	ncs_goodhits;		/* hits that we can reall use */
	long	ncs_badhits;		/* hits we must drop */
	long	ncs_falsehits;		/* hits with id mismatch */
	long	ncs_miss;		/* misses */
	long	ncs_long;		/* long names that ignore cache */
	long	ncs_pass2;		/* names found with passes == 2 */
	long	ncs_2passes;		/* number of times we attempt it */
};
#endif
