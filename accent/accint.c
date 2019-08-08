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

/****************************************************************************
*  File: 	accint.c
*
*  Author:	Richard F. Rashid
*
*  Copyright (C) 1981  Richard F. Rashid and Carnegie-Mellon University
*  Copyright (C) 1982  Richard F. Rashid and Carnegie-Mellon University
*  Copyright (C) 1983  Richard F. Rashid and Carnegie-Mellon University
*  Copyright (C) 1984  Richard F. Rashid and Carnegie-Mellon University
*  Copyright (C) 1985  Richard F. Rashid and Carnegie-Mellon University
*  Copyright (C) 1986  Richard F. Rashid and Carnegie-Mellon University
****************************************************************************
* ABSTRACT:
*	Accent system primitives invoked by sending messages to a process
*	kernel port.  All non-message primitive operations should be 
*	implemented here. Note that MACH primitives are implemented elsewhere
*	but mirror many Accent calls.
****************************************************************************
* HISTORY
 * 13-May-86  Rick Rashid (rfr) at Carnegie-Mellon University
 *	Removed references to kmsgptr->DataPtr.
 *
 * 13-May-86  Rick Rashid (rfr) at Carnegie-Mellon University
 *	Fixed arguments to vm_allocate.
 *
 * 29-Apr-86  Rick Rashid (rfr) at Carnegie-Mellon University
 *	Fixed unxXFork again, this time to move data into ipc_soft_map
 *	as it should.  I also added new last parameter to vm_allocate.
 *
* 29-Arp-86  Richard F. Rashid
*		Changed shmem call in unxXFork to kmem_alloc.
* 12-Mar-86  Richard F. Rashid
*		Fixed to match up with new Mach VM code.
* 11-Dec-85  Robert V Baron (rvb) at Carnegie-Mellon University
*		Rewrite of Fork() [actually unxXFork, these days] so that the
*		accent proc record for the child is created before the unix fork.
*		The reply message is put on the parent and child process before
*		the actual fork.  This guarantees that the parent can't get a
*		message on the child's queue before the reply is posted for the
*		child.
* 11-Jul-85 Alan Cox
*		Modified for Mach OS.
* 11-May-81 Richard F. Rashid
*		Started. (For Accent)
*****************************************************************************/

#include "mach_mach.h"

#include "../h/queue.h"
#include "../accent/accent.h"
#include "../accent/accentint.h"
#include "../vm/vm_map.h"
#include "../vm/vm_kern.h"
#include "../h/task.h"
#include "../h/proc.h"
#include "../machine/cpu.h"

AccentMsg(kmsgptr)
register kmsgp_ kmsgptr;
/****************************************************************************
* Abstract:
*       General dispatcher for all message supplied Accent Kernel
*       facilities.
*
* Parameters:
*       kmsgptr - An incoming kernel message, already translated from
*                 process local to kernel global form (i.e., all ports
*                 are translated to their global forms).
*
* Environment:
*       Called from kernel when the remote port is a kernel port.
*       Note that kmsgptr points to an allocated message which
*       must eventually be deallocated but is not deallocated here.
*
* Results:
*       Returns no result, but does queue a message to be sent as a
*       reply to the sender (the port to which the reply is sent is
*       the local port field of the kmsgptr).
*
* Side Effects:
*       Depends upon routine called.
*
* Errors:
*       Returned through AccentError.
****************************************************************************/
{
	register kmsgp_ replykmsgptr;


	/* Because some 4.3 UNIX functions will be invoked, make sure that we are not
	 * running on a slave CPU (at least for now).
         */
	if (cpu_number() != master_cpu) {
		unix_swtch(u.u_procp,TRUE);
	}
	
	/* Specialized hook for handling UNIX fork operation: */
	if (kmsgptr->MsgHeader.ID == 105)
	{
		unxXFork(kmsgptr);
		return;
	}

	/* Allocate a reply message for kernel request */
	MKMsgAllocate(replykmsgptr);

	/* Call Matchmaker generated message interpretation and dispatching code: */
#if	MACH_MACH
	if (kmsgptr->MsgHeader.ID >= 1000) {
		(void) mach_server(&kmsgptr->MsgHeader, &replykmsgptr->MsgHeader);
		replykmsgptr->MsgHeader.LocalPort = kmsgptr->MsgHeader.RemotePort;
		replykmsgptr->MsgHeader.RemotePort = kmsgptr->MsgHeader.LocalPort;
	}
	else
#endif	MACH_MACH
	(void) AccIntServer(&kmsgptr->MsgHeader, &replykmsgptr->MsgHeader);

	/* Free incoming kernel message: */
	MKMsgFree(kmsgptr);

	/* Queue up reply message: */
	replykmsgptr->DataSize = replykmsgptr->MsgHeader.MsgSize - sizeof(msg_);
	(void) QueueMsg(replykmsgptr);
}

return_ AllocatePort(ServPort, /**/ LocalPort, Backlog)
register port_ *LocalPort;
port_ ServPort;
long Backlog;
/****************************************************************************
* Abstract:
*       Allocate a new port to the process for which ServPort is its
*       KERNEL_PORT.
*
* Parameters:
*       ServPort        - KERNEL_PORT of process for which we want a new port.
*       LocalPort       - Returns local port number allocated.
*       Backlog         - Initial port backlog.
*
* Returns:
*       NoMorePorts - if there are no ports available
*       SUCCESS     - if everything is  ok
****************************************************************************/
{
	if (Backlog > MaxBacklog)
		Backlog = MaxBacklog;
	if (Backlog <= 0)
		Backlog = ActualDefaultBacklog;
	*LocalPort = NewPort(PortArray[ServPort].Owner, Backlog);
	if (*LocalPort == NullPort)
		return NoMorePorts;
	else
		return SUCCESS;
}

return_ DeallocatePort(ServPort, LocalPort, Reason)
port_ ServPort;
port_ LocalPort;
long Reason;
/****************************************************************************
* Abstract:
*       Deallocate a port from the process whose KERNEL_PORT is ServPort.
*
* Parameters:
*       ServPort        - KERNEL_PORT of process whose port we want to
*                         deallocate.
*       LocalPort       - port we want to deallocate.
*       Reason          - Why port is being deallocated.
*
* Results:
*       SUCCESS is the only result.
*
* Design:
*       As it turns out, the port will have been deallocated already
*       by the time this routine is called since the deallocate bit
*       in the message which is sent to the kernel is  set.
****************************************************************************/
{
	return SUCCESS;
}

return_ SetBackLog(ServPort, LocalPort, Backlog)
port_ ServPort;
port_ LocalPort;
long Backlog;
/****************************************************************************
* Abstract:
*       Change the backlog of a port.
*
* Parameters:
*       ServPort        - KERNEL_PORT of a process which must be the
*                         receiver for port LocalPort.
*       LocalPort       - port whose backlog is to change.
*       Backlog         - New backlog for LocalPort.
*
* Results:
*       NotPortReceiver - if the owner of ServPort is not the receiver
*                         of LocalPort
*       SUCCESS         - if all goes  well
****************************************************************************/
{
	if (PortArray[LocalPort].Receiver == PortArray[ServPort].Owner)
	{
		PortArray[LocalPort].Backlog = Backlog;
		return SUCCESS;
	}
	else
		return NotPortReceiver;
}

Fork() {
} 	

/*
* This is defined because the matchmaker interface (accentint.c) expects Fork.
* We don't define Fork that way.  We handle it below in a very unMatchmaker way.
*/

unxXFork(kmsgptr)
register kmsgp_ kmsgptr;
{
	kmsgp_ QueueForkReply();

	struct MyMessage
	    {
		msg_		Head;
		type_ Type2;
		port_	Arg2;
		type_ Type3;
		port_	Arg3;
		type_ Type4;
		short Tname4;
		short Tsize4;
		long  Numelts4;
		ptrPortArray	Arg4;
	};

	procp_ pipcp = CurrentProcess();
	register struct MyMessage *MyMsgp = (struct MyMessage *)(&kmsgptr->MsgHeader);
	port_ ServPort = (MyMsgp->Head.RemotePort);
	short retcode;
	port_ HisKernelPort;
	port_ HisDataPort;
	ptrPortArray Ports, CPorts;
	long Ports_cnt;
	procp_ cipcp;
	kmsgp_ pkmsg, ckmsg;
	extern procp_ unxIPCActivate();

	if (pipcp->KPorts[KernelPort] != ServPort) {
		return NotCurrentProcess;
	}

#if TypeCheck
	if (MyMsgp->Type2.TypeName != TypePt)
	{
		Repmsgp->RetCode = WrongArgs;
		return;
	}
	if (MyMsgp->Type3.TypeName != TypePt)
	{
		Repmsgp->RetCode = WrongArgs;
		return;
	}
	if (MyMsgp->Tname4 != TypePt)
	{
		Repmsgp->RetCode = WrongArgs;
		return;
	}
#endif
	Ports_cnt = MyMsgp->Numelts4;
	Ports = (MyMsgp->Arg4);

	cipcp = unxIPCActivate();
	HisKernelPort = cipcp->KPorts[KernelPort];
	HisDataPort = cipcp->KPorts[DataPort];

	pkmsg = QueueForkReply(kmsgptr->MsgHeader.RemotePort,
	kmsgptr->MsgHeader.LocalPort,
	kmsgptr->MsgHeader.ID + 100,
	IsParent,
	HisKernelPort,
	HisDataPort,
	Ports,
	Ports_cnt);
	if (Ports_cnt) {
 		extern vm_map_t ipc_soft_map;

		if (!(CPorts =(port_ *)MoveMemory(ipc_soft_map,(ptr_)Ports,ipc_soft_map,Ports_cnt*sizeof(port_),FALSE)))
			return Failure;
	} 
	else {
		CPorts = Ports;
	}
	ckmsg = QueueForkReply(HisKernelPort,
	HisDataPort,
	kmsgptr->MsgHeader.ID + 100,
	IsChild,
	HisKernelPort,
	HisDataPort,
	CPorts,
	Ports_cnt);

	MKMsgFree(kmsgptr);
	Afork(cipcp);
	if (u.u_error) {
		struct yech {
			msg_	Head;
			type_ RetCodeType;
			short	RetCode;
		};
		((struct yech *)&pkmsg->MsgHeader)->RetCode = CantFork;
		unxIPCSuicide(cipcp);
	}
}

kmsgp_
QueueForkReply(Local, Remote, ID, retcode, HisKernelPort, HisDataPort, Ports, Ports_cnt)
port_ HisKernelPort;
port_ HisDataPort;
ptrPortArray Ports;
long Ports_cnt;
{
	struct RepMessage
	    {
		msg_	Head;
		type_ RetCodeType;
		short	RetCode;
		type_ Type2;
		port_	Arg2;
		type_ Type3;
		port_	Arg3;
		type_ Type4;
		short Tname4;
		short Tsize4;
		long  Numelts4;
		ptrPortArray Arg4;
	};

	register kmsgp_ replykmsgptr;
	register struct RepMessage *RepMsgp;

	MKMsgAllocate(replykmsgptr);
	RepMsgp = (struct RepMessage *)(&replykmsgptr->MsgHeader);

	RepMsgp->Head.LocalPort = Local;
	RepMsgp->Head.RemotePort = Remote;
	RepMsgp->Head.ID = ID;
	RepMsgp->Head.MsgType = NormalMsg;
	RepMsgp->RetCodeType.TypeName = TypeInt16;
	RepMsgp->RetCodeType.TypeSizeInBits = 16;
	RepMsgp->RetCodeType.NumObjects = 1;
	RepMsgp->RetCodeType.InLine = TRUE;
	RepMsgp->RetCodeType.LongForm = FALSE;
	RepMsgp->RetCodeType.Deallocate = FALSE;
	RepMsgp->RetCode = retcode;

	RepMsgp->Head.SimpleMsg = FALSE;
	RepMsgp->Head.MsgSize = sizeof (struct RepMessage);

#if Fastassign
	RepMsgp->Type2.Lng = 02000220006;
#else
	RepMsgp->Type2.InLine = TRUE;
	RepMsgp->Type2.Deallocate = FALSE;
	RepMsgp->Type2.LongForm = FALSE;
	RepMsgp->Type2.TypeName = TypePt;
	RepMsgp->Type2.TypeSizeInBits = 32;
	RepMsgp->Type2.NumObjects = 1;
#endif
	/* Because of nothing leftnot */
	RepMsgp->Arg2 = (HisKernelPort);
#if Fastassign
	RepMsgp->Type3.Lng = 02000220006;
#else
	RepMsgp->Type3.InLine = TRUE;
	RepMsgp->Type3.Deallocate = FALSE;
	RepMsgp->Type3.LongForm = FALSE;
	RepMsgp->Type3.TypeName = TypePt;
	RepMsgp->Type3.TypeSizeInBits = 32;
	RepMsgp->Type3.NumObjects = 1;
#endif
	/* Because of nothing leftnot */
	RepMsgp->Arg3 = (HisDataPort);
#if Fastassign
	RepMsgp->Type4.Lng = 04000000000;
#else
	RepMsgp->Type4.InLine = FALSE;
	RepMsgp->Type4.Deallocate = Ports_cnt ? TRUE : FALSE;
	/* This just means charge the
	* user for the pointer */
	RepMsgp->Type4.LongForm = TRUE;
#endif
	/* Because of Long form*/
	RepMsgp->Tname4 = TypePt;
	RepMsgp->Tsize4 = 32;
	RepMsgp->Numelts4 = Ports_cnt;
	RepMsgp->Arg4 = (Ports);

	replykmsgptr->DataSize = replykmsgptr->MsgHeader.MsgSize - sizeof(msg_);

	(void) QueueMsg(replykmsgptr);
	return replykmsgptr;
}

return_ Terminate(ServPort, Reason)
port_ ServPort;
long Reason;
/****************************************************************************
* Abstract:
*       Destroy the process for which ServPort is the KernelPort.
*
* Parameters:
*       ServPort        - KernelPort of process we want to terminate.
*       Reason          - Why we want to terminate.
*
* Results:
*       SUCCESS
*
* Side Effects:
*       If the process is the current one, we never  return.
****************************************************************************/
{
	procp_ ipcp;

	if (PortArray[ServPort].Receiver == pKERNEL_PROCESS)
	{
		ipcp = PortArray[ServPort].Owner;
		psignal(ipcp->task->proc, SIGKILL);
		return SUCCESS;
	}
	else
		return Failure;
}

return_ Suspend(ServPort)
port_ ServPort;
/****************************************************************************
* Abstract:
*       Suspend the process whose KernelPort is ServPort.  If the current
*       process is suspending itself, any form of wakeup (such as an
*       I/O wakeup or a message) will terminate the suspend.  If the
*       process doing the suspending is other than the current process,
*       only a Resume (see below) will terminate the suspend.
*
* Parameters:
*       ServPort        - KernelPort of process to suspend.
*
* Results:
*        SUCCESS
****************************************************************************/
{
	procp_ ipcp;

	if (PortArray[ServPort].Receiver == pKERNEL_PROCESS)
	{
		ipcp = PortArray[ServPort].Owner;
		psignal(ipcp->task->proc, SIGSTOP);
		return SUCCESS;
	}
	else
		return Failure;
}

PSSuspend(ipcp)
procp_ ipcp;
{
	psignal(ipcp->task->proc, SIGSTOP);
}

return_ RESUME(ServPort)
port_ ServPort;
/****************************************************************************
* Abstract:
*       Resume the process whose KernelPort is ServPort.
*
* Parameters:
*       ServPort
*
* Results:
*        SUCCESS
****************************************************************************/
{
	procp_ ipcp;

	if (PortArray[ServPort].Receiver == pKERNEL_PROCESS)
	{
		ipcp = PortArray[ServPort].Owner;
		psignal(ipcp->task->proc, SIGCONT);
		return SUCCESS;
	}
	else
		return Failure;
}

return_ SoftInterrupt(ServPort, NormOrEmerg, EnOrDisable)
port_ ServPort;
long_bool NormOrEmerg;
long_bool *EnOrDisable;
{
	register procp_ ipcp;
	register kmsgp_ kmsgptr;
	long_bool PrevEnOrDis;

	if (PortArray[ServPort].Receiver == pKERNEL_PROCESS) {
		if ((ipcp = PortArray[ServPort].Owner) <= pLAST_USED_PROCESS)
			if (NormOrEmerg) {
				PrevEnOrDis = ipcp->pcb.MsgEnable;
				ipcp->pcb.MsgEnable = fubyte((caddr_t)EnOrDisable);
			} 
			else {
				PrevEnOrDis = ipcp->pcb.EMsgEnable;
				ipcp->pcb.EMsgEnable = fubyte((caddr_t)EnOrDisable);
			}

		if (copyout(&PrevEnOrDis, EnOrDisable, sizeof (long)))
			return NotAUserAddress;

		kmsgptr = (kmsgp_)queue_first(&ipcp->MsgsWaiting);
		while (!queue_end(&ipcp->MsgsWaiting,(queue_entry_t)kmsgptr)) {
			if (!PortArray[kmsgptr->MsgHeader.RemotePort].Locked) {
				register intr_ pcbp = &ipcp->pcb;

				if (pcbp->EMsgEnable &&
				    kmsgptr->MsgHeader.MsgType == EmergencyMsg) {
					SetSoft(ipcp, TRUE);
					pcbp->EMsgEnable = FALSE;
				} 
				else if (pcbp->MsgEnable) {
					SetSoft(ipcp, FALSE);
					pcbp->MsgEnable = FALSE;
				}
				return SUCCESS;
			}
			kmsgptr = (kmsgp_)queue_next((queue_entry_t)kmsgptr);
		}
		return SUCCESS;

	} 
	else
		return Failure;
}

return_ ValidateMemory (ServPort, Address, Numbytes, Createmask)
port_		ServPort;
vm_offset_t	*Address;
long		Numbytes;
long		Createmask;
/****************************************************************************
* Abstract:
*	Make a range of address in the address space of the task with kernel
*	port == ServPort valid.
*
* Parameters:
*	ServPort	-	Task port of task address map to modify.
*	Address		-	Starting address to validate.  Find address if NULL
*				and return through reference parameter.
*	Numbytes	-	Number of bytes to validate.
*	CreateMask	-	Unused parameter
*
* Results:
*	Returns error code or SUCCESS.  If Address is NULL it returns
*	located address through reference parameter.
*
* Side Effects:
*	Makes address range valid (future faults on pages in that range will
*	result in fill zero pages).  May cause task map to be locked.
****************************************************************************/
{
	procp_			ipcp;
	boolean_t		find_space;
	register kern_return_t	result;

	if (PortArray[ServPort].Receiver == pKERNEL_PROCESS) {
		ipcp = PortArray[ServPort].Owner;

		if (*Address == (vm_offset_t)NULL) find_space = TRUE;

		result = vm_allocate(task_table[((struct proc *)(ipcp->task->proc))-proc]->map, 
					Address, (vm_size_t)Numbytes, find_space);

		if (result == KERN_SUCCESS) {
			return SUCCESS;
		} else return Failure;

	} else return Failure;
}

return_ InvalidateMemory (ServPort, Address, Numbytes)
port_		ServPort;
vm_offset_t	Address;
long		Numbytes;
/****************************************************************************
* Abstract:
*	Invalidate a range of the address map of the task whose task port
*	is ServPort.
****************************************************************************/
{
	procp_			ipcp;
	register kern_return_t	result;

	if (PortArray[ServPort].Receiver == pKERNEL_PROCESS) {
		ipcp = PortArray[ServPort].Owner;
		result = vm_deallocate(task_table[((struct proc *)(ipcp->task->proc))-proc]->map, 
					Address, (Address + (vm_size_t)Numbytes));
		if (result == KERN_SUCCESS) {
			return SUCCESS;
		} else return Failure;

	} else return Failure;	
}

return_ SetProtection (ServPort, Address, Numbytes, Protection)
{
	uprintf("SetpPotection is not a working function.\n");
}

return_ ReadProcessMemory (ServPort, Address, Numbytes, Data, Data_cnt)
{
	uprintf("ReadProcessMemory is not a working function.\n");
}

return_ WriteProcessMemory (ServPort, Address, Numbytes, Data, Data_cnt)
{
	uprintf("WriteProcessMemory is not a working function.\n");
}

/* ***************************************************************************
*
* 	THE GREAT OUT DOORS
*
* 	Note: These calls have no real correspondence in Mach and are
*	not reproduced.  Access to them will cause an error message to
*	be printed on the user tty.
*
* **************************************************************************/

return_ IndexInterpose (ServPort, MyPort, Hisindex, HisPort)
{
	uprintf("IndexInterpose is not a working function.\n");
	return Failure;
}

return_ PortInterpose (ServPort, MyPort, HisPort, MynewPort)
{
	uprintf("PortInterpose is not a working function.\n");
	return Failure;
}

return_ Status (ServPort, Nstats)
{
	uprintf("Status is not a working function.\n");
}

return_ SetPriority (ServPort, Priority)
{
	uprintf("SetPriority is not a working function.\n");
}

return_ SetLimit (ServPort, ReplyPort, Limit)
{
	uprintf("SetLimit is not a working function.\n");
}

return_ Examine (ServPort, Regorstack, Index, Value)
{
	uprintf("Examine is not a working function.\n");
}

return_ Deposit (ServPort, Regorstack, Index, Value)
{
	uprintf("Deposit is not a working function.\n");
}

return_ CreateSegment (ServPort, ImagsegPort, Segmentkind, Initialsize, Maxsize, Stable, Segment)
{
	uprintf("CreateSegment is not a working function.\n");
}

return_ TruncateSegment (ServPort, Segment, Newsize)
{
	uprintf("TruncateSegment is not a working function.\n");
}

return_ DestroySegment (ServPort, Segment)
{
	uprintf("DestroySegment is not a working function.\n");
}

return_ ReadSegment (ServPort, Segment, Offset, Numpages, Data, Data_cnt)
{
	uprintf("ReadSegment is not a working function.\n");
}

return_ WriteSegment (ServPort, Segment, Offset, Data, Data_cnt)
{
	uprintf("WriteSegment is not a working function.\n");
}

return_ GetDiskPartitions (ServPort, Devnum, Devname, Partl, Partl_cnt)
{
	uprintf("GetDiskPartitions is not a working function.\n");
}

return_ PartMount (ServPort, Partname, Exuse, Rootid, Partkind, PartPort, Parts, Parte)
{
	uprintf("PartMount is not a working function.\n");
}

return_ PartDismount (ServPort)
{
	uprintf("PartDismount is not a working function.\n");
}

return_ SetTempSegPartition (ServPort, Partname)
{
	uprintf("SetTempSegPartition is not a working function.\n");
}

return_ SetDebugPort (ServPort, DebugPort)
{
	uprintf("SetDebugPort is not a working function.\n");
}

return_ Touch (ServPort, Address)
{
	uprintf("Touch is not a working function.\n");
}

return_ GetPortIndexStatus (ServPort, Portindex, Backlog, Nwaitingmsgs, Ewaitingmsgs, Portright, Porttype)
{
	uprintf("GetPortIndexStatus is not a working function.\n");
}

return_ GetPortStatus (ServPort, Portright, Backlog, Nwaitingmsgs, Ewaitingmsgs, Portindex, Porttype)
{
	uprintf("GetPortStatus is not a working function.\n");
}

return_ ExtractAllRights (ServPort, Portindex, Portright, Porttype)
{
	uprintf("ExtractAllRights is not a working function.\n");
}

return_ InsertAllRights (ServPort, Portindex, Portright, Porttype)
{
	uprintf("InsertAllRights is not a working function.\n");
}

return_ CreateProcess (ServPort, HisKernelPort, HisDataPort)
{
	uprintf("CreateProcess is not a working function.\n");
}

return_ InterceptSegmentCalls (ServPort, OldsysPorts, OldsysPorts_cnt, SysPorts, SysPorts_cnt)
{
	uprintf("InterceptSegmentCalls is not a working function.\n");
}

return_ DirectIO (ServPort, Cmdblk, Cmdblk_cnt, Datahdr, Datahdr_cnt, Data, Data_cnt)
{
	uprintf("DirectIO is not a working function.\n");
}

return_ SetPagingSegment (ServPort, Segment)
{
	uprintf("SetPagingSegment is not a working function.\n");
}

return_ CreateRectangle (ServPort, RectPort, Baseaddr, Scanwidth, Basex, Basey, Maxx, Maxy, Isfont)
{
	uprintf("CreateRectangle is not a working function.\n");
}

return_ DestroyRectangle (ServPort, RectPort)
{
	uprintf("DestroyRectangle is not a working function.\n");
}

return_ AvailableVM (ServPort, Numbytes)
{
	uprintf("AvailableVM is not a working function.\n");
}

return_ EnableRectangles (ServPort, Rectlist, Rectlist_cnt, Enable)
{
	uprintf("EnableRectangles is not a working function.\n");
}

return_ SetKernelWindow (ServPort, Leftx, Topy, Width, Height, Inverted)
{
	uprintf("SetKernelWindow is not a working function.\n");
}
