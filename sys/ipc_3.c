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
 *  File: ipc3.c (Copyright 1981 (c) Richard F. Rashid)
 *
 *********************************************************************
 * HISTORY
 * 06-Aug-85  Mike Accetta (mja) at Carnegie-Mellon University
 *	Upgraded from 4.1BSD.
 *	[V1(1)]
 *
 *  7-Feb-85  Glenn Marcy (gm0w) at Carnegie-Mellon University
 *	Changed debug printfs to follow message with newline, rather than
 *	precede the message with one.  Added more debug statements and
 *	changed the base of pointers printed from decimal to hex (V3.07n).
 *
 *  2-Jan-85  Glenn Marcy (gm0w) at Carnegie-Mellon University
 *	Fixed inconsistant usage of booleans that was detect by new
 *	boolean enumerated type.
 *
 * 18-Oct-84  Glenn Marcy (gm0w) at Carnegie-Mellon University
 *	Changed references to KMsgType and MsgType for new MsgType
 *	union.
 *
 * 17-Oct-84  Glenn Marcy (gm0w) at Carnegie-Mellon University
 *	Insert code to catch IPC wait timeout crashes.
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
 *      The UNIX IPC facility: basic utilities.
 *
 *  Author:
 *      Richard F. Rashid 
 */

#include "cs_ipc.h"

#if	CS_IPC > 0
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

#define READ    1
#define WRITE   0

/*
 *  Procedure:
 *      IPCAtrium()
 *
 *  Purpose:
 *      The IPC facility is entered through a single system
 *    call but provides many distinct functions.  IPCAtrium
 *    is the basic IPC system call routine and it checks the
 *    arguments passed to it to determine the actual IPC 
 *    call routine to enter.
 *
 */

IPCAtrium()
  {
    struct a
      {
        integer  cmd;
        integer *argvec;
      } *uap;

    integer vec[MAXIPCARGS];

#if IPCTRACE
printf("IPCAtrium: current process = %d\n",Me);
#endif

    if (!IPCStarted) IPCInit();

    if (!MePtr->Active)
      {
        /* Introduce this process to the IPC world. */
        MePtr->Active = true;
        MePtr->TimerSet = false;
        MePtr->PreviewMsg = NULL;
        MePtr->Signal[NORMALMSG].Want = false;
        MePtr->Signal[EMERGENCYMSG].Want = false;

        initQueue(&(ProcTable[Me].MsgsWaiting));
        /* Allocate the default data port. */
        (void)NewPort(Me,DATAPORT,DEFAULTBACKLOG);

        /* Allocate the default kernel port. */
        (void)NewPort(Me,KERNELPORT,DEFAULTBACKLOG);
      }

    /* We always start out in the NOTWAITING state. */
    MePtr->State = NOTWAITING;

    uap = (struct a *)u.u_ap;
    if (u.u_error=copyin((caddr_t)uap->argvec,(caddr_t)vec,sizeof vec))
      {
        return;
      }
      
    switch (uap->cmd)
      {
        case ALLOCATEPORT:
          {
            u.u_r.r_val1 = IPCAllocatePort(vec);
          }
        endcase

        case DEALLOCATEPORT:
          {
            u.u_r.r_val1 = (int)IPCDeallocatePort(vec);
          }
        endcase

        case PORTSWITHMESSAGESWAITING:
          {
            u.u_r.r_val1 = (int)IPCPortsWithMessagesWaiting(vec);
          }
        endcase

        case ASSERTNAME:
          {
            u.u_r.r_val1 = (int)IPCAssertName(vec);
          }
        endcase

        case REMOVENAME:
          {
            u.u_r.r_val1 = (int)IPCRemoveName(vec);
          }
        endcase

        case LOCATE:
          {
            u.u_r.r_val1 = IPCLocate(vec);
          }
        endcase

        case INCREMENTUSECOUNT:
          {
            u.u_r.r_val1 = (int)IPCIncrementUseCount(vec);
          }
        endcase

        case DECREMENTUSECOUNT:
          {
            u.u_r.r_val1 = (int)IPCDecrementUseCount(vec);
          }
        endcase

        case SETBACKLOG:
          {
            u.u_r.r_val1 = (int)IPCSetBacklog(vec);
          }
        endcase

        case CHILDPORTS:
          {
            u.u_r.r_val1 = IPCChildPorts(vec);
          }
        endcase

        case DEFINESIGNAL:
          {
            u.u_r.r_val1 = (int)IPCDefineSignal(vec);
          }
        endcase

        case CLEARSIGNAL:
          {
            u.u_r.r_val1 = (int)IPCClearSignal(vec);
          }
        endcase

        case GETSIGNAL:
          {
            u.u_r.r_val1 = IPCGetSignal(vec);
          }
        endcase

        case DISMISSSIGNAL:
          {
            u.u_r.r_val1 = IPCDismissSignal(vec);
          }
        endcase

        default:
          {
            u.u_error = NOTANIPCCALL;
          }
        endcase


	/*
	 ***********************************************************
	 ***********************************************************
	 *	      Message Sending Code.			   *
	 ***********************************************************
	 ***********************************************************
	 */

        case SEND:
          {
            struct Msg   msg;
            struct KMsg *kmsg;
	    struct Msg  *m;
	    int          numbytes;
            struct LS    ls;
            struct argstruct
              { 
                caddr_t m; 
                integer maxwait; 
                integer option; 
              } * args;

            args = (struct argstruct *)vec;

#if IPCTRACE
printf("IPCSend: maxwait = %d, option = %d\n",args->maxwait,args->option);
#endif

            if (u.u_error=copyin(args->m,(caddr_t)&msg,sizeof(struct Msg)))
              {
                return;
              }

#if IPCTRACE
printf("Contents of msg:\n");
printf("  MsgType = %d, LPType = %d, LocalPort  = %d\n",msg.MsgType,msg.LPType,msg.LocalPort);
printf("                RPType = %d, RemotePort = %d\n",msg.RPType,msg.RemotePort);
printf("                ID     = %d, Type       = %d\n",msg.ID,msg.Type);
#endif

	    m = (struct Msg *)args->m;

            KMsgAllocate(kmsg);

            kmsg->KMsgType      = msg.MsgType;
            kmsg->KLPType       = msg.LPType;
	    LocalTranslate(kmsg->KLocalPort,msg.LocalPort,msg.LPType);
            kmsg->KRPType       = msg.RPType;
	    LocalTranslate(kmsg->KRemotePort,msg.RemotePort,msg.RPType);
            kmsg->KID           = msg.ID;

            switch (msg.Type)    
              {
                default:
                  {
                    (void)ReadDescriptor(kmsg,(struct Descriptor *)&(m->Type),0,READ);
                  }
                endcase
		
		case TYPEUNSTRUCTURED:
		  {
		    numbytes = (msg.NumElts + 7)>>3;
		    if ((numbytes != 0) && (numbytes <= MAXMSGSIZE))
		      {
			Allocate(kmsg->LinearData,numbytes);
		        if (kmsg->LinearData == NULL) endcase;
			kmsg->DataAlloc = numbytes;
			if (u.u_error=copyin(msg.DataPtr,kmsg->LinearData,(u_int)numbytes))
			  {
			    endcase;
			  }
			kmsg->DataSize = numbytes;
		      }
		    else
		      {
			if (numbytes != 0) u.u_error = MSGTOOBIG;
		      }
		  }
		endcase

                case TYPELINEARSTRUCTURE:
                  {
                    if (msg.NumElts == 1)
                      {
                        if (u.u_error=copyin(msg.DataPtr,(caddr_t)&ls,sizeof(struct LS)))
                          {
                            endcase;
                          }
			
			if ((ls.LSSize + ls.LSDataSize) > MAXMSGSIZE)
			  {
			    u.u_error = MSGTOOBIG;
			    endcase;
			  }

                        if (ls.LSSize != 0)
			  {
			    Allocate(kmsg->LinearStructure,ls.LSSize);
			    if (kmsg->LinearStructure == NULL) endcase;
			    kmsg->StructAlloc = ls.LSSize;
			    if (u.u_error=copyin(ls.LSPtr,kmsg->LinearStructure,(u_int)ls.LSSize))
			      {
                            	endcase;
			      }
			    kmsg->StructSize = ls.LSSize;
			  }
				
                        if (ls.LSDataSize != 0) 
			  {
			    Allocate(kmsg->LinearData,ls.LSDataSize);
			    if (kmsg->LinearData == NULL) endcase;
			    kmsg->DataAlloc = ls.LSDataSize;
			    if (u.u_error=copyin(ls.LSDataPtr,kmsg->LinearData,
				       (u_int)ls.LSDataSize))
			      {
                            	endcase;
			      }
			    kmsg->DataSize = ls.LSDataSize;
			  }
                      }
                    else
                      {
                        (void)ReadDescriptor(kmsg,
				(struct Descriptor *)&(m->Type),0,READ);
                      }
                  }
                endcase
              }

            if (u.u_error)
              {
                KMsgFree(kmsg);
		return;
              }

            TranslateMsgBody(kmsg,TranLP, localport);

            if (u.u_error)
              {
                KMsgFree(kmsg);
                return;
              }

            u.u_r.r_val1 = QueueMsg(kmsg,args->maxwait,args->option);
          }
        endcase

	/*
	 ***********************************************************
	 ***********************************************************
	 *	      Message Reception Code.			   *
	 * 		    RECEIVE				   *
	 *		    PREVIEW				   *
	 *		    MESSAGEWAIT			   	   *
	 ***********************************************************
	 ***********************************************************
	 */

        case RECEIVE:
        case PREVIEW:
        case MESSAGEWAIT:
          {
            caddr_t           p;
            caddr_t           m;
            integer           maxwait;
	    integer           waitstate;
            boolean           preview;
            struct KMsg      *kmsg;
            struct Msg        msg;
            struct argstruct
              { 
                caddr_t p; 
                caddr_t m; 
                integer maxwait; 
              } * args;
        

            args = (struct argstruct *) vec;
	    preview = (uap->cmd == PREVIEW) ? true : false;

            /* First, check to see if interrupt is pending -- 
            /* if so, then act like we have been interrupted.  
            /* (This is done to make sure that emergency messages
            /* get handled by emergency message handlers if that
            /* is wanted).  12/19/80 RFR
             */
  
            if (ISSIG(u.u_procp))
              {
                u.u_error = EINTR;
                return;
              }


            /* Get arguments into local variables. */

	    if (uap->cmd == MESSAGEWAIT)
	      {
		struct mwargstruct
		  {
		    caddr_t p;
		    int     maxwait;
		  } * mwargs;
		mwargs = (struct mwargstruct *)args;
		p = mwargs->p;
		m = NULL;
		maxwait = mwargs->maxwait;
	      }
	    else
	      {
                p = args->p;
                m = args->m;
                maxwait = args->maxwait;

                /* Copy message data structure out of user space. */    

                if (u.u_error=copyin(m,(caddr_t)&msg,sizeof(struct Msg)))
                  {
                    return;
                  }
	      }

#if IPCTRACE
printf("Contents of msg:\n");
printf("  MsgType = %d, LPType = %d, LocalPort  = %d\n",msg.MsgType,msg.LPType,msg.LocalPort);
printf("                RPType = %d, RemotePort = %d\n",msg.RPType,msg.RemotePort);
printf("                ID     = %d, Type       = %d\n",msg.ID,msg.Type);
#endif

 	    if ((integer)p == -1)
	      {
		waitstate = ALLPORTWAIT;		
	      }
	    else
	      {
	        if (((integer)p >= 0) && ((integer)p < MAXPORTS))
	          {
		    waitstate = ONEPORTWAIT;
		    MePtr->WaitPort = (integer)p;
	          }
	        else
	          {
		    waitstate = MULTIPLEPORTWAIT;
	            if (u.u_error=copyin(p,(caddr_t)&(MePtr->WaitPorts),sizeof (struct SetOfPorts)))
	              {
	                return;
	              }
	          }
	      }

	    if ((kmsg = MePtr->PreviewMsg) == NULL) 
	      {
		while (kmsg == NULL)
		  {
		    struct Queue *next;
		    localport     lp;

                    next = MePtr->MsgsWaiting.F;
                    while (next != &(MePtr->MsgsWaiting))
                      {
		    	switch (waitstate) 
		          {
			    case ALLPORTWAIT:
			      {
			        kmsg = (struct KMsg *) next;
			      }
			    endcase

			    case ONEPORTWAIT:
			      {
                    	        LTPort(lp,Me,((struct KMsg *)next)->KRemotePort);
			        if (lp == MePtr->WaitPort) 
			          {
				    kmsg = (struct KMsg *)next;
			          }
			      }
			    endcase

			    case MULTIPLEPORTWAIT:
			      {
                                LTPort(lp,Me,((struct KMsg *)next)->KRemotePort);
                                if (TestBit(&(MePtr->WaitPorts),(int)lp))
                                  {
				    kmsg = (struct KMsg *)next;
                                  }
			      }
			    endcase
		          }
		        if (kmsg != NULL) break;
                        next = next->F;
                      }
		     
		    if (kmsg != NULL) break;

		    IPCWait(maxwait,waitstate);
	            if (MePtr->State == TIMEDOUT) 
		      {
                        u.u_r.r_val1 = FALSE;
			return;
		      }
		  }

		if (m != NULL)
		  {
                    remQueue(&(MePtr->MsgsWaiting),(struct Queue *)kmsg);
		  }
  	      }

            if (m != NULL)
	      {
		GiveMsgToUser(m,&msg,kmsg,preview);
	      }

            u.u_r.r_val1 = TRUE;
          }
        endcase

      }
  }

/*
 *******************************************************************
 *******************************************************************
 *           Routines for queueing and accepting messages.         *
 *                      QueueMsg                                   *
 *                      DQMsg                                      *
 *                      AcceptMsg                                  *
 *                      ThrowMsgAway                               *
 *                      GiveMsgToUser                              *
 *******************************************************************
 *******************************************************************
 */

QueueMsg(kmsg,maxwait,option)
register struct KMsg *kmsg;
integer               maxwait;
integer               option;
  {
    process           prc;
    struct Port      *port;
    struct Proc      *procptr;
    boolean           emergency;
    localport         lp;

#if MSGTRACE
printf("QueueMsg: maxwait = %d, option = %d\n",maxwait,option);
#endif

    port = kmsg->KRemotePort;
    if (port == NULL)
      {
#if MSGTRACE
	printf("QueueMsg: remote port NULL\n");
#endif
        KMsgFree(kmsg);
        return(TRUE);
      }

    procptr = &ProcTable[(prc = port->Receiver)];

    /* See if this is a message for the KERNELPROC. */
    if (port->Receiver == KERNELPROC)
      {
#if MSGTRACE
	printf("QueueMsg: Receiver KERNELPROC\n");
#endif
        HandleKernelMsg(kmsg);
        return(TRUE);
      }

    emergency = (kmsg->KMsgCode == EMERGENCYMSG) ? true : false;

    if ((!emergency) && (port->NumQueued >= port->Backlog))
      {
        if (option == DONTWAIT)
          {
#if MSGTRACE
	    printf("QueueMsg: DONTWAIT\n");
#endif
            KMsgFree(kmsg);
            u.u_error = PORTFULL;
            return(FALSE);
          }

        enQueue(&(port->SecondaryMsgQ),(struct Queue *)kmsg);
        port->NumQueued++;
        kmsg->WakeupProc = Me;

        if (option == REPLY)
          {
#if MSGTRACE
	    printf("QueueMsg: REPLY\n");
#endif
            kmsg->What = SENDACK;
            /* 
            /* Fix: return false on queue full with option == REPLY. 
            /* 12/19/80 RFR
             */
            return(FALSE);
          }
        else
          {
            kmsg->What = WAKEME;
	    IPCWait(maxwait,FULLWAIT);
            if (MePtr->State == TIMEDOUT)
	      {
	        struct Queue *qp;

		for (qp = port->SecondaryMsgQ.F;
		     qp != &(port->SecondaryMsgQ); qp = qp->F)
		  if (qp == (struct Queue *)kmsg) {
		    remQueue(&(port->SecondaryMsgQ),(struct Queue *)kmsg);
		    port->NumQueued--;
#if MSGTRACE
		    printf("QueueMsg: PORTFULL TIMEDOUT\n");
#endif
		    KMsgFree(kmsg);
		    u.u_error = PORTFULL;
		    return(FALSE);
		  }
		printf("ipc3: knfosq\n");
	      }
#if MSGTRACE
	    printf("QueueMsg: PORTWAIT\n");
#endif
            return(TRUE);
          }
      }

    kmsg->What = DONTWAKE;
    if (emergency)
      {
        /* Insert message at head of queue rather than end. */
        /* Also, don't increment NumQueued for emergency messages. */
        enQueue(procptr->MsgsWaiting.F,(struct Queue *)kmsg);
      }
    else
      {
        enQueue(&(procptr->MsgsWaiting),(struct Queue *)kmsg);
        port->NumQueued++;
      }

    if (procptr->Signal[kmsg->KMsgCode].Want)
      {
	MsgSignal(procptr,emergency);
      }

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

#if MSGTRACE
    printf("QueueMsg: success\n");
#endif
    return(TRUE);
  }

struct KMsg *DQMsg(port)
register struct Port *port;
  {
    register struct Queue     *next;
    register struct Proc      *procptr;

#if MSGTRACE
printf("DQMsg: port = 0x%X\n",port);
#endif

    procptr = &ProcTable[port->Receiver];
    
    next = procptr->MsgsWaiting.F;
    
    while (next != &(procptr->MsgsWaiting))
      {
        if (port == ((struct KMsg *)next)->KRemotePort)
          {
            remQueue(&(procptr->MsgsWaiting),next);
            return((struct KMsg *)next);
          }
        next = next->F;
      }
    return(NULL);
  }

AcceptMsg(kmsg)
struct KMsg * kmsg;
  {
    struct KMsg      *skmsg;
    struct Port      *port;
    struct Proc      *procptr;

#if MSGTRACE
printf("AcceptMsg\n");
#endif

    port = kmsg->KRemotePort;
    procptr = &ProcTable[port->Receiver];

    if (kmsg->KMsgCode != EMERGENCYMSG) port->NumQueued--;

    if ((skmsg = (struct KMsg *)deQueue(&(port->SecondaryMsgQ))) != NULL)
      {
        enQueue(&(procptr->MsgsWaiting),(struct Queue *)skmsg);
        switch (skmsg->What)
          {
            case WAKEME:
              {
                IPCWakeup(&ProcTable[skmsg->WakeupProc]);
              }
            endcase

            case SENDACK:
              {
                SendAck(skmsg);
              }
            endcase
          }

        if (MePtr->Signal[skmsg->KMsgCode].Want)
          {
            MsgSignal(MePtr,(skmsg->KMsgCode == EMERGENCYMSG));
          }
      }
  }

ThrowMsgAway(kmsg)
struct KMsg *kmsg;
  {

#if MSGTRACE
printf("ThrowAwayMsg\n");
#endif

    AcceptMsg(kmsg);
    (void)TranGP(Me,kmsg->KRemotePort,kmsg->KRPType);
    (void)TranGP(Me,kmsg->KLocalPort,kmsg->KLPType);
    TranslateMsgBody(kmsg,TranGP, struct Port *);
    KMsgFree(kmsg); 
  }

GiveMsgToUser(m,msg,kmsg,preview)
caddr_t      m;
struct Msg  *msg;
struct KMsg *kmsg;
boolean      preview;
  {

    int       numbytes;
    int       numbits;
    int       tranbytes;
    struct LS ls;


#if MSGTRACE
printf("GiveMsgToUser\n");
#endif

    if (MePtr->PreviewMsg != kmsg)
      {
        AcceptMsg(kmsg);
        TranslateMsgBody(kmsg,TranGP, struct Port *);
      }

    msg->MsgType    = kmsg->KMsgType;
    msg->LPType     = kmsg->KRPType;
    GlobalTranslate(msg->LocalPort,kmsg->KRemotePort,kmsg->KRPType);
    msg->RPType     = kmsg->KLPType;
    GlobalTranslate(msg->RemotePort,kmsg->KLocalPort,kmsg->KLPType);
    msg->ID         = kmsg->KID;

    if (preview)
      {
        if (msg->Type != TYPELINEARSTRUCTURE)
          {
            u.u_error = BADMSGTYPE;
            return;
          }

        if (u.u_error=copyin((caddr_t)msg->DataPtr,(caddr_t)&ls,sizeof(struct LS)))
          {
            return;     
          }

        ls.LSSize     = kmsg->StructSize;
        ls.LSDataSize = kmsg->DataSize;
        u.u_error=copyout((caddr_t)&ls,(caddr_t)msg->DataPtr,sizeof(struct LS));
      }
    else
      {

    switch (msg->Type)
      {
	case TYPEUNSTRUCTURED:
	  {
	    numbytes = (msg->NumElts + 7)>>3;
	    if (numbytes >= kmsg->DataSize)
	      { 
		if (kmsg->DataSize > 0)
	          {
		    if (u.u_error=copyout(kmsg->LinearData,msg->DataPtr,(u_int)kmsg->DataSize))
		      {
		        endcase;
		      }
		  }
                /* OK to use bcopy, we know data is accessible. RFR 7/1/81*/
		numbits = (kmsg->DataSize)<<3;
                bcopy((caddr_t)&numbits,(caddr_t)&(((struct Msg *)m)->NumElts),sizeof (int));
	      }
	    else
	      {
		u.u_error = NOTENOUGHROOM;
	      }
	  }
	endcase

        case TYPELINEARSTRUCTURE:
          {
            if (msg->NumElts != 1)
              {
                (void)ReadDescriptor(kmsg,
			(struct Descriptor *)&(((struct Msg *)m)->Type),0,WRITE);
                if (u.u_error) return;
                if (kmsg->CurrentByte < kmsg->DataSize)
                  {
                    u.u_error = NOTENOUGHROOM;
                  }
                return;
               }

            if (u.u_error=copyin(msg->DataPtr,(caddr_t)&ls,sizeof(struct LS)))
              {
                break;
              }

            numbytes = ls.LSSize;
            if (numbytes < kmsg->StructSize)
              {
                u.u_error = NOTENOUGHROOM;
                tranbytes = numbytes;
              }
            else
              {
                tranbytes = kmsg->StructSize;
              }

            if (tranbytes > 0) 
              {
		if (u.u_error=copyout(kmsg->LinearStructure,ls.LSPtr,(u_int)tranbytes))
		  {
		    break;
		  }
              }

            numbytes = ls.LSDataSize;
            if (numbytes < kmsg->DataSize)
              {
                u.u_error = NOTENOUGHROOM;
                tranbytes = numbytes;
              }
            else
              {
                tranbytes = kmsg->DataSize;
              }

            if (tranbytes > 0)
              {
		if (u.u_error=copyout(kmsg->LinearData,ls.LSDataPtr,(u_int)tranbytes))
		  {
		    break;
		  }
              }

            ls.LSSize     = kmsg->StructSize;
            ls.LSDataSize = kmsg->DataSize;

            /* OK to use bcopy, we know data is accessible. RFR 7/1/81*/
            bcopy((caddr_t)&ls,(caddr_t)msg->DataPtr,sizeof(struct LS));
          }
        endcase

        default: 
          {
            (void)ReadDescriptor(kmsg,
		(struct Descriptor *)&(((struct Msg *)m)->Type),0,WRITE);
            if (u.u_error) endcase;
            if (kmsg->CurrentByte < kmsg->DataSize)
              {
                u.u_error = NOTENOUGHROOM;
              }
          }
        endcase
      }
      }

    u.u_error=copyout((caddr_t)msg,m,
                sizeof(struct Msg)-sizeof(struct Descriptor));
    if ((u.u_error) || (preview))
      {
        MePtr->PreviewMsg = kmsg;
      }
    else
      {
        MePtr->PreviewMsg = NULL;
        KMsgFree(kmsg);
      }
  }


/*
 *******************************************************************
 *******************************************************************
 *       Routines for copying messages in and out of kernel        *
 *                      ReadDescriptor                             *
 *                      ReadData                                   *
 *                      ReadLS                                     *
 *                      AddDescriptor                              *
 *                      AddData                                    *
 *******************************************************************
 *******************************************************************
 */

ReadDescriptor(kmsg,ptr,level,read)
struct KMsg *kmsg;
struct Descriptor *ptr;
integer level;
integer read;
  {
    integer i,nondescwords,extrawords;
    struct Descriptor d;

#if STRUCTRACE
printf("ReadDescriptor:\n");
#endif

    if (level > MAXDESCDEPTH)
      {
        u.u_error = EFAULT;
        return(0);
      }

    if (u.u_error=copyin((caddr_t)ptr,(caddr_t)&d,sizeof(struct Descriptor)))
      {
        return(0);
      }

#if STRUCTRACE
printf("  d.DescName = %d, d.DescKind = %d, d.NumObjects = %d\n",
       d.DescName,d.DescKind,d.NumObjects);
#endif

    if (d.NumObjects < 0)
      {
        u.u_error = EFAULT;
        return(0);
      }

    nondescwords = 0;

    switch (d.DescName)
      {
        default:
          {
            nondescwords = ReadData(kmsg,ptr,read);
          }
        endcase

        case TYPELINEARSTRUCTURE:
          {
            struct LSArray *ls;
            struct LS *newptr;

            if (read != READ)
              {
                u.u_error = BADMSGTYPE;
                return(0);
              }

            ls = (d.DescKind == INLINE) ? 
                        (struct LSArray *)&(ptr->DataPart.LSPtrs[0]) :
                        (struct LSArray *)d.DataPart.LSPtrs[0];

            if (d.DescKind == INLINE) 
              {
                nondescwords = 4*d.NumObjects-1;
              }

            if (d.DescKind == ARRAYOFPTRS) 
              {
                nondescwords = d.NumObjects-1;
              }

            for (i = 0; i < d.NumObjects; i++)
              {
                if (d.DescKind == ARRAYOFPTRS)
                  {
                    if (u.u_error=copyin( (caddr_t)&(ptr->DataPart.LSPtrs[i]),
                                (caddr_t)&newptr,
                                sizeof(struct LS *)))
                      {
                        return(nondescwords);
                      }
                  }
                else
                  {
                    newptr = &(ls->LSEntry[i]);
                  }
                ReadLS(kmsg,newptr);
                if (u.u_error) return(nondescwords);
              }
          }
        endcase

        case TYPEDESCRIPTOR:
          {
            struct DescArray *darray;
            struct Descriptor *newptr;

            darray = (d.DescKind == INLINE) ?
                        (struct DescArray *)&(ptr->DataPart.DescPtrs[0]) :
                        (struct DescArray *)d.DataPart.DescPtrs[0];

            if (d.DescKind == INLINE) nondescwords = 3*d.NumObjects-1;
            if (d.DescKind == ARRAYOFPTRS)
              {
                nondescwords = d.NumObjects-1;
              }

            for (i = 0; i < d.NumObjects; i++)
              {
                if (d.DescKind == ARRAYOFPTRS)
                  {
                    if (u.u_error=copyin( (caddr_t)&(ptr->DataPart.DescPtrs[i]),
                                (caddr_t)&newptr,
                                sizeof(struct Descriptor *)))
                      {
                        return(nondescwords);
                      }
                  }
                else
                  {
                    newptr = &(darray->DescEntry[i]);
                  }
                extrawords = ReadDescriptor(kmsg,newptr,level+1,read);
                if (u.u_error) return(nondescwords);
                if (d.DescKind != ARRAYOFPTRS)
                  {
                    darray = (struct DescArray *) 
                             (((int *)darray) + extrawords);
                    if (d.DescKind == INLINE) 
                      nondescwords += extrawords;
                  }
              }
          }
        endcase
      }
    return(nondescwords);
  }

ReadData(kmsg,ptr,read)
struct KMsg *kmsg;
struct Descriptor *ptr;
integer read;
  {
    integer numbytes;
    integer dataoffset,descoffset;
    integer numblks,numbits;
    integer numeltsperblk,prefixbits,suffixbits,numbitsperelt;
    caddr_t dataptr;
    struct Descriptor d;

#if STRUCTRACE
printf("ReadData:\n");
#endif

    if (u.u_error=copyin((caddr_t)ptr,(caddr_t)&d,sizeof(struct Descriptor)))
      {
        return(0);
      }

#if STRUCTRACE
printf("  d.DescName = %d, d.DescKind = %d, d.NumObjects = %d\n",
       d.DescName,d.DescKind,d.NumObjects);
#endif

    switch (d.DescName)
      {
        case TYPEUNSTRUCTURED:
          {
            numeltsperblk = 1;
            prefixbits = 0;
            suffixbits = 0;
            numbitsperelt = 1;
          }
        endcase

        case TYPEINTEGER: case TYPEPTOWNERSHIP: case TYPEPT:
        case TYPEPTRECEIVERIGHTS: case TYPEPTALLRIGHTS:
          {
            numeltsperblk = 1;
            prefixbits = 0;
            suffixbits = 0;
            numbitsperelt = 32;
          }
        endcase

        case TYPEBYTE:
        case TYPECHAR:
          {
            numeltsperblk = 1;
            prefixbits = 0;
            suffixbits = 0;
            numbitsperelt = 8;
          }
        endcase

        default:
          {
            numeltsperblk = d.NumEltsPerBlk;
            prefixbits = d.PrefixBits;
            suffixbits = d.SuffixBits;
            numbitsperelt = d.NumBitsPerElt+1;
            if (numeltsperblk == 0) numeltsperblk = 1;
          }
        endcase
      }

    /* Calculate number of bytes which the user has given us to
     * write the data of the message into, or which he wants us
     * to transmit (depending on read flag).
     */

    numblks = (d.NumObjects + numeltsperblk - 1)/(numeltsperblk);
    numbits = (prefixbits + suffixbits)*numblks;
    numbits = numbits + numbitsperelt*(d.NumObjects);
    numbytes = (numbits + 7)>>3;

    dataptr = (d.DescKind==INLINE)? (caddr_t)&(ptr->DataPart.DescData):
                                    (caddr_t)d.DataPart.DescData;
    
    descoffset = (d.DescKind==INLINE)? ((numbytes+3)>>2)-1 : 0;

    if (read == READ)
      {
        AddDescriptor(kmsg,(caddr_t)&d,(sizeof (int)) *2,0,true);
        dataoffset = ((kmsg->DataSize)+(sizeof(int))-1)/(sizeof (int));
        AddDescriptor(kmsg,(caddr_t)&dataoffset,sizeof(int),0,true);
        AddData(kmsg,dataptr,numbytes);
      }
    else
      {
        integer bytesleft;
        caddr_t freedata;

        bytesleft = kmsg->DataSize - kmsg->CurrentByte;

        if (bytesleft <= 0) return(descoffset);

        if (numbytes > bytesleft)
          {
            /* There is less information left in the message than space
             * provided by the receiver of the message. 
             * Calculate number of objects which remain according to
             * type specification made by receiver in his descriptor 
             */

            integer bitsleft,bitsperblk,elts;
            bitsleft = bytesleft<<3;
            bitsperblk = numeltsperblk*numbitsperelt+prefixbits+suffixbits;
            numblks = bitsleft/bitsperblk;
            bitsleft = bitsleft%bitsperblk;
            bitsleft = max(0,bitsleft-prefixbits);
            elts = numblks*numeltsperblk+(bitsleft/numbitsperelt);
            u.u_error=copyout((caddr_t)&elts,(caddr_t)&(ptr->NumObjects),sizeof(int));
          }

        freedata   = kmsg->LinearData + kmsg->CurrentByte;
        
        if (copyout(freedata,dataptr,(u_int)min(numbytes,bytesleft)))
          {
	    if (min(numbytes,bytesleft) > 0) u.u_error = EFAULT;
	    return(descoffset);
          }
        
        kmsg->CurrentByte += numbytes;

      }
    return(descoffset);
  }

ReadLS(kmsg,ptr)
struct KMsg *kmsg;
struct LS *ptr;
  {
    struct LS ls;
    integer dataoffset;

#if STRUCTRACE
printf("ReadLS:\n");
#endif

    if (u.u_error=copyin((caddr_t)ptr,(caddr_t)&ls,sizeof(struct LS)))
      {
        return;
      }

    dataoffset = ((kmsg->DataSize)+(sizeof (int))-1)/(sizeof (int));
    AddDescriptor(kmsg,(caddr_t)ls.LSPtr,ls.LSSize,dataoffset,false);
    if (u.u_error) return;
    AddData(kmsg,(caddr_t)ls.LSDataPtr,ls.LSDataSize);
  }

AddDescriptor(kmsg,ptr,numbytes,dataoffset,fromkernel)
struct KMsg *kmsg;
caddr_t ptr;
integer numbytes;
integer dataoffset;
boolean fromkernel;
  {
    caddr_t d;
    integer newsize;

#if STRUCTRACE
printf("AddDescriptor: %d, %d, %d, %d\n",kmsg,ptr,numbytes,dataoffset); 
#endif

    if ((kmsg->StructSize + kmsg->DataSize + numbytes) > MAXMSGSIZE)
      {
        u.u_error = MSGTOOBIG;
        return;
      }

    newsize = kmsg->StructSize + numbytes;
    if (kmsg->LinearStructure == NULL)
      {
	Allocate(kmsg->LinearStructure,1024);
	if (kmsg->LinearStructure == NULL) return;
	kmsg->StructAlloc = 1024;
      }

    d = kmsg->LinearStructure + kmsg->StructSize;

    if (fromkernel)
      {
	bcopy(ptr,d,(u_int)numbytes);
      }
    else
      {
	if (u.u_error=copyin(ptr,d,(u_int)numbytes))
	  {
	    return;
	  }
      }

    if (dataoffset != 0)
      {
        integer i;
	struct intarray 
	  {
	    int Data[MAXMSGSIZE/(sizeof (int))];
	  };

        for (i = ((kmsg->StructSize+3)>>2)*3; 
	     i < (((kmsg->StructSize + numbytes + 3)>>2)*3); 
	     i += 3)
          {
            ((struct intarray *)d)->Data[i] += dataoffset;
          }
      }

    /* Update size of structure information.  Round up to nearest integer. */
    kmsg->StructSize = ((newsize + sizeof(int)-1)/(sizeof(int)))*(sizeof(int));
  }

AddData(kmsg,ptr,numbytes)
struct KMsg *kmsg;
caddr_t ptr;
integer numbytes;
  {
    caddr_t d;
    integer newsize;

#if STRUCTRACE
printf("AddData: %d, %d, %d\n",kmsg,ptr,numbytes); 
#endif
    
    if (numbytes == 0) return;

    if ((kmsg->StructSize + kmsg->DataSize + numbytes) > MAXMSGSIZE)
      {
        /* We have exceeded maximum message size. */
        u.u_error = MSGTOOBIG;
        return;
      }

    newsize = 0;
    if (kmsg->DataSize != 0)
      {
        /* Round up to nearest integer */
        newsize = (((kmsg->DataSize)+sizeof(int)-1)/(sizeof(int)))*(sizeof(int));
      }

    if (kmsg->LinearData == NULL)
      {
	Allocate(kmsg->LinearData,MAXMSGSIZE);
	if (kmsg->LinearData == NULL) return;
	kmsg->DataAlloc = MAXMSGSIZE;
      }

    d = kmsg->LinearData + newsize;

    if (u.u_error=copyin(ptr,d,(u_int)numbytes))
      {
	return;
      }

    /* Update size of message */
    kmsg->DataSize = newsize + numbytes;
  }

/*
 *******************************************************************
 *******************************************************************
 *     Routines for interpreting structured data in messages.      *
 *                      TranGP                                     *
 *                      TranLP                                     *
 *******************************************************************
 *******************************************************************
 */

TranGP(prc,port,pt)
process   prc;
struct Port *port;
porttype  pt;
  {
    localport         lp;

#if PORTTRACE
printf("TranGP: prc = %d, port = 0x%X, pt = %d\n",prc,port,pt);
#endif

    /* This routine translates a global port number to a local 
     * port number for message reception.  If if transfer 
     * of ownership or receive rights occurs, rights are passed 
     * from the kernel to the receiving process.
     */

    if (port == NULL) return (NULL);
    lp = GiveSendRights(port,prc);

    switch (pt)
      {
        case TYPEPTOWNERSHIP:
          {
            if (port->Receiver == NOONE)
              {
                if ((&ProcTable[prc])->Active)
                  {
                    GiveReceiveRights(port,INTRANSIT);
                    SendKernelMsg(prc,RECEIVERIGHTS,TYPEPTRECEIVERIGHTS,(int)port);
                  }
                else
                  {
                    GiveReceiveRights(port,prc);
                  }
              }
            GiveOwnership(port,prc);
          }
        endcase

        case TYPEPTRECEIVERIGHTS:
          {
            if (port->Owner == NOONE)
              {
                if ((&ProcTable[prc])->Active)
                  {
                    GiveOwnership(port,INTRANSIT);
                    SendKernelMsg(prc,OWNERSHIPRIGHTS,TYPEPTOWNERSHIP,(int)port);
                  }
                else
                  {
                    GiveOwnership(port,prc);
                  }
              }
            GiveReceiveRights(port,prc);
          }
        endcase

        case TYPEPTALLRIGHTS:
          {
            GiveOwnership(port,prc);
            GiveReceiveRights(port,prc);
          }
        endcase
      }
    return(lp);
  }

struct Port *TranLP(prc,lp,pt)
process   prc;
localport lp;
porttype  pt;
  {
    struct Port      *port;
    struct PortEntry *pe;

#if PORTTRACE
printf("TranLP: prc = %d, lp = %d, pt = %d\n",prc,lp,pt);
#endif

    /* This routine translates a local port number to a global
     * port number for message transmission.  If transfer 
     * of ownership or receive rights occurs, rights are passed 
     * to the kernel for that period of time that the message 
     * is in transit.
     */

    if (lp == NULL) return (NULL);

    PortLookup(pe,&LocalToGlobal,prc,lp);
    if (pe == NULL)
      { 
        u.u_error = NOTAPORT;
        return(NULL);
      }

    port = (struct Port *)(pe->LTGValue);

    switch (pt)
      {
        case TYPEPTOWNERSHIP:
          {
            if (port->Owner != prc)
              {
                u.u_error = BADRIGHTS;
                return(NULL);
              }

            GiveOwnership(port,INTRANSIT);
          }
        endcase

        case TYPEPTRECEIVERIGHTS:
          {
            if (port->Receiver != prc)
              {
                u.u_error = BADRIGHTS;
                return(NULL);
              }

            GiveReceiveRights(port,INTRANSIT);
          }
        endcase

        case TYPEPTALLRIGHTS:
          {
            if ((port->Owner != prc) || (port->Receiver != prc))
              {
                u.u_error = BADRIGHTS;
                return(NULL);
              }

            GiveOwnership(port,INTRANSIT);
            GiveReceiveRights(port,INTRANSIT);
          }
        endcase
      }

    return(port);
  }
#endif	CS_IPC
