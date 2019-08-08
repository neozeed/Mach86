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
 *  File: ipc1.c (Copyright 1980 (c) Richard F. Rashid)
 *
 *********************************************************************
 * HISTORY
 * 16-Nov-85  Avadis Tevanian (avie) at Carnegie-Mellon University
 *	Fixed IPCDefineSignal to accept all NSIG signals.
 *
 * 06-Aug-85  Mike Accetta (mja) at Carnegie-Mellon University
 *	Upgraded from 4.1BSD.
 *	[V1(1)]
 *
 *  7-Feb-85  Glenn Marcy (gm0w) at Carnegie-Mellon University
 *	Changed debug printfs to follow message with newline, rather than
 *	precede the message with one (V3.07n).
 *
 *  2-Jan-85  Glenn Marcy (gm0w) at Carnegie-Mellon University
 *	Fixed inconsistant usage of booleans that was detect by new
 *	boolean enumerated type.
 *
 * 26-Nov-82  Mike Accetta (mja) at Carnegie-Mellon University
 *	Added some bullet-proofing of paramaters passed to kernel
 *	in signal calls before allowing them to be used as array
 *	indices.
 *
 * 16-Feb-82  Mike Accetta (mja) at Carnegie-Mellon University
 *	Changed CMU_IPC conditional to NCMUIPC (V3.04b).
 *
 * 21-Aug-81  Mike Accetta (mja) at Carnegie-Mellon University
 *	Upgraded for 4.1BSD by changing all references to "NRPOC" to instead
 *	use the new dynamically determined "nproc" variable (V3.00).
 *
 *********************************************************************
 *
 *  Purpose: 
 *      The UNIX IPC facility: system interface routines.
 *
 *  Author:
 *      Richard F. Rashid 
 */

#include "cs_ipc.h"

#if	CS_IPC
#include "../h/param.h"
#include "../h/systm.h"
#include "../h/dir.h"
#include "../h/user.h"
#include "../h/proc.h"
#include "../h/buf.h"
#include "../h/conf.h"
#include "../h/queue.h"
#include "../h/ipc.h"
#include "../h/ipcmacros.h"

/*
 *  Data structures:
 *      FreeData
 *      FreeKMsgQ
 *      ProcTable
 *      FreePortQ
 *      FreeEntryQ
 *      LocalToGlobal
 *      GlobalToLocal
 *      FreeNameQ
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
struct Queue FreeNameQ;
struct Queue FreeEntryQ;

struct Proc *ProcTable;			/* changed 8/21 for 4.1 by mja */
struct HashTable LocalToGlobal;
struct HashTable GlobalToLocal;
struct FreeDataTable FreeData;
struct HashTable IPCNameTable;

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

/*
 *******************************************************************
 *******************************************************************
 *                Basic entry points into IPC                      *
 *                      IPCInit                                    *
 *                      IPCAtrium                                  *
 *                      IPCSuicide                                 *
 *******************************************************************
 *******************************************************************
 */

/*
 *  Procedure:
 *      IPCInit()
 *
 *  Purpose:
 *      Initialize all data structures used by the IPC.
 *
 */

IPCInit()
  {
    integer i;
    caddr_t data;

#if IPCTRACE
printf("IPCInit\n");
#endif

    IPCStarted = true;

    for (i = 0; i < nproc+2; i++)
      {
        ProcTable[i].Active = false;
      }
    
    initQueue(&(ProcTable[KERNELPROC].MsgsWaiting));
    initQueue(&(ProcTable[NOONE].MsgsWaiting));
    initQueue(&(ProcTable[INTRANSIT].MsgsWaiting));

    initQueue(&FreePortQ);
    initQueue(&FreeKMsgQ);
    initQueue(&FreeEntryQ);
    initQueue(&FreeNameQ);

    for (i = 0; i < TABLESIZE; i++)
      {
        initQueue(&(LocalToGlobal.HashHead[i]));
        initQueue(&(GlobalToLocal.HashHead[i]));
        initQueue(&( IPCNameTable.HashHead[i]));
      }
      
    for (i = 0; i < NUMDATASIZES; i++)
      {
        initQueue(&(FreeData.DataHead[i]));
      }     


    /* Be sure we have data in all sizes <= 1024 */
    Allocate(data,32);
    Deallocate(data,32);
    Allocate(data,64);
    Deallocate(data,64);
    Allocate(data,256);
    Deallocate(data,256);
    Allocate(data,512);
    Deallocate(data,512);
    Allocate(data,1024);
    Deallocate(data,1024);
  }

/*
 *  Procedure:
 *      IPCSuicide()
 *
 *  Purpose:
 *      Called from process kill code in sys1.c whenever a process
 *    dies.  This routine cleans up all IPC data structures owned 
 *    by the dying process and sends emergency messages to all processes
 *    which references ports which are thereby deleted.
 *
 */

IPCSuicide()
  {
    integer      i;
    struct KMsg *kmsg;
    boolean      haveports;

    if (!MePtr->Active) return;

#if IPCTRACE
printf("IPCSuicide: proc = %d\n",Me);
#endif

    /* If we have left possibly left a timer on, make sure
     * it is turned off. (Note: if timer has gone off on its
     * own then this is a harmless noop.)
     */

    if (MePtr->TimerSet) untimeout(IPCTimer,(caddr_t)MePtr);

    /* Receive all pending messages and throw them away. */

    if ((kmsg = MePtr->PreviewMsg) != NULL)
      {
        /* Previewed msg, if any, has already been received. */
        KMsgFree(kmsg);
      }

    /* This loop will normally be executed twice.  Overall,
     * this represents an unneeded expense and though it is
     * consistent with the lazy way such things are normally
     * done in UNIX, I don't like it and will eventually
     * fix it.
     */
    MePtr->Active = false;
    haveports = true;
    while (haveports) 
      {
        haveports = false;
        for (i = 1; i < MAXPORTS; i++)
          {
            if (TestBit(&(MePtr->PortsInUse),i))
              {
                haveports = true;
                RemPort(Me,(localport)i);
              }
          }
      }
  }

/*
 *******************************************************************
 *******************************************************************
 *                    IPC 'system calls':                          *
 *                      IPCAllocatePort                            *
 *                      IPCDeallocatePort                          *
 *                      IPCPortsWithMessagesWaiting                *
 *                      IPCLocate                                  *
 *                      IPCAssertName                              *
 *                      IPCIncrementUseCount                       *
 *                      IPCDecrementUseCount                       *
 *                      IPCSetBacklog                              *
 *                      IPCChildPorts                              *
 *                      IPCDefineSignal                            *
 *                      IPCClearSignal                             *
 *                      IPCDismissSignal                           *
 *                      IPCGetSignal                               *
 *******************************************************************
 *******************************************************************
 */

integer IPCAllocatePort(vec)
integer *vec;
  {
    localport             lp;
    unsigned char         backlog;
    struct args { integer backlog; } * args = (struct args *)vec;

#if IPCTRACE
printf("IPCAllocate: backlog = %d\n",args->backlog);
#endif

    /* Insure legality of backlog and enforce limits. */
    backlog = args->backlog;
    if (backlog > MAXBACKLOG) backlog = MAXBACKLOG;
    if (backlog == 0) 
      {
        backlog = DEFAULTBACKLOG;
      }

#if IPCTRACE
printf(" Me = %d\n",Me);
#endif
    /* Find a local port number not in use. */
    lp = FindPort(Me);

    if (lp == 0)
      {
        u.u_error = TOOMANYPORTS;
        return(NULL);
      }

    return(NewPort(Me,lp,backlog));
  }

boolean IPCDeallocatePort(vec)
integer *vec;
  {
    localport lp;
    struct args { integer lp; } * args = (struct args *)vec;

#if IPCTRACE
printf("IPCDeallocate: local port = %d\n",args->lp);
#endif

    /* Get needed information from environment and arguments. */

    lp = args->lp;
    if (!LegalPort(lp))    
      {
        /* Port not in use.  Return error to user. */
        u.u_error = NOTAPORT;
        return(false);
      }

    /* Get hash entry and check use count. */
    if (HashLookup(&LocalToGlobal,PortHash,(int(*)())PortEqual,Me,(int)lp)->UseCount)
      {
        /* Port has a non-zero use count. Don't release it. */
        return(false);
      }

    RemPort(Me,lp);
    return(true);
  }

boolean IPCPortsWithMessagesWaiting(vec)
integer *vec;
  {
    localport         lp;
    struct Queue     *next;
    struct KMsg      *kmsg;
    struct SetOfPorts ports;
    integer           i;
    struct args { caddr_t p; } * args = (struct args *)vec;
 
#if IPCTRACE
printf("IPCPortsWithMessagesWaiting\n");
#endif
    /* Initialize set of ports data structure. */
    for (i = 0; i < ((MAXPORTS+31)/32); i++)
      {
        ports.PortBits[i] = 0;
      }

    next = MePtr->MsgsWaiting.F;
    
    while (next != &(MePtr->MsgsWaiting))
      {
        kmsg = (struct KMsg *)next;
        LTPort(lp,Me,kmsg->KRemotePort);
        SetBit((char *)&ports,lp);
        next = next->F;
      }

    if (u.u_error=copyout((caddr_t)&ports,args->p,sizeof(struct SetOfPorts)))
      {
        return(false);
      }

    return(true);
  }

IPCLocate(vec)
integer *vec;
  {
    struct IPCName *name;
    struct Port * port;
    struct args { caddr_t n;} * args = (struct args *)vec;

#if IPCTRACE
printf("IPCLocate\n");
#endif

    name = GetIPCName(args->n);

#if IPCNAMETRACE
printf("  name: %s\n",name->String);
#endif

    port = FindName(name,(struct IPCName **)NULL);
    FreeIPCName(name);
    if (port == NULL) return(NULL);
    return(GiveSendRights(port,Me));
  }

boolean IPCAssertName(vec)
integer *vec;
  {
    struct Port    *port;
    struct IPCName *name;
    struct IPCName *oldname;
    struct args { caddr_t n; integer lp;} * args = (struct args *)vec;

#if IPCTRACE
printf("IPCAssertName: lp = %d\n",args->lp);
#endif

    if (!LegalPort((localport)args->lp))
      {
        u.u_error = NOTAPORT;
        return(false);
      }

    name = GetIPCName(args->n);

#if IPCNAMETRACE
printf("  name: %s\n",name->String);
#endif

    port = FindName(name,&oldname);
    if (port != NULL)
      {
        u.u_error = BADIPCNAME;
        FreeIPCName(name);
        return(false);
      }

    GTPort(port,Me,args->lp);
    if (port == NULL)
      {
        u.u_error = NOTAPORT;
        FreeIPCName(name);
        return(false);
      }

    Associate(name,port);
    return(true);
  }

boolean IPCRemoveName(vec)
integer *vec;
  {
    struct IPCName *name;
    struct IPCName *oldname;
    struct Port    *port;
    struct args { caddr_t n; } * args = (struct args *)vec;

#if IPCTRACE
printf("IPCRemoveName\n");
#endif

    name = GetIPCName(args->n);

#if IPCNAMETRACE
printf("  name: %s\n",name->String);
#endif

    port = FindName(name,&oldname);
    FreeIPCName(name);

    if (port== NULL)
      {
        u.u_error = BADIPCNAME;
        return(false);
      }

    if ((port->Receiver != Me) && (port->Owner != Me))
      {
        u.u_error = BADIPCNAME;
        return(false);
      }

    Disassociate(oldname,port);
    FreeIPCName(oldname);
    return(true);
  }

boolean IPCIncrementUseCount(vec)
integer *vec;
  {
    localport         lp;
    struct PortEntry *pe;
    struct args { integer lp; } * args = (struct args *)vec;

#if IPCTRACE
printf("IPCIncrementUseCount: local port = %d\n",args->lp);
#endif

    /* Get needed information from environment and arguments. */

    lp = args->lp;
    if (!LegalPort(lp))
      {
        u.u_error = NOTAPORT;
        return(false);
      }

    pe = HashLookup(&LocalToGlobal,PortHash,(int(*)())PortEqual,Me,(int)lp);

    /* Check for validity of request. */
    if (pe == NULL) 
      {
        u.u_error = NOTAPORT;
        return(false);
      }

    /* Increment use count if possible. */

    if (pe->UseCount < MAXUSECOUNT)
      {
        pe->UseCount++;
      }

    return(true);
  }

boolean IPCDecrementUseCount(vec)
integer * vec;
  {
    localport         lp;
    struct PortEntry *pe;
    struct args { integer lp; } * args = (struct args *)vec;

#if IPCTRACE
printf("IPCDecrementUseCount: local port = %d\n",args->lp);
#endif

    /* Get needed information from environment and arguments. */
    lp    = args->lp;
    if (!LegalPort(lp))
      {
        u.u_error = NOTAPORT;
        return(false);
      }

    pe = HashLookup(&LocalToGlobal,PortHash,(int(*)())PortEqual,Me,(int)lp);

    /* Check for validity of request. */
    if (pe == NULL) 
      {
        u.u_error = NOTAPORT;
        return(false);
      }

    /* Decrement use count if possible. */

    if (pe->UseCount > 0)
      {
        pe->UseCount--;
      }

    return(true);
  }

boolean IPCSetBacklog(vec)
integer * vec;
  {
    localport           lp;
    integer             newbacklog;
    struct PortEntry   *pe;
    struct Port        *port;
    struct args { integer lp; integer newbacklog; } * args = (struct args *)vec;
    

#if IPCTRACE
printf("IPCSetBacklog: local port = %d, backlog = %d\n",args->lp,args->newbacklog);
#endif

    /* Get needed information from environment and arguments. */

    lp = args->lp;
    if (!LegalPort(lp))
      {
        u.u_error = NOTAPORT;
        return(false);
      }

    newbacklog = args->newbacklog;

    if (newbacklog > MAXBACKLOG) newbacklog = MAXBACKLOG;
    if ((newbacklog < 0) || (newbacklog == 0)) 
      {
        newbacklog = DEFAULTBACKLOG;
      }

    pe = HashLookup(&LocalToGlobal,PortHash,(int(*)())PortEqual,Me,(int)lp);

    /* Check for validity of request. */
    if (pe == NULL) 
      {
        u.u_error = NOTAPORT;
        return(false);
      }

    port = (struct Port *)pe->LTGValue;

    if (port->Receiver == Me)
      {
        /* Only receiver can set backlog. */

        port->Backlog = newbacklog;
        return(true);
      }
    else
      {
        u.u_error = NOTPORTRECEIVER;
        return(false);
      }
  }

IPCChildPorts(vec)
integer * vec;
  {
    struct proc *p;
    int         *kernelport;
    int         *dataport;
    int          pid;
    process      child;
    struct Proc *childptr;
    localport    klp,dlp;
    struct Port *port;
    struct args { integer pid; integer *kernelport; integer *dataport } *args = (struct args *)vec;

#if IPCTRACE
printf("IPCChildPorts\n");
#endif
    kernelport = args->kernelport;
    dataport = args->dataport;
    pid = args->pid;

    /* Loop through all processes to find child. */
    for (p = proc; p < procNPROC; p++)
      {
        /* Is this process active? */
        if (p->p_stat == NULL) continue;

        /* Is this the process we want? */
        if (p->p_pid != pid) continue;

        /* Is the father of this process the current process? */
        if (p->p_ppid != (u.u_procp)->p_pid) break;
        
        /* Get index of child and pointer to IPC process table entry. */
        child = p->p_ndx;
        childptr = &ProcTable[child];

        /* Is child active (in the IPC sense)? */
        if (!childptr->Active)
          {
            /* No, introduce child to IPC world. */
            childptr->Active = true;
            childptr->TimerSet = false;
            childptr->Signal[NORMALMSG].Want = false;
            childptr->Signal[EMERGENCYMSG].Want = false;
            initQueue(&(childptr->MsgsWaiting));
            (void)NewPort(child,KERNELPORT,DEFAULTBACKLOG);
            (void)NewPort(child,DATAPORT,DEFAULTBACKLOG);
          }

        GTPort(port,child,KERNELPORT);
        klp = GiveSendRights(port,Me);

        GTPort(port,child,DATAPORT);
        dlp = GiveSendRights(port,Me);

	if (u.u_error == 0)
	    u.u_error=copyout((caddr_t)&klp,(caddr_t)kernelport,sizeof(localport));
	if (u.u_error == 0)
	    u.u_error=copyout((caddr_t)&dlp,(caddr_t)dataport,sizeof(localport));

        return(0);
      }

    u.u_error = NOTYOURCHILD;
    return(0);
  }

boolean IPCDefineSignal(args)
integer * args;
  {
    integer msgtype;

#if IPCTRACE
printf("IPCDefineSignal: signal = %d, msgtype = %d\n",args->signal,args->msgtype);
#endif

    msgtype = args[0];
    if ((unsigned)(msgtype) >= NUMMSGTYPES || (unsigned)(args[1]) > NSIG)
    {
	u.u_error = EINVAL;
	return(false);
    }
    MePtr->Signal[msgtype].HowMany = 0;
    MePtr->Signal[msgtype].Set     = false;
    MePtr->Signal[msgtype].Want    = true;
    MePtr->Signal[msgtype].Which   = args[1];
    return(true);
  }

integer IPCDismissSignal(args)
integer *args;
  {
    integer msgtype;

#if IPCTRACE
printf("DismissSignal\n");
#endif

    msgtype = *args;
    if ((unsigned)(msgtype) >= NUMMSGTYPES)
    {
	u.u_error = EINVAL;
	return(0);
    }
    MePtr->Signal[msgtype].HowMany--;
    if (MePtr->Signal[msgtype].HowMany == 0)
      {
        MePtr->Signal[msgtype].HowMany = 0;
        MePtr->Signal[msgtype].Set = false;
      }
    return(MePtr->Signal[msgtype].HowMany);
  }

boolean IPCClearSignal(args)
integer * args;
  {

#if IPCTRACE
printf("IPCClearSignal\n");
#endif

    if ((unsigned)(*args) >= NUMMSGTYPES)
    {
	u.u_error = EINVAL;
	return(false);
    }
    MePtr->Signal[*args].HowMany = 0;
    MePtr->Signal[*args].Set     = false;
    return(true);
  }

integer IPCGetSignal(args)
integer * args;
  {

#if IPCTRACE
printf("GetSignal\n");
#endif

    if ((unsigned)(*args) >= NUMMSGTYPES)
    {
	u.u_error = EINVAL;
	return(0);
    }
    return(MePtr->Signal[*args].HowMany);
  }
#endif	CS_IPC
