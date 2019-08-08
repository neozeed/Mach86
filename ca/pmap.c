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
#define	DISPLAY_DEBUG 0
/*
 *	File:	ca/pmap.c
 *	Author:	Avadis Tevanian, Jr., Michael Wayne Young,
 *		William J. Bolosky
 *
 *	RT PC (romp)/Rosetta Version:
 *	Copyright (c) 1986, William J. Bolosky, Avadis Tevanian, Jr.
 *
 *	Vax Version:
 *	Copyright (C) 1985, Avadis Tevanian, Jr., Michael Wayne Young
 *
 *	Romp/Rosetta Physical memory management routines
 *	(Some routines are still dummies).
 *
 * HISTORY
 *  8-Apr-86  Bill Bolosky (bolosky) at Carnegie-Mellon University
 *	Created from vax version.
 *
 */

/*
 *	Manages physical address maps.
 *
 *	In addition to hardware address maps, this
 *	module is called upon to provide software-use-only
 *	maps which may or may not be stored in the same
 *	form as hardware maps.  These pseudo-maps are
 *	used to store intermediate results from copy
 *	operations to and from address spaces.
 *
 *	Since the information managed by this module is
 *	also stored by the logical address mapping module,
 *	this module may throw away valid virtual-to-physical
 *	mappings at almost any time.  However, invalidations
 *	of virtual-to-physical mappings must be done as
 *	requested.
 *
 *	In order to cope with hardware architectures which
 *	make virtual-to-physical map invalidates expensive,
 *	this module may delay invalidate or reduced protection
 *	operations until such time as they are actually
 *	necessary.  This module is given full information as
 *	to which processors are currently using which maps,
 *	and to when physical maps must be made correct.
 */

#include "cpus.h"
#include "romp_cache.h"
#include "romp_debug.h"

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/map.h"
#include "../h/buf.h"
#include "../h/errno.h"
#include "../h/vm.h"
#include "../h/dir.h"
#include "../h/user.h"
#include "../h/proc.h"
#include "../h/types.h"
#include "../h/thread.h"
#include "../h/zalloc.h"

#include "../sync/lock.h"

#include "../vm/pmap.h"
#include "../vm/vm_map.h"
#include "../vm/vm_kern.h"
#include "../vm/vm_param.h"
#include "../vm/vm_prot.h"
#include "../vm/vm_page.h"

#if	ROMP_DEBUG
#include "../ca/debug.h"
#endif	ROMP_DEBUG
#include "../ca/pcb.h"
#include "../ca/rosetta.h"

#if	DISPLAY_DEBUG
#define	DDISPLAY(number) display(((number/10) << 4) + (number%10))
#else	DISPLAY_DEBUG
#define	DDISPLAY(number) /*NOOP*/
#endif	DISPLAY_DEBUG

#define	HACK_SEG 7
/*
 * Note to future hackers: Several places in this code we want to have rosetta
 * do a virtual address translation on an address which isn't in the currently
 * active pmap.  To do this, we munge the segment register represented by
 * HACK_SEG and then coerce the virtual address to be translated into this
 * segment.  If you should, for some strange reason, decide to change the
 * value of HACK_SEG, you should not use segments e (where the kernel lives),
 * f (which is used for IO), 1 (where the kernel stack lives), 0 (in case
 * virtual = real is turned on), and d (since I plan to move the kernel stack
 * there).
 */


pmap_t	active_pmap;	/* XXX - should be an array for multiprocessors */
struct pmap	kernel_pmap_store;
pmap_t		kernel_pmap;
vm_offset_t	phys_map_vaddr1, phys_map_vaddr2;

struct zone	*pmap_zone;		/* zone of pmap structures */

lock_data_t	pmap_lock;		/* read/write lock for pmap system */

int		rose_page_size = 2048;	/* XXX - Should be init'ed at boot. */
int		rose_page_shift = 11;
int		pmap_threshold;		/* pmap_remove's and pmap_protects on
					   more than this # of pages scan the 
					   ipt rather than the virtual address
					   range.
					 */
vm_offset_t		min_user_pa;	/* Minimum user physical address */
long *is_in; 				/* Initialized by bootstrap */
extern	int	endmem;			/* Physical end of memory */

/*BJB*/ int	magic_ppage = 0;
/*BJB*/ int 	magic_sid = 0;
/*BJB*/ int	magic_vpage = -1;

vm_offset_t trans_virt_addr();

/*
 *	Initialize the physical map module.
 *
 *	For now, VM is already on, we only need to map the
 *	specified memory.
 */
vm_offset_t pmap_map(virt, start, end, prot)
	register vm_offset_t	virt;
	register vm_offset_t	start;
	register vm_offset_t	end;
	int		prot;
{
#if	ROMP_DEBUG
 if (vmdebug)
	printf("Entering pmap_map, virt = 0x%x, start= 0x%x, end = 0x%x, prot = 0x%x\n",
		virt,start,end,prot);
#endif	ROMP_DEBUG
	while (start < end) {
		pmap_enter(kernel_pmap, virt, start, prot, FALSE);
		virt += PAGE_SIZE;
		start += PAGE_SIZE;
	}
	return(virt);
}


long pmap_seg_tab[(RTA_NSEGS + 31) / 32];	/* Bit array for segs used. */
short	max_free_seg = RTA_NSEGS - 1;
/*	
 *	Allocate a segment number which is currently unused.
 *	Current algorithm allocates by doing a linear scan of an
 *	array.  Should be done with a free list.
 */
short pmap_alloc_seg()

{
	register short	cur_seg;

	cur_seg = max_free_seg;
	while (cur_seg > 0) {
		if (isclr(pmap_seg_tab, cur_seg))
			break;
		cur_seg--;
	}

	if (cur_seg <= 0)
		panic("out of segments");		/* XXX */

	setbit(pmap_seg_tab, cur_seg);
	max_free_seg = cur_seg;
#if	ROMP_DEBUG
       if (vmdebug) printf("pmap_alloc_seg: 0x%x.\n",cur_seg);
	return(cur_seg);
#endif	ROMP_DEBUG
}


/*
 *	Deallocate a formerly used segment (duplicate deallocates are NOT ok).
 */
short pmap_dealloc_seg(segid)
short	segid;

{
#if	ROMP_DEBUG
 if (vmdebug) printf("pmap_dealloc_seg: 0x%x\n",segid);
#endif	ROMP_DEBUG
 if (isclr(pmap_seg_tab,segid))
	panic("pmap_dealloc_seg: dup free");

 clrbit(pmap_seg_tab,segid);
	if (max_free_seg < segid)
		max_free_seg = segid;
}

/*
 *	Initialize the segment allocation system.
 */
void pmap_init_seg_alloc()

{
 int	current_seg = RTA_NSEGS;

 for (;current_seg >=0;current_seg--) clrbit(pmap_seg_tab,current_seg);

 setbit(pmap_seg_tab,RTA_SID_SYSTEM);	/* Allocate system segment - XXX */
}


/*
 *	Bootstrap the system enough to run with virtual memory.
 *	Called with mapping ON.
 *
 *	Parameters:
 *	load_start:	PA where kernel was loaded
 *	avail_start	PA of first available physical page
 *	avail_end	PA of last available physical page
 *	virtual_avail	VA of first available page (after kernel bss)
 *	virtual_end	VA of last available page (end of kernel addr space)
 *
 *	&start		start of kernel text
 *	&etext		end of kernel text
 *
 */
void pmap_bootstrap(load_start, avail_start, avail_end,
			virtual_avail, virtual_end)

	vm_offset_t	load_start;
	vm_offset_t	*avail_start;	/* IN/OUT */
	vm_offset_t	*avail_end;	/* IN/OUT */
	vm_offset_t	*virtual_avail;	/* IN/OUT */
	vm_offset_t	*virtual_end;	/* OUT */
{
	long			slr;
	vm_offset_t		vaddr;
	vm_offset_t		s_text, e_text;
	extern int		start_text, etext, end;
	int			index;

	/*
	 *	Initialize the "is it mapped in" table.  lohatipt makes
	 *	sure that all pages are indeed mapped in when we start up.
	 */
	for (index = 0;index < endmem;index++)
		setbit(is_in,index);

#if	ROMP_CACHE
	/*
	 *	Initialize the shared-memory cache hash table.
	 */
	cache_init();
#endif	ROMP_CACHE

	/*
	 *	The kernel's pmap is statically allocated so we don't
	 *	have to use pmap_create, which is unlikely to work
	 *	correctly at this part of the boot sequence.
	 */

	kernel_pmap = &kernel_pmap_store;

	lock_init(&pmap_lock);
	simple_lock_init(&kernel_pmap->lock);

	pmap_init_seg_alloc();/* Initialize the rosetta segment allocator. */

	for (index = 0;index < NUSR_SEGS;index++)
		kernel_pmap->sidtab[index] = 0;
	/*
 	 *	We do not need to set the system segment in the kernel pmap
 	 *	because the system segment (SYS_SEG) is not considered part
	 * 	of a pmap, but rather is a constant (and ser(SYS_SEG) is never
	 *	changed after it is initialized by lohatipt.s).
	 */
	kernel_pmap->ref_count = 1;
	
	*virtual_avail = round_page(VM_MIN_KERNEL_ADDRESS + mem_size); /*XXX*/
	*virtual_end = trunc_page(VM_MAX_KERNEL_ADDRESS);
	min_user_pa = (vm_offset_t)avail_end;  /*No user physical pages yet*/

	return;
}

/*
 *	Initialize the pmap module.
 *	Called by vm_init, to initialize any structures that the pmap
 *	system needs to map virtual memory.
 */
void pmap_init(phys_start, phys_end)
	vm_offset_t	phys_start, phys_end;
{
	long			npages;
    	vm_offset_t		addr;
	vm_size_t		s;
	extern vm_size_t	mem_size;

	/*
	 *	Create the zone of physical maps.
	 */

	s = (vm_size_t) sizeof(struct pmap);
	pmap_zone = zinit(s, 400*s, 4096, FALSE, "pmap"); /* XXX */

}

/*
 *	Create and return a physical map.
 *
 *	If the size specified for the map
 *	is zero, the map is an actual physical
 *	map, and may be referenced by the
 *	hardware.
 *
 *	If the size specified is non-zero,
 *	the map will be used in software only, and
 *	is bounded by that size.
 */
pmap_t pmap_create(size)
	vm_size_t	size;
{
	register vm_size_t		s;
	register pmap_t			p;
	register pmap_statistics_t	stats;
	int				index;

	/*
	 *	A software use-only map doesn't even need a map.
	 */

	if (size != 0) {
		return(PMAP_NULL);
	}

	p = (pmap_t) zalloc(pmap_zone);
	if (p == NULL) {
		panic("pmap_create: cannot allocate a pmap");
	}

	/*
	 *	Initialize the sidtab.
	 */
	for (index = 0;index < NUSR_SEGS;index++)
		p->sidtab[index] = 0;

	p->ref_count = 1;

	simple_lock_init(&p->lock);

	/*
	 *	Initialize statistics.
	 */

	stats = &p->stats;
	stats->resident_count = 0;
	stats->wired_count = 0;

#if	ROMP_DEBUG
	if (vmdebug) printf("pmap_create returns 0x%x.\n",p);
#endif	ROMP_DEBUG
	return(p);
}

/*
 *	Retire the given physical map from service.
 *	Should only be called if the map contains
 *	no valid mappings.
 */

void pmap_destroy(p)
	pmap_t	p;
{
	int		c, index, phys, pl;
	vm_offset_t	addr;

	DDISPLAY(1);
	if (p != PMAP_NULL) {
		simple_lock(&p->lock);
		c = --p->ref_count;
		simple_unlock(&p->lock);
		if (c == 0) {
			/*
			 *	Free up the allocated segments.
			 */
			pl = splvm();
			for (index = 0;index < NUSR_SEGS;index++) 
				if (p->sidtab[index]) 
					pmap_dealloc_seg(p->sidtab[index]);
			zfree(pmap_zone, p);
			splx(pl);
		}
	}
	DDISPLAY(0);
}

/*
 *	Add a reference to the specified pmap.
 */

void pmap_reference(p)
	pmap_t	p;
{
	if (p != PMAP_NULL) {
		simple_lock(&p->lock);
		p->ref_count++;
		simple_unlock(&p->lock);
	}
}


/*
 *
 * mapout - remove a given physical address from the hat/ipt.
 *
 * ipte is the hat/ipt entry for this page.  t is the segid/pagenum of the
 * virtual address of s.  sid is the seg-id of s, vpage is the page num.
 * this and next are ipte pointers used in the hat manipulation, and
 * ppage is the physical page number of this page.
 */
mapout(addr_p)
vm_offset_t	addr_p;
{
	register struct hatipt_entry *ipte, *this, *next;
	register u_int t, sid, vpage, ppage;

#if	ROMP_DEBUG
	if (vmdebug)
		printf("mapout, ppage = 0x%x.\n",addr_p);
#endif	ROMP_DEBUG

	ppage = addr_p >> rose_page_shift;  /* Shift off the offset */

/*BJB*/ if (ppage == magic_ppage) 
/*BJB*/		Debugger("mapout: magic ppage hit.");
/*BJB*/ if ((((magic_sid<<RTA_VPAGE_BITS) | magic_vpage) == 
/*BJB*/	   (RTA_HATIPT[ppage].key_addrtag & (RTA_VPAGE_MASK | RTA_SID_MASK))))
/*BJB*/		Debugger("mapout: magic vaddr hit.");


	if (isclr(is_in,ppage))
		panic("mapout: already out");

	ipte = &RTA_HATIPT[ppage];

	t = ipte->key_addrtag & RTA_ADDRTAG_MASK;
	sid = t >> RTA_VPAGE_BITS;
	vpage = t & RTA_VPAGE_MASK;

	/* hash to find the head of the ipt chain */
	this = &RTA_HATIPT[RTA_HASH(sid, vpage)];

	/* check the chain for empty */
	if (RTA_ENDCHAIN(t = get_hatptr(this)))
		panic("mapout: empty hash chain");
	next = &RTA_HATIPT[t];

	/* if first on the hash chain, simply point hash anchor table at the
 	 * successor of the entry being unmapped.
	 */
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
		/* 
		 * point predecessor of entry being unmapped to successor of
		 * entry being unmapped.
	 	 *
		 */
		this->ipt_ptr = ipte->ipt_ptr;
	}
	/* end case not first */

	ipte->ipt_ptr = RTA_UNDEF_PTR; /* debug */
	invalidate_tlb(vpage);
	clrbit(is_in,ppage);
}


void pmap_range_remove(map, s, e)
	pmap_t		map;
	vm_offset_t	s, e;
{
	vm_offset_t	pa;
	int		seg = atoseg(s), old_sr;
	int		sid;
	int		pl;

	DDISPLAY(3);
#if	ROMP_DEBUG
	if (vmdebug) printf("pmap_remove_rng: map= 0x%x, s = 0x%x, e=0x%x.\n",
				map,s,e);
#endif	ROMP_DEBUG

	pl = splvm();

	lock_read(&pmap_lock);
	simple_lock(&map->lock); 

	if (seg == SYS_SEG)
		sid = RTA_SID_SYSTEM;
	else
		sid = map->sidtab[seg];

	if (sid == 0) {
		printf("pmap_remove: segment %x not mapped in, pmap=0x%x.\n",
			seg,map);
		splx(pl);
		simple_unlock(&map->lock);
		lock_read_done(&pmap_lock);
		return;
	}

	/*
	 *	map the correct segment into HACK_SEG.
	 */

	old_sr = ior(ROSEBASE + HACK_SEG);
	iow(ROSEBASE + HACK_SEG,(sid<<2) | RTA_SEG_PRESENT);
	iow(ROSEBASE + 0x81,HACK_SEG<<28); /* TLB invalidate HACK_SEG */
	while (s < e) {
		if ((pa = 
		    trans_virt_addr((HACK_SEG << 28) | segoffset(s))) != -1) {
			mapout(pa);
#if	ROMP_CACHE
			cache_remove(sid,segoffset(s));
#endif	ROMP_CACHE
			map->stats.resident_count--;
		};

		s += rose_page_size;
	}

	iow(ROSEBASE + HACK_SEG,old_sr);	/* restore HACK_SEG */
	iow(ROSEBASE + 0x81,HACK_SEG<<28); /* TLB invalidate seg HACK_SEG */
	iow(ROSEBASE + 0x81,seg << 28); /* TLB invalidate seg seg */

	simple_unlock(&map->lock);
	lock_read_done(&pmap_lock);
	DDISPLAY(0);
	splx(pl);
}

void pmap_phys_remove(map, s, e)
	register pmap_t		map;
	register vm_offset_t	s, e;
{
	int		seg = atoseg(s);
	register int	sid;
	int		pl;
	register long	key_addrtag;
	register int	pagenum;
	register struct hatipt_entry *ipte;


	DDISPLAY(4);
#if	ROMP_DEBUG
	if (vmdebug) 
		printf("pmap_remove_physical: map= 0x%x, s = 0x%x, e=0x%x.\n",
				map,s,e);
#endif	ROMP_DEBUG

	pl = splvm();

	lock_read(&pmap_lock);
	simple_lock(&map->lock); 

#if	ROMP_CACHE
	cache_remove_range(sid,segoffset(s)>>rose_page_shift,
		(atos(s) == atos(e)) ? segoffset(e) : 0xfffffff);
#endif	ROMP_CACHE

	if (seg == SYS_SEG)
		sid = RTA_SID_SYSTEM;
	else
		sid = map->sidtab[seg];

	if (sid == 0) {
 	 	printf("pmap_remove: segment %x not mapped in, pmap=0x%x.\n",
			seg,map);
		splx(pl);
		simple_unlock(&map->lock);
		lock_read_done(&pmap_lock);
		return;
	}

	if (atoseg(e) == seg)
		e = segoffset(e) >> rose_page_shift;
	else
		e = 0x10000000 - 1 /*XXX*/;
	s = segoffset(s) >> rose_page_shift;

	/*
 	 *	Optimization:  Do not check the kernel text
	 *	if we are removing from a non-system segment (we probably
	 *	do not need to anyway, but just for safety's sake...)
	 */
	for (pagenum = (sid == RTA_SID_SYSTEM) ? 
			0 : min_user_pa;
	     pagenum < endmem;pagenum++) {
		ipte = &RTA_HATIPT[pagenum];
		key_addrtag = ipte->key_addrtag;
		if ((isset(is_in,pagenum))
		  && (((key_addrtag & RTA_SID_MASK) >> RTA_VPAGE_BITS) == sid)
		  && ((key_addrtag & RTA_VPAGE_MASK) >= s)
		  && ((key_addrtag & RTA_VPAGE_MASK) < e)) {
			mapout(pagenum << rose_page_shift);
			map->stats.resident_count--;
		  }
        }


	iow(ROSEBASE + 0x81, seg << 28); /* TLB invalidate seg seg */
	simple_unlock(&map->lock);
	lock_read_done(&pmap_lock);
	DDISPLAY(0);
	splx(pl);
}




/*
 *	Remove the given range of (virtual) addresses
 *	from the specified map.
 *
 *	It is assumed that the start and end are properly
 *	rounded to the ROMP page size.
 *
 *	This routine requires that the start and end address both
 *	be in the same segment.
 *
 *	All pmap_remove does is to decide whether we are removing enough
 *	of a range to warrent scanning through the ipt as opposed to checking
 *	each page in the range and then calls the appropriate (internal)
 *	version of pmap_remove.
 */
void pmap_remove(map, s, e)
	pmap_t		map;
	vm_offset_t	s, e;

{
 
	if (map == PMAP_NULL)
		return;

	if (atoseg(s) != atoseg(e-1))  /* XXX - should work */
		panic("pmap_remove");

	if (((e - s) >> rose_page_shift) > pmap_threshold)
		pmap_phys_remove(map, s, e);
	else
		pmap_range_remove(map, s, e);
}

/*
 *	Routine:	pmap_remove_all
 *	Function:
 *		Removes this physical page from
 *		all physical maps in which it resides.
 *		Reflects back modify bits to the pager.
 *
 *		Since a physical page may have at most one virtual
 * 		address on the rosetta, just map it out.
 *
 *		XXX - pmap locking.
 */
void pmap_remove_all(phys)
	vm_offset_t	phys;
{
	vm_offset_t	addr = phys;
	int		s = splvm();

	DDISPLAY(5);
#if	ROMP_DEBUG
	if (vmdebug) printf("pmap_remove_all: 0x%x.\n",phys);
#endif	ROMP_DEBUG
	for (;addr < (PAGE_SIZE + phys);) {
		if (isset(is_in,addr>>rose_page_shift)) {
			DDISPLAY(35);
			mapout(addr);
			DDISPLAY(5);
		}
		if (get_mod_bit(addr>>rose_page_shift))
			vm_page_set_modified(PHYS_TO_VM_PAGE(addr));
		addr += rose_page_size;
	}
	iow(ROSEBASE + 0x80,1); /* TLB invalidate all XXX (?) */
	DDISPLAY(0);
	splx(s);
}

/*
 *	Routine:	pmap_copy_on_write
 *	Function:
 *		Remove write privileges from all
 *		physical maps for this physical page.
 */
void pmap_copy_on_write(phys)
	vm_offset_t	phys;
{
	int	ppage;
	struct hatipt_entry *ipte;

	DDISPLAY(6);
	/*
	 *	Lock the entire pmap system, since we may be changing
	 *	several maps.
	 */
	lock_write(&pmap_lock);
	if (isset(is_in,phys>>rose_page_shift)) {
		ipte = &RTA_HATIPT[phys>>rose_page_shift];
		ipte->key_addrtag &= ~(3 << RTA_KEY_SHIFT);
		ipte->key_addrtag |= rtakey(0,VM_PROT_READ) << RTA_KEY_SHIFT;
	}
	lock_write_done(&pmap_lock);
	DDISPLAY(0);
}

void pmap_pr_range(map, s, e, prot)
	pmap_t		map;
	vm_offset_t	s, e;
	vm_prot_t	prot;
{
	int		pl, pg, old_sr, seg = atoseg(s);
	int		internal_prot = rtakey(map,prot) << RTA_KEY_SHIFT;
	struct  hatipt_entry	*ipte;
	
	DDISPLAY(7);
	if (seg != (atoseg(e - 1)))
		panic("pmap_protect");

	pl = splvm();
	
	lock_read(&pmap_lock);
	simple_lock(&map->lock);

	old_sr = ior(ROSEBASE + HACK_SEG);
	iow(ROSEBASE + HACK_SEG,RTA_SEG_PRESENT | ((map->sidtab[seg]) << 2));
	iow(ROSEBASE + 0x81,HACK_SEG<<28); /* TLB invalidate HACK_SEG */

	while (s < e) {
		if ((pg = 
		   trans_virt_addr((HACK_SEG << 28) | segoffset(s))) != -1) {
			ipte = &RTA_HATIPT[pg>>rose_page_shift];
			ipte->key_addrtag &= ~(3 << RTA_KEY_SHIFT);
			ipte->key_addrtag |= internal_prot;
		}
		s += rose_page_size;
	}

	iow(ROSEBASE + HACK_SEG,old_sr);
	iow(ROSEBASE + 0x81,HACK_SEG<<28); /* TLB invalidate HACK_SEG */

	simple_unlock(&map->lock);
	lock_read_done(&pmap_lock);

	DDISPLAY(0);
	splx(pl);
}

void pmap_pr_phys(map, s, e, prot)
	pmap_t		map;
	vm_offset_t	s, e;
	vm_prot_t	prot;
{
	int		pl, seg = atoseg(s);
	int		internal_prot = rtakey(map,prot) << RTA_KEY_SHIFT;
	int		sid;
	long		key_addrtag;
	int		pagenum;
	struct  hatipt_entry	*ipte;
	
	lock_read(&pmap_lock);
	simple_lock(&map->lock);


	DDISPLAY(8);
#if	ROMP_DEBUG
	if (vmdebug) 
		printf("pmap_pr_phys: map= 0x%x, s = 0x%x, e=0x%x, prot-0x%x.\n",
				map,s,e,prot);
#endif	ROMP_DEBUG

	pl = splvm();

	if (seg == SYS_SEG)
		sid = RTA_SID_SYSTEM;
	else
		sid = map->sidtab[seg];

	if (sid == 0) {
 	 	printf("pmap_pr_phys: segment %x not mapped in, pmap=0x%x.\n",
			seg,map);
		splx(pl);
		simple_unlock(&map->lock);
		lock_read_done(&pmap_lock);
		return;
	}

	if (atoseg(e) == atoseg(s))
		e = segoffset(e) >> rose_page_shift;
	else
		e = 0x10000000 - 1 /*XXX*/;
	s = segoffset(s) >> rose_page_shift;

	/*
 	 *	Optimization:  Do not check the kernel text
	 *	if we are removing from a non-system segment (we probably
	 *	do not need to anyway, but just for safety's sake...)
	 */
	for (pagenum = (sid == RTA_SID_SYSTEM) ? 
			0 : min_user_pa;
	     pagenum < endmem;pagenum++) {
		ipte = &RTA_HATIPT[pagenum];
		key_addrtag = ipte->key_addrtag;
		if ((isset(is_in,pagenum))
		  && (((key_addrtag & RTA_SID_MASK) >> RTA_VPAGE_BITS) == sid)
		  && ((key_addrtag & RTA_VPAGE_MASK) >= s)
		  && ((key_addrtag & RTA_VPAGE_MASK) < e)) {
			ipte->key_addrtag &= ~(3 << RTA_KEY_SHIFT);
			ipte->key_addrtag |= internal_prot;
		  }
	     }

	simple_unlock(&map->lock);
	lock_read_done(&pmap_lock);

	DDISPLAY(0);
	splx(pl);
}


/*
 *	Set the physical protection on the
 *	specified range of this map as requested.
 */
void pmap_protect(map, s, e, prot)
	pmap_t		map;
	vm_offset_t	s, e;
	vm_prot_t	prot;
{
 
	if (map == PMAP_NULL)
		return;

	if (atoseg(s) != atoseg(e-1))  /* XXX - should work */
		panic("pmap_protect");

	if (((e - s) >> rose_page_shift) > pmap_threshold)
		pmap_pr_phys(map, s, e, prot);
	else
		pmap_pr_range(map, s, e, prot);
}


rtakey(map,prot)
	pmap_t	map;
	register vm_prot_t prot;
{
	if (map == kernel_pmap)
		return(0);
	if (prot & VM_PROT_WRITE)
		return(2);
	if (prot & VM_PROT_READ)
		return(3);
	return(0);
}

#ifdef	notdef
unsigned char do_loop_check = 0;
unsigned char do_chain_check = 0;
int hats_set, ipts_set, max_chain;
loop_check()		/* This debugging routine checks for loops in the 
			    HAT/IPT. */

{
 static long chain_check[(endmem + 31)/32];
 register struct hatipt_entry *ipte;
 int hat_index,ipt_index, len_this_chain;

 if (!do_loop_check) goto check_chain;

/* Check for circular ipt chains. */
 max_chain = 0;
 for (hat_index = 0;hat_index < endmem;hat_index++) {
	ipte = &RTA_HATIPT[hat_index];
	if (ipte->hat_ptr & 0x8000)
		continue;
	for (ipt_index = 0;ipt_index < endmem;ipt_index++)
		clrbit(chain_check,ipt_index);
	ipt_index = ipte->hat_ptr & 0x07ff;
	len_this_chain = 0;
	do {
		len_this_chain++;
		if (isset(chain_check,ipt_index)) {
			printf("chain_check = 0x%x, hat_index = 0x%x.\n",
				chain_check,ipt_index);
			panic("circular chain");
		}
		setbit(chain_check,ipt_index);
		ipte = &RTA_HATIPT[ipt_index];
		ipt_index = ipte->ipt_ptr & 0x07ff;
	} while (!(ipte->ipt_ptr & 0x8000));
 	max_chain = (max_chain > len_this_chain) ? max_chain : len_this_chain;
 }

check_chain:
 if (!do_chain_check)

	return;
/* Check for duplicate ipt pointed to's. */

 hats_set = ipts_set = 0;

 for (ipt_index = 0;ipt_index < endmem;ipt_index++)
	clrbit(chain_check,ipt_index);
 for (hat_index = 0;hat_index < endmem;hat_index++) {
	ipte = &RTA_HATIPT[hat_index];
	if (!(ipte->hat_ptr & 0x8000)) {
		hats_set++;
		if (isset(chain_check,ipte->hat_ptr & 0x07ff)) {
			printf("chain_check = 0x%x, hat_index = 0x%x., hat = 0x%x\n",
				chain_check,hat_index,ipte->hat_ptr);
			panic("dup hat ptrs");
		}
		setbit(chain_check,ipte->hat_ptr & 0x07ff);
	}
 }
 for (ipt_index = 0;ipt_index < endmem;ipt_index++) {
	ipte = &RTA_HATIPT[ipt_index];
	if (!(ipte->ipt_ptr & 0x8000)) {
		ipts_set++;
		if (isset(chain_check,ipte->ipt_ptr & 0x07ff)) {
			printf("chain_check = 0x%x, ipt_index = 0x%x, ipt = 0x%x\n",
				chain_check,ipt_index,ipte->ipt_ptr);
			panic("dup hat/ipt ptrs");
		}
		setbit(chain_check,ipte->ipt_ptr & 0x07ff);
	}
 }
}
#endif	notdef				
	

mapin(ppage, vpage, prot, count, sid)
	vm_offset_t ppage;
	register u_int vpage;
	int prot;
	int count, sid;
{
	register struct hatipt_entry *ipte, *head;
	register int t;
	u_int key, write, tid, lockbits;

/*BJB*/ if (ppage == magic_ppage) 
/*BJB*/		Debugger("mapin: magic ppage hit.");
/*BJB*/ if ((vpage == magic_vpage) && (sid == magic_sid))
		Debugger("mapin: magic sid hit.");

	if (isset(is_in,ppage))
		panic("mapin: page already in");

	if ((sid != RTA_SID_SYSTEM) && (ppage < min_user_pa))
		min_user_pa = ppage;

#if	ROMP_DEBUG
	if (vmdebug)
		printf("mapin(ppage=0x%x,vpage=0x%x,prot=0x%x,count=0x%x, sid = 0x%x)",
		    ppage, vpage, prot, count, sid);
#endif	ROMP_DEBUG

	while (--count >= 0) {
		/* point to the ipte for this page frame */
		ipte = &RTA_HATIPT[ppage];

		/* determine the key, write, tid, lockbits */
		key = prot;	  /* non-special segments only */
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

#if	ROMP_DEBUG
		if (vmdebug)
			printf("head=%x...", head);
#endif	ROMP_DEBUG

		/* link in the new ipte (first on the chain) */
		ipte->ipt_ptr = head->hat_ptr;
		set_hatptr(head, ppage);

		/* paranoia */
		invalidate_tlb(vpage);

		/* Mark it in in our external table. */
		setbit(is_in,ppage);

	}
#ifdef	notdef
 loop_check();
#endif	notdef
}

/*BJB*/ shm_faults = 0;
/*
 *	Insert the given physical page (p) at
 *	the specified virtual address (v) in the
 *	target physical map with the protection requested.
 *
 *	If specified, the page will be wired down, meaning
 *	that the related pte can not be reclaimed.
 *
 *	NB:  This is the only routine which MAY NOT lazy-evaluate
 *	or lose information.  That is, this routine must actually
 *	insert this page into the given map NOW.
 */
void pmap_enter(map, v, p, prot, wired)
	register pmap_t		map;
	vm_offset_t		v;
	register vm_offset_t	p;
	vm_prot_t		prot;
	boolean_t		wired;
{
	int			sid;
	int			i, old_sr;
	int			seg = atoseg(v);
	int			do_mapout = 0;
	int			pl, this_page;

	DDISPLAY(9);
#if	ROMP_DEBUG
	if (vmdebug)
	    printf("pmap_enter, p = 0x%x, v=0x%x, prot = %d, wired = %d.\n",
		p, v, prot, wired);
#endif	ROMP_DEBUG

	if (map == PMAP_NULL)
		return;

	pl = splvm();

	lock_read(&pmap_lock);
	simple_lock(&map->lock);

	if (seg != SYS_SEG) {
		sid = map->sidtab[seg];
		if (sid == 0) {
			sid = map->sidtab[seg] = pmap_alloc_seg();
/* If we are faulting in the currently active pmap, physically enter the
 * new segment id in the rosetta segment register.
 */
			if (map == active_pmap)
				iow(ROSEBASE + seg,RTA_SEG_PRESENT |sid << 2);
	/* No need for TLB invalidate here (or it wouldn't have faulted) */
		}
	} else {
		sid = RTA_SID_SYSTEM;
	}

/*
 * This fakes sr0 into pointing at the segment on which we're working so that
 * the tva will tell us if the virtual address is REALLY mapped in.
 */
	old_sr = ior(ROSEBASE + HACK_SEG);
	iow(ROSEBASE + HACK_SEG,RTA_SEG_PRESENT | (sid << 2));
	iow(ROSEBASE + 0x81,HACK_SEG<<28); /* TLB invalidate HACK_SEG */


	if (isset(is_in,p>>rose_page_shift)) {
		do_mapout = 1;
/*BJB*/		shm_faults++;
	} else
		map->stats.resident_count++;

	prot = rtakey(map,prot);

	i = (page_size / rose_page_size);
	while (i > 0) {
		if (do_mapout) {
			/*
			 *	Here there is no need for a TLB invalidate,
			 *	since any extra TLB entries correspond to
			 *	VALID shared memory aliases.
			 */
			mapout(p);
		}
		if ((this_page = 
		    trans_virt_addr((HACK_SEG << 28) | segoffset(v))) != -1) {
			mapout(this_page);
			iow(ROSEBASE + 0x82,v); /* TLB invalidate address */
		}
		mapin(p>>rose_page_shift,segoffset(v)>>rose_page_shift,
				prot,1,sid);
#if	ROMP_CACHE
		cache_insert(sid,segoffset(v)>>rose_page_shift,p);
#endif	ROMP_CACHE
		p += rose_page_size;
		v += rose_page_size;
		i--;
	}

	iow(ROSEBASE + HACK_SEG,old_sr);

	if (do_mapout) {
		iow(ROSEBASE + 0x80,0);	/* TLB invalidate all */
	} else
		iow(ROSEBASE + 0x81,HACK_SEG<<28); /*TLB invalidate HACK_SEG*/

	simple_unlock(&map->lock);
	lock_read_done(&pmap_lock);
	DDISPLAY(0);
	splx(pl);
}

/*
 *	Routine:	pmap_extract
 *	Function:
 *		Extract the physical page address associated
 *		with the given map/virtual_address pair.
 */

vm_offset_t pmap_extract(pmap, va)
	pmap_t		pmap;
	vm_offset_t	va;
{
	register int	phys;
	int		pl, old_sr;


	simple_lock(&pmap->lock);
	
	pl = splvm();
	DDISPLAY(11);
	if (atoseg(va) == SYS_SEG) {
		phys = trans_virt_addr(va);
		DDISPLAY(0);
		splx(pl);
		return(phys == -1 ? 0 : phys);
	}
	old_sr = ior(ROSEBASE + HACK_SEG);
	iow(ROSEBASE + HACK_SEG,RTA_SEG_PRESENT | 
		((pmap->sidtab[va >> 28]) << 2));
	iow(ROSEBASE + 0x81,HACK_SEG<<28); /* TLB invalidate HACK_SEG */

	phys = trans_virt_addr((HACK_SEG<<28) | segoffset(va));
	if (phys == -1)
		phys = 0;

	iow(ROSEBASE + HACK_SEG,old_sr);
	iow(ROSEBASE + 0x81,HACK_SEG<<28); /* TLB invalidate HACK_SEG */
	splx(pl);

	simple_unlock(&pmap->lock);
	DDISPLAY(0);
	return (phys);
}

/*
 *	Routine:	pmap_access
 *	Function:
 *		Returns whether there is a valid mapping for the
 *		given virtual address stored in the given physical map.
 */

boolean_t pmap_access(pmap, va)
	pmap_t		pmap;
	vm_offset_t	va;
{
	boolean_t	ok;
	short		sid;
	int		index, pl, old_sr;

	if (pmap == PMAP_NULL)
		return;

	simple_lock(&pmap->lock);
	pl = splvm();
	DDISPLAY(12);

	old_sr = ior(ROSEBASE + HACK_SEG);
	iow(ROSEBASE + HACK_SEG,
		pmap->sidtab[atoseg(va)] << 2 | RTA_SEG_PRESENT);
	iow(ROSEBASE + 0x81,HACK_SEG<<28); /* TLB invalidate seg 0 */

	if (trans_virt_addr((HACK_SEG << 28) | segoffset(va)) == -1) 
			/* is it not mapped in? */
		ok = FALSE;
	else
		ok = TRUE;
	iow(ROSEBASE + HACK_SEG,old_sr);
	iow(ROSEBASE + 0x81,HACK_SEG<<28); /* TLB invalidate HACK_SEG */
	splx(pl);
	simple_unlock(&pmap->lock);
	DDISPLAY(0);
	return (ok);
}

/*
 *	Routine:	pmap_copy
 *	Function:
 *		Copy the range specified by src_addr/len
 *		from the source map to the range dst_addr/len
 *		in the destination map.
 */
void pmap_copy(dst_pmap, src_pmap, dst_addr, len, src_addr)
	pmap_t		dst_pmap;
	pmap_t		src_pmap;
	vm_offset_t	dst_addr;
	vm_size_t	len;
	vm_offset_t	src_addr;
{
#ifdef lint
	pmap_copy(dst_pmap, src_pmap, dst_addr, len, src_addr);
#endif lint
}

/*
 *	Require that all active physical maps contain no
 *	incorrect entries NOW.  [This update includes
 *	forcing updates of any address map caching.]
 *
 *	Generally used to insure that a thread about
 *	to run will see a semantically correct world.
 */
void pmap_update()
{
	/*
	 * everyting should properly invalidate the TLB when it runs, so this
	 * should be OK.
	 *
	 */

}

/*
 *	Routine:	pmap_collect
 *	Function:
 *		Garbage collects the physical map system for
 *		pages which are no longer used.
 *		Success need not be guaranteed -- that is, there
 *		may well be pages which are not referenced, but
 *		others may be collected.
 *	Usage:
 *		Called by the pageout daemon when pages are scarce.
 */
void pmap_collect()
{
}

/*
 *	Routine:	pmap_activate
 *	Function:
 *		Binds the given physical map to the given
 *		processor.  (Since there are only uniprocessor
 *		romp/rosetta's, all this does is remember which
 *		pmap is currently active for pmap_enter.)
 *
 */
void pmap_activate(pmap, th, cpu)
	pmap_t		pmap;
	thread_t	th;
	int		cpu;
{
	active_pmap = pmap;
}

/*
 *	Routine:	pmap_deactivate
 *	Function:
 *		Indicates that the given physical map is no longer
 *		in use on the specified processor.
 */
void pmap_deactivate(pmap, th, cpu)
	pmap_t		pmap;
	thread_t	th;
	int		cpu;
{
	active_pmap = PMAP_NULL;
#ifdef lint
	pmap++; th++; cpu++;
#endif lint
}

/*
 *	Routine:	pmap_kernel
 *	Function:
 *		Returns the physical map handle for the kernel.
 */
pmap_t pmap_kernel()
{
    	return (kernel_pmap);
}


/*
 *	pmap_zero_page zeros the specified (machine independent)
 *	page.  
 */
pmap_zero_page(phys)
	register vm_offset_t	phys;
{
	int s = splvm();	/* Can't interrupt with virt_eq_real on! */

	DDISPLAY(13);
	virt_eq_real();
	bzero(phys,PAGE_SIZE);
	virt_neq_real();
	DDISPLAY(0);
	splx(s);
}

/*
 *	pmap_copy_page copies the specified (machine independent)
 *	pages.  
 */

pmap_copy_page(src, dst)
	vm_offset_t	src, dst;
{
	int s = splvm();

	DDISPLAY(14);
	virt_eq_real();
	bcopy(src,dst,PAGE_SIZE);
	virt_neq_real();
	splx(s);
	DDISPLAY(0);
}

/*
 *	Routine:	pmap_pageable
 *	Function:
 *		Make the specified pages (by pmap, offset)
 *		pageable (or not) as requested.
 *
 *		A page which is not pageable may not take
 *		a fault; therefore, its page table entry
 *		must remain valid for the duration.
 *
 *		This routine is merely advisory; pmap_enter
 *		will specify that these pages are to be wired
 *		down (or not) as appropriate.
 */
pmap_pageable(pmap, start, end, pageable)
	pmap_t		pmap;
	vm_offset_t	start;
	vm_offset_t	end;
	boolean_t	pageable;
{
}

/*
 *	Routine:	pmap_redzone
 *	Function:
 *		Make the particular address specified invalid
 *		in the hardware map.  Note that the hardware
 *		page size (not the logical page size) determines
 *		how much memory is made invalid.
 *
 *		[Perhaps only meaningful for wired down pages.]
 */
void pmap_redzone(pmap, addr)
	pmap_t		pmap;
	vm_offset_t	addr;
{
	simple_lock(&pmap->lock);
	/* XXX - Do something! */
	simple_unlock(&pmap->lock);
}

/*
 *	This function takes a virtual address and returns the corresponding
 *	physical address if it is mapped in, else returns -1.
 *
 */
vm_offset_t trans_virt_addr(va)
	register vm_offset_t va;
{
 int	phys;

 iow(ROSEBASE+0x083,va); 	/* 0x083 is Load Real Address */
 phys = ior(ROSEBASE+0x13); 	/* 0x13 is TRAR */
 if (phys < 0)
	return(-1);
 return(phys);
}


/*
 *	copystr - copy a string from kernel vm to kernel vm subject
 *		  to length constraints.
 *
 */
copystr(fromaddr, toaddr, maxlength, lencopied)
register char *fromaddr, *toaddr;
register int	maxlength, *lencopied;
{
 register int bytes_copied = 0;

 DDISPLAY(15);
 while ((bytes_copied < maxlength) && (*toaddr++ = *fromaddr++))
	bytes_copied++;
 if (lencopied != NULL) *lencopied = bytes_copied + 1;
 DDISPLAY(0);
 if (*(toaddr - 1) == '\0') 
 	return(0);
 else
	return(ENOENT);
}

/*
 *	Copy routines for moving data between kernel vm and user vm
 *	subject to length and validation checks.  
 */

copyinstr(fromaddr, toaddr, maxlength, lencopied)
register char *fromaddr, *toaddr;
register int  maxlength, *lencopied;
{
 register int bytes_copied = 0;
 label_t	jmpbuf;
 int		pl;

 DDISPLAY(16);
 pl = splvm();

 if (setjmp(&jmpbuf)) {
	current_thread()->recover = NULL;
	splx(pl);
	return(EFAULT);
 }
 current_thread()->recover = (vm_offset_t)&jmpbuf;

  while ((bytes_copied < maxlength) &&
	 (bytes_copied++,*toaddr++ = *fromaddr++))
	;

  if (lencopied != NULL) *lencopied = bytes_copied;

  (current_thread()->recover) = NULL;

  if (*(toaddr - 1) == '\0') return(0);

  DDISPLAY(0);
  splx(pl);

  if (!(bytes_copied < maxlength)) 
	return(ENOENT);
}



copyoutstr(fromaddr, toaddr, maxlength, lencopied)
register char *fromaddr, *toaddr;
register int  maxlength, *lencopied;
{
 register int bytes_copied = 0;
 label_t	jmpbuf;
 int		pl = splvm();


 DDISPLAY(17); 
 if (setjmp(&jmpbuf)) {
	current_thread()->recover = NULL;
	splx(pl);
	return(EFAULT);
 }
 current_thread()->recover = (vm_offset_t)&jmpbuf;

  while ((bytes_copied < maxlength) &&
	 (bytes_copied++,*toaddr++ = *fromaddr++))
    ;

  if (lencopied != NULL) *lencopied = bytes_copied;

  current_thread()->recover = NULL;

  if ((bytes_copied > 0) && ((*(fromaddr - 1) == '\0')))  return(0);

  DDISPLAY(0);
  splx(pl);
  if (!(bytes_copied < maxlength)) return(ENOENT);
  
}

copyin(from,to,length)
	register caddr_t from, to;
	register unsigned length;
{
 label_t	jmpbuf;
 int		pl = splvm();

 DDISPLAY(18);
 if (setjmp(&jmpbuf)) {
	current_thread()->recover = NULL;
	DDISPLAY(0);
	splx(pl);
	return(EFAULT);
 }
 current_thread()->recover = (vm_offset_t)&jmpbuf;
 bcopy(from,to,length);
 current_thread()->recover = NULL;
 DDISPLAY(0);
 splx(pl);
 return(0);
}

copyout(from,to,length)
	register caddr_t from,to;
	register unsigned length;

{
 label_t	jmpbuf;
 int		pl = splvm();

 DDISPLAY(19);
 if (setjmp(&jmpbuf)) {
	current_thread()->recover = NULL;
	DDISPLAY(0);
	splx(pl);
	return(EFAULT);
 }
 current_thread()->recover = (vm_offset_t)&jmpbuf;
 bcopy(from,to,length);
 current_thread()->recover = NULL;
 DDISPLAY(0);
 splx(pl);
 return(0);
}


/*
 * copy_to_phys and copy_from_phys work by a hack--they use the
 * rosetta virtual-equal-to-physical mode.  Thus, things wouldn't
 * work quite right if the stuff to be copied was in segment-register
 * zero, so if it is we save the value of segment register 1, zot the
 * value of sr0 over it, fix up the virtual address, turn on v_eq_p
 * mode, do the bcopy, zot the register back and are happy.
 *
 */


/*
 *	Copy virtual memory to physical memory.  The virtual memory
 *	must be resident (e.g. the buffer pool).
 */

copy_to_phys(src_addr_v, dst_addr_p, count)
	register vm_offset_t src_addr_v, dst_addr_p;
	register unsigned count;
{
 register old_sr, seg_hack = 0, pl = splvm();
 
 DDISPLAY(20);
 if ((src_addr_v >> 28) == 0) {
	old_sr = ior(ROSEBASE+HACK_SEG);
	seg_hack = 1;
	iow(ROSEBASE + HACK_SEG,ior(ROSEBASE + 0x0));	/* HACK_SEG = sr0 */
	iow(ROSEBASE + 0x81,HACK_SEG<<28); /* TLB invalidate seg 2 */
	src_addr_v |= HACK_SEG<<28;		/* Make src addr in vm */
 }

 virt_eq_real();
 bcopy(src_addr_v,dst_addr_p,count);
 virt_neq_real();

 if (seg_hack) {
	iow(ROSEBASE + HACK_SEG,old_sr);	/* Restore HACK_SEG. */
	iow(ROSEBASE + 0x81,HACK_SEG<<28); /* TLB invalidate HACK_SEG */
 }

 DDISPLAY(0);
 splx(pl);	/* Everything's back to normal. */
}



/*
 *	Copy physical memory to virtual memory.  The virtual memory
 *	must be resident (e.g. the buffer pool).
 */

copy_from_phys(src_addr_p, dst_addr_v, count)
	register vm_offset_t src_addr_p, dst_addr_v;
	register unsigned count;
{
 register old_sr, seg_hack = 0, pl = splvm();
 
 DDISPLAY(21);
 if ((dst_addr_v >> 28) == 0) {
	old_sr = ior(ROSEBASE+HACK_SEG);
	seg_hack = 1;
	iow(ROSEBASE + HACK_SEG,ior(ROSEBASE + 0x0));	/* HACK_SEG = sr0 */
	iow(ROSEBASE + 0x81,HACK_SEG<<28); /* TLB invalidate HACK_SEG */
	dst_addr_v = HACK_SEG<<28 | (segoffset(dst_addr_v));
 }

 virt_eq_real();
 bcopy(src_addr_p,dst_addr_v,count);
 virt_neq_real();

 if (seg_hack) {
	iow(ROSEBASE + HACK_SEG,old_sr);
	iow(ROSEBASE + 0x81,HACK_SEG<<28); /* TLB invalidate HACK_SEG */
 }
 DDISPLAY(0);
 splx(pl);	/* Everything's back to normal. */
}

pagemove(from, to, size)
	register vm_offset_t from, to;
	register int size;
{
	int pl = splvm();
	register caddr_t ra;
	DDISPLAY(21);
#if	ROMP_DEBUG
	if (vmdebug)
		printf("pagemove(from=0x%x,to=0x%x,size0x%x)\n", from, to, size);
#endif	ROMP_DEBUG

	if (size % rose_page_size)
		panic("pagemove");
	if ((((unsigned)from >> 28) != SYS_SEG) 
		|| (((unsigned)to >> 28) != SYS_SEG))
		panic("pagemove seg reg");
	while ((size -= rose_page_size) >= 0) {
		if ((ra = (caddr_t)trans_virt_addr(from)) == (caddr_t) - 1)
			panic("pagemove vtop");
		mapout(ra);
		mapin((int)(ra)>>rose_page_shift, btop(segoffset(to)), 
			0, 1,
			RTA_SID_SYSTEM); /* XXX - Protection */
		from += rose_page_size;
		to += rose_page_size;
	}
  iow(ROSEBASE + 0x82,from);	/* TLB invalidate address */
  DDISPLAY(0);
  splx(pl);
}

/*
 *
 * Glorified tva to make the drivers happy.
 *
 */
caddr_t
real_buf_addr(bp, bufaddr)
	register struct buf *bp;
	register vm_offset_t bufaddr;
{
	caddr_t		phys;
	pmap_t		map;

	if ((bp->b_flags & B_PHYS) && ((atoseg(bufaddr)) != SYS_SEG))
		map = vm_map_pmap(task_table[bp->b_proc-proc]->map);

	if (((phys = (caddr_t)pmap_extract(map,bufaddr)) != (caddr_t)0))
		return(phys);
	else
		return((caddr_t)((int)bufaddr & (rose_page_size - 1)));
}

/*
 * XXX - fix this later.
 */
kernacc(va)
vm_offset_t	va;
{
 DDISPLAY(24);
 if (trans_virt_addr(va) == -1)
	return(0);
 else
	return(1);
}


fuibyte(vaddr)
	register char *vaddr;
{
	return(fubyte(vaddr));
}


/*
 *	Routine:	pmap_change_wiring
 *	Function:	Change the wiring attribute for a map/virtual-address
 *			pair.
 *	Note:		This routine is a dummy for Rosetta.
 */
void pmap_change_wiring(map, v, wired)
{
}



/*
 *	Routine:	pmap_clear_modify
 *	Function:	Clear the hardware modified ("dirty") bit for one
 *			machine independant page starting at the given
 *			physical address.  phys must be aligned on a machine
 *			independant page boundary.
 */

void pmap_clear_modify(phys)
vm_offset_t	phys;

{
 vm_offset_t	addr = phys;

 DDISPLAY(25);
 for (;addr < PAGE_SIZE + phys;addr += rose_page_size)
	set_mod_bit(addr>>rose_page_shift,0);
 DDISPLAY(0);
}

