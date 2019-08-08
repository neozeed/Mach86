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
 * File:	acc_debug.h
 *
 * Abstract:
 *	Define bit mask values to turn on tracing of IPC activity in the 
 *	kernel.
 *
 * Author:	Robert V. Baron
 *		Copyright (c) 1984 by Robert V. Baron
 *
 * HISTORY
 */

#define DBG(x) if (ipc_debug & DBG_/**/x) 

/* 
 *			Commands
 */

#define DBG_Send	0x1
#define DBG_Receive	0x2
#define DBG_Room	0x4
#define DBG_Queue	0x8

#define DBG_flow	0x10
#define DBG_SW		0x20
#define DBG_Move	0x40
#define DBG_Ptr		0x80

/*
 *			Port translations
 */
#define DBG_Tran		0x0100
#define DBG_FTran		0x0200
#define DBG_Copying		0x0400

/* 
 *			Storage Allocation
 */
#define DBG_MKMsgAllocate	0x00010000
#define DBG_DataAlloc		0x00020000
/*
#define DBG_BlockMove		0x00040000
#define DBG_MoveWords		0x00080000
*/

/* 
 *			Message en/de quing
 */
#define DBG_Queued		0x00100000
/*
#define DBG_enqueue		0x00400000
*/
#define DBG_dequeue		0x00800000

#define DBG_XX			0x80000000

extern int ipc_debug;
