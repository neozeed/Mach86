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
/* $Header: vm_machdep.c,v 4.1 85/09/09 19:00:58 relyea Exp $ */
/* $Source: /ibm/acis/usr/sys_ca/ca/RCS/vm_machdep.c,v $ */

#ifndef lint
static char *rcsid = "$Header: vm_machdep.c,v 4.1 85/09/09 19:00:58 relyea Exp $";
#endif

#ifdef	CMU
/***********************************************************************
 * HISTORY
 * 27-Mar-86  Bill Bolosky (bolosky) at Carnegie-Mellon University
 *	MACH_VM: Turned this file off.
 *
 ***********************************************************************
 */

#include "mach_vm.h"
#endif	CMU

#if	MACH_VM
#else	MACH_VM

/*     vm_machdep.c    6.1     83/07/29        */
/* $Header: vm_machdep.c,v 4.1 85/09/09 19:00:58 relyea Exp $ */
/* $Source: /ibm/acis/usr/sys_ca/ca/RCS/vm_machdep.c,v $ */

#include "../machine/debug.h"
#include "../machine/rosetta.h"
#include "../machine/pte.h"

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/dir.h"
#include "../h/user.h"
#include "../h/proc.h"
#include "../h/cmap.h"
#include "../h/mount.h"
#include "../h/vm.h"
#include "../h/text.h"
#include "../h/buf.h"


#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif

/*
 * Set a red zone in the kernel stack after the u. area.
 */
setredzone(pte)
	register struct pte *pte;
{
	if (vmdebug)
		printf("setredzone( pte=%x )\n", pte);
	*(int *)(pte - 1) = PG_URKR;	  /* fake pte in memory of VAX */
}


/*
 * Map a page into the HATIPT.  Make sure the hash chains are always valid,
 * as ROSETTA (or Sweet Rose in VTL locore) may saunter down one at any time.
 */

mapin(pte, vpage, model, count)
	struct pte *pte;
	register u_int vpage;
	register u_int model;		  /* THIS IS REALLY A STRUCT PTE */
	int count;
{
	register struct hatipt_entry *ipte, *head;
	register int t;
	u_int sid, seg, key, write, tid, lockbits;

	if (vmdebug)
		printf("mapin(pte=0x%x,vpage=0x%x,model=0x%x,count=0x%x)",
		    pte, vpage, model, count);

	while (--count >= 0) {
		/* point to the ipte for this page framt */
		ipte = &RTA_HATIPT[model & PG_PFNUM];
		if (ipte->ipt_ptr != RTA_UNDEF_PTR) /* debug */
			panic("dup mapin");

		/* look up the sid (placed there my memalloc) */
		sid = (ipte->key_addrtag & RTA_ADDRTAG_MASK) >> RTA_VPAGE_BITS;

		/* determine the vpage and segment from the virt addr */
		seg = (vpage & btop(0xf0000000)) >> RTA_VPAGE_BITS;
		vpage &= btop(0x0fffffff);

		if (vmdebug)
			printf(" sid=%x,vpage=%x,seg=%x,", sid, vpage, seg);

		/* determine the key, write, tid, lockbits */
		key = rtakey(model);	  /* non-special segments only */
		write = 1;		  /* special segments only */
		tid = 0;		  /* special segments only */
		lockbits = -1;		  /* special segments only */

		/* fill in the fields to reflect the new virt addr and prot */
		ipte->key_addrtag = (key << RTA_KEY_SHIFT)
			|(sid << RTA_VPAGE_BITS)
			|vpage;
		ipte->w = write;
		ipte->tid = tid;
		ipte->lockbits = lockbits;

		/* hash to find the head of the ipte chain */
		head = &RTA_HATIPT[RTA_HASH(sid, vpage)];

		if (vmdebug)
			printf("head=%x\n", head);

		/* link in the new ipte (first on the chain) */
		ipte->ipt_ptr = head->hat_ptr;
		set_hatptr(head, model & PG_PFNUM);

		/* paranoia */
		invalidate_tlb(vpage);

		/* set the mod bit to reflect prot */
		set_mod_bit(model & PG_PFNUM, ((model & PG_M) != 0));

		/* reset the referenced bit */
		set_ref_bit(model & PG_PFNUM, 0);

		/* set up the vax page table entry and go on */
		*(unsigned *)pte++ = model++; /* model++ IS REALLY model.pg_pgnum++ */
	}
}


rtakey(apte)
	register unsigned apte;		  /* THIS IS REALLY A STRUCT PTE */
{
	switch (apte & PG_PROT) {
	case PG_KW:
		return (RTA_KEY_KW);
	case PG_URKW:
		return (RTA_KEY_URKW);
	case PG_UW:
		return (RTA_KEY_UW);
	case PG_URKR:
		return (RTA_KEY_URKR);
	}
	printf("\nvax pte = 0x%x\n", apte);
	panic("rtakey");
}


/* set the segment register for the specified virtual address */

alias(pte, vpage)
	register struct pte *pte;
	register u_int vpage;
{
	register struct hatipt_entry *ipte;
	register u_int sid, seg;

	if (vmdebug)
		printf("alias( pte 0x%x -> 0x%x, vpage=0x%x ) ",
		    pte, *(unsigned *)pte, vpage);

	/* point to the ipte for this page framt */
	ipte = &RTA_HATIPT[pte->pg_pfnum];

	/* look up the sid (placed there my memalloc) */
	sid = (ipte->key_addrtag & RTA_ADDRTAG_MASK) >> RTA_VPAGE_BITS;

	/* determine the vpage and segment from the virt addr */
	seg = (vpage & btop(0xf0000000)) >> RTA_VPAGE_BITS;

	if (vmdebug)
		printf("sid=%x,seg=%x\n", sid, seg);

	/* load the segment register for this seg */
	set_segreg(seg, sid);
}


/*
 * Unmap a page from the HATIPT, and keep those hash chains valid !
 */

mapout(pte, size)
	struct pte *pte;
	int size;
{
	register struct hatipt_entry *ipte, *this, *next;
	register u_int t, sid, vpage;

	if (vmdebug)
		printf("mapout:  pte=%x,size=%x,", pte, size);

	while (--size >= 0) {
		/* point to the ipte for this page frame */
		ipte = &RTA_HATIPT[pte->pg_pfnum];

		if (vmdebug)
			printf("pfnum=%x,ipte=%x\n", pte->pg_pfnum, ipte);

		/* look up the sid and vpage fields */
		t = ipte->key_addrtag & RTA_ADDRTAG_MASK;
		sid = t >> RTA_VPAGE_BITS;
		vpage = t & RTA_VPAGE_MASK;

		/* hash to find the head of the ipt chain */
		this = &RTA_HATIPT[RTA_HASH(sid, vpage)];

		/* check the chain for empty */
		if (RTA_ENDCHAIN(t = get_hatptr(this)))
			panic("mapout: empty hash chain");
		next = &RTA_HATIPT[t];

		/* if first on the hash chain, simply point hash anchor table at the /*
		   successor of the entry being unmapped. */
		if (next == ipte) {	  /* was first, unlink */
			this->hat_ptr = ipte->ipt_ptr;
		}
		/* end case this one first */

		else {			  /* not first, chase down the chain */
			while (next != ipte) {
				/* go on */
				this = next;
				/* not found? */
				if (RTA_ENDCHAIN(t = get_iptptr(this)))
					panic("mapout: ipte not on chain");
				next = &RTA_HATIPT[t];
			}
			/* point predecessor of entry being unmapped to successor of *
			   entry being unmapped. */
			this->ipt_ptr = ipte->ipt_ptr;
		}
		/* end case not first */

		ipte->ipt_ptr = RTA_UNDEF_PTR; /* debug */
		invalidate_tlb(vpage);

		/* clear the vax pte and go on */
		/* *(int *)pte++ = 0;          */
	}
}


/*
 * Check for valid program size
 */
chksize(ts, ds, ss)
	register unsigned ts, ds, ss;
{
	static int maxdmap = 0;

	if (ts > MAXTSIZ || ds > MAXDSIZ || ss > MAXSSIZ) {
		u.u_error = ENOMEM;
		return (1);
	}
	/* check for swap map overflow */
	if (maxdmap == 0) {
		register int i, blk;

		blk = dmmin;
		for (i = 0; i < NDMAP; i++) {
			maxdmap += blk;
			if (blk < dmmax)
				blk *= 2;
		}
	}
	if (ctod(ts) > NXDAD * dmtext ||
	    ctod(ds) > maxdmap || ctod(ss) > maxdmap) {
		u.u_error = ENOMEM;
		return (1);
	}
	/*
	 * Make sure the process isn't bigger than our
	 * virtual memory limit.
	 *
	 * THERE SHOULD BE A CONSTANT FOR THIS.
	 */
	if (ts + ds + ss + LOWPAGES + HIGHPAGES > btoc(USRSTACK)) {
		u.u_error = ENOMEM;
		return (1);
	}
	return (0);
}


/*ARGSUSED*/
newptes(pte, v, size)
	register struct pte *pte;
	register u_int v;
	register int size;
{
	register int t;
	while (--size >= 0) {
		if (pte->pg_v)
			mapin(pte, v, *(unsigned *)pte, 1);
		invalidate_tlb(v);
		pte++;
		v++;
	}
}


#ifndef NEWPORT
 /* TODO: pass more info to this proc so that non VTL_RELOCATE hardware * can selectively
    purge TLB entries as required.  Better yet, make is * proc a preprocessor macro on
    ROSETTA harwdare. */
invalidate_tlb(vpage)
	register unsigned vpage;
{
	register unsigned i;

	i = (vpage & (RTA_NTLBS - 1)) * RTA_TLBSTEP;
	iow(RTA_TLBAW1 + i, RTA_TLBUNUSED);
	iow(RTA_TLBBW1 + i, RTA_TLBUNUSED);
}


#else

/* TODO: pass more info to this proc so that non VTL_RELOCATE hardware
 * can selectively purge TLB entries as required.  Better yet, make is
 * proc a preprocessor macro on ROSETTA harwdare:
 * #define invalidate_tlb(vpage) iow(RTA_INV_ADDR, ptob(vpage))
 */
invalidate_tlb(vpage)
	register unsigned vpage;
{

	iow(RTA_INV_ADDR, ptob(vpage));
}


#endif

/*
 * Change protection codes of text segment.
 * Have to flush translation buffer since this
 * affect virtual memory mapping of current process.
 */
chgprot(addr, tprot)
	caddr_t addr;
	long tprot;
{
	unsigned v;
	int tp;
	register struct pte *pte;
	register struct cmap *c;
	register struct hatipt_entry *ipte;

	if (vmdebug)
		printf("chgprot(addr=%x,tprot=%x)\n", addr, tprot);

	v = clbase(btop(addr));
	if (!isatsv(u.u_procp, v)) {
		u.u_error = EFAULT;
		return (0);
	}
	tp = vtotp(u.u_procp, v);
	pte = tptopte(u.u_procp, tp);
	if (pte->pg_fod == 0 && pte->pg_pfnum) {
		c = &cmap[pgtocm(pte->pg_pfnum)];
		if (c->c_blkno && c->c_mdev != MSWAPX)
			munhash(mount[c->c_mdev].m_dev,
			    (daddr_t)(u_long)c->c_blkno);
	}
	*(int *)pte &= ~PG_PROT;
	*(int *)pte |= tprot;

	/* fix ipte for non-special segments only -- added for Romp 6/84 */
	if (pte->pg_fod == 0 && pte->pg_pfnum) {
		ipte = &RTA_HATIPT[pte->pg_pfnum];
		ipte->key_addrtag = (ipte->key_addrtag & (~RTA_KEY_BITS))
			|(rtakey(tprot) << RTA_KEY_SHIFT);
		invalidate_tlb(v);
	}
	distcl(pte);
/***   tbiscl(v);   ***/
	return (1);
}


settprot(tprot)
	register long tprot;
{
	register unsigned *pteadr, i;
	register struct hatipt_entry *ipte;

	if (vmdebug)
		printf("settprot(tprot=%x,pid=%d)\n", tprot, u.u_procp->p_pid);

	pteadr = (unsigned *)u.u_pcb.pcb_p0br;
	for (i = 0; i < u.u_procp->p_tsize; i += 1, pteadr += 1) {
		*pteadr &= ~PG_PROT;
		*pteadr |= tprot;

		/* fix ipte for non-special segments only -- added for Romp 6/84 */
		if (!(*pteadr & PG_FOD) && (*pteadr & PG_PFNUM)) {
			ipte = &RTA_HATIPT[*pteadr & PG_PFNUM];
			ipte->key_addrtag = (ipte->key_addrtag & (~RTA_KEY_BITS))
				|(rtakey(tprot) << RTA_KEY_SHIFT);
		}
	}
}


/*
 * Rest are machine-dependent
 */

getmemc(addr)
	caddr_t addr;
{
#ifdef notdef
	register int c;
	struct pte savemap;

	savemap = mmap[0];
	*(int *)mmap = PG_V | PG_KR | btop(addr);
	mtpr(TBIS, vmmap);
	c = *(char *) & vmmap[(int)addr & PGOFSET];
	mmap[0] = savemap;
	mtpr(TBIS, vmmap);
	return (c & 0377);
#endif
	printf("getmemc(addr=%x)\n", addr);
	return (0);
}


putmemc(addr, val)
	caddr_t addr;
{
#ifdef notdef
	struct pte savemap;

	savemap = mmap[0];
	*(int *)mmap = PG_V | PG_KW | btop(addr);
	mtpr(TBIS, vmmap);
	*(char *) & vmmap[(int)addr & PGOFSET] = val;
	mmap[0] = savemap;
	mtpr(TBIS, vmmap);
#endif
	printf("putmemc(addr=%x,val=%x)\n", addr, val);
}


/*
 * Move pages from one kernel virtual address to another.
 * Both addresses are assumed to reside in the Sysmap,
 * and size must be a multiple of CLSIZE.
 */
pagemove(from, to, size)
	register caddr_t from, to;
	register int size;
{

	register caddr_t ra;
	struct pte temp_pte;

	if (vmdebug)
		printf("pagemove(from=0x%x,to=0x%x,size0x%x)\n", from, to, size);

	if (size % CLBYTES)
		panic("pagemove");
	while ((size -= NBPG) >= 0) {
		if ((ra = (caddr_t)vtop(from)) == (caddr_t) - 1)
			panic("pagemove vtop");
		temp_pte.pg_pfnum = btop(ra);
		mapout(&temp_pte, 1);
		mapin(&temp_pte, btop(to), PG_V | PG_KW | (btop(ra)), 1);
		from += NBPG;
		to += NBPG;
	}
}


/****************************************************************************
  cloneseg -  Copy one page (NBPG bytes) from the running process' virtual
              memory address "from", to the same virtual memory address in
              the process owning "pte".
 ****************************************************************************/

cloneseg(from, pte)
	register from;
	register struct pte *pte;
{
	register to;
	extern copybase;

	if (vmdebug)
		printf("cloneseg( from=0x%x, to pte 0x%x -> 0x%x)\n",
		    from, pte, *(unsigned *)pte);

	mapin(pte, btop(from), PG_V | PG_M | PG_UW | (*(unsigned *)pte), 1);
	to = ((int) & copybase) | (from & SEGMENT_OFFSET);
	alias(pte, btop(to));
	bcopy(from, to, NBPG);
}


/****************************************************************************
  clearseg -   Clear one relocation unit (NBPG bytes)
 ****************************************************************************/
#ifdef notdef
clearseg(where)				  /* === see locore.ruasm === */
{
	printf("clearseg: where=%x,ptov(where)=%x\n", where, ptov(ctob(where)));
	mfill(ptov(ctob(where)), ctob(1), 0);
}


#endif
 /* *************************************************************************** vtop -
    map virtual address(bytes) to physical address(bytes) *************************************************************************** 
 */
#ifdef VTL_RELOCATE
vtop(where)
	register unsigned int where;
{
	register struct hatipt_entry *ipte;
	register unsigned int rpage, addrtag, vpage, sid;
	register int probes;
	unsigned int page_off;
#define MAX_PROBES 100
	sid = get_segreg(where >> 28) & 0xFFF;
	vpage = btop(where & 0x0FFFFFFF);
	page_off = where & PGOFSET;
	ipte = &RTA_HATIPT[RTA_HASH(sid, vpage)];
/*printf("vtop:  where=%x,sid=%x,vpage=%x,ipte=%x\n",where,sid,vpage,ipte);*/
	/* make sure there is an ipt chain */
	if (RTA_ENDCHAIN(ipte->hat_ptr)) { /* hat pointer empty...not found */
/*printf("hat chain empty\n");*/
		return (-1);
	}
	/* chase down the hat ptr */
	ipte = &RTA_HATIPT[ipte->hat_ptr];
/*printf("ipte now at %x\n",ipte);*/
	addrtag = (sid << RTA_VPAGE_BITS) | vpage;
/*printf("addrtag=%x\n",addrtag);*/
	/* check addrtag of first element of ipt chain */
	if ((ipte->key_addrtag & 0x1fffffff) == addrtag) {
					  /* same, return vpage + page offset from where 
					  */
/*printf("first on chain,=%x\n",ctob(ipte-RTA_HATIPT)+page_off);*/
		return (ctob(ipte - RTA_HATIPT) + page_off);
	}
	/* for second element ff. */
	probes = 0;
	while (!RTA_ENDCHAIN(ipte->ipt_ptr)) { /* while there is a next element */
		/* point to the next element */
		ipte = &RTA_HATIPT[ipte->ipt_ptr];
/*printf("checking ipte at %x\n",ipte);*/
		/* check addrtag for this element against desired one */
		if ((ipte->key_addrtag & 0x1fffffff) == addrtag) {
					  /* same, return vpage + page offset from where 
					  */
/*printf("found it, = %x\n",ctob(ipte-RTA_HATIPT)+page_off);*/
			return (ctob(ipte - RTA_HATIPT) + page_off);
		}
		if (probes++ >= MAX_PROBES) {
/*printf("VTOP:  loop chasing down an ipt chain!!!"n");*/
			return (-1);
		}
	}
/*printf("end of chain...not found\n");*/
	return (-1);
}


 /* end of vtop */
#endif
#ifdef ROSETTA_0
vtop(where)
	register int *where;
{
	register caddr_t paddr;
	register u_int x;

	x = splimp();
	iow(RTA_VTOP, where);
	paddr = (caddr_t)ior(RTA_REALADDR);
	splx(x);
	return ((int)paddr < 0 ? -1 : (int)paddr);
}


#endif

/****************************************************************************
  ptov - map physical address(bytes) to virtual address(bytes)
 ****************************************************************************/
ptov(where)
	register unsigned int where;
{
	register struct hatipt_entry *ipte;
	register unsigned int rpage, addrtag, vpage, sid;
	rpage = btop(where);		  /* shift by log2(pagesize) */
	ipte = &RTA_HATIPT[rpage];
	addrtag = ipte->key_addrtag & 0x1fffffff;
/*printf("ptov:  where=%x,addrtag=%x,ipte=%x  ",where,addrtag,ipte);*/
	vpage = addrtag & RTA_VPAGE_MASK;
	sid = addrtag >> RTA_VPAGE_BITS;
/*printf("  rpage=%x,addrtag=%x,vpage=%x,sid=%x - ",rpage,addrtag,vpage,sid);*/
	where = (where & PGOFSET) | ctob(vpage);
	if (sid == RTA_SID_SYSTEM) {
/*printf("system:%x\n",where+SYSBASE);*/
		return (where | SYSBASE);
	} else if (sid == RTA_SID_UNUSED) {
/*printf("unused\n");*/
		return (-1);
	} else if (sid < RTA_SID_SYSTEM) { /* user text */
/*printf("user text:%x\n",where);*/
		return (where);
	} else {
/*printf("user data:%x\n",where | DATABASE);*/
		return (where | DATABASE);
	}
}


 /* end of ptov */

/****************************************************************************
  isitok - check the validity of an access to a range of virtual addresses
 ****************************************************************************/
isitok(vaddr, length, access)
	register unsigned vaddr;	  /* THIS IS REALLY A CADDR_T */
	register int length;
	register unsigned access;
{
	register unsigned laddr, v_max;	  /* THIS IS REALLY A CADDR_T */
	register int i;

	if (length < 0)			  /* Negative or really huge values */
		return(FALSE);		  /* are no good. */

	if (vaddr < ENDOFP1) {		  /* user virtual address */
		register struct pte *pte;

		{
/* calculate pte and v_max (largest legal virtual address in segment) */
			register unsigned v;
			register struct proc *p;

			p = u.u_procp;
			v = btop(vaddr);

			if (isatsv(p, v)) {
				pte = tptopte(p, vtotp(p, v));
				v_max = (p)->p_tsize;
			} else if (isadsv(p, v)) {
				pte = dptopte(p, vtodp(p, v));
				v_max = (p)->p_dsize + ((p)->p_tsize ? btop(DATABASE) : 0);
			} else if (isassv(p, v)) {
				pte = sptopte(p, vtosp(p, v));
				v_max = btop(USRSTACK);
			} else
				return (FALSE);
		}
		do {
			i = *(int *)pte++ & PG_PROT;
			switch (access) {
			case UWRITE_OK:
				if (i == PG_UW)
					break;
				return (FALSE);
			case UREAD_OK:
				if (i >= PG_UW)
					break;
				return (FALSE);
			case KWRITE_OK:
				if (i == PG_UW || i == PG_URKW || i == PG_KW)
					break;
				return (FALSE);
			case KREAD_OK:
				if (i != PG_NOACC)
					break;
				return (FALSE);
			}
			/* end switch */
		}
		while ((length -= NBPG) > 0 && (vaddr += NBPG));

		if (btop(vaddr) >= v_max)
			return (FALSE);

		if ((length += NBPG - 1) > 0) {
			laddr = vaddr + length;	/* last address in range */
			if ((laddr >> PGSHIFT) == (vaddr >> PGSHIFT))
				return (TRUE); /* same page as checked above */
			i = *(int *)pte & PG_PROT;
			switch (access) {
			case UWRITE_OK:
				if (i == PG_UW)
					break;
				return (FALSE);
			case UREAD_OK:
				if (i >= PG_UW)
					break;
				return (FALSE);
			case KWRITE_OK:
				if (i == PG_UW || i == PG_URKW || i == PG_KW)
					break;
				return (FALSE);
			case KREAD_OK:
				if (i != PG_NOACC)
					break;
				return (FALSE);
			}
			/* end switch */
		}
		return (TRUE);
	}
	/* kernel virtual address -- used only by kernel memory device driver */

	do {
		if ((i = vtop(vaddr) >> PGSHIFT) == -1)
			return (FALSE);
		if (!(RTA_KEYBIT(RTA_HATIPT[i].key_addrtag) & access))
			return (FALSE);
	}
	while ((length -= NBPG) > 0 && (vaddr += NBPG));

	if ((length += NBPG - 1) > 0) {
		laddr = vaddr + length;	  /* last address in range */
		if ((laddr >> PGSHIFT) == (vaddr >> PGSHIFT))
			return (TRUE);	  /* same page as checked above */
		if ((i = vtop(laddr) >> PGSHIFT) == -1)
			return (FALSE);
		if (!(RTA_KEYBIT(RTA_HATIPT[i].key_addrtag) & access))
			return (FALSE);
	}
	return (TRUE);
}


#ifdef notdef
 /* *************************************************************************** willbeok
    - check the validity of a future access to a virtual addresses *************************************************************************** 
 */
willbeok(vaddr, access)
	register unsigned vaddr;	  /* THIS IS REALLY A CADDR_T */
	register unsigned access;
{
	register struct pte *pte;

	return ((pte = vtopte(u.u_procp, btop(vaddr)))
			&&pte->pg_fod
	    &&(access & RTA_KEY_OK(rtakey(*(unsigned *)pte)))
		);
}


#endif

/****************************************************************************
real_buf_addr - translate a virtual address into a real memory address.
		The virtual address need not be mapped into an address space.
****************************************************************************/
caddr_t
real_buf_addr(bp, bufaddr)
	register struct buf *bp;
	register char *bufaddr;
{
	register v, o;
	register struct pte *pte;
	struct proc *p;

	v = btop(bufaddr);
	o = (int)bufaddr & PGOFSET;
	p = bp->b_flags & B_DIRTY ? &proc[2] : bp->b_proc;

	if ((bp->b_flags & B_PHYS) == 0)
		return ((caddr_t)vtop(bufaddr));
	else if (bp->b_flags & B_UAREA)
		pte = &p->p_addr[v];
	else if (bp->b_flags & B_PAGET)
		pte = &Usrptmap[btokmx((struct pte *)bufaddr)];
	else if ((pte = vtopte(p, v)) == 0)
		panic("real_buf_addr");

	return ((caddr_t)((pte->pg_pfnum << PGSHIFT) | o));
}
#endif	MACH_VM
