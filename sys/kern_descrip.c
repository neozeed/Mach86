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
 *	@(#)kern_descrip.c	6.11 (Berkeley) 6/8/85
 */
#if	CMU
/*
 **********************************************************************
 * HISTORY
 * 25-Jan-86  Avadis Tevanian (avie) at Carnegie-Mellon University
 *	Upgraded to 4.3.
 *
 * 13-Jan-86  Robert V Baron (rvb) at Carnegie-Mellon University
 *	sensor changes for mll
 *
 * 18-Jul-85  Mike Accetta (mja) at Carnegie-Mellon University
 *	CS_RFS:  Added explicit hook in fstat() into remote system
 *	call handling.
 *	[V1(1)]
 *
 **********************************************************************
 */
 
#include "cs_bugfix.h"
#include "cs_rfs.h"
#include "wb_sens.h"
#endif	CMU

#include "param.h"
#include "systm.h"
#include "dir.h"
#include "user.h"
#include "kernel.h"
#include "inode.h"
#include "proc.h"
#include "file.h"
#include "socket.h"
#include "socketvar.h"
#include "mount.h"
#include "stat.h"

#include "ioctl.h"

#if	NWB_SENS > 0
#include "../sensor/kern_sensors.h"
#endif	NWB_SENS > 0
/*
 * Descriptor management.
 */

/*
 * TODO:
 *	eliminate u.u_error side effects
 */

/*
 * System calls on descriptors.
 */
getdtablesize()
{

	u.u_r.r_val1 = NOFILE;
}

getdopt()
{

}

setdopt()
{

}

dup()
{
	register struct a {
		int	i;
	} *uap = (struct a *) u.u_ap;
	struct file *fp;
	int j;

	if (uap->i &~ 077) { uap->i &= 077; dup2(); return; }	/* XXX */

	GETF(fp, uap->i);
	j = ufalloc(0);
	if (j < 0)
		return;
	dupit(j, fp, u.u_pofile[uap->i] &~ UF_EXCLOSE);
}

dup2()
{
	register struct a {
		int	i, j;
	} *uap = (struct a *) u.u_ap;
	register struct file *fp;

	GETF(fp, uap->i);
	if (uap->j < 0 || uap->j >= NOFILE) {
		u.u_error = EBADF;
		return;
	}
	u.u_r.r_val1 = uap->j;
	if (uap->i == uap->j)
		return;
	if (u.u_ofile[uap->j]) {
		if (u.u_pofile[uap->j] & UF_MAPPED)
			munmapfd(uap->j);
		closef(u.u_ofile[uap->j]);
		if (u.u_error)
			return;
	}
	dupit(uap->j, fp, u.u_pofile[uap->i] &~ UF_EXCLOSE);
}

dupit(fd, fp, flags)
	int fd;
	register struct file *fp;
	register int flags;
{

	u.u_ofile[fd] = fp;
	u.u_pofile[fd] = flags;
	fp->f_count++;
	if (fd > u.u_lastfile)
		u.u_lastfile = fd;
}

/*
 * The file control system call.
 */
fcntl()
{
	register struct file *fp;
	register struct a {
		int	fdes;
		int	cmd;
		int	arg;
	} *uap;
	register i;
	register char *pop;

	uap = (struct a *)u.u_ap;
	GETF(fp, uap->fdes);
	pop = &u.u_pofile[uap->fdes];
	switch(uap->cmd) {
	case F_DUPFD:
		i = uap->arg;
		if (i < 0 || i >= NOFILE) {
			u.u_error = EINVAL;
			return;
		}
		if ((i = ufalloc(i)) < 0)
			return;
		dupit(i, fp, *pop &~ UF_EXCLOSE);
		break;

	case F_GETFD:
		u.u_r.r_val1 = *pop & 1;
		break;

	case F_SETFD:
		*pop = (*pop &~ 1) | (uap->arg & 1);
		break;

	case F_GETFL:
		u.u_r.r_val1 = fp->f_flag+FOPEN;
		break;

	case F_SETFL:
		fp->f_flag &= FCNTLCANT;
		fp->f_flag |= (uap->arg-FOPEN) &~ FCNTLCANT;
		u.u_error = fset(fp, FNDELAY, fp->f_flag & FNDELAY);
		if (u.u_error)
			break;
		u.u_error = fset(fp, FASYNC, fp->f_flag & FASYNC);
		if (u.u_error)
			(void) fset(fp, FNDELAY, 0);
		break;

	case F_GETOWN:
		u.u_error = fgetown(fp, &u.u_r.r_val1);
		break;

	case F_SETOWN:
		u.u_error = fsetown(fp, uap->arg);
		break;

	default:
		u.u_error = EINVAL;
	}
}

fset(fp, bit, value)
	struct file *fp;
	int bit, value;
{

	if (value)
		fp->f_flag |= bit;
	else
		fp->f_flag &= ~bit;
	return (fioctl(fp, (int)(bit == FNDELAY ? FIONBIO : FIOASYNC),
	    (caddr_t)&value));
}

fgetown(fp, valuep)
	struct file *fp;
	int *valuep;
{
	int error;

	switch (fp->f_type) {

	case DTYPE_SOCKET:
		*valuep = ((struct socket *)fp->f_data)->so_pgrp;
		return (0);

	default:
		error = fioctl(fp, (int)TIOCGPGRP, (caddr_t)valuep);
		*valuep = -*valuep;
		return (error);
	}
}

fsetown(fp, value)
	struct file *fp;
	int value;
{

	if (fp->f_type == DTYPE_SOCKET) {
		((struct socket *)fp->f_data)->so_pgrp = value;
		return (0);
	}
	if (value > 0) {
		struct proc *p = pfind(value);
		if (p == 0)
			return (EINVAL);
		value = p->p_pgrp;
	} else
		value = -value;
	return (fioctl(fp, (int)TIOCSPGRP, (caddr_t)&value));
}

fioctl(fp, cmd, value)
	struct file *fp;
	int cmd;
	caddr_t value;
{

	return ((*fp->f_ops->fo_ioctl)(fp, cmd, value));
}

close()
{
	struct a {
		int	i;
	} *uap = (struct a *)u.u_ap;
	register int i = uap->i;
	register struct file *fp;
	register u_char *pf;

	GETF(fp, i);
	pf = (u_char *)&u.u_pofile[i];
	if (*pf & UF_MAPPED)
		munmapfd(i);
	u.u_ofile[i] = NULL;
	while (u.u_lastfile >= 0 && u.u_ofile[u.u_lastfile] == NULL)
		u.u_lastfile--;
	*pf = 0;
 	closef(fp);
	/* WHAT IF u.u_error ? */
}

fstat()
{
	register struct file *fp;
	register struct a {
		int	fdes;
		struct	stat *sb;
	} *uap;
	struct stat ub;

	uap = (struct a *)u.u_ap;
	GETF(fp, uap->fdes);
	switch (fp->f_type) {

#if	CS_RFS
	/*
	 *  The original code probably should have included fstat() in the file
	 *  ops structure in which case these explicit hooks could have been
	 *  avoided but ...
	 */
	case DTYPE_RFSINO:
		u.u_error = rfs_fstat(fp);
		return;
	case DTYPE_RFSCTL:
		u.u_error = rfsC_fstat(fp);
		return;
#endif	CS_RFS
	case DTYPE_INODE:
		u.u_error = ino_stat((struct inode *)fp->f_data, &ub);
		break;

	case DTYPE_SOCKET:
		u.u_error = soo_stat((struct socket *)fp->f_data, &ub);
		break;

	default:
		panic("fstat");
		/*NOTREACHED*/
	}
	if (u.u_error == 0)
		u.u_error = copyout((caddr_t)&ub, (caddr_t)uap->sb,
		    sizeof (ub));
}

/*
 * Allocate a user file descriptor.
 */
ufalloc(i)
	register int i;
{

	for (; i < NOFILE; i++)
		if (u.u_ofile[i] == NULL) {
			u.u_r.r_val1 = i;
			u.u_pofile[i] = 0;
			if (i > u.u_lastfile)
				u.u_lastfile = i;
			return (i);
		}
	u.u_error = EMFILE;
	return (-1);
}

ufavail()
{
	register int i, avail = 0;

	for (i = 0; i < NOFILE; i++)
		if (u.u_ofile[i] == NULL)
			avail++;
	return (avail);
}

struct	file *lastf;
/*
 * Allocate a user file descriptor
 * and a file structure.
 * Initialize the descriptor
 * to point at the file structure.
 */
struct file *
falloc()
{
	register struct file *fp;
	register i;

	i = ufalloc(0);
	if (i < 0)
		return (NULL);
	if (lastf == 0)
		lastf = file;
	for (fp = lastf; fp < fileNFILE; fp++)
		if (fp->f_count == 0)
			goto slot;
	for (fp = file; fp < lastf; fp++)
		if (fp->f_count == 0)
			goto slot;
	tablefull("file");
	u.u_error = ENFILE;
	return (NULL);
slot:
	u.u_ofile[i] = fp;
	fp->f_count = 1;
	fp->f_data = 0;
	fp->f_offset = 0;
	lastf = fp + 1;
	return (fp);
}

/*
 * Convert a user supplied file descriptor into a pointer
 * to a file structure.  Only task is to check range of the descriptor.
 * Critical paths should use the GETF macro.
 */
struct file *
getf(f)
	register int f;
{
	register struct file *fp;

	if ((unsigned)f >= NOFILE || (fp = u.u_ofile[f]) == NULL) {
		u.u_error = EBADF;
		return (NULL);
	}
	return (fp);
}

/*
 * Internal form of close.
 * Decrement reference count on file structure.
 * If last reference not going away, but no more
 * references except in message queues, run a
 * garbage collect.  This would better be done by
 * forcing a gc() to happen sometime soon, rather
 * than running one each time.
 */
closef(fp)
	register struct file *fp;
{

	if (fp == NULL)
		return;
#if	NWB_SENS > 0
 	FileClose(((struct inode *)fp->f_data)->i_dev,
		  ((struct inode *)fp->f_data)->i_number,
		  ((struct inode *)fp->f_data)->i_size);
#endif	NWB_SENS > 0
	if (fp->f_count > 1) {
		fp->f_count--;
		if (fp->f_count == fp->f_msgcount)
			unp_gc();
		return;
	}
	(*fp->f_ops->fo_close)(fp);
	fp->f_count = 0;
}

/*
 * Apply an advisory lock on a file descriptor.
 */
flock()
{
	register struct a {
		int	fd;
		int	how;
	} *uap = (struct a *)u.u_ap;
	register struct file *fp;

	GETF(fp, uap->fd);
	if (fp->f_type != DTYPE_INODE) {
		u.u_error = EOPNOTSUPP;
		return;
	}
	if (uap->how & LOCK_UN) {
		ino_unlock(fp, FSHLOCK|FEXLOCK);
		return;
	}
	if ((uap->how & (LOCK_SH | LOCK_EX)) == 0)
		return;					/* error? */
	if (uap->how & LOCK_EX)
		uap->how &= ~LOCK_SH;
	/* avoid work... */
	if ((fp->f_flag & FEXLOCK) && (uap->how & LOCK_EX) ||
	    (fp->f_flag & FSHLOCK) && (uap->how & LOCK_SH))
		return;
	u.u_error = ino_lock(fp, uap->how);
}
