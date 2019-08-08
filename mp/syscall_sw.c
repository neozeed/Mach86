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
 * HISTORY
 *  2-Jun-86  Michael Young (mwyoung) at Carnegie-Mellon University
 *	Added traps to get important kernel ports.
 *
 *  1-Jun-86  Avadis Tevanian (avie) at Carnegie-Mellon University
 *	Conditional compilation for kernel semaphores under MACH_SEM.
 *
 * 28-Apr-86  Avadis Tevanian (avie) at Carnegie-Mellon University
 *	Cleaned up some stuff... conditionalized the MACH_SHM stuff (will
 *	be purged later), removed bogus timekeeper stuff.
 *
 * 13-Jan-86  Robert V Baron (rvb) at Carnegie-Mellon University
 *	Add sensor call for mll
 *
 *  8-Jan-86  Avadis Tevanian (avie) at Carnegie-Mellon University
 *	Changed timemode to set_timemode and added get_timemode.
 *
 *  5-Dec-85  Avadis Tevanian (avie) at Carnegie-Mellon University
 *	Fix to conditionally compile with timing code on or off
 *	(MACH_TIME).
 *
 * 15-Aug-85  Avadis Tevanian (avie) at Carnegie-Mellon University
 *	Add TPswtch and TPsystem_name.
 */

#include "mach_acc.h"
#include "mach_sem.h"
#include "mach_shm.h"
#include "mach_time.h"
#include "wb_sens.h"
#include "mach_vm.h"
#include "mach_mach.h"

#include "../h/types.h"

#include "../mp/syscall_sw.h"

#if	MACH_VM
#include "../h/kern_return.h"
#endif	MACH_VM
#if	MACH_MACH
#include "../h/port.h"
#endif	MACH_MACH

/*
 *	Random Hacks
 */

extern int TPDummy(/* */);
extern int TPSuccess(/* */);
extern int TPFailure(/* */);
extern int TPgetpid(/* *pid */);
extern int TPread(/* fdes, *cbuf, count */);
extern int TPnm(/* indicator, *va */);

/*
 *	InterMachine Communication (IMC) routines
 */

extern int send_imc_msg();
extern int receive_imc_msg();
extern int get_cpu_number();

#if	MACH_SEM
/*
 *	Kernel semaphores.
 */
extern int TPP(/* (s)emaphore */);
extern int TPV(/* (s)emaphore */);
extern int TPallocate_semaphore(/* *(s)emaphore */);
extern int TPdeallocate_semaphore(/* (s)emaphore */);
#endif	MACH_SEM

#if	MACH_SHM
/*
 *	Shared Memory Interface
 */
extern int TPshmalloc(/* *ptr, size */);
extern int TPshmfree  (/* ptr, size */);
extern int TPshmlimit(/* limit */);
#endif	MACH_SHM

#if	MACH_ACC
/*
 *	Accent Trap Interface
 */

extern int TPCheat(/* lp */);
extern int TPaccentinit(/* NPROC, NPORTS */);
extern int TPInitPort(/* prt, MaxcBackLog */);
extern int TPSend(/* msgptr, MaxWait, Option */);
	/*extern int TPSetPortsWaiting( Ports );*/
extern int TPReceive(/* msgptr, MaxWait, PtOption, Option */);
	/*extern int TPPortsWithMessages(MsgType, ports);*/
extern int TPMoveWords(/* SrcAddr, DstAddr, NumWords, Delete, Create, Mask, DontShare */);
extern int TPSoftEnable(/* NormalOrEmergency, EnableOrDisable */);
extern int TPLockPorts (/* LockDoLock, LockPortPtr, LockPortCnt */);
extern int TPKPortToPID();

/*
 * The normal accent interface for MessagesWaiting has
 * three arguments and returns var parameters
 */

extern int TPMessagesWaiting(/* MsgWType, MsgWPortPtr, MsgWPortCnt, ActualMsgWPortCnt */);
#endif	MACH_ACC
#if	MACH_TIME
extern int TPgetutime();
extern int TPset_timemode();
extern int TPget_timemode();
#endif	MACH_TIME

#if	NWB_SENS > 0
extern int monitor();
#endif	NWB_SENS > 0

#if	MACH_MACH
extern	port_t	task_self();
extern	port_t	task_data();
extern	port_t	task_notify();
extern	port_t	thread_self();

extern	port_t	ns_port();
#endif	MACH_MACH

/*
 *	More random hacks.
 */

extern int	TPswtch();
extern int	TPsystem_name();

struct mp_sysent mp_sysent[] = {
/*	FN(TPnewentry, arg#),				/* 0ffset */

	FN(TPDummy, 0),				/* 0 */
	FN(TPDummy, 0),				/* 1 */
	FN(TPDummy, 0),				/* 2 */
	FN(TPDummy, 0),				/* 3 */
	FN(TPDummy, 0),				/* 4 */
	FN(TPDummy, 0),				/* 5 */
	FN(TPDummy, 0),				/* 6 */
	FN(TPDummy, 0),				/* 7 */
	FN(TPDummy, 0),				/* 8 */
	FN(TPDummy, 0),				/* 9 */
#if	MACH_SEM
/*
 *	Kernel semaphores.
 */
	FF1(TPP, 1, ASETJMP),			/* 10 */
	FN(TPV, 1),				/* 11 */
	FN(TPallocate_semaphore, 1),		/* 12 */
	FN(TPdeallocate_semaphore, 1),		/* 13 */
	FN(TPDummy, 0),				/* 14 */
#else	MACH_SEM
	FN(TPDummy, 0),				/* 10 */
	FN(TPDummy, 0),				/* 11 */
	FN(TPDummy, 0),				/* 12 */
	FN(TPDummy, 0),				/* 13 */
	FN(TPDummy, 0),				/* 14 */
#endif	MACH_SEM
#if	MACH_SHM
/*
 *	Shared Memory Interface
 */
	FN(TPshmalloc, 2),			/* 15 */
	FN(TPshmfree, 2),			/* 16 */
	FN(TPshmlimit, 1),			/* 17 */
#else	MACH_SHM
	FN(TPDummy, 0),				/* 15 */
	FN(TPDummy, 0),				/* 16 */
	FN(TPDummy, 0),				/* 17 */
#endif	MACH_SHM
	FN(TPDummy, 0),				/* 18 */
	FN(TPDummy, 0),				/* 19 */

#if	MACH_ACC
/*
 *	Accent Trap Interface
 */
	FN(TPCheat, 1),				/* 20 */
	FN(TPaccentinit, 2),			/* 21 */
	FN(TPInitPort, 2),			/* 22 */
	FF1(TPSend, 3, APSIG),			/* 23 */
	FN(TPDummy/*TPSetPortsWaiting*/, 1),	/* 24 */
	FF1(TPReceive, 4, APSIG),		/* 25 */
	FN(TPDummy, 0),				/* 26 */
	FN(TPDummy/*TPPortsWithMessages*/, 2),	/* 27 */
	FN(TPMoveWords, 7),			/* 28 */
	FN(TPSoftEnable, 2),			/* 29 */
	FN(TPLockPorts, 3),			/* 30 */
	FN(TPMessagesWaiting, 4),		/* 31 */
	FN(TPKPortToPID, 1),			/* 32 */
	FN(TPDummy, 0),				/* 33 */
	FN(TPDummy, 0),				/* 34 */
	FN(TPDummy, 0),				/* 35 */
	FN(TPDummy, 0),				/* 36 */
	FN(TPDummy, 0),				/* 37 */
	FN(TPDummy, 0),				/* 38 */
	FN(TPDummy, 0),				/* 39 */
#else	MACH_ACC
	FN(TPDummy, 0),				/* 20 */
	FN(TPDummy, 0),				/* 21 */
	FN(TPDummy, 0),				/* 22 */
	FN(TPDummy, 0),				/* 23 */
	FN(TPDummy, 0),				/* 24 */
	FN(TPDummy, 0),				/* 25 */
	FN(TPDummy, 0),				/* 26 */
	FN(TPDummy, 0),				/* 27 */
	FN(TPDummy, 0),				/* 28 */
	FN(TPDummy, 0),				/* 29 */
	FN(TPDummy, 0),				/* 30 */
	FN(TPDummy, 0),				/* 31 */
	FN(TPDummy, 0),				/* 32 */
	FN(TPDummy, 0),				/* 33 */
	FN(TPDummy, 0),				/* 34 */
	FN(TPDummy, 0),				/* 35 */
	FN(TPDummy, 0),				/* 36 */
	FN(TPDummy, 0),				/* 37 */
	FN(TPDummy, 0),				/* 38 */
	FN(TPDummy, 0),				/* 39 */
#endif	MACH_ACC
	FN(TPDummy, 0),					/* 40 */
	FN(TPDummy, 0),					/* 41 */
	FN(TPDummy, 0),					/* 42 */
	FN(TPDummy, 0),					/* 43 */
	FN(TPDummy, 0),					/* 44 */
	FN(TPDummy, 0),					/* 45 */
	FN(TPDummy, 0),					/* 46 */
	FN(TPDummy, 0),					/* 47 */
	FN(TPDummy, 0),					/* 48 */
	FN(TPDummy, 0),					/* 49 */
/*
 *	More syscalls for PIE and others
 */
	FN(TPDummy, 0),					/* 50 */
	FN(TPDummy, 0),					/* 51 */
	FN(TPDummy, 0),					/* 52 */
#if	MACH_TIME
	FN(TPgetutime, 2),				/* 53 */
	FN(TPset_timemode, 1),				/* 54 */
	FN(TPget_timemode, 1),				/* 55 */
#else	MACH_TIME
	FN(TPDummy, 0),					/* 53 */
	FN(TPDummy, 0),					/* 54 */
	FN(TPDummy, 0),					/* 55 */
#endif	MACH_TIME
	FN(TPDummy, 0),					/* 56 */
	FN(TPDummy, 0),					/* 57 */
	FN(TPDummy, 0),					/* 58 */
	FN(TPDummy, 0),					/* 59 */
/*
 *	More random hacks.
 */
	FN(TPswtch, 0),					/* 60 */
	FN(TPsystem_name, 2),				/* 61 */
/*
 *	Random Hacks
 */
	FN(TPgetpid, 1),				/* 62 */
	FN(TPnm, 2),					/* 63 */
	FN(send_imc_msg, 4),				/* 64 */
	FF1(receive_imc_msg, 4, ASETJMP),		/* 65 */
	FN(get_cpu_number, 1),				/* 66 */

	FN(TPDummy, 0),					/* 67 */
	FN(TPDummy, 0),					/* 68 */
	FN(TPDummy, 0),					/* 69 */
#if	NWB_SENS > 0
	FN(monitor, 1),					/* 70 */
#else	NWB_SENS > 0
	FN(TPDummy, 0),					/* 70 */
#endif	NWB_SENS > 0
/*
 *	Space for old VM traps
 */
	FN(TPDummy, 0),					/* 71 */
	FN(TPDummy, 0),					/* 72 */
	FN(TPDummy, 0),					/* 73 */
	FN(TPDummy, 0),					/* 74 */
	FN(TPDummy, 0),					/* 75 */

#if	MACH_MACH
	FN(task_self, 0),				/* 76 */
	FN(task_data, 0),				/* 77 */
	FN(task_notify, 0),				/* 78 */
	FN(thread_self, 0),				/* 79 */
	FN(ns_port, 0),					/* 80 */
#else	MACH_MACH
	FN(TPDummy, 0),					/* 76 */
	FN(TPDummy, 0),					/* 77 */
	FN(TPDummy, 0),					/* 78 */
	FN(TPDummy, 0),					/* 79 */
	FN(TPDummy, 0),					/* 80 */
#endif	MACH_MACH
};

int nmp_sysent = (sizeof mp_sysent / sizeof (struct mp_sysent) );

/*newentry: and lastly add the mp_sysent(newentry) to 
	/usr/mach/src/lib/libmach/mach_traps.c and recompile */
