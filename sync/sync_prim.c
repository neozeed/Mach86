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
 *	File:	sync/sync_prim.c
 *	Author:	Avadis Tevanian, Jr.
 *
 *	Copyright (C) 1985, Avadis Tevanian, Jr.
 *
 *	Kernel synchronization primitives.
 *
 ***********************************************************************
 * HISTORY
 * 17-Mar-85  Avadis Tevanian (avie) at Carnegie-Mellon University
 *	Created.
 *
 ***********************************************************************
 */

#ifdef romp
#include "../h/param.h"  /*Needed for spl macro definitions on romp*/
#endif romp

#include "../h/types.h"

#include "../sync/lock.h"

/*
 *	mp_init is used to initialize things.  It is called to do
 *	automatic locking when one or more processors may race to
 *	perform an initialization, and it doesn't matter which processor
 *	does the initiation.
 */

/*VARARGS*/
mp_init(semap, fn, arg1, arg2, arg3, arg4)
	lock_t	semap;
	int	(*fn)();
{
	BUSYP((int *)semap, 1);			/* lock out everyone else */
	if (!bbssi(0, (int *)semap)) {		/* was I first? */
		(*fn)(arg1, arg2, arg3, arg4);
	}
	BUSYV((int *)semap, 1);			/* unlock */
}

/*
 *	Perform a P operation on given memory location at the specified bit
 *	position.  If the semaphore is in use, then busy waiting is used.
 *	No queueing is done.  When a process is done with the semaphore,
 *	it must call busyV.
 */

busyP(s,bit)
	caddr_t s;
	int bit;
{
	register int spl;
	int count;

	spl = splhigh();		/* no interrupts allowed while waiting */
/* the following code uses somewhat of a sanity check */
	count = 1000000;		/* a nice big number */
	while (bbssi(bit,s)) {
		if (--count == 0) {
			splx(spl);
			printf("Warning: busyP waiting too long\n");
			printf("Addr, bit = 0x%x, %d.\n", s, bit);
			bbcci(bit, s);
			panic("busyP");
			return;
		}
	}
	splx(spl);
}

/*
 *	Perform a V operation on the given memory location at the specified
 *	bit position.  This is busyP's counterpart.
 */

busyV(s,bit)
	register caddr_t s;
	register int bit;
{
	bbcci(bit,s);			/* turn bit off */
}
