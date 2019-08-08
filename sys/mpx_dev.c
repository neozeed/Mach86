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
/*	mpx_dev.c	4.6	81/07/06	*/

/*
 **********************************************************************
 * HISTORY
 * 29-Jan-86  Avadis Tevanian (avie) at Carnegie-Mellon University
 *	Several random changes to wedge this into 4.3.
 *
 * 16-May-85  Mike Accetta (mja) at Carnegie-Mellon University
 *	Upgraded to 4.2BSD (unfortunately).
 *	[V1(1)]
 *
 * 26-Jul-82  Mike Accetta (mja) at Carnegie-Mellon University
 *	Fixed up mxclose() parameters for CHAOSNET mods from
 *	Symbolics, Inc. (V3.05c).
 *
 * 01-Mar-82  Mike Accetta (mja) at Carnegie-Mellon University
 *	Fixed bug in mxclose() which executed with inode reference count
 *	of 1 permitting possible deallocation while still in use (the
 *	result being that the inode could potentially be freed more than
 *	once and would trash some future file allocated in that slot
 *	since the reference count would be off by at least one thereafter);
 *	fixed another bug in chfree() which failed to deallocate terminal
 * 	channels (fix courtesy of James Gosling) (V3.04e).
 *
 **********************************************************************
 */


#include "../h/param.h"
#include "../h/systm.h"
#include "../h/dir.h"
#include "../h/user.h"
#include "../h/proc.h"
#include "../h/ioctl.h"
#include "../h/tty.h"
#include "../h/inode.h"
#include "../h/mx.h"
#include "../h/file.h"
#include "../h/conf.h"
#include "../h/uio.h"
/*
 * multiplexor driver
 */
struct	chan	chans[NCHANS];
struct	group	*groups[NMPXGRPS];
int	mpxline;

short	mpx_cmask[16]	={
	01,	02,	04,
	010,	020,	040,
	0100,	0200,	0400,
	01000,	02000,	04000,
	010000,	020000,	040000, 0100000
};

#define	UIOMOVE	uiomove
struct chan *xcp(),*addch(),*nextcp();

#define	HIQ	100
#define	LOQ	20
#define	FP	((struct file *)cp)

char mcdebugs[NDEBUGS];

struct group *
getmpx(dev)
dev_t dev;
{
	register d;

	d = minor(dev);
	if (d >= NMPXGRPS) {
		u.u_error = ENXIO;
		return(NULL);
	}
	return(groups[d]);
}


/*ARGSUSED*/
mxopen(dev, flag)
{
	register struct group *gp;
	register struct file *fp;
	register struct chan *cp;
	int	msg;

	if ((gp=getmpx(dev)) == NULL) {
		return(u.u_error);
	}
	if (!(gp->g_state&INUSE))
		return(ENXIO);
	fp = u.u_ofile[u.u_r.r_val1];
	if ((struct inode *)fp->f_data != gp->g_inode)
		return(ENXIO);
	if ((cp=addch(gp->g_inode,0)) == NULL)
		return(ENXIO);

	cp->c_flags = XGRP;
	cp->c_ottyp = cp->c_ttyp = (struct tty *)cp;
	cp->c_line = cp->c_oline = mpxline;

	fp->f_flag |= FMPY;
	fp->f_flag |= FREAD+FWRITE;
	fp->f_un.f_chan = cp;

	if (gp->g_inode == mpxip) {
		ilock(mpxip);
		mpxname(cp);
		msg = M_OPEN;
	} else
		msg = M_WATCH;

	scontrol(cp, msg+(cp->c_index<<8), u.u_uid);
	sleep((caddr_t)cp,TTIPRI);
	if (cp->c_flags&NMBUF)
		iunlock(mpxip);
	if (cp->c_flags & WCLOSE) {
		chdrain(cp);
		chfree(cp);
		return(ENXIO);
	}
	cp->c_fy = fp;
	cp->c_pgrp = u.u_procp->p_pgrp;
	return(0);
}


char	mxnmbuf[NMSIZE];
int	nmsize;

mpxname(cp)
register struct chan *cp;
{
	register char *np;
	register c;

	copyinstr(u.u_arg[0], mxnmbuf, NMSIZE, &nmsize);
	nmsize = np - mxnmbuf;

	cp->c_flags |= NMBUF;
}


#if	NCHAOSFIX > 0
mxclose(dev, flag, cp)
dev_t	dev;
register struct chan *cp;
#else	NCHAOSFIX
mxclose(dev, flag, fp)
dev_t	dev;
register struct file *fp;
#endif	NCHAOSFIX
{
#if	NCHAOSFIX > 0
register struct file *fp;
#else	NCHAOSFIX
register struct chan *cp = fp->f_un.f_chan;
#endif	NCHAOSFIX
register struct group *gp;
register struct inode *ip;
int	i, fmp;

	fmp = flag&FMP;
	if ((gp=getmpx(dev)) == NULL)
		return;

	ip = gp->g_inode;
	if (ip==NULL || (ip->i_mode&IFMT)!=IFMPC) {
		return;
	}

#ifdef	CMU
	/*
	 *  There is a nasty race condition here if all files for the
	 *  same mpx file are closed at the same time.  Some number of the
	 *  closes may execute through but block somewhere within this routine
	 *  after having deallocated their inode pointer in the file table in
	 *  closef().  Thus it's possible that when one or more close sequences
	 *  have blocked, the "final" close may execute and deallocate the
	 *  inode when it finds that the reference count is only one.  Then the
	 *  blocked closes will resume after the inode has been deallocated
 	 *  and naively assume that they may continue to access it or worse yet
	 *  deallocate it again, possibly causing the reference count to go
	 *  from 0 to -1 if the slot has yet to be reallocated or freeing
	 *  someone else's inode if the slot has already been reallocated.
 	 *
	 *  The unfortunate manifestation of this problem will be that
	 *  some slot will eventually be freed when it shouldn't
	 *  and a new inode will be substituted when the original
	 *  inode is still in use, perhaps for file I/O etc. which
	 *  will have (and has had) disastrous consequences.
 	 *
	 *  The attempted fix here is to immediately bump the reference count
	 *  of the inode upon entry to the routine.  This guarantees that
	 *  all the close code executes with the reference count >= 2 and that
	 *  the inode will not be deallocated so long as there are any other
	 *  pending closes (either prior to the iput() in closef() or blocked
	 *  within the routine but AFTER they have announced their presence by
	 *  incrementing the reference count)
	 *
	 *  When the routine is about to finish up and exit but discovers a
	 *  reference count of 2, it is guaranteed that there are no other
	 *  pending closes and may safely deallocate the group before
	 *  performing the final iput() which will deallocate the inode. 
	 */
	ilock(ip);
	iincr_chk(ip);
	iunlock(ip);
#endif	CMU

	/*
	 * close a channel
	 */
	if (cp!=NULL && fmp && fmp!=FMP) {
		for(fp=file; fp< fileNFILE; fp++)
		    if(fp->f_count && fp->f_flag&FMP && fp->f_un.f_chan==cp){
#ifdef	CMU
				goto put;
#else	CMU
				return;
#endif	CMU
			}
		chdrain(cp);
		if ((cp->c_flags&WCLOSE)==0) {
			scontrol(cp, M_CLOSE, 0);
			cp->c_flags |= WCLOSE;
		} else {
			chfree(cp);
		}
		goto out;
	}


	for(fp=file; fp < fileNFILE; fp++) {
		if (fp->f_count && (fp->f_flag&FMP)==FMP && (struct inode *)fp->f_data==ip)
#ifdef	CMU
			goto put;
#else	CMU
			return;
#endif	CMU
	}

	if (ip == mpxip) {
		mpxip = NULL;
		iunlock(ip);
	}

	for(i=0;i<NINDEX;i++)
		(void) detach(gp->g_chans[i]);

out:
#ifdef	CMU
	ilock(ip);
	if (ip->i_count == 2) {
#else	CMU
	if (ip->i_count == 1) {
#endif	CMU
		groups[minor(dev)] = NULL;
#ifndef	CMU
		ilock(ip);
#endif	CMU
		zero((caddr_t)gp, sizeof (struct group));
		ip->i_mode = IFREG + 0666;
		ip->i_rdev = 0;
		ip->i_flag |= IUPD|ICHG;
		iput(ip);
	}
#ifdef	CMU
	else
	    iunlock(ip);

put:
	ilock(ip);
	iput(ip);
#endif	CMU
}

zero(s, cc)
register char *s;
register cc;
{
	while (cc--)
		*s++ = 0;
}

char	m_eot[] ={ M_EOT, 0, 0, 0};

/*
 * Mxread + mxwrite are entered from cdevsw
 * for all read/write calls.  Operations on
 * an mpx file are handled here.
 * Calls are made through linesw to handle actual
 * data movement.
 */
mxread(dev, uio)
	struct uio *uio;
{
	register struct group *gp;
	register struct chan *cp;
	register esc;
	struct rh h;
	caddr_t	base;
	unsigned count;
	int s, xfr, more, fmp;

	if ((gp=getmpx(dev))==NULL || (FP=getf(u.u_arg[0]))==NULL)
		return(ENXIO);

	fmp = FP->f_flag & FMP;
	if (fmp != FMP) {
		if (uio->uio_resid == 0)
			return(0);
		return(msread(fmp, FP->f_un.f_chan, uio));
	}

	if ((int)uio->uio_iov->iov_base & 1)
		return(ENXIO);
	if (uio->uio_iovcnt > 1)
		return(EINVAL);

	s = spl6();
	if (uio->uio_resid == 0)
	{
		int error = 0;

		if (gp->g_datq == 0)
			error = ENXIO;
		splx(s);
		return(error);
	}
	while (gp->g_datq == 0) {
		sleep((caddr_t)&gp->g_datq, TTIPRI);
	}
	splx(s);

	while (gp->g_datq && uio->uio_resid >= CNTLSIZ + 2) {
		esc = 0;
		cp = nextcp(gp);
		if (cp==NULL) {
			continue;
		}
		h.index = cpx(cp);
		if (count = cp->c_ctlx.c_cc) {
			count += CNTLSIZ;
			if (cp->c_flags&NMBUF)
				count += nmsize;
			if (count > uio->uio_resid) {
				(void) sdata(cp);
				return(0);
			}
			esc++;
		}
		base = uio->uio_iov->iov_base;
		count = uio->uio_resid;
		uio->uio_iov->iov_base += sizeof h;
		uio->uio_iov->iov_len -= sizeof h;
		uio->uio_resid -= sizeof h;
		xfr = uio->uio_resid;
		if (esc) {
			more = mcread(cp, uio);
		} else {
			more = (*linesw[cp->c_line].l_read)(cp->c_ttyp, uio);
		}
		if (more > 0)
			(void) sdata(cp);
		if (more < 0)
			scontrol(cp, M_CLOSE, 0);
		(void) spl0();
		if (xfr == uio->uio_resid) {
			esc++;
			(void) UIOMOVE((caddr_t)m_eot, sizeof m_eot, UIO_READ, uio);
		}
		xfr -= uio->uio_resid;
		if (esc) {
			h.count = 0;
			h.ccount = xfr;
		} else {
			h.count = xfr;
			h.ccount = 0;
			mxrstrt(cp, &cp->cx.datq, BLOCK|ALT);
		}
		if (uio->uio_resid && (xfr&1)) {
			uio->uio_iov->iov_base++;
			uio->uio_iov->iov_len--;
			uio->uio_resid--;
		}
		(void) copyout((caddr_t)&h, base, sizeof h);

	}
	return(0);
}


mxwrite(dev, uio)
	struct uio *uio;
{
register struct chan *cp;
struct	wh h;
struct group *gp;
int	ucount, esc, fmp, burpcount;
caddr_t	ubase, hbase;

	if ((gp=getmpx(dev))==NULL || (FP=getf(u.u_arg[0]))==NULL) {
		return(u.u_error);
	}
	if (uio->uio_iovcnt > 1)
		return(EINVAL);
	fmp = FP->f_flag & FMP;
	if (fmp != FMP)
		return(mswrite(fmp, FP->f_un.f_chan, uio));

	burpcount = 0;
	while (uio->uio_resid >= sizeof h) {
		int error;

		hbase = uio->uio_iov->iov_base;
		error = UIOMOVE((caddr_t)&h, sizeof h, UIO_WRITE, uio);
		if (error)
			return(error);
		esc = 0;
		if (h.count==0) {
			esc++;
			h.count = h.ccount;
		}
		cp = xcp(gp, h.index);
		if (cp==NULL || cp->c_flags&ISGRP)
			return(ENXIO);
		ucount = uio->uio_resid;
		ubase = uio->uio_iov->iov_base;
		uio->uio_resid = h.count;
		uio->uio_iov->iov_len = h.count;
		uio->uio_iov->iov_base = h.data;

		if (esc==0) {
			struct tty *tp;
			caddr_t waddr;
			int line;

			if (cp->c_flags&PORT) {
				line = cp->c_line;
				tp = cp->c_ttyp;
			} else {
				line = cp->c_oline;
				tp = cp->c_ottyp;
			}
		loop:
			waddr = (caddr_t)(*linesw[line].l_write)(tp, uio);
			if (uio->uio_resid) {
				if (gp->g_state&ENAMSG) {
					burpcount++;
					cp->c_flags |= BLKMSG;
/*
					scontrol(cp, M_BLK, u.u_count);
*/
					h.ccount = -1;
					h.count = uio->uio_iov->iov_len;
					h.data = uio->uio_iov->iov_base;
					(void) copyout((caddr_t)&h, hbase, sizeof h);
				} else {
					if(waddr == 0) {
						return(ENXIO);
					}
					sleep(waddr, TTOPRI);
					goto loop;
				}
			}
		} else {
			u.u_error = mxwcontrol(cp, uio); 
		}
		uio->uio_resid = ucount;
		uio->uio_iov->iov_len = ucount;
		uio->uio_iov->iov_base = ubase;
	}
	uio->uio_resid = burpcount;
	return(u.u_error);
}



/*
 * Mcread and mcwrite move data on an mpx file.
 * Transfer addr and length is controlled by mxread/mxwrite.
 * Kernel-to-Kernel and other special transfers are not
 * yet in.
 */
mcread(cp, uio)
register struct chan *cp;
struct uio *uio;
{
register struct clist *q;
register char *np;


	q = (cp->c_ctlx.c_cc) ? &cp->c_ctlx : &cp->cx.datq;
	(void) mxmove(q, UIO_READ, uio);

	if (cp->c_flags&NMBUF && q == &cp->c_ctlx) {
		np = mxnmbuf;
		while (nmsize--)
			(void) ureadc(*np++, uio);
		cp->c_flags &= ~NMBUF;
		iunlock(mpxip);
	}
	if (cp->c_flags&PORT)
		return(cp->c_ctlx.c_cc + cp->c_ttyp->t_rawq.c_cc); else
		return(cp->c_ctlx.c_cc + cp->cx.datq.c_cc);

}


mcwrite(cp, uio)
register struct chan *cp;
struct uio *uio;
{
register struct clist *q;
int	s;

	q = &cp->cy.datq;
	while (uio->uio_resid) {
		s = spl6();
		if (q->c_cc > HIQ || (cp->c_flags&EOTMARK)) {
			cp->c_flags |= SIGBLK;
			splx(s);
			break;
		}
		splx(s);
		(void) mxmove(q, UIO_WRITE, uio);
	}
	wakeup((caddr_t)q);
}


/*
 * Msread and mswrite move bytes
 * between user and non-multiplexed channel.
 */
msread(fmp, cp, uio)
register struct chan *cp;
struct uio *uio;
{
register struct clist *q;
int s;
int error = 0;

	q = (fmp&FMPX) ? &cp->cx.datq : &cp->cy.datq;
	s = spl6();
	while (q->c_cc == 0) {
		if (cp->c_flags&WCLOSE) {
			error = ENXIO;
			goto out;
		}
		if (cp->c_flags & EOTMARK) {
			cp->c_flags &= ~EOTMARK;
			if(msgenab(cp))
				scontrol(cp, M_UBLK, 0);
			else {
				wakeup((caddr_t)cp);
				wakeup((caddr_t)q);
			}
			goto out;
		}
		sleep((caddr_t)q,TTIPRI);
	}
	if (cp->c_flags&WCLOSE) {
		error = ENXIO;
		goto out;
	}
	splx(s);
	while (mxmove(q, UIO_READ, uio) > 0)
		;
	mxrstrt(cp, q, SIGBLK);
	return(error);
out:
	splx(s);
	return(error);
}


mswrite(fmp, cp, uio)
register struct chan *cp;
struct uio *uio;
{
	register struct clist *q;
	register int cc;

	q = (fmp&FMPX) ? &cp->cy.datq : &cp->cx.datq;
	while (uio->uio_resid) {
		(void) spl6();
		if (cp->c_flags&WCLOSE) {
			gsignal(cp->c_pgrp, SIGPIPE);
			(void) spl0();
			return(0);
		}
		if (q->c_cc>= HIQ || cp->c_flags&FBLOCK) {
			if (cp->c_flags&WCLOSE) {
				gsignal(cp->c_pgrp, SIGPIPE);
				(void) spl0();
				return(0);
			}
			(void) sdata(cp);
			cp->c_flags |= BLOCK;
			sleep((caddr_t)q+1,TTOPRI);
			(void) spl0();
			continue;
		}
		(void) spl0();
		cc = mxmove(q, UIO_WRITE, uio);
		if (cc < 0)
			break;
	}
	if (fmp&FMPX) {
		if (cp->c_flags&YGRP)  (void) sdata(cp);
		else		       wakeup((caddr_t)q);
	} else {
		if (cp->c_flags&XGRP)  (void) sdata(cp);
		else		       wakeup((caddr_t)q);
	}
	return(0);
}


/*
 * move chars between clist and user space.
 */

mxmove(q, dir, uio)
register struct clist *q;
register enum uio_rw dir;
struct uio *uio;
{
register cc;
char cbuf[HIQ];

	cc = MIN(uio->uio_resid, sizeof cbuf);
	if (dir == UIO_READ) 
		cc = q_to_b(q, cbuf, cc);
	if (cc <= 0)
		return(cc);
	(void) UIOMOVE((caddr_t)cbuf, cc, dir, uio);
	if (dir == UIO_WRITE)
		cc = b_to_q(cbuf, cc, q);
	return(cc);
}



mxrstrt(cp, q, b)
register struct chan *cp;
register struct clist *q;
register b;
{
int s;

	s = spl6();
	if (cp->c_flags&b && q->c_cc<LOQ) {
		cp->c_flags &= ~b;
		if (b&ALT)
			wakeup((caddr_t)q+1); else
			if (b&(BLKMSG))
				scontrol(cp, M_UBLK, 0); else
				wakeup((caddr_t)q);
	}
	if (cp->c_flags&WFLUSH)
		wakeup((caddr_t)q+2);
	splx(s);
}



/*
 * called from driver start or xint routines
 * to wakeup output sleeper.
 */
mcstart(cp, q)
register struct chan *cp;
register caddr_t q;
{

	if (cp->c_flags&(BLKMSG)) {
		cp->c_flags &= ~BLKMSG;
		scontrol(cp, M_UBLK, 0);
	} else
		wakeup((caddr_t)q);
}


mxwcontrol(cp, uio)
register struct chan *cp;
struct uio *uio;
{
short cmd;
struct sgttyb vec;
int	s;
int error;

	error = UIOMOVE((caddr_t)&cmd, sizeof cmd, UIO_WRITE, uio);
	if (error)
		return(error);
	switch(cmd) {
	/*
	 * not ready to queue this up yet.
	 */
	case M_EOT:
		s = spl6();
		while (cp->c_flags & EOTMARK)
			if(msgenab(cp)){
				scontrol(cp, M_BLK, 0);
				goto out;
			} else
				sleep((caddr_t)cp, TTOPRI);
				cp->c_flags |= EOTMARK;
	out:
		wakeup((caddr_t)&cp->cy.datq);
		splx(s);
		break;
	case M_IOCTL:
		break;
	case M_IOANS:
		if (cp->c_flags&SIOCTL) {
			(void) UIOMOVE((caddr_t)&vec, sizeof vec, UIO_WRITE, uio);
			(void) b_to_q((caddr_t)&vec, sizeof vec, &cp->c_ctly);
			cp->c_flags &= ~SIOCTL;
			wakeup((caddr_t)cp);
		}
		break;
	case M_BLK:
		cp->c_flags |= FBLOCK;
		break;
	case M_UBLK:
		cp->c_flags &= ~FBLOCK;
		chwake(cp);
		break;
	default:
		return(ENXIO);
	}
	return(0);
}



/*ARGSUSED*/
mxioctl(dev, cmd, data, flag)
caddr_t data;
{
struct group *gp;
int fmp;
struct file *fp;
struct {
	short c_ctl;
	short c_cmd;
	struct sgttyb c_vec;
} ctlbuf;
int error = 0;

	if ((gp=getmpx(dev))==NULL || (fp=getf(u.u_arg[0]))==NULL) {
		return(u.u_error);
	}

	fmp = fp->f_flag & FMP;
	if (fmp == FMP) {
		switch(cmd) {

		case MXLSTN:
			if (mpxip == NULL) {
				mpxip = gp->g_inode;
			} else {
				return(ENXIO);
			}
			break;

		case MXNBLK:
			gp->g_state |= ENAMSG;
			break;

		default:
			return(ENXIO);
		}
	} else {
		ctlbuf.c_ctl = M_IOCTL;
		ctlbuf.c_cmd = cmd;
		bcopy(data, (caddr_t)&ctlbuf.c_vec, sizeof (struct sgttyb));
		error = sioctl(fp->f_un.f_chan, (char *)&ctlbuf, sizeof ctlbuf);
		bcopy((caddr_t)&ctlbuf, data, sizeof (struct sgttyb));
	}
	return(error);
}


chdrain(cp)
register struct chan *cp;
{
register struct tty *tp;
int wflag;

	chwake(cp);

	wflag = (cp->c_flags&WCLOSE)==0;
	tp = cp->c_ttyp;
	if (tp == NULL)		/* prob not required */
		return;
	if (cp->c_flags&PORT && tp->t_chan == cp) {
		cp->c_ttyp = NULL;
		tp->t_chan = NULL;
		return;
	}
	if (wflag) 
		wflush(cp,&cp->cx.datq); else
		flush(&cp->cx.datq);
	if (!(cp->c_flags&YGRP)) {
		flush(&cp->cy.datq);
	}
}

chwake(cp)
register struct chan *cp;
{
register char *p;

	wakeup((caddr_t)cp);
	flush(&cp->c_ctlx);
	p = (char *)&cp->cx.datq;
	wakeup((caddr_t)p); wakeup((caddr_t)++p); wakeup((caddr_t)++p);
	p = (char *)&cp->cy.datq;
	wakeup((caddr_t)p); wakeup((caddr_t)++p); wakeup((caddr_t)++p);
}


chfree(cp)
register struct chan *cp;
{
register struct group *gp;
register i;
#ifdef	CMU
register struct tty *tp;

	tp = cp->c_ttyp;
	if (tp != NULL && cp == tp->t_chan)
	{
		tp->t_chan = NULL;
		cp->c_ttyp = NULL;
	}
#endif	CMU
	gp = cp->c_group;
	if (gp==NULL)
		return;
	i = cp->c_index;
	if (cp == gp->g_chans[i]) {
		gp->g_chans[i] = NULL;
	}
	cp->c_group = NULL;
	wakeup((caddr_t)gp);
}


flush(q)
register struct clist *q;
{

	while(q->c_cc)
		(void) getc(q);
}


wflush(cp,q)
register struct chan *cp;
register struct clist *q;
{
register s;

	s = spl6();
	if(q->c_cc && (cp->c_flags&WCLOSE) == 0) {
		cp->c_flags |= WFLUSH;
		(void) sdata(cp);
		(void) tsleep((caddr_t)q+2, TTOPRI, 30);
	}
	flush(q);
	cp->c_flags &= ~WFLUSH;
	splx(s);
}


scontrol(cp,event,value)
register struct chan *cp;
short event,value;
{
register struct clist *q;
int s;

	q = &cp->c_ctlx;
	s = spl6();
	if (sdata(cp) == NULL)
		return;
	(void) putw(event,q);
	(void) putw(value,q);
	splx(s);
}



sioctl(cp, vec, cc)
register struct chan *cp;
char *vec;
{
register s;
register struct clist *q;

	s = spl6();
	q = &cp->cx.datq;
	while (q->c_cc) {
		cp->c_flags |= BLOCK;
		if (sdata(cp)==NULL) {
			return(ENXIO);
		}
		sleep((caddr_t)q+1, TTOPRI);
	}
	(void) b_to_q(vec, cc, &cp->c_ctlx);
	cp->c_flags |= SIOCTL;
	while (cp->c_flags&SIOCTL) {
		if (cp->c_ctlx.c_cc)
			if (sdata(cp)==NULL) {
				return(ENXIO);
			}
		sleep((caddr_t)cp, TTOPRI);
	}
	(void) q_to_b(&cp->c_ctly, vec, cp->c_ctly.c_cc);
	splx(s);
	return(0);
}

sdata(cp)
struct chan *cp;
{
	register struct group *gp = (struct group *)cp;
	register struct group *ngp;
	register int	s;

	ngp = gp->g_group;
	if (ngp==NULL || (ngp->g_state&ISGRP)==0)
		return(NULL);

	s = spl6();
	do {
		ngp->g_datq |= mpx_cmask[gp->g_index];
		wakeup((caddr_t)&ngp->g_datq);
		gp = ngp;
	} while(ngp=ngp->g_group);
	splx(s);
	return((int)gp);
}



struct chan *
xcp(gp, x)
register struct group *gp;
register short x;
{
	register int i;

	while (gp->g_group) gp=gp->g_group;
	for (i=0;i<NLEVELS;i++) {
		if ((x&017) >= NINDEX)
			break;
		if (gp==NULL || (gp->g_state&ISGRP)==0)
			return((struct chan *)NULL);
		gp = (struct group *)gp->g_chans[x&017];
		x >>= 4;
	}
	return((struct chan *)gp);
}

cpx(cp)
register struct chan *cp;
{
	register x;
	register struct group *gp;

	x = (-1<<4) + cp->c_index;
	gp = cp->c_group;
	while (gp->g_group) {
		x <<= 4;
		x |= gp->g_index;
		gp = gp->g_group;
	}
	return(x);
}


struct chan *
nextcp(gp)
register struct group *gp;
{
	register struct group *lgp, *ngp;

	do {
		while ((gp->g_datq & mpx_cmask[gp->g_rot]) == 0) {
			gp->g_rot = (gp->g_rot+1)%NINDEX;
		}
		lgp = gp;
		gp = (struct group *)gp->g_chans[gp->g_rot];
	} while (gp!=NULL && gp->g_state&ISGRP);

	lgp->g_datq &= ~mpx_cmask[lgp->g_rot];
	lgp->g_rot  =  (lgp->g_rot+1)%NINDEX;

	while (ngp=lgp->g_group) {
		ngp->g_datq &= ~mpx_cmask[lgp->g_index];
		if (ngp->g_datq)
			break;
		lgp = ngp;
	}
	return((struct chan *)gp);
}



msgenab(cp)
register struct chan *cp;
{
	register struct group *gp;

	for(gp=cp->c_group;gp;gp=gp->g_group)
		if(gp->g_state & ENAMSG)return(1);
	return(0);
}
