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
 * 10-Jun-86  Rick Rashid (rfr) at Carnegie-Mellon University
 *	IPCWakeup now releases the process lock.
 *
 *  4-Jun-86  Rick Rashid (rfr) at Carnegie-Mellon University
 *	Added ipl parameter to locking macros.  Software interrupts are 
 *	now off during locks.  This allows software interrupt handlers
 *	to send messages (in particular to call QueueMsg).
 *
 *  3-Jun-86  Rick Rashid (rfr) at Carnegie-Mellon University
 *	Continued cleanup and parallelization of IPC code.
 *
 * 25-May-86  Rick Rashid (rfr) at Carnegie-Mellon University
 *	Added code for multiprocessor locking of IPC data structures.
 *	Note that we still need to change references to "process" to be
 *	appropriate task and thread references.
 *
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
*		SHOULD BE TAKEN FIRST.  To ensure that software interrupt
*		handlers can send messages, an ipl value is used to keep
*		track of the ipl level before a lock and to restore it.
*		Normally, proc locks and port locks use variables named
*		proc_ipl and port_ipl as arguments to store and restore
*		the ipl level.  During a lock, the ipl level should always
*		be raised by splipc() and the ipl is returned in the 
*		ipl parameter to the lock routine.  The ipl parameter
*		specified to the unlock routine is used to restore the ipl
*		level to its initial value.
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
/***************************************************************************
* Abstract:
*     Allocates a port reserved for "servers"  for process proc.
* The port may not be in use.  This mechanism is used to establish well
* known ports for server processes.
*
* Results:
*       Returns an unused port identifier.
****************************************************************************/
{
	register portp_ prp = &PortArray[port];
	register procp_ ipcp = CurrentProcess();
	int port_ipl, proc_ipl;

	/*
	* Setup default return value:
	*/

	if ( port > LastInitPort) {
		return BadInitPort;
	} 
	else {
		port_lock_write(prp,port_ipl);
		proc_lock_read(ipcp,proc_ipl);
		if (!prp->InUse) {
			prp->Locked = FALSE;
			prp->Backlog = MaxBackLog;
			prp->Receiver = ipcp;
			prp->Owner = ipcp;
			queue_init(&prp->SecondaryMsgQ);
			prp->NumQueued = 0;
			bzero(prp->Senders, sizeof(procs_));
			_bit_set(pidx(ipcp), prp->Senders);
			prp->InUse = TRUE;
			proc_lock_done(ipcp,proc_ipl);
			port_lock_done(prp,port_ipl);
			return SUCCESS;
		}
		else {
			proc_lock_done(ipcp,proc_ipl);
			port_lock_done(prp,port_ipl);
			return InitPortInUse;
		}
	} 
}

Send()
/***************************************************************************
*	OUT OF DATE CALLING ROUTINE
****************************************************************************/
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
/***************************************************************************
* Abstract:
*       Send transmits a message from the current process to the
*       RemotePort specified in the message header.
* Parameters:
*     Msgptr  -	a p to the message to be sent.
*     MaxWait -	the maximum time in milliseconds to wait should
*                     the destination port queue be full before giving up
*                     (where a wait time of zero implies infinity).
*     Option  -	specifies whether a send 
*                    operation should Wait, DontWait or Reply if the
*                    destination port is full at the time of the send.
*		     The low order 3 bits determines the type of operation
*		     option requested. 
*                    The various options are:
*                       Wait-
*			     should be used when the sending process wishes
*                            to be suspended for MaxWait milliseconds if the
*                            queue is full.  If by that time the port is
*                            still full, the call returns without having sent
*                            the message.
*                       DontWait-
*			     should be used if the sending process does
*                            not wish to wait for any length of time in the
*                            case of a full destination port.
*                       Reply-
*			     allows the sender to give exactly one message
*                            to the operating system without being suspended
*                            should the destination port be full.  When that
*                            message can in fact be posted to the receiving
*                            port's queue, a message is sent to the DATA_PORT
*                            of the sending process notifying it that another
*                            message can be sent.  A second message sent to
*                            a full port from the same process before this
*                            notification arrives results in an error.
*			Send_Rec_DefaultPts -
*			     Do a send with the Wait option and then a receive
*			     with the DefaultPts option where the wait time is
*			     the high order 29 bits of the option field.
*			Send_Rec_AllPts
*			     Do a send with the Wait option and then a receive
*			     with the AllPts option where the wait time is
*			     the high order 29 bits of the option field.
*			Send_Rec_LocalPt
*			     Do a send with the Wait option and then a receive
*			     with the LocalPt option where the wait time is
*			     the high order 29 bits of the option field.
*
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
*                 not possessed by the current process
****************************************************************************/
{
	extern int CopyInMsg();
	return_ TPReceive();

	register procp_ 	ipcp = CurrentProcess();
	register kmsgp_ 	kmsgptr;
	register port_ 		lp;
	register port_ 		rp;
	register long_bool 	emerg;
	msg_ 			usermsg;
	return_ 		ReturnValue = SUCCESS;
	long_bool 		do_receive = FALSE;
	int 			receive_msgsize;
	int 			receive_pt_option;
	procp_ 			RecProc;
	int			proc_ipl, port_ipl;

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
	* Test to see that we are sending the message to a legitimate
	* port.  If not, destroy the message and continue:
	*/

	if (rp != NullPort) {
		register portp_ prp = &(PortArray[rp]);

		port_lock_read(prp,port_ipl);

		if (!prp->InUse) {
			port_lock_done(prp,port_ipl);
			KMsgDeallocateAll(kmsgptr);
			return NotAPort;
		}

		RecProc = prp->Receiver;

		/* 
		* Handle specially the case when we are sending a
		* message to the kernel.
		*/

		if (RecProc == pKERNEL_PROCESS) {

			/* Release locks */
			port_lock_done(prp,port_ipl);

			/* 
			* Make sure kernel is not the one to get reply (avoids
			* infinite loop):
			*/

			if ((PortArray[lp].Receiver == pKERNEL_PROCESS)) {
				KMsgDeallocateAll (kmsgptr);
				return BadKernelMsg;
			} 
			else {
				AccentMsg (kmsgptr);
				if (do_receive) {
					return TPReceive(msgptr,MaxWait,receive_pt_option,
							 InternalCall,ipcp,receive_msgsize,lp);
				} else {
					return ReturnValue;
				}
			}
		}

		/* 
		* If this is an emergency message or if
		* we are queueing a message when the prp->backlog is
		* less than the port defined prp->backlog,
		* we must check to see if the process needs to
		* to be interrupted or woken up.
		*/

		/* 
		 * Port record prp is still locked for read!
		 */

		if ((prp->NumQueued < prp->Backlog) || emerg) {
			register intr_ pcbp = &(RecProc->pcb);

			proc_lock_read(RecProc,proc_ipl);

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

			kmsgptr->What = DONT_WAKE;

			/* 
			* If emergency message, insert message at
			* head of queue rather than end and do not
			* increment prp->NumQueued.
			*/

			port_lock_read_to_write(prp,port_ipl);
			proc_lock_read_to_write(RecProc,proc_ipl);

			if (emerg) {
				enqueue_head (&RecProc->MsgsWaiting, kmsgptr);
			} 
			else {
				enqueue (&RecProc->MsgsWaiting, kmsgptr);
				prp->NumQueued++;
			}

			/* 
			 * We release the port lock first!
			 * We must use the proc_ipl here since we acquired the
			 * port lock first and the proc lock last.
			 */
			if (RecProc->State == MSG_WAIT) {
				port_lock_done(prp,proc_ipl);
				IPCWakeup(RecProc);
				/* IPCWakeup now releases process lock. */
				/* Reset interrupt level */
				(void) splipc_done(port_ipl);
			} else {
				port_lock_done(prp,port_ipl);
				proc_lock_done(RecProc,proc_ipl);
			}
		} 
		else {
			/* 
			* port is full, check our options:
			*/

			/* Port prp is still locked for read */

			if (Option == DontWait) {
				port_lock_done(prp,port_ipl);
				KMsgDeallocateAll (kmsgptr);
				return PortFull;
			}

			/* 
			* If we are supposed to wait, enqueue
			* the message on the port secondary message queue.
			*/

			port_lock_read_to_write(prp,port_ipl);
			proc_lock_write(ipcp,proc_ipl);

			kmsgptr->WakeupProc = ipcp;
			enqueue (&prp->SecondaryMsgQ, kmsgptr);
			prp->NumQueued++;

			if (Option == Reply) {
				kmsgptr->What = SEND_ACK;
				ReturnValue = WillReply;
				proc_lock_done(ipcp,proc_ipl);
				port_lock_done(prp,port_ipl);
			} 
			else {
				kmsgptr->What = WAKE_ME;
				/* Use proc_ipl because we acquired proc lock last */
				port_lock_done(prp,proc_ipl);
Label4: 
				/* Port lock still in force.*/

				IPCWait(ipcp,MaxWait,FULL_WAIT);
				if (ipcp->State != NOT_WAITING) {
					/* 
					*
					* We timed out.  Check to see if we were waiting
					* forever in that case, we were probably
					* Resumed and should go back to sleep.
					*/
					if (MaxWait == 0 && (ipcp->State == TIMED_OUT))
						goto Label4;

					/* Use port_ipl because we are releasing locks in reverse order */
					proc_lock_done(ipcp,port_ipl);

					port_lock_write(prp,port_ipl);
					if (delqueue (&prp->SecondaryMsgQ, kmsgptr)) {
						prp->NumQueued--;
						port_lock_done(prp,port_ipl);
						KMsgDeallocateAll (kmsgptr);
					} else {
						port_lock_done(prp,port_ipl);
					}
					return (ipcp->State == TIMED_OUT)?	PortFull : Intr;
				} else {
					/* Use port_ipl because we are releasing locks in reverse order */
					proc_lock_done(ipcp,port_ipl);
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
/***************************************************************************
*	NO LONGER USED
****************************************************************************/
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
/****************************************************************************
* Abstract:
*    Receive retrieves the next message from a port queue on which the current
*    process is waiting.
* Parameters:
*    Msgptr-a p to a message data structure into which a message
*           can be received.
*    MaxWait-the maximum time in milliseconds to wait for a message before
*            giving up (where a wait time of zero implies infinity).
*    PtOption- an enumerated type specifying what port or sets of ports
*            the receive should take place on.
*               DefaultPts- use ports already set by SetPortsWaiting.
*               AllPts- receive on all ports.
*               LocalPt - receive only on the port stored in Msg.LocalPort
*    Option - If Option != InternalCall, then get rest of arguments from msgptr, else use
*	      following arguments
*    rec_ipcp - pointer to process record for receiving process
*    rec_msg_size - size of message to be received into
*    rec_lp - local port to receive message on (if PtOption == LocalPt)
*
* Returns:
*     SUCCESS-The message has been received.
*     TIME_OUT-The message was not received after MaxWait milliseconds.
*     MEM_FAULT-The message pointed to a data area inaccessible to the receiver.
****************************************************************************/
{
	register procp_	ipcp;
	register kmsgp_	kmsgptr;
        register port_ 	lp;
	register port_ 	rp;
	register portp_	prp;
	register queue_t msg_waiting_q;
	long 		receive_msgsize;
	port_ 		tmp;
	long 		msize;
	kmsgp_ 		nextkmsg,tmpptr;
	msg_ 		usermsg;
	int		proc_ipl,port_ipl;

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

	proc_lock_write(ipcp,proc_ipl);
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
		* Well, Lets wait for it. A wait of ImmediateTimeout means dont wait at all.
		*/

		if (MaxWait != ImmediateTimeout) {
			IPCWait(ipcp,MaxWait,MSG_WAIT);
		} else
			ipcp->State = TIMED_OUT;

		/*
		* I wonder what happened, possibly we try again at WHILE
		*/

		/* If we got a message, then continue while loop. */
		if (ipcp->State == NOT_WAITING) continue;

		/* We must have either timed out, or gotten an interrupt. */
		if (ipcp->State != TIMED_OUT) {
			proc_lock_done(ipcp,proc_ipl);
			return Intr;
		} else {
			proc_lock_done(ipcp,proc_ipl);
			return TimeOut;
		}

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

	/* A write lock to ipcp is held at this point in the code. */

	tmpptr = kmsgptr; /* can't use register variable here */
	remqueue (msg_waiting_q, tmpptr);

	proc_lock_done(ipcp,proc_ipl);

	prp = &(PortArray[rp]);
	port_lock_write(prp,port_ipl);

	if (kmsgptr->MsgHeader.MsgType != EmergencyMsg)
		prp->NumQueued--;

	proc_lock_write(ipcp,proc_ipl);

	if (!queue_empty(&prp->SecondaryMsgQ)) {
		nextkmsg = (kmsgp_)dequeue_head(&prp->SecondaryMsgQ);
		enqueue_tail(msg_waiting_q, (queue_entry_t)nextkmsg);
	}

	proc_lock_done(ipcp,proc_ipl);
	port_lock_done(prp,port_ipl);
	
	if ((kmsgptr->What == WAKE_ME)) {
		proc_lock_write(kmsgptr->WakeupProc,proc_ipl);
		IPCWakeup (kmsgptr->WakeupProc);
		/* IPCWakeup releases process lock */
		(void) splipc_done(proc_ipl);
	} else {
		if ((kmsgptr->What == SEND_ACK)) {
			SendAcknowledge (kmsgptr); 
		}
	}

	kmsgptr->What = DONT_WAKE;

	/* 
	* Check for simple case:
	*/

	/* There are no locks held at this point */

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
/****************************************************************************
*	OBSOLETE
*****************************************************************************/
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
/****************************************************************************
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
*****************************************************************************/
{
	register procp_ ipcp = CurrentProcess();
	register long i;
	register port_ lp;
	register port_ gp;
	int port_ipl;

	if (!useracc((caddr_t)LockPortArray, sizeof(port_)*LockPortCnt, 1)) {
		uprintf("cant read LockPortArray (%x).\n", (int)LockPortArray);
		return NotAUserAddress;
	}

	for (i = 0; i <= LockPortCnt - 1; i++) {
		lp = LockPortArray[i];
		FastLTran (ipcp, lp, gp);
		if (gp != NullPort) {
			port_lock_write((&PortArray[gp]),port_ipl);
			if (PortArray[gp].Receiver == ipcp) {
				PortArray[gp].Locked = LockDoLock;
			}
			port_lock_done((&PortArray[gp]),port_ipl);
		}
	}
	return SUCCESS;

}      /* TPLockPorts */

MessagesWaiting()
/****************************************************************************
*	OBSOLETE
*****************************************************************************/
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
/****************************************************************************
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
*****************************************************************************/
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
	int proc_ipl;

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

	proc_lock_read(ipcp,proc_ipl);
	if (queue_empty(&ipcp->MsgsWaiting)) { 
		int tmp = 0;
		proc_lock_done(ipcp,proc_ipl);
		/* Test for no messages  waiting */
		if (copyout(&tmp, varMsgWPortCnt, sizeof (int)))
			return NotAUserAddress;
		return SUCCESS;
	}

	/* Read lock on ipcp held at this point */

	count = 0;
	msgptr = (kmsgp_)queue_first(&ipcp->MsgsWaiting);
	while (!queue_end(&ipcp->MsgsWaiting,(queue_entry_t)msgptr)) {
		if (msgptr->MsgHeader.MsgType == MsgWType)
			count++;
		msgptr = (kmsgp_)queue_next((queue_entry_t)msgptr);
	}

	proc_lock_done(ipcp,proc_ipl);

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

	/* No locks held */

	proc_lock_read(ipcp,proc_ipl);

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

	proc_lock_done(ipcp,proc_ipl);

	return SUCCESS;
}      /* TPMessagesWaiting */

SoftEnable()
/****************************************************************************
*	OBSOLETE
*****************************************************************************/
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
/****************************************************************************
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
*****************************************************************************/
{
	register procp_ ipcp = CurrentProcess();
	register intr_ pcbp = &ipcp->pcb;
	register kmsgp_ kmsgptr;
	int proc_ipl;

	SoftOnOrOff(ipcp, NormalOrEmergency, EnableOrDisable);

	/* Interrupts may now be enabled - check conditions and set flags: */

	proc_lock_read(ipcp,proc_ipl);

	if (!queue_empty(&ipcp->MsgsWaiting)) {
		kmsgptr = (kmsgp_) queue_first(&ipcp->MsgsWaiting);
		while (PortArray[kmsgptr->MsgHeader.RemotePort].Locked) {
			kmsgptr = (kmsgp_)queue_next((queue_entry_t)kmsgptr);
			if (queue_end(&ipcp->MsgsWaiting,(queue_entry_t)kmsgptr)) {
				proc_lock_done(ipcp,proc_ipl);
				return SUCCESS;
			}
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

	proc_lock_done(ipcp,proc_ipl);
	return SUCCESS;
}

KPortToPID()
/****************************************************************************
*	OBSOLETE
*****************************************************************************/
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
/****************************************************************************
*	OBSOLETE
*****************************************************************************/
{
	register procp_ ipcp = CurrentProcess();
	register int gp;

	FastLTran(ipcp, lp, gp);
	if (PortArray[gp].Receiver == pKERNEL_PROCESS)
		return ((struct proc *)PortArray[gp].Owner->task->proc)->p_pid;
	else
		return -1;
}

Cheat()
/****************************************************************************
*	OBSOLETE
*****************************************************************************/
{
	return_ TPCheat();
	register struct a {
		int lp;
	} 
	*uap = (struct a *) u.u_ap;

	u.u_r.r_val1 = TPCheat(uap->lp);
}


return_ TPCheat(lp)
/****************************************************************************
*	OBSOLETE
*****************************************************************************/
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

