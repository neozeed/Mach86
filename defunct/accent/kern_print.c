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
 * File:	osprint.c
 *
 * Abstract:
 *	Print out Accent OS Structures.
 *
 * Author:	Robert V. Baron
 *		Copyright (c) 1984 by Robert V. Baron
 *
 * History:	Created Nov/30/84
 *
 ************************************************************************
 * HISTORY
 * 13-May-86  Rick Rashid (rfr) at Carnegie-Mellon University
 *	Removed obsolete references to kmsgptr->DataPtr.
 *
 *  4-May-86  Avadis Tevanian (avie) at Carnegie-Mellon University
 *	Relocate queuescan to mp/queuescan
 *
 ************************************************************************
 */

#if 0

#include "../h/queue.h"
#include "../accent/accent.h"
#ifndef	KERNEL
#include <accent/acc_macro.h>
#include <accent/kern_ipc.h>
#include <accent/vax/impl_proc.h>
#endif	KERNEL


extern char *msgtype_strs[];
extern char *send_option_strs[];
extern char *receive_option_strs[];
extern char *port_options_strs[];
extern char *distinguished_Portstrs[];
extern char *deallocate_reason_strs[];
extern char *send_type_strs[];
extern char *proc_state_strs[];
extern char *accent_errors[];
extern char *matchmaker_errors[];
extern char *kernel_msgid_strs[];

printkmsg(kmsgp)
kmsgp_ kmsgp;
{
	printf("Kmsg @%x: ",kmsgp);
	printmsgheader(&kmsgp->MsgHeader);
	printf("Kmsg ");
	printf("F = %x, B = %x, ",
		kmsgp->qhead.mpq_next,
		kmsgp->qhead.mpq_prev);
	printf("VA = %x, L = %d, ", kmsgp->Data, kmsgp->DataSize);
	printf("Wake = %d, Type = %d\n", pidx(kmsgp->WakeupProc), kmsgp->What);
	if (!kmsgp->DataSize) {
		/* printdescriptor has been designed so that if it gets called
		   as printdescriptor(x,x), it will try to print one descriptor
		   thus you don't really have to know how big a descriptor is 
		   to print it.  Unfortunately printmsg now has to be smart, if
		   there is no message */
		printf("NO DATA IN MESSAGE\n");
	} else {
		printf("DESCRIPTOR: start\n");
		printdescriptor(kmsgp->Data,
				&((char *) kmsgp->Data)[kmsgp->DataSize]);
		printf("DESCRIPTOR: END\n");
	}
}

printport(prp)
portp_ prp;
{
register int j;

	printf("PortRecord #%d: Recv = %d, Own = %d, #%d(Max %d), Inuse %d, Lock %d\n",
		prp - PortArray,
		pidx(prp->Receiver),
		pidx(prp->Owner),
		prp->NumQueued,
		prp->Backlog,
		prp->InUse,
		prp->Locked);
	printf("PortRecver: ");
	for (j = KERNEL_PROCESS; j <= LAST_USED_PROCESS + 1; j++)
		if (_bit_tst(j, prp->Senders))
				printf("%d, ", j);
	printf("\n");
	printqueue("SecondaryMsgQ", prp->SecondaryMsgQ);
}

printipc(ipcp)
procp_ ipcp;
{
	printf("IPCRECORD #%d: Active = %d, State = \"%s\", WaitOption = \"%s\"\n",
		ipcp - ProcArray,
		ipcp->Active,
		proc_state_strs[(int) ipcp->State],
		port_options_strs[ipcp->WaitOption]);
	printf("unix env : uid = %d, pid = %d, procp = %x\n",
		ipcp->uid, ipcp->pid, ipcp->procp);
	printf("IPCports : KPort = %d, DPort = %d, WPort = %d\n",
		ipcp->KPorts[KernelPort], ipcp->KPorts[DataPort],
		ipcp->WaitPt);
	printf("IPCpcb   : NEnable = %d, NPending %d// EEnable = %d, EPending = %d\n",
		ipcp->pcb.MsgEnable, ipcp->pcb.MsgPending,
		ipcp->pcb.EMsgEnable, ipcp->pcb.EMsgPending);

	printqueue("MSG_Waiting", ipcp->MsgsWaiting);
	printf("Preview @%x", ipcp->PreviewMsg);
	if (ipcp->PreviewMsg)
		printkmsg(ipcp->PreviewMsg);
	else
		printf("\n");
	printf("IPCRECORD #%d: END\n", ipcp - ProcArray);
}

printqueue(str, kmsgptr)
char *str;
register kmsgp_ kmsgptr;
{
register int len = 0;
register kmsgp_ lastp;

	if (!kmsgptr) {
		printf("queue: %s uninitialized\n", str);
		return;
	}

	printf("queue: %s @%x Size = ", str, kmsgptr);

	lastp = kmsgptr;
	kmsgptr = qnext(kmsgptr, kmsgp_);
	for ( ; lastp != kmsgptr ; kmsgptr = qnext(kmsgptr, kmsgp_)) {
		len++;
	}
	printf("%d\n", len);

	kmsgptr = qnext(kmsgptr, kmsgp_);
	for ( ; lastp != kmsgptr ; kmsgptr = qnext(kmsgptr, kmsgp_)) {
		printkmsg(kmsgptr);
	}
}

queuecheck(str, kmsgptr, limit)
char *str;
register mpqhead_t *kmsgptr;
{
register int len = 0;
register mpqhead_t *lastp;

	if (!kmsgptr) {
		printf("queuecheck: %s uninitialized\n", str) ;
		return ;
	}

	printf("queuecheck: %s @%x Size = ", str, kmsgptr) ;

	lastp = kmsgptr;
	kmsgptr = qnext(kmsgptr, mpqhead_t *);
	for ( ; lastp != kmsgptr ; kmsgptr = qnext(kmsgptr, mpqhead_t *)) {
		len++ ;
		if (limit && len > limit) {
			printf("MUNGED\n");
			return 0;
		}
	}
	printf("%d\n", len) ;
	return 1;

}

queuecheckcells(str, kmsgptr, limit)
char *str;
register ptr_ kmsgptr;
register int limit;
{
register int len = 0;
register ptr_ lastp;
register int i = 0;

	if (!kmsgptr) {
		printf("queuecheck_cells: %s uninitialized\n", str);
		return;
	}

	printf("queuecheck_cells: %s @%x Size = ", str, kmsgptr);

	lastp = kmsgptr;
	kmsgptr = qnext(kmsgptr, ptr_);
	for ( ; lastp != kmsgptr ; kmsgptr = qnext(kmsgptr, ptr_)) {
		len++;
		if (limit && len > limit) {
			printf("MUNGED\n");
			return 0;
		}
	}
	printf("%d\n", len);
	kmsgptr = qnext(kmsgptr, ptr_);
	for ( ; lastp != kmsgptr ; kmsgptr = qnext(kmsgptr, ptr_)) {
		printf("%d @%x\t", i, kmsgptr);
		if ( !((++i) % 5) )
			printf("\n");
	}
	if ( (i % 5) )
		printf("\n");
}

#endif 0
