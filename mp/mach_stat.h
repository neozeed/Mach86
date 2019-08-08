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
/*CMU:	%M%	%I%	%G%	*/

/*
 **************************************************************************
 * HISTORY
 * 11-Oct-85  Robert V Baron (rvb) at Carnegie-Mellon University
 *	New header to define the things that, I need one of per machine.
 *	Note: the structure MUST be quad word aligned.
 *	Note: MSG_SPACE is defined in ../mp/remote_prim.h so you must
 *	include that header as well as the ../sync/mp_queue.h
 *
 **************************************************************************
 */

struct MACHstate {

/* the following queues must be quad-word aligned */

	struct	mpqueue msg_free;		/* free list for comm */
	struct	mpqueue msg_pending;		/* pending list for comm */
	char	msg_buffer_space[MSG_SPACE];	/* must be quad-word aligned */
	int	msg_magic;			/* magic number */
	int	fill;		/* must be quad word in size */
};

#ifdef KERNEL
#ifndef LOCORE
extern struct MACHstate *mach_states;
extern struct MACHstate *mach_state[];
#define MY_MACH_STAT() (mach_state[cpu_number()])
#endif
#endif
