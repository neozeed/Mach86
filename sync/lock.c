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
 *	File:	sync/lock.c
 *	Author:	Avadis Tevanian, Jr., Michael Wayne Young
 *
 *	Copyright (C) 1985, Avadis Tevanian, Jr., Michael Wayne Young
 *
 *	Locking primitives implementation
 *
 * HISTORY
 * 26-Nov-85  Michael Young (mwyoung) at Carnegie-Mellon University
 *	Improved read-to-write upgrade to have priority over pure write.
 *
 * 07-Nov-85  Michael Wayne Young (mwyoung) at Carnegie-Mellon University
 *	Overhauled from previous version.
 */

#include "cpus.h"
#include "../sync/lock.h"

#if	NCPUS > 1

/*
 *	Module:		lock
 *	Function:
 *		Provide reader/writer sychronization.
 *	Implementation:
 *		Simple interlock on a bit.  Readers first interlock
 *		increment the reader count, then let go.  Writers hold
 *		the interlock (thus preventing further readers), and
 *		wait for already-accepted readers to go away.
 */

#undef P
#undef V

#define		P(l)	BUSYP((int *)l, 0)
#define		V(l)	BUSYV((int *)l, 0)

/*
 *	simple_lock_init initializes a simple lock.  A simple lock
 *	may only be used for exclusive locks.
 */

void simple_lock_init(l)
	simple_lock_t	l;
{
	bzero(l, sizeof(l));
}

void simple_lock(l)
	simple_lock_t	l;
{
	BUSYP((int *)l, 0);
}

void simple_unlock(l)
	simple_lock_t	l;
{
	BUSYV((int *)l, 0);
}

/*
 *	Routine:	lock_init
 *	Function:
 *		Initialize a lock; required before use.
 *		Note that clients declare the "struct lock"
 *		variables and then initialize them, rather
 *		than getting a new one from this module.
 */
void lock_init(l)
	lock_t		l;
{
	bzero(l, sizeof(lock_data_t));
	V(l);
	l->want_write = FALSE;
	l->want_upgrade = FALSE;
	l->read_count = 0;
}

void lock_write(l)
	lock_t		l;
{
	boolean_t	old_want;
	boolean_t	old_upgrade;
	int		count;

	do {
		/* First, wait for a possible write */

		while (l->want_write)
			continue;

		/* Really try to acquire the want_write bit */
		 
		P(&l->interlock);
		old_want = l->want_write;
		l->want_write = TRUE;
		count = l->read_count;
		old_upgrade = l->want_upgrade;
		V(&l->interlock);
	} while (old_want);

	/* Wait for readers (and upgrades) to finish */

	while ((count > 0) || old_upgrade) {
		/* Again, wait non-interlocked first */

		while ((l->read_count > 0) || l->want_upgrade)
			continue;

		P(&l->interlock);
		count = l->read_count;
		old_upgrade = l->want_upgrade;
		V(&l->interlock);
	}
}

void lock_done(l)
	lock_t		l;
{
	P(&l->interlock);
	if (l->read_count > 0)
		l->read_count--;
	else if (l->want_upgrade)
	 	l->want_upgrade = FALSE;
	else
	 	l->want_write = FALSE;
	V(&l->interlock);
}

void lock_read(l)
	lock_t		l;
{
	boolean_t	old_want;

	do {
		while (l->want_write || l->want_upgrade)
			continue;

		P(&l->interlock);
		if (! (old_want = l->want_write || l->want_upgrade))
			l->read_count++;
		V(&l->interlock);
	} while (old_want);
}

/*
 *	Routine:	lock_read_to_write
 *	Function:
 *		Improves a read-only lock to one with
 *		write permission.  If another reader has
 *		already requested an upgrade to a write lock,
 *		no lock is held upon return.
 *
 *		Returns TRUE if the upgrade *failed*.
 */
boolean_t lock_read_to_write(l)
	lock_t		l;
{
	boolean_t	old_upgrade;
	int		count;

	P(&l->interlock);
	old_upgrade = l->want_upgrade;
	l->want_upgrade = TRUE;
	count = --l->read_count;
	V(&l->interlock);
	
	if (old_upgrade)
		return (TRUE);

	while (count > 0) {
		while (l->read_count > 0)
			continue;

		P(&l->interlock);
		count = l->read_count;
		V(&l->interlock);
	}

	return(FALSE);
}

void lock_write_to_read(l)
	lock_t		l;
{
	P(&l->interlock);
	l->read_count++;
	if (l->want_upgrade)
		l->want_upgrade = FALSE;
	else
	 	l->want_write = FALSE;
	V(&l->interlock);
}

#endif	NCPUS > 1
