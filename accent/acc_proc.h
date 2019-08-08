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
 * acc_proc.h
 *
 * Types for Accent process management calls.
 *
 * Michael B. Jones
 *
 * Copyright (C) 1981, 1982, 1983, 1984, 1985
 *	Richard F. Rashid, Michael B. Jones, and Carnegie-Mellon University
 *
 * History:
 *
 *  15-Aug-85	Michael B. Jones
 *		Merged in Perq definitions for s5.b2 release.
 *
 *  14-Aug-85	Michael B. Jones
 *		Started.
 */

#ifndef _Acc_Proc
#define _Acc_Proc

typedef short	PriorID;        /* 0..NumPriorities-1 */
typedef short	QID;            /* 0..NumQueues-1 */


typedef short	ProcState;

#define SupervisorState	0    /* 00 - supervisor with privileges */
#define PrivilegedState	1    /* 01 - user with privileges */
#define BadSupervisor	2    /* 10 - supervisor without privileges */
#define UserState	3    /* 11 - user without privileges */


typedef struct
	{
	ProcState	State;
	PriorID		Priority;
	short_bool	MsgPending;
	short_bool	EMsgPending;
	short_bool	MsgEnable;
	short_bool	EMsgEnable;
	short_bool	LimitSet;
	short_bool	SVStkInCore;
	QID		QueueID;
	Pointer		SleepID;
	long		RunTime;
	long		LimitTime;
	}
    PStatus;


#endif	_Acc_Proc
