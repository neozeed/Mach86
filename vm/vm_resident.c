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
 *	File:	vm/vm_page.c
 *	Author:	Avadis Tevanian, Jr., Michael Wayne Young
 *
 *	Copyright (C) 1985, Avadis Tevanian, Jr., Michael Wayne Young
 *
 *	Resident memory management module.
 *
 * HISTORY
 *  1-Jun-86  Avadis Tevanian (avie) at Carnegie-Mellon University
 *	Added vm_page_activate.
 *
 * 29-May-86  David Golub (dbg) at Carnegie-Mellon University
 *	Changed (auto-increment + test) in vm_page_wire, vm_page_unwire
 *	to separate operations to compensate for RT code-generation bug.
 *
 * 21-May-86  David Golub (dbg) at Carnegie-Mellon University
 *	Added page_mask, and vm_set_page_size to set all page_size
 *	related constants from page_size.
 *
 * 17-May-86  Avadis Tevanian (avie) at Carnegie-Mellon University
 *	Call pmap_clear_modify on newly allocated pages.
 *
 * 10-Jun-85  Avadis Tevanian (avie) at Carnegie-Mellon University
 *	Created.
 */

#include "../h/types.h"
#include "../vm/vm_map.h"
#include "../vm/vm_page.h"
#include "../vm/vm_prot.h"
#include "../vm/vm_statistics.h"
#include "../vm/vm_pageout.h"

#include "../vm/pmap.h"

/*
 *	Associated with page of user-allocatable memory is a
 *	page structure.
 */

vm_offset_t	map_data;
vm_size_t	map_data_size;

vm_offset_t	kentry_data;
vm_size_t	kentry_data_size;

vm_offset_t	kobject_data;
vm_size_t	kobject_data_size;

vm_offset_t	zdata;
vm_size_t	zdata_size;

queue_head_t	*vm_page_buckets;	/* Array of buckets */
int		vm_page_bucket_count;	/* How big is array? */

vm_size_t	page_size  = 4096;
vm_size_t	page_mask  = 4095;
int		page_shift = 12;

/*
 *	vm_set_page_size:
 *
 *	Sets the page size, perhaps based upon the memory
 *	size.  Must be called before any use of page-size
 *	dependent functions.
 *
 *	Sets page_shift and page_mask from page_size.
 *	Spins forever if page_size is not a power of 2.
 */
void vm_set_page_size()
{
	vm_size_t	i;

	page_mask = page_size - 1;
	page_shift = 0;
	for (i = 1; i != page_size; i *= 2)
		page_shift++;
}


/*
 *	vm_page_init:
 *
 *	Initializes the resident memory module.
 *
 *	Allocates memory for the page cells, and
 *	for the object/offset-to-page hash table headers.
 *	Each page cell is initialized and placed on the free list.
 */
vm_offset_t vm_page_init(start, end, vaddr)
	register vm_offset_t	start;
	vm_offset_t	end;
	register vm_offset_t	vaddr;
{
	register vm_offset_t	mapped;
	register vm_page_t	m;
	register queue_t	bucket;
	vm_size_t		npages;
	register vm_offset_t	new_start;
	int			i;

	/*
	 *	Initialize the paging system lock
	 */

	simple_lock_init(&vm_page_system_lock);

	/*
	 *	Initialize the queue headers for the free queue,
	 *	the active queue and the reclaim queue.
	 */

	queue_init(&vm_page_queue_free);
	queue_init(&vm_page_queue_active);
	queue_init(&vm_page_queue_reclaim);

	/*
	 *	Allocate (and initialize) the hash table buckets.
	 *
	 *	The number of buckets MUST BE a power of 2, and
	 *	the actual value is the next power of 2 greater
	 *	than the number of physical pages in the system.
	 *
	 *	Note:
	 *		This computation can be tweaked if desired.
	 */

	vm_page_buckets = (queue_t) vaddr;
	bucket = vm_page_buckets;
	vm_page_bucket_count = 1;
	while (vm_page_bucket_count < atop(end - start))
		vm_page_bucket_count <<= 1;

	/*
	 *	Validate these addresses.
	 */

	new_start = round_page(((queue_t)start) + vm_page_bucket_count);
	mapped = vaddr;
	vaddr = pmap_map(mapped, start, new_start,
			VM_PROT_READ|VM_PROT_WRITE);
	start = new_start;
	blkclr(mapped, vaddr - mapped);
	mapped = vaddr;

	for (i = vm_page_bucket_count; i--;) {
		queue_init(bucket);
		bucket++;
	}

	/*
	 *	round (or truncate) the addresses to our page size.
	 */

	end = trunc_page(end);

	/*
	 *	Steal pages for some zones that cannot be
	 *	dynamically allocated.
	 */

	zdata_size = round_page(1*1024);	/* XXX - should compute */
	zdata = (vm_offset_t) vaddr;
	vaddr += zdata_size;

	map_data_size = page_size;		/* will expand at run-time */
	map_data = (vm_offset_t) vaddr;
	vaddr += map_data_size;

	/*
	 *	Allow 1000 kernel map entries... this should be plenty
	 *	since people shouldn't be cluttering up the kernel
	 *	map (they should use their own maps).
	 */

	kentry_data_size = round_page(1000*sizeof(struct vm_map_entry));
	kentry_data = (vm_offset_t) vaddr;
	vaddr += kentry_data_size;

	kobject_data_size = round_page(10*1024); /* XXX - should compute */
	kobject_data = (vm_offset_t) vaddr;
	vaddr += kobject_data_size;

	/*
	 *	Validate these zone addresses.
	 */

	new_start = start + (vaddr - mapped);
	pmap_map(mapped, start, new_start, VM_PROT_READ|VM_PROT_WRITE);
	blkclr(mapped, (vaddr - mapped));
	mapped = vaddr;
	start = new_start;

	/*
 	 *	Compute the number of pages of memory that will be
	 *	available for use (taking into account the overhead
	 *	of a page structure per page).
	 */

	vm_page_free_count = npages =
		(end - start)/(PAGE_SIZE + sizeof(struct vm_page));

	/*
	 *	Initialize the mem entry structures now, and
	 *	put them in the free queue.
	 */

	m = vm_page_array = (vm_page_t) vaddr;
	first_page = start;
	first_page += npages*sizeof(struct vm_page);
	first_page = atop(round_page(first_page));

	/*
	 *	Validate these addresses.
	 */

	new_start = start + (round_page(m + npages) - mapped);
	mapped = pmap_map(mapped, start, new_start,
			VM_PROT_READ|VM_PROT_WRITE);
	start = new_start;

	while (npages--) {
		m->reclaimable = FALSE;
		m->active = FALSE;
		m->busy = FALSE;
		m->object = VM_OBJECT_NULL;
		queue_enter(&vm_page_queue_free, m, vm_page_t, pageq);
		m++;
	}

	return(mapped);
}

/*
 *	vm_page_hash:
 *
 *	Distributes the object/offset key pair among hash buckets.
 *
 *	NOTE:  This macro depends on vm_page_bucket_count being a power of 2.
 */
#define vm_page_hash(object, offset) \
	(((unsigned)object+(unsigned)atop(offset))&(vm_page_bucket_count-1))

/*
 *	vm_page_insert:		[ internal use only ]
 *
 *	Inserts the given mem entry into the object/object-page
 *	table and object list.
 *
 *	Memory system must be locked for write; no side effects.
 */

void vm_page_insert(mem, object, offset)
	register vm_page_t	mem;
	register vm_object_t	object;
	register vm_offset_t	offset;
{
	register queue_t	bucket;

	/*
	 *	Record the object/offset pair in this page
	 */

	mem->object = object;
	mem->offset = offset;

	/*
	 *	Insert it into the object_object/offset hash table
	 */

	bucket = &vm_page_buckets[vm_page_hash(object, offset)];
	queue_enter(bucket, mem, vm_page_t, hashq);

	/*
	 *	Now link into the object's list of backed pages.
	 */

	queue_enter(&object->memq, mem, vm_page_t, listq);
}

/*
 *	vm_page_remove:		[ internal use only ]
 *
 *	Removes the given mem entry from the object/offset-page
 *	table and the object page list.
 *
 *	Memory system must be locked for write; no side effects.
 */

void vm_page_remove(mem)
	register vm_page_t	mem;
{
	register queue_t	bucket;

	/*
	 *	Remove from the object_object/offset hash table
	 */

	bucket = &vm_page_buckets[vm_page_hash(mem->object, mem->offset)];
	queue_remove(bucket, mem, vm_page_t, hashq);

	/*
	 *	Now remove from the object's list of backed pages.
	 */

	queue_remove(&mem->object->memq, mem, vm_page_t, listq);
}

/*
 *	vm_page_lookup:
 *
 *	Returns the page associated with the object/offset
 *	pair specified; if none is found, VM_PAGE_NULL is returned.
 *
 *	The memory system must be locked at least for read;
 *	no side effects.
 */

vm_page_t vm_page_lookup(object, offset)
	register vm_object_t	object;
	register vm_offset_t	offset;
{
	register vm_page_t	mem;
	register queue_t	bucket;

	/*
	 *	Search the hash table for this object/offset pair
	 */

	bucket = &vm_page_buckets[vm_page_hash(object, offset)];

	mem = (vm_page_t) queue_first(bucket);
	while (!queue_end(bucket, (queue_entry_t) mem)) {
		if ((mem->object == object) && (mem->offset == offset))
			return(mem);
		mem = (vm_page_t) queue_next(&mem->hashq);
	}

	return(VM_PAGE_NULL);
}

/*
 *	vm_page_rename:
 *
 *	Move the given memory entry from its
 *	current object to the specified target object/offset.
 *
 *	Memory system must be locked; will remain so.
 */
void vm_page_rename(mem, new_object, new_offset)
	register vm_page_t	mem;
	register vm_object_t	new_object;
	vm_offset_t		new_offset;
{
	vm_page_t	old_page;

	if (mem->object == new_object)
		return;

    	vm_page_remove(mem);
	vm_page_insert(mem, new_object, new_offset);
}
 
/*
 *	vm_page_alloc:
 *
 *	Allocate and return a memory cell associated
 *	with this VM object/offset pair.
 *
 *	Memory system must be locked for write prior to entry.
 */
vm_page_t vm_page_alloc(object, offset)
	vm_object_t	object;
	vm_offset_t	offset;
{
	register vm_page_t	mem;

	if (queue_empty(&vm_page_queue_free))
		return(VM_PAGE_NULL);

	queue_remove_first(&vm_page_queue_free, mem, vm_page_t, pageq);
	mem->busy = TRUE;
	vm_page_insert(mem, object, offset);
	mem->active = FALSE;
	mem->reclaimable = FALSE;
	mem->wire_count = 0;
	mem->clean = TRUE;
	pmap_clear_modify(VM_PAGE_TO_PHYS(mem));
	vm_page_free_count--;

	/*
	 *	Decide if we should poke the pageout daemon.
	 *	We do this if the free count is less than the low
	 *	water mark, or if the free count is less than the high
	 *	water mark (but above the low water mark) and the reclaim
	 *	count is less that its target.
	 */

	if ((vm_page_free_count < vm_page_free_min) ||
			((vm_page_free_count < vm_page_free_target) &&
			(vm_page_reclaim_count < vm_page_reclaim_target)))
		wakeup(&vm_pages_needed);
	return(mem);
}

/*
 *	vm_page_free:
 *
 *	Returns the given page to the free list,
 *	disassociating it with any VM object.
 *
 *	Memory system must be locked prior to entry.
 */
void vm_page_free(mem)
	register vm_page_t	mem;
{
	vm_page_remove(mem);
	if (mem->active) {
		queue_remove(&vm_page_queue_active, mem, vm_page_t, pageq);
		mem->active = FALSE;
		vm_page_active_count--;
	}

	if (mem->reclaimable) {
		queue_remove(&vm_page_queue_reclaim, mem, vm_page_t, pageq);
		mem->reclaimable = FALSE;
		vm_page_reclaim_count--;
	}

	queue_enter(&vm_page_queue_free, mem, vm_page_t, pageq);
	vm_page_free_count++;
}

/*
 *	vm_page_wire:
 *
 *	Mark this page as wired down by yet
 *	another map, removing it from paging queues
 *	as necessary.  The resident memory system
 *	should be locked on entry and exit.
 */
void vm_page_wire(mem)
	register vm_page_t	mem;
{
	if (mem->wire_count == 0) {
		if (mem->active) {
			queue_remove(&vm_page_queue_active, mem, vm_page_t,
						pageq);
			vm_page_active_count--;
			mem->active = FALSE;
		}
		if (mem->reclaimable) {
			queue_remove(&vm_page_queue_reclaim, mem, vm_page_t,
						pageq);
			vm_page_active_count--;
			mem->reclaimable = FALSE;
		}
		vm_page_wire_count++;
	}
	mem->wire_count++;
}

/*
 *	vm_page_unwire:
 *
 *	Release one wiring of this page, potentially
 *	enabling it to be paged again.
 */
void vm_page_unwire(mem)
	register vm_page_t	mem;
{
	mem->wire_count--;
	if (mem->wire_count == 0) {
		queue_enter(&vm_page_queue_active, mem, vm_page_t, pageq);
		vm_page_active_count++;
		mem->active = TRUE;
		vm_page_wire_count--;
	}
}

/*
 *	vm_page_reclaim:
 *
 *	Returns the given page to the reclaim list,
 *	indicating that no physical maps have access
 *	to this page.  [Used by the physical mapping system.]
 *
 *	Memory system must be locked.
 */
void vm_page_reclaim(m)
	register vm_page_t	m;
{
	/*
	 *	Only move active pages -- ignore locked or already
	 *	reclaimable ones.
	 */

	if (m->active) {
		queue_remove(&vm_page_queue_active, m, vm_page_t, pageq);
		queue_enter(&vm_page_queue_reclaim, m, vm_page_t, pageq);
		m->active = FALSE;
		m->reclaimable = TRUE;
		vm_page_active_count--;
		vm_page_reclaim_count++;
		if (m->clean)
			m->laundry = FALSE;
		else
			m->laundry = TRUE;
	}
}

/*
 *	vm_page_activate:
 *
 *	Put the specified page on the active list (if appropriate).
 *
 *	Memory system must be locked.
 */

void vm_page_activate(m)
	register vm_page_t	m;
{
	if (m->wire_count == 0) {
		queue_enter(&vm_page_queue_active, m, vm_page_t, pageq);
		m->active = TRUE;
		vm_page_active_count++;
	}
}

/*
 *	vm_page_zero_fill:
 *
 *	Zero-fill the specified page.
 *	Written as a standard pagein routine, to
 *	be used by the zero-fill object.
 */

boolean_t vm_page_zero_fill(m)
	vm_page_t	m;
{
	pmap_zero_page(VM_PAGE_TO_PHYS(m));
	return(TRUE);
}

/*
 *	vm_page_copy:
 *
 *	Copy one page to another
 */

void vm_page_copy(src_m, dest_m)
	vm_page_t	src_m;
	vm_page_t	dest_m;
{
	pmap_copy_page(VM_PAGE_TO_PHYS(src_m), VM_PAGE_TO_PHYS(dest_m));
}
