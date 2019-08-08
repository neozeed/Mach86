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
 *	@(#)vm_mem.c	6.9 (Berkeley) 6/8/85
 */
#if	CMU

/*
 **********************************************************************
 * HISTORY
 * 16-Feb-86  Bill Bolosky (bolosky) at Carnegie-Mellon University
 *	Added changes from IBM under switch ROMP to support operation on
 *	a Sailboat.
 *
 * 26-Jan-86  Avadis Tevanian (avie) at Carnegie-Mellon University
 *	Upgraded to 4.3.
 *
 * 11-Oct-85  Robert V Baron (rvb) at Carnegie-Mellon University
 *	shared memory is treated much the way that the "kernelmap"
 *	is.  It is a separate map (shmap) in the SYSMAP and it has
 *	an allocator shmemall and deallocator shmemfree that work
 * 	just like zmemall.  They get pages from the cmap and stuff
 *	them into shmap, using shrmap as the resource map.
 *
 *  8-Mar-85  Robert V Baron (rvb) at Carnegie-Mellon University
 *	must displace physical pages by load point in memall, etc
 *
 * 13-Feb-85  Avadis Tevanian (avie) at Carnegie-Mellon University
 *	Added munhash bugfix.
 **********************************************************************
 *
 */
 
#include "cmu_bugfix.h"
#include "mach_shm.h"
#include "mach_load.h"
#include "mach_vm.h"
#endif	CMU

#if	MACH_VM
#else	MACH_VM
#ifdef	romp
#include "../machine/rosetta.h"
#endif	romp

#include "../machine/pte.h"

#include "param.h"
#include "systm.h"
#include "cmap.h"
#include "dir.h"
#include "user.h"
#include "proc.h"
#include "text.h"
#include "vm.h"
#include "file.h"
#include "inode.h"
#include "buf.h"
#include "mount.h"
#include "trace.h"
#include "map.h"
#include "kernel.h"

#ifdef	DEBUG
#include "../ca/debug.h"
#endif	DEBUG

#if	MACH_SHM
#include "../mp/shmem.h"
#endif	MACH_SHM

/*
 * Allocate memory, and always succeed
 * by jolting page-out daemon
 * so as to obtain page frames.
 * To be used in conjunction with vmemfree().
 */
vmemall(pte, size, p, type)
	register struct pte *pte;
	int size;
	struct proc *p;
{
	register int m;

	if (size <= 0 || size > maxmem)
		panic("vmemall size");
	while (size > 0) {
		if (freemem < desfree)
			outofmem();
		while (freemem == 0)
			sleep((caddr_t)&freemem, PSWP+2);
		m = imin(size, freemem);
		(void) memall(pte, m, p, type);
		size -= m;
		pte += m;
	}
	if (freemem < desfree)
		outofmem();
	/*
	 * Always succeeds, but return success for
	 * vgetu and vgetpt (e.g.) which call either
	 * memall or vmemall depending on context.
	 */
	return (1);
}

/*
 * Free valid and reclaimable page frames belonging to the
 * count pages starting at pte.  If a page is valid
 * or reclaimable and locked (but not a system page), then
 * we simply mark the page as c_gone and let the pageout
 * daemon free the page when it is through with it.
 * If a page is reclaimable, and already in the free list, then
 * we mark the page as c_gone, and (of course) don't free it.
 *
 * Determines the largest contiguous cluster of
 * valid pages and frees them in one call to memfree.
 */
vmemfree(pte, count)
	register struct pte *pte;
	register int count;
{
	register struct cmap *c;
	register struct pte *spte;
	register int j;
	int size, pcnt;
#ifdef notdef
	int fileno;
#endif

	if (count % CLSIZE)
		panic("vmemfree");
	for (size = 0, pcnt = 0; count > 0; pte += CLSIZE, count -= CLSIZE) {
		if (pte->pg_fod == 0 && pte->pg_pfnum) {
			c = &cmap[pgtocm(pte->pg_pfnum)];
			pcnt += CLSIZE;
			if (c->c_lock && c->c_type != CSYS) {
				for (j = 0; j < CLSIZE; j++)
					*(int *)(pte+j) &= PG_PROT;
				c->c_gone = 1;
				goto free;
			}
			if (c->c_free) {
				pcnt -= CLSIZE;
				for (j = 0; j < CLSIZE; j++)
					*(int *)(pte+j) &= PG_PROT;
				if (c->c_type == CTEXT)
					distpte(&text[c->c_ndx], (int)c->c_page, pte);
				c->c_gone = 1;
				goto free;
			}
			if (size == 0)
				spte = pte;
			size += CLSIZE;
			continue;
		}
#ifdef notdef
		/* Don't do anything with mapped ptes */
		if (pte->pg_fod && pte->pg_v)
			goto free;
#endif
		if (pte->pg_fod) {
#ifdef notdef
			fileno = ((struct fpte *)pte)->pg_fileno;
			if (fileno < NOFILE)
				panic("vmemfree vread");
#endif
			for (j = 0; j < CLSIZE; j++)
				*(int *)(pte+j) &= PG_PROT;
		}
free:
		if (size) {
			memfree(spte, size, 1);
			size = 0;
		}
	}
	if (size)
		memfree(spte, size, 1);
	return (pcnt);
}

/*
 * Unlink a page frame from the free list -
 *
 * Performed if the page being reclaimed
 * is in the free list.
 */
munlink(c)
	register struct cmap *c;
{
	register int next, prev;

	next = c->c_next;
	prev = c->c_prev;
	cmap[prev].c_next = next;
	cmap[next].c_prev = prev;
	c->c_free = 0;
	if (freemem < minfree)
		outofmem();
	freemem -= CLSIZE;
}

/*
 * Allocate memory -
 *
 * The free list appears as a doubly linked list
 * in the core map with cmap[0] serving as a header.
 */
memall(pte, size, p, type)
	register struct pte *pte;
	int size;
	struct proc *p;
{
	register struct cmap *c;
	register struct pte *rpte;
	register struct proc *rp;
#ifdef	romp
	register struct hatipt_entry *ipte;
	unsigned sid;
#endif	romp
	int i, j, next, curpos;
	unsigned pf;
	struct cmap *c1, *c2;
	int s;

	if (size % CLSIZE)
		panic("memall");
	s = splimp();
	if (size > freemem) {
		splx(s);
		return (0);
	}
	trace(TR_MALL, size, u.u_procp->p_pid);
	for (i = size; i > 0; i -= CLSIZE) {
		curpos = cmap[CMHEAD].c_next;
		c = &cmap[curpos];
		freemem -= CLSIZE;
		next = c->c_next;
		cmap[CMHEAD].c_next = next;
		cmap[next].c_prev = CMHEAD;
		if (c->c_free == 0)
			panic("dup mem alloc");
#ifdef	romp
#if	MACH_LOAD
		if ((pf = cmtopg(curpos)) > maxfree + loadpg)
#else	MACH_LOAD
		if ((pf = cmtopg(curpos)) > maxfree)
#endif	MACH_LOAD
#else	romp
#if	MACH_LOAD
		if (cmtopg(curpos) > maxfree + loadpg)
#else	MACH_LOAD
		if (cmtopg(curpos) > maxfree)
#endif	MACH_LOAD
#endif	romp
			panic("bad mem alloc");
#ifndef	romp
		if (c->c_gone == 0 && c->c_type != CSYS) {
#else	romp
		/* clear out any previous mapping */
		if (c->c_gone == 0)
			if (c->c_type == CSYS)
				mapout(&pf, CLSIZE);
					/* note: using pf as a pte */
			else {
#endif	romp
			if (c->c_type == CTEXT)
				rp = text[c->c_ndx].x_caddr;
			else
				rp = &proc[c->c_ndx];
			while (rp->p_flag & SNOVM)
				rp = rp->p_xlink;
			switch (c->c_type) {

			case CTEXT:
				rpte = tptopte(rp, c->c_page);
				break;

			case CDATA:
				rpte = dptopte(rp, c->c_page);
				break;

			case CSTACK:
				rpte = sptopte(rp, c->c_page);
				break;
			}
			zapcl(rpte, pg_pfnum) = 0;
			if (c->c_type == CTEXT)
				distpte(&text[c->c_ndx], (int)c->c_page, rpte);
		}
		switch (type) {

		case CSYS:
			c->c_ndx = p->p_ndx;
#ifdef	romp
			sid = RTA_SID_SYSTEM; /* kernel poopa */
#endif	romp
			break;

		case CTEXT:
			c->c_page = vtotp(p, ptetov(p, pte));
			c->c_ndx = p->p_textp - &text[0];
#ifdef	romp
			sid = make410sid( p->p_textp );
#endif	romp
			break;

		case CDATA:
			c->c_page = vtodp(p, ptetov(p, pte));
			c->c_ndx = p->p_ndx;
#ifdef	romp
			sid = p->p_tsize
                             ? makeUsid(p)     /* 410/413 data in P1 */
                             : make407sid(p);  /* 407 data in P0 */
#endif	romp
			break;

		case CSTACK:
			c->c_page = vtosp(p, ptetov(p, pte));
			c->c_ndx = p->p_ndx;
#ifdef	romp
			sid = makeUsid(p);      /* user stack in P1 */
#endif 	romp
			break;
#ifdef	romp
		case CUSTRUCT:
                       c->c_ndx = p->p_ndx;
                       sid = makeUsid(p);      /* upages in P1 */
			break;
#endif	romp
		}
		if (c->c_blkno) {
			/*
			 * This is very like munhash(), except
			 * that we really don't want to bother
			 * to calculate a dev to pass to it.
			 */
			j = CMHASH(c->c_blkno);
			c1 = &cmap[cmhash[j]];
			if (c1 == c)
				cmhash[j] = c1->c_hlink;
			else {
				for (;;) {
					if (c1 == ecmap)
						panic("memall ecmap");
					c2 = c1;
					c1 = &cmap[c2->c_hlink];
					if (c1 == c)
						break;
				}
				c2->c_hlink = c1->c_hlink;
			}
#if	CMU_BUGFIX
			/*
			 *  USENIX Bug #51
			 *
			 *  The (daddr_t)(u_long) causes the c_blkno field to
			 *  be sign extended which causes havoc for large block
			 *  numbers.
			 */
 			if (mfind(c->c_mdev == MSWAPX ?
			      swapdev : mount[c->c_mdev].m_dev,
			      c->c_blkno))
				panic("memall mfind");
#else	CMU_BUGFIX
			if (mfind(c->c_mdev == MSWAPX ?
			      swapdev : mount[c->c_mdev].m_dev,
			      (daddr_t)(u_long)c->c_blkno))
				panic("memall mfind");
#endif	CMU_BUGFIX
			c1->c_mdev = 0;
			c1->c_blkno = 0;
			c1->c_hlink = 0;
		}
#ifndef	romp
		pf = cmtopg(curpos);
		for (j = 0; j < CLSIZE; j++)
			*(int *)pte++ = pf++;
#else	romp
		/* map in all the pages	in this click */
		ipte = &RTA_HATIPT[pf];
#ifdef DEBUG
		if (vmdebug)
			printf("sid=%x,ipte=%x\n", sid,ipte);
#endif DEBUG
		for (j = 0; j < CLSIZE; j++) {
			ipte->key_addrtag = sid << RTA_VPAGE_BITS;
			*(int *)pte++ = pf++;
		}
#endif	romp
		c->c_free = 0;
		c->c_gone = 0;
		if (c->c_intrans || c->c_want)
			panic("memall intrans|want");
		c->c_lock = 1;
		c->c_type = type;
	}
	splx(s);
	return (size);
}

/*
 * Free memory -
 *
 * The page frames being returned are inserted
 * to the head/tail of the free list depending
 * on whether there is any possible future use of them.
 *
 * If the freemem count had been zero,
 * the processes sleeping for memory
 * are awakened.
 */
memfree(pte, size, detach)
	register struct pte *pte;
	register int size;
{
	register int i, j, prev, next;
	register struct cmap *c;
	int s;
	
	if (size % CLSIZE)
		panic("memfree");
	if (freemem < CLSIZE * KLMAX)
		wakeup((caddr_t)&freemem);
	while (size > 0) {
		size -= CLSIZE;
		i = pte->pg_pfnum;
#if	MACH_LOAD
		if (i < firstfree + loadpg || i > maxfree + loadpg)
#else	MACH_LOAD
		if (i < firstfree || i > maxfree)
#endif	MACH_LOAD
			panic("bad mem free");
		i = pgtocm(i);
		c = &cmap[i];
		if (c->c_free)
			panic("dup mem free");
#ifdef	romp
		if (c->c_type != CSYS)	 /* added 3/84	*/
			mapout(pte, CLSIZE);

#endif	romp
		if (detach && c->c_type != CSYS) {
			for (j = 0; j < CLSIZE; j++)
				*(int *)(pte+j) &= PG_PROT;
			c->c_gone = 1;
		}
		s = splimp();
		if (detach && c->c_blkno == 0) {
			next = cmap[CMHEAD].c_next;
			cmap[next].c_prev = i;
			c->c_prev = CMHEAD;
			c->c_next = next;
			cmap[CMHEAD].c_next = i;
		} else {
			prev = cmap[CMHEAD].c_prev;
			cmap[prev].c_next = i;
			c->c_next = CMHEAD;
			c->c_prev = prev;
			cmap[CMHEAD].c_prev = i;
		}
		c->c_free = 1;
		freemem += CLSIZE;
		splx(s);
		pte += CLSIZE;
	}
}

/*
 * Allocate wired-down (non-paged) pages in kernel virtual memory.
 */
caddr_t
wmemall(pmemall, n)
	int (*pmemall)(), n;
{
	register int npg;
	register caddr_t va;
	register int a;

	npg = clrnd(btoc(n));
	a = rmalloc(kernelmap, (long)npg);
	if (a == 0)
		return (0);
	if ((*pmemall)(&Usrptmap[a], npg, &proc[0], CSYS) == 0) {
		rmfree(kernelmap, (long)npg, (long)a);
		return (0);
	}
	va = (caddr_t) kmxtob(a);
	vmaccess(&Usrptmap[a], va, npg);
	return (va);
}

/*
 * Allocate wired-down (non-paged) pages in kernel virtual memory.
 * (and clear them)
 */
caddr_t
zmemall(pmemall, n)
	int (*pmemall)(), n;
{
	register int npg;
	register caddr_t va;
	register int a;

	npg = clrnd(btoc(n));
	a = rmalloc(kernelmap, (long)npg);
	if (a == 0)
		return (0);
	if ((*pmemall)(&Usrptmap[a], npg, &proc[0], CSYS) == 0) {
		rmfree(kernelmap, (long)npg, (long)a);
		return (0);
	}
	va = (caddr_t) kmxtob(a);
	vmaccess(&Usrptmap[a], va, npg);
	while (--npg >= 0)
		clearseg((unsigned)(PG_PFNUM & *(int *)&Usrptmap[a++]));
	return (va);
}

wmemfree(va, n)
	caddr_t va;
	int n;
{
	register int a, npg;

	a = btokmx((struct pte *) va);
	npg = clrnd(btoc(n));
		(void) memfree(&Usrptmap[a], npg, 0);
	rmfree(kernelmap, (long)npg, (long)a);
}

#if	MACH_SHM
/*
 * Allocate wired-down (non-paged) pages in shared memory map.
 * (and clear them)
 */
caddr_t
shmemall(pmemall, n)
	int (*pmemall)(), n;
{
	register int npg;
	register caddr_t va;
	register int a;

	npg = clrnd(btoc(n));
	a = rmalloc(shrmap, (long)npg);
	if (a == 0)
		return (0);
	if ((*pmemall)(&shmap[a], npg, &proc[0], CSYS) == 0) {
		rmfree(shrmap, (long)npg, (long)a);
		return (0);
	}
	va = (caddr_t) (shutl + a * NBPG);
	shmaccess(&shmap[a], va, npg);
	bzero(va, n);
	return (va);
}

shmemfree(va, n)
	caddr_t va;
	int n;
{
	register int a, npg;
	npg = clrnd(btoc(n));

	a = (va - shutl) / NBPG;
	(void) memfree(&shmap[a], npg, 0);
	shmvoid(&shmap[a], va, npg);

	rmfree(shrmap, (long)npg, (long)a);
}
#endif	MACH_SHM

/*
 * Enter clist block c on the hash chains.
 * It contains file system block bn from device dev.
 * Dev must either be a mounted file system or the swap device
 * so we panic if getfsx() cannot find it.
 */
mhash(c, dev, bn)
	register struct cmap *c;
	dev_t dev;
	daddr_t bn;
{
	register int i = CMHASH(bn);

	c->c_hlink = cmhash[i];
	cmhash[i] = c - cmap;
	c->c_blkno = bn;
	i = getfsx(dev);
	if (i == -1)
		panic("mhash");
	c->c_mdev = i;
}

/*
 * Pull the clist entry of <dev,bn> off the hash chains.
 * We have checked before calling (using mfind) that the
 * entry really needs to be unhashed, so panic if we can't
 * find it (can't happen).
 * Must be called at splimp.
 */
munhash(dev, bn)
	dev_t dev;
	daddr_t bn;
{
	register int i = CMHASH(bn);
	register struct cmap *c1, *c2;

	c1 = &cmap[cmhash[i]];
	if (c1 == ecmap)
		panic("munhash");
	if (c1->c_blkno == bn && getfsx(dev) == c1->c_mdev)
		cmhash[i] = c1->c_hlink;
	else {
		for (;;) {
			c2 = c1;
			c1 = &cmap[c2->c_hlink];
			if (c1 == ecmap)
				panic("munhash");
			if (c1->c_blkno == bn && getfsx(dev) == c1->c_mdev)
				break;
		}
		c2->c_hlink = c1->c_hlink;
	}
	if (mfind(dev, bn))
		panic("munhash mfind");
	c1->c_mdev = 0;
	c1->c_blkno = 0;
	c1->c_hlink = 0;
}

/*
 * Look for block bn of device dev in the free pool.
 * Currently it should not be possible to find it unless it is
 * c_free and c_gone, although this may later not be true.
 * (This is because active texts are locked against file system
 * writes by the system.)
 */
struct cmap *
mfind(dev, bn)
	dev_t dev;
	daddr_t bn;
{
	register struct cmap *c1 = &cmap[cmhash[CMHASH(bn)]];
	int si = splimp();

	while (c1 != ecmap) {
		if (c1->c_blkno == bn && c1->c_mdev == getfsx(dev)) {
			splx(si);
			return (c1);
		}
		c1 = &cmap[c1->c_hlink];
	}
	splx(si);
	return ((struct cmap *)0);
}

/*
 * Purge blocks from device dev from incore cache
 * before umount().
 */
mpurge(mdev)
	int mdev;
{
	register struct cmap *c1, *c2;
	register int i;
	int si = splimp();

	for (i = 0; i < CMHSIZ; i++) {
more:
		c1 = &cmap[cmhash[i]];
		if (c1 == ecmap)
			continue;
		if (c1->c_mdev == mdev)
			cmhash[i] = c1->c_hlink;
		else {
			for (;;) {
				c2 = c1;
				c1 = &cmap[c1->c_hlink];
				if (c1 == ecmap)
					goto cont;
				if (c1->c_mdev == mdev)
					break;
			}
			c2->c_hlink = c1->c_hlink;
		}
		c1->c_mdev = 0;
		c1->c_blkno = 0;
		c1->c_hlink = 0;
		goto more;
cont:
		;
	}
	splx(si);
}

/*
 * Initialize core map
 */
meminit(first, last)
	int first, last;
{
	register int i;
	register struct cmap *c;

	firstfree = clrnd(first);
	maxfree = clrnd(last - (CLSIZE - 1));
	freemem = maxfree - firstfree;
	ecmx = ecmap - cmap;
	if (ecmx < freemem / CLSIZE)
		freemem = ecmx * CLSIZE;
	for (i = 1; i <= freemem / CLSIZE; i++) {
#ifdef	romp
		unsigned temp_pte;    /* note: used as a pte */
		/* unlink that real page from hatipt */
		temp_pte = firstfree+i-1;
		if (ishole(temp_pte)) {
			c = &cmap[i];
			c->c_type = CSYS;
			c->c_free = 0;
			continue;	/* do not put into free list */
		}
		mapout((struct pte *)&temp_pte,1); /* link this page as free */
#endif	romp
		cmap[i-1].c_next = i;
		c = &cmap[i];
		c->c_prev = i-1;
		c->c_free = 1;
		c->c_gone = 1;
		c->c_type = CSYS;
		c->c_mdev = 0;
		c->c_blkno = 0;
	}
	cmap[freemem / CLSIZE].c_next = CMHEAD;
	for (i = 0; i < CMHSIZ; i++)
		cmhash[i] = ecmx;
	cmap[CMHEAD].c_prev = freemem / CLSIZE;
	cmap[CMHEAD].c_type = CSYS;
	avefree = freemem;
}

#ifdef notdef
/*
 * Wait for frame pf to become unlocked
 * if it is currently locked.
 */
mwait(c)
	struct cmap *c;
{

	mlock(c);
	munlock(c);
}

/*
 * Lock a page frame.
 */
mlock(c)
	register struct cmap *c;
{

	while (c->c_lock) {
		c->c_want = 1;
		sleep((caddr_t)c, PSWP+1);
	}
	c->c_lock = 1;
}

/*
 * Unlock a page frame.
 */
munlock(c)
	register struct cmap *c;
{

	if (c->c_lock == 0)
		panic("dup page unlock");
	if (c->c_want) {
		wakeup((caddr_t)c);
		c->c_want = 0;
	}
	c->c_lock = 0;
}
#endif notdef

/* 
 * Lock a virtual segment.
 *
 * For each cluster of pages, if the cluster is not valid,
 * touch it to fault it in, otherwise just lock page frame.
 * Called from physio to ensure that the pages 
 * participating in raw i/o are valid and locked.
 */
vslock(base, count)
	caddr_t base;
{
	register unsigned v;
	register int npf;
	register struct pte *pte;
	register struct cmap *c;

	v = btop(base);
	pte = vtopte(u.u_procp, v);
	npf = btoc(count + ((int)base & CLOFSET));
	while (npf > 0) {
		if (pte->pg_v) {
			c = &cmap[pgtocm(pte->pg_pfnum)];
			MLOCK(c);
		} else
			pagein(ctob(v), 1);	/* return it locked */
		pte += CLSIZE;
		v += CLSIZE;
		npf -= CLSIZE;
	}
}

/* 
 * Unlock a virtual segment.
 */
vsunlock(base, count, rw)
	caddr_t base;
{
	register struct pte *pte;
	register struct cmap *c;
	int npf;

	pte = vtopte(u.u_procp, btop(base));
	npf = btoc(count + ((int)base & CLOFSET));
	while (npf > 0) {
		c = &cmap[pgtocm(pte->pg_pfnum)];
		MUNLOCK(c);
		if (rw == B_READ)	/* Reading from device writes memory */
#ifndef	romp
			pte->pg_m = 1;
#else	romp
		{
			pte->pg_m = 1;
			set_mod_bit(pte->pg_pfnum,1);
		}
#endif	romp
		pte += CLSIZE;
		npf -= CLSIZE;
	}
}
#endif	MACH_VM
