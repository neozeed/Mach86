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
 *	File:	remote_subr.c
 *	Author:	Avadis Tevanian, Jr.
 *
 *	Copyright (C) 1984, Avadis Tevanian (rvb)		CMU
 *
 *	Routines for calling (and handling) remote services to/from other
 *	processors.
 *
 * HISTORY
 * 24-Feb-85	Robert V Baron
 *	Added unasleep and untimeout
 *
 * 29-Sep-84  Avadis Tevanian (avie) at Carnegie-Mellon University
 *	Added remote psignal.
 *
 * 27-Sep-84  Avadis Tevanian (avie) at Carnegie-Mellon University
 *	Created.  (remote wakeups handled)
 *
 */

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/dir.h"
#include "../h/user.h"
#include "../h/proc.h"

#include "../sync/mp_queue.h"
#include "../mp/remote_sw.h"
#include "../mp/remote_prim.h"

remote_wakeup(processor, channel)
	int processor;
	caddr_t channel;
{
	request_processor(processor, TYPE_WAKEUP, channel);
}

handle_remote_wakeup(msg)
	struct msg_buffer *msg;
{
	wakeup((caddr_t) msg->param1);
}

remote_psignal(processor, p, sig)
	register int processor;
	register struct proc *p;
	register int sig;
{
	request_processor(processor, TYPE_PSIGNAL, p, sig);
}

handle_remote_psignal(msg)
	register struct msg_buffer *msg;
{
	psignal((struct proc *) msg->param1, (int) msg->param2);
}

remote_unasleep(processor, channel)
{
	request_processor(processor, TYPE_UNASLEEP, channel);
}

handle_remote_unasleep(msg)
	register struct msg_buffer *msg;
{
	extern int asleepexpire();

	untimeout(asleepexpire, (int) msg->param1);
}

remote_untimeout(processor, fun, arg)
	int (*fun)();
{
	request_processor(processor, TYPE_UNTIMEOUT, fun, arg);
}

handle_remote_untimeout(msg)
	register struct msg_buffer *msg;
{
	untimeout(msg->param1, (int) msg->param2);
}
