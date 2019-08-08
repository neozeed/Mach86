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
 * File:	acc_macro.h
 *
 * Abstract:
 *	This file is those macros necessary to support IPC
 *
 * Author:	Robert V. Baron
 *		Copyright (c) 1984 by Robert V. Baron
 *
 * HISTORY
 *
 * Origins:	"<accent>/accent.macro.pas"
 *  Copyright (C) 1984 
 *
 */

#ifndef	_ACC_MACROS_
#define	_ACC_MACROS_
#endif	_ACC_MACROS_

#include "mach_vm.h"
#ifndef	NPTEPG
#include "../h/param.h"
#include "../h/map.h"
#include "../h/dir.h"
#include "../h/user.h"
#include "../h/proc.h"
#include "../h/signal.h"
#endif	NPTEPG
#if	MACH_VM
#include "../vm/vm_kern.h"
#endif	MACH_VM

/*
 * macro:
 *	pidx(ipcp)
 *
 * Abstract:
 *	Return the accent process table index of the process.
 */

#define pidx(ipcp) (ipcp - ProcArray)


/*
 * Macro:
 *       GetKernelPort
 *
 * Abstract:
 *       Returns kernelport of a process
 *
 */
#define GetKernelPort(ipcp, lp)\
{\
	lp = ipcp->KPorts[KernelPort];\
}


/*
 * Macro:
 *       GetDataPort
 *
 * Abstract:
 *       Returns Dataport of a process
 *
 */
#define GetDataPort(ipcp, lp)\
{\
	lp = ipcp->KPorts[DataPort];\
}

#define GiveOwnership(gp, ipcp)\
PortArray[gp].Owner = ipcp;



/*
 * Macro:
 *       FastGTran
 *
 * Abstract:
 *       Translates a proc, gp pair into a local port number (lp)
 */
#define FastGTran(ipcp, lp, gp)\
{\
	_bit_set(pidx(ipcp), PortArray[gp].Senders);\
	if (gp == ipcp->KPorts[DataPort]) {\
		lp = DataPort;\
	} else if (gp == ipcp->KPorts[KernelPort]) {\
		lp = KernelPort;\
	} else\
		lp = gp;\
}

/*
 * Macro:
 *       FastLTran
 *
 * Abstract:
 *       Translates a proc, lp pair into a port p (gp)
 */
#define FastLTran(ipcp, lp, gp)\
{\
	if ((lp < FirstNonReservedPort)) {\
		if (lp < KernelPort) {\
			gp = NullPort;\
		} else {\
			gp = ipcp->KPorts[lp];\
		}\
	} else {\
		if ( (lp < NumPorts)) {\
			if (_bit_tst(pidx(ipcp), PortArray[lp].Senders)) gp =  lp;\
			else if (lp <= LastInitPort && PortArray[lp].InUse) {\
				_bit_set(pidx(ipcp), PortArray[lp].Senders);\
				gp =  lp;\
			} else gp = NullPort;\
		} else gp = NullPort;\
	}\
}

/*  SoftOnOrOff */

#define SoftOnOrOff(ipcp, NormOrEmerg, OnOrOff) \
{\
      if (ipcp <= pLAST_USED_PROCESS)\
          if (NormOrEmerg)\
	  	ipcp->pcb.MsgEnable = OnOrOff;\
	  else\
            	ipcp->pcb.EMsgEnable = OnOrOff;\
}
/*
 * macro:
 *	CurrentProcess
 *
 * Abstract:
 *	Return the accent process table p of the current process.
 */

#define CurrentProcess() (aproc[u.u_procp-proc])

/*
 * Macro:
 *       MKMsgAllocate,MKMsgFree
 *
 * Abstract:
 *	Allocate or free a kernel message structure from the free list.
 *
 */
#if	MACH_VM
#define MKMsgAllocate(kmsg)\
{\
	if ((kmsg = (kmsgp_)dequeue_head(&FreeKMsgQ)) == NULL) \
		kmsg = (kmsgp_) kmem_alloc(ipc_map, TotalKMsgSize, TRUE);\
}

#define MKMsgFree(kmsgptr)\
{	enqueue_head(&FreeKMsgQ, kmsgptr);\
}
#else	MACH_VM
#define MKMsgAllocate(kmsg)\
{ \
	if ((kmsg = (kmsgp_)dequeue_head(&FreeKMsgQ)) == NULL) \
		kmsg = (kmsgp_) shmustalloc(TotalKMsgSize);\
}

#define MKMsgFree(kmsgptr)\
{register ptr_ vaddr = kmsgptr->DataPtr;\
	if ((int) vaddr && vaddr != (ptr_) kmsgptr->Data) {\
		shmfree(vaddr, kmsgptr->DataSize);\
	}\
	enqueue_head(&FreeKMsgQ, kmsgptr);\
}
#endif	MACH_VM

#define SetSoft(ipcp, NormOrEmerg)\
{\
	/* true is Emerg */\
	psignal(ipcp->task->proc, NormOrEmerg ? SIGEMSG : SIGMSG);\
}

#ifndef	LOCORE
extern asleepexpire();
#endif	LOCORE

/*
 * Macro:
 *	IPCWakeup, IPCWait
 *
 * Abstract:
 *	Wakeup or sleep (in IPC land).
 *
 */
#if	MACH_VM
#define IPCWakeup(ipcp) \
{ \
	register struct proc *p; \
	register int s; \
	s = splhigh(); \
	p = ipcp->task->proc; \
	if (((ipcp->State == MSG_WAIT) || (ipcp->State == FULL_WAIT))&& (p->p_stat == SSLEEP)) { \
		if (ipcp->TimerSet) { \
			untimeout(asleepexpire, ipcp); \
			ipcp->TimerSet = FALSE; \
		} \
		ipcp->State = NOT_WAITING; \
		setrun(p); \
		(void) splx(s); \
	}  else (void) splx(s);\
}


#define IPCWait(maxwait, waitstate, wake_reason) \
{ \
	register procp_ ipcp = CurrentProcess(); \
	register struct proc *p; \
	register int s; \
	s = splhigh(); \
	ipcp->State = waitstate; \
	if (maxwait != 0) { \
		maxwait = (maxwait + 9) / 10; \
		ipcp->TimerSet = TRUE; \
		timeout(asleepexpire, ipcp, maxwait); \
	}  \
	else { \
		ipcp->TimerSet = FALSE; \
	} \
	p = ipcp->task->proc; \
	p->p_stat = SSLEEP; \
	(void) spl0(); \
	u.u_ru.ru_nvcsw++; \
	unix_swtch((struct proc *) 0, 0); \
	*(wake_reason) = ISSIG(p); \
	(void) splx(s); \
	if (*(wake_reason)) \
		*(wake_reason) = -1; \
	else \
		*(wake_reason) = (ipcp->State == NOT_WAITING); \
	if (ipcp->TimerSet) { \
		untimeout(asleepexpire,ipcp); \
		ipcp->TimerSet = FALSE; \
	} \
}

#else	MACH_VM
#define IPCWakeup(ipcp) \
{ \
	register struct proc *p; \
	register int s; \
	s = splhigh(); \
	p = ipcp->task->proc; \
	if (((ipcp->State == MSG_WAIT) || (ipcp->State == FULL_WAIT))&& (p->p_stat == SSLEEP)) { \
		if (ipcp->TimerSet) { \
			untimeout(asleepexpire, ipcp); \
			ipcp->TimerSet = FALSE; \
		} \
		ipcp->State = NOT_WAITING; \
		/* Not in core, use setrun */ \
		if ((p->p_flag & SLOAD)==0) { \
			(void) splx(s); \
			setrun(p); \
			return; \
		} else { \
			(void) splx(s); \
			setrun(p); \
		} \
	} else (void) splx(s);\
}

#define IPCWait(maxwait, waitstate, wake_reason) \
{ \
	register procp_ ipcp = CurrentProcess(); \
	register struct proc *p; \
	register int s; \
	s = splhigh(); \
	ipcp->State = waitstate; \
	if (maxwait != 0) { \
		maxwait = (maxwait + 9) / 10; \
		ipcp->TimerSet = TRUE; \
		timeout(asleepexpire, ipcp, maxwait); \
	}  \
	else { \
		ipcp->TimerSet = FALSE; \
	} \
	p = ipcp->task->proc; \
	p->p_stat = SSLEEP; \
	(void) spl0(); \
	u.u_ru.ru_nvcsw++; \
	swtch(); \
	*(wake_reason) = ISSIG(p); \
	(void) splx(s); \
	if (*(wake_reason)) \
		*(wake_reason) = -1; \
	else \
		*(wake_reason) = (ipcp->State == NOT_WAITING); \
	if (ipcp->TimerSet) { \
		untimeout(asleepexpire,ipcp); \
		ipcp->TimerSet = FALSE; \
	} \
}

#endif	MACH_VM
