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
 *	@(#)vm_text.c	6.6 (Berkeley) 6/8/85
 */
#if	CMU
 
/*
 **********************************************************************
 * HISTORY
 * 17-Feb-86  Bill Bolosky (bolosky) at Carnegie-Mellon University
 *	Added IBM code for Sailboat under switch ROMP.  This looks like
 *	a silly feature which deals with shared text, but it should all
 *	go with the new VM anyway, so no big deal.
 *
 * 26-Jan-86  Avadis Tevanian (avie) at Carnegie-Mellon University
 *	Upgraded to 4.3.
 *
 * 15-May-85  Glenn Marcy (gm0w) at Carnegie-Mellon University
 *	Upgraded to 4.2BSD.  Carried over changes below [V1(1)].
 *
 * 19-Feb-82  Mike Accetta (mja) at Carnegie-Mellon University
 *	CS_IINCR:  Changed inode reference count modifications to use
 *	incr/decr macros to check for consistency (V3.04c).
 *
 **********************************************************************
 */
 
#include "cs_ichk.h"
#include "mach_vm.h"
#endif	CMU

#if	MACH_VM
#else	MACH_VM

#include "../machine/pte.h"

#ifdef	romp
#include "../machine/rosetta.h"
#endif	romp

#include "param.h"
#include "systm.h"
#include "map.h"
#include "dir.h"
#include "user.h"
#include "proc.h"
#include "text.h"
#include "inode.h"
#include "buf.h"
#include "seg.h"
#include "vm.h"
#include "cmap.h"
#include "uio.h"
#include "exec.h"

/*
 * relinquish use of the shared text segment
 * of a process.
 */
xfree()
{
	register struct text *xp;
	register struct inode *ip;

	if((xp=u.u_procp->p_textp) == NULL)
		return;
	xlock(xp);
	ip = xp->x_iptr;
	if(--xp->x_count==0 && (ip->i_mode&ISVTX)==0) {
#ifndef	romp
		xunlink(u.u_procp);
#else	romp
		xunlink(u.u_procp, 0);
#endif	romp
		xp->x_rssize -= vmemfree(tptopte(u.u_procp, 0), u.u_tsize);
		if (xp->x_rssize != 0)
			panic("xfree rssize");
		ip->i_flag &= ~ITEXT;
		irele(ip);
		while (xp->x_poip)
			sleep((caddr_t)&xp->x_poip, PSWP+1);
		vsxfree(xp, (long)xp->x_size);
		xp->x_flag &= ~XLOCK;
		xp->x_iptr = NULL;
	} else {
		xp->x_flag &= ~XLOCK;
		xccdec(xp, u.u_procp);
	}
	u.u_procp->p_textp = NULL;
}

/*
 * Attach to a shared text segment.
 * If there is no shared text, just return.
 * If there is, hook up to it:
 * if it is not currently being used, it has to be read
 * in from the inode (ip); the written bit is set to force it
 * to be written out as appropriate.
 * If it is being used, but is not currently in core,
 * a swap has to be done to get it back.
 */
xalloc(ip, ep, pagi)
	struct exec *ep;
	register struct inode *ip;
{
	register struct text *xp;
	register size_t ts;
	register struct text *xp1;

	if (ep->a_text == 0)
		return;
again:
	xp1 = NULL;
	for (xp = text; xp < textNTEXT; xp++) {
		if(xp->x_iptr == NULL) {
			if(xp1 == NULL)
				xp1 = xp;
			continue;
		}
		if ((xp->x_count > 0 || (xp->x_iptr->i_mode&ISVTX)) &&
		    xp->x_iptr == ip) {
			if (xp->x_flag&XLOCK) {
				xwait(xp);
				goto again;
			}
			xlock(xp);
			xp->x_count++;
			u.u_procp->p_textp = xp;
			xlink(u.u_procp);
			xunlock(xp);
			return;
		}
	}
	if((xp=xp1) == NULL) {
		tablefull("text");
		psignal(u.u_procp, SIGKILL);
		return;
	}
	xp->x_flag = XLOAD|XLOCK;
	if (pagi)
		xp->x_flag |= XPAGI;
	ts = clrnd(btoc(ep->a_text));
	xp->x_size = ts;
	if (vsxalloc(xp) == NULL) {
		swkill(u.u_procp, "xalloc: no swap space");
		return;
	}
	xp->x_count = 1;
	xp->x_ccount = 0;
	xp->x_rssize = 0;
	xp->x_iptr = ip;
	ip->i_flag |= ITEXT;
#if	CS_ICHK
	iincr_chk(ip);
#else	CS_ICHK
	ip->i_count++;
#endif	CS_ICHK
	u.u_procp->p_textp = xp;
	xlink(u.u_procp);
	if (pagi == 0) {
		settprot(RW);
		u.u_procp->p_flag |= SKEEP;
		(void) rdwri(UIO_READ, ip,
			(caddr_t)ctob(tptov(u.u_procp, 0)),
			(int)ep->a_text, sizeof (struct exec),
			2, (int *)0);
		u.u_procp->p_flag &= ~SKEEP;
	}
	settprot(RO);
	xp->x_flag |= XWRIT;
	xp->x_flag &= ~XLOAD;
	xunlock(xp);
}

/*
 * Lock and unlock a text segment from swapping
 */
xlock(xp)
register struct text *xp;
{

	while(xp->x_flag&XLOCK) {
		xp->x_flag |= XWANT;
		sleep((caddr_t)xp, PSWP);
	}
	xp->x_flag |= XLOCK;
}

/*
 * Wait for xp to be unlocked if it is currently locked.
 */
xwait(xp)
register struct text *xp;
{

	xlock(xp);
	xunlock(xp);
}

xunlock(xp)
register struct text *xp;
{

	if (xp->x_flag&XWANT)
		wakeup((caddr_t)xp);
	xp->x_flag &= ~(XLOCK|XWANT);
}

/*
 * Decrement the in-core usage count of a shared text segment.
 * When it drops to zero, free the core space.
 */
xccdec(xp, p)
register struct text *xp;
register struct proc *p;
{

	if (xp==NULL || xp->x_ccount==0)
		return;
	xlock(xp);
	if (--xp->x_ccount == 0) {
		if (xp->x_flag & XWRIT) {
			vsswap(p, tptopte(p, 0), CTEXT, 0, xp->x_size, (struct dmap *)0);
			if (xp->x_flag & XPAGI)
				swap(p, xp->x_ptdaddr, (caddr_t)tptopte(p, 0),
				    xp->x_size * sizeof (struct pte),
				    B_WRITE, B_PAGET, swapdev, 0);
			xp->x_flag &= ~XWRIT;
		} else
			xp->x_rssize -= vmemfree(tptopte(p, 0), xp->x_size);
		if (xp->x_rssize != 0)
			panic("text rssize");
	}
#ifndef	romp
	xunlink(p);
#else	romp
	xunlink(p, 0);
#endif	romp
	xunlock(xp);
}

/*
 * free the swap image of all unused saved-text text segments
 * which are from device dev (used by umount system call).
 */
xumount(dev)
register dev;
{
	register struct text *xp;

	for (xp = text; xp < textNTEXT; xp++) 
		if (xp->x_iptr!=NULL && dev==xp->x_iptr->i_dev)
			xuntext(xp);
}

/*
 * remove a shared text segment from the text table, if possible.
 */
xrele(ip)
register struct inode *ip;
{
	register struct text *xp;

	if ((ip->i_flag&ITEXT)==0)
		return;
	for (xp = text; xp < textNTEXT; xp++)
		if (ip==xp->x_iptr)
			xuntext(xp);
}

/*
 * remove text image from the text table.
 * the use count must be zero.
 */
xuntext(xp)
register struct text *xp;
{
	register struct inode *ip;

	xlock(xp);
	if (xp->x_count) {
		xunlock(xp);
		return;
	}
	ip = xp->x_iptr;
	xp->x_flag &= ~XLOCK;
	xp->x_iptr = NULL;
	vsxfree(xp, (long)xp->x_size);
	ip->i_flag &= ~ITEXT;
	irele(ip);
}

/*
 * Add a process to those sharing a text segment by
 * getting the page tables and then linking to x_caddr.
 */
xlink(p)
	register struct proc *p;
{
	register struct text *xp = p->p_textp;

	if (xp == 0)
		return;
	vinitpt(p);
	p->p_xlink = xp->x_caddr;
	xp->x_caddr = p;
	xp->x_ccount++;
#ifdef	romp

       /* switch to new address space for shared text */

       p->p_sid0 = make410sid( xp );
       if( p == u.u_procp )
               set_segreg( 0, p->p_sid0 );
#endif	romp
}

#ifndef	romp
xunlink(p)
#else	romp
xunlink(p, isxrepl)
#endif	romp
	register struct proc *p;
{
	register struct text *xp = p->p_textp;
	register struct proc *q;

	if (xp == 0)
		return;
#ifdef romp

       /* switch from address space for shared text */

       if (isxrepl==0) {
               p->p_sid0 = makeP0sid(p);
               if (p == u.u_procp)
                       set_segreg(0, p->p_sid0);
	}

#endif romp
	if (xp->x_caddr == p) {
		xp->x_caddr = p->p_xlink;
		p->p_xlink = 0;
		return;
	}
	for (q = xp->x_caddr; q->p_xlink; q = q->p_xlink)
		if (q->p_xlink == p) {
			q->p_xlink = p->p_xlink;
			p->p_xlink = 0;
			return;
		}
	panic("lost text");
}

/*
 * Replace p by q in a text incore linked list.
 * Used by vfork(), internally.
 */
xrepl(p, q)
	struct proc *p, *q;
{
	register struct text *xp = q->p_textp;

	if (xp == 0)
		return;
#ifndef	romp
	xunlink(p);
#else	romp
	xunlink(p,1);
#endif	romp
	q->p_xlink = xp->x_caddr;
	xp->x_caddr = q;
}
#endif	MACH_VM
