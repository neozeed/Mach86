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
 *  File: ipcmacros.h (Copyright 1981 (c) Richard F. Rashid)
 *
 *********************************************************************
 * HISTORY
 *********************************************************************
 *
 *  Purpose: 
 *      Macros which replace calls which were once subroutines.  This allows
 *      in line expansion of crucial calls and avoids excessive VAX procedure
 *      call overhead.
 *
 *  Author:
 *      Richard F. Rashid 
 */

/*
 *******************************************************************
 *******************************************************************
 *   Routines to calculate minimum and maximum of two numbers.     *
 *******************************************************************
 *******************************************************************
 */

#define min(x,y)                                                            \
  ((x) < (y) ? (x) : (y))

#define max(x,y)                                                            \
  ((x) > (y) ? (x) : (y))

/*
 *******************************************************************
 *******************************************************************
 *        Routines to allocate and free raw data blocks.           *
 *******************************************************************
 *******************************************************************
 */

#ifndef	lint
#define SelectAllocSize(bytes)						    \
  (((bytes) > 2048) ?    4096 :                                             \
  (((bytes) > 1024) ?    2048 :                                             \
  (((bytes) > 512)  ?    1024 :                                             \
  (((bytes) > 256)  ?     512 :     					    \
  (((bytes) > 64)   ?     256 :                                             \
  (((bytes) > 32)   ?      64 : 32))))))                                    

#define SelectAllocQ(bytes)						    \
  (((bytes) > 2048) ? 0 :                                                   \
  (((bytes) > 1024) ? 1 :                                                   \
  (((bytes) > 512)  ? 2 :                                                   \
  (((bytes) > 256)  ? 3 :     						    \
  (((bytes) > 64)   ? 4 :                                                   \
  (((bytes) > 32)   ? 5 : 6))))))                    
#else	lint
/*
 *  Avoid lint "constant in conditional context" errors.
 */
#define SelectAllocSize(bytes)	bytes
#define SelectAllocQ(bytes)	bytes
#endif	lint

#define Allocate(ptr,bytes)                                                 \
  {                                                                         \
    if (((ptr) = (caddr_t)deQueue(&(FreeData.DataHead[SelectAllocQ(bytes)])))\
        == NULL)     							    \
      {                                                                     \
	(ptr) = GetMemory(bytes);                                           \
      }                                                                     \
  };                                                                        
    
#ifndef	lint
#define Deallocate(data,size)                                               \
  {                                                                         \
    if ((size) > 1024)                                                      \
      {                                                                     \
	FreeMemory(data,SelectAllocSize(size));                             \
      }                                                                     \
    else                                                                    \
      {                                                                     \
        enQueue(&(FreeData.DataHead[SelectAllocQ(size)]),(struct Queue *)(data));\
      }                                                                     \
  };
#else	lint
#define Deallocate(data,size) 						    \
	FreeMemory(data,SelectAllocSize(size))
#endif	lint
    
/*
 *******************************************************************
 *******************************************************************
 *             Routines to allocate and free kmsgs.                *
 *******************************************************************
 *******************************************************************
 */

#define KMsgAllocate(newkmsg)                                               \
  {                                                                         \
    if (((struct Queue *)(newkmsg) = deQueue(&FreeKMsgQ)) == NULL)          \
      {                                                                     \
        (newkmsg) = GetKMsg();						    \
      }                                                                     \
    (newkmsg)->StructSize = 0;                                              \
    (newkmsg)->DataSize   = 0;                                              \
    (newkmsg)->CurrentByte= 0;                                              \
    (newkmsg)->StructAlloc= 0;                                              \
    (newkmsg)->DataAlloc  = 0;                                              \
    (newkmsg)->LinearStructure = NULL;				    	    \
    (newkmsg)->LinearData      = NULL;				            \
   };                                                                       

#define KMsgFree(oldkmsg)                                                   \
  {                                                                         \
    int KMsgFree_wasempty;						    \
    if ((oldkmsg)->LinearStructure != NULL)				    \
      {                                                                     \
        Deallocate((oldkmsg)->LinearStructure,(oldkmsg)->StructAlloc);      \
      }                                                                     \
    if ((oldkmsg)->LinearData != NULL)				            \
      {                                                                     \
        Deallocate((oldkmsg)->LinearData,(oldkmsg)->DataAlloc);             \
      }                                                                     \
    KMsgFree_wasempty = Queueempty(&FreeKMsgQ);				    \
    enQueue(&FreeKMsgQ,(struct Queue *)(oldkmsg));                          \
    if (KMsgFree_wasempty) wakeup((caddr_t)&FreeKMsgQ);			    \
  };                                                                        

/*
 *******************************************************************
 *******************************************************************
 *           Routines for looking up ports.                        *
 *******************************************************************
 *******************************************************************
 */

#define PortLookup(retval,table,key1,key2)                                  \
  {                                                                         \
    int            q_key1;                                                  \
    int            q_key2;                                                  \
    struct Queue * q_head;                                                  \
    struct Queue * q_next;                                                  \
    retval = NULL;                                                          \
    q_key1 = (int)(key1);                                                   \
    q_key2 = (int)(key2);                                                   \
    q_head = &((table)->HashHead[((q_key1 ^ q_key2)&(TABLESIZE-1))]);       \
    q_next = q_head->F;                                                     \
    while ((q_next != q_head) && ((retval) == NULL))                        \
      {                                                                     \
        if ((((struct PortEntry *)q_next)->Key1 == q_key1) &&               \
            (((struct PortEntry *)q_next)->Key2 == q_key2))                 \
          {                                                                 \
            (retval) = (struct PortEntry *)q_next;                          \
          }                                                                 \
         else q_next = q_next->F;                                           \
      }                                                                     \
  };   
  
#define GTPort(retval,proc,lp)						    \
  {                                                                         \
    struct PortEntry *GTPort_pe;                                            \
    process           GTPort_proc;					    \
    localport         GTPort_lp;				            \
									    \
    GTPort_proc = (proc);						    \
    GTPort_lp   = (lp);                                                     \
    PortLookup(GTPort_pe,&LocalToGlobal,GTPort_proc,GTPort_lp);             \
    if (GTPort_pe == NULL)                                                  \
      {                                                                     \
	(retval) = NULL;                                                    \
      }                                                                     \
    else                                                                    \
      {                                                                     \
        (retval) = (struct Port *)(GTPort_pe->LTGValue);                    \
      }                                                                     \
  };

#define LTPort(retval,proc,port)                                            \
  {                                                                         \
    struct PortEntry *LTPort_pe;                                            \
    process           LTPort_proc;					    \
    struct Port      *LTPort_port;					    \
									    \
    LTPort_proc = (proc);						    \
    LTPort_port = (port);                                                   \
    PortLookup(LTPort_pe,&GlobalToLocal,LTPort_proc,LTPort_port);           \
    if (LTPort_pe == NULL)                                                  \
      {                                                                     \
	(retval) = NULL;                                                    \
      }                                                                     \
    else                                                                    \
      {                                                                     \
        (retval) = (LTPort_pe->GTLValue);                                   \
      }                                                                     \
  };
                                                                         
/*
 *******************************************************************
 *******************************************************************
 *         Routines which do wakeup and sleep and signalling.      *
 *******************************************************************
 *******************************************************************
 */

#define IPCWakeup(procptr)                                                  \
  {                                                                         \
    (procptr)->State = NOTWAITING;                                          \
    wakeup((caddr_t)(procptr));                                             \
  };                                                                        

#define IPCWait(maxwait,state)                                              \
  {                                                                         \
    (void)spl6();                                                           \
    while (true)                                                            \
      {                                                                     \
        MePtr->State = (state);                                             \
                                                                            \
        if (MePtr->TimerSet) untimeout(IPCTimer,(caddr_t)MePtr);            \
        MePtr->TimerSet = false;                                            \
                                                                            \
        if (maxwait)                                                        \
          {                                                                 \
            timeout(IPCTimer, (caddr_t)MePtr, (6*(maxwait)+99)/100);        \
            MePtr->TimerSet = true;                                         \
          }                                                                 \
                                                                            \
        sleep((caddr_t)MePtr,PUSER);                                        \
                                                                            \
        if ((maxwait) && (MePtr->State != TIMEDOUT))                        \
          {                                                                 \
            untimeout(IPCTimer,(caddr_t)MePtr);                             \
            MePtr->TimerSet = false;                                        \
          }                                                                 \
                                                                            \
        if (MePtr->State == TIMEDOUT)                                       \
          {                                                                 \
            spl0();                                                         \
	    break;							    \
          }                                                                 \
                                                                            \
        if (MePtr->State == NOTWAITING)                                     \
          {                                                                 \
            spl0();                                                         \
	    break;							    \
          }                                                                 \
      }                                                                     \
  };

/*
 *******************************************************************
 *******************************************************************
 *     Routines for interpreting structured data in messages.      *
 *                      TranslateMsg                               *
 *			LocalTranslate				   *
 *			GlobalTranslate				   *
 *******************************************************************
 *******************************************************************
 */

#define TranslateMsgBody(TMB_kmsg,routine,type)				    \
  {                                                                         \
    struct intarray                                                         \
      {                                                                     \
	int Data[MAXMSGSIZE/(sizeof (int))];                                \
      };                                                                    \
                                                                            \
    integer           TMB_i;                                                \
    struct intarray  *TMB_d;                                                \
    struct intarray  *TMB_l;                                                \
                                                                            \
    TMB_l = (struct intarray *)TMB_kmsg->LinearStructure;                   \
    TMB_d = (struct intarray *)TMB_kmsg->LinearData;                        \
                                                                            \
    for (TMB_i = 0; TMB_i < TMB_kmsg->StructSize>>2; TMB_i += 3)            \
      {                                                                     \
        integer       TMB_dtype;					    \
        integer       TMB_doff;                                             \
        integer       TMB_j;                                                \
                                                                            \
        TMB_dtype = (TMB_l->Data[TMB_i] & 0177); 			    \
        TMB_doff  = TMB_l->Data[TMB_i+2];                                   \
        if ((TMB_dtype >= TYPEPTOWNERSHIP) && (TMB_dtype <= TYPEPT))        \
          {                                                                 \
            for (TMB_j = 0; TMB_j < TMB_l->Data[TMB_i+1]; TMB_j++)          \
              {                                                             \
                TMB_d->Data[TMB_doff] = (int)(routine(Me,                   \
				(type)TMB_d->Data[TMB_doff],		    \
				(porttype)TMB_dtype));		            \
                TMB_doff++;                                                 \
              }                                                             \
          }                                                                 \
      }                                                                     \
  };

#define GlobalTranslate(lp,port,pt)					    \
  {                                                                         \
    struct PortEntry *GT_pe;                                                \
                                                                            \
    if ((port) == NULL)                                                     \
      {                                                                     \
	(lp) = NULL;                                                        \
      }                                                                     \
    else                                                                    \
      {                                                                     \
	if ((pt) == TYPEPT)                                                 \
  	  {                                                                 \
            PortLookup(GT_pe,&GlobalToLocal,Me,(port));                     \
            if (GT_pe == NULL)                                              \
              {                                                             \
		(lp) = GiveSendRights((port),Me);                           \
	      }                                                             \
	    else                                                            \
	      {                                                             \
		(lp) = GT_pe->GTLValue;                                     \
	      }                                                             \
	  }                                                                 \
	else                                                                \
	  {                                                                 \
	    (lp) = TranGP(Me,(port),(pt));                                  \
	  }                                                                 \
      }                                                                     \
  };                                                                         

#define LocalTranslate(port,lp,pt)					    \
  {                                                                         \
    struct PortEntry *LT_pe;                                                \
    if ((lp) == NULL)                                                       \
      {                                                                     \
	(port) = NULL;                                                      \
      }                                                                     \
    else                                                                    \
      {                                                                     \
	if ((pt) == TYPEPT)                                                 \
  	  {                                                                 \
            PortLookup(LT_pe,&LocalToGlobal,Me,(lp));                       \
            if (LT_pe == NULL)                                              \
              {                                                             \
		u.u_error = NOTAPORT;                                       \
		(port) = NULL;                                              \
	      }                                                             \
	    else                                                            \
	      {                                                             \
		(port) = (struct Port *)(LT_pe->LTGValue);                  \
	      }                                                             \
	  }                                                                 \
	else                                                                \
	  {                                                                 \
	    (port) = TranLP(Me,(lp),(pt));                                  \
	  }                                                                 \
      }                                                                     \
  };                                                                         
	

