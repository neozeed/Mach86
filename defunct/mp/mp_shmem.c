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
 * HISTORY
 *  8-Dec-85  Avadis Tevanian (avie) at Carnegie-Mellon University
 *	Modified unxshmpinit to initialize the process' rmap to
 *	the actual addresses corresponding to shared memory.  This
 *	is necessary for the new VM code.  Also needed to write a
 *	new version of unxshmpfree.
 *
 *  6-Nov-85  Avadis Tevanian (avie) at Carnegie-Mellon University
 *	Changed NPROCESSORS to NCPUS, someday this will be dynamic.
 *
 * 11-Oct-85  Robert V Baron (rvb) at Carnegie-Mellon University
 *	Total rewrite:
 *	TPshmalloc/TPshmfree for user and shmalloc/shmfree for system
 *	users account for their memory in the aarea as before.
 *	shmemall() and shmfree are now used to plunk cmap pages into 
 *	the shmap.
 *	shmustalloc() is like shmemalloc but it will panic if it can't
 *	get enough memory.  It also keeps a small special pool that only
 *	shmustalloc can touch.  Hopefully, this will take care of users
 *	who hog all shared memory and freeze IPC out.
 *
 * 20-Aug-85  Robert V Baron (rvb) at Carnegie-Mellon University
 *	Don't use shared memory proper, just allocate space from
 *	the system.
 *
 * 28-Jun-85  Avadis Tevanian (avie) at Carnegie-Mellon University
 *	Simulate shared memory on a uniprocessor.
 *
 * 19-Mar-85  Robert V Baron (rvb) at Carnegie-Mellon University
 *	define shmprotect to set shmared tables to URKW
 *	TBIA at the end of shmmapreset
 *	limit on TPshmlimit size
 *
 * 26-Feb-85  Robert V Baron (rvb) at Carnegie-Mellon University
 *	check size value on TPshmalloc/shmfree else rmxxx panic's
 *
 * 22-Feb-85  Robert V Baron (rvb) at Carnegie-Mellon University
 *	NPROCESSORS is now defined in mp_param.h.
 *	moved mpq_alloc ... to mp_galloc.c
 *	moved SHMUNIT and associated macros to mp_shmem.h
 *
 * 22-Feb-85  Robert V Baron (rvb) at Carnegie-Mellon University
 *	have shmpfree zero the map cells being freed, because the subr_rmap
 *	routines are pretty stupid.
 *
 * 22-Oct-84  Robert V Baron (rvb) at Carnegie-Mellon University
 *	Sync code with avie's Cleaner version
 */

#include "wb_ml.h"
#include "mach_mp.h"
#include "mach_mpm.h"
#include "mach_vm.h"
#include "cpus.h"

#include "../h/map.h"
#include "../h/param.h"
#include "../h/dir.h"
#include "../h/user.h"
#include "../h/proc.h"
#include "../h/vmmac.h"
#include "../h/systm.h"
#ifdef	vax
#include "../vax/mtpr.h"
#endif	vax
#include "../machine/pte.h"
#include "../machine/cpu.h"

#include "../accent/acc_errors.h"
#ifndef	VAX
#define VAX 1
#include "../accent/vax/acc_impl.h"
#undef VAX
#else	VAX
#include "../accent/vax/acc_impl.h"
#endif	VAX
#include "../sync/mp_queue.h"
#include "../mp/shmem.h"
#include "../mp/remote_prim.h"
#include "../mp/mach_stat.h"

#if	MACH_VM
#include "../vm/vm_map.h"
#include "../vm/vm_kern.h"
#endif	MACH_VM

struct map *shrmap, *eshrmap;
struct aarea *aarea;

struct SHMstate *SHM;

int shareable_pages;

static int shm_lock;

#define w_sh_limit aarea[u.u_procp-proc].a_sh_limit
#define w_sh_size aarea[u.u_procp-proc].a_sh_size
#define w_sh_map aarea[u.u_procp-proc].a_sh_map

/*
 *		Trap interface to shared memory
 */

unxshminit()
{
	SHM->slop = TOTAL_KMSG_PAGES;

	SHM->bytes = shareable_pages * NBPG;

	clrbit(&shm_lock,0);

#if	MACH_VM
#else	MACH_VM
	mpq_allochead_init();		/* Q of Q hdrs is mt */
#endif	MACH_VM
	return Success;
}


#if	MACH_VM
#else	MACH_VM
caddr_t shmemall();
#endif	MACH_VM

TPshmalloc(ptr, sz)
int *ptr;
{
	register unsigned int size = rndshmbndry(sz);
	caddr_t va;

	if (sz <= 0)
		return BadSharedMemorySize;

	if (w_sh_size + size > w_sh_limit) {
		return ProcessSharedMemoryExceded;
	}

		/* spin on lock */
	BUSYP(&shm_lock, 0);

#if	MACH_VM
	if (SHM->bytes < (int) size || (va = (caddr_t)kmem_alloc(sh_map, size, TRUE)) == 0) {
#else	MACH_VM
	if (SHM->bytes < (int) size || (va = shmemall(vmemall, size)) == 0) {
#endif	MACH_VM
		BUSYV(&shm_lock, 0);
		return NotEnoughRoom;
	}

	SHM->bytes -= size;
	BUSYV(&shm_lock, 0);

	shmuseraccess(va, size);

	rmfree(w_sh_map, size, (int)va^0x80000000);
	w_sh_size += size;
	return (! suword(ptr, va) ? Success : NotAUserAddress) ;
}
 
TPshmfree(ptr, sz)
int ptr;
{
	register int size = rndshmbndry(sz);

	if (sz <= 0)
		return BadSharedMemorySize;

	if (!(ptr&0x80000000)) {
		return BadSharedMemoryArg;
	}

	if (!rmget(w_sh_map, size, ptr^0x80000000)) {
		return BadShmfreeRange;
	}

	w_sh_size -= size;
		/* spin on lock */
	BUSYP(&shm_lock, 0);
#if	MACH_VM
	kmem_free(sh_map, ptr, size);
#else	MACH_VM
	shmemfree(ptr, size);
#endif	MACH_VM
	SHM->bytes += size;
	BUSYV(&shm_lock, 0);
	return Success;
}

TPshmlimit(limit)
{
	/* Leave 1Meg of space for others.  Of course, this does not
	 * guarantee several users won't have a collective limit
	 * execeding sh_bytes.  And then can the system run out?
	 */	
	if (limit > (SHM->bytes >>1 ) )
		return BadSharedMemorySize;

	w_sh_limit = limit;
	return Success;
}


	/*
	 *	routines called from other unix calls like, fork
	 *	exit, exec ... as necessary
	 */

/* per user of shared memory at fork time */

unxshmpinit()
{
	w_sh_limit = 40000;
	w_sh_size = 0;

#if	MACH_VM
	rminit(w_sh_map, eshutl-shutl, (long) shutl & ~0x80000000,
		"user shmem rmap", sizeof (w_sh_map)/sizeof(struct mapent));
	rmget(w_sh_map, eshutl-shutl, (long) shutl & ~0x80000000);
#else	MACH_VM
	rminit(w_sh_map, 0, 0, "user shmem rmap",
		sizeof (w_sh_map)/sizeof(struct mapent));
#endif	MACH_VM
}

#if	MACH_VM
unxshmpfree()
{
	register vm_map_t	map;
	register vm_map_entry_t	entry;
	register long		start;

	map = (vm_map_t) w_sh_map->m_limit;

	entry = map->header.next;	/* AKA head of list */
	start = map->min_offset;

		/* spin on lock */
	BUSYP(&shm_lock, 0);

	/*	This is a bit tricky, we must remove the holes! */

	while (entry != &map->header) {
		if (entry->start > start) {	/* hole detected */
			kmem_free(sh_map, start, entry->start - start);
			SHM->bytes += entry->start - start;
		}
		start = entry->end;
		entry = entry->next;
	}
	BUSYV(&shm_lock, 0);

	vm_map_deallocate(map);
}
#else	MACH_VM
unxshmpfree()
{
	register struct map *rmap = w_sh_map;
	register struct mapent *ep = (struct mapent *) (rmap + 1);

	if (!w_sh_size) {
		return;
	}

		/* spin on lock */
	BUSYP(&shm_lock, 0);
	for ( ; ep->m_size && ep < rmap->m_limit; ep++) {

		shmemfree(0x80000000^ep->m_addr, ep->m_size);
		SHM->bytes += ep->m_size;
		ep->m_size = 0;
	}
	BUSYV(&shm_lock, 0);

	unxshmpinit();

}
#endif	MACH_VM

/*
 *	Utilities for one and all
 */

caddr_t
shmalloc(sz)
{
	register int size = rndshmbndry(sz);
	caddr_t va;

		/* spin on lock */
	BUSYP(&shm_lock, 0);

	if (SHM->bytes + (TOTAL_KMSG_PAGES - KMSG_HEADER_PAGES) * NBPG < size ||
#if	MACH_VM
	    (va = (caddr_t)kmem_alloc(sh_map, size, TRUE)) == 0) {
#else	MACH_VM
	    (va = shmemall(vmemall, size)) == 0) {
#endif	MACH_VM
		BUSYV(&shm_lock, 0);
		return 0;
	}
	SHM->bytes -= size;
	BUSYV(&shm_lock, 0);

#if	MACH_VM
	shmuseraccess(va, size);
#endif	MACH_VM

	return va;
}

caddr_t
shmustalloc(sz)
{
	register int size = rndshmbndry(sz);
	caddr_t va;


		/* spin on lock */
	BUSYP(&shm_lock, 0);

#if	MACH_VM
	if ((va = (caddr_t)kmem_alloc(sh_map, size, TRUE)) == 0) {
#else	MACH_VM
	if ((va = shmemall(memall, size)) == 0) {
#endif	MACH_VM
		printf("shmustallocate: shmemall(memall, %x)\
failed.  Trying vmemall\n", size);
#if	MACH_VM
		if (!(int)(va = (caddr_t)kmem_alloc(sh_map, size, TRUE) )) {
#else	MACH_VM
		if (!(int)(va = shmemall(vmemall, size) )) {
#endif	MACH_VM
			printf("shmustallocate: shmemall(vmemall, %d)\
failed.  TILT!!!\n", size);
			panic("shmustallocate");
		}
	}
	SHM->bytes -= size;
	BUSYV(&shm_lock, 0);

	return va;
}


shmfree(ptr, sz)
int ptr;
{
	register int size = rndshmbndry(sz);

	if (!(ptr&0x80000000)) {
		return 0;
	}

	BUSYP(&shm_lock, 0);
#if	MACH_VM
	kmem_free(sh_map, ptr, size);
#else	MACH_VM
	shmemfree(ptr, size);
#endif	MACH_VM
	SHM->bytes += size;
	BUSYV(&shm_lock, 0);

	return 1;
}

shmuseraccess(ptr, size)
register caddr_t ptr;
register int size;
{
#ifndef	romp
#if	MACH_VM
	/* XXX this is bogus and will be flushed */
	register int *ptep = (int *) &Sysmap[btop((unsigned) ptr - 0x80000000)];
#else	MACH_VM
	register int *ptep = (int *) &shmap[btop(ptr - shutl)];
#endif	MACH_VM
	register int i;


	/* user can trash himself */

	for ( i = 0; i < size; i += NBPG) {
		*ptep &= ~PG_PROT;
		*ptep++ |= PG_UW;
#ifdef	vax
		if (size <= TotalKMsgSize) mtpr(TBIS, ptr + i);
#endif	vax
	}
#ifdef	vax
	if (size > TotalKMsgSize) mtpr(TBIA, 0);
#endif	vax
#endif	romp
}

		/*
		 * Old trash
		 */

shmbill(va, sz)
caddr_t va;
{
	register int size = rndshmbndry(sz);

			/* charge to process */
	rmfree(w_sh_map, size, (int)va^0x80000000);
	w_sh_size += size;
	return 1;
}
