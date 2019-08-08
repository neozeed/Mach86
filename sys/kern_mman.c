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
 *	@(#)kern_mman.c	6.5 (Berkeley) 9/5/85
 */
#if	CMU

/*
 ************************************************************************
 * HISTORY
 *  6-May-86  Avadis Tevanian (avie) at Carnegie-Mellon University
 *	Converted vm_allocate call to use vm_allocate_with_pager.
 *
 * 14-Apr-86  David Golub (dbg) at Carnegie-Mellon University
 *	Lock and disable interrupts around call to vm_map_copy.
 *
 * 25-Feb-86  Avadis Tevanian (avie) at Carnegie-Mellon University
 *	Installed VM changes.
 *
 ************************************************************************
 */
#include "mach_vm.h"
#endif	CMU

#include "../machine/reg.h"
#ifndef	romp
#include "../machine/psl.h"
#endif	romp
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
#if	MACH_VM
#include "../h/kern_return.h"
#include "../h/task.h"
#include "../vm/vm_param.h"
#include "../vm/vm_map.h"
#include "../vm/vm_pager.h"
#endif	MACH_VM

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
#if	MACH_VM
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
	int off;
	kern_return_t	ret;
	vm_pager_id_t	pager_id;
	vm_map_t	copy_map, my_map;
	vm_size_t	copy_size;
	extern struct file *getinode();
	int	s;

	uap->addr = (caddr_t) round_page(uap->addr);
	uap->pos = round_page(uap->pos);
	uap->len = round_page(uap->len);
	fp = getinode(uap->fd);
	if (fp == NULL) {
		u.u_error = KERN_INVALID_ARGUMENT;
		return;
	}
	if (fp->f_type != DTYPE_INODE) {
		u.u_error = KERN_INVALID_ARGUMENT;
		return;
	}
	ip = (struct inode *)fp->f_data;
	copy_size = round_page(ip->i_size);
	pager_id = inode_pager_setup(ip);
	copy_map = vm_map_create(pmap_create(copy_size), 0, copy_size, TRUE);
	off = 0;

	/*
	 *	Map the file into our new map.
	 */

	if (vm_allocate_with_pager(copy_map, &off, copy_size, FALSE,
	    vm_pager_inode, pager_id, (vm_offset_t) 0) != KERN_SUCCESS)
		panic("smmap: cannot map inode into user address space");
	copy_size -= uap->pos;
	if (uap->len < copy_size)
		copy_size = uap->len;
	if (copy_size < 0) {
		vm_map_deallocate(copy_map);
		u.u_error = KERN_INVALID_ARGUMENT;
		return;
	}

	/*
	 *	Copy the file into the user's map.
	 */
	my_map = current_task()->map;
	s = splvm();
	ret = vm_map_copy(my_map, copy_map, uap->addr, copy_size,
		uap->pos, FALSE, FALSE);
	splx(s);
	if (ret != KERN_SUCCESS) {
		vm_map_deallocate(copy_map);
		u.u_error = ret;
		return;
	}
	vm_map_deallocate(copy_map);
	u.u_error = KERN_SUCCESS;
	return;
#else	MACH_VM
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
#endif	MACH_VM
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
#if	MACH_VM
	panic("munmap: VM code confused");
#else	MACH_VM
	for (off = 0; off < uap->len; off += NBPG) {
		pte = vtopte(u.u_procp, fv);
		u.u_procp->p_rssize -= vmemfree(pte, 1);
		*(int *)pte = (PG_UW|PG_FOD);
		((struct fpte *)pte)->pg_fileno = PG_FZERO;
		fv++;
	}
#endif	MACH_VM
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
	register int n, d, ds;
#if	MACH_VM
	vm_offset_t	old, new;
	kern_return_t	ret;
#endif	MACH_VM

	/*
	 * set n to new data size
	 */
#ifdef	romp
	n = btoc(((struct a *)u.u_ap)->nsiz);
	if (u.u_tsize)
		n -= btoc(DATABASE);
#else	romp
	n = btoc(((struct a *)u.u_ap)->nsiz) - ctos(u.u_tsize) * stoc(1);
#endif	romp
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
#if	MACH_VM
#ifdef	romp
	old = round_page(DATABASE + ctob(u.u_dsize));
	new = round_page(DATABASE + ctob(n + u.u_dsize));
#else	romp 
	old = round_page(ctob(u.u_tsize + u.u_dsize));
	new = round_page(ctob(u.u_tsize + n + u.u_dsize));
#endif	romp
	if (new > old) {
		ret = vm_allocate(current_task()->map, &old, new - old, FALSE);
		if (ret != KERN_SUCCESS)
			uprintf("could not sbrk, return = %d\n", ret);
		else
			u.u_dsize += btoc(new - old);
	}
#else	MACH_VM
	if (chksize((u_int)u.u_tsize, (u_int)ds, (u_int)d, (u_int)u.u_ssize))
		return;
	if (swpexpand(ds + d, u.u_ssize, &u.u_dmap, &u.u_smap) == 0)
		return;
	expand(n, 0);
#endif	MACH_VM
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
	register int i;

#ifdef lint
	both = 0;
#endif
	uap = (struct a *)u.u_ap;
#if	MACH_VM
#else	MACH_VM
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
#endif	MACH_VM
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
#if	MACH_VM
#else	MACH_VM
	if (chksize((u_int)u.u_tsize, (u_int)u.u_dsize, (u_int)0,
	    (u_int)u.u_ssize+si))
		return (0);
	if (swpexpand(u.u_dsize, u.u_ssize+si, &u.u_dmap, &u.u_smap)==0)
		return (0);
	
	expand(si, 1);
#endif	MACH_VM
	return (1);
}
