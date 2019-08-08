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
 *	File:	vm/vm_pageout.c
 *	Author:	Avadis Tevanian, Jr., Michael Wayne Young
 *
 *	Copyright (C) 1985, Avadis Tevanian, Jr., Michael Wayne Young
 *
 *	The proverbial page-out daemon.
 *
 ************************************************************************
 * HISTORY
 *  6-Jun-86  Avadis Tevanian (avie) at Carnegie-Mellon University
 *	Always move at least one page from active to reclaim on a pass
 *	through the pageout scan which freed no pages.  Actually, this
 *	probably indicates that the paging system is in deep trouble
 *	because it is trying to through away pages and it can't find any
 *	place to throw them.
 *
 *  2-May-86  David Golub (dbg) at Carnegie-Mellon University
 *	Try to collapse an object before paging it out.
 *
 * 10-Apr-86  David Golub (dbg) at Carnegie-Mellon University
 *	Pageout now locks object (by setting "pageout_in_progress") to
 *	prevent it from vanishing while paging out.  Moved code that
 *	sets up default pager from vm_pager to this module.  If we can't
 *	get paging space for an object, don't panic; leave it in the
 *	laundry and hope that paging space will show up later.
 *
 * 20-Mar-86  Avadis Tevanian (avie) at Carnegie-Mellon University
 *	Complete rewrite.  New algorithm tries to allow pages to be on
 *	the reclaim queue longer.  This allows active processes to
 *	simply reclaim their pages before they actually get thrown away.
 *	In addition, the write of dirty pages is delayed a bit more than
 *	the first algorithm.  More delay time for both dirty and clean
 *	pages can be gained by increasing the "distance" between the
 *	free and reclaim targets.  This new algorithm is actually much
 *	simpler than the first one (there are no intermediate queues used).
 *
 * 19-Mar-86  Avadis Tevanian (avie) at Carnegie-Mellon University
 *	Finally got a real working version that pages both clean and
 *	dirty pages.
 *
 * 16-Mar-86  Avadis Tevanian (avie) at Carnegie-Mellon University
 *	First working version... will pageout clean pages, dirty pages
 *	get stuck on the reclaim queue (unless reclaimed).
 *
 * 11-Nov-85  Michael Young (mwyoung) at Carnegie-Mellon University
 *	First cut.
 ************************************************************************
 */

#include "../vm/vm_page.h"
#include "../vm/pmap.h"
#include "../vm/vm_object.h"
#include "../vm/vm_pageout.h"
#include "../vm/vm_statistics.h"

#include "../h/param.h"			/* for PSWP */

int	vm_pages_needed;

/*
 *	vm_pageout is the high level pageout daemon.
 */

void vm_pageout()
{
	(void) spl0();

	/*
	 *	Initialize some paging parameters.
	 */

	vm_page_reclaim_target = vm_page_free_count / 10;
	vm_page_free_target = vm_page_reclaim_target / 2;
	vm_page_free_min = vm_page_free_target / 2;

	if (vm_page_free_min < 3)
		vm_page_free_min = 3;

	if (vm_page_free_target <= vm_page_free_min)
		vm_page_free_target = vm_page_free_min + 1;

	if (vm_page_reclaim_target <= vm_page_free_target)
		vm_page_reclaim_target = vm_page_free_target + 1;

	/*
	 *	The pageout daemon is never done, so loop
	 *	forever.
	 */

	printf("min free = %d pages, free target = %d pages, reclaim target = %d pages\n",
		vm_page_free_min, vm_page_free_target, vm_page_reclaim_target);

	vm_page_lock();
	while (TRUE) {
		sleep_and_unlock(&vm_pages_needed, PSWP+2, &vm_page_system_lock);
		vm_pageout_scan();
		vm_page_lock();
		wakeup(&vm_page_free_count);
	}
}

/*
 *	vm_pageout_scan does the dirty work for the pageout daemon.
 */
vm_pageout_scan()
{
	register vm_page_t	m;
	register int		page_shortage;
	register int		s;
	register int		pages_freed;

	/*
	 *	Acquire the resident page system lock,
	 *	as we may be changing what's resident quite a bit.
	 */

	s = splvm();
	vm_page_lock();

	/*
	 *	Only continue when we want more pages to be "free"
	 */

	if (vm_page_free_count < vm_page_free_target) {
		/*
		 *	See whether the physical mapping system
		 *	knows of any pages which are not being used.
		 */
		 
		pmap_collect();

		/*
		 *	And be sure the pmap system is updated so
		 *	we can scan the reclaim queue.
		 */

		pmap_update();
	}

	/*
	 *	Start scanning the reclaim queue for pages we can free.
	 *	We keep scanning until we have enough free pages or
	 *	we have scanned through the entire queue.  If we
	 *	encounter dirty pages, we start cleaning them.
	 */

	pages_freed = 0;
	m = (vm_page_t) queue_first(&vm_page_queue_reclaim);
	while (vm_page_free_count < vm_page_free_target) {
		/*
		 *	If we are at the end of the queue, there's nothing
		 *	more we can do here.
		 */

		if (queue_end(&vm_page_queue_reclaim, (queue_entry_t) m))
			break;

		if (m->clean) {
			vm_page_t	next;

			next = (vm_page_t) queue_next(&m->pageq);
			vm_page_free(m);	/* will dequeue */
			pages_freed++;
			m = next;
		}
		else {
			/*
			 *	If a page is dirty, then it is either
			 *	being washed (but not yet cleaned)
			 *	or it is still in the laundry.  If it is
			 *	still in the laundry, then we start the
			 *	cleaning operation.
			 */

			if (m->laundry) {
				/*
				 *	Clean the page and remove it from the
				 *	laundry.
				 *
				 *	We set the busy bit to cause
				 *	potential page faults on this page to
				 *	block.
				 *
				 *	And we set pageout-in-progress to keep
				 *	the object from disappearing during pageout.
				 *	This guarantees that the page won't move
				 *	from the reclaim queue.  (However, any other
				 *	page on the reclaim queue may move!)
				 */

				register vm_object_t	object;
				register vm_pager_t	pager;

				object = m->object;

				/*
				 *	Try to collapse the object before making a
				 *	pager for it.
				 */
				vm_object_collapse(object);

				object->pageout_in_progress = TRUE;

				m->busy = TRUE;
				vm_stat.pageouts++;

				/*
				 *	Do a wakeup here in case the following
				 *	operations block.
				 */
				wakeup(&vm_page_free_count);

				vm_page_unlock();
				splx(s);

				/*
				 *	If there is no pager for the page,
				 *	use the default pager.  If there's
				 *	no place to put the page at the
				 *	moment, leave it in the laundry and
				 *	hope that there will be paging space
				 *	later.
				 */

				if ((pager = object->pager) == vm_pager_null) {
					vm_pager_id_t	id;

					id = vm_pager_allocate(vm_pager_default,
								object->size);
					if (id != (vm_pager_id_t) NULL) {
						pager = vm_pager_default;
						vm_object_setpager(object, pager,
								id, 0, FALSE);
					}
				}

				if (pager != vm_pager_null) {
					vm_pager_put(pager,
							object->paging_space, m);

					/*
					 *	OK to turn off "laundry" here;
					 *	no other process or thread looks
					 *	at that flag.
					 */
					m->laundry = FALSE;
				}

				s = splvm();
				vm_page_lock();

				/* DEBUG */
				if (!m->reclaimable) {
					panic("vm_pageout: page moved off reclaim queue");
				}

				m->busy = FALSE;
				wakeup(m);

				object->pageout_in_progress = FALSE;
				wakeup(object);
			}
			m = (vm_page_t) queue_next(&m->pageq);
		}
	}
	
	/*
	 *	Compute the page shortage.  If we are still very low on memory
	 *	be sure that we will move a minimal amount of pages from active
	 *	to reclaimable.
	 */

	page_shortage = vm_page_reclaim_target - vm_page_reclaim_count;

	if ((page_shortage <= 0) && (pages_freed == 0))
		page_shortage = 1;

	while (page_shortage > 0) {
		/*
		 *	Move some more pages from active to reclaim.
		 */

		if (queue_empty(&vm_page_queue_active)) {
			break;
		}
		m = (vm_page_t) queue_first(&vm_page_queue_active);
		pmap_remove_all(VM_PAGE_TO_PHYS(m));
		vm_page_reclaim(m);
		page_shortage--;
	}

	vm_page_unlock();
	splx(s);
}
