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
 * Author:	Richard F. Rashid
 *
 * HISTORY
 * 10-Jun-86  Rick Rashid (rfr) at Carnegie-Mellon University
 *	Fixed IPCWakeup to simply release process lock.  Modified
 *	IPCWait to switch timeout and untimeout calls to master for now.
 *	The correct fix is to make timeout and untimeout callable on
 *	the slaves.
 *
 *  4-Jun-86  Rick Rashid (rfr) at Carnegie-Mellon University
 *	Macros for IPCWakeup and IPCWait added (some time ago) and
 *	splipc added to allow messages to be sent from interrupt level.
 *
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
#include "../h/sched.h"
#include "../machine/cpu.h"
#endif	MACH_VM

/*
 * macro:
 *	pidx(ipcp)
 *
 * Abstract:
 *	Return the accent process table index of the process.
 */

#define pidx(ipcp) (ipcp - ProcArray)

#define splipc() splnet()
#define splipc_done(s) splx(s)

#define port_lock_init(portptr) simple_lock_init(&portptr->PortLock)

#define port_lock_read(portptr,ipl) \
{ \
	ipl = splipc(); \
	simple_lock(&portptr->PortLock); \
}

#define port_lock_write(portptr,ipl) port_lock_read(portptr,ipl)

#define port_lock_done(portptr,ipl) \
{ \
	simple_unlock(&portptr->PortLock); \
	(void) splipc_done(ipl); \
}

#define port_lock_read_to_write(portptr,ipl)
#define port_lock_write_to_read(portptr,ipl)

#define proc_lock_init(procptr) simple_lock_init(&procptr->ProcLock)

#define proc_lock_read(procptr,ipl) \
{ \
	ipl = splipc(); \
	simple_lock(&procptr->ProcLock); \
}

#define proc_lock_write(procptr,ipl) proc_lock_read(procptr,ipl)

#define proc_lock_done(procptr,ipl) \
{ \
	simple_unlock(&procptr->ProcLock); \
	(void) splipc_done(ipl); \
}

#define proc_lock_read_to_write(procptr,ipl)
#define proc_lock_write_to_read(procptr,ipl)

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
	int ipl; \
	proc_lock_read(ipcp,ipl); \
	lp = ipcp->KPorts[KernelPort];\
	proc_lock_done(ipcp,ipl); \
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
	int ipl; \
	proc_lock_read(ipcp,ipl); \
	lp = ipcp->KPorts[DataPort];\
	proc_lock_done(ipcp,ipl); \
}

#define GiveOwnership(gp, ipcp)\
 { \
	int ipl; \
	port_lock_write((&PortArray[gp]),ipl); \
	PortArray[gp].Owner = ipcp; \
	port_lock_done((&PortArray[gp]),ipl); \
 }


/*
 * Macro:
 *       FastGTran
 *
 * Abstract:
 *       Translates a proc, gp pair into a local port number (lp)
 */
#define FastGTran(ipcp, lp, gp)\
{\
	int proc_ipl, port_ipl; \
	portp_ tmp_prp; \
	port_lock_write(((portp_)(tmp_prp = &PortArray[gp])),port_ipl); \
	_bit_set(pidx(ipcp), PortArray[gp].Senders);\
	if (gp == ipcp->KPorts[DataPort]) {\
		lp = DataPort;\
	} else if (gp == ipcp->KPorts[KernelPort]) {\
		lp = KernelPort;\
	} else\
		lp = gp;\
	port_lock_done(tmp_prp,port_ipl); \
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
	int proc_ipl, port_ipl; \
	portp_ tmp_prp; \
	if ((lp < FirstNonReservedPort)) {\
		if (lp < KernelPort) {\
			gp = NullPort;\
		} else {\
			proc_lock_read(ipcp,proc_ipl); \
			gp = ipcp->KPorts[lp];\
			proc_lock_done(ipcp,proc_ipl); \
		}\
	} else {\
		if ( (lp < NumPorts)) {\
			port_lock_write(((portp_)(tmp_prp = &PortArray[lp])),port_ipl); \
			if (_bit_tst(pidx(ipcp), PortArray[lp].Senders)) gp =  lp;\
			else if (lp <= LastInitPort && PortArray[lp].InUse) {\
				_bit_set(pidx(ipcp), PortArray[lp].Senders);\
				gp =  lp;\
			} else gp = NullPort;\
			port_lock_done(tmp_prp,port_ipl); \
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
{ \
	int s; \
	s = splipc(); \
	simple_lock(KMsgLock); \
	if ((kmsg = (kmsgp_)dequeue_head(&FreeKMsgQ)) == NULL) { \
		simple_unlock(KMsgLock); \
		(void) splipc_done(s); \
		kmsg = (kmsgp_) kmem_alloc(ipc_map, TotalKMsgSize, TRUE);\
	} else { \
		simple_unlock(KMsgLock); \
		(void) splipc_done(s); \
	} \
}

#define MKMsgFree(kmsgptr)\
{ \
	int s; \
	s = splipc(); \
	simple_lock(KMsgLock); \
	enqueue_head(&FreeKMsgQ, kmsgptr);\
	simple_unlock(KMsgLock); \
	(void)splipc_done(s); \
}
#else	MACH_VM
#define MKMsgAllocate(kmsg)\
{ \
	int s; \
	s = splipc(); \
	simple_lock(KMsgLock); \
	if ((kmsg = (kmsgp_)dequeue_head(&FreeKMsgQ)) == NULL) \
		kmsg = (kmsgp_) shmustalloc(TotalKMsgSize);\
	simple_unlock(KMsgLock); \
	(void)splipc_done(s); \
}

#define MKMsgFree(kmsgptr)\
{register ptr_ vaddr; \
	int s; \
	vaddr = kmsgptr->DataPtr;\
	if ((int) vaddr && vaddr != (ptr_) kmsgptr->Data) {\
		shmfree(vaddr, kmsgptr->DataSize);\
	}\
	s = splipc(); \
	simple_lock(KMsgLock); \
	enqueue_head(&FreeKMsgQ, kmsgptr);\
	simple_unlock(KMsgLock); \
	(void)splipc_done(s); \
}
#endif	MACH_VM

#define SetSoft(ipcp, NormOrEmerg)\
{\
	/* true is Emerg */\
	psignal (ipcp->task->proc, NormOrEmerg ? SIGEMSG : SIGMSG);\
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
	p = (struct proc *)ipcp->task->proc; \
	if (((ipcp->State == MSG_WAIT) || (ipcp->State == FULL_WAIT))&& (p->p_stat == SSLEEP)) { \
		ipcp->State = NOT_WAITING; \
		/* Not in core, use setrun */ \
		if ((p->p_flag & SLOAD)==0) { \
			s = splhigh(); \
			if (p->p_stat == SSLEEP) \
				setrun(p); \
			proc_lock_done(ipcp,s); \
		} else { \
			p->p_stat = SRUN; \
			u.u_ru.ru_nvcsw++; \
			ipc_wakeup(thread_table[p - proc],&ipcp->ProcLock); \
			/* ipc_wakeup releases lock */\
		} \
	}  else { \
		s = splhigh(); \
		if ((ipcp->State == MSG_WAIT) || (ipcp->State == FULL_WAIT)) { \
			ipcp->State = NOT_WAITING; \
		} \
		proc_lock_done(ipcp,s); \
	} \
}


#define IPCWait(ipcp, maxwait, waitstate) \
{ \
	register struct proc *p; \
	register int s,tmp_ipl; \
	register run_queue_t lq; \
	s = splhigh(); \
	ipcp->State = waitstate; \
	if (maxwait != 0) { \
		/* timeout can only be called on the master right now */\
		if (cpu_number() != master_cpu) { \
			proc_lock_done(ipcp,s); \
			(void) unix_swtch(u.u_procp,TRUE); \
			proc_lock_write(ipcp,tmp_ipl); \
		} \
		if (ipcp->State == waitstate) { \
			(void) splhigh(); \
			maxwait = (maxwait + 9) / 10; \
			ipcp->TimerSet = TRUE; \
			timeout(asleepexpire, ipcp, maxwait); \
		} \
	}  \
	else { \
		ipcp->TimerSet = FALSE; \
	} \
	/* Idle loop if there is nothing to do.*/ \
	lq = &local_runq[cpu_number()]; \
	if ((lq->count <= 1) && (global_runq.count == 0)) { \
		proc_lock_done(ipcp,s); \
		(void) spl0(); \
		while ((lq->count <= 1) && (global_runq.count == 0)) { \
			if (ipcp->State != waitstate) break; \
		} \
		proc_lock_write(ipcp,tmp_ipl); \
	} \
	if (ipcp->State == waitstate) { \
		p = (struct proc *)ipcp->task->proc; \
		p->p_stat = SSLEEP; \
		p->p_wchan = 0; \
		p->p_pri = PIPC; \
		(void) spl0(); \
		u.u_ru.ru_nvcsw++; \
		ipc_wait(&ipcp->ProcLock); \
		proc_lock_write(ipcp,tmp_ipl); \
	} \
	if (ipcp->TimerSet) { \
		/* untimeout can only be called on the master right now */\
		if (cpu_number() != master_cpu) { \
			proc_lock_done(ipcp,s); \
			(void) unix_swtch(u.u_procp,TRUE); \
			(void) splhigh(); \
			untimeout(asleepexpire,ipcp); \
			ipcp->TimerSet = FALSE; \
			proc_lock_write(ipcp,tmp_ipl); \
		} else { \
			untimeout(asleepexpire,ipcp); \
			ipcp->TimerSet = FALSE; \
		} \
	} \
	(void) splx(s); \
}

#else	MACH_VM
#define IPCWakeup(ipcp) \
{ \
	ipcp->State = NOT_WAITING; \
	wakeup(ipcp); \
}

#define IPCWait(ipcp, maxwait, waitstate) \
{ \
	register struct proc *p; \
	register int s; \
	s = splhigh();
	ipcp->State = waitstate; \
	if (maxwait != 0) { \
		maxwait = (maxwait + 9) / 10; \
		ipcp->TimerSet = TRUE; \
		timeout(asleepexpire, ipcp, maxwait); \
	}  \
	else { \
		ipcp->TimerSet = FALSE; \
	} \
	proc_lock_done(ipcp,s); \
	sleep(ipcp,PIPC); \
	(void) splx(s); \
	proc_lock_write(ipcp,s); \
	if (ipcp->TimerSet) { \
		untimeout(asleepexpire,ipcp); \
		ipcp->TimerSet = FALSE; \
	} \
}

#endif	MACH_VM
