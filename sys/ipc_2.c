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
 *  File: ipc2.c (Copyright 1980 (c) Richard F. Rashid)
 *
 *********************************************************************
 * HISTORY
 * 06-Aug-85  Mike Accetta (mja) at Carnegie-Mellon University
 *	Upgraded from 4.1BSD.
 *	[V1(1)]
 *
 * 22-Jun-85  Glenn Marcy (gm0w) at Carnegie-Mellon University
 *	Rewrote memory allocation routines to use their own memory
 *	allocation pool (V3.07p).
 *
 *  7-Feb-85  Glenn Marcy (gm0w) at Carnegie-Mellon University
 *	Changed debug printfs to follow message with newline, rather than
 *	precede the message with one.  Added more debug statements and
 *	changed the base of pointers printed from decimal to hex (V3.07n).
 *
 * 15-Jan-85  Glenn Marcy (gm0w) at Carnegie-Mellon University
 *	Another illegal call to SendKernelMsg.  Don't send an ack for
 *	a message moved into the primary queue if the process that sent
 *	the message is no longer active (V3.07n).
 *
 *  2-Jan-85  Glenn Marcy (gm0w) at Carnegie-Mellon University
 *	Fixed inconsistant usage of booleans that was detect by new
 *	boolean enumerated type.
 *
 * 18-Oct-84  Glenn Marcy (gm0w) at Carnegie-Mellon University
 *	Changed references to KMsgType and MsgType for new MsgType
 *	union.
 *
 * 12-Apr-84  Glenn Marcy (gm0w) at Carnegie-Mellon University
 *	Only call SendKernelMsg in DestroyPort if the process is active.
 *	It is sometimes the case that while removing ports in
 *	IPCSuicide, deleting one port can cause a message to be sent to
 *	another port in the same process.  Even though we aren't active
 *	any longer, DestroyPort would still try to send a kernel message
 *	(V3.07i).
 *
 * 16-Feb-82  Mike Accetta (mja) at Carnegie-Mellon University
 *	Changed CMU_IPC conditional to NCMUIPC (V3.04b).
 *
 * 21-Aug-81  Mike Accetta (mja) at Carnegie-Mellon University
 *	Upgraded for 4.1BSD by changing all references to "NRPOC" to instead
 *	use the new dynamically determined "nproc" variable (V3.00).
 *
 * 25-May-81  Rick Rashid (rfr) at Carnegie-Mellon University
 *      Fixed reply option in Send by correcting typo.  Changed SendAck(kmsg)
 *      to SendAck(skmsg) in AcceptMsg. 
 *
 * 28-Mar-81  Rick Rashid (rfr) at Carnegie-Mellon University
 *      Restored a previous change which had somehow gotten lost.
 *      Used a 0177 mask to get only the DescName field of a data
 *      type in a linear structure in the routine TranslateMsg.
 *
 *********************************************************************
 *
 *  Purpose: 
 *      The UNIX IPC facility: basic utilities.
 *
 *  Author:
 *      Richard F. Rashid 
 */

#include "cs_ipc.h"
#include "mach_vm.h"

#if	CS_IPC
#include "../h/param.h"
#include "../h/systm.h"
#include "../h/dir.h"
#include "../h/user.h"
#include "../h/proc.h"
#include "../h/buf.h"
#include "../h/conf.h"
#include "../h/queue.h"
#include "../h/map.h"
#include "../h/kernel.h"
#include "../h/ipc.h"
#include "../h/ipcmacros.h"

#if	MACH_VM
#include "../vm/vm_map.h"
#include "../vm/vm_kern.h"
#endif	MACH_VM

/*
 *  Data structures:
 *      FreeData
 *      FreeKMsgQ
 *      FreeNameQ
 *      ProcTable
 *      FreePortQ
 *      FreeEntryQ
 *      LocalToGlobal
 *      GlobalToLocal
 *      IPCNameTable
 *
 *  Purpose:
 *      All global data structures are defined here.
 *    See the descriptions associated with the data structure
 *    definitions above for details.
 *
 */

struct Queue FreeKMsgQ;
struct Queue FreePortQ;
struct Queue FreeEntryQ;
struct Queue FreeNameQ;

struct HashTable LocalToGlobal;
struct HashTable GlobalToLocal;
struct HashTable IPCNameTable;
struct FreeDataTable FreeData;

/*
 *  Global variables:
 *      IPCStarted
 *
 *  Purpose:
 *      These global variables contain information
 *    for the current IPC call. IPCStarted
 *    is a flag which indicates that the IPC facility is
 *    already in use and initialized.
 *
 */

boolean IPCStarted;
int MemUsed = 0;

#define READ    1
#define WRITE   0


/*
 *******************************************************************
 *******************************************************************
 *              Utilities for handling ports.                      *
 *                      FindPort                                   *
 *                      NewPort                                    *
 *                      RemPort                                    *
 *                      GiveSendRights                             *
 *                      GiveReceiveRights                          *
 *                      GiveOwnership                              *
 *******************************************************************
 *******************************************************************
 */

FindPort(prc)
process prc;
  {
    integer      i;
    struct Proc *procptr;

#if PORTTRACE
printf("FindPorts: prc = %d\n",prc);
#endif

    procptr = &ProcTable[prc];

    /* Find a local port number not in use. */

    for (i = FIRSTNONRESERVEDPORT; i < MAXPORTS; i++)
      {
        if (!TestBit(&(procptr->PortsInUse),i))
          {
            return(i);
          }
      }

    return(NULL);
  }

NewPort(prc,lp,backlog)
process prc;
localport     lp;
unsigned char backlog;
  {
    register struct Port *port;
    struct Proc          *p;

#if PORTTRACE
printf("NewPort: prc = %d, lp = %d, backlog = %d\n",prc,lp,backlog);
#endif

    p = &ProcTable[prc];

    /* Find a free port. */
    port = GetPort();
    
    SetBit((char *)&(p->PortsInUse),lp);

    /* Initialize port. */
    port->Owner         = prc;
    port->Receiver      = prc;
    port->NumQueued     = 0;
    port->Backlog       = backlog;
    initQueue(&(port->Link));
    initQueue(&(port->SecondaryMsgQ));

    if (lp == KERNELPORT) port->Receiver = KERNELPROC;

    /* Enter portnames into hash tables. */

    HashEnter(&LocalToGlobal,PortHash,prc,(int)lp,(int)port,0);
    HashEnter(&GlobalToLocal,PortHash,prc,(int)port,(int)lp,0);

    /* Return local port name to process. */       
    return(lp);    
  }

RemPort(prc,lp)
process   prc;
localport lp;
  {
    struct Port        *port;
    struct Proc        *procptr;

#if PORTTRACE
printf("RemPort: prc = %d, lp = %d\n",prc,lp);
#endif

    /* Port can be deallocated from this process' name space. */

    GTPort(port,prc,lp);
    procptr = &ProcTable[prc];

    HashRemove(&GlobalToLocal,PortHash,(int (*)())PortEqual,prc,(int)port);
    HashRemove(&LocalToGlobal,PortHash,(int (*)())PortEqual,prc,(int)lp);
    ClearBit((char *)&(procptr->PortsInUse),lp);

    /* Check for ownership or receive rights. */

    if ((prc == port->Receiver) || (prc == port->Owner))
      {

        if (((prc == port->Receiver) && (prc == port->Owner)) ||
            (port->Receiver == KERNELPROC) ||
            (port->Receiver == NOONE) ||
            (port->Owner == NOONE))
          {
            /* Must be owner and receiver. */
            /* Get rid of port completely. */
            DestroyPort(port);
            return;
          }

        /* Check for receive rights only. */
        if (prc == port->Receiver)
          {
            /* Test to see if port ownership is in transit. */
            if (port->Owner == INTRANSIT)
              {
                /* We don't know who eventual receiver will be. 
                 * When ownership right arrives at some destination 
                 * a RECEIVERIGHTS message will be automatically 
                 * sent. (See TranGP.)
                 */
                GiveReceiveRights(port,NOONE);
              }
            else
              {
                /* In a very rare and still not too well understood
                 * way it is possible for the owner to not be active.
                 * This really only occurs when the owner is in
                 * the process of shutting itself down.  In this case
                 * we must destroy the port.
                 */

                if ((&ProcTable[port->Owner])->Active)
                  {
                    SendKernelMsg(port->Owner,RECEIVERIGHTS,
                                  TYPEPTRECEIVERIGHTS,(int)port);
                  }
                else
                  {
                    GiveReceiveRights(port,NOONE);
                    DestroyPort(port);
                  }
              }
          }
        else
          {
            /* Test to see if port receive right is in transit. */
            if (port->Receiver == INTRANSIT)
              {
                /* We don't know who eventual owner will be. 
                 * When receive right arrives at some destination 
                 * an OWNERSHIPRIGHTS message will be automatically 
                 * sent. (See TranGP.)
                 */
                GiveOwnership(port,NOONE);
              }
            else
              {
                /* In a very rare and still not too well understood
                 * way it is possible for the receiver to not be active.
                 * This really only occurs when the receiver is in
                 * the process of shutting itself down.  In this case
                 * we must destroy the port.
                 */

                if ((&ProcTable[port->Receiver])->Active)
                  {
                    SendKernelMsg(port->Receiver,OWNERSHIPRIGHTS,
                                TYPEPTOWNERSHIP,(int)port);
                  }
                else
                  {
                    GiveOwnership(port,NOONE);
                    DestroyPort(port);
                  }
              }
          }
      }
  }

DestroyPort(port)
struct Port *port;
  {
    register integer    i;
    localport           lp;
    struct PortEntry   *pe;
    struct KMsg        *kmsg;
    struct IPCName     *name;
    

#if PORTTRACE
    printf("DestroyPort: port 0x%X\n", port);
#endif
    /* For each process which refers to that port, the port
     * must be deallocated and an emergency message sent
     * which identifies the deallocated port.
     * Then, the port data structure itself must be deleted.
     */

    for (i = 0; i < nproc; i++)
      {
        PortLookup(pe,&GlobalToLocal,i,port);
        if (pe != NULL)
          {
            lp = pe->GTLValue;
	    /* no message if we are on the way out */
	    if ((&ProcTable[i])->Active)
	      SendKernelMsg(i,PORTDELETED,TYPEINTEGER,(int)lp);
            RemPort(i,lp);
          }
      }

    while ((kmsg = DQMsg(port)) != NULL)
      {
        ThrowMsgAway(kmsg);
      }

    /* Delete all names which refer to this port. */
    while ((name = (struct IPCName *)deQueue(&(port->Link))) != NULL)
      {
        Disassociate(name,port);
        FreeIPCName(name);
      }

    port->Receiver = NULL;
    port->Owner = NULL;
    FreePort(port);
  }

GiveSendRights(port,prc)
struct Port *port;
process      prc;
  {
    struct PortEntry *pe;
    localport         lp;

#if PORTTRACE
printf("GiveSendRights: port = 0x%X, prc = %d\n",port,prc);
#endif

    PortLookup(pe,&GlobalToLocal,prc,port);
    
    if (pe == NULL)
      {
        if ((port->Receiver == NULL) && (port->Owner == NULL)) 
          { 
            return(NULL); 
          }
        lp = FindPort(prc);
        HashEnter(&LocalToGlobal,PortHash,prc,(int)lp,(int)port,0);
        HashEnter(&GlobalToLocal,PortHash,prc,(int)port,(int)lp,0);
        SetBit((char *)&(ProcTable[prc].PortsInUse),lp);
        return(lp);
      }
    else 
      {
        return(pe->GTLValue);
      }
  }

GiveReceiveRights(port,prc)
struct Port *port;
process      prc;
  {
    struct KMsg      *kmsg;
    struct Queue     *next;
    struct Proc      *procptr;    
    struct Proc      *oldprocptr;
    localport         lp;

#if PORTTRACE
printf("GiveReceiveRights: port = 0x%X, prc = %d\n",port,prc);
#endif

    /* First move all message from the port's primary queue 
     * associated with the current receiver to the primary queue
     * associated with the new receiver.
     */

    procptr = &ProcTable[prc];
    oldprocptr = &ProcTable[port->Receiver];

    while ((next = (struct Queue *)DQMsg(port)) != NULL)
      {
        kmsg = (struct KMsg *)next;
        UnMsgSignal(oldprocptr,(kmsg->KMsgCode == EMERGENCYMSG));
        enQueue(&(procptr->MsgsWaiting),next);
        MsgSignal(procptr,(kmsg->KMsgCode == EMERGENCYMSG));
      }

    /* Set the receiver field to be the new process. */

    port->Receiver = prc;

    /* Check to see if this process is waiting for a message
     * from this port. If so, wake it up.
     */

    switch (procptr->State)
      {
	case ALLPORTWAIT:
	  {
	    IPCWakeup(procptr);
	  }
	endcase

	case ONEPORTWAIT:
	  {
	    LTPort(lp,prc,port);
	    if (lp == procptr->WaitPort)
	      {
		IPCWakeup(procptr);
	      }
	  }
	endcase

	case MULTIPLEPORTWAIT:
	  {
            LTPort(lp,prc,port);
            if (TestBit(&(procptr->WaitPorts),(int)lp))
              {
                IPCWakeup(procptr);
              }
	  }
	endcase
      }
  }

GiveOwnership(port,prc)
struct Port *port;
process      prc;
  {

#if PORTTRACE
printf("GiveOwnership: port = 0x%X, prc = %d\n",port,prc);
#endif

    port->Owner = prc;
  }

/*
 *******************************************************************
 *******************************************************************
 *                      General utilities.                         *
 *                                                                 *
 *                          SendAck                                *
 *                          SendKernelMsg                          *
 *                          HandleKernelMsg                        *
 *******************************************************************
 *******************************************************************
 */

SendAck(kmsg)
struct KMsg *kmsg;
  {
    process prc;
    localport lp;

    /* This routine is called when a user process has sent a message
     * with the SENDACK option when the receiver's primary queue
     * was full.  The sender's message is put in the secondary queue
     * and when eventually it is placed in the primary queue a MSGACCEPTED
     * message is sent by the kernel to the sending process.
     */

    prc = kmsg->WakeupProc;
    if (!((&ProcTable[prc])->Active))
	return;
    LTPort(lp,prc,kmsg->KRemotePort);
    SendKernelMsg(prc,MSGACCEPTED,TYPEINTEGER,(int)lp);
  }

SendKernelMsg(prc,id,type1,data1)
process prc;
integer id;
integer type1;
integer data1;
  {
    struct KMsg *kmsg;
    struct intarray
      {
	int Data[4];
      };

#if PORTTRACE
printf("SendKernelMsg: prc = %d, id = %d\n",prc,id);
#endif

    /* This routine composes a message in internal form
     * and transmits it to a user process.  Currently all
     * such messages are emergency messages.  The file
     * ipc.h contains a list of possible system message ids.
     */

    if (!((&ProcTable[prc])->Active))
      {
        /* This error condition should not occur.  I am currently
         * very interested in situations in which it does.  As long
         * as this interest persists, I will leave this print statement
         * in and enabled.
         */
        printf("Illegal call to SendKernelMsg: %d,%d,%d,%d,%d\n",prc,id,type1,data1,Me);
        return;
      }

    KMsgAllocate(kmsg);

    kmsg->KLPType       = TYPEPT;
    GTPort(kmsg->KLocalPort,prc,KERNELPORT);
    kmsg->KRPType       = TYPEPT;
    GTPort(kmsg->KRemotePort,prc,DATAPORT);
    kmsg->KID           = id;
    kmsg->KMsgType	= EMERGENCYMSG;
    kmsg->StructSize    = sizeof(struct LSDescriptor);
    kmsg->DataSize      = 4;

    Allocate(kmsg->LinearStructure,(sizeof (int))*3);
    if (kmsg->LinearStructure == NULL)
      {
        KMsgFree(kmsg);
        return;
      }
    kmsg->StructAlloc      = (sizeof (int))*3;
    
    Allocate(kmsg->LinearData,(sizeof (int))*1);
    if (kmsg->LinearData == NULL)
      {
        KMsgFree(kmsg);
        return;
      }
    kmsg->DataAlloc        = (sizeof (int))*1;

    ((struct intarray *)kmsg->LinearStructure)->Data[0] = type1;
    ((struct intarray *)kmsg->LinearStructure)->Data[1] = 1;
    ((struct intarray *)kmsg->LinearStructure)->Data[2] = 0;

    ((struct intarray *)kmsg->LinearData)->Data[0] = data1;

    (void)QueueMsg(kmsg,0,DONTWAIT);
  }

SendAMsg(port,kp,id,type1,data1)
struct Port *port;
struct Port *kp;
integer    id;
integer     type1;
integer     data1;
  {
    struct KMsg *kmsg;
    struct intarray
      {
	int Data[4];
      };

    /* This routine composes a message in internal form
     * and transmits it to a user process.  
     */

    KMsgAllocate(kmsg);

    kmsg->KLPType       = TYPEPT;
    kmsg->KLocalPort    = kp;
    kmsg->KRPType       = TYPEPT;
    kmsg->KRemotePort   = port;
    kmsg->KID           = id;
    kmsg->KMsgType	= NORMALMSG;
    kmsg->StructSize    = sizeof(struct LSDescriptor);
    kmsg->DataSize      = 4;

    Allocate(kmsg->LinearStructure,(sizeof (int))*3);
    if (kmsg->LinearStructure == NULL)
      {
        KMsgFree(kmsg);
        return;
      }
    kmsg->StructAlloc      = (sizeof (int))*3;

    Allocate(kmsg->LinearData,(sizeof (int))*1);
    if (kmsg->LinearData == NULL)
      {
        KMsgFree(kmsg);
        return;
      }
    kmsg->DataAlloc        = (sizeof (int))*1;

    ((struct intarray *)kmsg->LinearStructure)->Data[0] = type1;
    ((struct intarray *)kmsg->LinearStructure)->Data[1] = 1;
    ((struct intarray *)kmsg->LinearStructure)->Data[2] = 0;

    ((struct intarray *)kmsg->LinearData)->Data[0] = data1;

    (void)QueueMsg(kmsg,0,DONTWAIT);
  }

HandleKernelMsg(kmsg)
struct KMsg *kmsg;
  {
    localport     lp2;                
    int           type1,type2;
    process       owner;
    struct Port  *port,*port1,*port2;
    struct PortEntry *pe;
    struct intarray
      {
	int Data[4];
      };
    struct intarray *ls,*ld;

    /* Currently message for the kernel are thrown
     * away.  Eventually a variety of services will
     * be provided through kernel messages.
     */
    switch (kmsg->KID)
      {
        case INTERPOSE:
          {
            port =  kmsg->KRemotePort;
            owner=  port->Owner;
            ls   =  (struct intarray *)kmsg->LinearStructure;
            ld   =  (struct intarray *)kmsg->LinearData;
        
            if ((ls == NULL) || (ld == NULL)) 
              SendAMsg(kmsg->KLocalPort,port,KERNELMSGERROR,TYPEINTEGER,BADMSG);
                    
            /* WARNING!!! The next two statements
             * depend on the structure of the type field in
             * an IPC msg.
             */
            type1 = (ls->Data[0]) & 0177;
            type2 = (ls->Data[3]) & 0177;
            
            port1 = (struct Port *)(ld->Data[0]);
            port2 = (struct Port *)(ld->Data[1]);

            if ((type1 == TYPEPTRECEIVERIGHTS) && 
                ((type2 == TYPEPT) || (type2 == TYPEINTEGER)))
              {
                if (type2 == TYPEINTEGER) 
                  {
                    lp2   = (localport) port2;
                    pe =HashLookup(&LocalToGlobal,PortHash,(int (*)())PortEqual,owner,(int)lp2);
                    if (pe == NULL) 
                      {
                        SendAMsg(kmsg->KLocalPort,port,KERNELMSGERROR,
                                 TYPEINTEGER,BADRIGHTS);
                        endcase;
                      }
                    port2 = (struct Port *)(pe->LTGValue);
                  }
                  
                if ((port2->Receiver) == owner)
                  {
                    pe = HashLookup(&GlobalToLocal,PortHash,(int (*)())PortEqual,owner,(int)port2);                    
                    lp2 = (localport)(pe->GTLValue);

                    /* Replace p2 with p1 as port lp2 in process owner */
                    HashRemove(&GlobalToLocal,PortHash,(int (*)())PortEqual,owner,(int)port2);
                    HashRemove(&LocalToGlobal,PortHash,(int (*)())PortEqual,owner,(int)lp2);
                    HashEnter(&GlobalToLocal,PortHash,owner,(int)port1,(int)lp2,0);
                    HashEnter(&LocalToGlobal,PortHash,owner,(int)lp2,(int)port1,0);

                    GiveReceiveRights(port1,owner);
                    GiveReceiveRights(port2,INTRANSIT);
                    SendAMsg(kmsg->KLocalPort,port,INTERPOSEDONE,TYPEPTRECEIVERIGHTS,(int)port2);
                  }
                else
                  {
                    SendAMsg(kmsg->KLocalPort,port,KERNELMSGERROR,TYPEINTEGER,BADRIGHTS);
                  }
              }
            else
              {
                SendAMsg(kmsg->KLocalPort,port,KERNELMSGERROR,TYPEINTEGER,BADRIGHTS);
              }
          }
        endcase
      }
    
    KMsgFree(kmsg);
  }

/*
 *******************************************************************
 *******************************************************************
 *         Routines which do wakeup and sleep and signalling.      *
 *******************************************************************
 *******************************************************************
 */

IPCTimer(procptr)
struct Proc *procptr;
  {
    procptr->State = TIMEDOUT;
    wakeup((caddr_t)procptr);
  }

MsgSignal(procptr,emergency)					    
struct Proc *procptr;
boolean      emergency;
  {                                                                         
    int msgtype;                                                  
    msgtype = (emergency) ? EMERGENCYMSG : NORMALMSG;             
    if (procptr->Signal[msgtype].Want)                       
      {                                                                     
        if (procptr->Signal[msgtype].Set)                    
          {                                                                 
          }                                                                 
        else                                                                
          {                                                                 
            (procptr)->Signal[msgtype].Set = true;             
            psignal(&proc[procptr-ProcTable],			    
		    (int)procptr->Signal[msgtype].Which);	    
          }                                                                 
        procptr->Signal[msgtype].HowMany++;                  
      }                                                                     
  };                                                                         
                                                                            
UnMsgSignal(procptr,emergency)				    
struct Proc *procptr;
boolean      emergency;
  {                                                                         
    integer msgtype;                                              
    msgtype = (emergency) ? EMERGENCYMSG : NORMALMSG;             
    if (procptr->Signal[msgtype].Want)                       
      {                                                                     
        procptr->Signal[msgtype].HowMany--;                  
      }                                                                     
  };
                                                                            
/*
 *******************************************************************
 *******************************************************************
 *                     Name lookup utilities.                      *
 *                          GetIPCName                             *
 *                          FreeIPCName                            *
 *                          Associate                              *
 *                          Disassociate                           *
 *                          NameHash                               *
 *                          NameEqual                              *
 *******************************************************************
 *******************************************************************
 */

struct IPCName *GetIPCName(userptr)
caddr_t userptr;
  {
    struct IPCName *n;
    char    *nptr;
    register integer i;

    n = GetName();
    nptr = n->String;

#if IPCNAMETRACE
printf("GetIPCName\n");
#endif

    for (i = 0; i < (sizeof n->String)-1; i++)
      {
        nptr[i] = fubyte(userptr++);
        if (nptr[i] == 0) break;
      }

    nptr[i] = 0;
    return(n);
  }

FreeIPCName(n)
struct IPCName *n;
  {
#if IPCNAMETRACE
printf("FreeIPCName\n");
#endif

    FreeName(n);
  }

Associate(n,port)
struct IPCName *n;
struct Port *port;
  {
#if IPCNAMETRACE
printf("Associate: n = %s, port = 0x%X\n",n->String,port);
#endif

    enQueue(&(port->Link),&(n->Link));
    HashEnter(&IPCNameTable,NameHash,(int)n,(int)port,NULL,NULL);
  }

Disassociate(n,port)
struct IPCName *n;
struct Port *port;
  {

#if IPCNAMETRACE
printf("Disassociate: n = %s\n",n->String);
#endif

    remQueue(&(port->Link),&(n->Link));
    HashRemove(&IPCNameTable,NameHash,(int (*)())NameEqual,(int)n,(int)port);
  }

struct Port *FindName(n,oldname)
struct IPCName *n;
struct IPCName **oldname;
  {
    struct PortEntry *pe;
#if IPNAMETRACE
printf("FindName:\n");
#endif

    pe = HashLookup(&IPCNameTable,NameHash,(int(*)())NameEqual,(int)n,NULL);
    if (pe == NULL) return (NULL);
    if (oldname != NULL) *oldname = (struct IPCName *)pe->Key1;
    return ((struct Port *)pe->Key2);
  }

NameHash(n)
struct IPCName *n;
  {
    register integer i;
    register integer sum;
    register char   *nptr;

#if IPCNAMETRACE
printf("NameHash: nptr = %s\n",n->String);
#endif

    sum = 0;
    nptr = n->String;

    for (i = 0; i < sizeof(n->String)-1;i++)
      {
        sum += nptr[i]>>2;
        if (nptr[i] == 0) break;
      }

    return(sum&(TABLESIZE-1));
  }

boolean NameEqual(entry,n)
struct PortEntry *entry;
struct IPCName    *n;
  {
    char *str1;
    char *str2;
    str1 = ((struct IPCName *)entry->Key1)->String;
    str2 = n->String;

#if IPCNAMETRACE
printf("NameEqual: str1 = %s, str2 = %s\n",str1,str2);
#endif

    while ((*str1 != 0) && (*str2 != 0))
      {
        if (*str1 != *str2)
          {

#if IPCNAMETRACE
printf("NameEqual returns false\n");
#endif
            return(false);
          }
        str1++;
        str2++;
      }
    if ((*str1 == 0) && (*str2 == 0))
      {

#if IPCNAMETRACE
printf("NameEqual returns true\n");
#endif
        return(true);
      }
    else
      {

#if IPCNAMETRACE
printf("NameEqual returns: false\n");
#endif
        return(false);
      }
  }

/*
 *******************************************************************
 *******************************************************************
 *                      Hash table utilities.                      *
 *                          HashEnter                              *
 *                          HashLookup                             *
 *                          HashRemove                             *
 *                          PortHash                               *
 *                          PortEqual                              *
 *******************************************************************
 *******************************************************************
 */

HashEnter(table,hash,key1,key2,value1,value2)
struct HashTable *table;
integer (*hash)();
integer key1;
integer key2;
integer value1;
integer value2;
  {
    register struct Queue     *head;
    register struct PortEntry *entry;
    integer                    hashindex;

#if HASHTRACE
printf("HashEnter: key1 = %d, key2 = %d, value1 = %d, value2 = %d\n",
       key1,key2,value1,value2);
#endif

    hashindex = (*hash)(key1,key2);
    head = &(table->HashHead[hashindex]);
    entry = GetHashEntry();
    entry->Key1 = key1;
    entry->Key2 = key2;
    entry->Value1 = value1;
    entry->Value2 = value2;
    enQueue(head,&(entry->Link));
  }

struct PortEntry *HashLookup(table,hash,equal,key1,key2)
struct HashTable *table;
integer         (*hash)();
integer         (*equal)();
integer           key1;
integer           key2;
  {
    register struct Queue *head;
    register struct Queue *next;
    integer                hashindex;

#if HASHTRACE
printf("HashLookup: key1 = %d, key2 = %d\n",key1,key2);
#endif

    hashindex = (*hash)(key1,key2);
    head = &(table->HashHead[hashindex]);
    next = head->F;
    while (next != head)
      {
        if ((*equal)((struct PortEntry *)next,key1,key2))
          {
            return((struct PortEntry *)next);
          }
        next = next->F;
      }
    return(NULL);
  }

HashRemove(table,hash,equal,key1,key2)
struct HashTable *table;
integer (*hash)();
integer (*equal)();
integer key1;
integer key2;
  {
    register struct Queue *head;
    register struct Queue *next;
    integer               hashindex;

#if HASHTRACE
printf("HashRemove: key1 = %d, key2 = %d\n",key1,key2);
#endif

    hashindex = (*hash)(key1,key2);
    head = &(table->HashHead[hashindex]);
    next = head->F;
    while (next != head)
      {
        if ((*equal)((struct PortEntry *)next,key1,key2))
          {
            remQueue(head,next);
            FreeHashEntry((struct PortEntry *)next);
            return;
          }
        next = next->F;
      }
  }

PortHash(key1,key2)
integer key1;
integer key2;
  {
    return((key1 ^ key2))&(TABLESIZE-1);
  }

boolean PortEqual(entry,key1,key2)
struct PortEntry *entry;
integer key1,key2;
  {
    return(((entry->Key1 == key1) && (entry->Key2 == key2)) ? true : false);
  }

/*                                                                          
 *******************************************************************        
 *******************************************************************        
 *          Routine to test validity of local port name            *        
 *******************************************************************        
 *******************************************************************
 */

boolean LegalPort(lp)                                                
localport lp;
  {                                                                         
    if (lp < MAXPORTS)                      
      {                                                                     
        return(TestBit(&(MePtr->PortsInUse),(int)lp));                       
      }                                                                     
    else                                                                    
      {                                                                     
	return(false);
      }                                                                     
  }

/*
 *******************************************************************
 *******************************************************************
 *              Memory allocation.                                 *
 *******************************************************************
 *******************************************************************
 */

#include "../machine/pte.h"

#include "../h/cmap.h"
#include "../h/vm.h"

extern struct pte ipcpt[], Ipcptmap[];
struct map *ipcmap;

/* Turn virtual addresses into ipc map indices */
#define	imxtob(a)	(ipcpt + (a) * NPTEPG)
#define	btoimx(b)	(((b) - ipcpt) / NPTEPG)

/*
 * Allocate wired-down (non-paged) pages in kernel virtual memory.
 */
caddr_t
ipcmemall(pmemall, n)
	int (*pmemall)(), n;
{
	int npg;
	caddr_t va;
	register int a;

#if	MACH_VM
	va = (caddr_t)kmem_alloc(kernel_map, n, TRUE);
#else	MACH_VM
	npg = btoc(n);
	a = rmalloc(ipcmap, (long)npg);
	if (a == 0)
		return (0);
	if ((*pmemall)(&Ipcptmap[a], npg, &proc[0], CSYS) == 0) {
		rmfree(ipcmap, (long)npg, (long)a);
		return (0);
	}
	va = (caddr_t) imxtob(a);
	vmaccess(&Ipcptmap[a], va, npg);
#endif	MACH_VM
	return (va);
}

ipcmemfree(va, n)
	caddr_t va;
	int n;
{
	register int a;
	int npg;

#if	MACH_VM
	kmem_free(kernel_map, va, n);
#else	MACH_VM
	a = btoimx((struct pte *) va);
	npg = btoc(n);
	(void) memfree(&Ipcptmap[a], npg, 0);
	rmfree(ipcmap, (long)npg, (long)a);
#endif	MACH_VM
}

caddr_t GetMemory(size)
  {
    int     i;
    int     realsize, allocsize;
    int     whichqueue;
    caddr_t newdatablock;

    realsize   = SelectAllocSize(size);
    allocsize = realsize < 1024 ? 1024 : realsize;
    MemUsed   += allocsize;
    if ((MemUsed > 90000) && (allocsize == 1024)) 
      {
	u.u_error = OUTOFIPCSPACE;	
        return(NULL);
      }

#if	MACH_VM
    newdatablock = (caddr_t)ipcmemall( (int (*)()) 0,allocsize);
#else	MACH_VM
    newdatablock = (caddr_t)ipcmemall(vmemall,allocsize);
#endif	MACH_VM
    if (newdatablock == NULL)
      {
	printf("Out of IPC space!\n");
	u.u_error = OUTOFIPCSPACE;
	return(NULL);
      }

    if (realsize < 1024)
      {
        whichqueue = SelectAllocQ(size);
        for (i = 0; i <= (1024 - realsize); 
	     i += realsize)           
          {                                                             
            enQueue(&FreeData.DataHead[whichqueue],(struct Queue *)newdatablock);
            newdatablock += realsize;                  
          }                                                             
        return((caddr_t)deQueue(&FreeData.DataHead[whichqueue]));                
      }
    return(newdatablock);
  }

FreeMemory(data,size)
caddr_t data;
  {
    MemUsed -= size;
    ipcmemfree(data,size);
  }

caddr_t GetSpace(size)
  {
#if	MACH_VM
    return((caddr_t)ipcmemall((int (*)()) 0,size));
#else	MACH_VM
    return((caddr_t)ipcmemall(vmemall,size));
#endif	MACH_VM
  }

/*                                                                          
 *******************************************************************        
 *******************************************************************        
 *       Routines to allocate and free port data structures.       *        
 *******************************************************************        
 *******************************************************************
 */

struct Port *GetPort()                                               
  {                                                                         
    int i;                                                                 
    caddr_t newdatablock; 
    struct Port *newport;
                                                  
    while (((struct Queue *)(newport) = deQueue(&FreePortQ)) == NULL)          
      {                                                                     
        newdatablock = GetSpace(1024);                      
        if (newdatablock == 0)                                              
          { 
	    sleep((caddr_t)&FreePortQ,PUSER);
          }                                                                 
        else                                                                
          {                                                                 
            for (i = 0; i <= (1024 - (sizeof (struct Port))); 
		 i += (sizeof (struct Port)))              
              {                                                             
                enQueue(&FreePortQ,(struct Queue *)newdatablock);           
                newdatablock += (sizeof (struct Port));                     
              }                                                             
            (struct Queue *)(newport) = deQueue(&FreePortQ);                
          }                                                                 
      }                                                                     
    return(newport);
  }                         

FreePort(oldport)                                                   
struct Port *oldport;
  {                                                                         
    int wasempty;
    wasempty = Queueempty(&FreePortQ);
    enQueue(&FreePortQ,(struct Queue *)(oldport));                          
    if (wasempty) wakeup((caddr_t)&FreePortQ);
  }                                                                        
            
/*
 *******************************************************************
 *******************************************************************
 *       Routines to allocate and free hash entries.               *
 *******************************************************************
 *******************************************************************
 */

struct PortEntry *GetHashEntry() 
  {                                                                         
    int i;                                                                 
    caddr_t newdatablock;                                                   
    struct PortEntry *newentry;

    while (((struct Queue *)newentry = deQueue(&FreeEntryQ)) == NULL)        
      {                                                                     
        newdatablock = GetSpace(1024);                      
        if (newdatablock == 0)                                              
          {                                                                 
	    sleep((caddr_t)&FreeEntryQ,PUSER);
          }                                                                 
        else                                                                
          {                                                                 
            for (i = 0; i <= (1024 - (sizeof (struct PortEntry))); 
		 i += (sizeof (struct PortEntry)))         
              {                                                             
                enQueue(&FreeEntryQ,(struct Queue *)newdatablock);          
                newdatablock += (sizeof (struct PortEntry));                
              }                                                             
            (struct Queue *)newentry = deQueue(&FreeEntryQ);              
          }                                                                 
      }                                                                     
    return(newentry);
  }                                                                        

FreeHashEntry(oldentry)                                             
struct PortEntry *oldentry;
  {                                                                         
    int wasempty;
    wasempty = Queueempty(&FreeEntryQ);
    enQueue(&FreeEntryQ,(struct Queue *)(oldentry));                        
    if (wasempty) wakeup((caddr_t)&FreeEntryQ);
  }                                                                        
            
/*
 *******************************************************************
 *******************************************************************
 *       Routines to allocate and free ipc string names.           *
 *******************************************************************
 *******************************************************************
 */

struct IPCName *GetName()                                       
  {                                                                         
    int i;                                                                 
    caddr_t newdatablock;                                                   
    struct IPCName *newname;

    while (((struct Queue *)newname = deQueue(&FreeNameQ)) == NULL)          
      {                                                                     
        newdatablock = GetSpace(1024);                      
        if (newdatablock == 0)                                              
          {                                                                 
	    sleep((caddr_t)&FreeNameQ,PUSER);
          }                                                                 
        else                                                                
          {                                                                 
            for (i = 0; i <= (1024 - (sizeof (struct IPCName))); 
		 i += (sizeof (struct IPCName)))           
              {                                                             
                enQueue(&FreeNameQ,(struct Queue *)newdatablock);           
                newdatablock += (sizeof (struct IPCName));                  
              }                                                             
            (struct Queue *)newname = deQueue(&FreeNameQ);                
          }                                                                 
      }                            
    return(newname);                                         
  }                                                                        

FreeName(oldname)                                                   
struct IPCName *oldname;
  {                                                                         
    int wasempty;
    wasempty = Queueempty(&FreeNameQ);
    enQueue(&FreeNameQ,(struct Queue *)oldname);                            
    if (wasempty) wakeup((caddr_t)&FreeNameQ);
  }                                                                        
            
struct KMsg* GetKMsg()                                               
  {                                                                         
    int i;                                                             
    caddr_t newdata;                                                   
    struct KMsg * newkmsg;
    while (((struct Queue *)(newkmsg) = deQueue(&FreeKMsgQ)) == NULL)          
      {                                                                     
        newdata = (caddr_t)GetSpace(1024);                      	    
        if (newdata == 0)                                              
          {                                                                 
	    sleep((caddr_t)&FreeKMsgQ,PUSER);
          }                                                                 
        else                                                                
          {                                                                 
            for (i = 0; i <= (1024 - (sizeof (struct KMsg)));     
		 i += (sizeof (struct KMsg)))                          
              {                                                             
                enQueue(&FreeKMsgQ,(struct Queue *)newdata);           
                newdata += (sizeof (struct KMsg));                     
              }                                                             
            (struct Queue *)(newkmsg) = deQueue(&FreeKMsgQ);                
          }                                                                 
      }                                                                     
    if ((newkmsg) != NULL)						    
      {									    
    	(newkmsg)->StructSize = 0;                                          
    	(newkmsg)->DataSize   = 0;                                          
    	(newkmsg)->CurrentByte= 0;                                          
    	(newkmsg)->StructAlloc= 0;                                          
    	(newkmsg)->DataAlloc  = 0;                                          
    	(newkmsg)->LinearStructure = NULL;				    
    	(newkmsg)->LinearData      = NULL;				    
      }									    
    return(newkmsg);
   }                                                                       
#endif	CS_IPC
