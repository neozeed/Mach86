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
 *	File:	vm/vm_map.c
 *	Author:	Avadis Tevanian, Jr., Michael Wayne Young
 *
 *	Copyright (C) 1985, Avadis Tevanian, Jr., Michael Wayne Young
 *
 *	Virtual memory mapping module.
 *
 * HISTORY
 * 12-Jun-86  David Golub (dbg) at Carnegie-Mellon University
 *	Fixed vm_map_copy_entry to not collapse a null object.
 *
 *  1-Jun-86  Avadis Tevanian (avie) at Carnegie-Mellon University
 *	Modified vm_map_insert to merge with a previous entry if
 *	possible.
 *
 *  1-Jun-86  Avadis Tevanian (avie) at Carnegie-Mellon University
 *	Modified vm_map_lookup to create an object on the fly when a
 *	fault is taken on an address for which there is no object.
 *
 * 31-May-86  Avadis Tevanian (avie) at Carnegie-Mellon University
 *	Make macros for clip routines.  A couple of calls to the clip
 *	routines had to be "fixed" to allow this to work.
 *
 * 18-Apr-86  David Golub (dbg) at Carnegie-Mellon University
 *	Use vm_fault_wire and vm_fault_unwire to quickly wire or
 *	unwire pages.
 *
 * 17-Dec-85  Michael Young (mwyoung) at Carnegie-Mellon University
 *	Simplified vm_map_copy to merely pmap_protect if the source
 *	is a main map entry (thus not shared).
 *
 * 20-Nov-85  Michael Young (mwyoung) at Carnegie-Mellon University
 *	Simplified maps to use the intermediate map level only when
 *	required.  Fixed sundry bugs.  Completed first cut.
 *
 *  6-Jun-85  Avadis Tevanian (avie) at Carnegie-Mellon University
 *	Created.
 */

#include "../vm/vm_param.h"
#include "../vm/vm_map.h"
#include "../h/zalloc.h"
#include "../h/kern_return.h"
#include "../vm/vm_page.h"

/*
 *	Virtual memory maps provide for the mapping, protection,
 *	and sharing of virtual memory objects.  In addition,
 *	this module provides for an efficient virtual copy of
 *	memory from one map to another.
 *
 *	Synchronization is required prior to most operations.
 *
 *	Maps consist of an ordered doubly-linked list of simple
 *	entries; a single hint is used to speed up lookups.
 *
 *	In order to properly represent the sharing of virtual
 *	memory regions among maps, the map structure is bi-level.
 *	Top-level ("address") maps refer to regions of sharable
 *	virtual memory.  These regions are implemented as
 *	("sharing") maps, which then refer to the actual virtual
 *	memory objects.  When two address maps "share" memory,
 *	their top-level maps both have references to the same
 *	sharing map.  When memory is virtual-copied from one
 *	address map to another, the references in the sharing
 *	maps are actually copied -- no copying occurs at the
 *	virtual memory object level.
 *
 *	Since portions of maps are specified by start/end addreses,
 *	which may not align with existing map entries, all
 *	routines merely "clip" entries to these start/end values.
 *	[That is, an entry is split into two, bordering at a
 *	start or end value.]  Note that these clippings may not
 *	always be necessary (as the two resulting entries are then
 *	not changed); however, the clipping is done for convenience.
 *	No attempt is currently made to "glue back together" two
 *	abutting entries.
 *
 *	As mentioned above, virtual copy operations are performed
 *	by copying VM object references from one sharing map to
 *	another, and then marking both regions as copy-on-write.
 *	It is important to note that only one writeable reference
 *	to a VM object region exists in any map -- this means that
 *	shadow object creation can be delayed until a write operation
 *	occurs.
 */

struct zone	*vm_map_zone;		/* zone for vm_map structures */
struct zone	*vm_map_entry_zone;	/* zone for vm_map_entry structures */
struct zone	*vm_map_kentry_zone;	/* zone for kernel entry structures */

/*
 *	vm_map_init:
 *
 *	Initialize the vm_map module.  Must be called before
 *	any other vm_map routines.
 *
 *	Map and entry structures are allocated from zones -- we must
 *	initialize those zones.
 *
 *	There are three zones of interest:
 *
 *	vm_map_zone:		used to allocate maps.
 *	vm_map_entry_zone:	used to allocate map entries.
 *	vm_map_kentry_zone:	used to allocate map entries for the kernel.
 *
 *	The kernel allocates map entries from a special zone that is initially
 *	"crammed" with memory.  It would be difficult (perhaps impossible) for
 *	the kernel to allocate more memory to a entry zone when it became
 *	empty since the very act of allocating memory implies the creatio
 *	of a new entry.  Further, since the kernel map is created from the
 *	map zone, the map zone is initially "crammed" with enough memory
 *	to fullfill that need.
 */

void vm_map_init()
{
	extern vm_offset_t	map_data, kentry_data;
	extern vm_size_t	map_data_size, kentry_data_size;

	vm_map_zone = zinit((vm_size_t) sizeof(struct vm_map), 40*1024,
					page_size, FALSE, "maps");
	vm_map_entry_zone = zinit((vm_size_t) sizeof(struct vm_map_entry),
					1024*1024, page_size*10,
					TRUE, "pageable map entries");
	vm_map_kentry_zone = zinit((vm_size_t) sizeof(struct vm_map_entry),
					kentry_data_size, kentry_data_size,
					FALSE, "non-pageable map entries");
	/*
	 *	Cram the map and kentry zones with initial data.
	 */
	zcram(vm_map_zone, map_data, map_data_size);
	zcram(vm_map_kentry_zone, kentry_data, kentry_data_size);
}

/*
 *	vm_map_create:
 *
 *	Creates and returns a new empty VM map with
 *	the given physical map structure, and having
 *	the given lower and upper address bounds.
 */
vm_map_t vm_map_create(pmap, min, max, pageable)
	pmap_t		pmap;
	vm_offset_t	min, max;
	boolean_t	pageable;
{
	register vm_map_t	result;

	result = (vm_map_t) zalloc(vm_map_zone);
	if (result == (vm_map_t)NULL)
		panic("vm_map_create: out of maps");

	result->header.next = result->header.prev = &result->header;
	result->nentries = 0;
	result->ref_count = 1;
	result->pmap = pmap;
	result->is_main_map = TRUE;
	result->min_offset = min;
	result->max_offset = max;
	result->entries_pageable = pageable;
	result->hint = &result->header;
	lock_init(&result->lock);
	simple_lock_init(&result->ref_lock);
	simple_lock_init(&result->hint_lock);
	simple_lock_init(&result->pageable_lock);
	return(result);
}

/*
 *	vm_map_entry_create:	[ internal use only ]
 *
 *	Allocates a VM map entry for insertion.
 *	No entry fields are filled in.  This routine is
 */
vm_map_entry_t vm_map_entry_create(map)
	vm_map_t	map;
{
	zone_t		zone;
	vm_map_entry_t	entry;

	if (map->entries_pageable)
		zone = vm_map_entry_zone;
	else
		zone = vm_map_kentry_zone;
	entry = (vm_map_entry_t) zalloc(zone);
	if (entry == (vm_map_entry_t) NULL)
		panic("vm_map_entry_create: out of map entries");

	return(entry);
}

/*
 *	vm_map_entry_dispose:	[ internal use only ]
 *
 *	Inverse of vm_map_entry_create.
 */
void vm_map_entry_dispose(map, entry)
	vm_map_t	map;
	vm_map_entry_t	entry;
{
	if (map->entries_pageable)
		zfree(vm_map_entry_zone, (caddr_t) entry);
	else
		zfree(vm_map_kentry_zone, (caddr_t) entry);
}

/*
 *	vm_map_entry_{un,}link:
 *
 *	Insert/remove entries from maps.
 */
#define	vm_map_entry_link(map, after_where, entry) \
		{ \
		(map)->nentries++; \
		(entry)->prev = (after_where); \
		(entry)->next = (after_where)->next; \
		(entry)->prev->next = (entry); \
		(entry)->next->prev = (entry); \
		}
#define	vm_map_entry_unlink(map, entry) \
		{ \
		(map)->nentries--; \
		(entry)->next->prev = (entry)->prev; \
		(entry)->prev->next = (entry)->next; \
		}

/*
 *	vm_map_reference:
 *
 *	Creates another valid reference to the given map.
 *
 */
void vm_map_reference(map)
	register vm_map_t	map;
{
	simple_lock(&map->ref_lock);
	map->ref_count++;
	simple_unlock(&map->ref_lock);
}

/*
 *	vm_map_deallocate:
 *
 *	Removes a reference from the specified map,
 *	destroying it if no references remain.
 *	The map should not be locked.
 */
void vm_map_deallocate(map)
	register vm_map_t	map;
{
	register int		c;
	int			s;

	s = splvm();
	simple_lock(&map->ref_lock);
	c = --map->ref_count;
	simple_unlock(&map->ref_lock);

	if (c > 0) {
		splx(s);
		return;
	}

	/*
	 *	Lock the map, to wait out all other references
	 *	to it.
	 */

	vm_map_lock(map);

	(void) vm_map_delete(map, map->min_offset, map->max_offset);

	pmap_destroy(map->pmap);

	zfree(vm_map_zone, (caddr_t) map);
	splx(s);
}

/*
 *	vm_map_insert:	[ internal use only ]
 *
 *	Inserts the given whole VM object into the target
 *	map at the specified address range.  The object's
 *	size should match that of the address range.
 *
 *	Requires that the map be locked, and leaves it so.
 */
kern_return_t	vm_map_insert(map, object, offset, start, end)
	vm_map_t	map;
	vm_object_t	object;
	vm_offset_t	offset;
	vm_offset_t	start;
	vm_offset_t	end;
{
	register vm_map_entry_t		new_entry;
	vm_map_entry_t			prev_entry;

	/*
	 *	Check that the start and end points are not bogus.
	 */

	if ((start < map->min_offset) || (end > map->max_offset) ||
			(start >= end))
		return(KERN_INVALID_ADDRESS);

	/*
	 *	Find the entry prior to the proposed
	 *	starting address; if it's part of an
	 *	existing entry, this range is bogus.
	 */

	if (vm_map_lookup_entry(map, start, &prev_entry))
		return(KERN_NO_SPACE);

	/*
	 *	Assert that the next entry doesn't overlap the
	 *	end point.
	 */

	if ((prev_entry->next != &map->header) &&
			(prev_entry->next->start < end))
		return(KERN_NO_SPACE);

	/*
	 *	See if we can avoid creating a new entry by
	 *	extending one of our neighbors.
	 */

	if (object == VM_OBJECT_NULL) {
		if ((prev_entry != &map->header) &&
		    (prev_entry->end == start) &&
		    (prev_entry->is_a_map == FALSE) &&
		    (prev_entry->inheritance == VM_INHERIT_DEFAULT) &&
		    (prev_entry->protection == VM_PROT_DEFAULT) &&
		    (prev_entry->max_protection == VM_PROT_DEFAULT) &&
		    (prev_entry->pageable == TRUE)) {
			register vm_object_t	prev_object;
			int			c;

			prev_object = prev_entry->object.vm_object;
			if (prev_object == VM_OBJECT_NULL) {
				prev_entry->end = end;
				return(KERN_SUCCESS);
			}
			simple_lock(&prev_object->lock);
			c = prev_object->ref_count;
			simple_unlock(&prev_object->lock);
			if ((c == 1) && (prev_object->pager == vm_pager_null)) {
				vm_offset_t	offset, newsize;

				prev_entry->end = end;
				offset = prev_entry->offset;
				/*
				 *	Remove any pages that may be
				 *	still be in the object from
				 *	a previous deallocation.
				 *	[We should probably check if we
				 *	really need to do this first]
				 */
				vm_object_page_remove(prev_object,
					offset + (start - prev_entry->start),
					offset + (end - prev_entry->start));
				/*
				 *	See if the size of the object
				 *	needs to be extended.
				 */
				newsize = offset + (end - prev_entry->start);
				if (newsize > prev_object->size)
					prev_object->size = newsize;
				return(KERN_SUCCESS);
			}
		}
	}

	/*
	 *	Create a new entry
	 */

	new_entry = vm_map_entry_create(map);
	new_entry->start = start;
	new_entry->end = end;

	new_entry->is_a_map = FALSE;
	new_entry->object.vm_object = object;
	new_entry->offset = offset;

	new_entry->copy_on_write = FALSE;
	new_entry->needs_copy = FALSE;

	if (map->is_main_map) {
		new_entry->inheritance = VM_INHERIT_DEFAULT;
		new_entry->protection = VM_PROT_DEFAULT;
		new_entry->max_protection = VM_PROT_DEFAULT;
		new_entry->pageable = TRUE;
	}

	/*
	 *	Insert the new entry into the list
	 */

	vm_map_entry_link(map, prev_entry, new_entry);

	return(KERN_SUCCESS);
}

/*
 *	SAVE_HINT:
 *
 *	Saves the specified entry as the hint for
 *	future lookups.  Performs necessary interlocks.
 */
#define	SAVE_HINT(map,value) \
		simple_lock(&(map)->hint_lock); \
		(map)->hint = (value); \
		simple_unlock(&(map)->hint_lock);

/*
 *	vm_map_lookup_entry:	[ internal use only ]
 *
 *	Finds the map entry containing (or
 *	immediately preceding) the specified address
 *	in the given map; the entry is returned
 *	in the "entry" parameter.  The boolean
 *	result indicates whether the address is
 *	actually contained in the map.
 */
boolean_t vm_map_lookup_entry(map, address, entry)
	register vm_map_t	map;
	register vm_offset_t	address;
	vm_map_entry_t		*entry;		/* OUT */
{
	register vm_map_entry_t		cur;
	register vm_map_entry_t		last;

	/*
	 *	Start looking either from the head of the
	 *	list, or from the hint.
	 */

	simple_lock(&map->hint_lock);
	cur = map->hint;
	simple_unlock(&map->hint_lock);

	if (cur == &map->header)
		cur = cur->next;

	if (address >= cur->start) {
	    	/*
		 *	Go from hint to end of list.
		 *
		 *	But first, make a quick check to see if
		 *	we are already looking at the entry we
		 *	want (which is usually the case).
		 *	Note also that we don't need to save the hint
		 *	here... it is the same hint (unless we are
		 *	at the header, in which case the hint didn't
		 *	buy us anything anyway).
		 */
		last = &map->header;
		if ((cur != last) && (cur->end > address)) {
			*entry = cur;
			return(TRUE);
		}
	}
	else {
	    	/*
		 *	Go from start to hint, *inclusively*
		 */
		last = cur->next;
		cur = map->header.next;
	}

	/*
	 *	Search linearly
	 */

	while (cur != last) {
		if (cur->end > address) {
			if (address >= cur->start) {
			    	/*
				 *	Save this lookup for future
				 *	hints, and return
				 */

				*entry = cur;
				SAVE_HINT(map, cur);
				return(TRUE);
			}
			break;
		}
		cur = cur->next;
	}
	*entry = cur->prev;
	SAVE_HINT(map, *entry);
	return(FALSE);
}

/*
 *	vm_map_find finds an unallocated region in the target address
 *	map with the given length.  The search is defined to be
 *	first-fit from the specified address; the region found is
 *	returned in the same parameter.
 *
 */
kern_return_t	vm_map_find(map, object, offset, addr, length, find_space)
	vm_map_t	map;
	vm_object_t	object;
	vm_offset_t	offset;
	vm_offset_t	*addr;		/* IN/OUT */
	vm_size_t	length;
	boolean_t	find_space;
{
	register vm_map_entry_t	entry;
	vm_map_entry_t		tmp_entry;
	register vm_offset_t	start;
	register vm_offset_t	end;
	kern_return_t		result;
	int			s;

#if	0
#define round_up(addr,mask) (vm_offset_t) (((addr)+~(mask))&(mask))
#endif	0

#define	NO_SPACE \
	{ vm_map_unlock(map); splx(s); return (KERN_NO_SPACE);}

	s = splvm();

#if	0
	start = round_up(*addr, mask);
#endif
	start = *addr;

	vm_map_lock(map);

	if (find_space) {
		/*
		 *	Calculate the first possible address.
		 */

		if (start < map->min_offset)
			start = map->min_offset;
		if (start > map->max_offset)
			NO_SPACE;

#if	0
		start = round_up(start, mask);
#endif

		/*
		 *	Look for the first possible address;
		 *	if there's already something at this
		 *	address, we have to start after it.
		 */

		if (vm_map_lookup_entry(map, start, &tmp_entry))
#if	0
			start = round_up(tmp_entry->end, mask);
#endif
			start = tmp_entry->end;
		entry = tmp_entry;

		/*
		 *	In any case, the "entry" always precedes
		 *	the proposed new region throughout the
		 *	loop:
		 */

		while (TRUE) {
			register vm_map_entry_t	next;

		    	/*
			 *	Find the end of the proposed new region
			 */

			end = start + length;
			if (end > map->max_offset)
				NO_SPACE;

			/*
			 *	Did the proposed region "wrap around"?
			 *	If so, we've run out of places to look.
			 */

			if (end < start)
				NO_SPACE;

			/*
			 *	If there are no more entries, we must win.
			 */

			next = entry->next;
			if (next == &map->header)
				break;

			/*
			 *	If there is another entry, it must be
			 *	after the end of the potential new region.
			 */

			if (next->start >= end)
				break;

			/*
			 *	Didn't fit -- move to the next entry.
			 */

			entry = next;
#if	0
			start = round_up(entry->end, mask);
#endif	0
			start = entry->end;
		}
		*addr = start;
	}

	result = vm_map_insert(map, object, offset, start, start + length);
	
	vm_map_unlock(map);

	splx(s);
	return(result);

#undef	round_up
#undef	NO_SPACE
}

/*
 *	vm_map_simplify_entry:	[ internal use only ]
 *
 *	Simplify the given map entry by:
 *		removing extra sharing maps
 *		[XXX maybe later] merging with a neighbor
 */
void vm_map_simplify_entry(map, entry)
	vm_map_t	map;
	vm_map_entry_t	entry;
{
	/*
	 *	If this entry corresponds to a sharing map, then
	 *	see if we can remove the level of indirection.
	 *	If it's not a sharing map, then it points to
	 *	a VM object, so see if we can merge with either
	 *	of our neighbors.
	 */

	if (entry->is_a_map) {
		vm_map_t	my_share_map;
		int		count;

#if	0
		my_share_map = entry->object.share_map;	
		simple_lock(&my_share_map->ref_lock);
		count = my_share_map->ref_count;
		simple_unlock(&my_share_map->ref_lock);
		
		if (count == 1) {
			/* Can move the region from
			 * entry->start to entry->end (+ entry->offset)
			 * in my_share_map into place of entry.
			 * Later.
			 */
		}
#endif	0
	}
	else {
		/*
		 *	Try to merge with our neighbors.
		 *
		 *	Conditions for merge are:
		 *
		 *	1.  entries are adjacent.
		 *	2.  both entries point to objects
		 *	    with null pagers.
		 *
		 * 	If a merge is possible, we replace the two
		 *	entries with a single entry, then merge
		 *	the two objects into a single object.
		 *
		 *	Now, all that is left to do is write the
		 *	code!
		 */
	}
}

/*
 *	VM_MAP_RANGE_CHECK:	[ internal use only ]
 *
 *	Asserts that the starting and ending region
 *	addresses fall within the valid range of the map.
 */
#define	VM_MAP_RANGE_CHECK(map, start, end)		\
		{					\
		if (start < vm_map_min(map))		\
			start = vm_map_min(map);	\
		if (end > vm_map_max(map))		\
			end = vm_map_max(map);		\
		if (start > end)			\
			start = end;			\
		}

#define vm_map_clip_start(map, entry, startaddr) \
{ \
	if (startaddr > entry->start) \
		_vm_map_clip_start(map, entry, startaddr); \
}

/*
 *	vm_map_clip_start:	[ internal use only ]
 *
 *	Asserts that the given entry begins at or after
 *	the specified address; if necessary,
 *	it splits the entry into two.
 */
void _vm_map_clip_start(map, entry, start)
	vm_map_t	map;
	vm_map_entry_t	entry;
	vm_offset_t	start;
{
	if (start > entry->start) {
		register vm_map_entry_t	new_entry;

		/*
		 *	See if we can simplify this entry first
		 */
		 
		vm_map_simplify_entry(map, entry);

		/*
		 *	Split off the front portion --
		 *	note that we must insert the new
		 *	entry BEFORE this one, so that
		 *	this entry has the specified starting
		 *	address.
		 */

		new_entry = vm_map_entry_create(map);
		*new_entry = *entry;

		new_entry->end = start;
		entry->offset += (start - entry->start);
		entry->start = start;

		vm_map_entry_link(map, entry->prev, new_entry);

		if (entry->is_a_map)
		 	vm_map_reference(new_entry->object.share_map);
		else
			vm_object_reference(new_entry->object.vm_object);
	}
}

#define vm_map_clip_end(map, entry, endaddr) \
{ \
	if (endaddr < entry->end) \
		_vm_map_clip_end(map, entry, endaddr); \
}

/*
 *	vm_map_clip_end:	[ internal use only ]
 *
 *	Asserts that the given entry ends at or before
 *	the specified address; if necessary,
 *	it splits the entry into two.
 */
void _vm_map_clip_end(map, entry, end)
	vm_map_t	map;
	vm_map_entry_t	entry;
	vm_offset_t	end;
{
	if (end < entry->end) {
		register vm_map_entry_t	new_entry;

		/*
		 *	Create a new entry and insert it
		 *	AFTER the specified entry
		 */

		new_entry = vm_map_entry_create(map);
		*new_entry = *entry;

		new_entry->start = entry->end = end;
		new_entry->offset += (end - entry->start);

		vm_map_entry_link(map, entry, new_entry);

		if (entry->is_a_map)
		 	vm_map_reference(new_entry->object.share_map);
		else
			vm_object_reference(new_entry->object.vm_object);
	}
}

/*
 *	vm_map_protect:
 *
 *	Sets the protection of the specified address
 *	region in the target map.  If "set_max" is
 *	specified, the maximum protection is to be set;
 *	otherwise, only the current protection is affected.
 */
kern_return_t vm_map_protect(map, start, end, new_prot, set_max)
	vm_map_t	map;
	vm_offset_t	start;
	register
	vm_offset_t	end;
	vm_prot_t	new_prot;
	boolean_t	set_max;
{
	register vm_map_entry_t		current;
	vm_map_entry_t	entry;
	int		s;

	s = splvm();
	vm_map_lock(map);

	VM_MAP_RANGE_CHECK(map, start, end);

	if (!vm_map_lookup_entry(map, start, &entry))
		entry = entry->next;

	/*
	 *	Make a first pass to check for protection
	 *	violations.
	 */

	current = entry;
	while ((current != &map->header) && (current->start < end)) {
		if ((new_prot & current->max_protection) != new_prot) {
			vm_map_unlock(map);
			splx(s);
			return(KERN_PROTECTION_FAILURE);
		}

		current = current->next;
	}

	/*
	 *	Go back and fix up protections.
	 *	[Note that clipping is not necessary the second time.]
	 */

	current = entry;
	vm_map_clip_start(map, current, start);

	while ((current != &map->header) && (current->start < end)) {
		vm_prot_t	old_prot;

		vm_map_clip_end(map, current, end);

		old_prot = current->protection;
		if (set_max)
			current->protection =
				(current->max_protection = new_prot) &
					old_prot;
		else
			current->protection = new_prot;

		/*
		 *	Update physical map if necessary.
		 *	Worry about copy-on-write here -- CHECK THIS XXX
		 */

		if (current->protection != old_prot) {

#define MASK(entry)	((entry)->copy_on_write ? ~VM_PROT_WRITE : \
							VM_PROT_ALL)
#define	max(a,b)	((a) > (b) ? (a) : (b))

			if (current->is_a_map) {
				vm_map_entry_t	share_entry;
				vm_offset_t	share_end;

				vm_map_lock(current->object.share_map);
				(void) vm_map_lookup_entry(
						current->object.share_map,
						current->offset,
						&share_entry);
				share_end = current->offset +
					(current->end - current->start);
				while ((share_entry !=
					&current->object.share_map->header) &&
					(share_entry->start < share_end)) {

					pmap_protect(map->pmap,
						(max(share_entry->start,
							current->offset) -
							current->offset +
							current->start),
						max(share_entry->end,
							share_end) -
						current->offset +
						current->start,
						current->protection &
							MASK(share_entry));

					share_entry = share_entry->next;
				}
				vm_map_unlock(current->object.share_map);
			}
			else
			 	pmap_protect(map->pmap, current->start,
					current->end,
					current->protection & MASK(entry));
#undef	max
#undef	MASK
		}
		current = current->next;
	}

	vm_map_unlock(map);
	splx(s);
	return(KERN_SUCCESS);
}

/*
 *	vm_map_inherit:
 *
 *	Sets the inheritance of the specified address
 *	range in the target map.  Inheritance
 *	affects how the map will be shared with
 *	child maps at the time of vm_map_fork.
 */
kern_return_t vm_map_inherit(map, start, end, new_inheritance)
	vm_map_t		map;
	vm_offset_t		start;
	register vm_offset_t	end;
	vm_inherit_t		new_inheritance;
{
	register vm_map_entry_t	entry;
	vm_map_entry_t	temp_entry;
	int		s;

	switch (new_inheritance) {
	case VM_INHERIT_NONE:
	case VM_INHERIT_COPY:
	case VM_INHERIT_SHARE:
		break;
	default:
		return(KERN_INVALID_ARGUMENT);
	}

	s = splvm();
	vm_map_lock(map);

	VM_MAP_RANGE_CHECK(map, start, end);

	if (vm_map_lookup_entry(map, start, &temp_entry)) {
		entry = temp_entry;
		vm_map_clip_start(map, entry, start);
	}
	else
		entry = temp_entry->next;

	while ((entry != &map->header) && (entry->start < end)) {
		vm_map_clip_end(map, entry, end);

		entry->inheritance = new_inheritance;

		entry = entry->next;
	}

	vm_map_unlock(map);
	splx(s);
	return(KERN_SUCCESS);
}

/*
 *	vm_map_pageable:
 *
 *	Sets the pageability of the specified address
 *	range in the target map.  Regions specified
 *	as not pageable require locked-down physical
 *	memory and physical page maps.
 *
 *	The map must not be locked, but a reference
 *	must remain to the map throughout the call.
 */
kern_return_t vm_map_pageable(map, start, end, new_pageable)
	vm_map_t		map;
	vm_offset_t		start;
	register vm_offset_t	end;
	boolean_t		new_pageable;
{
	register vm_map_entry_t	entry;
	vm_map_entry_t		temp_entry;
	int			s;

	/*
	 *	Only one pageability change may take place at one
	 *	time, since vm_fault assumes it will be called
	 *	only once for each wiring/unwiring.
	 *
	 *	Unfortunately, we can't hold the map's lock
	 *	during the vm_fault loop, so we have to hold
	 *	a different lock.  Fortunately, it can be on a per-map
	 *	basis.
	 */

	s = splvm();
	simple_lock(&map->pageable_lock);
	vm_map_lock(map);

	VM_MAP_RANGE_CHECK(map, start, end);

	/*
	 *	For the same reason (vm_fault must be called once
	 *	per wiring/unwiring), we have to make sure we're
	 *	actually changing the pageability for the entire
	 *	region.  We do so before making any changes.
	 */

	if (vm_map_lookup_entry(map, start, &temp_entry)) {
		entry = temp_entry;
		vm_map_clip_start(map, entry, start);
	}
	else
		entry = temp_entry->next;
	temp_entry = entry;

	while ((entry != &map->header) && (entry->start < end)) {
	    	if (entry->pageable == new_pageable) {
			vm_map_unlock(map);
			simple_unlock(&map->pageable_lock);
			return(KERN_INVALID_ARGUMENT);
		}
		entry = entry->next;
	}

	/*
	 *	The request checks out OK.  Mark the changes.
	 */

	entry = temp_entry;
	while ((entry != &map->header) && (entry->start < end)) {
		vm_map_clip_end(map, entry, end);

		entry->pageable = new_pageable;
		entry = entry->next;
	}

	vm_map_unlock(map);

	if (new_pageable)
		vm_fault_unwire(map, start, end);
	else
		vm_fault_wire(map, start, end);

	simple_unlock(&map->pageable_lock);
	splx(s);

	return(KERN_SUCCESS);
}

/*
 *	vm_map_entry_unwire:	[ internal use only ]
 *
 *	Make the region specified by this entry pageable.
 *
 *	The map in question should be locked.
 *	[This is the reason for this routine's existence.]
 */
void vm_map_entry_unwire(map, entry)
	vm_map_t	map;
	vm_map_entry_t	entry;
{
	vm_fault_unwire(map, entry->start, entry->end);
	entry->pageable = TRUE;
}

/*
 *	vm_map_entry_delete:	[ internal use only ]
 *
 *	Deallocate the given entry from the target map.
 */		
void vm_map_entry_delete(map, entry)
	vm_map_t		map;
	register vm_map_entry_t	entry;
{
	if (!entry->pageable)
		vm_map_entry_unwire(map, entry);
		
	vm_map_entry_unlink(map, entry);

	if (entry->is_a_map)
		vm_map_deallocate(entry->object.share_map);
	else
	 	vm_object_deallocate(entry->object.vm_object);

	vm_map_entry_dispose(map, entry);
}

/*
 *	vm_map_delete:	[ internal use only ]
 *
 *	Deallocates the given address range from the target
 *	map.
 *
 *	When called with a sharing map, removes pages from
 *	that region from all physical maps.
 */
kern_return_t vm_map_delete(map, start, end)
	vm_map_t		map;
	vm_offset_t		start;
	register vm_offset_t	end;
{
	register vm_map_entry_t	entry;
	vm_map_entry_t		first_entry;

	/*
	 *	Find the start of the region, and clip it
	 */

	if (!vm_map_lookup_entry(map, start, &first_entry))
		entry = first_entry->next;
	else {
		entry = first_entry;
		vm_map_clip_start(map, entry, start);

		/*
		 *	Fix the lookup hint now, rather than each
		 *	time though the loop.
		 */

		SAVE_HINT(map, entry->prev);
	}

	/*
	 *	Step through all entries in this region
	 */

	while ((entry != &map->header) && (entry->start < end)) {
		vm_map_entry_t		next;
		register vm_offset_t	s, e;

		vm_map_clip_end(map, entry, end);

		next = entry->next;
		s = entry->start;
		e = entry->end;

		/*
		 *	Unwire before removing addresses from the pmap;
		 *	otherwise, unwiring will put the entries back in
		 *	the pmap.
		 */

		if (!entry->pageable)
			vm_map_entry_unwire(map, entry);
		
		/*
		 *	If this is a sharing map, we must remove
		 *	*all* references to this data, since we can't
		 *	find all of the physical maps which are sharing
		 *	it.
		 */

		if (!map->is_main_map)
			vm_object_pmap_remove(entry->object.vm_object,
					 entry->offset,
					 entry->offset + (e - s));

		pmap_remove(map->pmap, s, e);

		/*
		 *	Delete the entry (which may delete the object)
		 *	only after removing all pmap entries pointing
		 *	to its pages.  (Otherwise, its page frames may
		 *	be reallocated, and any modify bits will be
		 *	set in the wrong object!)
		 */

		vm_map_entry_delete(map, entry);
		entry = next;
	}
	return(KERN_SUCCESS);
}

/*
 *	vm_map_remove:
 *
 *	Remove the given address range from the target map.
 *	This is the exported form of vm_map_delete.
 */
kern_return_t	vm_map_remove(map, start, end)
	vm_map_t	map;
	vm_offset_t	start;
	vm_offset_t	end;
{
	kern_return_t	result;
	int		s;

	s = splvm();
	vm_map_lock(map);
	VM_MAP_RANGE_CHECK(map, start, end);
	result = vm_map_delete(map, start, end);
	vm_map_unlock(map);
	splx(s);
	return(result);
}

/*
 *	vm_map_check_protection:
 *
 *	Assert that the target map allows the specified
 *	privilege on the entire address region given.
 *	The entire region must be allocated.
 */
boolean_t vm_map_check_protection(map, start, end, protection)
	vm_map_t		map;
	register vm_offset_t	start;
	register vm_offset_t	end;
	register vm_prot_t	protection;
{
	register vm_map_entry_t	entry;
	vm_map_entry_t		tmp_entry;
	int			s;

	s = splvm();
	if (!vm_map_lookup_entry(map, start, &tmp_entry)) {
		splx(s);
		return(FALSE);
	}

	entry = tmp_entry;

	while (start < end) {
		if (entry == &map->header) {
			splx(s);
			return(FALSE);
		}

		/*
		 *	No holes allowed!
		 */

		if (start < entry->start) {
			splx(s);
			return(FALSE);
		}

		/*
		 * Check protection associated with entry.
		 */

		if ((entry->protection & protection) != protection) {
			splx(s);
			return(FALSE);
		}

		/* go to next entry */

		start = entry->end;
		entry = entry->next;
	}
	splx(s);
	return(TRUE);
}

/*
 *	vm_map_copy:
 *
 *	Perform a virtual memory copy from the source
 *	address map/range to the destination map/range.
 *
 *	Both maps must be locked or exclusive-use.
 *	Interrupt priority level must be at splvm or higher.
 *
 *	If src_destroy or dst_alloc is requested,
 *	the source and destination regions should be
 *	disjoint, not only in the top-level map, but
 *	in the sharing maps as well.  [The best way
 *	to guarantee this is to use a new intermediate
 *	map to make copies.  This also reduces map
 *	fragmentation.]
 */
kern_return_t vm_map_copy(dst_map, src_map,
			  dst_addr, len, src_addr,
			  dst_alloc, src_destroy)
	vm_map_t	dst_map;
	vm_map_t	src_map;
	vm_offset_t	dst_addr;
	vm_size_t	len;
	vm_offset_t	src_addr;
	boolean_t	dst_alloc;
	boolean_t	src_destroy;
{
	register vm_map_entry_t		src_entry;
	register vm_map_entry_t		dst_entry;
	vm_map_entry_t			tmp_entry;
	vm_offset_t			src_start;
	vm_offset_t			src_end;
	vm_offset_t			dst_start;
	vm_offset_t			dst_end;
	vm_offset_t			src_clip;
	vm_offset_t			dst_clip;
	int				s;
	kern_return_t			result;

	/*
	 *	Compute start and end of region in both maps
	 */

	src_start = src_addr;
	src_end = src_start + len;
	dst_start = dst_addr;
	dst_end = dst_start + len;

	/*
	 *	Check that the region can exist in both source
	 *	and destination.
	 */

	if ((dst_end < dst_start) || (src_end < src_start))
		return(KERN_NO_SPACE);

	/*
	 *	Lock the maps in question -- we avoid deadlock
	 *	by ordering lock acquisition by map value
	 */

	s = splvm();
	if (src_map == dst_map)
		vm_map_lock(src_map);
	else if ((int) src_map < (int) dst_map) {
	 	vm_map_lock(src_map);
		vm_map_lock(dst_map);
	} else {
		vm_map_lock(dst_map);
	 	vm_map_lock(src_map);
	}

	result = KERN_SUCCESS;

	/*
	 *	Check protections... source must be completely readable and
	 *	destination must be completely writable.  [Note that if we're
	 *	allocating the destination region, we don't have to worry
	 *	about protection, but instead about whether the region
	 *	exists.]
	 */

	if (src_map->is_main_map && dst_map->is_main_map) {
		if (!vm_map_check_protection(src_map, src_start, src_end,
					VM_PROT_READ)) {
			result = KERN_PROTECTION_FAILURE;
			goto Return;
		}

		if (dst_alloc) {
			/* XXX Consider making this a vm_map_find instead */
			if ((result = vm_map_insert(dst_map, vm_object_null(),
					(vm_offset_t) 0, dst_start, dst_end)) != KERN_SUCCESS)
				goto Return;
		}
		else if (!vm_map_check_protection(dst_map, dst_start, dst_end,
					VM_PROT_WRITE)) {
			result = KERN_PROTECTION_FAILURE;
			goto Return;
		}
	}

	/*
	 *	Find the start entries and clip.
	 *
	 *	Note that checking protection asserts that the
	 *	lookup cannot fail.
	 *
	 *	Also note that we wait to do the second lookup
	 *	until we have done the first clip, as the clip
	 *	may affect which entry we get!
	 */

	(void) vm_map_lookup_entry(src_map, src_addr, &tmp_entry);
	src_entry = tmp_entry;
	vm_map_clip_start(src_map, src_entry, src_start);
	(void) vm_map_lookup_entry(dst_map, dst_addr, &tmp_entry);
	dst_entry = tmp_entry;
	vm_map_clip_start(dst_map, dst_entry, dst_start);

	/*
	 *	If both source and destination entries are the same,
	 *	a null copy is being performed.
	 */

	if (src_entry == dst_entry)
		goto Return;

	/*
	 *	Go through entries until we get to the end of the
	 *	region.
	 */

	while (src_start < src_end) {
		/*
		 *	Clip the entries to the endpoint of the entire region.
		 */

		vm_map_clip_end(src_map, src_entry, src_end);
		vm_map_clip_end(dst_map, dst_entry, dst_end);

		/*
		 *	Clip each entry to the endpoint of the other entry.
		 */

		src_clip = src_entry->start +
					(dst_entry->end - dst_entry->start);
		vm_map_clip_end(src_map, src_entry, src_clip);
		dst_clip = dst_entry->start +
					(src_entry->end - src_entry->start);
		vm_map_clip_end(dst_map, dst_entry, dst_clip);

		/*
		 *	Both entries now match in size and relative endpoints.
		 *
		 *	If both entries refer to a VM object, we can
		 *	deal with them now.
		 */

		if (!src_entry->is_a_map && !dst_entry->is_a_map) {
			vm_map_copy_entry(src_map, dst_map, src_entry,
						dst_entry);
		}
		else {
			register vm_map_t	new_dst_map;
			vm_offset_t		new_dst_start;
			vm_size_t		new_size;
			vm_map_t		new_src_map;
			vm_offset_t		new_src_start;

			/*
			 *	We have to follow at least one sharing map.
			 */

			new_size = (dst_entry->end - dst_entry->start);

			if (src_entry->is_a_map) {
				new_src_map = src_entry->object.share_map;
				new_src_start = src_entry->offset;
			}
			else {
			 	new_src_map = src_map;
				new_src_start = src_entry->start;
			}

			if (dst_entry->is_a_map) {
			    	vm_offset_t	new_dst_end;

				new_dst_map = dst_entry->object.share_map;
				new_dst_start = dst_entry->offset;

				/*
				 *	Since the destination sharing entries
				 *	will be merely deallocated, we can
				 *	do that now, and replace the region
				 *	with a null object.  [This prevents
				 *	splitting the source map to match
				 *	the form of the destination map.]
				 *	Note that we can only do so if the
				 *	source and destination do not overlap.
				 */

				new_dst_end = new_dst_start + new_size;

				if (new_dst_map != new_src_map) {
					vm_map_lock(new_dst_map);
					(void) vm_map_delete(new_dst_map,
							new_dst_start,
							new_dst_end);
					(void) vm_map_insert(new_dst_map,
							vm_object_null(),
							(vm_offset_t) 0,
							new_dst_start,
							new_dst_end);
					vm_map_unlock(new_dst_map);
				}
			}
			else {
			 	new_dst_map = dst_map;
				new_dst_start = dst_entry->start;
			}

			/*
			 *	Recursively copy the sharing map.
			 */

			(void) vm_map_copy(new_dst_map, new_src_map,
				new_dst_start, new_size, new_src_start,
				FALSE, TRUE);

		}

		/*
		 *	Update variables for next pass through the loop.
		 */

		src_start = src_entry->end;
		src_entry = src_entry->next;
		dst_start = dst_entry->end;
		dst_entry = dst_entry->next;

		/*
		 *	If the source is to be destroyed, here is the
		 *	place to do it.
		 */

		if (src_destroy && src_map->is_main_map &&
						dst_map->is_main_map)
			vm_map_entry_delete(src_map, src_entry->prev);
	}

	/*
	 *	Update the physical maps as appropriate
	 */

	if (src_map->is_main_map && dst_map->is_main_map) {
		if (src_destroy)
			pmap_remove(src_map->pmap, src_addr, src_addr + len);
	}

	/*
	 *	Unlock the maps
	 */

	Return: ;

	vm_map_unlock(src_map);
	if (src_map != dst_map)
		vm_map_unlock(dst_map);
	splx(s);

	return(result);
}

/*
 *	vm_map_copy_entry:
 *
 *	Copies the contents of the source entry to the destination
 *	entry.  The entries *must* be aligned properly.
 */
vm_map_copy_entry(src_map, dst_map, src_entry, dst_entry)
	vm_map_t		src_map, dst_map;
	register vm_map_entry_t	src_entry, dst_entry;
{
	vm_object_t	temp_object;
	boolean_t	su;

	/*
	 *	Check to see if the object can be collapsed.
	 */

	if (src_entry->object.vm_object) {
		vm_page_lock();
		vm_object_collapse(src_entry->object.vm_object);
		vm_page_unlock();
	}

	/*
	 *	If our destination map was wired down,
	 *	unwire it now.
	 */

	if (!dst_entry->pageable)
		vm_map_entry_unwire(dst_map, dst_entry);

	/*
	 *	If we're dealing with a sharing map, we
	 *	must remove the destination pages from
	 *	all maps (since we cannot know which maps
	 *	this sharing map belongs in).
	 */

	if (!dst_map->is_main_map)
		vm_object_pmap_remove(dst_entry->object.vm_object,
				 dst_entry->offset,
				 dst_entry->offset +
					(dst_entry->end -
					dst_entry->start));

	/*
	 *	See whether this is the only
	 *	reference to this data by a main map.
	 */

	if (!(su = src_map->is_main_map)) {
		simple_lock(&src_map->ref_lock);
		su = (src_map->ref_count == 1);
		simple_unlock(&src_map->ref_lock);
	}

	/*
	 *	If this region is already copy-on-write,
	 *	protections are already fixed up.
	 *
	 *	Of course, wired down pages can't be set copy-on-write.
	 *	Wired pages are copied into the new map below.
	 *	(This doesn't handle the src_destroy option optimally).
	 */

	if ((!src_entry->needs_copy) && (src_entry->pageable)) {
		/*
		 *	If this is the only main map,
		 *	we can either reprotect just this
		 *	physical map, or give the region
		 *	away entirely.
		 */

		if (su) {
/*			if (!src_destroy)*/
				pmap_protect(src_map->pmap,
					src_entry->start,
					src_entry->end,
					src_entry->protection & ~VM_PROT_WRITE);
/*			else
			 	goto give_away;*/
		}
		else
			/*
			 *	This object is shared by more than
			 *	one main map.  We have to track the
			 *	resident pages down from the object.
			 */

			vm_object_pmap_copy(src_entry->object.vm_object,
				src_entry->offset,
				src_entry->offset +
					(src_entry->end -
					src_entry->start));

		/*
		 *	Mark the source as needing a copy.
		 */

		src_entry->needs_copy = TRUE;
		src_entry->copy_on_write = TRUE;
					
		give_away: ;
	}

	if (!src_entry->pageable) {
		dst_entry->needs_copy = TRUE;
	}
	else {
		dst_entry->needs_copy = src_entry->needs_copy;
		dst_entry->copy_on_write = src_entry->copy_on_write;
	}

	/*
	 *	Copy the source object/offset into the
	 *	destination entry, making appropriate
	 *	references.
	 */

	temp_object = dst_entry->object.vm_object;
	vm_object_reference(dst_entry->object.vm_object =
				src_entry->object.vm_object);
	vm_object_deallocate(temp_object);
	dst_entry->offset = src_entry->offset;

	pmap_copy(dst_map->pmap, src_map->pmap, dst_entry->start,
			dst_entry->end - dst_entry->start, src_entry->start);

	/*
	 *	Cause wired pages to be copied into the new
	 *	map by simulating faults (the new pages are
	 *	pageable)
	 */

	if (!src_entry->pageable) {
		register vm_offset_t	start, end;

		start = dst_entry->start;
		end = dst_entry->end;
		while (start < end) {
			vm_fault(dst_map, start, dst_entry->max_protection,
					FALSE);
			start += PAGE_SIZE;
		}

		/*
		 *	The destination no longer needs to shadow the source
		 *	since all pages have been copied from the source.
		 */

		temp_object = dst_entry->object.vm_object->shadow;
		dst_entry->object.vm_object->shadow = VM_OBJECT_NULL;
		dst_entry->object.vm_object->shadow_offset = 0;
		vm_object_deallocate(temp_object);
	}
}

/*
 *	vm_map_fork:
 *
 *	Create and return a new map based on the old
 *	map, according to the inheritance values on the
 *	regions in that map.
 *
 *	The source map must not be locked.
 */
vm_map_t vm_map_fork(old_map)
	vm_map_t	old_map;
{
	vm_map_t	new_map;
	vm_map_entry_t	old_entry;
	vm_map_entry_t	new_entry;
	int		s;

	s = splvm();
	vm_map_lock(old_map);

	new_map = vm_map_create(pmap_create((vm_size_t) 0),
			old_map->min_offset,
			old_map->max_offset);
	old_entry = old_map->header.next;

	while (old_entry != &old_map->header) {
		switch (old_entry->inheritance) {
		case VM_INHERIT_NONE:
			break;

		case VM_INHERIT_SHARE:
			/*
			 *	If we don't already have a sharing map:
			 */

			if (!old_entry->is_a_map) {
			 	vm_map_t	new_share_map;
				vm_map_entry_t	new_share_entry;
				
				/*
				 *	Create a new sharing map
				 */
				 
				new_share_map = vm_map_create(PMAP_NULL,
							old_entry->start,
							old_entry->end);
				new_share_map->is_main_map = FALSE;

				/*
				 *	Create the only sharing entry from the
				 *	old task map entry.
				 */

				new_share_entry =
					vm_map_entry_create(new_share_map);
				*new_share_entry = *old_entry;

				/*
				 *	Insert the entry into the new sharing
				 *	map
				 */

				vm_map_entry_link(new_share_map,
						new_share_map->header.prev,
						new_share_entry);

				/*
				 *	Fix up the task map entry to refer
				 *	to the sharing map now.
				 */

				old_entry->is_a_map = TRUE;
				old_entry->object.share_map = new_share_map;
				old_entry->offset = old_entry->start;
			}

			/*
			 *	Clone the entry, referencing the sharing map.
			 */

			new_entry = vm_map_entry_create(new_map);
			*new_entry = *old_entry;
			vm_map_reference(new_entry->object.share_map);

			/*
			 *	Insert the entry into the new map -- we
			 *	know we're inserting at the end of the new
			 *	map.
			 */

			vm_map_entry_link(new_map, new_map->header.prev,
						new_entry);

			/*
			 *	Update the physical map
			 */

			pmap_copy(new_map->pmap, old_map->pmap,
				new_entry->start,
				(old_entry->end - old_entry->start),
				old_entry->start);
			break;

		case VM_INHERIT_COPY:
			/*
			 *	Clone the entry and link into the map.
			 */

			new_entry = vm_map_entry_create(new_map);
			*new_entry = *old_entry;
			new_entry->pageable = TRUE;
			new_entry->object.vm_object = vm_object_null();
			new_entry->is_a_map = FALSE;
			vm_map_entry_link(new_map, new_map->header.prev,
							new_entry);
			vm_map_copy_entry(old_map, new_map,
						old_entry, new_entry);
			break;
		}
		old_entry = old_entry->next;
	}

	vm_map_unlock(old_map);

	splx(s);
	return(new_map);
}

/*
 *	vm_map_lookup:
 *
 *	Finds the VM object, offset, and
 *	protection for a given virtual address in the
 *	specified map, assuming a page fault of the
 *	type specified.
 *
 *	Leaves the map in question locked for read; return
 *	values are guaranteed until a vm_map_lookup_done
 *	call is performed.
 *
 *	A handle (out_entry) is returned for use in
 *	vm_map_lookup_done, to make that fast.
 *
 *	If a lookup is requested with "write protection"
 *	specified, the map may be changed to perform virtual
 *	copying operations, although the data referenced will
 *	remain the same.
 */
kern_return_t vm_map_lookup(map, vaddr, fault_type, out_entry,
				object, offset, prot, wired, single_use)
	vm_map_t	map;
	vm_offset_t	vaddr;
	vm_prot_t	fault_type;

	vm_map_entry_t	*out_entry;	/* OUT */
	vm_object_t	*object;	/* OUT */
	vm_offset_t	*offset;	/* OUT */
	vm_prot_t	*prot;		/* OUT */
	boolean_t	*wired;		/* OUT */
	boolean_t	*single_use;	/* OUT */
{
	vm_map_t			share_map;
	register vm_map_entry_t		entry;
	vm_offset_t			share_offset;

	RetryLookup: ;

	/*
	 *	Lookup the faulting address.
	 */

	vm_map_lock_read(map);

#define	RETURN(why) \
		{ \
		vm_map_unlock_read(map); \
		return(why); \
		}

	if (!vm_map_lookup_entry(map, vaddr, out_entry))
		RETURN(KERN_INVALID_ADDRESS);

	entry = *out_entry;

	/*
	 *	Check whether this task is allowed to have
	 *	this page.
	 */

	*prot = entry->protection;
	if ((fault_type & (*prot)) != fault_type)
		RETURN(KERN_PROTECTION_FAILURE);

	/*
	 *	If this page is not pageable, we have to get
	 *	it for all possible accesses.
	 */

	if (*wired = !entry->pageable)
		*prot = fault_type = entry->protection;

	/*
	 *	If we don't already have a VM object, track
	 *	it down.
	 */

	if (*single_use = !entry->is_a_map) {
	 	share_map = map;
		share_offset = vaddr;
	}
	else {
		vm_map_entry_t	share_entry;

		/*
		 *	Compute the sharing map, and offset into it.
		 */

		share_map = entry->object.share_map;
		share_offset = (vaddr - entry->start) + entry->offset;

		/*
		 *	Look for the backing store object and offset
		 */

		vm_map_lock_read(share_map);

		if (!vm_map_lookup_entry(share_map, share_offset,
					&share_entry)) {
			vm_map_unlock_read(share_map);
			RETURN(KERN_INVALID_ADDRESS);
		}
		entry = share_entry;
	}

	/*
	 *	If the entry was copy-on-write, we either ...
	 */

	if (entry->needs_copy) {
	    	/*
		 *	If we want to write the page, we may as well
		 *	handle that now since we've got the sharing
		 *	map locked.
		 *
		 *	If we don't need to write the page, we just
		 *	demote the permissions allowed.
		 */

		if (fault_type & VM_PROT_WRITE) {
			/*
			 *	Make a new object, and place it in the
			 *	object chain.  Note that no new references
			 *	have appeared -- one just moved from the
			 *	share map to the new object.
			 */

			if (lock_read_to_write(&share_map->lock)) {
				if (share_map != map)
					vm_map_unlock_read(map);
				goto RetryLookup;
			}

			vm_object_shadow(
				&entry->object.vm_object,
				&entry->offset,
				(vm_size_t) (entry->end - entry->start));
				
			entry->needs_copy = FALSE;
			
			lock_write_to_read(&share_map->lock);
		}
		else {
			/*
			 *	We're attempting to read a copy-on-write
			 *	page -- don't allow writes.
			 */

			*prot &= (~VM_PROT_WRITE);
		}
	}


	/*
	 *	Return the object/offset from this entry.
	 *	Create an object if necessary.
	 *	[If it is copy-on-written, the entry has been fixed up.]
	 */

	*offset = (share_offset - entry->start) + entry->offset;
	*object = entry->object.vm_object;
	if (*object == VM_OBJECT_NULL) {
		*object = vm_object_allocate(entry->end - entry->start);
		entry->object.vm_object = *object;
	}

	/*
	 *	Return whether this is the only map sharing this data.
	 */

	if (!*single_use) {
		simple_lock(&share_map->ref_lock);
		*single_use = (share_map->ref_count == 1);
		simple_unlock(&share_map->ref_lock);
	}

	return(KERN_SUCCESS);
	
#undef	RETURN
}

/*
 *	vm_map_lookup_done:
 *
 *	Releases locks acquired by a vm_map_lookup
 *	(according to the handle returned by that lookup).
 */
void vm_map_lookup_done(map, entry)
	vm_map_t		map;
	register vm_map_entry_t	entry;
{
	/*
	 *	If this entry references a map, unlock it first.
	 */

	if (entry->is_a_map)
		vm_map_unlock_read(entry->object.share_map);

	/*
	 *	Unlock the main-level map
	 */

	vm_map_unlock_read(map);
}

/*
 *	vm_map_print:	[ debug ]
 */
void vm_map_print(map)
	register vm_map_t	map;
{
	register vm_map_entry_t	entry;
	extern int indent;

#if	DEBUG_LONGFORM
	iprintf("Address Map 0x%x. (%s map)\n", map, map->is_main_map ?
					"TASK" : "SHARE");
	indent += 2;
	iprintf("Header = (head(next) = 0x%x, tail(prev) = 0x%x).\n",
			map->header.next, map->header.prev);
	iprintf("Physical map = 0x%x.\n", map->pmap);
	iprintf("Ref count = %d.\n", map->ref_count);
	iprintf("Nentries = %d.\n", map->nentries);
	iprintf("Dumping entries...\n");
	entry = map->header.next;		/* head */
	while (entry != &map->header) {
		iprintf("Entry 0x%x.\n", entry);
		indent += 2;
		iprintf("(next = 0x%x, prev = 0x%x).\n",
				entry->next, entry->prev);
		iprintf("(start = 0x%x, end = 0x%x (len = 0x%x(%d))).\n",
				entry->start, entry->end,
				entry->end - entry->start,
				entry->end - entry->start);
		if (!map->is_main_map) {
			iprintf("(object = 0x%x, offset = 0x%x(%d)).\n",
				entry->object.vm_object, entry->offset,
				entry->offset);
			iprintf("copy_on_write = %s, needs_copy = %s.\n",
					entry->copy_on_write ?
							"TRUE" : "FALSE",
					entry->needs_copy ? "TRUE" : "FALSE");
			indent += 2;
			vm_object_print(entry->object.vm_object);
			indent -= 2;
		}
		else {
			iprintf("(share_map = 0x%x, offset = 0x%x(%d)).\n",
				entry->object.share_map, entry->offset,
				entry->offset);
			iprintf("(prot = %d, max_prot = %d).\n",
					entry->protection,
					entry->max_protection);
			iprintf("inheritance = %d.\n", entry->inheritance);
			indent += 2;
			vm_map_print(entry->object.share_map);
			indent -= 2;
		}
		entry = entry->next;
		indent -= 2;
	}
	indent -= 2;
#else	DEBUG_LONGFORM

	iprintf("%s map 0x%x: pmap=0x%x,ref=%d,nentries=%d\n",
		(map->is_main_map ? "Task" : "Share"),
 		(int) map, (int) (map->pmap), map->ref_count, map->nentries);
	indent += 2;
	for (entry = map->header.next; entry != &map->header;
				entry = entry->next) {
		iprintf("map entry 0x%x: start=0x%x, end=0x%x, ",
			(int) entry, (int) entry->start, (int) entry->end);
		if (map->is_main_map) {
		     	static char *inheritance_name[4] =
				{ "share", "copy", "none", "donate_copy"};
			printf("prot=%x/%x/%s, ",
				entry->protection,
				entry->max_protection,
				inheritance_name[entry->inheritance]);
			if (!entry->pageable)
				printf("wired, ");
		}

		if (entry->is_a_map) {
		 	printf("share=0x%x, offset=0x%x\n",
				(int) entry->object.share_map,
				(int) entry->offset);
			if ((entry->prev == &map->header) || (!entry->prev->is_a_map) ||
			    (entry->prev->object.share_map != entry->object.share_map)) {
				indent += 2;
				vm_map_print(entry->object.share_map);
				indent -= 2;
			}
				
		}
		else {
			printf("object=0x%x, offset=0x%x",
				(int) entry->object.vm_object,
				(int) entry->offset);
			if (entry->copy_on_write)
				printf(", copy (%s)", entry->needs_copy ? "needed" : "done");
			printf("\n");

			if ((entry->prev == &map->header) || (entry->prev->is_a_map) ||
			    (entry->prev->object.vm_object != entry->object.vm_object)) {
				indent += 2;
				vm_object_print(entry->object.vm_object);
				indent -= 2;
			}
		}
	}
	indent -= 2;
#endif	DEBUG_LONGFORM
}
