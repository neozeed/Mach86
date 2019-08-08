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
 *	File:	remote_prim.c
 *
 *	Copyright (C) 1984, Avadis Tevanian, Jr.
 *
 *	Machine independent procedures for processor-to-processor
 *	communications at the lowest level.  (Just above the hardware)
 *
 * HISTORY
 * 11-Oct-85  Robert V Baron (rvb) at Carnegie-Mellon University
 *	Now Uses ../mp/mach_stat to define the data areas that it needs
 *	on a per machine basis.  Check it out.
 *
 * 28-Jun-85  Avadis Tevanian (avie) at Carnegie-Mellon University
 *	Properly handle the case when we are running on a uniprocessor.
 *
 * 20-Sep-84  Avadis Tevanian (avie) at Carnegie-Mellon University
 *	Created.
 *
 */

#include "wb_ml.h"
#include "mach_mpm.h"

#include "../h/map.h"
#include "../h/param.h"
#include "../h/systm.h"
#include "../h/dir.h"
#include "../h/user.h"
#include "../h/proc.h"
#include "../h/reboot.h"

#include "../machine/cpu.h"
#include "../sync/mp_queue.h"
#include "../mp/remote_sw.h"
#include "../mp/remote_prim.h"
#include "../mp/mach_stat.h"

struct MACHstate *mach_state[NCPUS];
struct MACHstate *mach_states;

mach_state_init()
{
	register int i;

	for (i = 0; i < NCPUS; i++) {
		mach_state[i] = &mach_states[i];
	}
}

static comm_init = 0;
/*
 *	Initialize local status memory for communications
 */

mp_msg_init(addr, len)
	caddr_t addr;
	int len;
{
	int nq;

	initmpqueue(&MY_MACH_STAT()->msg_free);
	initmpqueue(&MY_MACH_STAT()->msg_pending);

	/*
	 * break up our memory into buffer regions and enqueue them
	 * all on msg_free queue
	 */

	nq = 0;
	while (len >= sizeof(struct msg_buffer)) {
		mpenqueue_tail(&MY_MACH_STAT()->msg_free, addr);
		addr += sizeof(struct msg_buffer);
		len -= sizeof(struct msg_buffer);
		nq++;
	}
	MY_MACH_STAT()->msg_magic = MP_MSG_MAGIC;
	if (show_space) {
		printf("communications area allocated: ");
		printf("%d entries using %d bytes.\n", nq,
				nq*sizeof(struct msg_buffer));
	}
	comm_init = 1;
}

/*
 *	Send a message to a processor.  An interrupt is generated if
 *	this is the first message on the destination processors queue.
 */

/*VARARGS*/
request_processor(processor, type, param1, param2, param3, param4)
	int processor, type;
	long param1, param2, param3, param4;
{
	struct msg_buffer *msg;
	static nprints = 0;
#if	NMACH_MPM > 0
	extern nummpm;
#endif

	if (processor >= 4) return;
 	if (mach_state[processor]->msg_magic != MP_MSG_MAGIC) {
/*		printf("Processor %d running wrong version software.\n",
				processor);*/
		return;
	}

	/* get a buffer */

	mpdequeue_head(&mach_state[processor]->msg_free, &msg);

	if (msg == NULL) {
		if (nprints < 5)
			printf("No free buffers for processor %d.\n",
					processor);
		nprints++;
		return;
	}
	msg->to = processor;
	msg->from = cpu_number();
	msg->type = type;
	msg->param1 = param1;
	msg->param2 = param2;
	msg->param3 = param3;
	msg->param4 = param4;
	if (mpenqueue_tail(&mach_state[processor]->msg_pending, msg))
#if	NMACH_MPM > 0
		if (nummpm > 0) {
			interrupt_processor(processor);
		}
		else {
			mp_rint();		/* simulate */
		}
#else
		printf("Don't know how to interrupt other processor!\n");
#endif
}

/*
 *	Process an interrupt from another processor.
 */

mp_rint()
{
	struct msg_buffer *msg;
	extern struct mp_comm_entry mp_comm_ent[];
	register int type;

	if (!comm_init)
		return;
	/*
 	 * Pull next request off the queue.
	 */

	while (1) {
		mpdequeue_head(&MY_MACH_STAT()->msg_pending, &msg);
		if (msg == NULL)
			break;
		if (msg->to != cpu_number()) {
			printf("Received a message not for me!\n");
			printf("Addr = %x, To: = %d, From = %d, Type = %d.\n",
					msg, msg->to, msg->from, msg->type);
			continue;
		}
		type = msg->type;
		if ((type >= 0) && (type < NCOMMTYPES)) {
				(*mp_comm_ent[type].mp_call)(msg);
		}

	/*	put buffer back on my free queue */

		mpenqueue_tail(&MY_MACH_STAT()->msg_free, msg);
	}
}

nomp()
{
}
