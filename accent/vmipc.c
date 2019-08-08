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
*
*  File: 	VMIPC.PasMac
*
*  Author: 	Richard F. Rashid
*
*  Copyright (C) 1981  Richard F. Rashid and Carnegie-Mellon University
*  Copyright (C) 1982  Richard F. Rashid and Carnegie-Mellon University
*  Copyright (C) 1983  Richard F. Rashid and Carnegie-Mellon University
*  Copyright (C) 1984  Richard F. Rashid and Carnegie-Mellon University
*  Copyright (C) 1985  Richard F. Rashid and Carnegie-Mellon University
*  Copyright (C) 1986  Richard F. Rashid and Carnegie-Mellon University
*
****************************************************************************
* ABSTRACT:
*       Routines implementing all primitive (i.e., not message supplied)
*       IPC functions.  Also contains IPCInit and IPCSuicide for starting
*       up the IPC facility and for killing IPC data structures associated
*       with a process.
****************************************************************************
* HISTORY
 * 10-Jun-86  Rick Rashid (rfr) at Carnegie-Mellon University
 *	IPCWakeup now releases the process lock.
 *
 *  4-Jun-86  Rick Rashid (rfr) at Carnegie-Mellon University
 *	Added ipl parameter to locking macros so that software interrupt
 *	handlers would be locked out during any ipc lock.  This allows
 *	software interrupt handlers to allocate kmsgs and to call
 *	QueueMsg so that messages can be sent from such interrupts.
 *
 * 25-May-86  Rick Rashid (rfr) at Carnegie-Mellon University
 *	Added lock handling and initialization for multiprocessor
 *	execution of ipc code.  Most low level routines set and clear
 *	locks and assume high level routines leave locks unset upon
 *	entry.  Exceptions to this principle are noted.
 *
 * 13-May-86  Rick Rashid (rfr) at Carnegie-Mellon University
 *	Removed bogus "mp" queuing calls and increment/decrement calls.
 *
 * 13-May-86  Rick Rashid (rfr) at Carnegie-Mellon University
 *	Removed references to kmsgptr->DataPtr.  Continued cleanup.
 *	Removed DBG calls.
 *
 * 12-May-86  Rick Rashid (rfr) at Carnegie-Mellon University
 *	Changed IPCWakeup to be a routine rather than a macro.  Modified
 *	IPCWakeup and IPCWait to avoid using wakeup and sleep and use
 *	context_switch, unix_swtch and setrun instead.  Old asleep code
 *	remains in for imc support (but will be removed later).
 *
 *  6-May-86  Rick Rashid (rfr) at Carnegie-Mellon University
 *	Changed IPCWait to figure out if timeout call should be made and
 *	set the field TimerSet if that will be the case.
 *
 * 30-Apr-86  Rick Rashid (rfr) at Carnegie-Mellon University
 *	Removed include of vm_kern.h, it is now in vax/impl_macros.h.
 *
* 29-Apr-86  Richard F. Rashid
*		Changed various printf calls to uprintf. Added include of
*		vm_kern.h to get kernel memory allocation routines.
*
* 19-Mar-86  Richard F. Rashid
*		Changed handling of secondary/primary queues on full ports so that
*		the sender would only be woken if a secondary queue message was
*		later received rather than when it was posted on primary queue.
*
* 20-Feb-86  Richard F. Rashid
*		Reformated, removed macros (put in acc_macro.h).
*
* 11-Dec-85  Robert V Baron (rvb) at Carnegie-Mellon University
*		Now unxIPCActivate only creates a ipc proc record.  And
*		unxIPCenvironment binds the u.u_procp to ipc proc.  This way the
*		ipc record can exist before the new proces has been forked.
*
*  5-Dec-85  Robert V Baron (rvb) at Carnegie-Mellon University
*		Messages that were slightly larger than could fit in a kmsg were
*		being copied there by CopyInMsg.
*
* 11-Oct-85  Robert V Baron (rvb) at Carnegie-Mellon University
*		new shared memory scheme
*
*  24-Sep-85  Richard F. Rashid
*	       	Fixed handling of DEFAULT_PORTS option and locked ports 
*	       	so that locked ports can be received on when option is not
*	       	DEFAULT_PORTS.
*
*  21-Jun-84  Robert V. Baron
*             	vmipc.pasmac converted to C with the following major changes
*             	1.  KMsg is no longer a variant record MsgHeader is always used.
*             	2.  TypeType is no longer a variant record only the first 
*             	element is now used.
*             	3.  ptr_ is no longer used as a variant ptr_ == char * and the
*             	variant fields are not necessary in C, i.e. p_s are easily
*             	coerced and addition to p_s is legal.
*...
*...
*  30-Jul-81  Richard F. Rashid
*             	Started.
*****************************************************************************/

/***************************************************************************
*	Note on multiprocessor locking:
*
*		The IPC code depends on 3 types of locks: locks on
*		ports, locks on the process data structures (soon to be
*		tasks data structures) and locks on free queues for port,
*		process and kmsg records.  Subroutines are called with
*		locks off and must do their own locking.  Locks are held for
*		a minimum time.  Most importantly, if both a process and a
*		port lock need to acquired at the same time, THE PORT LOCK
*		MUST BE TAKEN FIRST.  This must be done to avoid deadlock.
*
*	Exceptions to locking rules:
*		delqueue is always called with the port lock ON.
*		SetSoft is always called with the proc lock ON.
*		IPCWait and IPCWakeup are called with the proc lock ON.
*		DQMsg is called with the port and proc locks ON.
****************************************************************************/

#include "../h/queue.h"
#include "../sync/lock.h"
#include "../accent/accent.h"
#include "../vm/vm_map.h"
#include "../h/task.h"
#include "../h/thread.h"

extern vm_map_t ipc_soft_map;

port_ NewPort();
port_ NewKernelPort();
void RemPort();
void DelPort();

port_ GiveSendRights();
void GiveReceiveRights();

return_ QueueMsg();
void DQMsg();
return_ CopyOutMsg();
return_ CopyInMsg();

void GTran();
return_ LTran();

void SendAcknowledge();
void SendKernelMsg();

void EnterPortTranslation();
port_ PortTranslation();
void RemovePortTranslation();

/****************************************************************************
 * Macro: SimpleKMsg
 *
 * Abstract:
 *	Macro for building the type and data fields for a simple message to
 *	be sent by the kernel.
 *****************************************************************************/
#define SimpleKMsg(sdp, typ, dat)\
{\
	sdp->type.TypeName = typ;\
	sdp->type.InLine = TRUE;\
	sdp->type.Deallocate = FALSE;\
	sdp->type.LongForm = FALSE;\
	sdp->type.NumObjects = 1;\
\
	switch (typ) {\
	case TypePt: \
	case TypePtReceive: \
	case TypePtOwnership: \
	case TypePtAll: \
	case TypeReal: \
	case TypeInt32: \
			sdp->type.TypeSizeInBits = 32;\
			break;\
	case TypeByte: \
	case TypeChar: \
			sdp->type.TypeSizeInBits = 8;\
			break;\
	case TypeShort: \
			sdp->type.TypeSizeInBits = 16;\
			break;\
	case TypeBit: \
			sdp->type.TypeSizeInBits = 1;\
			break;\
	default: \
			sdp->type.TypeSizeInBits = 32;\
	} \
\
	sdp->data = (long) dat;\
	sdp++;\
}

/****************************************************************************
 * Macro: PackMsg1
 *
 * Abstract:
 *	Macro for setting up a KMsg to be sent by the kernel to a user process.
 *****************************************************************************/
#define PackMsg1(kmsgp, mty, lp, rp, msgid, type1, data1)\
{\
 {			/* with ((*kmsg)) */\
		kmsgp->MsgHeader.SimpleMsg = FALSE;\
		kmsgp->MsgHeader.MsgType = mty;\
		kmsgp->MsgHeader.ID = msgid;\
		kmsgp->MsgHeader.LocalPort = lp;\
		kmsgp->MsgHeader.RemotePort = rp;\
		kmsgp->DataSize = sizeof (type_) + sizeof (long) ;\
		kmsgp->MsgHeader.MsgSize = sizeof (msg_) + kmsgp->DataSize;\
		{ register simple_descriptor_ *tp =\
					(simple_descriptor_ *) kmsgp->Data;\
			SimpleKMsg (tp, type1, data1);\
		}\
 } \
} \

procp_ unxIPCActivate()
/****************************************************************************
* Abstract:
*       Bring a process into the IPC world.
*
* Parameters:
*       ipcp - Process to activate
*
* Environment:
*       Process is not currently active (i.e. Active flag is FALSE)
*
* Results:
*       None.
*
* Side Effects:
*       Create a kernel port and data port for the process.
*****************************************************************************/
{
	port_ 	kp;
	procp_ 	ipcvp;
	int 	proc_ipl;

	simple_lock(FreeProcLock);
	if (ipcvp = (procp_) dequeue_head(&FreeProcQ)) {
		register procp_ ipcp = ipcvp;

		simple_unlock(FreeProcLock);

		proc_lock_write(ipcp,proc_ipl);
		queue_init(&(ipcp->MsgsWaiting));
		ipcp->Active = TRUE;
		ipcp->State = NOT_WAITING;
		ipcp->KPorts[KernelPort] = kp = NewPort (ipcp, ActualDefaultBacklog);
		ipcp->KPorts[DataPort] = NewPort (ipcp, ActualDefaultBacklog);

		/* 
		* Make the kernel the receiver for the KernelPort:
		*/

		PortArray[kp].Receiver = pKERNEL_PROCESS;
		bzero(&ipcp->pcb, sizeof (struct PCBHandle));

		proc_lock_done(ipcp,proc_ipl);
		return ipcp;
	} 
	else {
		simple_unlock(FreeProcLock);
		uprintf("Out of Proc Slots! See your system maintainer!\n") ;
		return (procp_) -1;
	}
}

unxIPCenv(procp, ipcp)
struct proc *procp;
register procp_ ipcp;
{
	aproc[procp-proc] = ipcp;
	ipcp->task = task_table[procp - proc];
}

void unxIPCSuicide (ipcp)
register procp_ ipcp;

/****************************************************************************
* Abstract:
* 	THis code is INCREDIBLY SLOW;  IT loops over NUM_PORTS 
*	and for ports InUse does some stuff.  Methinks it is time to
*	have a list of ports in use.
*
*     Called from process kill code in ACCINT whenever a process
*   dies.  This routine cleans up all IPC data structures owned
*   by the dying process and sends emergency messages to all processes
*   which references ports which are thereby deleted.
*
* Parameters:
*      ipcp - process that is dying
*****************************************************************************/
{
	register long_bool haveports;
	register long i;
	register long idx = pidx(ipcp);
	int proc_ipl, port_ipl;

	proc_lock_write(ipcp,proc_ipl);
	if (!ipcp->Active) {
		proc_lock_done(ipcp,proc_ipl);
		return;
	}
	ipcp->State = NOT_WAITING;
	ipcp->Active = FALSE;
	proc_lock_done(ipcp,proc_ipl);

	haveports = TRUE;
	while (haveports) {
		haveports = FALSE;

		for (i = FirstNonReservedPort; i < NumPorts; i++) {
			register portp_ prp = &(PortArray[i]) ;
			port_lock_read(prp,port_ipl);
			if (prp->InUse &&
			    _bit_tst(idx, prp->Senders)) {
				port_lock_done(prp,port_ipl);
				RemPort (ipcp, i);
				haveports = TRUE;
			} else {
				port_lock_done(prp,port_ipl);
			}
		}
	}

	simple_lock(FreeProcLock);
	enqueue_tail(&FreeProcQ, ipcp);
	simple_unlock(FreeProcLock);
}

port_ NewPort (ipcp, MaxBackLog)
procp_ ipcp;
long MaxBackLog;
/****************************************************************************
* Abstract:
*     Finds a local port number for process ipcp which is not is use.
*
* Results:
*       Returns an unused port identifier.
*****************************************************************************/
{
	port_ _NewPort;
	portp_ prvp;
	int port_ipl;

	simple_lock(FreePortLock);

	if (prvp = (portp_)dequeue_head(&FreePortQ)) {
		register portp_ prp = prvp;

		simple_unlock(FreePortLock);
		port_lock_write(prp,port_ipl);

		_NewPort = prp - PortArray;
		prp->InUse = TRUE;
		prp->Locked = FALSE;
		prp->Backlog = MaxBackLog;
		prp->Receiver = ipcp;
		prp->Owner = ipcp;
		queue_init(&(prp->SecondaryMsgQ));
		prp->NumQueued = 0;
		bzero(prp->Senders, sizeof (procs_));
		_bit_set(pidx(ipcp), prp->Senders);

		port_lock_done(prp,port_ipl);
		return (_NewPort);
	} 
	else {
		simple_unlock(FreePortLock);

		uprintf ("Out of ports! See your system maintainer!\n");
		return (NullPort);
	}
}

port_ NewKernelPort ()
/****************************************************************************
* Abstract:
*       Allocate a new port for use by the kernel.
*
* Parameters:
*       None.
*
* Results:
*       Returns a port owned by the  pager.
*****************************************************************************/
{
	return NewPort (pPAGER_PROCESS, ActualDefaultBacklog);
}

return_ QueueMsg (kmsgptr)
register kmsgp_ kmsgptr;
/****************************************************************************
* Abstract:
*     	Queue a message at a port.
*
* Parameters:
*    	 kmsgptr  - KMsg to queue.
*
* Environment:
*	May be called within the kernel.  It will always queue message
*	regardless of backlog and should only be used for critical kernel
*	messages.  This call never calls IPCWait.
*
* Results:
*	Returns normal IPC return code.
*
* Side Effects:
*     	May wake up receiving process if it is waiting.
*****************************************************************************/
{
	register procp_ RecProc;
	register procp_ ipcp;
	long_bool 	emerg;
	port_ 		lp;
	register port_ 	rp;
	int 		port_ipl, proc_ipl;

	rp = kmsgptr->MsgHeader.RemotePort;
	if (rp != NullPort) { 
		register portp_ prp = &(PortArray[rp]) ;

		port_lock_write(prp,port_ipl);

		if (!prp->InUse) {
			port_lock_done(prp,port_ipl);
			KMsgDeallocateAll(kmsgptr);
			return NotAPort;
		}

		RecProc = prp->Receiver;

		if (RecProc == pKERNEL_PROCESS) {
			/* Release locks */
			port_lock_done(prp,port_ipl);
			/* The kernel should not be sending itself a message! */
			printf("QueueMsg: Kernel sending itself a message!\n");
			return BadKernelMsg;
		}

		emerg = (kmsgptr->MsgHeader.MsgType == EmergencyMsg);
		kmsgptr->What = DONT_WAKE;

		if ((prp->NumQueued < prp->Backlog) || emerg) {
			register intr_ pcbp = &(RecProc->pcb);

			proc_lock_write(RecProc,proc_ipl);

			if (pcbp->EMsgEnable || pcbp->MsgEnable) {
				if (!prp->Locked) {
					if (((emerg) && pcbp->EMsgEnable)) {
						SetSoft (RecProc, TRUE);
						pcbp->EMsgEnable = FALSE;
					} 
					else if (pcbp->MsgEnable) {
						SetSoft (RecProc, FALSE);
						pcbp->MsgEnable = FALSE;
					} 
				}
			}

			/* 
			* If emergency message insert message at head of
			* queue rather than end and dont increment prp->NumQueued.
			*/

			if (emerg) {
				enqueue_head (&RecProc->MsgsWaiting, kmsgptr);
			} 
			else {
				enqueue (&RecProc->MsgsWaiting, kmsgptr);
				prp->NumQueued++;
			}

			port_lock_done(prp,port_ipl);
			proc_lock_done(RecProc,proc_ipl);

			proc_lock_write(RecProc,proc_ipl);
			if (RecProc->State == MSG_WAIT) {
				IPCWakeup(RecProc);
				/* IPCWakeup releases process lock */
				(void) splipc_done(proc_ipl);
			} else {
				proc_lock_done(RecProc,proc_ipl);
			}
		} 
		else {
			enqueue (&prp->SecondaryMsgQ, kmsgptr);
			prp->NumQueued++;
			port_lock_done(prp,port_ipl);
		}
	} 
	else {
		KMsgDeallocateAll (kmsgptr);
		return NotAPort;
	}
	return Success;
}


void RemPort (ipcp, lp)
register procp_ ipcp;
register port_ lp;
/****************************************************************************
* Abstract:
*     Remove a port from a process name space.
*
* Parameters:
*     ipcp  - Process number.
*     lp    - Local name for port in process ipcp.
*
* Environment:
*     lp is a port to which ipcp has access.
*
* Results:
*
* Side Effects:
*   It this is the
*   owner and receiver for the port, the port is destroyed
*   and all processes which have access to it are notified.  If
*   ipcp is either the owner or receiver but not both, the
*   receiver or owner respectively are sent a message containing
*   the released port  rights.
*****************************************************************************/
{
	register portp_ prp = &(PortArray[lp]) ;
	int proc_ipl, port_ipl;

	port_lock_write(prp,port_ipl);
	proc_lock_write(ipcp,proc_ipl);

	_bit_clear(pidx(ipcp), prp->Senders);

	if ((ipcp == prp->Receiver) || (ipcp == prp->Owner)) {
		if ((((ipcp == prp->Receiver) && (ipcp == prp->Owner)) ||
		    (prp->Receiver == pKERNEL_PROCESS) ||
		    (prp->Receiver == pNO_PROCESS) ||
		    (prp->Owner == pNO_PROCESS))) {
			/* 
			* Must be prp->owner and prp->receiver.
			* Get rid of lp completely.
			*/

			proc_lock_done(ipcp,proc_ipl);
			port_lock_done(prp,port_ipl);

			DelPort (lp);

			return;
		}

		/* 
		* Check for receive rights only.
		*/

		if (ipcp == prp->Receiver) {
			/* 
			* Test to see if lp ownership is in transit.
			*/


			if (prp->Owner == pRIGHTS_IN_TRANSIT) {

				/* 
				* We dont know who eventual prp->receiver will be.
				* when ownership right arrives at some destination
				* a RECEIVERIGHTS message will be automatically
				* sent. (See TranGP.)
				*/

				proc_lock_done(ipcp,proc_ipl);
				port_lock_done(prp,port_ipl);
				GiveReceiveRights (lp, pNO_PROCESS);
			} 
			else {

				/* 
				* In a very rare and still not too well understood
				* way it is possible for the prp->owner to not be active.
				* This really only occurs when the prp->owner is in
				* the process of shutting itself down.  In this case
				* we must destroy the lp.
				*/

				if (prp->Owner->Active) {
					proc_lock_done(ipcp,proc_ipl);
					port_lock_done(prp,port_ipl);
					SendKernelMsg (prp->Owner, M_ReceiveRights,
					TypePtReceive, lp);
				} 
				else {
					proc_lock_done(ipcp,proc_ipl);
					port_lock_done(prp,port_ipl);
					GiveReceiveRights (lp, pNO_PROCESS);
					DelPort (lp);
				}
			}
		} 
		else {
			/* 
			* Test to see if lp receive right is in transit.
			*/

			if (prp->Receiver == pRIGHTS_IN_TRANSIT) {

				/* 
				* We dont know who eventual prp->owner will be.
				* When receive right arrives at some destination
				* an OWNERSHIPRIGHTS message will be automatically
				* sent. (See TranGP.)
				*/

				proc_lock_done(ipcp,proc_ipl);
				port_lock_done(prp,port_ipl);
				GiveOwnership (lp, pNO_PROCESS);
			} 
			else {

				/* 
				* In a very rare and still not too well understood
				* way it is possible for the prp->receiver to not be active.
				* This really only occurs when the prp->receiver is in
				* the process of shutting itself down.  In this case
				* we must destroy the lp.
				*/

				if (prp->Receiver->Active) {
					proc_lock_done(ipcp,proc_ipl);
					port_lock_done(prp,port_ipl);
					SendKernelMsg (prp->Receiver, M_OwnershipRights,
					TypePtOwnership, lp);
				} 
				else {
					proc_lock_done(ipcp,proc_ipl);
					port_lock_done(prp,port_ipl);
					GiveOwnership (lp, pNO_PROCESS);
					DelPort (lp);
				}
			}
		}
	} else {
		proc_lock_done(ipcp,proc_ipl);
		port_lock_done(prp,port_ipl);
	}
}

void DelPort (lp)
register port_ lp;
/****************************************************************************
* Abstract:
*     Destroy a port.
*
* Parameters:
*     lp  - a port.
*
* Side Effects:
*     Removes port name from all processes, sends kernel message to
*   all processes which had access to the port, and frees the port
*   data structure.
*****************************************************************************/
{
	register long i;
	register portp_ prp = &(PortArray[lp]) ;
	kmsgp_ kmsgptr;
	procp_ ipcp;
	queue_head_t tmpqdata;
	queue_t tmpq;
	int port_ipl, proc_ipl;

	/* Create a queue for released messages */
	tmpq = &tmpqdata;
	queue_init(tmpq);

	/* 
	* For each process which refers to that port, the port
	* must be deallocated and an emergency message sent
	* which identifies the deallocated port.
	* Then, the port data structure itself must be deleted.
	*/

	port_lock_write(prp,port_ipl);

	if (!prp->InUse) {
		port_lock_done(prp,port_ipl);
		printf("Attempting to deallocate port not in use.\n");
		return;
	}

	ipcp = prp->Receiver;

	prp->InUse = FALSE;

 	proc_lock_write(ipcp,proc_ipl);

	/* DQMsg expects locks to be in place on both port and process */

	for (;DQMsg(&kmsgptr, lp), kmsgptr != (kmsgp_) NULL;) {
		enqueue_tail(tmpq,(queue_entry_t)kmsgptr);
	}

	/* Copy secondary message queue to end of tmpq */

	while (!queue_empty(&prp->SecondaryMsgQ)) {
		kmsgptr = (kmsgp_)dequeue_head(&prp->SecondaryMsgQ);
		enqueue_tail(tmpq,(queue_entry_t)kmsgptr);
	}

	proc_lock_done(ipcp,proc_ipl);
	port_lock_done(prp,port_ipl);

	while (!queue_empty(tmpq)) {

		kmsgptr = (kmsgp_) dequeue_head(tmpq);

		/* Wakeup or send acknowledgement to sender if needed */

		if ((kmsgptr->What == WAKE_ME)){
			proc_lock_write(kmsgptr->WakeupProc,proc_ipl);
			IPCWakeup (kmsgptr->WakeupProc);
			kmsgptr->What = DONT_WAKE;
			/* IPCWakeup releases process lock */
			(void) splipc_done(proc_ipl);
			}
		else if ((kmsgptr->What == SEND_ACK))
			SendAcknowledge (kmsgptr);

		kmsgptr->What = DONT_WAKE;

		/* Release message */
		KMsgDeallocateAll(kmsgptr);
	}

	for (i = KERNEL_PROCESS+1; i <= LAST_USED_PROCESS - 1; i++) {
		if (_bit_tst(i, prp->Senders)) {
			SendKernelMsg (&ProcArray[i], M_PortDeleted, TypeInt32, lp);
			_bit_clear(i, prp->Senders);
		}
	}

	/* Release port data structure */

	if (lp > LastInitPort) {
		simple_lock(FreePortLock);	
		enqueue_tail(&FreePortQ, prp);
		simple_unlock(FreePortLock);
	}

}


port_ GiveSendRights (gp, ipcp)
register port_ gp;
procp_ ipcp;
/****************************************************************************
* Abstract:
*     Gives process ipcp send rights to port portptr.
*
* Parameters:
*       lp      - port
*       ipcp    - Process number.
*
* Environment:
*     It is permissible for ipcp to already have access rights to portptr.
*
* Results:
*     Returns local port number for portptr in process ipcp.
*****************************************************************************/
{
	port_ lp;
	register portp_ prp = &(PortArray[gp]) ;
	int port_ipl;

	FastGTran (ipcp, lp, gp);

	port_lock_write(prp,port_ipl);

	if (!prp->InUse) {
		port_lock_done(prp,port_ipl);
		return NullPort;
	} 
	else {
		_bit_set(pidx(ipcp), prp->Senders);
		port_lock_done(prp,port_ipl);
		return lp;
	}
}


void GiveReceiveRights (gp, ipcp)
register port_ gp;
register procp_ ipcp;
/****************************************************************************
* Abstract:
*     Transfers receive rights to port represented by portptr to process
*   ipcp from whatever process currently has receive rights.
*
* Parameters:
*     gp        - a port
*     ipcp      - Process number.
*****************************************************************************/
{
	portp_		prp = &PortArray[gp];
	long_bool 	islocked;
	intr_ 		pcbp = &(ipcp->pcb);
	kmsgp_ 		kmsgptr;
	long_bool 	reset_emsg,reset_msg;
	procp_ 		old_ipcp;
	queue_head_t 	tmpq_head;
	queue_t		tmpq;
	int		port_ipl, proc_ipl;

	/* Do we need to do setsoft operations? */
	reset_emsg = FALSE;
	reset_msg  = FALSE;

	/* Setup temporary queue */
	tmpq = &tmpq_head;
	queue_init(tmpq);

	/* Setup locking for the entire time. */
	port_lock_write(prp,port_ipl);
	old_ipcp = prp->Receiver;
	proc_lock_write(old_ipcp,proc_ipl);

	islocked = prp->Locked;

	/* 
	* First move all message from the ports primary queue
	* associated with the current receiver to the primary queue
	* associated with the new receiver.
	*/

	/* NOTE: DQMsg expects locks to be in place */
	for (;DQMsg (&kmsgptr, gp), kmsgptr != (kmsgp_) NULL;) {
		enqueue_tail(tmpq, (queue_entry_t) kmsgptr);
	}

	/* 
	* Set the receiver field to be the new process.
	*/

	prp->Receiver = ipcp;

	proc_lock_done(old_ipcp,proc_ipl);
	
	proc_lock_write(ipcp,proc_ipl);
	while (!queue_empty(tmpq)) {
		kmsgptr = (kmsgp_) dequeue_head(tmpq);
		enqueue_tail(&ipcp->MsgsWaiting,(queue_entry_t)kmsgptr);
		if (!islocked) {
			if (((kmsgptr->MsgHeader.MsgType == EmergencyMsg) && pcbp->EMsgEnable)) {
				reset_emsg = TRUE;
			} 
			else if (pcbp->MsgEnable) {
				reset_msg = TRUE;
			}
		}
	}

	if (reset_emsg) {
		SetSoft (ipcp, TRUE);
		pcbp->EMsgEnable = FALSE;
	}

	if (reset_msg) {
		SetSoft (ipcp, FALSE);
		pcbp->MsgEnable = FALSE;
	}

	port_lock_done(prp,port_ipl);
	proc_lock_done(ipcp,proc_ipl);

	proc_lock_write(ipcp,proc_ipl);
	IPCWakeup(ipcp);
	/* IPCWakeup releases process lock */
	(void) splipc_done(proc_ipl);

}

void DQMsg ( /**/ kmsgptr, lp)
kmsgp_ *kmsgptr;
port_ lp;
/****************************************************************************
* Abstract:
*     Dequeues the next message waiting a a port.  It does not touch
*   or modify the secondary queue.
*
* Parameters:
*       kmsgptr - Var parameter into which message is placed.
*       lp      - port
*
* Results:
*     Returns a message in kmsgptr.
*****************************************************************************/
{
	register kmsgp_ next;
	register queue_t kmsgp;
	kmsgp_ tmp,tmp1;
	register procp_ ipcp = PortArray[lp].Receiver;


	*kmsgptr = (kmsgp_) NULL;

	kmsgp = &ipcp->MsgsWaiting;
	if (queue_empty(kmsgp)) {
		return;
	}

	next = (kmsgp_)queue_first(kmsgp);
	while (!queue_end(kmsgp,(queue_entry_t)next)) {
		if (lp == next->MsgHeader.RemotePort) {
			tmp = next;	 /* can't be in register */
			remqueue (kmsgp, (queue_entry_t)tmp);
			*kmsgptr = next;
			return;
		}
		next = (kmsgp_)queue_next((queue_entry_t)next);
	}

}

delqueue (Q, kmsgptr)
queue_t Q;
kmsgp_  kmsgptr;
/****************************************************************************
* Abstract:
*	Remove a message from a queue
*
*	Assumes necessary locks are already in place!
*
* Parameters:
*	Q 	- message queue
*	kmsgptr - message to remove
****************************************************************************/
{
	register kmsgp_ next;

	if (!(int)kmsgptr) {
		return FALSE;
	} 
	else 	if (queue_empty(Q)) {
		return FALSE;
	}

	next = (kmsgp_)queue_first(Q);
	while (!queue_end(Q,(queue_entry_t)next)) {
		if (next == kmsgptr) {
			remqueue (Q, kmsgptr);
			return TRUE;
		}
		next = (kmsgp_)queue_next((queue_entry_t)next);
	}
	return FALSE;
}

return_ CopyOutMsg (msgptr, kmsgptr, dealloconly)
register msgp_ msgptr;
register kmsgp_ kmsgptr;
long_bool dealloconly;
/****************************************************************************
* Abstract:
*       Copies a message from system space (pointed to by kmsgptr) to
*       user space (pointed to by msgptr).
*
* Parameters:
*       msgptr     - Destination of message.  If NULL, the system message
*                    will be deallocated without copying.
*       kmsgptr    - Source message.
*       dealloconly- We only want to deallocate the message.
*
* Results:
*       Returns no value.
*
* Side Effects:
*       Transfers data, may allocate space in the current process for
*       p_ed data which needs to be transferred.  It may also
*       transfer port capabilities.
*
* Errors:
*       Not enough room implies msgptr does not point to a record which
*       says it is long enough to accept the message.
*****************************************************************************/
{
	register procp_ ipcp = CurrentProcess();
	register ptr_ saddr;
	register ptr_ endaddr;
	long tn, ts;
	long    elts;
	long    numbytes;
	long    j;
	ptr_ vaddr;
	ptr_ ptaddr;
	port_ tmpport;

	if (msgptr == (msgp_) NULL) dealloconly = TRUE;

	if (dealloconly) {
		saddr = (ptr_) kmsgptr->Data;
	} 
	else {
		FastGTran (ipcp, kmsgptr->MsgHeader.LocalPort, kmsgptr->MsgHeader.LocalPort);
		FastGTran (ipcp, kmsgptr->MsgHeader.RemotePort, kmsgptr->MsgHeader.RemotePort);

		if (copyout((msgp_) &kmsgptr->MsgHeader, msgptr, sizeof (msg_))) {
			return NotAUserAddress;
		}

		if (kmsgptr->MsgHeader.MsgSize > msgptr->MsgSize) {
			return (NotEnoughRoom);
		}	
		tmpport = msgptr->LocalPort;
		msgptr->LocalPort = msgptr->RemotePort;
		msgptr->RemotePort = tmpport;

		saddr = (ptr_) msgptr + sizeof (msg_) ;

		if (copyout(kmsgptr->Data, saddr,kmsgptr->DataSize)) {
			return NotAUserAddress;
		}
	}

	if (kmsgptr->MsgHeader.SimpleMsg) return SUCCESS;

	endaddr = saddr + kmsgptr->DataSize;
	while (saddr < endaddr)
	{
		register type_ *tp = (type_ *) saddr;

		if (!*(int *)tp)
			return TypeZero;

		if (tp->LongForm)
		{
			register ltype_ *ltp = (ltype_ *) tp;

			elts = ltp->NumObjects;
			tn = ltp->TypeName;
			ts = ltp->TypeSizeInBits;
			saddr += sizeof (ltype_);
		} 
		else {
			tn = tp->TypeName;
			ts = tp->TypeSizeInBits;
			elts = tp->NumObjects;
			saddr += sizeof (type_);
		}
		numbytes = ((elts * ts) + 7) >> 3;

		if (tp->InLine) {
			ptaddr = saddr;
			saddr += ( (numbytes + 3) & (~0x3) );
		} 
		else {
			vaddr = (*(ptr_ *) saddr);

			if (dealloconly) {
				(void) MoveMemory(ipc_soft_map,vaddr,
						(vm_map_t)NULL,numbytes,TRUE);
			}
			else {
				*(ptr_ *) saddr = (ptr_) MoveMemory(ipc_soft_map,vaddr,
							current_task()->map,numbytes,TRUE);
			}

			ptaddr = (*(ptr_ *) saddr);
			saddr += sizeof (ptr_);
		}
		if (tn >= TypePtOwnership && tn <= TypePt && !dealloconly)
			for (j = 0; j < elts; ++j)
			{
				GTran (ipcp, (port_) ptaddr, tn);
				ptaddr += sizeof (port_);
			}
	}
	return SUCCESS;
}

return_ CopyInMsg (msgptr, var_kmsgptr)
register msgp_ msgptr;
kmsgp_ *var_kmsgptr;
/****************************************************************************
* Abstract:
*       Creates a new internal to the kernel message and copies msgptr into
*       the kernel.
*
* Parameters:
*       msgptr - Message (in user space) to copy into the kernel.
*
* Results:
*       Allocates and returns a message.
*
* Side Effects:
*       May allocate kernel memory.  Copies data into kernel.  May
*       transfer port capabilities.
*****************************************************************************/
{
	register procp_ ipcp = CurrentProcess();
	register ptr_ saddr;
	register ptr_ endaddr;
	register long    j;
	long tn, ts;
	long_bool issimple;
	long    srcbytes;
	long    elts;
	long    numbytes;
	ptr_ vaddr;
	ptr_ uaddr;
	ptr_ ptaddr;
	kmsgp_ kmsgptr;

	/* 
	* Get current process id:
	*/

	/* 
	* Allocate a new kernel message:
	*/

	MKMsgAllocate(kmsgptr);
	*var_kmsgptr = kmsgptr;

	if (copyin(msgptr, &(kmsgptr->MsgHeader), sizeof(msg_))) {
		return NotAUserAddress;
	}

	srcbytes = kmsgptr->MsgHeader.MsgSize  - sizeof (msg_);
	kmsgptr->DataSize = srcbytes;

	if (kmsgptr->MsgHeader.MsgSize > MaxKMsgSize) {
		return NotEnoughRoom;
	} 
	else {
		copyin(msgptr, &kmsgptr->MsgHeader, kmsgptr->MsgHeader.MsgSize);
	}

	FastLTran (ipcp, kmsgptr->MsgHeader.LocalPort, kmsgptr->MsgHeader.LocalPort);
	FastLTran (ipcp, kmsgptr->MsgHeader.RemotePort, kmsgptr->MsgHeader.RemotePort);

	if (kmsgptr->MsgHeader.SimpleMsg) return (SUCCESS);

	issimple = TRUE;
	saddr = (ptr_) kmsgptr->Data;
	endaddr = saddr + kmsgptr->DataSize;

	while (saddr < endaddr) {
		{ 
			int *abort = (int *)saddr;
			register  type_ * tp = (type_ *) saddr;
			int dealloc = tp->Deallocate;

			if (!*(int *)tp)
				return TypeZero;

			if (tp->LongForm) {
				register ltype_ *ltp = (ltype_ *) tp;
				elts = ltp->NumObjects;
				tn = ltp->TypeName;
				ts = ltp->TypeSizeInBits;
				saddr += sizeof (ltype_);
			} 
			else {
				tn = tp->TypeName;
				ts = tp->TypeSizeInBits;
				elts = tp->NumObjects;
				saddr += sizeof (type_);
			}

			numbytes = ((elts * ts) + 7) >> 3;

			if (saddr + (tp->InLine ? numbytes : 4) > endaddr) {
				*abort = 0;
				return TypeTooBig;
			}

			if (tp->InLine) {
				ptaddr = saddr;
				saddr += ( (numbytes+3) & (~0x3)) ;
				/* for a null pointer and for 
				* shared memory, we don't allocate
				* storage. */

				/* Translate ports */
				if ((tn >= TypePtOwnership) &&
			    	    (tn <= TypePt) && ptaddr) {
					issimple = FALSE;
					j = 0;

					while (j < elts) {
						register int ok =
					  	  LTran (ipcp, (port_) ptaddr,tn,dealloc);
						if (ok != SUCCESS) {
							*abort = 0;
							return ok;
						}
						ptaddr += sizeof (port_) ;
						j++ ;
					}
				}
			} 
			else {
				/* now the allocate case, but
				* check it out first.
				*/
				uaddr = *(ptr_ *)saddr;
				if (!useracc(uaddr, numbytes, 1)) {
					*abort = 0;
					return NotAUserAddress;
				}
				issimple = FALSE;

				/* Translate port rights: */

				ptaddr = uaddr;
				if ((tn >= TypePtOwnership) &&
			    	    (tn <= TypePt) && ptaddr) {
					issimple = FALSE;
					j = 0;

					while (j < elts) {
						register int ok =
					    	  LTran (ipcp, (port_) ptaddr,tn,dealloc);
						if (ok != SUCCESS) {
							*abort = 0;
							return ok;
						}
						ptaddr += sizeof (port_) ;
						j++ ;
					}
				}

				/* Copy Memory */
				vaddr = (ptr_) MoveMemory(current_task()->map,
							  uaddr,ipc_soft_map,numbytes,dealloc);

				*(ptr_ *)saddr = vaddr;
				saddr += sizeof (ptr_) ;
			}

		}
	}
	kmsgptr->MsgHeader.SimpleMsg = issimple;

	return (SUCCESS);
}

void GTran (ipcp, /**/ gp, pt)
register procp_ ipcp;
port_ *gp;
long pt;
/****************************************************************************
* Abstract:	
*      This routine translates a global port number to a local
*   port number for message reception.  If if transfer
*   of ownership or receive rights occurs, rights are passed
*   from the kernel to the receiving process.
*
* Parameters:
*       ipcp    - process number
*       gp      - var parameter, on input a global port number,
*                 on output the local port number
*       pt      - type of rights to be transferred.
*****************************************************************************/
{
	port_ lp;
	register int g = *gp ;
	register portp_ prp = &(PortArray[g]) ;
	int proc_ipl, port_ipl;

	if (g == NullPort) return;

	lp = GiveSendRights (g, ipcp);
	*gp = lp;

	port_lock_read(prp,port_ipl);
	proc_lock_read(ipcp,proc_ipl);

	switch (pt) {
	case TypePtOwnership: 
		if (prp->Receiver == pNO_PROCESS) {
			if (!ipcp->Active) {
				proc_lock_done(ipcp,proc_ipl);
				port_lock_done(prp,port_ipl);
				GiveReceiveRights (g, pRIGHTS_IN_TRANSIT);
				SendKernelMsg (ipcp, M_ReceiveRights, TypePtReceive, g);
			} 
			else {
				proc_lock_done(ipcp,proc_ipl);
				port_lock_done(prp,port_ipl);
				GiveReceiveRights (g, ipcp);
			}
		} else {
			proc_lock_done(ipcp,proc_ipl);
			port_lock_done(prp,port_ipl);
		}
		GiveOwnership (g, ipcp);
		break;

	case TypePtReceive: 
		if (prp->Owner == pNO_PROCESS) {
			if (!ipcp->Active) {
				proc_lock_done(ipcp,proc_ipl);
				port_lock_done(prp,port_ipl);
				GiveOwnership (g, pRIGHTS_IN_TRANSIT);
				SendKernelMsg (ipcp, M_OwnershipRights, TypePtOwnership, g);
			} 
			else {
				proc_lock_done(ipcp,proc_ipl);
				port_lock_done(prp,port_ipl);
				GiveOwnership (g, ipcp);
			}
		} else {
			proc_lock_done(ipcp,proc_ipl);
			port_lock_done(prp,port_ipl);
		}
		GiveReceiveRights (g, ipcp);
		break;

	case TypePtAll: 
		proc_lock_done(ipcp,proc_ipl);
		port_lock_done(prp,port_ipl);
		GiveOwnership (g, ipcp);
		GiveReceiveRights (g, ipcp);
		break;

	default:
		proc_lock_done(ipcp,proc_ipl);
		port_lock_done(prp,port_ipl);
		break;
		
	}
}


return_ LTran (ipcp, /**/ lp, pt, deallocate)
register procp_ ipcp;
port_ *lp;
long pt;
long_bool deallocate;
/****************************************************************************
* Abstract:
*     This routine translates a local port number to a global
*   port number for message transmission.  If if transfer
*   of ownership or receive rights occurs, rights are passed
*   to the kernel for that period of time that the message
*   is in transit.
*
* Parameters:
*       ipcp    - process number
*       gp      - on input a local port number for ipcp, on output
*                 a global port number
*       pt      - type of rights being transferred
*       deallocate - a boolean indicating whether or not the port
*                    should be deallocated
*****************************************************************************/
{
	port_ gp = 0;
	register procp_ Me;
	register portp_ prp;
	int proc_ipl, port_ipl;

	FastLTran (ipcp, *lp, gp);
	*lp = gp;
	if ((gp == NullPort)) {
		return SUCCESS;
	}


	prp = &(PortArray[gp]) ;
	port_lock_read(prp,port_ipl);
	proc_lock_read(ipcp,proc_ipl);

	switch (pt) {
	case TypePtOwnership: 
		if (prp->Owner != ipcp) {
			proc_lock_done(ipcp,proc_ipl);
			port_lock_done(prp,port_ipl);
			return BadRights;
		}
		proc_lock_done(ipcp,proc_ipl);
		port_lock_done(prp,port_ipl);
		GiveOwnership (gp, pRIGHTS_IN_TRANSIT);
		break;

	case TypePtReceive: 
		if (prp->Receiver != ipcp) {
			proc_lock_done(ipcp,proc_ipl);
			port_lock_done(prp,port_ipl);
			return BadRights;
		}
		proc_lock_done(ipcp,proc_ipl);
		port_lock_done(prp,port_ipl);
		GiveReceiveRights (gp, pRIGHTS_IN_TRANSIT);
		break;

	case TypePtAll: 
		if ((prp->Receiver != ipcp) ||
		    (prp->Owner != ipcp)) {
			proc_lock_done(ipcp,proc_ipl);
			port_lock_done(prp,port_ipl);
			return BadRights;
		}
		proc_lock_done(ipcp,proc_ipl);
		port_lock_done(prp,port_ipl);
		GiveOwnership (gp, pRIGHTS_IN_TRANSIT);
		GiveReceiveRights (gp, pRIGHTS_IN_TRANSIT);
		break;

	default:
		proc_lock_done(ipcp,proc_ipl);
		port_lock_done(prp,port_ipl);
		break;
	}

	if (deallocate) {
		RemPort (CurrentProcess(), gp);
	}
	return SUCCESS;
}

void SendAcknowledge (kmsgptr)
register kmsgp_ kmsgptr;
/****************************************************************************
* Abstract:
*
* Parameters:
*
* Environment:
*
* Results:
*
* Side Effects:
*
* Errors:
*
* Calls:
*
* Design:
*****************************************************************************/
{
	port_ lport;


	FastGTran (kmsgptr->WakeupProc, lport, kmsgptr->MsgHeader.RemotePort);
	SendKernelMsg (kmsgptr->WakeupProc, M_MsgAccepted, TypeInt32, lport);

}


void SendKernelMsg (ipcp, kid, type1, data1)
procp_ ipcp;
long type1;
long    data1;
/****************************************************************************
* Abstract:
*
* Parameters:
*
*****************************************************************************/
{
	kmsgp_ kmsgptr;
	port_ gp, kp;
	int proc_ipl;

	proc_lock_read(ipcp,proc_ipl);

	if (ipcp->Active) {

		proc_lock_done(ipcp,proc_ipl);

		MKMsgAllocate(kmsgptr);

		GetKernelPort (ipcp, gp);
		GetDataPort (ipcp, kp);
		PackMsg1 (kmsgptr, EmergencyMsg, gp, kp, kid, type1, data1);

		(void) QueueMsg (kmsgptr);
	} else {
		proc_lock_done(ipcp,proc_ipl);
	}
}

/* ************************************************************************ *
*		Interface with unix sleep/wakeup ...
* ************************************************************************ */

asleepexpire(ipcp)
procp_ ipcp;
{
	register struct proc *p;
	register int s;
	int proc_ipl;
	
	p = (struct proc *)ipcp->task->proc;

	s = splhigh();
	proc_lock_write(ipcp,proc_ipl);

	if (((ipcp->State == MSG_WAIT) || (ipcp->State == FULL_WAIT))&& (p->p_stat == SSLEEP)) {
		ipcp->State = TIMED_OUT;
		setrun(p);
	}
	
	proc_lock_done(ipcp,proc_ipl);
	(void) splx(s);
}

old_asleepexpire(chan)
procp_ chan;
{
	wakeup(chan);
}

asleep(chan, pri, time)
{
	if (setjmp(&u.u_qsave)) {
		return -1;
	} 
	else {
		if (time)
			timeout(old_asleepexpire, chan, time);
		sleep(chan, pri);
		return 1;
	}
}

