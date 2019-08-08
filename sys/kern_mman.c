/*
 * Copyright (c) 1982, 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)kern_mman.c	7.1 (Berkeley) 6/5/86
 */

#include "../machine/reg.h"
#include "../machine/psl.h"
#include "../machine/pte.h"

#include "param.h"
#include "systm.h"
#include "map.h"
#include "dir.h"
#include "user.h"
#include "proc.h"
#include "buf.h"
#include "inode.h"
#include "seg.h"
#include "acct.h"
#include "wait.h"
#include "vm.h"
#include "text.h"
#include "file.h"
#include "vadvise.h"
#include "cmap.h"
#include "trace.h"
#include "mman.h"
#include "conf.h"

sbrk()
{

}

sstk()
{

}

getpagesize()
{

	u.u_r.r_val1 = NBPG * CLSIZE;
}

smmap()
{
#ifdef notdef
	struct a {
		caddr_t	addr;
		int	len;
		int	prot;
		int	share;
		int	fd;
		off_t	pos;
	} *uap = (struct a *)u.u_ap;
	register struct file *fp;
	register struct inode *ip;
	register struct fpte *pte;
	int off;
	int fv, lv, pm;
	dev_t dev;
	int (*mapfun)();
	extern struct file *getinode();

	fp = getinode(uap->fd);
	if (fp == NULL)
		return;
	ip = (struct inode *)fp->f_data;
	if ((ip->i_mode & IFMT) != IFCHR) {
		u.u_error = EINVAL;
		return;
	}
	dev = ip->i_rdev;
	mapfun = cdevsw[major(dev)].d_mmap;
	if (mapfun == NULL) {
		u.u_error = EINVAL;
		return;
	}
	if (((int)uap->addr & CLOFSET) || (uap->len & CLOFSET) ||
	    (uap->pos & CLOFSET)) {
		u.u_error = EINVAL;
		return;
	}
	if ((uap->prot & PROT_WRITE) && (fp->f_flag&FWRITE) == 0) {
		u.u_error = EINVAL;
		return;
	}
	if ((uap->prot & PROT_READ) && (fp->f_flag&FREAD) == 0) {
		u.u_error = EINVAL;
		return;
	}
	if (uap->share != MAP_SHARED) {
		u.u_error = EINVAL;
		return;
	}
	fv = btop(uap->addr);
	lv = btop(uap->addr + uap->len - 1);
	if (lv < fv || !isadsv(u.u_procp, fv) || !isadsv(u.u_procp, lv)) {
		u.u_error = EINVAL;
		return;
	}
	for (off=0; off<uap->len; off += NBPG) {
		if ((*mapfun)(dev, uap->pos+off, uap->prot) == -1) {
			u.u_error = EINVAL;
			return;
		}
	}
	if (uap->prot & PROT_WRITE)
		pm = PG_UW;
	else
		pm = PG_URKR;
	for (off = 0; off < uap->len; off += NBPG) {
		pte = (struct fpte *)vtopte(u.u_procp, fv);
		u.u_procp->p_rssize -= vmemfree(pte, 1);
		*(int *)pte = pm;
		pte->pg_v = 1;
		pte->pg_fod = 1;
		pte->pg_fileno = uap->fd;
		pte->pg_blkno = (*mapfun)(dev, uap->pos+off, uap->prot);
		fv++;
	}
	u.u_procp->p_flag |= SPTECHG;
	u.u_pofile[uap->fd] |= UF_MAPPED;
#endif
}

mremap()
{

}

munmap()
{
#ifdef notdef
	register struct a {
		caddr_t	addr;
		int	len;
	} *uap = (struct a *)u.u_ap;
	int off;
	int fv, lv;
	register struct pte *pte;

	if (((int)uap->addr & CLOFSET) || (uap->len & CLOFSET)) {
		u.u_error = EINVAL;
		return;
	}
	fv = btop(uap->addr);
	lv = btop(uap->addr + uap->len - 1);
	if (lv < fv || !isadsv(u.u_procp, fv) || !isadsv(u.u_procp, lv)) {
		u.u_error = EINVAL;
		return;
	}
	for (off = 0; off < uap->len; off += NBPG) {
		pte = vtopte(u.u_procp, fv);
		u.u_procp->p_rssize -= vmemfree(pte, 1);
		*(int *)pte = (PG_UW|PG_FOD);
		((struct fpte *)pte)->pg_fileno = PG_FZERO;
		fv++;
	}
	u.u_procp->p_flag |= SPTECHG;
#endif
}

munmapfd(fd)
{
#ifdef notdef
	register struct fpte *pte;
	register int i;

	for (i = 0; i < u.u_dsize; i++) {
		pte = (struct fpte *)dptopte(u.u_procp, i);
		if (pte->pg_v && pte->pg_fod && pte->pg_fileno == fd) {
			*(int *)pte = (PG_UW|PG_FOD);
			pte->pg_fileno = PG_FZERO;
		}
	}
#endif
	u.u_pofile[fd] &= ~UF_MAPPED;
	
}

mprotect()
{

}

madvise()
{

}

mincore()
{

}

/* BEGIN DEFUNCT */
obreak()
{
	struct a {
		char	*nsiz;
	};
	register size_t n, d, ds;

	/*
	 * set n to new data size
	 */
	n = btoc(((struct a *)u.u_ap)->nsiz) - ctos(u.u_tsize) * stoc(1);
	if (n < 0)
		n = 0;
	/*
	 * since we can't pass a -ve argument for the difference to chksize,
	 * if d is negative, make ds equal to the final value and clear d.
	 * keep the real difference in n for later use in expand.
	 */
	ds = u.u_dsize;
	if ((n = d = clrnd(n - u.u_dsize)) < 0) {
		ds += d;
		d = 0;
	}
	if (ctob(ds + d) > u.u_rlimit[RLIMIT_DATA].rlim_cur) {
		u.u_error = ENOMEM;
		return;
	}
	if (chksize((u_int)u.u_tsize, (u_int)ds, (u_int)d, (u_int)u.u_ssize))
		return;
	if (swpexpand(ds + d, u.u_ssize, &u.u_dmap, &u.u_smap) == 0)
		return;
	expand((int)n, 0);
}

int	both;

ovadvise()
{
	register struct a {
		int	anom;
	} *uap;
	register struct proc *rp = u.u_procp;
	int oanom = rp->p_flag & SUANOM;
	register struct pte *pte;
	register struct cmap *c;
	register unsigned i;

#ifdef lint
	both = 0;
#endif
	uap = (struct a *)u.u_ap;
	trace(TR_VADVISE, uap->anom, u.u_procp->p_pid);
	rp->p_flag &= ~(SSEQL|SUANOM);
	switch (uap->anom) {

	case VA_ANOM:
		rp->p_flag |= SUANOM;
		break;

	case VA_SEQL:
		rp->p_flag |= SSEQL;
		break;
	}
	if ((oanom && (rp->p_flag & SUANOM) == 0) || uap->anom == VA_FLUSH) {
		for (i = 0; i < rp->p_dsize; i += CLSIZE) {
			pte = dptopte(rp, i);
			if (pte->pg_v) {
				c = &cmap[pgtocm(pte->pg_pfnum)];
				if (c->c_lock)
					continue;
				pte->pg_v = 0;
				if (anycl(pte, pg_m))
					pte->pg_m = 1;
				distcl(pte);
			}
		}
	}
	if (uap->anom == VA_FLUSH) {	/* invalidate all pages */
		for (i = 1; i < rp->p_ssize; i += CLSIZE) {
			pte = sptopte(rp, i);
			if (pte->pg_v) {
				c = &cmap[pgtocm(pte->pg_pfnum)];
				if (c->c_lock)
					continue;
				pte->pg_v = 0;
				if (anycl(pte, pg_m))
					pte->pg_m = 1;
				distcl(pte);
			}
		}
		for (i = 0; i < rp->p_tsize; i += CLSIZE) {
			pte = tptopte(rp, i);
			if (pte->pg_v) {
				c = &cmap[pgtocm(pte->pg_pfnum)];
				if (c->c_lock)
					continue;
				pte->pg_v = 0;
				if (anycl(pte, pg_m))
					pte->pg_m = 1;
				distcl(pte);
				distpte(rp->p_textp, i, pte);
			}
		}
	}
#ifdef vax
#include "../vax/mtpr.h"		/* XXX */
	mtpr(TBIA, 0);
#endif
}
/* END DEFUNCT */

/*
 * grow the stack to include the SP
 * true return if successful.
 */
grow(sp)
	unsigned sp;
{
	register int si;

	if (sp >= USRSTACK-ctob(u.u_ssize))
		return (0);
	si = clrnd(btoc((USRSTACK-sp)) - u.u_ssize + SINCR);
	if (ctob(si) > u.u_rlimit[RLIMIT_STACK].rlim_cur)
		return (0);
	if (chksize((u_int)u.u_tsize, (u_int)u.u_dsize, (u_int)0,
	    (u_int)u.u_ssize+si))
		return (0);
	if (swpexpand(u.u_dsize, u.u_ssize+si, &u.u_dmap, &u.u_smap)==0)
		return (0);
	
	expand(si, 1);
	return (1);
}
