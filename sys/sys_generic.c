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
 *	@(#)sys_generic.c	6.12 (Berkeley) 6/17/85
 */
#if	CMU
/*
 **********************************************************************
 * HISTORY
 * 10-May-86  Avadis Tevanian (avie) at Carnegie-Mellon University
 *	fixed a CS_NOFILE ifdef to also require CS_XMOD.  All of this
 *	should go away anyway.
 *
 * 25-Jan-86  Avadis Tevanian (avie) at Carnegie-Mellon University
 *	Upgraded to 4.3.
 *
 * 13-Jan-86  Robert V Baron (rvb) at Carnegie-Mellon University
 *	add sensors for mll
 *
 * 10-Oct-85  Avadis Tevanian (avie) at Carnegie-Mellon University
 *	Apply bug fix for selwakeup.
 *
 * 12-Aug-85  Mike Accetta (mja) at Carnegie-Mellon University
 *	CS_COMPAT:  Changed rwuio() to allow restarts unless old
 *	signal mechanism is in use without job-control.
 *	[V1(1)]
 *
 * 03-Aug-85  Mike Accetta (mja) at Carnegie-Mellon University
 *	CS_RPAUSE:  added resource pause hook in rwuio() for
 *	disk space exhaustion.
 *
 * 29-Jul-85  Mike Accetta (mja) at Carnegie-Mellon University
 *	CS_COMPAT:  Added MPX hooks for now.
 *	[V1(1)]
 *
 * 12-Jun-85  Mike Accetta (mja) at Carnegie-Mellon University
 *	CS_NOFILE:  carried over FIOCXNOF ioctl call.
 *	[V1(1)]
 *
 * 20-May-85  Glenn Marcy (gm0w) at Carnegie-Mellon University
 *	Upgraded from 4.1BSD.  Carried over changes below:
 *
 *	CS_COMPAT:  Added stty()/gtty() compatability routines.
 *	[V1(1)]
 *
 **********************************************************************
 */
 
#include "cs_compat.h"
#include "cs_rpause.h"
#include "cs_xmod.h"
#include "cs_nofile.h"
#include "cmu_bugfix.h"
#include "wb_sens.h"
#endif	CMU

#include "param.h"
#include "systm.h"
#include "dir.h"
#include "user.h"
#include "ioctl.h"
#include "file.h"
#include "proc.h"
#include "uio.h"
#include "kernel.h"
#include "stat.h"

#if	NWB_SENS > 0
#include "inode.h"
#include "../sensor/sys_sensors.h"
#endif	NWB_SENS > 0
/*
 * Read system call.
 */
read()
{
	register struct a {
		int	fdes;
		char	*cbuf;
		unsigned count;
	} *uap = (struct a *)u.u_ap;
	struct uio auio;
	struct iovec aiov;

	aiov.iov_base = (caddr_t)uap->cbuf;
	aiov.iov_len = uap->count;
	auio.uio_iov = &aiov;
	auio.uio_iovcnt = 1;
	rwuio(&auio, UIO_READ);
}

readv()
{
	register struct a {
		int	fdes;
		struct	iovec *iovp;
		int	iovcnt;
	} *uap = (struct a *)u.u_ap;
	struct uio auio;
	struct iovec aiov[16];		/* XXX */

	if (uap->iovcnt <= 0 || uap->iovcnt > sizeof(aiov)/sizeof(aiov[0])) {
		u.u_error = EINVAL;
		return;
	}
	auio.uio_iov = aiov;
	auio.uio_iovcnt = uap->iovcnt;
	u.u_error = copyin((caddr_t)uap->iovp, (caddr_t)aiov,
	    (unsigned)(uap->iovcnt * sizeof (struct iovec)));
	if (u.u_error)
		return;
	rwuio(&auio, UIO_READ);
}

/*
 * Write system call
 */
write()
{
	register struct a {
		int	fdes;
		char	*cbuf;
		int	count;
	} *uap = (struct a *)u.u_ap;
	struct uio auio;
	struct iovec aiov;

	auio.uio_iov = &aiov;
	auio.uio_iovcnt = 1;
	aiov.iov_base = uap->cbuf;
	aiov.iov_len = uap->count;
	rwuio(&auio, UIO_WRITE);
}

writev()
{
	register struct a {
		int	fdes;
		struct	iovec *iovp;
		int	iovcnt;
	} *uap = (struct a *)u.u_ap;
	struct uio auio;
	struct iovec aiov[16];		/* XXX */

	if (uap->iovcnt <= 0 || uap->iovcnt > sizeof(aiov)/sizeof(aiov[0])) {
		u.u_error = EINVAL;
		return;
	}
	auio.uio_iov = aiov;
	auio.uio_iovcnt = uap->iovcnt;
	u.u_error = copyin((caddr_t)uap->iovp, (caddr_t)aiov,
	    (unsigned)(uap->iovcnt * sizeof (struct iovec)));
	if (u.u_error)
		return;
	rwuio(&auio, UIO_WRITE);
}

rwuio(uio, rw)
	register struct uio *uio;
	enum uio_rw rw;
{
	struct a {
		int	fdes;
	};
	register struct file *fp;
	register struct iovec *iov;
	int i, count;
#if	CS_RPAUSE
	int rcount;	/* resume count */
#endif	CS_RPAUSE
#if	NWB_SENS > 0
	struct inode *ip;
#endif	NWB_SENS > 0

	GETF(fp, ((struct a *)u.u_ap)->fdes);
	if ((fp->f_flag&(rw==UIO_READ ? FREAD : FWRITE)) == 0) {
		u.u_error = EBADF;
		return;
	}
	uio->uio_resid = 0;
	uio->uio_segflg = UIO_USERSPACE;
	iov = uio->uio_iov;
	for (i = 0; i < uio->uio_iovcnt; i++) {
		if (iov->iov_len < 0) {
			u.u_error = EINVAL;
			return;
		}
		uio->uio_resid += iov->iov_len;
		if (uio->uio_resid < 0) {
			u.u_error = EINVAL;
			return;
		}
		iov++;
	}
	count = uio->uio_resid;
#if	CS_RPAUSE
resume:
	rcount = uio->uio_resid;
#endif	CS_RPAUSE
#if	CS_COMPAT
	if (fp->f_flag&FMP)
		uio->uio_offset = 0;
	else
		uio->uio_offset = fp->f_offset;
	if (/*(u.u_procp->p_flag&SJCSIG == 0) &&*/ setjmp(&u.u_qsave)) {
#else	CS_COMPAT
	uio->uio_offset = fp->f_offset;
	if (setjmp(&u.u_qsave)) {
#endif	CS_COMPAT
		if (uio->uio_resid == count) {
			if ((u.u_sigintr & sigmask(u.u_procp->p_cursig)) != 0)
				u.u_error = EINTR;
			else
				u.u_eosys = RESTARTSYS;
		}
	} else
		u.u_error = (*fp->f_ops->fo_rw)(fp, rw, uio);
	u.u_r.r_val1 = count - uio->uio_resid;
#if	NWB_SENS > 0
 	ip = (struct inode *)(fp->f_data);
 	if (rw == UIO_READ)					    
 		{						    
 		ReadSensor(ip->i_dev,				    
 			ip->i_number,				    
 			fp->f_offset,				    
 			u.u_r.r_val1);				    
 		}						    
 	else if (rw == UIO_WRITE)				    
 		{						    
 		WriteSensor(ip->i_dev,				    
 			ip->i_number,				    
 			fp->f_offset,				    
 			u.u_r.r_val1);				    
 		}						    
#endif	NWB_SENS > 0
#if	CS_COMPAT
	if ((fp->f_flag&FMP) == 0)
#endif	CS_COMPAT
#if	CS_RPAUSE
	fp->f_offset += (rcount - uio->uio_resid);
	if (u.u_error && fspause()) goto resume;
#else	CS_RPAUSE
	fp->f_offset += u.u_r.r_val1;
#endif	CS_RPAUSE
}
#if	CS_COMPAT
 
 
 
/*
 * stty/gtty writearound
 */
stty()
{
	u.u_arg[2] = u.u_arg[1];
	u.u_arg[1] = TIOCSETP;
	ioctl();
}
 
gtty()
{
	u.u_arg[2] = u.u_arg[1];
	u.u_arg[1] = TIOCGETP;
	ioctl();
}
#endif	CS_COMPAT

/*
 * Ioctl system call
 */
ioctl()
{
	register struct file *fp;
	struct a {
		int	fdes;
		int	cmd;
		caddr_t	cmarg;
	} *uap;
	register int com;
	register u_int size;
	char data[IOCPARM_MASK+1];

	uap = (struct a *)u.u_ap;
#if	CS_NOFILE && CS_XMOD
	if ((uap->cmd&0xffff)==(FIOCXNOF&0xffff))
	{
		if (uap->fdes > NOFILE)
			u.u_error = EBADF;
		return;
	}
#endif	CS_NOFILE && CS_XMOD
	GETF(fp, uap->fdes);
	if ((fp->f_flag & (FREAD|FWRITE)) == 0) {
		u.u_error = EBADF;
		return;
	}
	com = uap->cmd;

#if defined(vax) && defined(COMPAT)
	/*
	 * Map old style ioctl's into new for the
	 * sake of backwards compatibility (sigh).
	 */
	if ((com&~0xffff) == 0) {
		com = mapioctl(com);
		if (com == 0) {
			u.u_error = EINVAL;
			return;
		}
	}
#endif
	if (com == FIOCLEX) {
		u.u_pofile[uap->fdes] |= UF_EXCLOSE;
		return;
	}
	if (com == FIONCLEX) {
		u.u_pofile[uap->fdes] &= ~UF_EXCLOSE;
		return;
	}

	/*
	 * Interpret high order word to find
	 * amount of data to be copied to/from the
	 * user's address space.
	 */
	size = (com &~ (IOC_INOUT|IOC_VOID)) >> 16;
	if (size > sizeof (data)) {
		u.u_error = EFAULT;
		return;
	}
	if (com&IOC_IN) {
		if (size) {
			u.u_error =
			    copyin(uap->cmarg, (caddr_t)data, (u_int)size);
			if (u.u_error)
				return;
		} else
			*(caddr_t *)data = uap->cmarg;
	} else if ((com&IOC_OUT) && size)
		/*
		 * Zero the buffer on the stack so the user
		 * always gets back something deterministic.
		 */
		bzero((caddr_t)data, size);
	else if (com&IOC_VOID)
		*(caddr_t *)data = uap->cmarg;

	switch (com) {

	case FIONBIO:
		u.u_error = fset(fp, FNDELAY, *(int *)data);
		return;

	case FIOASYNC:
		u.u_error = fset(fp, FASYNC, *(int *)data);
		return;

	case FIOSETOWN:
		u.u_error = fsetown(fp, *(int *)data);
		return;

	case FIOGETOWN:
		u.u_error = fgetown(fp, (int *)data);
		return;
	}
	u.u_error = (*fp->f_ops->fo_ioctl)(fp, com, data);
	/*
	 * Copy any data to user, size was
	 * already set and checked above.
	 */
	if (u.u_error == 0 && (com&IOC_OUT) && size)
		u.u_error = copyout(data, uap->cmarg, (u_int)size);
}

int	unselect();
int	nselcoll;

/*
 * Select system call.
 */
select()
{
	register struct uap  {
		int	nd;
		fd_set	*in, *ou, *ex;
		struct	timeval *tv;
	} *uap = (struct uap *)u.u_ap;
	fd_set ibits[3], obits[3];
	struct timeval atv;
	int s, ncoll, ni;
	label_t lqsave;

	bzero(ibits, sizeof(ibits));
	bzero(obits, sizeof(obits));
	if (uap->nd > NOFILE)
		uap->nd = NOFILE;	/* forgiving, if slightly wrong */
	ni = howmany(uap->nd, NFDBITS);

#define	getbits(name, x) \
	if (uap->name) { \
		u.u_error = copyin((caddr_t)uap->name, (caddr_t)&ibits[x], \
		    ni * sizeof(fd_mask)); \
		if (u.u_error) \
			goto done; \
	}
	getbits(in, 0);
	getbits(ou, 1);
	getbits(ex, 2);
#undef	getbits

	if (uap->tv) {
		u.u_error = copyin((caddr_t)uap->tv, (caddr_t)&atv,
			sizeof (atv));
		if (u.u_error)
			goto done;
		if (itimerfix(&atv)) {
			u.u_error = EINVAL;
			goto done;
		}
		s = splhigh(); timevaladd(&atv, &time); splx(s);
	}
retry:
	ncoll = nselcoll;
	u.u_procp->p_flag |= SSEL;
	u.u_r.r_val1 = selscan(ibits, obits, uap->nd);
	if (u.u_error || u.u_r.r_val1)
		goto done;
	s = splhigh();
	/* this should be timercmp(&time, &atv, >=) */
	if (uap->tv && (time.tv_sec > atv.tv_sec ||
	    time.tv_sec == atv.tv_sec && time.tv_usec >= atv.tv_usec)) {
		splx(s);
		goto done;
	}
	if ((u.u_procp->p_flag & SSEL) == 0 || nselcoll != ncoll) {
		u.u_procp->p_flag &= ~SSEL;
		splx(s);
		goto retry;
	}
	u.u_procp->p_flag &= ~SSEL;
	if (uap->tv) {
		lqsave = u.u_qsave;
		if (setjmp(&u.u_qsave)) {
			untimeout(unselect, (caddr_t)u.u_procp);
			u.u_error = EINTR;
			splx(s);
			goto done;
		}
		timeout(unselect, (caddr_t)u.u_procp, hzto(&atv));
	}
	sleep((caddr_t)&selwait, PZERO+1);
	if (uap->tv) {
		u.u_qsave = lqsave;
		untimeout(unselect, (caddr_t)u.u_procp);
	}
	splx(s);
	goto retry;
done:
#define	putbits(name, x) \
	if (uap->name) { \
		int error = copyout((caddr_t)&obits[x], (caddr_t)uap->name, \
		    ni * sizeof(fd_mask)); \
		if (error) \
			u.u_error = error; \
	}
	if (u.u_error != EINTR) {
		putbits(in, 0);
		putbits(ou, 1);
		putbits(ex, 2);
#undef putbits
	}
}

unselect(p)
	register struct proc *p;
{
	register int s = splhigh();

	switch (p->p_stat) {

	case SSLEEP:
		setrun(p);
		break;

	case SSTOP:
		unsleep(p);
		break;
	}
	splx(s);
}

selscan(ibits, obits, nfd)
	fd_set *ibits, *obits;
{
	register int which, i, j;
	register fd_mask bits;
	int flag;
	struct file *fp;
	int n = 0;

	for (which = 0; which < 3; which++) {
		switch (which) {

		case 0:
			flag = FREAD; break;

		case 1:
			flag = FWRITE; break;

		case 2:
			flag = 0; break;
		}
		for (i = 0; i < nfd; i += NFDBITS) {
			bits = ibits[which].fds_bits[i/NFDBITS];
			while ((j = ffs(bits)) && i + --j < nfd) {
				bits &= ~(1 << j);
				fp = u.u_ofile[i + j];
				if (fp == NULL) {
					u.u_error = EBADF;
					break;
				}
				if ((*fp->f_ops->fo_select)(fp, flag)) {
					FD_SET(i + j, &obits[which]);
					n++;
				}
			}
		}
	}
	return (n);
}

/*ARGSUSED*/
seltrue(dev, flag)
	dev_t dev;
	int flag;
{

	return (1);
}

selwakeup(p, coll)
	register struct proc *p;
	int coll;
{

	if (coll) {
		nselcoll++;
		wakeup((caddr_t)&selwait);
	}
	if (p) {
		int s = splhigh();
		if (p->p_wchan == (caddr_t)&selwait) {
			if (p->p_stat == SSLEEP)
				setrun(p);
			else
				unsleep(p);
		} else if (p->p_flag & SSEL)
			p->p_flag &= ~SSEL;
		splx(s);
	}
}
