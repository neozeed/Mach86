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
 *	@(#)vm_subr.c	6.4 (Berkeley) 6/8/85
 */
#if	CMU

/*
 **************************************************************************
 * HISTORY
 * 22-Mar-86  Bill Bolosky (bolosky) at Carnegie-Mellon University
 *	romp: Merged in RT changes.
 *
 * 26-Jan-86  Avadis Tevanian (avie) at Carnegie-Mellon University
 *	Upgraded to 4.3.
 *
 * 11-Oct-85  Robert V Baron (rvb) at Carnegie-Mellon University
 *	Define shmaccess lime vmaccess.  It makes pages in shared memory
 *	PG_URKW and diddles the TLB.  The user can always read shared
 *	memory
 *
 **************************************************************************
 */

#include "mach_shm.h"
#include "mach_vm.h"
#endif	CMU

#if	MACH_VM
#else	MACH_VM

#include "../machine/pte.h"

#include "param.h"
#include "systm.h"
#include "dir.h"
#include "user.h"
#include "vm.h"
#include "proc.h"
#include "cmap.h"
#include "inode.h"
#include "buf.h"
#include "text.h"
#include "fs.h"

#ifdef vax
#include "../vax/mtpr.h"
#endif

#if	MACH_SHM
#include "map.h"
#include "../mp/shmem.h"
#endif	MACH_SHM
/*
 * Make uarea of process p addressible at kernel virtual
 * address uarea through sysmap locations starting at map.
#ifdef romp
 *
 * Note: uarea is the address of the u struct, not the address of the u area.
 * This is because Vax put the u struct at the start of the u area, and we
 * put it at the end (to keep the kernel stack from crashing into it.  It is
 * ok here, as it is used only by alias, which needs only the rosetta segment
 * number.
#endif romp
 */
#ifdef	romp
uaccess(p, map, uarea)
	register struct proc *p;
	register struct pte *map;
	register caddr_t uarea;
{
	register int i;

	for (i=0; i<UPAGES; ++i)
	       *(unsigned *)(map+i) = PG_PFNUM & (*(unsigned *)(p->p_addr + i));
	alias(map, btop(uarea));
}
#else	romp
uaccess(p, map, uarea)
	register struct proc *p;
	struct pte *map;
	register struct user *uarea;
{
	register int i;
	register struct pte *mp = map;

	for (i = 0; i < UPAGES; i++) {
		*(int *)mp = 0;
		mp->pg_pfnum = p->p_addr[i].pg_pfnum;
		mp++;
	}
	vmaccess(map, (caddr_t)uarea, UPAGES);
}
#endif	romp

/*
 * Validate the kernel map for size ptes which
 * start at ppte in the sysmap, and which map
 * kernel virtual addresses starting with vaddr.
 */
#ifdef	romp
vmaccess(ppte, vaddr, size)
	register struct pte *ppte;
	register caddr_t vaddr;
	register int size;
{
	register unsigned model;  /* THIS IS REALLY A STRUCT PTE */

	while (size--) {
		if ((model = *(unsigned	*)ppte) & PG_PROT);
		else model |= PG_V|PG_KW;
		mapin(ppte, btop(vaddr), model, 1);
		alias(ppte, btop(vaddr));
		ppte++;
		vaddr += NBPG;
	}
}
#else	romp
vmaccess(ppte0, vaddr, size0)
	struct pte *ppte0;
	register caddr_t vaddr;
	int size0;
{
	register struct pte *ppte = ppte0;
	register int size = size0;

	while (size != 0) {
		mapin(ppte, btop(vaddr), (unsigned)(*(int *)ppte & PG_PFNUM), 1,
			(int)(PG_V|PG_KW));
		ppte++;
		vaddr += NBPG;
		--size;
	}
}
#endif	romp

#if	MACH_SHM
/*
 * Validate the kernel map for size ptes which
 * start at ppte in the sysmap, and which map
 * kernel virtual addresses starting with vaddr.
 */
#ifdef	romp
shmaccess(ppte, vaddr, size)
	register struct pte *ppte;
	register caddr_t vaddr;
	register int size;
{
	register unsigned model;  /* THIS IS REALLY A STRUCT PTE */

	while (size--) {
		if ((model = *(unsigned	*)ppte) & PG_PROT)
			;
		else
			model |= PG_V|PG_URKW;
		mapin(ppte, btop(vaddr), model, 1);
		alias(ppte, btop(vaddr));
		ppte++;
		vaddr += NBPG;
	}
}
#else	romp
shmaccess(pte, vaddr, size)
	register struct pte *pte;
	register caddr_t vaddr;
	register int size;
{
	while (size != 0) {
		register unsigned pfnum = (unsigned)(*(int *)pte & PG_PFNUM);
		*(int *)(pte) = (pfnum) | (PG_V|PG_URKW);
		mtpr(TBIS, vaddr);
		pte++;
		vaddr += NBPG;
		--size;
	}
}
#endif	romp
shmvoid(pte, vaddr, size)
	register struct pte *pte;
	register caddr_t vaddr;
	register int size;
{
	while (size != 0) {
#ifndef	vax
		mapout(pte,1);
#endif	vax
		*(int *)(pte) = 0;
#ifdef	vax
		mtpr(TBIS, vaddr);
#endif	vax
		pte++;
		vaddr += NBPG;
		--size;
	}
}
#endif	MACH_SHM

/* 
 * Convert a pte pointer to
 * a virtual page number.
 */
#ifdef	romp
ptetov(p, pte)
	register struct proc *p;
	register struct pte *pte;
{

	if (isatpte(p, pte))
		return (tptov(p, ptetotp(p, pte)));
	else if (isadpte(p, pte))
		return (dptov(p, ptetodp(p, pte)));
	else
		return (sptov(p, ptetosp(p, pte)));
}
#else	romp
ptetov(p, pte)
	register struct proc *p;
	struct pte *pte;
{
	register int j;

	j = pte - p->p_p0br;
	if (j < p->p_tsize + p->p_dsize)
		return (j);
	return ((BTOPUSRSTACK + UPAGES) - p->p_szpt * NPTEPG + j);
}
#endif	romp

#ifdef notdef
/*
 * Convert a virtual page 
 * number to a pte address.
 */
struct pte *
vtopte(p, v)
	register struct proc *p;
	int v;
{

	if (v < p->p_tsize + p->p_dsize)
		return (p->p_p0br + v);
	return (p->p_p0br + (p->p_szpt * NPTEPG + v - (BTOPUSRSTACK + UPAGES)));
}
#endif notdef
#ifdef	romp
/*
 * Convert a virtual page 
 * number to a pte address.
 */
struct pte *
vtopte(p, v)
	register struct proc *p;
	register unsigned v;
{

	if (isatsv(p, v))
		return (tptopte(p, vtotp(p, v)));
	else if (isadsv(p, v))
		return (dptopte(p, vtodp(p, v)));
	else if (isassv(p, v))
		return (sptopte(p, vtosp(p, v)));
	else return(0);
}
#endif	romp

/*
 * Initialize the page tables for paging from an inode,
 * by scouring up the indirect blocks in order.
 * Corresponding area of memory should have been vmemfree()d
 * first or just created.
 */
vinifod(pte, fileno, ip, bfirst, count)
	register struct fpte *pte;
	int fileno;
	register struct inode *ip;
	daddr_t bfirst;
	size_t count;
{
	int blast = bfirst + howmany(count, CLSIZE);
	register int i, j;
	int bn;
	register struct fs *fs = ip->i_fs;
	int nclpbsize = fs->fs_bsize / CLBYTES;

	while (bfirst < blast) {
		i = bfirst % nclpbsize;
		bn = fsbtodb(fs, bmap(ip, bfirst / nclpbsize, B_READ, 0));
		for ( ; i < nclpbsize; i++) {
			pte->pg_fod = 1;
			pte->pg_fileno = fileno;
			if (u.u_error || bn < 0) {
				pte->pg_blkno = 0;
				pte->pg_fileno = PG_FZERO;
				cnt.v_nzfod += CLSIZE;
			} else {
				pte->pg_blkno = bn + btodb(i * CLBYTES);
				cnt.v_nexfod += CLSIZE;
			}
			for (j = 1; j < CLSIZE; j++)
				pte[j] = pte[0];
			pte += CLSIZE;
			bfirst++;
			if (bfirst == blast)
				break;
		}
	}
}
#endif	MACH_VM
