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
 *	File:	sem_sem.c
 *	Author:	Avadis Tevanian, Jr.
 *
 *	Low level synchronization primitives - P and V.
 *
 * HISTORY
 * 11-Oct-85  Robert V Baron (rvb) at Carnegie-Mellon University
 *	Moved data structures into this file.  We no longer use the MPS
 *	structure.  Some changes to initialization, and a few variables
 * 	that were queues now point to the queue
 *
 * 15-Aug-85  Avadis Tevanian (avie) at Carnegie-Mellon University
 *	Allow any process to perform a V, even if it didn't do the P.
 *	This allows a lightweight sleep/wakeup package to be written at
 *	user level.
 *
 * 15-Nov-84  Robert V Baron (rvb) at Carnegie-Mellon University
 *	add mp_init(sem, fn) == run (*fn)() iff it has never been run before
 *	and lock out all other processors till it has finished.
 *
 * 11-Nov-84  Avadis Tevanian (avie) at Carnegie-Mellon University
 *	Changed P, V, allocate_semaphore and deallocate_semaphore back
 *	to normal.  These are not intended to be used directly from a
 *	trap, but are rather called by routines in sem_user.c.
 *
 * 10-Nov-84  Avadis Tevanian (avie) at Carnegie-Mellon University
 *	Beefed up code a bit.  It now checks to be sure that a return
 *	from the sleep call was really do to a V operation from someone
 *	else.  It also checks to see that the process doing the P is the
 *	process with the semaphore.
 *
 * 22-Oct-84  Robert V Baron (rvb) at Carnegie-Mellon University
 *	Changed P, V, allocate_semaphore and deallocate_semaphore to be
 *	called using the new trap interface.  This bascially means that the
 *	routines now start with TP, and return "Success".  allocate_semaphore
 *	was changed to return the semaphore through its argument.
 *
 * 20-Jan-84  Avadis Tevanian (avie) at Carnegie-Mellon University
 *	Created.  (Historical note:  This was the first piece of software
 *	written for the Supercomputer Workbench Kernel, which still has
 *	no name).
 *
 ************************************************************************
 *
 *	Two types of semaphores are provided for.
 *
 *	The first type maintains wait queues for processes waiting
 *	for access to a semaphore.  When a P operation is requested,
 *	the requester is put on the queue, and if possible, allowed
 *	to run.  If a wait is necessary, the process is put to sleep.
 *	The corresponding V operation removes the process from the
 *	queue, and wakes up the next process waiting on the queue.
 *	This type of semaphore is handled by the P and V subroutines.
 *
 *	The second type waits on semaphores by busy waiting.  The bit
 *	that represents the semaphore is 1 if someone is using the
 *	semaphore, it is 0 if not.  This type of semaphore is suited for
 *	uses where a wait is not expected to be long, and the overhead
 *	of process switching is not desirable.  It also has the advantage
 *	that it doesn't need the use of the memory allocator.  Thus,
 *	the memory allocator itself uses this to protect the "memory
 *	resource."  This type of semaphore is handled by the busyP
 *	and busyV subroutines.
 */

#include "wb_ml.h"
#include "mach_mp.h"
#include "mach_mpm.h"
#include "mach_vm.h"

#include "../h/map.h"
#include "../h/param.h"
#include "../h/systm.h"
#include "../h/dir.h"
#include "../h/user.h"
#include "../h/proc.h"
#include "../h/reboot.h"

#include "../machine/cpu.h"

#include "../accent/acc_errors.h"
#include "../sync/mp_queue.h"
#include "../mp/sem_sem.h"
#include "../mp/sem_error.h"
#include "../mp/remote_sw.h"
#include "../mp/remote_prim.h"
#include "../mp/mp_proc.h"

#if	MACH_VM
#include "../vm/vm_kern.h"
#endif	MACH_VM

int sem_count;
struct mpqueue *sem_free;
struct mpqueue *semq_free;
struct mpqueue *sem_inuse;

/*
 *	Perform a P operation on given semaphore.  If the calling process
 *	must wait, it is put on the wait queue and put to sleep.  When
 *	a process is through with the resource, it must call V on the same
 *	semaphore.
 */

P(s)
	register struct semaphore *s;
{
	register struct sem_queue *sq, *sqn;
	register struct mp_proc *mpp = &mp_proc[u.u_procp-proc];
	register wake_reason;
	register where;
	struct sem_queue *newsq;

	mpinc(sem_count);		/* lock */
	if (!find_sem(s)) {
		mpdec(sem_count);
		return(SEM_INVALID);
	}

	for (where = 0; where < NUM_SEMAPHORES; where++)
		if (mpp->cur_sem[where] == NULL) break;

	if (where == NUM_SEMAPHORES) {
		mpdec(sem_count);
		return(SEM_TOOMANY);
	}
		
	mpdequeue_head(semq_free, &newsq);
	sq = newsq;			/* blast it into the register */
	if (sq == NULL) {
		mpdec(sem_count);
		return(SEM_TOOMANY);
	}

	sq->sq_proc = u.u_procp;
	sq->sq_chan = (caddr_t) u.u_procp;
	sq->processor = cpu_number();
	sq->sq_gone = 0;
	sq->sq_sem = s;

	/* Put ourselves on the semaphore's queue.  If it wasn't empty
	   then we sleep.  After waking up, make sure we are at the head
	   of the queue (i.e. it's our turn to execute). */

	if (!mpenqueue_tail(&s->wait_q, sq)) {
		do {
			mpdec(sem_count);
			mpp->sq = sq;
			wake_reason = asleep(sq->sq_chan, PSEM, 0);
			if (sq->sq_gone) {	/* semaphore destroyed? */
				mpenqueue_head(semq_free, sq);
				return(SEM_INVALID);
			}
			mpinc(sem_count);	/* lock */
			if (!find_sem(s)) {		/* still here? */
				mpdec(sem_count);
				return(SEM_INVALID);
			}
			LOCKQ(&s->wait_q);
			sqn = mpqnext(&s->wait_q, struct sem_queue *);
			UNLOCKQ(&s->wait_q);
			if (wake_reason == -1) {
				mpremqueue(&s->wait_q, sq);
				mpenqueue_head(semq_free, sq);
				mpdec(sem_count);
				return(SEM_INTR);
			}
		} while (sqn->sq_proc != u.u_procp);
	}
	mpp->sq = NULL;
	mpp->cur_sem[where] = s;
	mpdec(sem_count);
	return Success;
}

/*
 *	Perform a V operation on the given semaphore.  This is P's
 *	counterpart.
 */

V(s)
	register struct semaphore *s;
{
	register struct sem_queue *nextsq;
	register struct mp_proc *mpp;
	register i;
	struct sem_queue *sq;

	mpinc(sem_count);		/* lock */
	if (!find_sem(s)) {
		mpdec(sem_count);
		return(SEM_INVALID);
	}

	LOCKQ(&s->wait_q);
	if (mpqueueempty(&s->wait_q))
		sq = NULL;
	else
		sq = mpqnext(&s->wait_q, struct sem_queue *);
	UNLOCKQ(&s->wait_q);

	if (sq == NULL) {
		mpdec(sem_count);
		return(SEM_INVALID);
	}

	mpp = &mp_proc[sq->sq_proc-proc];

	for (i = 0; i < NUM_SEMAPHORES; i++) {
		if (mpp->cur_sem[i] == s) {
			mpp->cur_sem[i] = NULL;
			break;
		}
	}

	if (!mpdequeue_head(&s->wait_q, &sq)) {
		LOCKQ(&s->wait_q);
		nextsq = mpqnext(&s->wait_q, struct sem_queue *);
		UNLOCKQ(&s->wait_q);
		mpdec(sem_count);
		if (nextsq->processor == cpu_number())
			wakeup((caddr_t)nextsq->sq_chan);
		else
			remote_wakeup(nextsq->processor,
					(caddr_t)nextsq->sq_chan);
	}
	else mpdec(sem_count);
	mpenqueue_head(semq_free, sq);
	return Success;
}

#define NSEM 100

allocate_semaphore(ss)
struct semaphore **ss;
{
	register struct semaphore *s;
	struct semaphore *new_s;

	/* get a semaphore from the free list */

	mpdequeue_head(sem_free, &new_s);
	s = new_s;
	if (s == NULL)
		return(SEM_TOOMANY);

	initmpqueue(&s->wait_q);
	if (suword((caddr_t)ss, s)) {
		mpenqueue_head(sem_free, s);
		return(NotAUserAddress);
	}
	mpenqueue_head(sem_inuse, s);
	return(Success);
}

deallocate_semaphore(s)
register struct semaphore *s;
{
	struct sem_queue *sq;

	if (!find_sem(s))		/* look up the semaphore */
		return(SEM_INVALID);

	/* remove from inuse list */

	mpremqueue(sem_inuse, s);

	/* wait until there is no one using semaphores */

	while (sem_count != 0) ;	/* spin on sem_count */

	/* now that semaphore is safe to deal with clean it up */

	mpdequeue_head(&s->wait_q, &sq);	/* remove proc executing */
	if (sq != NULL) {
		sq->sq_gone = 1;
		mpenqueue_head(semq_free, sq);
		mpdequeue_head(&s->wait_q, &sq);	/* get next */
	}

	while (sq != NULL) {
		sq->sq_gone = 1;
		if (sq->processor == cpu_number())
			wakeup((caddr_t)sq->sq_chan);
		else
			remote_wakeup(sq->processor,
					(caddr_t)sq->sq_chan);
		mpdequeue_head(&s->wait_q, &sq);
	}

	/* add semaphore to the free list */

	mpenqueue_head(sem_free, s);
	return Success;
}

find_sem(s)
	struct semaphore *s;
{
	register struct mpqueue *inuse;

	if (s == NULL)
		return(0);

	inuse = mpqnext(sem_inuse, struct mpqueue *);
	while (inuse != sem_inuse) {
		if (inuse == (struct mpqueue *) s)
			return(1);
		inuse = mpqnext(inuse, struct mpqueue *);
	}
	return(0);
}

sem_init()
{
	int do_sem_init();

	do_sem_init();
	queuescan("Semaphore inuse sanity", sem_inuse, NSEM*2);
	queuescan("Semaphore free sanity", sem_free, NSEM*2);
	queuescan("Semaphore wait free sanity", semq_free, NSEM*2);
}

/*
 *	do_sem_init() initializes the semaphore data structures.
 *	This is done only once for all processors.  It first determines
 *	if it should do the initialization.  If it decides it should, then
 *	it initializes queues of semaphores and sem_queues.
 */

do_sem_init()
{
	register nq, len, size;
	register caddr_t addr;

	sem_count = 0;

	sem_free = mpq_allochead();
	semq_free = mpq_allochead();
	sem_inuse = mpq_allochead();

	/*
	 *  Allocate memory and break it up for the two queues.
	 */

	size = round_mpq(sizeof(struct sem_queue));
	len = size*NSEM;
#if	MACH_VM
	addr = (caddr_t) kmem_alloc(kernel_map, len, TRUE);
#else	MACH_VM
	addr = (caddr_t) shmustalloc(len);
#endif	MACH_VM

	nq = 0;
	while (len >= size) {
		mpenqueue_head(sem_free, addr);
		addr += size;
		len -= size;
		nq++;
	}
	if (show_space) {
		printf("Semaphore area allocated: %d entries using %d bytes.\n",
			nq, nq*sizeof(struct semaphore));
	}

	size = round_mpq(sizeof(struct sem_queue));
	len = size*NSEM;		/* same (NSEM) */
#if	MACH_VM
	addr = (caddr_t) kmem_alloc(kernel_map, len, TRUE);
#else	MACH_VM
	addr = (caddr_t) shmustalloc(len);
#endif	MACH_VM

	nq = 0;
	while (len >= size) {
		mpenqueue_head(semq_free, addr);
		addr += size;
		len -= size;
		nq++;
	}
	if (show_space) {
		printf("Semaphore wait area allocated: ");
		printf("%d entries using %d bytes.\n", nq,
			nq*sizeof(struct sem_queue));
	}
}

freesq(sq)
	struct sem_queue *sq;
{
	mpenqueue_head(semq_free, sq);
}

#include "mach_acc.h"
#if	MACH_ACC == 0
asleepexpire(chan)
{
	wakeup(chan);
}

asleep(chan, pri, time)
{
	if (setjmp(&u.u_qsave)) {
		return(-1);
	} 
	else {
		if (time)
			timeout(asleepexpire, chan, time);
		sleep(chan, pri);
		return(1);
	}
}
#endif	MACH_ACC == 0
