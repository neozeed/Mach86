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
*  File: 	Accent.PasMac
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
*****************************************************************************
* ABSTRACT:
*    This is the basic entry point for all MACH IPC calls.
*****************************************************************************
* HISTORY
 * 13-May-86  Rick Rashid (rfr) at Carnegie-Mellon University
 *	Removed bogus "mp" queuing and increment/decrement calls.
 *
 * 13-May-86  Rick Rashid (rfr) at Carnegie-Mellon University
 *	Removed references to kmsgptr->DataPtr, continued cleanup.
 *
 * 12-May-86  Rick Rashid (rfr) at Carnegie-Mellon University
 *	Modified TPSend to do combined send/receive if option field is
 *	set properly (see code for details).  Performed a bit of overall
 *	code cleanup and simplification.
 *
 *  6-May-86  Avadis Tevanian (avie) at Carnegie-Mellon University
 *	Changed calling sequence to vm_allocate.
 *
 *  6-May-86  Rick Rashid (rfr) at Carnegie-Mellon University
 *	Changed to add setting of TimerSet flag before call to asleep to
 *	avoid unnecessary untimeout calls.
 *
 * 30-Apr-86  Rick Rashid (rfr) at Carnegie-Mellon University
 *	Removed vm_kern.h include file.  It is now in vax/impl_macros.h.
 *
* 29-Apr-86  Richard F. Rashid
*		Changed various printf calls to uprintf. Added include of
*		vm_kern.h to get kernel memory allocation routines.
* 19-Mar-86  Richard F. Rashid
*		Changed handling of secondary/primary queues on full ports so that
*		the sender would only be woken if a secondary queue message was
*		later received rather than when it was posted on primary queue.
* 19-Feb-86  Richard F. Rashid
*	    	Restructured some parts of code and added calls to vm copy routines.
*  3-Dec-85  Robert V Baron (rvb) at Carnegie-Mellon University
*		Total rewrite of MessagesWaiting so it works.
*  25-Sep-85  Richard F. Rashid
*	       	Fixed MsgsWaiting and SoftEnable to not look at dummy queue header
*	       	as if it were a message.  Added error checking in MsgsWaiting and
*	       	LockPorts.
*  24-Sep-85  Richard F. Rashid
*	       	Fixed handling of DEFAULT_PORTS option and locked ports 
*	       	so that locked ports can be received on when option is not
*	       	DEFAULT_PORTS.
*  24-Sep-85  Richard F. Rashid
*	       	Fixed handling of locked ports in SoftEnable.
*  23-Jan-84  Michael B. Jones
*             	Added LockPorts and MessagesWaiting to replace the obselete
*             	SetPortsWaiting and PortsWithMessages calls.
* ...
* ...
*             	Moved PrintContext to VMDebug. Simplied entry in DoAccent.
*  30-Jul-81  Richard F. Rashid
*             	Started.
****************************************************************************/

#include "../h/queue.h"
#include "../accent/accent.h"
#include "../accent/accentint.h"
#include "../vm/vm_map.h"
#include "../vm/vm_user.h"
#include "../h/task.h"
#include "../h/thread.h"

InitPort()
{
	return_ TPInitPort();
	register struct a {
		port_ port;
		long MaxBackLog;
	} 
	*uap = (struct a *) u.u_ap;

	u.u_r.r_val1 = TPInitPort(uap->port, uap->MaxBackLog);
}


return_ TPInitPort (port, MaxBackLog)
port_ port;
long MaxBackLog;
/*
* Abstract:
*     Allocates a port reserved for "servers"  for process proc.
* The port may not be in use.
*
* Results:
*       Returns an unused port identifier.
*/
{
	register procp_ ipcp = CurrentProcess();
	register portp_ prp = &PortArray[port];

	/*
	* Setup default return value:
	*/

	if ( port > LastInitPort) {
		return BadInitPort;
	} 
	else if (non_blockingP(prp->InUse)) {
		prp->Locked = FALSE;
		prp->Backlog = MaxBackLog;
		prp->Receiver = ipcp;
		prp->Owner = ipcp;
		initqueue(&prp->SecondaryMsgQ);
		prp->NumQueued = 0;
		bzero(prp->Senders, sizeof(procs_));
		_bit_set(pidx(ipcp), prp->Senders);
		return SUCCESS;
	} 
	else {
		return InitPortInUse;
	}
}

Send()
{
	return_ TPSend();
	register struct a {
		msgp_ msgptr;
		long MaxWait;
		long Option;
	} 
	*uap = (struct a *) u.u_ap;

	u.u_r.r_val1 = TPSend(uap->msgptr, uap->MaxWait, uap->Option);
}

return_ TPSend (msgptr, MaxWait, Option)
msgp_ msgptr;
long MaxWait;
long Option;
/*
* Abstract:
*       Send transmits a message from the current process to the
*       RemotePort specified in the message header.
* Parameters:
*     Msgptr-a p to the message to be sent.
*     MaxWait-the maximum time in milliseconds to wait should
*                     the destination port queue be full before giving up
*                     (where a wait time of zero implies infinity).
*     Option-an enumerated type specifying whether the send
*                    operation should Wait, DontWait or Reply if the
*                    destination port is full at the time of the send.
*                    The various options are:
*                       WAIT-should be used when the sending process wishes
*                            to be suspended for MaxWait milliseconds if the
*                            queue is full.  If by that time the port is
*                            still full, the call returns without having sent
*                            the message.
*                       DONT_WAIT-should be used if the sending process does
*                            not wish to wait for any length of time in the
*                            case of a full destination port.
*                       REPLY-allows the sender to give exactly one message
*                            to the operating system without being suspended
*                            should the destination port be full.  When that
*                            message can in fact be posted to the receiving
*                            port's queue, a message is sent to the DATA_PORT
*                            of the sending process notifying it that another
*                            message can be sent.  A second message sent to
*                            a full port from the same process before this
*                            notification arrives results in an error.
* Returns:
*    SUCCESS-The message has been queued for the RemotePort.
*    TIME_OUT-The message was not sent since the destination port was
*                 still full after MaxWait milliseconds.
*    FAILURE-The message was not sent because the destination port was
*                 full and DontWait was specified.
*    WILL_REPLY-The destination port was full but the Reply option was
*                 specified.  An emergency message will be sent when the
*                 message can be posted.
*    TOO_MANY_REPLIES-The Reply option was specified but a reply request
*                 is already outstanding for this process and destination port.
*    MEM_FAULT-The message pointed to data not in the address space of
*                 the sender.
*    NOT_PORT-The message refers to a port number which is not
*                 available to the current process.
*    BAD_RIGHTS-The message contains a reference to port access rights
*                 not possessed by the current process.
*/
{
	extern int CopyInMsg();
	return_ TPReceive();
	register procp_ ipcp = CurrentProcess();
	register kmsgp_ kmsgptr;

	register port_ lp;
	register port_ rp;
	register long_bool emerg;
	msg_ usermsg;
	return_ ReturnValue = SUCCESS;

	long_bool do_receive = FALSE;
	int receive_msgsize;
	int receive_pt_option;

	procp_ RecProc;
	long_bool interrupt;

	if (copyin(msgptr, &usermsg, sizeof (msg_))) {
		uprintf("cannot read message (%x), bad user address!!!\n", msgptr);
		return NotAUserAddress;
	}

	lp = usermsg.LocalPort;
	rp = usermsg.RemotePort;
	emerg = (usermsg.MsgType == EmergencyMsg);

	/*
	 * Check for send/receive pair.  If option specifies a send/receive pair then
	 * setup for call to TPReceive after TPSend.  Note that message size of receive
	 * is in the high order 29 bits of Option field.
	 */
	if ((Option&7) == Send_Rec_AllPts) {
		do_receive = TRUE;
		receive_pt_option = AllPts;
		receive_msgsize = Option>>3;
		Option = Wait;
	} else {
		if ((Option&7) == Send_Rec_DefaultPts) {
			do_receive = TRUE;
			receive_pt_option = DefaultPts;
			receive_msgsize = Option>>3;
			Option = Wait;
		} else {
			if ((Option&7) == Send_Rec_LocalPt) {
				do_receive = TRUE;
				receive_pt_option = LocalPt;
				receive_msgsize = Option>>3;
				Option = Wait;
			}
		}
	} 

	Statistics.MessagesSent++;

	/* 
	* Test for simplest case:
	*/

	if (usermsg.SimpleMsg && (usermsg.MsgSize < MaxKMsgSize)) {
		/* 
		* We now know that there are no port rights imbedded in
		* the message body and that the message is less than the
		* maximum size of a kmsg.
		*/

		/* 
		* Allocate a kmsg structure to hold the message:
		*/

		MKMsgAllocate(kmsgptr);

		/* 
		* Now, copy the message from the users structure into
		* the allocated kmsg:
		*/

		if (copyin(msgptr, &kmsgptr->MsgHeader, usermsg.MsgSize)) {
			KMsgDeallocateAll (kmsgptr);
			return NotAUserAddress;
		}

		kmsgptr->DataSize = usermsg.MsgSize - sizeof (msg_);

		/* 
		* Translate the port rights to their global form and
		* save their values in lp, rp respectively:
		*/

		FastLTran (ipcp, lp, lp);
		FastLTran (ipcp, rp, rp);

		kmsgptr->MsgHeader.LocalPort  = lp;
		kmsgptr->MsgHeader.RemotePort = rp;
	} 
	else {
		int ret;
		kmsgp_ shadow;

		Statistics.ComplexMessages++;
		if ((ret = CopyInMsg (msgptr, &shadow)) != SUCCESS) {
			KMsgDeallocateAll(shadow);
			return ret;
		} 
		kmsgptr = shadow;
		lp = kmsgptr->MsgHeader.LocalPort;
		rp = kmsgptr->MsgHeader.RemotePort;
	}

	/* 
	*
	* This section of code is a copy of VMIPC.QueueMsg
	*/

	/* 
	* Test to see that we are sending the message to a legitimate
	* port.  If not, destroy the message and continue:
	*/

	if (rp != NullPort) {
		register portp_ prp = &(PortArray[rp]);

		if (!prp->InUse) {
			KMsgDeallocateAll(kmsgptr);
			return NotAPort;
		}
		RecProc = prp->Receiver;

		/* 
		* Handle specially the case when we are sending a
		* message to the kernel.
		*/

		if (RecProc == pKERNEL_PROCESS) {

			/* 
			* Make sure we have a reply port:
			*/

			if (lp == NullPort) {
				AccentMsg (kmsgptr);
				if (do_receive) {
					return TPReceive(msgptr,MaxWait,receive_pt_option,
							 InternalCall,ipcp,receive_msgsize,lp);
				} else {
					return ReturnValue;
				}
			}

			/* 
			* Make sure kernel is not the one to get reply (avoids
			* infinite loop):
			*/

			if ((PortArray[lp].Receiver == pKERNEL_PROCESS)) {
				KMsgDeallocateAll (kmsgptr);
				return BadKernelMsg;
			} 
			AccentMsg (kmsgptr);
			if (do_receive)
				return TPReceive(msgptr,MaxWait,receive_pt_option,
						 InternalCall,ipcp,receive_msgsize,lp);

			return ReturnValue;
		}

		/* 
		* If this is an emergency message or if
		* we are queueing a message when the prp->backlog is
		* less than the port defined prp->backlog,
		* we must check to see if the process needs to
		* to be interrupted or woken up.
		*/

		if ((prp->NumQueued < prp->Backlog) || emerg) {
			register intr_ pcbp = &(RecProc->pcb) ;

			if (pcbp->EMsgEnable || pcbp->MsgEnable) {
				if (prp->Locked)	
					interrupt = FALSE;
				else {
					if (((emerg) && pcbp->EMsgEnable)) {
						interrupt = TRUE;
						SetSoft (RecProc, TRUE);
						pcbp->EMsgEnable = FALSE;
					} 
					else if (pcbp->MsgEnable) {
						interrupt = TRUE;
						SetSoft (RecProc, FALSE);
						pcbp->MsgEnable = FALSE;
					} 
					else {
						interrupt = FALSE;
					}
				}
			}

			kmsgptr->What = DONT_WAKE;

			/* 
			* If emergency message, insert message at
			* head of queue rather than end and do not
			* increment prp->NumQueued.
			*/

			if (emerg) {
				enqueue_head (&RecProc->MsgsWaiting, kmsgptr);
			} 
			else {
				enqueue (&RecProc->MsgsWaiting, kmsgptr);
				prp->NumQueued++;
			}

			if (RecProc->State == MSG_WAIT) IPCWakeup(RecProc);
		} 
		else {
			/* 
			* port is full, check our options:
			*/

			if (Option == DontWait) {
				KMsgDeallocateAll (kmsgptr);
				return PortFull;
			}

			/* 
			* If we are supposed to wait, enqueue
			* the message on the port secondary message queue.
			*/

			enqueue (&prp->SecondaryMsgQ, kmsgptr);
			prp->NumQueued++;
			kmsgptr->WakeupProc = ipcp;

			if (Option == Reply) {
				kmsgptr->What = SEND_ACK;
				ReturnValue = WillReply;
			} 
			else {
				int wake_reason;
				kmsgptr->What = WAKE_ME;
Label4: 
				IPCWait(MaxWait,FULL_WAIT,&wake_reason);
				if (wake_reason <= 0) {
					/* 
					*
					* We timed out.  Check to see if we were waiting
					* forever in that case, we were probably
					* Resumed and should go back to sleep.
					*/
					if (MaxWait == 0 && !wake_reason)
						goto Label4;

					if (delqueue (&prp->SecondaryMsgQ, kmsgptr)) {
						prp->NumQueued--;
						KMsgDeallocateAll (kmsgptr);
					} 
					return (!wake_reason)?	PortFull : Intr;
				}
			}
		}
	} 
	else {
		/* 
		* Destroy the allocated message and inform the user
		* that he used an illegal port as the destination.
		*/

		KMsgDeallocateAll (kmsgptr);
		return NotAPort;
	}
	if (do_receive) {
		return TPReceive(msgptr,MaxWait,receive_pt_option,InternalCall,ipcp,receive_msgsize,lp);
	} else {
		return ReturnValue;
	}
}

Receive()
{
	return_ TPReceive();
	register struct a {
		msgp_ msgptr;
		long MaxWait;
		long PtOption;
		long Option;
	} 
	*uap = (struct a *) u.u_ap;

	u.u_r.r_val1 = TPReceive (uap->msgptr, uap->MaxWait, uap->PtOption, uap->Option, (procp_)0, 0, (portp_)0);
}


return_ TPReceive (msgptr, MaxWait, PtOption, Option, rec_ipcp, rec_msg_size, rec_lp)
msgp_ msgptr;
long MaxWait;
long PtOption;
long Option;
procp_ rec_ipcp;
long rec_msg_size;
port_ rec_lp;
/*
* Abstract:
*    Receive retrieves the next message from a port queue on which the current
*    process is waiting (see SetPortsWaiting.)
* Parameters:
*    Msgptr-a p to a message data structure into which a message
*           can be received.
*    MaxWait-the maximum time in milliseconds to wait for a message before
*            giving up (where a wait time of zero implies infinity).
*    PtOption- an enumerated type specifying what port or sets of ports
*            the receive should take place on.
*               DefaultPts- use ports already set by SetPortsWaiting.
*               ALL_PORTS- receive on all ports.
*               LocalPt - receive only on the port stored in Msg.LocalPort
*    receive_msgsize - size of message to be received into, if zero, then check msgptr->MsgSize.
* Returns:
*     SUCCESS-The message has been received.
*     TIME_OUT-The message was not received after MaxWait milliseconds.
*     MEM_FAULT-The message pointed to a data area inaccessible to the receiver.
*/

{
	register procp_ ipcp;
	register kmsgp_ kmsgptr;
	register procp_ RecProc;
        register port_ lp;
	register port_ rp;
	register portp_ prp;
	register queue_t msg_waiting_q;
	long receive_msgsize;
	port_ tmp;
	int wake_reason;
	long msize;
	long_bool gotmsg;
	kmsgp_ nextkmsg,tmpptr;
	msg_ usermsg;

	/* 
	* Setup default return value:
	*/
	return_ ReturnValue = SUCCESS;

	if (Option != InternalCall) {
		/* Validity of user msgptr address is in doubt.  Try to copy it into a local
		 * record to see if it is ok.
		 */
		if (copyin(msgptr, &usermsg, sizeof (msg_))) {
			uprintf("cant read message (%x), bad user address!!!\n", msgptr);
			return NotAUserAddress;
		}
		ipcp = CurrentProcess();
		receive_msgsize = usermsg.MsgSize;
		FastLTran(ipcp,usermsg.LocalPort,lp);
	}
	else {
		ipcp = rec_ipcp;
		receive_msgsize = rec_msg_size;
		lp = rec_lp;
	}

	msg_waiting_q = &ipcp->MsgsWaiting;

	while (TRUE) {
		/* 
		* Loop, looking on queue for any message that we are waiting for:
		*/
		if (!queue_empty(msg_waiting_q)) {
			kmsgptr = (kmsgp_)queue_first(msg_waiting_q);
			while (!queue_end(msg_waiting_q, (queue_entry_t)kmsgptr)) {
				/* 
				* Take anything we get:
				*/
				rp = kmsgptr->MsgHeader.RemotePort;
				if (PtOption == AllPts) {
					goto Label4;
				}
				if ((PtOption == LocalPt) && (lp == rp)) {
					goto Label4;
				}
				if ((PtOption == DefaultPts) && (!PortArray[rp].Locked)){
					goto Label4;
				}
				kmsgptr = (kmsgp_)queue_next((queue_entry_t)kmsgptr);
			}
		}

		/* 
		* Well, Lets wait for it. A wait of NOWAIT means dont wait at all.
		*/
		gotmsg = FALSE;

		if (MaxWait != ImmediateTimeout) {
			IPCWait(MaxWait,MSG_WAIT,&wake_reason);
			gotmsg = (ipcp->State == NOT_WAITING);
		}

		/*
		* I wonder what happened, possibly we try again at WHILE
		*/

		{
			register intr_ pcbp = &(ipcp->pcb);

			if (pcbp->EMsgPending || pcbp->MsgPending) {
				/* 
				* We were interrupted by an emergency message or
				* a normal message:
				*/
				ReturnValue = MsgInterrupt;
			} 
			else {
				/* 
				* If we got a message gotmsg will be TRUE:
				*/
				if (gotmsg)
					continue;
				/* 
				* OTHERwise, we probably timed out. Check to
				* see if we were waiting forever:
				* (If we were waiting forever and woke up anyway,
				*  we were probably Resumed.  Go back to sleep.)
				*/
				if (wake_reason == -1)
					return Intr;
				else /* We timed out */
				return TimeOut;
			}
		}
		return ReturnValue;
	} /* end lookup cycle */
Label4: 

	/* 
	* Check to see if we really wanted to receive a message or
	* just wait for it.
	*/
	/* 
	* Assuming the message has not been previewed,
	* remove it from the waiting queue and wakeup the
	* sender if he is waiting for a wakeup.  Also,
	* move any messages from the secondary queue
	* to the primary queue.
	*/

	tmpptr = kmsgptr; /* can't use register variable here */
	remqueue (msg_waiting_q, tmpptr);

	prp = &(PortArray[rp]);
	if (kmsgptr->MsgHeader.MsgType != EmergencyMsg)
		prp->NumQueued--;

	if ((kmsgptr->What == WAKE_ME)) {
		IPCWakeup (kmsgptr->WakeupProc);
		kmsgptr->What = DONT_WAKE;
	}

	if (!queue_empty(&prp->SecondaryMsgQ)) {
		nextkmsg = (kmsgp_)dequeue_head(&prp->SecondaryMsgQ);
		enqueue (msg_waiting_q, nextkmsg);
		if ((nextkmsg->What == WAKE_ME)) {
			/* New idea: dont wake up sending process until queue is empty */
			/* IPCWakeup (nextkmsg->WakeupProc) */
		}
		else { 
			if ((nextkmsg->What == SEND_ACK))
				SendAcknowledge (nextkmsg); 
			nextkmsg->What = DONT_WAKE;
		}
	}

	/* 
	* Check for simple case:
	*/

	msize = kmsgptr->MsgHeader.MsgSize;
	if (kmsgptr->MsgHeader.SimpleMsg && (msize < MaxKMsgSize)) {
		if ((msize > receive_msgsize)) {
			KMsgDeallocateAll(kmsgptr);
			return NotEnoughRoom;
		}
		/* 
		* Copy message from kmsg structure to users
		* msg structure and translate port rights:
		*/
		tmp = kmsgptr->MsgHeader.LocalPort;
		FastGTran (ipcp, kmsgptr->MsgHeader.LocalPort, rp);
		FastGTran (ipcp, kmsgptr->MsgHeader.RemotePort, tmp);

		if (copyout(&kmsgptr->MsgHeader, msgptr, msize)) {
			KMsgDeallocateAll(kmsgptr);
			return NotAUserAddress;
		}
	} 
	else {
		int ret;
		if ((ret = CopyOutMsg (msgptr,kmsgptr,FALSE)) != SUCCESS) {
			KMsgDeallocateAll(kmsgptr);
			return ret;
		}
	}

	/* KMsgDeallocateAll (kmsgptr); */
	MKMsgFree(kmsgptr);

	return ReturnValue;
	/* TPReceive */;
}

LockPorts()
{
	return_	TPLockPorts();
	register struct a {
		long_bool LockDoLock;
		ptrPortArray LockPortArray;
		long LockPortCnt;
	} 
	*uap = (struct a *) u.u_ap;

	u.u_r.r_val1 = TPLockPorts (uap->LockDoLock, uap->LockPortArray, uap->LockPortCnt);
}

return_ TPLockPorts (LockDoLock, LockPortArray, LockPortCnt)
long_bool LockDoLock;
ptrPortArray LockPortArray;
long LockPortCnt;
/*
* Abstract:
*       Either lock or unlock the ports at (*LockPortArray), depending
*       upon LockDoLock.
*
* Parameters:
*       (Parameters are at  (*ArgPtr))
*       LockDoLock      True if doing a lock, else an unlock.
*       LockPortArray     Pointer to the port array.
*       LockPortCnt     Number of ports in the array.
*
* Results:
*       (Results are at (*ArgPtr))
*       ReturnValue     SUCCESS
*
* Historical Footnote:
*       This routine obseletes the older SetPortsWaiting call.
*/
{
	register procp_ ipcp = CurrentProcess();
	register long i;
	register port_ lp;
	register port_ gp;

	if (!useracc((caddr_t)LockPortArray, sizeof(port_)*LockPortCnt, 1)) {
		uprintf("cant read LockPortArray (%x).\n", (int)LockPortArray);
		return NotAUserAddress;
	}

	for (i = 0; i <= LockPortCnt - 1; i++) {
		lp = LockPortArray[i];
		FastLTran (ipcp, lp, gp);
		if (gp != NullPort) {
			if (PortArray[gp].Receiver == ipcp) {
				PortArray[gp].Locked = LockDoLock;
			}
		}
	}
	return SUCCESS;

}      /* TPLockPorts */

MessagesWaiting()
{
	return_ TPMessagesWaiting();
	register struct a {
		long MsgWType;
		ptrPortArray *varMsgWPortArray;
		int *varMsgWPortCnt;
	} 
	*uap = (struct a *) u.u_ap;

	u.u_r.r_val1 = TPMessagesWaiting (uap->MsgWType,
					  uap->varMsgWPortArray,
					  uap->varMsgWPortCnt);
}


return_ TPMessagesWaiting (MsgWType, varMsgWPortArray, varMsgWPortCnt)
long MsgWType;
ptrPortArray *varMsgWPortArray;
int *varMsgWPortCnt;
/*
* Abstract:
*       Return a list of ports for which there are outstanding messages
*       of type MsgWType.  The algorithm used actually returns the
*       port for each outstanding message of the proper type, resulting
*       in potentially duplicated ports in the list.
*
* Parameters:
*       (Parameters are at ArgPtr^)
*       MsgWType         MsgWType of messages waiting to return.
*       MsgWPortArray           Pointer to where results wanted, or nil to do an alloc.
*       MsgWPortCnt      Max number of ports possible when MsgWPortArray non-nil.
*
* Results:
*       (Results are at ArgPtr^)
*       MsgWPortArray           Pointer to the result port array.
*       MsgWPortCnt      Number of ports in the result array (even on failure).
*       RESULT          SUCCESS, or NotEnoughRoom if actual number of ports
*                       is greater than the max given by MsgWPortCnt.
*
* Historical Footnote:
*       This routine obseletes the older PortsWithMessages call.
*/
{
	register procp_ ipcp = CurrentProcess();
	register kmsgp_ msgptr;
	register long pass;
	register long count;
	register port_ lp;
	register port_ gp;
	ptrPortArray PortA;
	int PortCnt;
	long    Words;
	kern_return_t result;

	if (copyin(varMsgWPortCnt, &PortCnt, sizeof (int)))
		return NotAUserAddress;
	if (copyin(varMsgWPortArray, &PortA, sizeof (ptrPortArray)))
		return NotAUserAddress;

	/* Test for validity of array if not null */
	if (PortA != (ptrPortArray)NULL) {
		if (!useracc((caddr_t)PortA, sizeof(port_)*PortCnt, 0)) {
			return NotAUserAddress;
		}
	}

	if (queue_empty(&ipcp->MsgsWaiting)) { 
		int tmp = 0;
		/* Test for no messages  waiting */
		if (copyout(&tmp, varMsgWPortCnt, sizeof (int)))
			return NotAUserAddress;
		return SUCCESS;
	}

	count = 0;
	msgptr = (kmsgp_)queue_first(&ipcp->MsgsWaiting);
	while (!queue_end(&ipcp->MsgsWaiting,(queue_entry_t)msgptr)) {
		if (msgptr->MsgHeader.MsgType == MsgWType)
			count++;
		msgptr = (kmsgp_)queue_next((queue_entry_t)msgptr);
	}

	{
		int tmp = count;
		if (copyout(&tmp, varMsgWPortCnt, sizeof (int)))
			return NotAUserAddress;
		if (count == 0)
			return SUCCESS;
		else if (count > PortCnt && PortA != (ptrPortArray) NULL)
			return NotEnoughRoom;
	}

	if (PortA == (ptrPortArray) NULL) {
		result = vm_allocate(current_task()->map,(vm_offset_t)&PortA,
				      (vm_size_t)count*sizeof(port_),TRUE);
		if (result != KERN_SUCCESS) return NotEnoughRoom;

		if (copyout(&PortA, varMsgWPortArray, sizeof (ptrPortArray)))
			return NotAUserAddress;
	}

	count = 0;
	msgptr = (kmsgp_)queue_first(&ipcp->MsgsWaiting);
	while (!queue_end(&ipcp->MsgsWaiting,(queue_entry_t)msgptr)) {
		if (msgptr->MsgHeader.MsgType == MsgWType) {
			gp = msgptr->MsgHeader.RemotePort;
			FastGTran (ipcp, lp, gp);
			PortA[count] = lp;
			count++;
		}
		msgptr = (kmsgp_)queue_next((queue_entry_t)msgptr);
	}

	return SUCCESS;
}      /* TPMessagesWaiting */

SoftEnable()
{
	return_ TPSoftEnable();
	register struct a {
		int NormalOrEmergency;
		int EnableOrDisable;
	} 
	*uap = (struct a *) u.u_ap;

	u.u_r.r_val1 = TPSoftEnable(uap->NormalOrEmergency, uap->EnableOrDisable);
}


return_ TPSoftEnable( NormalOrEmergency, EnableOrDisable)
/*
* Abstract:
*   Enables or disables software interrupts for a process.  Enabling an
*   interrupt raises the interrupt if the condition for the interrupt
*   is TRUE.
* Parameters:
*   Process:    process to enable or disable
*   NormalOrEmergency:
*               TRUE for normal message interrupts, FALSE for emergency
*   EnableOrDisable:
*               TRUE to enable interrupts (and raise interrupt if condition
*               is TRUE), FALSE to disable  interrupts.
*/
{
	register procp_ ipcp = CurrentProcess();
	register intr_ pcbp = &ipcp->pcb;
	register kmsgp_ kmsgptr;

	SoftOnOrOff(ipcp, NormalOrEmergency, EnableOrDisable);


	/* Interrupts may now be enabled - check conditions and set flags: */


	if (!queue_empty(&ipcp->MsgsWaiting)) {
		kmsgptr = (kmsgp_) queue_first(&ipcp->MsgsWaiting);
		while (PortArray[kmsgptr->MsgHeader.RemotePort].Locked) {
			kmsgptr = (kmsgp_)queue_next((queue_entry_t)kmsgptr);
			if (queue_end(&ipcp->MsgsWaiting,(queue_entry_t)kmsgptr))
				return SUCCESS;
		}


		/* Emergency message interrupts are for emergency messages only */
		if (pcbp->EMsgEnable &&
		    (kmsgptr->MsgHeader.MsgType == EmergencyMsg)) {
			SetSoft(ipcp, TRUE);
			pcbp->EMsgEnable = FALSE;
		} 
		else if (pcbp->MsgEnable) {
			/* Normal message interrupts are for both types of  messages. */
			SetSoft(ipcp, FALSE);
			pcbp->MsgEnable = FALSE;
		}
	}
	return SUCCESS;
}

KPortToPID()
{
	return_ TPKPortToPID();
	register struct a {
		int lp;
	} 
	*uap = (struct a *) u.u_ap;

	u.u_r.r_val1 = TPKPortToPID(uap->lp);
}

return_ TPKPortToPID(lp)
int lp;
{
	register procp_ ipcp = CurrentProcess();
	register int gp;

	FastLTran(ipcp, lp, gp);
	if (PortArray[gp].Receiver == pKERNEL_PROCESS)
		return PortArray[gp].Owner->task->proc->p_pid;
	else
		return -1;
}

Cheat()
{
	return_ TPCheat();
	register struct a {
		int lp;
	} 
	*uap = (struct a *) u.u_ap;

	u.u_r.r_val1 = TPCheat(uap->lp);
}


return_ TPCheat(lp)
{
	register procp_ ipcp = CurrentProcess();

	uprintf("TPCheat: Proc = %d, Lp = %d ==> ", pidx(ipcp), lp) ;

	if (suser() && (lp < NumPorts)) {
		if (PortArray[lp].InUse) {
			_bit_set(pidx(ipcp), PortArray[lp].Senders);
			return SUCCESS;
		}
	}

	return Failure;
}

