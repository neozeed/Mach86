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
 * Package:	IPC
 *
 * File:	acc_errorstrs.h
 *
 * Abstract:
 *	This files contains a string name corresponding to every "important"
 * 	manifest constant so we can encourage people to printout the reason
 *	things go bump.
 *
 * Author:	Robert V. Baron
 *
 *		Copyright (c) 1984 by Robert V. Baron
 * HISTORY
 * 11-Dec-85  Robert V Baron (rvb) at Carnegie-Mellon University
 *	Timing error valeus
 *
 */


/*
 *		for the file acc_ipc.h
 */

char *msgtype_strs[] = {
/* 0 */		/* NORMAL_MSG */		"Normal Message",
/* 1 */ 	/* EMERGENCY_MSG */		"Emergency Message"
};

char *send_option_strs[] = {
/* 0 */		/* WAIT */			"Wait",
/* 1 */		/* DONT_WAIT */			"Dont Wait",
/* 2 */		/* REPLY */			"Reply"
};

char *receive_option_strs[] = {
/* 0 */		/* PREVIEW */			"Preview",
/* 1 */		/* RECEIVE_IT */		"Receive it",
/* 2 */		/* RECEIVE_WAIT */		"Receive Wait"
};

char *port_options_strs[] = {
/* 0 */		/* DEFAULT_PORTS */		"Default Ports",
/* 1 */		/* ALL_PORTS */			"All Ports",
/* 2 */		/* LOCAL_PORTS */		"Local Ports"
};

char *distinguished_port_strs[] = {
/* 0 */		/* NULL_PORT */			"Null Port",
/* 1 */		/* KERNEL_PORT */		"Kernel Port",
/* 2 */		/* DATA_PORT */			"Data Port",
/* 3 */						"Name Server"
};

char *deallocate_reason_strs[] = {
/* 0 */		/* EXPLICIT_DEALLOC */		"Explicit Deallocation",
/* 1 */		/* PROCESS_DEATH */		"Process Death",
/* 2 */		/* NETWORK_TROUBLE */		"Network Trouble"
};

char  *type_strs[] = {
/* 0 */		/* TYPE_UNSTRUCTURED */		"unstructured",
/* 1 */		/* TYPE_INT16 */		"int16",
/* 2 */		/* TYPE_INT32 */		"int32",
/* 3 */		/* TYPE_PT_OWNERSHIP */		"pt ownership",
/* 4 */		/* TYPE_PT_RECEIVE */		"pt receive",
/* 5 */		/* TYPE_PT_ALL */		"pt all",
/* 6 */		/* TYPE_PT */			"pt",
/* 7 */		/*  */				"not in use",
/* 8 */		/* TYPE_CHAR */			"char",
/* 9 */		/* TYPE_INT8 */			"int8",
/* 10 */	/* TYPE_REAL */			"real",
/* 11 */	/* TYPE_PSTAT */		"pstat",
/* 12 */	/* TYPE_STRING */		"string",
/* 13 */	/* TYPE_SEGID */		"segid"
};

/*
 *			for the file kern_ipc.h
 */

char *send_type_strs[] = {
/* 0 */		/* DONT_WAKE */			"Dont Wake",
/* 1 */		/* SEND_ACK */			"Send Acknowledge",
/* 2 */		/* WAKE_ME */			"Wake Me"
};

char *proc_state_strs[] = {
/* 0 */		/* NOT_WAITING */		"Not Waiting",
/* 1 */		/* FULL_WAIT */			"Full Wait",
/* 2 */		/* MSG_WAIT */			"Msg Wait",
/* 3 */		/* TIMED_OUT */			"Timed Out"
};


/*
 *			for the file acc_errors.h
 */

char *accent_errors[] = {
/* +0 */	/* DUMMY */			"Dummy",
/* +1 */	/* SUCCESS */			"Success",
/* +2 */	/* TIME_OUT */			"Time Out",
/* +3 */	/* PORT_FULL */			"Port Full",
/* +4 */	/* WILL_REPLY */		"Will Reply",
/* +5 */	/* TOO_MANY_REPLIES */		"Too Many Replies",
/* +6 */	/* MEM_FAULT */			"Mem Fault",
/* +7 */	/* NOT_PORT */			"Not A Port",
/* +8 */	/* BAD_RIGHTS */		"Bad Rights",
/* +9 */	/* NO_MORE_PORTS */		"No More Ports",
/* +10 */	/* ILLEGAL_BACKLOG */		"Illegal Backlog",
/* +11 */	/* NET_FAIL */			"Net Fail",
/* +12 */	/* INTR */			"Intr",
/* +13 */	/* OTHER */			"Other",
/* +14 */	/* NOT_PORT_RECEIVER */		"Not Port Receiver",
/* +15 */	/* UNRECOGNIZED_MSGTYPE */	"Unrecognized Msg Type",
/* +16 */	/* NOT_ENOUGHROOM */		"Not Enough Room",
/* +17 */	/* NOT_IPC_CALL */		"Not An IPC Call",
/* +18 */	/* BAD_MSG_TYPE */		"Bad Msg Type",
/* +19 */	/* BAD_IPC_NAME */		"Bad IPC Name",
/* +20 */	/* MSG_TOO_BIG */		"Msg Too Big",
/* +21 */	/* NOT_YOUR_CHILD */		"Not Your Child",
/* +22 */	/* BAD_MSG */			"Bad Msg",
/* +23 */	/* OUT_OF_IPC_SPACE */		"Out Of IPC Space",
/* +24 */	/* FAILURE */			"Failure",
/* +25 */	/* MAP_FULL */			"Map Full",
/* +26 */	/* WRITE_FAULT */		"Write Fault",
/* +27 */	/* BAD_KERNELMSG */		"Bad Kernel Msg",
/* +28 */	/* NOT_CURRENT_PROCESS */	"Not Current Process",
/* +29 */	/* CANT_FORK */			"Cant Fork",
/* +30 */	/* BAD_PRIORITY */		"Bad Priority",
/* +31 */	/* BAD_TRAP */			"Bad Trap",
/* +32 */	/* DISK_ERR */			"Disk Err",
/* +33 */	/* BAD_SEG_TYPE */		"Bad Seg Type",
/* +34 */	/* BAD_SEGMENT */		"Bad Segment",
/* +35 */	/* IS_PARENT */			"Is Parent",
/* +36 */	/* IS_CHILD */			"Is Child",
/* +37 */	/* NO_AVAILABLE_PAGES */	"No Available Pages",
/* +38 */	/* FIVE_DEEP */			"Five Deep",
/* +39 */	/* BAD_VP_TABLE */		"Bad VP Table",
/* +40 */	/* VP_EXCLUSION_FAILURE */	"VP Exclusion Failure",
/* +41 */	/* MICRO_FAILURE */		"Micro Failure",
/* +42 */	/* E_STACK_TOO_DEEP */		"E Stack Too Deep",
/* +43 */	/* MSG_INTERRUPT */		"Msg Interrupt",
/* +44 */	/* UNCAUGHT_EXCEPTION */	"Uncaught Exception",
/* +45 */	/* BREAKPOINT_TRAP */		"Break Point Trap",
/* +46 */	/* AST_INCONSISTENCY */		"AST Inconsistency",
/* +47 */	/* INACTIVE_SEGMENT */		"Inactive Segment",
/* +48 */	/* SEGMENT_ALREADY_EXISTS */	"Segment Already Exists",
/* +49 */	/* OUT_OF_IMAG_SEGMENTS */	"Out Of Imag Segments",
/* +50 */	/* NOT_SYSTEM_ADDRESS */	"Not A System Address",
/* +51 */	/* NOT_USER_ADDRESS */		"Not A User Address",
/* +52 */	/* BAD_CREATE_MASK */		"Bad Create Mask",
/* +53 */	/* BAD_RECTANGLE */		"Bad Rectangle",
/* +54 */	/* OUT_OF_RECTANGLE_BOUNDS */	"Out Of Rectangle Bounds",
/* +55 */	/* ILLEGAL_SCAN_WIDTH */	"Illegal Scan Width",
/* +56 */	/* COVERED_RECTANGLE */		"Covered Rectangle",
/* +57 */	/* BUSY_RECTANGLE */		"Busy Rectangle",
/* +58 */	/* NOT_FONT */			"Not A Font",
/* +59 */	/* PARTITION_FULL */		"Partition Full",

/* +60 */	/* NoSuchPage */		"No Such Page",
/* +61 */	/* NoRecoveryManager */		"No Recovery Manager",
/* +62 */	/* HaveRecManPortAlready */	"Have Recovery Manager Port Already",
/* +63 */	/* RecovSegError */		"Recovery Segment Error",
/* +64 */	/* FlushAborted */		"Flush Aborted",
/* +65 */	/* PageNotRecoverable */	"Page Not Recoverable",
/* +66 */	/* RecovQueueFull */		"Recovery Queue Full",
						"",
						"",
						"",

/* +70 */	/* BAD_INITPORT */		"Bad Init Port",
/* +71 */	/* INITPORT_IN_USE */		"Init Port In Use",
/* +72 */	/* NOT_SUPERUSER */		"Not Super User",
/* +73 */	/* OUT_OF_SHARED_MEMORY */	"Out of Shared Memory",
/* +74 */	/* PROCESS_SHARED_MEMORY_EXCEDED */ "Process Shared Memory Limit Reached",
/* +75 */	/* BAD_SHARED_MEMORY_ARG */	"Arg to Shmfree is not Shared Memory",
/* +77 */	/* SHARED_MEMORY_HUH */		"Internal Shared Memory Bug",
/* +77 */	/* BAD_SHARED_MEMORY_SIZE */	"Shmxxx() size arg <= 0",
/* +78 */	/* BAD_SHMFREE_RANGE */		"Range given to shmfree does not exist",
/* +79 */	/* BAD_RECEIVE_SIZE */		"Bad MsgSize in Receive trap",
/* +80 */	/* TYPE_ZERO */			"TYPEType is identically zero",
/* +81 */	/* TYPE_TOO_BIG */		"TypeType field exceeds MSG size",
/* +82 */	/* TimingOff */			"Timing is turned off",
/* +83 */	/* CantFindProc */		"Cant find process in proc table",
/* +84 */	/* TimingLocked */		"Cant turn timing on or off on a uVax"
};

char *matchmaker_errors[] = {
/* +0 */			 "",
/* +1 */	/* BAD_MSGID */			"Bad Message ID",
/* +2 */	/* WRONG_ARGS */		"Wrong Argument Type",
/* +3 */	/* BAD_REPLY */			"Bad Reply",
/* +4 */	/* NO_REPLY */			"No Reply",
/* +5 */	/* UNSPEC_EXCEPTION */		"Unspecified Exception"
};

char *kernel_msgid_strs[] = {
/* +0 */			 "",
/* +1 */	/* PORT_DELETED */		"Port Deleted",
/* +2 */	/* MSG_ACCEPTED */		"Msg Accepted",
/* +3 */	/* OWNERSHIP_RIGHTS */		"Ownership Rights",
/* +4 */	/* RECEIVE_RIGHTS */		"Receive Rights",
/* +6 */	/* GENERAL_KERNEL_REPLY */	"General Kernel Reply",
/* +7 */	/* KERNEL_MSG_ERROR */		"Kernel Msg Error",
/* +10 */	/* PARENT_FORK_REPLY */		"Parent Fork Reply",
/* +11 */	/* CHILD_FORK_REPLY */		"Child Fork Reply",
/* +12 */	/* DEBUG_MSG */			"Debug Msg"
};
