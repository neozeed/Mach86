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
 *	File:	vm/vm_object.c
 *	Author:	Avadis Tevanian, Jr., Michael Wayne Young
 *
 *	Copyright (C) 1985, Avadis Tevanian, Jr., Michael Wayne Young
 *
 *	Virtual memory object module.
 *
 * HISTORY
 *  2-Jun-86  Avadis Tevanian (avie) at Carnegie-Mellon University
 *	Renamed vm_object_copy to vm_object_pmap_copy and
 *	vm_object_remove to vm_object_pmap_remove.  Added
 *	vm_object_page_remove which removes pages from an object's list
 *	of pages (used by the map code for extending a map entry).
 *
 * 21-May-86  David Golub (dbg) at Carnegie-Mellon University
 *	Added vm_object_cache_clear.
 *
 * 15-May-86  David Golub (dbg) at Carnegie-Mellon University
 *	In vm_object_collapse: if all of the pages in the backing object
 *	are also in the parent object, the parent no longer needs to see
 *	the backing object, and can shadow the next one down instead.
 *	Do this when the backing object's reference count is greater
 *	than 1.
 *
 * 13-May-86  David Golub (dbg) at Carnegie-Mellon University
 *	Oops... vm_object_collapse also has to remove the object from
 *	the list of all objects.
 *
 *  2-May-86  David Golub (dbg) at Carnegie-Mellon University
 *	Removed already_locked parameter from vm_object_deallocate.
 *	Cleaned up locking in cache routines.  Made vm_object_collapse
 *	free the backing object itself, so that it can hold the page
 *	system lock.
 *
 * 30-Apr-86  Avadis Tevanian (avie) at Carnegie-Mellon University
 *	Added object cache functions.
 *
 * 20-Jun-85  Avadis Tevanian (avie) at Carnegie-Mellon University
 *	Created.
 */

#include "../h/types.h"

#include "../h/queue.h"
#include "../sync/lock.h"
#include "../vm/vm_page.h"
#include "../vm/vm_map.h"
#include "../vm/vm_param.h"
#include "../vm/vm_object.h"
#include "../h/zalloc.h"

#include "../h/param.h"		/* for PSWP */

/*
 *	Virtual memory objects maintain the actual data
 *	associated with allocated virtual memory.  A given
 *	page of memory exists within exactly one object.
 *
 *	An object is only deallocated when all "references"
 *	are given up.  Only one "reference" to a given
 *	region of an object should be writeable.
 *
 *	Associated with each object is a list of all resident
 *	memory pages belonging to that object; this list is
 *	maintained (and locked) by the "vm_page" module.
 *
 *	Each object also records a "pager" routine which is
 *	used to retrieve (and store) pages to the proper backing
 *	storage.  In addition, objects may be backed by other
 *	objects from which they were virtual-copied.
 *
 *	The only items within the object structure which are
 *	modified after time of creation are:
 *		reference count		locked by object's lock
 *		pager routine		locked by memory system
 *	No locks need be taken prior to calling vm_object routines,
 *	but caller should be sure to be executing at splvm.
 */

struct zone	*vm_object_zone;	/* vm backing store zone */
struct zone	*vm_kobject_zone;	/* vm backing store zone (kernel) */

vm_object_t		vm_object_empty;

extern vm_offset_t	kobject_data;
extern vm_size_t	kobject_data_size;

#define	VM_OBJECT_HASH_COUNT 59

struct vm_object_hash_entry vm_object_hashtable[VM_OBJECT_HASH_COUNT];

long	object_collapses = 0;
long	object_bypasses  = 0;

/*
 *	vm_object_init:
 *
 *	Initialize the VM objects module.
 */
void vm_object_init()
{
	register int	i;

	vm_object_zone = zinit((vm_size_t) sizeof(struct vm_object),
				round_page(256*1024),
				round_page(32*1024),
				FALSE, "objects");
	vm_kobject_zone = zinit((vm_size_t) sizeof(struct vm_object),
				kobject_data_size,
				kobject_data_size,
				FALSE, "kernel objects");

	zcram(vm_kobject_zone, kobject_data, kobject_data_size);

	vm_object_empty = VM_OBJECT_NULL;
	queue_init(&vm_object_list);

	for (i = 0; i < VM_OBJECT_HASH_COUNT; i++) {
		vm_object_hashtable[i].object == VM_OBJECT_NULL;
		vm_object_hashtable[i].pager == vm_pager_null;
	}
}

/*
 *	vm_object_allocate:
 *
 *	Returns a new object with the given size.
 */

vm_object_t vm_kobject_allocate(size)
	vm_size_t	size;
{
	register vm_object_t	result;
	int	s;

	s = splvm();

	result = (vm_object_t) zalloc(vm_kobject_zone);
	_vm_object_allocate(size, result);
	result->is_kernel_object = TRUE;

	splx(s);
	return(result);
}

vm_object_t vm_object_allocate(size)
	vm_size_t	size;
{
	register vm_object_t	result;
	int	s;

	s = splvm();

	result = (vm_object_t) zalloc(vm_object_zone);
	_vm_object_allocate(size, result);
	result->is_kernel_object = FALSE;

	splx(s);
	return(result);
}

_vm_object_allocate(size, object)
	vm_size_t		size;
	register vm_object_t	object;
{
	int			s;

	queue_init(&object->memq);
	simple_lock_init(&object->lock);
	object->ref_count = 1;
	object->size = size;
	object->cluster_size = 1;
	object->pageout_in_progress = FALSE;

	/*
	 *	Object starts out read-write, with no pager.
	 */

	vm_object_setpager(object, vm_pager_null, VM_PAGER_ID_NULL,
			(vm_offset_t) 0, FALSE);
	vm_object_reference(object->shadow = vm_object_empty);
	s = splvm();
	vm_page_lock();
	queue_enter(&vm_object_list, object, vm_object_t, object_list);
	vm_page_unlock();
	splx(s);
}

/*
 *	vm_object_reference:
 *
 *	Gets another reference to the given object.
 */
void vm_object_reference(object)
	register vm_object_t	object;
{
	if (object == VM_OBJECT_NULL)
		return;

	simple_lock(&object->lock);
	object->ref_count++;
	simple_unlock(&object->lock);
}

/*
 *	vm_object_null:
 *
 *	Returns a dummy object, with one reference
 */
vm_object_t vm_object_null()
{
	return(VM_OBJECT_NULL);
}

/*
 *	vm_object_deallocate:
 *
 *	Release a reference to the specified object,
 *	gained either through a vm_object_allocate
 *	or a vm_object_reference call.  When all references
 *	are gone, storage associated with this object
 *	may be relinquished.
 *
 *	The paging system must not be locked.
 */
void vm_object_deallocate(object)
	register vm_object_t	object;
{
	register int		c;
	vm_object_t		temp;

	while (object != VM_OBJECT_NULL) {

		/*
		 *	Lose the reference
		 */

		simple_lock(&object->lock);
		c = --(object->ref_count);
		simple_unlock(&object->lock);

		/*
		 *	If there are no more references...
		 */

		if (c != 0)
			break;

		temp = object->shadow;
		vm_object_terminate(object);
		object = temp;
	}
}

/*
 *	vm_object_terminate actually destroys the specfied object, freeing
 *	up all previously used resources.
 */

void vm_object_terminate(object)
	register vm_object_t	object;
{
	int			s;
	register vm_page_t	p;

	/*
	 *	Wait until the pageout daemon is through
	 *	with the object.
	 */

	s = splvm();
	vm_page_lock();

	while (object->pageout_in_progress) {
		sleep_and_unlock(object, PSWP+2, &vm_page_system_lock);
		vm_page_lock();
	}

	/*
	 *	While the paging system is locked,
	 *	pull the object's pages off the active
	 *	and reclaim queues.  This keeps the
	 *	pageout daemon from playing with them
	 *	during vm_pager_deallocate.
	 *
	 *	We can't free the pages yet, because the
	 *	object's pager may have to write them out
	 *	before deallocating the paging space.
	 */

	p = (vm_page_t) queue_first(&object->memq);
	while (!queue_end(&object->memq, (queue_entry_t) p)) {

		if (p->active) {
			queue_remove(&vm_page_queue_active, p, vm_page_t, pageq);
			p->active = FALSE;
			vm_page_active_count--;
		}

		if (p->reclaimable) {
			queue_remove(&vm_page_queue_reclaim, p, vm_page_t, pageq);
			p->reclaimable = FALSE;
			vm_page_reclaim_count--;
		}
		p = (vm_page_t) queue_next(&p->listq);
	}
				
	vm_page_unlock();
	splx(s);

	/*
	 *	Let the pager know object is dead.
	 */

	vm_pager_deallocate(object->pager, object->paging_space);

	/* DEBUG */
	/* make sure the pager didn't sneak in... */
	if (object->pageout_in_progress) {
		panic("vm_object_deallocate: pageout in progress");
	}
	/* END DEBUG */

	/*
	 *	Free physical page resources.
	 */

	s = splvm();
	vm_page_lock();

	while (!queue_empty(&object->memq)) {
		p = (vm_page_t) queue_first(&object->memq);
		vm_page_free(p);
	}
	queue_remove(&vm_object_list, object, vm_object_t, object_list);

	vm_page_unlock();
	splx(s);

	/*
	 *	Free the space for the object.
	 */

	if (object->is_kernel_object)
		zfree(vm_kobject_zone, (caddr_t) object);
	else
		zfree(vm_object_zone, (caddr_t) object);
}

/*
 *	vm_object_shutdown()
 *
 *	Shut down the object system.  Unfortunately, while we
 *	may be trying to do this, init is happily waiting for
 *	processes to exit, and therefore will be causing some objects
 *	to be deallocated.  To handle this, we gain a fake reference
 *	to all objects we release paging areas for.  This will prevent
 *	a duplicate deallocation.  This routine is probably full of
 *	race conditions!
 */

void vm_object_shutdown()
{
	register vm_object_t	object;
	register int		s;
	register vm_pager_t	pager;
	register vm_pager_id_t	pager_id;

	printf("free paging spaces: ");

	/*
	 *	First we gain a reference to each object so that
	 *	noone else will deallocate them.
	 */

	s = splvm();
	vm_page_lock();
	object = (vm_object_t) queue_first(&vm_object_list);
	while (!queue_end(&vm_object_list, (queue_entry_t) object)) {
		vm_object_reference(object);
		object = (vm_object_t) queue_next(&object->object_list);
	}
	vm_page_unlock();
	splx(s);

	/*
	 *	Now we deallocate all the paging areas.  We don't need
	 *	to lock anything because we have references to everything.
	 *	This assumes of course that noone is creating new objects,
	 *	which should be safe since we are rebooting!
	 */

	object = (vm_object_t) queue_first(&vm_object_list);
	while (!queue_end(&vm_object_list, (queue_entry_t) object)) {
		vm_pager_deallocate(object->pager, object->paging_space);
		object = (vm_object_t) queue_next(&object->object_list);
		printf(".");
	}
	printf("done.\n");
}

/*
 *	vm_object_pmap_copy:
 *
 *	Makes all physical pages in the specified
 *	object range copy-on-write.  No writeable
 *	references to these pages should remain.
 *
 *	The memory system must *not* be locked.
 */
void vm_object_pmap_copy(object, start, end)
	vm_object_t	object;
	vm_offset_t	start;
	vm_offset_t	end;
{
	register vm_page_t	p;
	register vm_offset_t	phys;

	if (object == VM_OBJECT_NULL)
		return;

	vm_page_lock();
	p = (vm_page_t) queue_first(&object->memq);
	while (!queue_end(&object->memq, (queue_entry_t) p)) {
		if ((start <= p->offset) && (p->offset < end)) {
			phys = VM_PAGE_TO_PHYS(p);
			pmap_copy_on_write(phys);
		}
		p = (vm_page_t) queue_next(&p->listq);
	}
	vm_page_unlock();
}

/*
 *	vm_object_pmap_remove:
 *
 *	Removes all physical pages in the specified
 *	object range from all physical maps.
 *
 *	The memory system must *not* be locked.
 */
void vm_object_pmap_remove(object, start, end)
	vm_object_t	object;
	vm_offset_t	start;
	vm_offset_t	end;
{
	vm_page_t		p;
	register vm_offset_t	phys;

	if (object == VM_OBJECT_NULL)
		return;

	vm_page_lock();
	p = (vm_page_t) queue_first(&object->memq);
	while (!queue_end(&object->memq, (queue_entry_t) p)) {
		if ((start <= p->offset) && (p->offset < end)) {
			phys = VM_PAGE_TO_PHYS(p);
			pmap_remove_all(phys);
		}
		p = (vm_page_t) queue_next(&p->listq);
	}
	vm_page_unlock();
}

/*
 *	vm_object_page_remove:
 *
 *	Removes all physical pages in the specified
 *	object range from the object's list of pages.
 *
 *	The memory system must *not* be locked.
 */
void vm_object_page_remove(object, start, end)
	register vm_object_t	object;
	register vm_offset_t	start;
	register vm_offset_t	end;
{
	register vm_page_t	p, next;
	register vm_offset_t	phys;

	if (object == VM_OBJECT_NULL)
		return;

	vm_page_lock();
	p = (vm_page_t) queue_first(&object->memq);
	while (!queue_end(&object->memq, (queue_entry_t) p)) {
		next = (vm_page_t) queue_next(&p->listq);
		if ((start <= p->offset) && (p->offset < end)) {
			phys = VM_PAGE_TO_PHYS(p);
			pmap_remove_all(phys);
			vm_page_free(p);
		}
		p = next;
	}
	vm_page_unlock();
}

/*
 *	vm_object_shadow:
 *
 *	Create a new object which is backed by the
 *	specified existing object range.  The source
 *	object reference is deallocated.
 *
 *	The new object and offset into that object
 *	are returned in the source parameters.
 */

void vm_object_shadow(object, offset, length)
	vm_object_t	*object;	/* IN/OUT */
	vm_offset_t	*offset;	/* IN/OUT */
	vm_size_t	length;
{
	register vm_object_t	source;
	register vm_object_t	result;
	int			usage;

	source = *object;
	if (source == VM_OBJECT_NULL)
		return;

	/*
	 *	Allocate a new object with the given length
	 */

	if ((result = vm_object_allocate(length)) == VM_OBJECT_NULL)
		panic("vm_object_shadow: no object for shadowing");

	/*
	 *	Replace the backing for the new object --
	 *	we must first eliminate the reference to the
	 *	old shadow.
	 */

	vm_object_deallocate(result->shadow);

	/*
	 *	We have to get a new reference to the backing
	 *	object, and then give one up.  [Note that
	 *	this reference is for a new entity.]
	 */

	vm_object_reference(result->shadow = source);
	vm_object_deallocate(source);

	/*
	 *	Store the offset into the source object,
	 *	and fix up the offset into the new object.
	 */

	result->shadow_offset = *offset;
	result->cluster_size = source->cluster_size;	/* XXX here? */

	/*
	 *	Return the new things
	 */

	*offset = 0;
	*object = result;
}

/*
 *	Set the specified object's pager to the specified pager.
 */

void vm_object_setpager(object, pager, paging_space, paging_offset,
			read_only)
	vm_object_t	object;
	vm_pager_t	pager;
	vm_pager_id_t	paging_space;
	vm_offset_t	paging_offset;
	boolean_t	read_only;
{
	object->pager = pager;
	object->paging_space = paging_space;
	object->paging_offset = paging_offset;
	object->read_only = read_only;
}

/*
 *	vm_object_hash hashes the pager/id pair.
 */

#define vm_object_hash(pager,id) \
	(((unsigned)(int)pager+(int)id)%VM_OBJECT_HASH_COUNT)

/*
 *	vm_object_lookup looks in the object cache for an object with the
 *	specified pager and paging id.
 *
 *	Locking is currently done with the page system lock, which is
 *	sufficient, but hardly the best way to do it.
 */

vm_object_t vm_object_lookup(pager, id)
	vm_pager_t	pager;
	vm_pager_id_t	id;
{
	register vm_object_hash_entry_t	entry;
	vm_object_t			object;
	int				s;

	entry = &vm_object_hashtable[vm_object_hash(pager, id)];
	s = splvm();
	vm_page_lock();
	vm_stat.lookups++;
	if ((entry->pager == pager) && (entry->paging_space == id)) {
		object = entry->object;
		vm_stat.hits++;
	}
	else	object = VM_OBJECT_NULL;
	vm_page_unlock();
	splx(s);
	return(object);
}

/*
 *	vm_object_cache caches the specified object.  A reference is added
 *	to the object to prevent deallocation until the object becomes
 *	uncached.
 *
 *	Locking is currently done with the page system lock, which is
 *	sufficient, but hardly the best way to do it.
 */

void vm_object_cache(object, pager, id)
	vm_object_t	object;
	vm_pager_t	pager;
	vm_pager_id_t	id;
{
	register vm_object_hash_entry_t	entry;
	int				s;
	vm_object_t			old_object;

	/*
	 *	We don't cache null objects, and we can't cache
	 *	objects with the null pager.
	 */

	if (object == VM_OBJECT_NULL)
		return;
	if (pager == vm_pager_null)
		return;

	entry = &vm_object_hashtable[vm_object_hash(pager, id)];
	s = splvm();
	vm_page_lock();
	old_object = entry->object;
	entry->object = object;
	entry->pager = pager;
	entry->paging_space = id;
	vm_page_unlock();
	splx(s);
	vm_object_reference(object);

	if (old_object != VM_OBJECT_NULL)
		vm_object_deallocate(old_object);
}

/*
 *	vm_object_uncache uncaches the pager and paging space.
 */

void vm_object_uncache(pager, id)
	vm_pager_t	pager;
	vm_pager_id_t	id;
{
	register vm_object_hash_entry_t	entry;
	int				s;
	vm_object_t			old_object;

	entry = &vm_object_hashtable[vm_object_hash(pager, id)];
	s = splvm();
	vm_page_lock();
	if ((entry->pager == pager) && (entry->paging_space == id)) {
		old_object = entry->object;
		entry->object = VM_OBJECT_NULL;
		entry->pager = vm_pager_null;
		entry->paging_space = VM_PAGER_ID_NULL;
		vm_page_unlock();
		vm_object_deallocate(old_object);
	}
	else {
		vm_page_unlock();
	}
	splx(s);
}

/*
 *	vm_object_cache_clear removes all objects from the cache.
 *
 *	NOTE: the cache is not guaranteed to be empty after this
 *	operation unless its caller locks out all others who can
 *	call vm_object_cache.
 */

void vm_object_cache_clear()
{
	register vm_object_hash_entry_t	entry;
	register int			i, s;
	register vm_object_t		old_object;

	for (i = 0; i < VM_OBJECT_HASH_COUNT; i++) {
		entry = &vm_object_hashtable[i];

		s = splvm();
		vm_page_lock();
		old_object = entry->object;
		entry->object = VM_OBJECT_NULL;
		entry->pager = vm_pager_null;
		entry->paging_space = VM_PAGER_ID_NULL;
		vm_page_unlock();
		splx(s);
		vm_object_deallocate(old_object);
	}
}

/*
 *	vm_object_collapse:
 *
 *	Collapse an object with the object backing it.
 *	Pages in the backing object are moved into the
 *	parent, and the backing object is deallocated.
 *
 *	Requires that the paging system be locked.
 *
 */
void vm_object_collapse(object)
	register vm_object_t	object;

{
	register vm_object_t	backing_object;
	register vm_offset_t	backing_offset;
	register vm_size_t	size;
	register vm_offset_t	new_offset;
	register vm_page_t	p;

	int			r;

	while (TRUE) {
		/*
		 *	Verify that the conditions are right for collapse:
		 *		
		 *		There is a backing object, and
		 */
	
		if ((backing_object = object->shadow) == VM_OBJECT_NULL)
			return;
	
		/*
		 *	...
		 *		The parent object has not paged out.
		 *	...
		 *		The backing object is not read_only.
		 */
	
		if ((object->pager != vm_pager_null) ||
				(backing_object->read_only))
			return;
	
		/*
		 *	...
		 *		That no pages in the backing object
		 *		are currently being paged out.
		 */

		if (backing_object->pageout_in_progress)
			return;

		/*
		 *	We know that we can either collapse the backing
		 *	object (if the parent is the only reference to
		 *	it) or (perhaps) remove the parent's reference
		 *	to it.
		 */

		backing_offset = object->shadow_offset;
		size = object->size;

		/*
		 *	If there is exactly one reference to the backing
		 *	object, we can collapse it into the parent.
		 */
	
		simple_lock(&backing_object->lock);
		r = backing_object->ref_count;
		simple_unlock(&backing_object->lock);
	
		if (r == 1) {

			/*
			 *	We can collapse the backing object.
			 *
			 *	Move all in-memory pages from backing_object
			 *	to the parent.  Pages that have been paged out
			 *	will be overwritten by any of the parent's pages
			 *	that shadow them.
			 */

			while (!queue_empty(&backing_object->memq)) {

				p = (vm_page_t) queue_first(&backing_object->memq);

				new_offset = (p->offset - backing_offset);

				/*
				 *	If the parent has a page here, or if this page
				 *	falls outside the parent, dispose of it.
				 *
				 *	Otherwise, move it as planned.
				 */

				if ((vm_page_lookup(object, new_offset) !=
							VM_PAGE_NULL) ||
				     (p->offset < backing_offset) ||
				     (new_offset > size) )
					vm_page_free(p);
				else
					vm_page_rename(p, object, new_offset);
			}

			/*
			 *	Move the pager from backing_object to object.
			 *
			 *	XXX We're only using part of the paging space for keeps
			 *	now... we ought to discard the unused portion.
			 */

			object->pager = backing_object->pager;
			object->paging_space = backing_object->paging_space;
			object->paging_offset += backing_offset;

			backing_object->pager = vm_pager_null;

			/*
			 *	Object now shadows whatever backing_object did.
			 */

			vm_object_reference(object->shadow = backing_object->shadow);
			object->shadow_offset += backing_object->shadow_offset;

			/*
			 *	Discard backing_object.
			 *
			 *	Since the backing object has no pages and no pager left,
			 *	all that is necessary is to deallocate the shadowed object
			 *	(which will do nothing, since we made another reference to
			 *	it) and to free the space for the backing object.
			 */

			vm_object_deallocate(backing_object->shadow);

			queue_remove(&vm_object_list, backing_object, vm_object_t, object_list);

			if (backing_object->is_kernel_object)
				zfree(vm_kobject_zone, (caddr_t) backing_object);
			else
				zfree(vm_object_zone, (caddr_t) backing_object);

			object_collapses++;
		}

		else {
			/*
			 *	If all of the pages in the backing object are
			 *	shadowed by the parent object, the parent object
			 *	no longer has to shadow the backing object; it
			 *	can shadow the next one in the chain.
			 *
			 *	The backing object must not be paged out - we'd
			 *	have to check all of the paged-out pages, as well.
			 */

			if (backing_object->pager != vm_pager_null)
				return;

			/*
			 *	Should have a check for a 'small' number
			 *	of pages here.
			 */

			p = (vm_page_t) queue_first(&backing_object->memq);
			while (!queue_end(&backing_object->memq, (queue_entry_t) p)) {

				new_offset = (p->offset - backing_offset);

				/*
				 *	If the parent has a page here, or if
				 *	this page falls outside the parent,
				 *	keep going.
				 *
				 *	Otherwise, the backing_object must be
				 *	left in the chain.
				 */

				if (p->offset >= backing_offset &&
				    new_offset <= size &&
				    vm_page_lookup(object, new_offset) ==
						VM_PAGE_NULL) {
					/*
					 *	Page still needed.
					 *	Can't go any further.
					 */
					return;
				}
				p = (vm_page_t) queue_next(&p->listq);
			}

			/*
			 *	Make the parent shadow the next object
			 *	in the chain.  Deallocating backing_object
			 *	will not remove it, since its reference
			 *	count is at least 2.
			 */

			vm_object_reference(object->shadow = backing_object->shadow);
			object->shadow_offset += backing_object->shadow_offset;
			
			vm_object_deallocate(backing_object);

			object_bypasses ++;

		}

		/*
		 *	Try again with this object's new backing object.
		 */
	}
}

/*
 *	vm_object_print:	[ debug ]
 */
void vm_object_print(object)
	vm_object_t	object;
{
	register vm_page_t	p;
	extern indent;

#if	DEBUG_LONGFORM
	if (object)
		iprintf("Backing store object 0x%x.\n", object);
	else {
		iprintf("No backing store object.\n");
		return;
	}
	indent += 2;
	iprintf("Size = 0x%x(%d).\n", object->size, object->size);
	iprintf("Ref count = %d.\n", object->ref_count);
	iprintf("Pager = 0x%x, pager_info = 0x%x.\n", object->pager,
		object->pager_info);
	iprintf("Memq = (next = 0x%x, prev = 0x%x).\n", object->memq.next,
			object->memq.prev);
	iprintf("Resident memory:\n");
	p = (vm_page_t) object->memq.next;
	while (!queue_end(&object->memq, (queue_entry_t) p) {
		indent += 2;
		iprintf("Mem_entry 0x%x (phys = 0x%x).\n", p,
				VM_PAGE_TO_PHYS(p));
		p = (vm_page_t) queue_next(&p->listq);
		indent -= 2;
	}
	indent -= 2;
#else	DEBUG_LONGFORM

	register int count;

	if (object == VM_OBJECT_NULL)
		return;

	iprintf("Object 0x%x: size=0x%x, ref=%d, pager=%d, paging space=0x%x+0x%x, shadow=(0x%x)+0x%x\n",
		(int) object, (int) object->size, object->ref_count, (int) object->pager,
		(int) object->paging_space, (int) object->paging_offset,
		(int) object->shadow, (int) object->shadow_offset);

	indent += 2;

	count = 0;
	p = (vm_page_t) queue_first(&object->memq);
	while (!queue_end(&object->memq, (queue_entry_t) p)) {
		if (count == 0) iprintf("memory:=");
		else if (count == 6) {printf("\n"); iprintf(" ..."); count = 0;}
		else printf(",");
		count++;

		printf("(off=0x%x,page=0x%x)", p->offset, VM_PAGE_TO_PHYS(p));
		p = (vm_page_t) queue_next(&p->listq);
	}
	if (count != 0)
		printf("\n");
	indent -= 2;
#endif	DEBUG_LONGFORM
}
