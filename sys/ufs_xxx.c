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
 *	@(#)ufs_xxx.c	6.4 (Berkeley) 6/8/85
 */
#if	CMU
/*
 **********************************************************************
 * HISTORY
 * 26-Jan-86  Avadis Tevanian (avie) at Carnegie-Mellon University
 *	Upgraded to 4.3.
 *
 * 27-Sep-85  Mike Accetta (mja) at Carnegie-Mellon University
 *	Added simple-minded emulation for ofstat() on a pipe.
 *	[V1(1)]
 *
 * 18-Jul-85  Mike Accetta (mja) at Carnegie-Mellon University
 *	CS_RFS:  changed to enable remote names for all namei()
 *	calls in this module.
 *	[V1(1)]
 *
 **********************************************************************
 */
 
#include "cs_compat.h"
#include "cs_rfs.h"
#endif	CMU

#include "param.h"
#include "systm.h"
#include "inode.h"
#include "fs.h"
#include "mount.h"
#include "dir.h"
#include "user.h"
#include "buf.h"
#include "conf.h"

#ifdef COMPAT
#include "file.h"
#include "kernel.h"
#if	CS_RFS
 
/*
 *  Force all namei() calls to permit remote names since this module has
 *  been updated.
 */
#undef	namei
#define	namei	rnamei
#endif	CS_RFS

/*
 * Oh, how backwards compatibility is ugly!!!
 */
struct	ostat {
	dev_t	ost_dev;
	u_short	ost_ino;
	u_short ost_mode;
	short  	ost_nlink;
	short  	ost_uid;
	short  	ost_gid;
	dev_t	ost_rdev;
	int	ost_size;
	int	ost_atime;
	int	ost_mtime;
	int	ost_ctime;
};

/*
 * The old fstat system call.
 */
ofstat()
{
	register struct file *fp;
	register struct a {
		int	fd;
		struct ostat *sb;
	} *uap = (struct a *)u.u_ap;
	extern struct file *getinode();

	fp = getinode(uap->fd);
	if (fp == NULL)
#if	CS_COMPAT
	{
		/*
		 *  Some programs may want to do old style fstat() calls on a
		 *  pipe even though they are no longer files.  The only one
		 *  encountered so far has been the CAT program which checks if
		 *  its input is the same as its output.  This requires only that
		 *  the device and i_numbers of both pipes differ which should
		 *  work out (below).
		 */
		if (u.u_error == EINVAL && (fp=getf(uap->fd)) && (fp->f_flag&FPIPE))
		{
			struct inode pip;

			u.u_error = 0;
			pip.i_flag = 0;			/* prevent IUPDAT */
			pip.i_dev = rootdev;
			pip.i_number = (ino_t)fp->f_data;	/* something hopefully unique */
			pip.i_mode = IFREG;
			pip.i_nlink = 0;
			pip.i_uid = u.u_uid;
			pip.i_gid = u.u_gid;
			pip.i_rdev = rootdev;
			pip.i_size = 0;
			pip.i_atime = time.tv_sec;
			pip.i_mtime = time.tv_sec;
			pip.i_ctime = time.tv_sec;
			ostat1(&pip, uap->sb);
		}
 		return;
	}
#else	CS_COMPAT
		return;
#endif	CS_COMPAT
	ostat1((struct inode *)fp->f_data, uap->sb);
}

/*
 * Old stat system call.  This version follows links.
 */
ostat()
{
	register struct inode *ip;
	register struct a {
		char	*fname;
		struct ostat *sb;
	} *uap = (struct a *)u.u_ap;
	register struct nameidata *ndp = &u.u_nd;

	ndp->ni_nameiop = LOOKUP | FOLLOW;
	ndp->ni_segflg = UIO_USERSPACE;
	ndp->ni_dirp = uap->fname;
	ip = namei(ndp);
	if (ip == NULL)
		return;
	ostat1(ip, uap->sb);
	iput(ip);
}

ostat1(ip, ub)
	register struct inode *ip;
	struct ostat *ub;
{
	struct ostat ds;

	IUPDAT(ip, &time, &time, 0);
	/*
	 * Copy from inode table
	 */
	ds.ost_dev = ip->i_dev;
	ds.ost_ino = (short)ip->i_number;
	ds.ost_mode = (u_short)ip->i_mode;
	ds.ost_nlink = ip->i_nlink;
	ds.ost_uid = (short)ip->i_uid;
	ds.ost_gid = (short)ip->i_gid;
	ds.ost_rdev = (dev_t)ip->i_rdev;
	ds.ost_size = (int)ip->i_size;
	ds.ost_atime = (int)ip->i_atime;
	ds.ost_mtime = (int)ip->i_mtime;
	ds.ost_ctime = (int)ip->i_ctime;
	u.u_error = copyout((caddr_t)&ds, (caddr_t)ub, sizeof(ds));
}

/*
 * Set IUPD and IACC times on file.
 * Can't set ICHG.
 */
outime()
{
	register struct a {
		char	*fname;
		time_t	*tptr;
	} *uap = (struct a *)u.u_ap;
	register struct inode *ip;
	time_t tv[2];
	struct timeval tv0, tv1;

	if ((ip = owner(uap->fname, FOLLOW)) == NULL)
		return;
	u.u_error = copyin((caddr_t)uap->tptr, (caddr_t)tv, sizeof (tv));
	if (u.u_error == 0) {
		ip->i_flag |= IACC|IUPD|ICHG;
		tv0.tv_sec = tv[0]; tv0.tv_usec = 0;
		tv1.tv_sec = tv[1]; tv1.tv_usec = 0;
		iupdat(ip, &tv0, &tv1, 0);
	}
	iput(ip);
}
#endif
