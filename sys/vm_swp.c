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
 *	@(#)vm_swp.c	6.7 (Berkeley) 8/28/85
 */
#if	CMU
/*
 **********************************************************************
 * HISTORY
 * 26-Jan-86  Avadis Tevanian (avie) at Carnegie-Mellon University
 *	Upgraded to 4.3.
 *
 * 18-Dec-85  Mike Accetta (mja) at Carnegie-Mellon University
 *	Upgraded from 4.1BSD.  Carried over change below:
 *
 * 17-Jul-84  Fil Alleva (faa) at Carnegie-Mellon University
 *	NDS:  Added code in physio() to preserve SPHYSIO bit in
 *	users process flags so that the DSC device will work correctly
 *	with other devices.
 *
 **********************************************************************
 */
 
#include "ds.h"
#include "mach_vm.h"
#endif	CMU

#include "../machine/pte.h"

#include "param.h"
#include "systm.h"
#include "dir.h"
#include "user.h"
#include "buf.h"
#include "conf.h"
#include "proc.h"
#include "seg.h"
#include "vm.h"
#include "trace.h"
#include "map.h"
#include "uio.h"

#if	MACH_VM
#else	MACH_VM

/*
 * Swap IO headers -
 * They contain the necessary information for the swap I/O.
 * At any given time, a swap header can be in three
 * different lists. When free it is in the free list, 
 * when allocated and the I/O queued, it is on the swap 
 * device list, and finally, if the operation was a dirty 
 * page push, when the I/O completes, it is inserted 
 * in a list of cleaned pages to be processed by the pageout daemon.
 */
struct	buf *swbuf;

/*
 * swap I/O -
 *
 * If the flag indicates a dirty page push initiated
 * by the pageout daemon, we map the page into the i th
 * virtual page of process 2 (the daemon itself) where i is
 * the index of the swap header that has been allocated.
 * We simply initialize the header and queue the I/O but
 * do not wait for completion. When the I/O completes,
 * iodone() will link the header to a list of cleaned
 * pages to be processed by the pageout daemon.
 */
swap(p, dblkno, addr, nbytes, rdflg, flag, dev, pfcent)
	struct proc *p;
	swblk_t dblkno;
	caddr_t addr;
	int nbytes, rdflg, flag;
	dev_t dev;
	u_int pfcent;
{
	register struct buf *bp;
	register u_int c;
	int p2dp;
	register struct pte *dpte, *vpte;
	int s;
	extern swdone();

	s = spl6();
	while (bswlist.av_forw == NULL) {
		bswlist.b_flags |= B_WANTED;
		sleep((caddr_t)&bswlist, PSWP+1);
	}
	bp = bswlist.av_forw;
	bswlist.av_forw = bp->av_forw;
	splx(s);

	bp->b_flags = B_BUSY | B_PHYS | rdflg | flag;
	if ((bp->b_flags & (B_DIRTY|B_PGIN)) == 0)
		if (rdflg == B_READ)
			sum.v_pswpin += btoc(nbytes);
		else
			sum.v_pswpout += btoc(nbytes);
	bp->b_proc = p;
	if (flag & B_DIRTY) {
		p2dp = ((bp - swbuf) * CLSIZE) * KLMAX;
		dpte = dptopte(&proc[2], p2dp);
		vpte = vtopte(p, btop(addr));
		for (c = 0; c < nbytes; c += NBPG) {
			if (vpte->pg_pfnum == 0 || vpte->pg_fod)
				panic("swap bad pte");
			*dpte++ = *vpte++;
		}
		bp->b_un.b_addr = (caddr_t)ctob(dptov(&proc[2], p2dp));
		bp->b_flags |= B_CALL;
		bp->b_iodone = swdone;
		bp->b_pfcent = pfcent;
	} else
		bp->b_un.b_addr = addr;
	while (nbytes > 0) {
		bp->b_bcount = nbytes;
		minphys(bp);
		c = bp->b_bcount;
		bp->b_blkno = dblkno;
		bp->b_dev = dev;
#ifdef TRACE
		trace(TR_SWAPIO, dev, bp->b_blkno);
#endif
		physstrat(bp, bdevsw[major(dev)].d_strategy, PSWP);
		if (flag & B_DIRTY) {
			if (c < nbytes)
				panic("big push");
			return;
		}
		bp->b_un.b_addr += c;
		bp->b_flags &= ~B_DONE;
		if (bp->b_flags & B_ERROR) {
			if ((flag & (B_UAREA|B_PAGET)) || rdflg == B_WRITE)
				panic("hard IO err in swap");
			swkill(p, "swap: read error from swap device");
		}
		nbytes -= c;
		dblkno += btodb(c);
	}
	s = spl6();
	bp->b_flags &= ~(B_BUSY|B_WANTED|B_PHYS|B_PAGET|B_UAREA|B_DIRTY);
	bp->av_forw = bswlist.av_forw;
	bswlist.av_forw = bp;
	if (bswlist.b_flags & B_WANTED) {
		bswlist.b_flags &= ~B_WANTED;
		wakeup((caddr_t)&bswlist);
		wakeup((caddr_t)&proc[2]);
	}
	splx(s);
}

/*
 * Put a buffer on the clean list after I/O is done.
 * Called from biodone.
 */
swdone(bp)
	register struct buf *bp;
{
	register int s;

	if (bp->b_flags & B_ERROR)
		panic("IO err in push");
	s = spl6();
	bp->av_forw = bclnlist;
	cnt.v_pgout++;
	cnt.v_pgpgout += bp->b_bcount / NBPG;
	bclnlist = bp;
	if (bswlist.b_flags & B_WANTED)
		wakeup((caddr_t)&proc[2]);
	splx(s);
}

/*
 * If rout == 0 then killed on swap error, else
 * rout is the name of the routine where we ran out of
 * swap space.
 */
swkill(p, rout)
	struct proc *p;
	char *rout;
{

	printf("pid %d: %s\n", p->p_pid, rout);
	uprintf("sorry, pid %d was killed in %s\n", p->p_pid, rout);
	/*
	 * To be sure no looping (e.g. in vmsched trying to
	 * swap out) mark process locked in core (as though
	 * done by user) after killing it so noone will try
	 * to swap it out.
	 */
	psignal(p, SIGKILL);
	p->p_flag |= SULOCK;
}

#endif	MACH_VM
/*
 * Raw I/O. The arguments are
 *	The strategy routine for the device
 *	A buffer, which will always be a special buffer
 *	  header owned exclusively by the device for this purpose
 *	The device number
 *	Read/write flag
 * Essentially all the work is computing physical addresses and
 * validating them.
 * If the user has the proper access privilidges, the process is
 * marked 'delayed unlock' and the pages involved in the I/O are
 * faulted and locked. After the completion of the I/O, the above pages
 * are unlocked.
 */
physio(strat, bp, dev, rw, mincnt, uio)
	int (*strat)(); 
	register struct buf *bp;
	dev_t dev;
	int rw;
	unsigned (*mincnt)();
	struct uio *uio;
{
	register struct iovec *iov;
	register int c;
	char *a;
	int s, error = 0;

nextiov:
	if (uio->uio_iovcnt == 0)
		return (0);
	iov = uio->uio_iov;
	if (useracc(iov->iov_base,(u_int)iov->iov_len,rw==B_READ?B_WRITE:B_READ) == NULL)
		return (EFAULT);
	s = spl6();
	while (bp->b_flags&B_BUSY) {
		bp->b_flags |= B_WANTED;
		sleep((caddr_t)bp, PRIBIO+1);
	}
	splx(s);
	bp->b_error = 0;
	bp->b_proc = u.u_procp;
	bp->b_un.b_addr = iov->iov_base;
	while (iov->iov_len > 0) {
#if	NDS > 0
		int sphysio = (u.u_procp->p_flag&SPHYSIO);

#endif	NDS > 0
		bp->b_flags = B_BUSY | B_PHYS | rw;
		bp->b_dev = dev;
		bp->b_blkno = btodb(uio->uio_offset);
		bp->b_bcount = iov->iov_len;
		(*mincnt)(bp);
		c = bp->b_bcount;
		u.u_procp->p_flag |= SPHYSIO;
		vslock(a = bp->b_un.b_addr, c);
		physstrat(bp, strat, PRIBIO);
		(void) spl6();
		vsunlock(a, c, rw);
#if	NDS > 0
		u.u_procp->p_flag &= (~SPHYSIO | sphysio);
#else	NDS > 0
		u.u_procp->p_flag &= ~SPHYSIO;
#endif	NDS > 0
		if (bp->b_flags&B_WANTED)
			wakeup((caddr_t)bp);
		splx(s);
		c -= bp->b_resid;
		bp->b_un.b_addr += c;
		iov->iov_len -= c;
		uio->uio_resid -= c;
		uio->uio_offset += c;
		/* temp kludge for tape drives */
		if (bp->b_resid || (bp->b_flags&B_ERROR))
			break;
	}
	bp->b_flags &= ~(B_BUSY|B_WANTED|B_PHYS);
	error = geterror(bp);
	/* temp kludge for tape drives */
	if (bp->b_resid || error)
		return (error);
	uio->uio_iov++;
	uio->uio_iovcnt--;
	goto nextiov;
}

#define	MAXPHYS	(63 * 1024)

unsigned
minphys(bp)
	struct buf *bp;
{

	if (bp->b_bcount > MAXPHYS)
		bp->b_bcount = MAXPHYS;
}
