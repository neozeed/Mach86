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
 *	File:	vm_fault.c
 *	Author:	Avadis Tevanian, Jr., Michael Wayne Young
 *
 *	Copyright (C) 1985, Avadis Tevanian, Jr., Michael Wayne Young
 *
 *	Page fault handling module.
 *
 * HISTORY
 * 25-Jun-85  Avadis Tevanian (avie) at Carnegie-Mellon University
 *	Created.
 */

#include "../h/kern_return.h"
#include "../vm/vm_map.h"
#include "../vm/vm_object.h"
#include "../vm/vm_page.h"
#include "../vm/pmap.h"
#include "../vm/vm_statistics.h"
#include "../vm/vm_pageout.h"

#include "../h/param.h"			/* for PSWP */

/*
 *	vm_fault:
 *
 *	Handle a page fault occuring at the given address,
 *	requiring the given permissions, in the map specified.
 *	If successful, the page is inserted into the
 *	associated physical map.
 *
 *	NOTE: the given address should be truncated to the
 *	proper page address.
 *
 *	KERN_SUCCESS is returned if the page fault is handled; otherwise,
 *	a standard error specifying why the fault is fatal is returned.
 *
 *
 *	The map in question must be referenced, and remains so.
 *	Caller may hold no locks.
 */
kern_return_t vm_fault(map, vaddr, fault_type, change_wiring)
	vm_map_t	map;
	vm_offset_t	vaddr;
	vm_prot_t	fault_type;
	boolean_t	change_wiring;
{
	vm_object_t		first_object;
	vm_offset_t		first_offset;
	vm_object_t		retry_object;
	vm_offset_t		retry_offset;
	vm_map_entry_t		entry;
	register vm_object_t	object;
	register vm_offset_t	offset;
	register vm_page_t	m;
	vm_page_t		mem_cluster[MAX_CLUSTER_SIZE];
	int			cluster_size;
	vm_page_t		new_m;
	vm_prot_t		prot;
	kern_return_t		result;
	boolean_t		wired;
	boolean_t		su;
	int			spl;
	boolean_t		map_locked;

	vm_stat.faults++;		/* needs lock XXX */

	RetryFault: ;

	/*
	 *	Find the backing store object and offset into
	 *	it to begin the search.
	 */

	spl = splvm();
	if ((result = vm_map_lookup(map, vaddr, fault_type, &entry,
			&first_object, &first_offset,
			&prot, &wired, &su)) != KERN_SUCCESS) {
		splx(spl);
		return(result);
	}
	map_locked = TRUE;

   	/*
	 *	Make a reference to this object to
	 *	prevent its disposal while we are messing with
	 *	it.  Once we have the reference, the map is free
	 *	to be diddled.  Since objects reference their
	 *	shadows, they will stay around as well.
	 */

	vm_object_reference(first_object);

	/*
	 *	INVARIANTS (through entire routine):
	 *
	 *	1)	At all times, we must either have the memory
	 *		lock or a busy page in some object to prevent
	 *		some other thread from trying to bring in
	 *		the same page.
	 *
	 *		[Note that we cannot hold any locks during the
	 *		pager access or when waiting for memory, so
	 *		we use a busy page then.]
	 *
	 *	2)	Once we have a busy page, we must remove it from
	 *		the pageout queues, so that the pageout daemon
	 *		will not grab it away.
	 *
	 *	3)	Since map entries are pageable, they may not be
	 *		touched while the paging system is locked.
	 */

	vm_page_lock();

	/*
	 *	Search for the page at object/offset.
	 */

	object = first_object;
	offset = first_offset;
	cluster_size = object->cluster_size;

	/*
	 *	See if this page is resident
	 */

	m = vm_page_lookup(object, offset);
	while ((object != VM_OBJECT_NULL) && (m == VM_PAGE_NULL)) {

		if ((object->pager != vm_pager_null) &&
				(!change_wiring || wired)) {
			/*
			 *	Allocate a new page for this object/offset
			 *	pair.  Once we have a busy page, we can
			 *	release the memory system lock.
			 */

			m = vm_page_alloc(object, offset);

			if (m == VM_PAGE_NULL) {
				vm_page_unlock();
				vm_object_deallocate(first_object);
				if (map_locked)
					vm_map_lookup_done(map, entry);

				vm_page_lock();
				wakeup(&vm_pages_needed);
				sleep_and_unlock(&vm_page_free_count, PSWP+2,
						&vm_page_system_lock);
				splx(spl);
				goto RetryFault;
			}
			
			/*
			 *	Call the pager to retrieve the data, if any.
			 */

			vm_page_unlock();
			if (map_locked) {
				vm_map_lookup_done(map, entry);
				map_locked = FALSE;
			}
			splx(spl);
			if (vm_pager_get(object->pager, object->paging_space,
					m)) {
				spl = splvm();
				vm_page_lock();
				vm_stat.pageins++;
			    	m->busy = FALSE;

				wakeup(m);
				break;
			}

			/*
			 *	Remove the bogus page (which does not
			 *	exist at this object/offset); before
			 *	doing so, we must get back our memory
			 *	system lock to preserve our invariant.
			 *
			 *	Also wake up any other thread that may want
			 *	to bring in this page.
			 */

			spl = splvm();
			vm_page_lock();

			m->busy = FALSE;
			wakeup(m);
			vm_page_free(m);
		}

		/*
		 *	Move on to the next object -- note that
		 *	zero-length objects (notably the zero-fill
		 *	object) only allow offsets of zero.
		 */

		offset += object->shadow_offset;
		object = object->shadow;
		if (object != VM_OBJECT_NULL)
			m = vm_page_lookup(object, offset);
	}

	/*
	 *	PAGE HAS BEEN FOUND, or PAGE SHOULD BE ZERO-FILLED.
	 *	[Loop invariant still holds -- the paging system lock
	 *	is held.]
	 */

	/*
	 *	If the page is being brought in,
	 *	wait for it and then retry.
	 */

	if ((object != VM_OBJECT_NULL) && (m->busy)) {
		vm_page_unlock();
		vm_object_deallocate(first_object);
		if (map_locked)
			vm_map_lookup_done(map, entry);
		vm_page_lock();
		if (m->busy)
			sleep_and_unlock(m, PSWP+1, &vm_page_system_lock);
		else
			vm_page_unlock();
		splx(spl);
		goto RetryFault;
	}

#define	UNLOCK_THINGS \
		vm_page_unlock(); \
		vm_map_lookup_done(map, entry); \
		vm_object_deallocate(first_object);

	/*
	 *	If there's no object left, allocate a page and
	 *	fill it with zeros.
	 */

	if (object == VM_OBJECT_NULL) {
		m = vm_page_alloc(first_object, first_offset);
		if (m == VM_PAGE_NULL) {
			/*
			 *	Map might not be locked here
			 */
			vm_page_unlock();
			vm_object_deallocate(first_object);
			if (map_locked)
				vm_map_lookup_done(map, entry);
			vm_page_lock();
			wakeup(&vm_pages_needed);
			sleep_and_unlock(&vm_page_free_count, PSWP+2,
				&vm_page_system_lock);
			splx(spl);
			goto RetryFault;
		}
		vm_page_zero_fill(m);
		vm_stat.zero_fill_count++;
		m->busy = FALSE;
	}

	/*
	 *	Remove the page from the pageout daemon's reach while
	 *	we play with it.
	 */

	if (m->reclaimable) {
		queue_remove(&vm_page_queue_reclaim, m, vm_page_t, pageq);
		m->reclaimable = FALSE;
		vm_page_reclaim_count--;
		vm_stat.reclaims++;
	}

	if (m->active) {
		queue_remove(&vm_page_queue_active, m, vm_page_t, pageq);
		m->active = FALSE;
		vm_page_active_count--;
	}

	/*
	 *	We must verify that the maps have not changed
	 *	since our last lookup.
	 */

	if (!map_locked) {
		/*
		 *	Since map entries may be pageable, make sure we can
		 *	take a page fault on them.
		 */
		m->busy = TRUE;
		vm_page_unlock();
		result = vm_map_lookup(map, vaddr, fault_type, &entry,
				&retry_object, &retry_offset, &prot, &wired,
				&su);
		vm_page_lock();
		m->busy = FALSE;
		wakeup(m);

		/*
		 *	If we don't need the page any longer, put it on the
		 *	active list (the easiest thing to do here).  If no
		 *	one needs it, pageout will grab it eventually.
		 */

		if (result != KERN_SUCCESS) {
			vm_page_activate(m);
			vm_page_unlock();
			vm_object_deallocate(first_object);
			splx(spl);
			return(result);
		}
		if ((retry_object != first_object) ||
				(retry_offset != first_offset)) {
			vm_page_activate(m);
			UNLOCK_THINGS;
			splx(spl);
			goto RetryFault;
		}
	}

	if (wired)
		fault_type = prot;

	/*
	 *	If the page is being written, but isn't
	 *	already owned by the top-level object,
	 *	we have to copy it into a new page owned
	 *	by the top-level object.
	 *
	 *	Note that zero-fill pages (belonging to the null object)
	 *	don't have to be copied.
	 */

	if ((object != first_object) && (object != VM_OBJECT_NULL)) {
	    	/*
		 *	We only really need to copy if we
		 *	want to write it.
		 */

	    	if (fault_type & VM_PROT_WRITE) {

			/*
			 *	Try to collapse the object.
			 */
			vm_object_collapse(first_object);
				
			/*
			 *	If the collapse succeeded, we don't have to
			 *	allocate a new page.
			 */

			if (m->object != first_object ||
					m->offset != first_offset) {

			    	/*
				 *	Get a new page to copy into
				 */

				new_m = vm_page_alloc(first_object, first_offset);
				if (new_m == VM_PAGE_NULL) {
					vm_page_activate(m);
					UNLOCK_THINGS;
					vm_page_lock();
					wakeup(&vm_pages_needed);
					sleep_and_unlock(&vm_page_free_count,
						PSWP+2, &vm_page_system_lock);
					splx(spl);
					goto RetryFault;
				}

				/*
				 *	Copy into new page, then let it go.
				 */

				vm_page_copy(m, new_m);

				/*
				 *	If another map is truly sharing this
				 *	page with us, we have to flush all
				 *	uses of the original page, since we
				 *	can't distinguish those which want the
				 *	original from those which need the
				 *	new copy.
				 */

				if (!su) {
					/*
					 *	Also, once it's no longer in
					 *	use by any maps, move it to
					 *	the reclaim queue instead.
					 */

					pmap_remove_all(VM_PAGE_TO_PHYS(m));
					vm_page_reclaim(m);
				}
				else
					vm_page_activate(m);

				/*
				 *	Only use the new page below...
				 */

				new_m->busy = FALSE;
				m = new_m;
				vm_stat.cow_faults++;
			}
		}
		else {
		    	prot &= (~VM_PROT_WRITE);
		}
	}

	/*
	 *	If the map said this page is wired down, make it be so.
	 *	It's critically important that a wired-down page be faulted
	 *	only once in each map for which it is wired.
	 */
			
	if (change_wiring && wired)
		vm_page_wire(m);

	m->busy = TRUE;
	vm_page_unlock();

	/*
	 *	Put this page into the physical map.
	 *	We had to do the unlock above because pmap_enter
	 *	may cause other faults.   We don't put the
	 *	page back on the active queue until later so
	 *	that the page-out daemon won't find us (yet).
	 */

	pmap_enter(map->pmap, vaddr, VM_PAGE_TO_PHYS(m), prot, wired);

	vm_page_lock();
	m->busy = FALSE;
	wakeup(m);

	/*
	 *	If the page is not wired down, then put it where the
	 *	pageout daemon can find it.
	 */

	if (change_wiring && !wired) {
		vm_page_unwire(m);	/* may put page on active queue */
	}
	else
		vm_page_activate(m);

	/*
	 *	Unlock everything, and return
	 */

	UNLOCK_THINGS;
	splx(spl);

	return(KERN_SUCCESS);

#undef	UNLOCK_THINGS
}

/*
 *	vm_fault_wire:
 *
 *	Wire down a range of virtual addresses in a map.
 */
void vm_fault_wire(map, start, end)
	vm_map_t	map;
	vm_offset_t	start, end;
{

	register vm_offset_t	va, pa;
	register pmap_t		pmap;

	pmap = vm_map_pmap(map);

	/*
	 *	Inform the physical mapping system that the
	 *	range of addresses may not fault, so that
	 *	page tables and such can be locked down as well.
	 */

	pmap_pageable(pmap, start, end, FALSE);

	/*
	 *	If the page is already in the physical map, we can
	 *	get the physical page from the pmap system and wire
	 *	it down without having to look it up in the backing
	 *	object.  Otherwise we must simulate a fault to get
	 *	the page and enter it in the physical map.
	 */

	vm_page_lock();

	for (va = start; va < end; va += PAGE_SIZE) {

		pa = pmap_extract(pmap, va);
		if (pa != (vm_offset_t) 0) {
			vm_page_wire(PHYS_TO_VM_PAGE(pa));
			pmap_change_wiring(pmap, va, TRUE);
		}
		else {
			vm_page_unlock();
			(void) vm_fault(map, va, VM_PROT_NONE, TRUE);
			vm_page_lock();
		}
	}
	vm_page_unlock();
}


/*
 *	vm_fault_unwire:
 *
 *	Unwire a range of virtual addresses in a map.
 */
void vm_fault_unwire(map, start, end)
	vm_map_t	map;
	vm_offset_t	start, end;
{

	register vm_offset_t	va, pa;
	register pmap_t		pmap;

	pmap = vm_map_pmap(map);

	/*
	 *	Since the pages are wired down, we must be able to
	 *	get their mappings from the physical map system.
	 */

	vm_page_lock();

	for (va = start; va < end; va += PAGE_SIZE) {
		pa = pmap_extract(pmap, va);
		if (pa == (vm_offset_t) 0) {
			panic("unwire: page not in pmap");
		}
		pmap_change_wiring(pmap, va, FALSE);
		vm_page_unwire(PHYS_TO_VM_PAGE(pa));
	}
	vm_page_unlock();

	/*
	 *	Inform the physical mapping system that the range
	 *	of addresses may fault, so that page tables and
	 *	such may be unwired themselves.
	 */

	pmap_pageable(pmap, start, end, TRUE);

}
