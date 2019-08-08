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
 *	File:	sched_prim.c
 *	Author:	Avadis Tevanian, Jr.
 *
 *	Copyright (C) 1986, Avadis Tevanian, Jr.
 *
 *	Scheduling primitivies
 *
 * HISTORY
 * 11-Jun-86  Bill Bolosky (bolosky) at Carnegie-Mellon University
 *	romp: Added include of ../machine/param.h to get macro def'n of
 *	splsched (since there is no inline program for the RT).
 *
 *  1-Jun-86  Avadis Tevanian (avie) at Carnegie-Mellon University
 *	Relocated routines from mp/sched.c to this file.
 *
 *  8-Apr-86  Avadis Tevanian (avie) at Carnegie-Mellon University
 *	Created.
 */

#include "../h/types.h"

#include "../h/queue.h"
#include "../h/thread.h"
#include "../sync/lock.h"
#include "../h/sched.h"
#include "../vm/vm_map.h"
#ifdef	romp
#include "../machine/param.h"	/* For def'n of splsched() */
#endif	romp
#include "../machine/pmap.h"

#if	someday
/*
 *	create_wait(event, persistent)
 *
 *	Create and initialize a new wait structure corresponding to the
 *	specified event.  A persistence is specified to determine if the
 *	wait structure should ever be deallocated.  (Persistent wait
 *	structures are used for frequently used events).
 *
 *	The wait structure is entered into the event hash table.
 */

wait_t create_wait(event, persistent)
	event_t		event;
	boolean_t	persistent;
{
	wait_t		wait;
	wait_bucket_t	bucket;

	/*
	 *	Allocate and initialize and wait structure.
	 */

	wait = (wait_t) zalloc(wait_zone);
	wait->event = event;
	wait->persistent = persistent;
	queue_init(&wait->thread_list);
	simple_lock_init(&wait->lock);

	/*
	 *	Insert the wait structure into the hash table.
	 */

	bucket = wait_buckets[wait_hash(event)];
	lock_write(&bucket->lock);
	queue_enter(&bucket->waitq, wait, wait_t, listq);
	lock_write_done(&bucket->lock);
}

/*
 *	assert_wait(event)
 *
 *	assert that the current thread wants to wait for the specified
 *	event.  The thread does not actually wait at this time, but it
 *	should wait sometime in the near future.
 */

assert_wait(event)
	event_t		event;
{
	wait_t		wait;
	wait_bucket_t	bucket;
	boolean_t	found;
	thread_t	thread;

	/*
	 *	Find the bucket for this event.
	 */

	bucket = wait_buckets[wait_hash(event)];
	found = FALSE;

	/*
	 *	See if there is already a wait structure for the
	 *	event.
	 */

	lock_read(&bucket->lock);
	wait = (wait_t) queue_first(&bucket->waitq);
	while (!queue_end(&bucket->waitq, (queue_entry_t) wait)) {
		if (wait->event == event) {
			found = TRUE;
			break;
		}
		wait = (wait_t) queue_next(&wait->listq);
	}
	lock_read_done(&bucket->lock);

	/*
	 *	If there was no such entry, then create (and insert)
	 *	a non-persistent wait structure.
	 */

	if (!found)
		wait = create_wait(FALSE);
	}

	/*
	 *	Now we have a wait structure corresponding to our event
	 *	(which is in the hash table).  We must now insert our thread
	 *	into the list of threads waiting for this event, which means
	 *	we create yet another structure to represent it (because a
	 *	thread may be waiting for more than one event).
	 *
	 *	Then, link the thread wait structure into the thread's list
	 *	of what it is waiting on.
	 */

	thread = current_thread();
	twait = (thread_wait_t) zalloc(thread_wait_zone);
	twait->thread = thread;

	simple_lock(&wait->lock);
	queue_enter(&wait->thread_list, twait, thread_wait_t, threadq);
	queue_enter(&thread->wait_list, twait, thread_wait_t, waitq);
	simple_unlock(&wait->lock);
}
#endif	someday

#define	NUMQUEUES	59

queue_head_t		wait_queue[NUMQUEUES];
simple_lock_data_t	wait_lock[NUMQUEUES];

#define wait_hash(event) \
	(((unsigned)event)%NUMQUEUES)

wait_queue_init()
{
	register int i;

	for (i = 0; i < NUMQUEUES; i++) {
		queue_init(&wait_queue[i]);
		simple_lock_init(&wait_lock[i]);
	}
}

/*
 *	assert_wait:
 *
 *	Assert that the current thread is about to go to
 *	sleep until the specified event occurs.
 */
assert_wait(event)
	int	event;
{
	register queue_t	q;
	register int		index;
	register thread_t	thread;
	register simple_lock_t	lock;

	thread = current_thread();
	index = wait_hash(event);
	q = &wait_queue[index];
	lock = &wait_lock[index];
	simple_lock(lock);
	thread_lock(thread);
	enqueue_tail(q, thread);
	thread->state = THREAD_WILL_WAIT;
	thread->wait_event = event;
	thread_unlock(thread);
	simple_unlock(lock);
}

/*
 *	thread_wakeup:
 *
 *	Wakeup all threads that are waiting for the specified
 *	event.
 */
thread_wakeup(event)
	int	event;
{
	register queue_t	q;
	register int		index;
	register thread_t	thread;
	register simple_lock_t	lock;

	index = wait_hash(event);
	q = &wait_queue[index];
	lock = &wait_lock[index];
	simple_lock(lock);
	thread = (thread_t) queue_first(q);
	while (!queue_end(q, (queue_entry_t)thread)) {
		thread_lock(thread);
		if (thread->wait_event == event) {
			thread_t	t;

			t = thread;
			remqueue(q, t);
			switch (thread->state) {
			case THREAD_WAITING:
				thread->state = THREAD_RUNNING;
				thread_setrun(thread);
				break;
			case THREAD_WILL_WAIT:
				thread->state = THREAD_RUNNING;
				break;
			default:
				printf("thread_wakeup: state = %d\n",
						thread->state);
				panic("thread_wakeup");
			}
		}
		thread_unlock(thread);
		thread = (thread_t) queue_next((queue_t) thread);
	}
	simple_unlock(lock);
}

/*
 *	thread_sleep:
 *
 *	Cause the current thread to wait until the specfied event
 *	occurs.  The specified lock is unlocked before releasing
 *	the cpu.  (This is a convenient way to sleep without manually
 *	calling assert_wait).
 */
thread_sleep(event, lock)
	int		event;
	simple_lock_t	lock;
{
	assert_wait(event);		/* assert that we are going to wait */
	simple_unlock(lock);		/* release the specified lock */
	thread_block();			/* block ourselves */
}

/*
 *	thread_setrun:
 *
 *	Set the specfied thread on the appropriate run queue.
 */
thread_setrun(thread)
	register thread_t	thread;
{
	register int	s;

	s = splsched();
	lock_write(&sched_lock);
	if (thread->whichq)
		set_runq(thread, thread->whichq);
	else
		set_runq(thread, &global_runq);
	lock_write_done(&sched_lock);
	splx(s);
}

/*
 *	thread_block:
 *
 *	Block the current thread.  If the thread is runnable
 *	then someone must have woken it up between it's request
 *	to sleep and now.  In this case, it goes back on a
 *	run queue.
 */
thread_block()
{
	register thread_t	thread;
	register int		mycpu;

	thread = current_thread();
	PMAP_DEACTIVATE(vm_map_pmap(thread->task->map), thread,
				cpu_number());
	if (save_context()) {
		return;
	}

	/*
	 *	See if we should go back on a run queue.
	 */

	thread_lock(thread);
	if (thread->state == THREAD_RUNNING) {
		thread_setrun(thread);
	}
	thread_unlock(thread);

	lock_read(&sched_lock);
	thread = choose_thread();
	if (thread == NULL) {
		printf("thread_block: failed to find any threads.\n");
		lock_read_done(&sched_lock);
		panic("thread_block");
	}
	lock_read_done(&sched_lock);

	mycpu = cpu_number();
	active_threads[mycpu] = thread;
	PMAP_ACTIVATE(vm_map_pmap(thread->task->map), thread, mycpu);
	load_context(thread);
	/*NOTREACHED*/
}

int sched_debug = 0;

/*
 *	set_runq:
 *
 *	Put a thread on the specified run queue.  The
 *	scheduler should be locked externally (if necessary).
 */

set_runq(th, rq)
	thread_t		th;
	struct run_queue	*rq;
{
	register int	whichq;

	whichq = th->pri/4;
	if (whichq < 0) {
		printf("set_runq: pri negative (%d)\n", th->pri);
		whichq = 0;
	}
	if (whichq >= NRQS) {
		printf("set_runq pri too high (%d)\n", th->pri);
		whichq = NRQS - 1;
	}
	if (th->runq != RUN_QUEUE_NULL) {
		printf("set_runq: thread 0x%x already on run queue 0x%x.\n",
			th, th->runq);
		printf("set_runq: requested to place on run queue 0x%x.\n",
			rq);
		panic("set_runq");
	}

	simple_lock(&rq->lock);	/* lock the run queue */
	enqueue_tail(&rq->runq[whichq], th);
	if (whichq < rq->low) rq->low = whichq;	/* minimize */
	rq->count++;
	simple_unlock(&rq->lock);

	th->runq = rq;
}

/*
 *	rem_runq:
 *
 *	Remove a thread from it's run queue.  The
 *	scheduler should be locked externally (if necessary).  The
 *	run queue that the process was on is returned (or RUN_QUEUE_NULL if
 *	not on a run queue).
 */

struct run_queue *rem_runq(th)
	thread_t		th;
{
	register int		whichq;
	struct run_queue	*rq;

	whichq = 0;
	rq = th->runq;

	if (rq != RUN_QUEUE_NULL) {
		simple_lock(&rq->lock);
		remqueue(&rq->runq[whichq], th);
		rq->count--;
		simple_unlock(&rq->lock);
	}

	th->runq = RUN_QUEUE_NULL;
	return(rq);
}

/*
 *	set_pri:
 *
 *	Set the priority of the specified thread to the specified
 *	priority.  This may cause the thread to change queues.
 */

set_pri(th, pri)
	thread_t	th;
	int		pri;
{
	struct run_queue	*rq;

	lock_write(&sched_lock);	/* lock the scheduler */
	rq = rem_runq(th);
	th->pri = pri;
	if (rq != RUN_QUEUE_NULL) {
		set_runq(th, th->runq);
	}
	lock_write_done(&sched_lock);	/* unlock the scheduler */
}

/*
 *	choose_thread:
 *
 *	Choose a thread to execute.  The thread chosen is removed
 *	from it's run queue.
 *
 *	Strategy:
 *		lock global queue and private queue (IN THAT ORDER)
 *		t1 = highest priority thread in global queue (p1)
 *		t2 = highest priority thread in private queue (p2)
 *		t = (p1 > p2) ? t1 : t2		(ties go to private)
 *		remove t from its queue.
 *		unlock run queues
 */

int dump_queues = 0;

thread_t choose_thread()
{
	register int			i, j;
	register queue_t		q1, q2;
	register thread_t		t1, t2;
	register run_queue_t		lq;

	lq = &local_runq[cpu_number()];
	q1 = global_runq.runq;
	q2 = lq->runq;
	t1 = THREAD_NULL;

	simple_lock(&global_runq.lock);
	simple_lock(&lq->lock);
	if (global_runq.count > 0) {
		q1 += global_runq.low;		/* offset */
		for (i = global_runq.low; i < NRQS; i++) {
			if (!queue_empty(q1)) {
				t1 = (thread_t) queue_next(q1);
				break;
			}
			else {
				q1++;
			}
		}
		global_runq.low = i;
	}
	else {
		global_runq.low = NRQS;
		i = NRQS;
	}

	if (lq->count > 0) {
		q2 += lq->low;			/* offset */
		for (j = lq->low; j < i; j++) {
			if (!queue_empty(q2)) {
				t1 = (thread_t)queue_next(q2);
				q1 = q2;
				break;
			}
			else {
				q2++;
			}
		}
		if (j > lq->low)
			lq->low = j;
	}
	else {
		lq->low = NRQS;
	}

	if (t1 != THREAD_NULL) {
		t1 = (thread_t)dequeue_head(q1);
		t1->runq = RUN_QUEUE_NULL;
		if (q1 == q2)	/* picked local queue */
			lq->count--;
		else
			global_runq.count--;
	}

	/*
	 *	The queue locks are still held here because we don't
	 *	know which queue we did the final dequeue on.  This
	 *	could probably be improved if necessary.
	 */

	simple_unlock(&lq->lock);
	simple_unlock(&global_runq.lock);
	return(t1);
}

thread_copy(new, old)
	int	new;
	int	old;
{
	register struct thread	*new_thread, *old_thread;

#if	0
	new_thread = &thread[new];
	old_thread = &thread[old];
	queue_init(&new_thread->links);
	new_thread->runq = RUN_QUEUE_NULL;
	lock_init(&new_thread->lock);
	new_thread->pri = old_thread->pri;
#endif
}

thread_exit(tid)
	int	tid;
{
	register struct thread	*th;

	/*
	 * this is all probably unnecessary, but it's nice to have a
	 * "clean" thread table.
	 */

#if	0
	th = &thread[tid];
	queue_init(&th->links);
	th->runq = RUN_QUEUE_NULL;
	lock_init(&th->lock);
	th->pri = 0;
#endif
}
