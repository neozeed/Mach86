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
 * File:	kern_ipc.h
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
 * 25-Mar-86  Richard F. Rashid
 *	Added test for Romp to Vax test.
 *
 * 11-Oct-85  Robert V Baron (rvb) at Carnegie-Mellon University
 *	These variables are stored under the IPC variable so that the "ipc"
 *	pgm can get at them by doing a single nm(NM_IPC,&var) call.
 *
 * 11-Oct-85  Robert V Baron (rvb) at Carnegie-Mellon University
 *	procs_ now uses the MAX_NumProcs so that the array will be "big"
 *	enough for some growth.
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
 *             Added arg definitions for LockPorts and ing.
 */

#ifndef ACCENT_OSIPC
#define ACCENT_OSIPC

#define NULL 0
/*
 *   Purpose:
 *       Miscellaneous constants of the implementation.
 */
#define KERNEL_PROCESS 0
#define PAGER_PROCESS 1
		/* Illegal process numbers */
#define RIGHTS_IN_TRANSIT (NumProcs-1)
#define NO_PROCESS (NumProcs-2)
#define LAST_USED_PROCESS (NumProcs-3)

#define pKERNEL_PROCESS (&ProcArray[KERNEL_PROCESS])
#define pPAGER_PROCESS (&ProcArray[PAGER_PROCESS])
#define pNO_PROCESS (&ProcArray[NO_PROCESS])
#define pRIGHTS_IN_TRANSIT (&ProcArray[RIGHTS_IN_TRANSIT])
#define pLAST_USED_PROCESS (&ProcArray[LAST_USED_PROCESS])

/*
 *		types lost in last merge with mbj
 */
typedef	Pointer ptr_;
typedef Port port_;
typedef Msg msg_, *msgp_;
typedef LongTypeType ltype_, *ltypep_;
typedef short return_;
/*
 *		forward referenced type definitions
 */

typedef struct KMsg *kmsgp_;
typedef struct PortRecord *portp_;
typedef struct ProcRecord *procp_;

typedef unsigned long procs_[ (MaxNumProcs + 31) >> 5];

/*
 *   Purpose:
 *       Size of simple (non-mapped) message entries.
 */
#define MaxKMsgSize (TotalKMsgSize - KMSG_HEADER_SIZE)

/*
 *   Purpose:
 *       Values of KMsg.What which indicate what a receiver
 *     should do to the process which sent a message which is
 *     being moved from the secondary to the primary queue of
 *     a port_.
 * 
 */

enum WhatType {DONT_WAKE, SEND_ACK, WAKE_ME}; 
typedef enum WhatType send_type_;

/*
 *   Purpose:
 *       Basic data structure for a message stored in the kernel.
 */
struct KMsg {
		mpqhead_t	qhead;
		long		DataSize;
		ptr_		DataPtr;
		procp_		WakeupProc;
		send_type_	What;
		msg_		MsgHeader;
		long		Data[1];
	/*
	 *	More data may follow up to a total of MAX_KMSG_SIZE
	 *	per kmsg_
	 */
};

#define KMSG_HEADER_SIZE (sizeof (struct KMsg) - sizeof (long) - sizeof (msg_))

struct Statistics {
	int MessagesSent;
	int ComplexMessages;
};

/*
 * Simple Descriptor for PackMsg1 and PackMsg2
 */
typedef struct {
	type_	type;
	long	data;
} simple_descriptor_;

	/*
	 *	This structure must be quad word aligned since
	 * elements are drectly linked on a interlocked queue.
	 */

typedef struct PortRecord {
		queue_head_t	SecondaryMsgQ;
		lock_data_t	PortLock;
		procp_		Owner;		/* these two entries link Q */
		procp_		Receiver;     	/* members on free list */    
		short		NumQueued;
		short		Backlog;
		short_bool	Locked;
		short_bool	InUse;
		procs_		Senders;
#if	defined(vax) || defined(romp)
		int		fill;
		int 		fill1;
#endif
};

typedef struct PortRecord PortSize[NumPorts];

/*
 *    Purpose
 *       States of a process may be in when using the
 *     IPC.  
 */

enum IPCState {NOT_WAITING, FULL_WAIT, MSG_WAIT, TIMED_OUT};
typedef enum IPCState proc_state_;

typedef struct PCBHandle {
		byte_bool	MsgPending;
		byte_bool 	EMsgPending;
		byte_bool 	MsgEnable;
		byte_bool 	EMsgEnable;
} *intr_;

/*
 *		The actual PROCESS structure is machine dependent.
 *		It is found in acc_proc.h
 *	So:
		include "../accent/vax/impl_proc.h"
 */

struct IPCstate {
	procp_ ProcArray;
	portp_ PortArray;
	kmsgp_ FreeKMsgQ;
	procp_ FreeProcQ;
	portp_ FreePortQ;
};

extern procp_ ProcArray, eProcArray;
extern portp_ PortArray, ePortArray;
extern queue_head_t FreeKMsgQ;
extern queue_head_t FreeProcQ;
extern queue_head_t FreePortQ;

extern lock_t KMsgLock;
extern lock_t FreeProcLock;
extern lock_t FreePortLock;

#ifdef	KERNEL
extern struct IPCstate *IPC;
extern procp_ *aproc;
extern long PermSegPort;
extern struct Statistics Statistics;
#endif	KERNEL

#endif ACCENT_OSIPC
