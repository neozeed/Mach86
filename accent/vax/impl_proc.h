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
 * Package:	IPC
 *
 * File:	impl_proc.h
 *
 * Abstract:
 *	This file is a merge of the vmipctypes.h and vmtypes.h of old, BUT
 *	the applications level definitions have been pulled out into
 *	ipc.h.   Since the ProcRecord is machine dependent it has been put in 
 *	the separate file, proc.h.
 *
 * Author:	Robert V. Baron & Mike B. Jones
 *		Copyright (c) 1984 by Robert V. Baron
 *		Copyright (c) 1984 by Mike B. Jones
 *
 * HISTORY
 *  6-May-86  Rick Rashid (rfr) at Carnegie-Mellon University
 *	Added TimerSet field plus dummy fields to ProcRecord.
 *
 *  7-Mar-86  Bill Bolosky (bolosky) at Carnegie-Mellon University
 *	Added field 'trash' to ProcRec when ROMP is on to correct
 *	alignment problems.
 *
 * 24-Feb-86  Bill Bolosky (bolosky) at Carnegie-Mellon University
 *	Caused procp, u_uid, u_pid to be turned on when compiling on
 *	romp under switch ROMP.
 *
 * 25-Sep-84  Robert V Baron (rvb) at Carnegie-Mellon University
 *	Added processor element to IPCrecord and ProcRec structures, added
 *	some fill, and included the PCBHandle in the IPCrecord
 *
 * Origins:	"<accent>/vmipctypes.pas"
 *  Copyright (C) 1981  Richard F. Rashid and Carnegie-Mellon University
 *  Copyright (C) 1982  Richard F. Rashid and Carnegie-Mellon University
 *  Copyright (C) 1984  Richard F. Rashid and Carnegie-Mellon University
 * Most Recently Changed:
 *  23-Jan-84  Michael B. Jones
 *             Added arg definitions for LockPorts and MessagesWaiting.
 */

#include "../h/task.h"

/*
 *    Purpose
 *       Defines per process IPC data structures found in pcb.
 */

	/*
	 *	This structure must be quad word aligned since
	 * elements are drectly linked on a interlocked queue.
	 */

typedef struct ProcRecord {
#ifndef vax
		port_		DebugPort;	/* these two entries link Q */
		port_		LimitPort;     	/* members on free list */
#endif vax
		queue_head_t	MsgsWaiting;	/* this cell is not zeroed by
						 * IPCActivate when a record
						 * is taken from the free list
						 */
		lock_data_t	ProcLock;
		port_		KPorts[FirstNonReservedPort];
					   /* 0'th (ie. NULLPORT) is wasted */
					   /* now used for WaitPt.	*/
		proc_state_	State;
		short_bool	Active;
		short		WaitOption;
		short_bool	TimerSet;
		short		dummy1;		/* Force quad word alignment */
		kmsgp_		PreviewMsg;
		struct PCBHandle pcb;
#if defined(vax) || defined(romp)
		task_t		task;
		short		dummy2;		/* Force quad-word size */
		short		dummy3;
#endif vax || romp
};

#define	WaitPt KPorts[0]

typedef struct ProcRecord ProcSize[NumProcs];
