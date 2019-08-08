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

/*--------------------------------------------------------------------*/
/*
 * Hand edited by David Golub  20-Oct-83
 *
 *      This module was produced by MatchMaker version of 29-Jul-83
 *      from the interface definitions file 'Accent.Defs'.
 *
 *             %%%  EDIT THE DEFINITIONS FILE INSTEAD %%%
 *
 *--------------------------------------------------------------------*/



#include "../sync/mp_queue.h"
#include "../accent/accent.h"
#include "../accent/accentint.h"

#define Fastassign 0


/* function xsetbacklog (inp, rep) */
XSetBackLog (Inp, Rep)
ptr_ Inp, Rep;

{
   struct MyMessage
        {
           msg_	Head;
           type_	Type2;
           port_	Arg2;
           type_	Type3;
           short	Arg3;
        };

   struct RepMessage
        {
           msg_	Head;
           type_	RetCodeType;
           short	RetCode;
        };
   typedef struct MyMessage *ptrMyMsg;
   typedef struct RepMessage *ptrRepMsg;
   ptrMyMsg MyMsgp;
   ptrRepMsg Repmsgp;

   port_ ServPort;
   port_ LocalPort;
   short Backlog;


   MyMsgp = (ptrMyMsg )(Inp);
   Repmsgp = (ptrRepMsg )(Rep);
   Repmsgp->RetCode = SUCCESS;
   {
      register msg_	 *v123 = &Repmsgp->Head;

      v123->SimpleMsg = TRUE;
      v123->MsgSize = sizeof (struct RepMessage);
   }
   {
      register struct MyMessage *v125 = MyMsgp;

      ServPort = (v125->Head.RemotePort);
#if Typecheck
      if (v125->Type2.TypeName != TypePt)
         {
            Repmsgp->RetCode = WRONG_ARGS;
            return;
         }
#endif
      LocalPort = (v125->Arg2);
#if Typecheck
      if (v125->Type3.TypeName != TypeInt16)
         {
            Repmsgp->RetCode = WRONG_ARGS;
            return;
         }
#endif
      Backlog = (v125->Arg3);
   }

   Repmsgp->RetCode = SetBackLog(ServPort, LocalPort, Backlog);
   {
      register struct RepMessage *v126 = Repmsgp;

   }
}



/* function xallocatePort (inp, rep) */
XAllocatePort (Inp, Rep)
ptr_ Inp, Rep;

{
   struct MyMessage
        {
           msg_	Head;
           type_	Type3;
           short	Arg3;
        };

   struct RepMessage
        {
           msg_	Head;
           type_	RetCodeType;
           short	RetCode;
           type_	Type2;
           port_	Arg2;
        };
   typedef struct MyMessage *ptrMyMsg;
   typedef struct RepMessage *ptrRepMsg;
   ptrMyMsg MyMsgp;
   ptrRepMsg Repmsgp;

   port_ ServPort;
   port_ LocalPort;
   short Backlog;


   MyMsgp = (ptrMyMsg )(Inp);
   Repmsgp = (ptrRepMsg )(Rep);
   Repmsgp->RetCode = SUCCESS;
   {
      register msg_	 *v127 = &Repmsgp->Head;

      v127->SimpleMsg = FALSE;
      v127->MsgSize = sizeof (struct RepMessage);
   }
   {
      register struct MyMessage *v129 = MyMsgp;

      ServPort = (v129->Head.RemotePort);
#if Typecheck
      if (v129->Type3.TypeName != TypeInt16)
         {
            Repmsgp->RetCode = WRONG_ARGS;
            return;
         }
#endif
      Backlog = (v129->Arg3);
   }

   Repmsgp->RetCode = AllocatePort(ServPort, &LocalPort, Backlog);
   {
      register struct RepMessage *v130 = Repmsgp;

#if Fastassign
      v130->Type2.Lng = 02000220005;
#else
      v130->Type2.InLine = TRUE;
      v130->Type2.Deallocate = FALSE;
      v130->Type2.LongForm = FALSE;
      v130->Type2.TypeName = TypePtAll;
      v130->Type2.TypeSizeInBits = 32;
      v130->Type2.NumObjects = 1;
#endif
      /* Because of nothing leftnot */
      v130->Arg2 = (LocalPort);
   }
}



/* function xdeallocatePort (inp, rep) */
XDeallocatePort (Inp, Rep)
ptr_ Inp, Rep;

{
   struct MyMessage
        {
           msg_	Head;
           type_	Type2;
           port_	Arg2;
           type_	Type3;
           long Arg3;
        };

   struct RepMessage
        {
           msg_	Head;
           type_	RetCodeType;
           short	RetCode;
        };
   typedef struct MyMessage *ptrMyMsg;
   typedef struct RepMessage *ptrRepMsg;
   ptrMyMsg MyMsgp;
   ptrRepMsg Repmsgp;

   port_ ServPort;
   port_ LocalPort;
   long Reason;


   MyMsgp = (ptrMyMsg )(Inp);
   Repmsgp = (ptrRepMsg )(Rep);
   Repmsgp->RetCode = SUCCESS;
   {
      register msg_	 *v134 = &Repmsgp->Head;

      v134->SimpleMsg = TRUE;
      v134->MsgSize = sizeof (struct RepMessage);
   }
   {
      register struct MyMessage *v136 = MyMsgp;

      ServPort = (v136->Head.RemotePort);
#if Typecheck
      if (v136->Type2.TypeName != TypePt)
         {
            Repmsgp->RetCode = WRONG_ARGS;
            return;
         }
#endif
      LocalPort = (v136->Arg2);
#if Typecheck
      if (v136->Type3.TypeName != TypeInt32)
         {
            Repmsgp->RetCode = WRONG_ARGS;
            return;
         }
#endif
      Reason = (v136->Arg3);
   }

   Repmsgp->RetCode = DeallocatePort(ServPort, LocalPort, Reason);
   {
      register struct RepMessage *v137 = Repmsgp;

   }
}



/* function xindexinterpose (inp, rep) */
XIndexInterpose (Inp, Rep)
ptr_ Inp, Rep;

{
   struct MyMessage
        {
           msg_	Head;
           type_	Type2;
           port_	Arg2;
           type_	Type3;
           long Arg3;
        };

   struct RepMessage
        {
           msg_	Head;
           type_	RetCodeType;
           short	RetCode;
           type_	Type4;
           port_	Arg4;
        };
   typedef struct MyMessage *ptrMyMsg;
   typedef struct RepMessage *ptrRepMsg;
   ptrMyMsg MyMsgp;
   ptrRepMsg Repmsgp;

   port_ ServPort;
   port_ MyPort;
   long Hisindex;
   port_ HisPort;


   MyMsgp = (ptrMyMsg )(Inp);
   Repmsgp = (ptrRepMsg )(Rep);
   Repmsgp->RetCode = SUCCESS;
   {
      register msg_	 *v138 = &Repmsgp->Head;

      v138->SimpleMsg = FALSE;
      v138->MsgSize = sizeof (struct RepMessage);
   }
   {
      register struct MyMessage *v140 = MyMsgp;

      ServPort = (v140->Head.RemotePort);
#if Typecheck
      if (v140->Type2.TypeName != TypePt_RECEIVE)
         {
            Repmsgp->RetCode = WRONG_ARGS;
            return;
         }
#endif
      MyPort = (v140->Arg2);
#if Typecheck
      if (v140->Type3.TypeName != TypeInt32)
         {
            Repmsgp->RetCode = WRONG_ARGS;
            return;
         }
#endif
      Hisindex = (v140->Arg3);
   }

   Repmsgp->RetCode = IndexInterpose(ServPort, MyPort, Hisindex, HisPort);
   {
      register struct RepMessage *v141 = Repmsgp;

#if Fastassign
      v141->Type4.Lng = 02000220004;
#else
      v141->Type4.InLine = TRUE;
      v141->Type4.Deallocate = FALSE;
      v141->Type4.LongForm = FALSE;
      v141->Type4.TypeName = TypePtReceive;
      v141->Type4.TypeSizeInBits = 32;
      v141->Type4.NumObjects = 1;
#endif
      /* Because of nothing leftnot */
      v141->Arg4 = (HisPort);
   }
}



/* function xPortInterpose (inp, rep) */
XPortInterpose (Inp, Rep)
ptr_ Inp, Rep;

{
   struct MyMessage
        {
           msg_	Head;
           type_	Type2;
           port_	Arg2;
           type_	Type3;
           port_	Arg3;
        };

   struct RepMessage
        {
           msg_	Head;
           type_	RetCodeType;
           short	RetCode;
           type_	Type4;
           port_	Arg4;
        };
   typedef struct MyMessage *ptrMyMsg;
   typedef struct RepMessage *ptrRepMsg;
   ptrMyMsg MyMsgp;
   ptrRepMsg Repmsgp;

   port_ ServPort;
   port_ MyPort;
   port_ HisPort;
   port_ MynewPort;


   MyMsgp = (ptrMyMsg )(Inp);
   Repmsgp = (ptrRepMsg )(Rep);
   Repmsgp->RetCode = SUCCESS;
   {
      register msg_	 *v145 = &Repmsgp->Head;

      v145->SimpleMsg = FALSE;
      v145->MsgSize = sizeof (struct RepMessage);
   }
   {
      register struct MyMessage *v147 = MyMsgp;

      ServPort = (v147->Head.RemotePort);
#if Typecheck
      if (v147->Type2.TypeName != TypePt_RECEIVE)
         {
            Repmsgp->RetCode = WRONG_ARGS;
            return;
         }
#endif
      MyPort = (v147->Arg2);
#if Typecheck
      if (v147->Type3.TypeName != TypePt)
         {
            Repmsgp->RetCode = WRONG_ARGS;
            return;
         }
#endif
      HisPort = (v147->Arg3);
   }

   Repmsgp->RetCode = PortInterpose(ServPort, MyPort, HisPort, MynewPort);
   {
      register struct RepMessage *v148 = Repmsgp;

#if Fastassign
      v148->Type4.Lng = 02000220004;
#else
      v148->Type4.InLine = TRUE;
      v148->Type4.Deallocate = FALSE;
      v148->Type4.LongForm = FALSE;
      v148->Type4.TypeName  = TypePtReceive;
      v148->Type4.TypeSizeInBits = 32;
      v148->Type4.NumObjects = 1;
#endif
      /* Because of nothing leftnot */
      v148->Arg4 = (MynewPort);
   }
}



/* function xfork (inp, rep) */
XFork (Inp, Rep)
ptr_ Inp, Rep;

{
   struct MyMessage
        {
           msg_	Head;
           type_	Type2;
           port_	Arg2;
           type_	Type3;
           port_	Arg3;
           type_	Type4;
           short	Tname4;
           short	Tsize4;
           long Numelts4;
           ptrPortArray Arg4;
        };

   struct RepMessage
        {
           msg_	Head;
           type_	RetCodeType;
           short	RetCode;
           type_	Type2;
           port_	Arg2;
           type_	Type3;
           port_	Arg3;
           type_	Type4;
           short	Tname4;
           short	Tsize4;
           long Numelts4;
           ptrPortArray Arg4;
        };
   typedef struct MyMessage *ptrMyMsg;
   typedef struct RepMessage *ptrRepMsg;
   ptrMyMsg MyMsgp;
   ptrRepMsg Repmsgp;

   port_ ServPort;
   port_ HiskernelPort;
   port_ HisdataPort;
   ptrPortArray Ports;
   long Ports_cnt;


   MyMsgp = (ptrMyMsg )(Inp);
   Repmsgp = (ptrRepMsg )(Rep);
   Repmsgp->RetCode = SUCCESS;
   {
      register msg_	 *v152 = &Repmsgp->Head;

      v152->SimpleMsg = FALSE;
      v152->MsgSize = sizeof (struct RepMessage);
   }
   {
      register struct MyMessage *v154 = MyMsgp;

      ServPort = (v154->Head.RemotePort);
#if Typecheck
      if (v154->Type2.TypeName != TypePt)
         {
            Repmsgp->RetCode = WRONG_ARGS;
            return;
         }
#endif
      HiskernelPort = (v154->Arg2);
#if Typecheck
      if (v154->Type3.TypeName != TypePt)
         {
            Repmsgp->RetCode = WRONG_ARGS;
            return;
         }
#endif
      HisdataPort = (v154->Arg3);
#if Typecheck
      if (v154->Tname4 != TypePt)
         {
            Repmsgp->RetCode = WRONG_ARGS;
            return;
         }
#endif
      Ports_cnt = v154->Numelts4;
      Ports = (v154->Arg4);
   }

   Repmsgp->RetCode = Fork(ServPort, &HiskernelPort, &HisdataPort, &Ports, &Ports_cnt);
   {
      register struct RepMessage *v155 = Repmsgp;

#if Fastassign
      v155->Type2.Lng = 02000220006;
#else
      v155->Type2.InLine = TRUE;
      v155->Type2.Deallocate = FALSE;
      v155->Type2.LongForm = FALSE;
      v155->Type2.TypeName = TypePt;
      v155->Type2.TypeSizeInBits = 32;
      v155->Type2.NumObjects = 1;
#endif
      /* Because of nothing leftnot */
      v155->Arg2 = (HiskernelPort);
#if Fastassign
      v155->Type3.Lng = 02000220006;
#else
      v155->Type3.InLine = TRUE;
      v155->Type3.Deallocate = FALSE;
      v155->Type3.LongForm = FALSE;
      v155->Type3.TypeName = TypePt;
      v155->Type3.TypeSizeInBits = 32;
      v155->Type3.NumObjects = 1;
#endif
      /* Because of nothing leftnot */
      v155->Arg3 = (HisdataPort);
#if Fastassign
      v155->Type4.Lng = 04000000000;
#else
      v155->Type4.InLine = FALSE;
      v155->Type4.Deallocate = FALSE;
      v155->Type4.LongForm = TRUE;
#endif
      /* Because of Long form*/
      v155->Tname4 = TypePt;
      v155->Tsize4 = 32;
      v155->Numelts4 = Ports_cnt;
      v155->Arg4 = (Ports);
   }
}



/* function xstatus (inp, rep) */
XStatus (Inp, Rep)
ptr_ Inp, Rep;

{
   struct MyMessage
        {
           msg_	Head;
        };

   struct RepMessage
        {
           msg_	Head;
           type_	RetCodeType;
           short	RetCode;
           type_	Type2;
           Pstatus Arg2;
        };
   typedef struct MyMessage *ptrMyMsg;
   typedef struct RepMessage *ptrRepMsg;
   ptrMyMsg MyMsgp;
   ptrRepMsg Repmsgp;

   port_ ServPort;
   Pstatus Nstats;


   MyMsgp = (ptrMyMsg )(Inp);
   Repmsgp = (ptrRepMsg )(Rep);
   Repmsgp->RetCode = SUCCESS;
   {
      register msg_	 *v164 = &Repmsgp->Head;

      v164->SimpleMsg = TRUE;
      v164->MsgSize = sizeof (struct RepMessage);
   }
   {
      register struct MyMessage *v166 = MyMsgp;

      ServPort = (v166->Head.RemotePort);
   }

   Repmsgp->RetCode = Status(ServPort, Nstats);
   {
      register struct RepMessage *v167 = Repmsgp;

#if Fastassign
      v167->Type2.Lng = 02000370013;
#else
      v167->Type2.InLine = TRUE;
      v167->Type2.Deallocate = FALSE;
      v167->Type2.LongForm = FALSE;
      v167->Type2.TypeName = TypePStat;
      v167->Type2.TypeSizeInBits = 240;
      v167->Type2.NumObjects = 1;
#endif
      /* Because of nothing leftnot */
      v167->Arg2 = (Nstats);
   }
}



/* function xterminate (inp, rep) */
XTerminate (Inp, Rep)
ptr_ Inp, Rep;

{
   struct MyMessage
        {
           msg_	Head;
           type_	Type2;
           long Arg2;
        };

   struct RepMessage
        {
           msg_	Head;
           type_	RetCodeType;
           short	RetCode;
        };
   typedef struct MyMessage *ptrMyMsg;
   typedef struct RepMessage *ptrRepMsg;
   ptrMyMsg MyMsgp;
   ptrRepMsg Repmsgp;

   port_ ServPort;
   long Reason;


   MyMsgp = (ptrMyMsg )(Inp);
   Repmsgp = (ptrRepMsg )(Rep);
   Repmsgp->RetCode = SUCCESS;
   {
      register msg_	 *v171 = &Repmsgp->Head;

      v171->SimpleMsg = TRUE;
      v171->MsgSize = sizeof (struct RepMessage);
   }
   {
      register struct MyMessage *v173 = MyMsgp;

      ServPort = (v173->Head.RemotePort);
#if Typecheck
      if (v173->Type2.TypeName != TypeInt32)
         {
            Repmsgp->RetCode = WRONG_ARGS;
            return;
         }
#endif
      Reason = (v173->Arg2);
   }

   Repmsgp->RetCode = Terminate(ServPort, Reason);
   {
      register struct RepMessage *v174 = Repmsgp;

   }
}



/* function xsetpriority (inp, rep) */
XSetPriority (Inp, Rep)
ptr_ Inp, Rep;

{
   struct MyMessage
        {
           msg_	Head;
           type_	Type2;
           Priorid Arg2;
        };

   struct RepMessage
        {
           msg_	Head;
           type_	RetCodeType;
           short	RetCode;
        };
   typedef struct MyMessage *ptrMyMsg;
   typedef struct RepMessage *ptrRepMsg;
   ptrMyMsg MyMsgp;
   ptrRepMsg Repmsgp;

   port_ ServPort;
   Priorid Priority;


   MyMsgp = (ptrMyMsg )(Inp);
   Repmsgp = (ptrRepMsg )(Rep);
   Repmsgp->RetCode = SUCCESS;
   {
      register msg_	 *v175 = &Repmsgp->Head;

      v175->SimpleMsg = TRUE;
      v175->MsgSize = sizeof (struct RepMessage);
   }
   {
      register struct MyMessage *v177 = MyMsgp;

      ServPort = (v177->Head.RemotePort);
#if Typecheck
      if (v177->Type2.TypeName != TypeInt16)
         {
            Repmsgp->RetCode = WRONG_ARGS;
            return;
         }
#endif
      Priority = (v177->Arg2);
   }

   Repmsgp->RetCode = SetPriority(ServPort, Priority);
   {
      register struct RepMessage *v178 = Repmsgp;

   }
}



/* function xsetlimit (inp, rep) */
XSetLimit (Inp, Rep)
ptr_ Inp, Rep;

{
   struct MyMessage
        {
           msg_	Head;
           type_	Type2;
           port_	Arg2;
           type_	Type3;
           long Arg3;
        };

   struct RepMessage
        {
           msg_	Head;
           type_	RetCodeType;
           short	RetCode;
        };
   typedef struct MyMessage *ptrMyMsg;
   typedef struct RepMessage *ptrRepMsg;
   ptrMyMsg MyMsgp;
   ptrRepMsg Repmsgp;

   port_ ServPort;
   port_ ReplyPort;
   long Limit;


   MyMsgp = (ptrMyMsg )(Inp);
   Repmsgp = (ptrRepMsg )(Rep);
   Repmsgp->RetCode = SUCCESS;
   {
      register msg_	 *v179 = &Repmsgp->Head;

      v179->SimpleMsg = TRUE;
      v179->MsgSize = sizeof (struct RepMessage);
   }
   {
      register struct MyMessage *v181 = MyMsgp;

      ServPort = (v181->Head.RemotePort);
#if Typecheck
      if (v181->Type2.TypeName != TypePt)
         {
            Repmsgp->RetCode = WRONG_ARGS;
            return;
         }
#endif
      ReplyPort = (v181->Arg2);
#if Typecheck
      if (v181->Type3.TypeName != TypeInt32)
         {
            Repmsgp->RetCode = WRONG_ARGS;
            return;
         }
#endif
      Limit = (v181->Arg3);
   }

   Repmsgp->RetCode = SetLimit(ServPort, ReplyPort, Limit);
   {
      register struct RepMessage *v182 = Repmsgp;

   }
}



/* function xsuspend (inp, rep) */
XSuspend (Inp, Rep)
ptr_ Inp, Rep;

{
   struct MyMessage
        {
           msg_	Head;
        };

   struct RepMessage
        {
           msg_	Head;
           type_	RetCodeType;
           short	RetCode;
        };
   typedef struct MyMessage *ptrMyMsg;
   typedef struct RepMessage *ptrRepMsg;
   ptrMyMsg MyMsgp;
   ptrRepMsg Repmsgp;

   port_ ServPort;


   MyMsgp = (ptrMyMsg )(Inp);
   Repmsgp = (ptrRepMsg )(Rep);
   Repmsgp->RetCode = SUCCESS;
   {
      register msg_	 *v183 = &Repmsgp->Head;

      v183->SimpleMsg = TRUE;
      v183->MsgSize = sizeof (struct RepMessage);
   }
   {
      register struct MyMessage *v185 = MyMsgp;

      ServPort = (v185->Head.RemotePort);
   }

   Repmsgp->RetCode = Suspend(ServPort);
   {
      register struct RepMessage *v186 = Repmsgp;

   }
}



/* function xresume (inp, rep) */
XRESUME (Inp, Rep)
ptr_ Inp, Rep;

{
   struct MyMessage
        {
           msg_	Head;
        };

   struct RepMessage
        {
           msg_	Head;
           type_	RetCodeType;
           short	RetCode;
        };
   typedef struct MyMessage *ptrMyMsg;
   typedef struct RepMessage *ptrRepMsg;
   ptrMyMsg MyMsgp;
   ptrRepMsg Repmsgp;

   port_ ServPort;


   MyMsgp = (ptrMyMsg )(Inp);
   Repmsgp = (ptrRepMsg )(Rep);
   Repmsgp->RetCode = SUCCESS;
   {
      register msg_	 *v187 = &Repmsgp->Head;

      v187->SimpleMsg = TRUE;
      v187->MsgSize = sizeof (struct RepMessage);
   }
   {
      register struct MyMessage *v189 = MyMsgp;

      ServPort = (v189->Head.RemotePort);
   }

   Repmsgp->RetCode = RESUME(ServPort);
   {
      register struct RepMessage *v190 = Repmsgp;

   }
}



/* function xexamine (inp, rep) */
XExamine (Inp, Rep)
ptr_ Inp, Rep;

{
   struct MyMessage
        {
           msg_	Head;
           type_	Type2;
           long_bool	Arg2;
           type_	Type3;
           short	Arg3;
        };

   struct RepMessage
        {
           msg_	Head;
           type_	RetCodeType;
           short	RetCode;
           type_	Type4;
           short	Arg4;
        };
   typedef struct MyMessage *ptrMyMsg;
   typedef struct RepMessage *ptrRepMsg;
   ptrMyMsg MyMsgp;
   ptrRepMsg Repmsgp;

   port_ ServPort;
   long_bool Regorstack;
   short Index;
   short Value;


   MyMsgp = (ptrMyMsg )(Inp);
   Repmsgp = (ptrRepMsg )(Rep);
   Repmsgp->RetCode = SUCCESS;
   {
      register msg_	 *v191 = &Repmsgp->Head;

      v191->SimpleMsg = TRUE;
      v191->MsgSize = sizeof (struct RepMessage);
   }
   {
      register struct MyMessage *v193 = MyMsgp;

      ServPort = (v193->Head.RemotePort);
#if Typecheck
      if (v193->Type2.TypeName != TypeBoolean)
         {
            Repmsgp->RetCode = WRONG_ARGS;
            return;
         }
#endif
      Regorstack = (v193->Arg2);
#if Typecheck
      if (v193->Type3.TypeName != TypeInt16)
         {
            Repmsgp->RetCode = WRONG_ARGS;
            return;
         }
#endif
      Index = (v193->Arg3);
   }

   Repmsgp->RetCode = Examine(ServPort, Regorstack, Index, Value);
   {
      register struct RepMessage *v194 = Repmsgp;

#if Fastassign
      v194->Type4.Lng = 02000210001;
#else
      v194->Type4.InLine = TRUE;
      v194->Type4.Deallocate = FALSE;
      v194->Type4.LongForm = FALSE;
      v194->Type4.TypeName = TypeInt16;
      v194->Type4.TypeSizeInBits = 16;
      v194->Type4.NumObjects = 1;
#endif
      /* Because of nothing leftnot */
      v194->Arg4 = (Value);
   }
}



/* function xdeposit (inp, rep) */
XDeposit (Inp, Rep)
ptr_ Inp, Rep;

{
   struct MyMessage
        {
           msg_	Head;
           type_	Type2;
           long_bool Arg2;
           type_	Type3;
           short	Arg3;
           type_	Type4;
           short	Arg4;
        };

   struct RepMessage
        {
           msg_	Head;
           type_	RetCodeType;
           short	RetCode;
        };
   typedef struct MyMessage *ptrMyMsg;
   typedef struct RepMessage *ptrRepMsg;
   ptrMyMsg MyMsgp;
   ptrRepMsg Repmsgp;

   port_ ServPort;
   long_bool Regorstack;
   short Index;
   short Value;


   MyMsgp = (ptrMyMsg )(Inp);
   Repmsgp = (ptrRepMsg )(Rep);
   Repmsgp->RetCode = SUCCESS;
   {
      register msg_	 *v198 = &Repmsgp->Head;

      v198->SimpleMsg = TRUE;
      v198->MsgSize = sizeof (struct RepMessage);
   }
   {
      register struct MyMessage *v200 = MyMsgp;

      ServPort = (v200->Head.RemotePort);
#if Typecheck
      if (v200->Type2.TypeName != TypeBoolean)
         {
            Repmsgp->RetCode = WRONG_ARGS;
            return;
         }
#endif
      Regorstack = (v200->Arg2);
#if Typecheck
      if (v200->Type3.TypeName != TypeInt16)
         {
            Repmsgp->RetCode = WRONG_ARGS;
            return;
         }
#endif
      Index = (v200->Arg3);
#if Typecheck
      if (v200->Type4.TypeName != TypeInt16)
         {
            Repmsgp->RetCode = WRONG_ARGS;
            return;
         }
#endif
      Value = (v200->Arg4);
   }

   Repmsgp->RetCode = Deposit(ServPort, Regorstack, Index, Value);
   {
      register struct RepMessage *v201 = Repmsgp;

   }
}



/* function xsoftinterrupt (inp, rep) */
XSoftInterrupt (Inp, Rep)
ptr_ Inp, Rep;

{
   struct MyMessage
        {
           msg_	Head;
           type_	Type2;
           long_bool Arg2;
           type_	Type3;
           long_bool Arg3;
        };

   struct RepMessage
        {
           msg_	Head;
           type_	RetCodeType;
           short	RetCode;
           type_	Type3;
           long_bool Arg3;
        };
   typedef struct MyMessage *ptrMyMsg;
   typedef struct RepMessage *ptrRepMsg;
   ptrMyMsg MyMsgp;
   ptrRepMsg Repmsgp;

   port_ ServPort;
   long_bool Normoremerg;
   long_bool Enordisable;


   MyMsgp = (ptrMyMsg )(Inp);
   Repmsgp = (ptrRepMsg )(Rep);
   Repmsgp->RetCode = SUCCESS;
   {
      register msg_	 *v202 = &Repmsgp->Head;

      v202->SimpleMsg = TRUE;
      v202->MsgSize = sizeof (struct RepMessage);
   }
   {
      register struct MyMessage *v204 = MyMsgp;

      ServPort = (v204->Head.RemotePort);
#if Typecheck
      if (v204->Type2.TypeName != TypeBoolean)
         {
            Repmsgp->RetCode = WRONG_ARGS;
            return;
         }
#endif
      Normoremerg = (v204->Arg2);
#if Typecheck
      if (v204->Type3.TypeName != TypeBoolean)
         {
            Repmsgp->RetCode = WRONG_ARGS;
            return;
         }
#endif
      Enordisable = (v204->Arg3);
   }

   Repmsgp->RetCode = SoftInterrupt(ServPort, Normoremerg, Enordisable);
   {
      register struct RepMessage *v205 = Repmsgp;

#if Fastassign
      v205->Type3.Lng = 02000210000;
#else
      v205->Type3.InLine = TRUE;
      v205->Type3.Deallocate = FALSE;
      v205->Type3.LongForm = FALSE;
      v205->Type3.TypeName = TypeBoolean;
      v205->Type3.TypeSizeInBits = 16;
      v205->Type3.NumObjects = 1;
#endif
      /* Because of nothing leftnot */
      v205->Arg3 = (Enordisable);
   }
}



/* function xcreatesegment (inp, rep) */
XCreateSegment (Inp, Rep)
ptr_ Inp, Rep;

{
   struct MyMessage
        {
           msg_	Head;
           type_	Type2;
           port_	Arg2;
           type_	Type3;
           Spicesegkind Arg3;
           type_	Type4;
           short	Arg4;
           type_	Type5;
           short	Arg5;
           type_	Type6;
           long_bool Arg6;
        };

   struct RepMessage
        {
           msg_	Head;
           type_	RetCodeType;
           short	RetCode;
           type_	Type7;
           Segid Arg7;
        };
   typedef struct MyMessage *ptrMyMsg;
   typedef struct RepMessage *ptrRepMsg;
   ptrMyMsg MyMsgp;
   ptrRepMsg Repmsgp;

   port_ ServPort;
   port_ ImagsegPort;
   Spicesegkind Segmentkind;
   short Initialsize;
   short Maxsize;
   long_bool Stable;
   Segid Segment;


   MyMsgp = (ptrMyMsg )(Inp);
   Repmsgp = (ptrRepMsg )(Rep);
   Repmsgp->RetCode = SUCCESS;
   {
      register msg_	 *v209 = &Repmsgp->Head;

      v209->SimpleMsg = TRUE;
      v209->MsgSize = sizeof (struct RepMessage);
   }
   {
      register struct MyMessage *v211 = MyMsgp;

      ServPort = (v211->Head.RemotePort);
#if Typecheck
      if (v211->Type2.TypeName != TypePt)
         {
            Repmsgp->RetCode = WRONG_ARGS;
            return;
         }
#endif
      ImagsegPort = (v211->Arg2);
#if Typecheck
      if (v211->Type3.TypeName != TypeInt16)
         {
            Repmsgp->RetCode = WRONG_ARGS;
            return;
         }
#endif
      Segmentkind = (v211->Arg3);
#if Typecheck
      if (v211->Type4.TypeName != TypeInt16)
         {
            Repmsgp->RetCode = WRONG_ARGS;
            return;
         }
#endif
      Initialsize = (v211->Arg4);
#if Typecheck
      if (v211->Type5.TypeName != TypeInt16)
         {
            Repmsgp->RetCode = WRONG_ARGS;
            return;
         }
#endif
      Maxsize = (v211->Arg5);
#if Typecheck
      if (v211->Type6.TypeName != TypeBoolean)
         {
            Repmsgp->RetCode = WRONG_ARGS;
            return;
         }
#endif
      Stable = (v211->Arg6);
   }

   Repmsgp->RetCode = CreateSegment(ServPort, ImagsegPort, Segmentkind, Initialsize, Maxsize, Stable, Segment);
   {
      register struct RepMessage *v212 = Repmsgp;

#if Fastassign
      v212->Type7.Lng = 02000220002;
#else
      v212->Type7.InLine = TRUE;
      v212->Type7.Deallocate = FALSE;
      v212->Type7.LongForm = FALSE;
      v212->Type7.TypeName = TypeInt32;
      v212->Type7.TypeSizeInBits = 32;
      v212->Type7.NumObjects = 1;
#endif
      /* Because of nothing leftnot */
      v212->Arg7 = (Segment);
   }
}



/* function xtruncatesegment (inp, rep) */
XTruncateSegment (Inp, Rep)
ptr_ Inp, Rep;

{
   struct MyMessage
        {
           msg_	Head;
           type_	Type2;
           Segid Arg2;
           type_	Type3;
           short	Arg3;
        };

   struct RepMessage
        {
           msg_	Head;
           type_	RetCodeType;
           short	RetCode;
        };
   typedef struct MyMessage *ptrMyMsg;
   typedef struct RepMessage *ptrRepMsg;
   ptrMyMsg MyMsgp;
   ptrRepMsg Repmsgp;

   port_ ServPort;
   Segid Segment;
   short Newsize;


   MyMsgp = (ptrMyMsg )(Inp);
   Repmsgp = (ptrRepMsg )(Rep);
   Repmsgp->RetCode = SUCCESS;
   {
      register msg_	 *v216 = &Repmsgp->Head;

      v216->SimpleMsg = TRUE;
      v216->MsgSize = sizeof (struct RepMessage);
   }
   {
      register struct MyMessage *v218 = MyMsgp;

      ServPort = (v218->Head.RemotePort);
#if Typecheck
      if (v218->Type2.TypeName != TypeInt32)
         {
            Repmsgp->RetCode = WRONG_ARGS;
            return;
         }
#endif
      Segment = (v218->Arg2);
#if Typecheck
      if (v218->Type3.TypeName != TypeInt16)
         {
            Repmsgp->RetCode = WRONG_ARGS;
            return;
         }
#endif
      Newsize = (v218->Arg3);
   }

   Repmsgp->RetCode = TruncateSegment(ServPort, Segment, Newsize);
   {
      register struct RepMessage *v219 = Repmsgp;

   }
}



/* function xdestroysegment (inp, rep) */
XDestroySegment (Inp, Rep)
ptr_ Inp, Rep;

{
   struct MyMessage
        {
           msg_	Head;
           type_	Type2;
           Segid Arg2;
        };

   struct RepMessage
        {
           msg_	Head;
           type_	RetCodeType;
           short	RetCode;
        };
   typedef struct MyMessage *ptrMyMsg;
   typedef struct RepMessage *ptrRepMsg;
   ptrMyMsg MyMsgp;
   ptrRepMsg Repmsgp;

   port_ ServPort;
   Segid Segment;


   MyMsgp = (ptrMyMsg )(Inp);
   Repmsgp = (ptrRepMsg )(Rep);
   Repmsgp->RetCode = SUCCESS;
   {
      register msg_	 *v220 = &Repmsgp->Head;

      v220->SimpleMsg = TRUE;
      v220->MsgSize = sizeof (struct RepMessage);
   }
   {
      register struct MyMessage *v222 = MyMsgp;

      ServPort = (v222->Head.RemotePort);
#if Typecheck
      if (v222->Type2.TypeName != TypeInt32)
         {
            Repmsgp->RetCode = WRONG_ARGS;
            return;
         }
#endif
      Segment = (v222->Arg2);
   }

   Repmsgp->RetCode = DestroySegment(ServPort, Segment);
   {
      register struct RepMessage *v223 = Repmsgp;

   }
}



/* function xreadsegment (inp, rep) */
XReadSegment (Inp, Rep)
ptr_ Inp, Rep;

{
   struct MyMessage
        {
           msg_	Head;
           type_	Type2;
           Segid Arg2;
           type_	Type3;
           short	Arg3;
           type_	Type4;
           short	Arg4;
        };

   struct RepMessage
        {
           msg_	Head;
           type_	RetCodeType;
           short	RetCode;
           type_	Type5;
           short	Tname5;
           short	Tsize5;
           long Numelts5;
           ptr_ Arg5;
        };
   typedef struct MyMessage *ptrMyMsg;
   typedef struct RepMessage *ptrRepMsg;
   ptrMyMsg MyMsgp;
   ptrRepMsg Repmsgp;

   port_ ServPort;
   Segid Segment;
   short Offset;
   short Numpages;
   ptr_ Data;
   long Data_cnt;


   MyMsgp = (ptrMyMsg )(Inp);
   Repmsgp = (ptrRepMsg )(Rep);
   Repmsgp->RetCode = SUCCESS;
   {
      register msg_	 *v224 = &Repmsgp->Head;

      v224->SimpleMsg = FALSE;
      v224->MsgSize = sizeof (struct RepMessage);
   }
   {
      register struct MyMessage *v226 = MyMsgp;

      ServPort = (v226->Head.RemotePort);
#if Typecheck
      if (v226->Type2.TypeName != TypeInt32)
         {
            Repmsgp->RetCode = WRONG_ARGS;
            return;
         }
#endif
      Segment = (v226->Arg2);
#if Typecheck
      if (v226->Type3.TypeName != TypeInt16)
         {
            Repmsgp->RetCode = WRONG_ARGS;
            return;
         }
#endif
      Offset = (v226->Arg3);
#if Typecheck
      if (v226->Type4.TypeName != TypeInt16)
         {
            Repmsgp->RetCode = WRONG_ARGS;
            return;
         }
#endif
      Numpages = (v226->Arg4);
   }

   Repmsgp->RetCode = ReadSegment(ServPort, Segment, Offset, Numpages, Data, Data_cnt);
   {
      register struct RepMessage *v227 = Repmsgp;

#if Fastassign
      v227->Type5.Lng = 04000000000;
#else
      v227->Type5.InLine = FALSE;
      v227->Type5.Deallocate = FALSE;
      v227->Type5.LongForm = TRUE;
#endif
      /* Because of Long form*/
      v227->Tname5 = TypeInt8;
      v227->Tsize5 = 8;
      v227->Numelts5 = Data_cnt;
      v227->Arg5 = (Data);
   }
}



/* function xwritesegment (inp, rep) */
XWriteSegment (Inp, Rep)
ptr_ Inp, Rep;

{
   struct MyMessage
        {
           msg_	Head;
           type_	Type2;
           Segid Arg2;
           type_	Type3;
           short	Arg3;
           type_	Type4;
           short	Tname4;
           short	Tsize4;
           long Numelts4;
           ptr_ Arg4;
        };

   struct RepMessage
        {
           msg_	Head;
           type_	RetCodeType;
           short	RetCode;
        };
   typedef struct MyMessage *ptrMyMsg;
   typedef struct RepMessage *ptrRepMsg;
   ptrMyMsg MyMsgp;
   ptrRepMsg Repmsgp;

   port_ ServPort;
   Segid Segment;
   short Offset;
   ptr_ Data;
   long Data_cnt;


   MyMsgp = (ptrMyMsg )(Inp);
   Repmsgp = (ptrRepMsg )(Rep);
   Repmsgp->RetCode = SUCCESS;
   {
      register msg_	 *v230 = &Repmsgp->Head;

      v230->SimpleMsg = TRUE;
      v230->MsgSize = sizeof (struct RepMessage);
   }
   {
      register struct MyMessage *v232 = MyMsgp;

      ServPort = (v232->Head.RemotePort);
#if Typecheck
      if (v232->Type2.TypeName != TypeInt32)
         {
            Repmsgp->RetCode = WRONG_ARGS;
            return;
         }
#endif
      Segment = (v232->Arg2);
#if Typecheck
      if (v232->Type3.TypeName != TypeInt16)
         {
            Repmsgp->RetCode = WRONG_ARGS;
            return;
         }
#endif
      Offset = (v232->Arg3);
#if Typecheck
      if (v232->Tname4 != TypeInt8)
         {
            Repmsgp->RetCode = WRONG_ARGS;
            return;
         }
#endif
      Data_cnt = v232->Numelts4;
      Data = (v232->Arg4);
   }

   Repmsgp->RetCode = WriteSegment(ServPort, Segment, Offset, Data, Data_cnt);
   {
      register struct RepMessage *v233 = Repmsgp;

   }
}



/* function xvalidatememory (inp, rep) */
XValidateMemory (Inp, Rep)
ptr_ Inp, Rep;

{
   struct MyMessage
        {
           msg_	Head;
           type_	Type2;
           ptr_ Arg2;
           type_	Type3;
           long Arg3;
           type_	Type4;
           long Arg4;
        };

   struct RepMessage
        {
           msg_	Head;
           type_	RetCodeType;
           short	RetCode;
           type_	Type2;
           ptr_ Arg2;
        };
   typedef struct MyMessage *ptrMyMsg;
   typedef struct RepMessage *ptrRepMsg;
   ptrMyMsg MyMsgp;
   ptrRepMsg Repmsgp;

   port_ ServPort;
   ptr_ Address;
   long Numbytes;
   long Createmask;


   MyMsgp = (ptrMyMsg )(Inp);
   Repmsgp = (ptrRepMsg )(Rep);
   Repmsgp->RetCode = SUCCESS;
   {
      register msg_	 *v234 = &Repmsgp->Head;

      v234->SimpleMsg = TRUE;
      v234->MsgSize = sizeof (struct RepMessage);
   }
   {
      register struct MyMessage *v236 = MyMsgp;

      ServPort = (v236->Head.RemotePort);
#if Typecheck
      if (v236->Type2.TypeName != TypeInt32)
         {
            Repmsgp->RetCode = WRONG_ARGS;
            return;
         }
#endif
      Address = (v236->Arg2);
#if Typecheck
      if (v236->Type3.TypeName != TypeInt32)
         {
            Repmsgp->RetCode = WRONG_ARGS;
            return;
         }
#endif
      Numbytes = (v236->Arg3);
#if Typecheck
      if (v236->Type4.TypeName != TypeInt32)
         {
            Repmsgp->RetCode = WRONG_ARGS;
            return;
         }
#endif
      Createmask = (v236->Arg4);
   }

   Repmsgp->RetCode = ValidateMemory(ServPort, Address, Numbytes, Createmask);
   {
      register struct RepMessage *v237 = Repmsgp;

#if Fastassign
      v237->Type2.Lng = 02000220002;
#else
      v237->Type2.InLine = TRUE;
      v237->Type2.Deallocate = FALSE;
      v237->Type2.LongForm = FALSE;
      v237->Type2.TypeName = TypeInt32;
      v237->Type2.TypeSizeInBits = 32;
      v237->Type2.NumObjects = 1;
#endif
      /* Because of nothing leftnot */
      v237->Arg2 = (Address);
   }
}



/* function xinvalidatememory (inp, rep) */
XInvalidateMemory (Inp, Rep)
ptr_ Inp, Rep;

{
   struct MyMessage
        {
           msg_	Head;
           type_	Type2;
           ptr_ Arg2;
           type_	Type3;
           long Arg3;
        };

   struct RepMessage
        {
           msg_	Head;
           type_	RetCodeType;
           short	RetCode;
        };
   typedef struct MyMessage *ptrMyMsg;
   typedef struct RepMessage *ptrRepMsg;
   ptrMyMsg MyMsgp;
   ptrRepMsg Repmsgp;

   port_ ServPort;
   ptr_ Address;
   long Numbytes;


   MyMsgp = (ptrMyMsg )(Inp);
   Repmsgp = (ptrRepMsg )(Rep);
   Repmsgp->RetCode = SUCCESS;
   {
      register msg_	 *v241 = &Repmsgp->Head;

      v241->SimpleMsg = TRUE;
      v241->MsgSize = sizeof (struct RepMessage);
   }
   {
      register struct MyMessage *v243 = MyMsgp;

      ServPort = (v243->Head.RemotePort);
#if Typecheck
      if (v243->Type2.TypeName != TypeInt32)
         {
            Repmsgp->RetCode = WRONG_ARGS;
            return;
         }
#endif
      Address = (v243->Arg2);
#if Typecheck
      if (v243->Type3.TypeName != TypeInt32)
         {
            Repmsgp->RetCode = WRONG_ARGS;
            return;
         }
#endif
      Numbytes = (v243->Arg3);
   }

   Repmsgp->RetCode = InvalidateMemory(ServPort, Address, Numbytes);
   {
      register struct RepMessage *v244 = Repmsgp;

   }
}



/* function xsetprotection (inp, rep) */
XSetProtection (Inp, Rep)
ptr_ Inp, Rep;

{
   struct MyMessage
        {
           msg_	Head;
           type_	Type2;
           ptr_ Arg2;
           type_	Type3;
           long Arg3;
           type_	Type4;
           short	Arg4;
        };

   struct RepMessage
        {
           msg_	Head;
           type_	RetCodeType;
           short	RetCode;
        };
   typedef struct MyMessage *ptrMyMsg;
   typedef struct RepMessage *ptrRepMsg;
   ptrMyMsg MyMsgp;
   ptrRepMsg Repmsgp;

   port_ ServPort;
   ptr_ Address;
   long Numbytes;
   short Protection;


   MyMsgp = (ptrMyMsg )(Inp);
   Repmsgp = (ptrRepMsg )(Rep);
   Repmsgp->RetCode = SUCCESS;
   {
      register msg_	 *v245 = &Repmsgp->Head;

      v245->SimpleMsg = TRUE;
      v245->MsgSize = sizeof (struct RepMessage);
   }
   {
      register struct MyMessage *v247 = MyMsgp;

      ServPort = (v247->Head.RemotePort);
#if Typecheck
      if (v247->Type2.TypeName != TypeInt32)
         {
            Repmsgp->RetCode = WRONG_ARGS;
            return;
         }
#endif
      Address = (v247->Arg2);
#if Typecheck
      if (v247->Type3.TypeName != TypeInt32)
         {
            Repmsgp->RetCode = WRONG_ARGS;
            return;
         }
#endif
      Numbytes = (v247->Arg3);
#if Typecheck
      if (v247->Type4.TypeName != TypeInt16)
         {
            Repmsgp->RetCode = WRONG_ARGS;
            return;
         }
#endif
      Protection = (v247->Arg4);
   }

   Repmsgp->RetCode = SetProtection(ServPort, Address, Numbytes, Protection);
   {
      register struct RepMessage *v248 = Repmsgp;

   }
}



/* function xreadprocessmemory (inp, rep) */
XReadProcessMemory (Inp, Rep)
ptr_ Inp, Rep;

{
   struct MyMessage
        {
           msg_	Head;
           type_	Type2;
           ptr_ Arg2;
           type_	Type3;
           long Arg3;
        };

   struct RepMessage
        {
           msg_	Head;
           type_	RetCodeType;
           short	RetCode;
           type_	Type4;
           short	Tname4;
           short	Tsize4;
           long Numelts4;
           ptr_ Arg4;
        };
   typedef struct MyMessage *ptrMyMsg;
   typedef struct RepMessage *ptrRepMsg;
   ptrMyMsg MyMsgp;
   ptrRepMsg Repmsgp;

   port_ ServPort;
   ptr_ Address;
   long Numbytes;
   ptr_ Data;
   long Data_cnt;


   MyMsgp = (ptrMyMsg )(Inp);
   Repmsgp = (ptrRepMsg )(Rep);
   Repmsgp->RetCode = SUCCESS;
   {
      register msg_	 *v249 = &Repmsgp->Head;

      v249->SimpleMsg = FALSE;
      v249->MsgSize = sizeof (struct RepMessage);
   }
   {
      register struct MyMessage *v251 = MyMsgp;

      ServPort = (v251->Head.RemotePort);
#if Typecheck
      if (v251->Type2.TypeName != TypeInt32)
         {
            Repmsgp->RetCode = WRONG_ARGS;
            return;
         }
#endif
      Address = (v251->Arg2);
#if Typecheck
      if (v251->Type3.TypeName != TypeInt32)
         {
            Repmsgp->RetCode = WRONG_ARGS;
            return;
         }
#endif
      Numbytes = (v251->Arg3);
   }

   Repmsgp->RetCode = ReadProcessMemory(ServPort, Address, Numbytes, Data, Data_cnt);
   {
      register struct RepMessage *v252 = Repmsgp;

#if Fastassign
      v252->Type4.Lng = 04000000000;
#else
      v252->Type4.InLine = FALSE;
      v252->Type4.Deallocate = FALSE;
      v252->Type4.LongForm = TRUE;
#endif
      /* Because of Long form*/
      v252->Tname4 = TypeInt8;
      v252->Tsize4 = 8;
      v252->Numelts4 = Data_cnt;
      v252->Arg4 = (Data);
   }
}



/* function xwriteprocessmemory (inp, rep) */
XWriteProcessMemory (Inp, Rep)
ptr_ Inp, Rep;

{
   struct MyMessage
        {
           msg_	Head;
           type_	Type2;
           ptr_ Arg2;
           type_	Type3;
           long Arg3;
           type_	Type4;
           short	Tname4;
           short	Tsize4;
           long Numelts4;
           ptr_ Arg4;
        };

   struct RepMessage
        {
           msg_	Head;
           type_	RetCodeType;
           short	RetCode;
        };
   typedef struct MyMessage *ptrMyMsg;
   typedef struct RepMessage *ptrRepMsg;
   ptrMyMsg MyMsgp;
   ptrRepMsg Repmsgp;

   port_ ServPort;
   ptr_ Address;
   long Numbytes;
   ptr_ Data;
   long Data_cnt;


   MyMsgp = (ptrMyMsg )(Inp);
   Repmsgp = (ptrRepMsg )(Rep);
   Repmsgp->RetCode = SUCCESS;
   {
      register msg_	 *v255 = &Repmsgp->Head;

      v255->SimpleMsg = TRUE;
      v255->MsgSize = sizeof (struct RepMessage);
   }
   {
      register struct MyMessage *v257 = MyMsgp;

      ServPort = (v257->Head.RemotePort);
#if Typecheck
      if (v257->Type2.TypeName != TypeInt32)
         {
            Repmsgp->RetCode = WRONG_ARGS;
            return;
         }
#endif
      Address = (v257->Arg2);
#if Typecheck
      if (v257->Type3.TypeName != TypeInt32)
         {
            Repmsgp->RetCode = WRONG_ARGS;
            return;
         }
#endif
      Numbytes = (v257->Arg3);
#if Typecheck
      if (v257->Tname4 != TypeInt8)
         {
            Repmsgp->RetCode = WRONG_ARGS;
            return;
         }
#endif
      Data_cnt = v257->Numelts4;
      Data = (v257->Arg4);
   }

   Repmsgp->RetCode = WriteProcessMemory(ServPort, Address, Numbytes, Data, Data_cnt);
   {
      register struct RepMessage *v258 = Repmsgp;

   }
}



/* function xgetdiskpartitions (inp, rep) */
XGetDiskPartitions (Inp, Rep)
ptr_ Inp, Rep;

{
   struct MyMessage
        {
           msg_	Head;
           type_	Type2;
           short	Arg2;
        };

   struct RepMessage
        {
           msg_	Head;
           type_	RetCodeType;
           short	RetCode;
           type_	Type3;
           Devpartstring Arg3;
           type_	Type4;
           short	Tname4;
           short	Tsize4;
           long Numelts4;
           ptr_ Arg4;
        };
   typedef struct MyMessage *ptrMyMsg;
   typedef struct RepMessage *ptrRepMsg;
   ptrMyMsg MyMsgp;
   ptrRepMsg Repmsgp;

   port_ ServPort;
   short Devnum;
   Devpartstring Devname;
   ptr_ Partl;
   long Partl_cnt;


   MyMsgp = (ptrMyMsg )(Inp);
   Repmsgp = (ptrRepMsg )(Rep);
   Repmsgp->RetCode = SUCCESS;
   {
      register msg_	 *v259 = &Repmsgp->Head;

      v259->SimpleMsg = FALSE;
      v259->MsgSize = sizeof (struct RepMessage);
   }
   {
      register struct MyMessage *v261 = MyMsgp;

      ServPort = (v261->Head.RemotePort);
#if Typecheck
      if (v261->Type2.TypeName != TypeInt16)
         {
            Repmsgp->RetCode = WRONG_ARGS;
            return;
         }
#endif
      Devnum = (v261->Arg2);
   }

   Repmsgp->RetCode = GetDiskPartitions(ServPort, Devnum, Devname, Partl, Partl_cnt);
   {
      register struct RepMessage *v262 = Repmsgp;

#if Fastassign
      v262->Type3.Lng = 02006404010;
#else
      v262->Type3.InLine = TRUE;
      v262->Type3.Deallocate = FALSE;
      v262->Type3.LongForm = FALSE;
      v262->Type3.TypeName = TypeChar;
      v262->Type3.TypeSizeInBits = 8;
      v262->Type3.NumObjects = 26;
#endif
      /* Because of nothing leftnot */
      v262->Arg3 = (Devname);
#if Fastassign
      v262->Type4.Lng = 04000000000;
#else
      v262->Type4.InLine = FALSE;
      v262->Type4.Deallocate = FALSE;
      v262->Type4.LongForm = TRUE;
#endif
      /* Because of Long form*/
      v262->Tname4 = TypeInt32;
      v262->Tsize4 = 32;
      v262->Numelts4 = Partl_cnt;
      v262->Arg4 = (Partl);
   }
}



/* function xpartmount (inp, rep) */
XPartMount (Inp, Rep)
ptr_ Inp, Rep;

{
   struct MyMessage
        {
           msg_	Head;
           type_	Type2;
           Devpartstring Arg2;
           type_	Type3;
           long_bool Arg3;
        };

   struct RepMessage
        {
           msg_	Head;
           type_	RetCodeType;
           short	RetCode;
           type_	Type4;
           Segid Arg4;
           type_	Type5;
           Partitiontype Arg5;
           type_	Type6;
           port_	Arg6;
           type_	Type7;
           Diskaddr Arg7;
           type_	Type8;
           Diskaddr Arg8;
        };
   typedef struct MyMessage *ptrMyMsg;
   typedef struct RepMessage *ptrRepMsg;
   ptrMyMsg MyMsgp;
   ptrRepMsg Repmsgp;

   port_ ServPort;
   Devpartstring Partname;
   long_bool Exuse;
   Segid Rootid;
   Partitiontype Partkind;
   port_ PartPort;
   Diskaddr Parts;
   Diskaddr Parte;


   MyMsgp = (ptrMyMsg )(Inp);
   Repmsgp = (ptrRepMsg )(Rep);
   Repmsgp->RetCode = SUCCESS;
   {
      register msg_	 *v268 = &Repmsgp->Head;

      v268->SimpleMsg = FALSE;
      v268->MsgSize = sizeof (struct RepMessage);
   }
   {
      register struct MyMessage *v270 = MyMsgp;

      ServPort = (v270->Head.RemotePort);
#if Typecheck
      if (v270->Type2.TypeName != TypeChar)
         {
            Repmsgp->RetCode = WRONG_ARGS;
            return;
         }
#endif
      Partname = (v270->Arg2);
#if Typecheck
      if (v270->Type3.TypeName != TypeBoolean)
         {
            Repmsgp->RetCode = WRONG_ARGS;
            return;
         }
#endif
      Exuse = (v270->Arg3);
   }

   Repmsgp->RetCode = PartMount(ServPort, Partname, Exuse, Rootid, Partkind, PartPort, Parts, Parte);
   {
      register struct RepMessage *v271 = Repmsgp;

#if Fastassign
      v271->Type4.Lng = 02000220002;
#else
      v271->Type4.InLine = TRUE;
      v271->Type4.Deallocate = FALSE;
      v271->Type4.LongForm = FALSE;
      v271->Type4.TypeName = TypeInt32;
      v271->Type4.TypeSizeInBits = 32;
      v271->Type4.NumObjects = 1;
#endif
      /* Because of nothing leftnot */
      v271->Arg4 = (Rootid);
#if Fastassign
      v271->Type5.Lng = 02000210001;
#else
      v271->Type5.InLine = TRUE;
      v271->Type5.Deallocate = FALSE;
      v271->Type5.LongForm = FALSE;
      v271->Type5.TypeName = TypeInt16;
      v271->Type5.TypeSizeInBits = 16;
      v271->Type5.NumObjects = 1;
#endif
      /* Because of nothing leftnot */
      v271->Arg5 = (Partkind);
#if Fastassign
      v271->Type6.Lng = 02000220006;
#else
      v271->Type6.InLine = TRUE;
      v271->Type6.Deallocate = FALSE;
      v271->Type6.LongForm = FALSE;
      v271->Type6.TypeName = TypePt;
      v271->Type6.TypeSizeInBits = 32;
      v271->Type6.NumObjects = 1;
#endif
      /* Because of nothing leftnot */
      v271->Arg6 = (PartPort);
#if Fastassign
      v271->Type7.Lng = 02000220002;
#else
      v271->Type7.InLine = TRUE;
      v271->Type7.Deallocate = FALSE;
      v271->Type7.LongForm = FALSE;
      v271->Type7.TypeName = TypeInt32;
      v271->Type7.TypeSizeInBits = 32;
      v271->Type7.NumObjects = 1;
#endif
      /* Because of nothing leftnot */
      v271->Arg7 = (Parts);
#if Fastassign
      v271->Type8.Lng = 02000220002;
#else
      v271->Type8.InLine = TRUE;
      v271->Type8.Deallocate = FALSE;
      v271->Type8.LongForm = FALSE;
      v271->Type8.TypeName = TypeInt32;
      v271->Type8.TypeSizeInBits = 32;
      v271->Type8.NumObjects = 1;
#endif
      /* Because of nothing leftnot */
      v271->Arg8 = (Parte);
   }
}



/* function xpartdismount (inp, rep) */
XPartDismount (Inp, Rep)
ptr_ Inp, Rep;

{
   struct MyMessage
        {
           msg_	Head;
        };

   struct RepMessage
        {
           msg_	Head;
           type_	RetCodeType;
           short	RetCode;
        };
   typedef struct MyMessage *ptrMyMsg;
   typedef struct RepMessage *ptrRepMsg;
   ptrMyMsg MyMsgp;
   ptrRepMsg Repmsgp;

   port_ ServPort;


   MyMsgp = (ptrMyMsg )(Inp);
   Repmsgp = (ptrRepMsg )(Rep);
   Repmsgp->RetCode = SUCCESS;
   {
      register msg_	 *v287 = &Repmsgp->Head;

      v287->SimpleMsg = TRUE;
      v287->MsgSize = sizeof (struct RepMessage);
   }
   {
      register struct MyMessage *v289 = MyMsgp;

      ServPort = (v289->Head.RemotePort);
   }

   Repmsgp->RetCode = PartDismount(ServPort);
   {
      register struct RepMessage *v290 = Repmsgp;

   }
}



/* function xsettempsegpartition (inp, rep) */
XSetTempSegPartition (Inp, Rep)
ptr_ Inp, Rep;

{
   struct MyMessage
        {
           msg_	Head;
           type_	Type2;
           Devpartstring Arg2;
        };

   struct RepMessage
        {
           msg_	Head;
           type_	RetCodeType;
           short	RetCode;
        };
   typedef struct MyMessage *ptrMyMsg;
   typedef struct RepMessage *ptrRepMsg;
   ptrMyMsg MyMsgp;
   ptrRepMsg Repmsgp;

   port_ ServPort;
   Devpartstring Partname;


   MyMsgp = (ptrMyMsg )(Inp);
   Repmsgp = (ptrRepMsg )(Rep);
   Repmsgp->RetCode = SUCCESS;
   {
      register msg_	 *v291 = &Repmsgp->Head;

      v291->SimpleMsg = TRUE;
      v291->MsgSize = sizeof (struct RepMessage);
   }
   {
      register struct MyMessage *v293 = MyMsgp;

      ServPort = (v293->Head.RemotePort);
#if Typecheck
      if (v293->Type2.TypeName != TypeChar)
         {
            Repmsgp->RetCode = WRONG_ARGS;
            return;
         }
#endif
      Partname = (v293->Arg2);
   }

   Repmsgp->RetCode = SetTempSegPartition(ServPort, Partname);
   {
      register struct RepMessage *v294 = Repmsgp;

   }
}



/* function xsetdebugPort (inp, rep) */
XSetDebugPort (Inp, Rep)
ptr_ Inp, Rep;

{
   struct MyMessage
        {
           msg_	Head;
           type_	Type2;
           port_	Arg2;
        };

   struct RepMessage
        {
           msg_	Head;
           type_	RetCodeType;
           short	RetCode;
        };
   typedef struct MyMessage *ptrMyMsg;
   typedef struct RepMessage *ptrRepMsg;
   ptrMyMsg MyMsgp;
   ptrRepMsg Repmsgp;

   port_ ServPort;
   port_ DebugPort;


   MyMsgp = (ptrMyMsg )(Inp);
   Repmsgp = (ptrRepMsg )(Rep);
   Repmsgp->RetCode = SUCCESS;
   {
      register msg_	 *v295 = &Repmsgp->Head;

      v295->SimpleMsg = TRUE;
      v295->MsgSize = sizeof (struct RepMessage);
   }
   {
      register struct MyMessage *v297 = MyMsgp;

      ServPort = (v297->Head.RemotePort);
#if Typecheck
      if (v297->Type2.TypeName != TypePt)
         {
            Repmsgp->RetCode = WRONG_ARGS;
            return;
         }
#endif
      DebugPort = (v297->Arg2);
   }

   Repmsgp->RetCode = SetDebugPort(ServPort, DebugPort);
   {
      register struct RepMessage *v298 = Repmsgp;

   }
}



/* function xtouch (inp, rep) */
XTouch (Inp, Rep)
ptr_ Inp, Rep;

{
   struct MyMessage
        {
           msg_	Head;
           type_	Type2;
           ptr_ Arg2;
        };

   struct RepMessage
        {
           msg_	Head;
           type_	RetCodeType;
           short	RetCode;
        };
   typedef struct MyMessage *ptrMyMsg;
   typedef struct RepMessage *ptrRepMsg;
   ptrMyMsg MyMsgp;
   ptrRepMsg Repmsgp;

   port_ ServPort;
   ptr_ Address;


   MyMsgp = (ptrMyMsg )(Inp);
   Repmsgp = (ptrRepMsg )(Rep);
   Repmsgp->RetCode = SUCCESS;
   {
      register msg_	 *v299 = &Repmsgp->Head;

      v299->SimpleMsg = TRUE;
      v299->MsgSize = sizeof (struct RepMessage);
   }
   {
      register struct MyMessage *v301 = MyMsgp;

      ServPort = (v301->Head.RemotePort);
#if Typecheck
      if (v301->Type2.TypeName != TypeInt32)
         {
            Repmsgp->RetCode = WRONG_ARGS;
            return;
         }
#endif
      Address = (v301->Arg2);
   }

   Repmsgp->RetCode = Touch(ServPort, Address);
   {
      register struct RepMessage *v302 = Repmsgp;

   }
}



/* function xgetPortindexstatus (inp, rep) */
XGetPortIndexStatus (Inp, Rep)
ptr_ Inp, Rep;

{
   struct MyMessage
        {
           msg_	Head;
           type_	Type2;
           long Arg2;
        };

   struct RepMessage
        {
           msg_	Head;
           type_	RetCodeType;
           short	RetCode;
           type_	Type3;
           short	Arg3;
           type_	Type4;
           short	Arg4;
           type_	Type5;
           short	Arg5;
           type_	Type6;
           port_	Arg6;
           type_	Type7;
           short	Arg7;
        };
   typedef struct MyMessage *ptrMyMsg;
   typedef struct RepMessage *ptrRepMsg;
   ptrMyMsg MyMsgp;
   ptrRepMsg Repmsgp;

   port_ ServPort;
   long Portindex;
   short Backlog;
   short Nwaitingmsgs;
   short Ewaitingmsgs;
   port_ Portright;
   short Porttype;


   MyMsgp = (ptrMyMsg )(Inp);
   Repmsgp = (ptrRepMsg )(Rep);
   Repmsgp->RetCode = SUCCESS;
   {
      register msg_	 *v303 = &Repmsgp->Head;

      v303->SimpleMsg = FALSE;
      v303->MsgSize = sizeof (struct RepMessage);
   }
   {
      register struct MyMessage *v305 = MyMsgp;

      ServPort = (v305->Head.RemotePort);
#if Typecheck
      if (v305->Type2.TypeName != TypeInt32)
         {
            Repmsgp->RetCode = WRONG_ARGS;
            return;
         }
#endif
      Portindex = (v305->Arg2);
   }

   Repmsgp->RetCode = GetPortIndexStatus(ServPort, Portindex, Backlog, Nwaitingmsgs, Ewaitingmsgs, Portright, Porttype);
   {
      register struct RepMessage *v306 = Repmsgp;

#if Fastassign
      v306->Type3.Lng = 02000210001;
#else
      v306->Type3.InLine = TRUE;
      v306->Type3.Deallocate = FALSE;
      v306->Type3.LongForm = FALSE;
      v306->Type3.TypeName = TypeInt16;
      v306->Type3.TypeSizeInBits = 16;
      v306->Type3.NumObjects = 1;
#endif
      /* Because of nothing leftnot */
      v306->Arg3 = (Backlog);
#if Fastassign
      v306->Type4.Lng = 02000210001;
#else
      v306->Type4.InLine = TRUE;
      v306->Type4.Deallocate = FALSE;
      v306->Type4.LongForm = FALSE;
      v306->Type4.TypeName = TypeInt16;
      v306->Type4.TypeSizeInBits = 16;
      v306->Type4.NumObjects = 1;
#endif
      /* Because of nothing leftnot */
      v306->Arg4 = (Nwaitingmsgs);
#if Fastassign
      v306->Type5.Lng = 02000210001;
#else
      v306->Type5.InLine = TRUE;
      v306->Type5.Deallocate = FALSE;
      v306->Type5.LongForm = FALSE;
      v306->Type5.TypeName = TypeInt16;
      v306->Type5.TypeSizeInBits = 16;
      v306->Type5.NumObjects = 1;
#endif
      /* Because of nothing leftnot */
      v306->Arg5 = (Ewaitingmsgs);
#if Fastassign
      v306->Type6.Lng = 02000220006;
#else
      v306->Type6.InLine = TRUE;
      v306->Type6.Deallocate = FALSE;
      v306->Type6.LongForm = FALSE;
      v306->Type6.TypeName = TypePt;
      v306->Type6.TypeSizeInBits = 32;
      v306->Type6.NumObjects = 1;
#endif
      /* Because of nothing leftnot */
      v306->Arg6 = (Portright);
#if Fastassign
      v306->Type7.Lng = 02000210001;
#else
      v306->Type7.InLine = TRUE;
      v306->Type7.Deallocate = FALSE;
      v306->Type7.LongForm = FALSE;
      v306->Type7.TypeName = TypeInt16;
      v306->Type7.TypeSizeInBits = 16;
      v306->Type7.NumObjects = 1;
#endif
      /* Because of nothing leftnot */
      v306->Arg7 = (Porttype);
   }
}



/* function xgetPortstatus (inp, rep) */
XGetPortStatus (Inp, Rep)
ptr_ Inp, Rep;

{
   struct MyMessage
        {
           msg_	Head;
           type_	Type2;
           port_	Arg2;
        };

   struct RepMessage
        {
           msg_	Head;
           type_	RetCodeType;
           short	RetCode;
           type_	Type3;
           short	Arg3;
           type_	Type4;
           short	Arg4;
           type_	Type5;
           short	Arg5;
           type_	Type6;
           long Arg6;
           type_	Type7;
           short	Arg7;
        };
   typedef struct MyMessage *ptrMyMsg;
   typedef struct RepMessage *ptrRepMsg;
   ptrMyMsg MyMsgp;
   ptrRepMsg Repmsgp;

   port_ ServPort;
   port_ Portright;
   short Backlog;
   short Nwaitingmsgs;
   short Ewaitingmsgs;
   long Portindex;
   short Porttype;


   MyMsgp = (ptrMyMsg )(Inp);
   Repmsgp = (ptrRepMsg )(Rep);
   Repmsgp->RetCode = SUCCESS;
   {
      register msg_	 *v322 = &Repmsgp->Head;

      v322->SimpleMsg = TRUE;
      v322->MsgSize = sizeof (struct RepMessage);
   }
   {
      register struct MyMessage *v324 = MyMsgp;

      ServPort = (v324->Head.RemotePort);
#if Typecheck
      if (v324->Type2.TypeName != TypePt)
         {
            Repmsgp->RetCode = WRONG_ARGS;
            return;
         }
#endif
      Portright = (v324->Arg2);
   }

   Repmsgp->RetCode = GetPortStatus(ServPort, Portright, Backlog, Nwaitingmsgs, Ewaitingmsgs, Portindex, Porttype);
   {
      register struct RepMessage *v325 = Repmsgp;

#if Fastassign
      v325->Type3.Lng = 02000210001;
#else
      v325->Type3.InLine = TRUE;
      v325->Type3.Deallocate = FALSE;
      v325->Type3.LongForm = FALSE;
      v325->Type3.TypeName = TypeInt16;
      v325->Type3.TypeSizeInBits = 16;
      v325->Type3.NumObjects = 1;
#endif
      /* Because of nothing leftnot */
      v325->Arg3 = (Backlog);
#if Fastassign
      v325->Type4.Lng = 02000210001;
#else
      v325->Type4.InLine = TRUE;
      v325->Type4.Deallocate = FALSE;
      v325->Type4.LongForm = FALSE;
      v325->Type4.TypeName = TypeInt16;
      v325->Type4.TypeSizeInBits = 16;
      v325->Type4.NumObjects = 1;
#endif
      /* Because of nothing leftnot */
      v325->Arg4 = (Nwaitingmsgs);
#if Fastassign
      v325->Type5.Lng = 02000210001;
#else
      v325->Type5.InLine = TRUE;
      v325->Type5.Deallocate = FALSE;
      v325->Type5.LongForm = FALSE;
      v325->Type5.TypeName = TypeInt16;
      v325->Type5.TypeSizeInBits = 16;
      v325->Type5.NumObjects = 1;
#endif
      /* Because of nothing leftnot */
      v325->Arg5 = (Ewaitingmsgs);
#if Fastassign
      v325->Type6.Lng = 02000220002;
#else
      v325->Type6.InLine = TRUE;
      v325->Type6.Deallocate = FALSE;
      v325->Type6.LongForm = FALSE;
      v325->Type6.TypeName = TypeInt32;
      v325->Type6.TypeSizeInBits = 32;
      v325->Type6.NumObjects = 1;
#endif
      /* Because of nothing leftnot */
      v325->Arg6 = (Portindex);
#if Fastassign
      v325->Type7.Lng = 02000210001;
#else
      v325->Type7.InLine = TRUE;
      v325->Type7.Deallocate = FALSE;
      v325->Type7.LongForm = FALSE;
      v325->Type7.TypeName = TypeInt16;
      v325->Type7.TypeSizeInBits = 16;
      v325->Type7.NumObjects = 1;
#endif
      /* Because of nothing leftnot */
      v325->Arg7 = (Porttype);
   }
}



/* function xextractallrights (inp, rep) */
XExtractAllRights (Inp, Rep)
ptr_ Inp, Rep;

{
   struct MyMessage
        {
           msg_	Head;
           type_	Type2;
           long Arg2;
        };

   struct RepMessage
        {
           msg_	Head;
           type_	RetCodeType;
           short	RetCode;
           type_	Type3;
           port_	Arg3;
           type_	Type4;
           short	Arg4;
        };
   typedef struct MyMessage *ptrMyMsg;
   typedef struct RepMessage *ptrRepMsg;
   ptrMyMsg MyMsgp;
   ptrRepMsg Repmsgp;

   port_ ServPort;
   long Portindex;
   port_ Portright;
   short Porttype;


   MyMsgp = (ptrMyMsg )(Inp);
   Repmsgp = (ptrRepMsg )(Rep);
   Repmsgp->RetCode = SUCCESS;
   {
      register msg_	 *v341 = &Repmsgp->Head;

      v341->SimpleMsg = FALSE;
      v341->MsgSize = sizeof (struct RepMessage);
   }
   {
      register struct MyMessage *v343 = MyMsgp;

      ServPort = (v343->Head.RemotePort);
#if Typecheck
      if (v343->Type2.TypeName != TypeInt32)
         {
            Repmsgp->RetCode = WRONG_ARGS;
            return;
         }
#endif
      Portindex = (v343->Arg2);
   }

   Repmsgp->RetCode = ExtractAllRights(ServPort, Portindex, Portright, Porttype);
   {
      register struct RepMessage *v344 = Repmsgp;

#if Fastassign
      v344->Type3.Lng = 02000220000 + Porttype; /** Hand editednot RFR/DBG **/
#else
      v344->Type3.InLine = TRUE;
      v344->Type3.Deallocate = FALSE;
      v344->Type3.LongForm = FALSE;
      v344->Type3.TypeName = Porttype; /** Hand editednot RFR/DBG **/
      v344->Type3.TypeSizeInBits = 32;
      v344->Type3.NumObjects = 1;
#endif
      /* Because of nothing leftnot */
      v344->Arg3 = (Portright);
#if Fastassign
      v344->Type4.Lng = 02000210001;
#else
      v344->Type4.InLine = TRUE;
      v344->Type4.Deallocate = FALSE;
      v344->Type4.LongForm = FALSE;
      v344->Type4.TypeName = TypeInt16;
      v344->Type4.TypeSizeInBits = 16;
      v344->Type4.NumObjects = 1;
#endif
      /* Because of nothing leftnot */
      v344->Arg4 = (Porttype);
   }
}



/* function xinsertallrights (inp, rep) */
XInsertAllRights (Inp, Rep)
ptr_ Inp, Rep;

{
   struct MyMessage
        {
           msg_	Head;
           type_	Type2;
           long Arg2;
           type_	Type3;
           port_	Arg3;
           type_	Type4;
           short	Arg4;
        };

   struct RepMessage
        {
           msg_	Head;
           type_	RetCodeType;
           short	RetCode;
        };
   typedef struct MyMessage *ptrMyMsg;
   typedef struct RepMessage *ptrRepMsg;
   ptrMyMsg MyMsgp;
   ptrRepMsg Repmsgp;

   port_ ServPort;
   long Portindex;
   port_ Portright;
   short Porttype;


   MyMsgp = (ptrMyMsg )(Inp);
   Repmsgp = (ptrRepMsg )(Rep);
   Repmsgp->RetCode = SUCCESS;
   {
      register msg_	 *v351 = &Repmsgp->Head;

      v351->SimpleMsg = TRUE;
      v351->MsgSize = sizeof (struct RepMessage);
   }
   {
      register struct MyMessage *v353 = MyMsgp;

      ServPort = (v353->Head.RemotePort);
#if Typecheck
      if (v353->Type2.TypeName != TypeInt32)
         {
            Repmsgp->RetCode = WRONG_ARGS;
            return;
         }
#endif
      Portindex = (v353->Arg2);
#if Typecheck
      /** Hand edited to remove type checknot RFR / DBG **/
#endif
      Portright = (v353->Arg3);
#if Typecheck
      if (v353->Type4.TypeName != TypeInt16)
         {
            Repmsgp->RetCode = WRONG_ARGS;
            return;
         }
#endif
      Porttype = (v353->Arg4);
   }

   Repmsgp->RetCode = InsertAllRights(ServPort, Portindex, Portright, Porttype);
   {
      register struct RepMessage *v354 = Repmsgp;

   }
}



/* function xcreateprocess (inp, rep) */
XCreateProcess (Inp, Rep)
ptr_ Inp, Rep;

{
   struct MyMessage
        {
           msg_	Head;
        };

   struct RepMessage
        {
           msg_	Head;
           type_	RetCodeType;
           short	RetCode;
           type_	Type2;
           port_	Arg2;
           type_	Type3;
           port_	Arg3;
        };
   typedef struct MyMessage *ptrMyMsg;
   typedef struct RepMessage *ptrRepMsg;
   ptrMyMsg MyMsgp;
   ptrRepMsg Repmsgp;

   port_ ServPort;
   port_ HiskernelPort;
   port_ HisdataPort;


   MyMsgp = (ptrMyMsg )(Inp);
   Repmsgp = (ptrRepMsg )(Rep);
   Repmsgp->RetCode = SUCCESS;
   {
      register msg_	 *v355 = &Repmsgp->Head;

      v355->SimpleMsg = FALSE;
      v355->MsgSize = sizeof (struct RepMessage);
   }
   {
      register struct MyMessage *v357 = MyMsgp;

      ServPort = (v357->Head.RemotePort);
   }

   Repmsgp->RetCode = CreateProcess(ServPort, HiskernelPort, HisdataPort);
   {
      register struct RepMessage *v358 = Repmsgp;

#if Fastassign
      v358->Type2.Lng = 02000220006;
#else
      v358->Type2.InLine = TRUE;
      v358->Type2.Deallocate = FALSE;
      v358->Type2.LongForm = FALSE;
      v358->Type2.TypeName = TypePt;
      v358->Type2.TypeSizeInBits = 32;
      v358->Type2.NumObjects = 1;
#endif
      /* Because of nothing leftnot */
      v358->Arg2 = (HiskernelPort);
#if Fastassign
      v358->Type3.Lng = 02000220006;
#else
      v358->Type3.InLine = TRUE;
      v358->Type3.Deallocate = FALSE;
      v358->Type3.LongForm = FALSE;
      v358->Type3.TypeName = TypePt;
      v358->Type3.TypeSizeInBits = 32;
      v358->Type3.NumObjects = 1;
#endif
      /* Because of nothing leftnot */
      v358->Arg3 = (HisdataPort);
   }
}



/* function xinterceptsegmentcalls (inp, rep) */
XInterceptSegmentCalls (Inp, Rep)
ptr_ Inp, Rep;

{
   struct MyMessage
        {
           msg_	Head;
        };

   struct RepMessage
        {
           msg_	Head;
           type_	RetCodeType;
           short	RetCode;
           type_	Type2;
           short	Tname2;
           short	Tsize2;
           long Numelts2;
           ptrAllPortArray Arg2;
           type_	Type3;
           short	Tname3;
           short	Tsize3;
           long Numelts3;
           ptrPortArray Arg3;
        };
   typedef struct MyMessage *ptrMyMsg;
   typedef struct RepMessage *ptrRepMsg;
   ptrMyMsg MyMsgp;
   ptrRepMsg Repmsgp;

   port_ ServPort;
   ptrAllPortArray OldsysPorts;
   long OldsysPorts_cnt;
   ptrPortArray SysPorts;
   long SysPorts_cnt;


   MyMsgp = (ptrMyMsg )(Inp);
   Repmsgp = (ptrRepMsg )(Rep);
   Repmsgp->RetCode = SUCCESS;
   {
      register msg_	 *v365 = &Repmsgp->Head;

      v365->SimpleMsg = FALSE;
      v365->MsgSize = sizeof (struct RepMessage);
   }
   {
      register struct MyMessage *v367 = MyMsgp;

      ServPort = (v367->Head.RemotePort);
   }

   Repmsgp->RetCode = InterceptSegmentCalls(ServPort, OldsysPorts, OldsysPorts_cnt, SysPorts, SysPorts_cnt);
   {
      register struct RepMessage *v368 = Repmsgp;

#if Fastassign
      v368->Type2.Lng = 04000000000;
#else
      v368->Type2.InLine = FALSE;
      v368->Type2.Deallocate = FALSE;
      v368->Type2.LongForm = TRUE;
#endif
      /* Because of Long form*/
      v368->Tname2 = TypePtAll;
      v368->Tsize2 = 32;
      v368->Numelts2 = OldsysPorts_cnt;
      v368->Arg2 = (OldsysPorts);
#if Fastassign
      v368->Type3.Lng = 04000000000;
#else
      v368->Type3.InLine = FALSE;
      v368->Type3.Deallocate = FALSE;
      v368->Type3.LongForm = TRUE;
#endif
      /* Because of Long form*/
      v368->Tname3 = TypePt;
      v368->Tsize3 = 32;
      v368->Numelts3 = SysPorts_cnt;
      v368->Arg3 = (SysPorts);
   }
}



/* function xdirectio (inp, rep) */
XDirectIO (Inp, Rep)
ptr_ Inp, Rep;

{
   struct MyMessage
        {
           msg_	Head;
           type_	Type2;
           short	Tname2;
           short	Tsize2;
           long Numelts2;
           ptr_ Arg2;
           type_	Type3;
           short	Tname3;
           short	Tsize3;
           long Numelts3;
           ptr_ Arg3;
           type_	Type4;
           short	Tname4;
           short	Tsize4;
           long Numelts4;
           ptr_ Arg4;
        };

   struct RepMessage
        {
           msg_	Head;
           type_	RetCodeType;
           short	RetCode;
           type_	Type2;
           short	Tname2;
           short	Tsize2;
           long Numelts2;
           ptr_ Arg2;
           type_	Type3;
           short	Tname3;
           short	Tsize3;
           long Numelts3;
           ptr_ Arg3;
           type_	Type4;
           short	Tname4;
           short	Tsize4;
           long Numelts4;
           ptr_ Arg4;
        };
   typedef struct MyMessage *ptrMyMsg;
   typedef struct RepMessage *ptrRepMsg;
   ptrMyMsg MyMsgp;
   ptrRepMsg Repmsgp;

   port_ ServPort;
   ptr_ Cmdblk;
   long Cmdblk_cnt;
   ptr_ Datahdr;
   long Datahdr_cnt;
   ptr_ Data;
   long Data_cnt;


   MyMsgp = (ptrMyMsg )(Inp);
   Repmsgp = (ptrRepMsg )(Rep);
   Repmsgp->RetCode = SUCCESS;
   {
      register msg_	 *v373 = &Repmsgp->Head;

      v373->SimpleMsg = FALSE;
      v373->MsgSize = sizeof (struct RepMessage);
   }
   {
      register struct MyMessage *v375 = MyMsgp;

      ServPort = (v375->Head.RemotePort);
#if Typecheck
      if (v375->Tname2 != TypeInt8)
         {
            Repmsgp->RetCode = WRONG_ARGS;
            return;
         }
#endif
      Cmdblk_cnt = v375->Numelts2;
      Cmdblk = (v375->Arg2);
#if Typecheck
      if (v375->Tname3 != TypeInt8)
         {
            Repmsgp->RetCode = WRONG_ARGS;
            return;
         }
#endif
      Datahdr_cnt = v375->Numelts3;
      Datahdr = (v375->Arg3);
#if Typecheck
      if (v375->Tname4 != TypeInt8)
         {
            Repmsgp->RetCode = WRONG_ARGS;
            return;
         }
#endif
      Data_cnt = v375->Numelts4;
      Data = (v375->Arg4);
   }

   Repmsgp->RetCode = DirectIO(ServPort, Cmdblk, Cmdblk_cnt, Datahdr, Datahdr_cnt, Data, Data_cnt);
   {
      register struct RepMessage *v376 = Repmsgp;

#if Fastassign
      v376->Type2.Lng = 04000000000;
#else
      v376->Type2.InLine = FALSE;
      v376->Type2.Deallocate = FALSE;
      v376->Type2.LongForm = TRUE;
#endif
      /* Because of Long form*/
      v376->Tname2 = TypeInt8;
      v376->Tsize2 = 8;
      v376->Numelts2 = Cmdblk_cnt;
      v376->Arg2 = (Cmdblk);
#if Fastassign
      v376->Type3.Lng = 04000000000;
#else
      v376->Type3.InLine = FALSE;
      v376->Type3.Deallocate = FALSE;
      v376->Type3.LongForm = TRUE;
#endif
      /* Because of Long form*/
      v376->Tname3 = TypeInt8;
      v376->Tsize3 = 8;
      v376->Numelts3 = Datahdr_cnt;
      v376->Arg3 = (Datahdr);
#if Fastassign
      v376->Type4.Lng = 04000000000;
#else
      v376->Type4.InLine = FALSE;
      v376->Type4.Deallocate = FALSE;
      v376->Type4.LongForm = TRUE;
#endif
      /* Because of Long form*/
      v376->Tname4 = TypeInt8;
      v376->Tsize4 = 8;
      v376->Numelts4 = Data_cnt;
      v376->Arg4 = (Data);
   }
}



/* function xsetpagingsegment (inp, rep) */
XSetPagingSegment (Inp, Rep)
ptr_ Inp, Rep;

{
   struct MyMessage
        {
           msg_	Head;
           type_	Type2;
           Segid Arg2;
        };

   struct RepMessage
        {
           msg_	Head;
           type_	RetCodeType;
           short	RetCode;
        };
   typedef struct MyMessage *ptrMyMsg;
   typedef struct RepMessage *ptrRepMsg;
   ptrMyMsg MyMsgp;
   ptrRepMsg Repmsgp;

   port_ ServPort;
   Segid Segment;


   MyMsgp = (ptrMyMsg )(Inp);
   Repmsgp = (ptrRepMsg )(Rep);
   Repmsgp->RetCode = SUCCESS;
   {
      register msg_	 *v383 = &Repmsgp->Head;

      v383->SimpleMsg = TRUE;
      v383->MsgSize = sizeof (struct RepMessage);
   }
   {
      register struct MyMessage *v385 = MyMsgp;

      ServPort = (v385->Head.RemotePort);
#if Typecheck
      if (v385->Type2.TypeName != TypeInt32)
         {
            Repmsgp->RetCode = WRONG_ARGS;
            return;
         }
#endif
      Segment = (v385->Arg2);
   }

   Repmsgp->RetCode = SetPagingSegment(ServPort, Segment);
   {
      register struct RepMessage *v386 = Repmsgp;

   }
}



/* function xcreaterectangle (inp, rep) */
XCreateRectangle (Inp, Rep)
ptr_ Inp, Rep;

{
   struct MyMessage
        {
           msg_	Head;
           type_	Type2;
           port_	Arg2;
           type_	Type3;
           ptr_ Arg3;
           type_	Type4;
           short	Arg4;
           type_	Type5;
           short	Arg5;
           type_	Type6;
           short	Arg6;
           type_	Type7;
           short	Arg7;
           type_	Type8;
           short	Arg8;
           type_	Type9;
           long_bool Arg9;
        };

   struct RepMessage
        {
           msg_	Head;
           type_	RetCodeType;
           short	RetCode;
        };
   typedef struct MyMessage *ptrMyMsg;
   typedef struct RepMessage *ptrRepMsg;
   ptrMyMsg MyMsgp;
   ptrRepMsg Repmsgp;

   port_ ServPort;
   port_ RectPort;
   ptr_ Baseaddr;
   short Scanwidth;
   short Basex;
   short Basey;
   short Maxx;
   short Maxy;
   long_bool Isfont;


   MyMsgp = (ptrMyMsg )(Inp);
   Repmsgp = (ptrRepMsg )(Rep);
   Repmsgp->RetCode = SUCCESS;
   {
      register msg_	 *v387 = &Repmsgp->Head;

      v387->SimpleMsg = TRUE;
      v387->MsgSize = sizeof (struct RepMessage);
   }
   {
      register struct MyMessage *v389 = MyMsgp;

      ServPort = (v389->Head.RemotePort);
#if Typecheck
      if (v389->Type2.TypeName != TypePt)
         {
            Repmsgp->RetCode = WRONG_ARGS;
            return;
         }
#endif
      RectPort = (v389->Arg2);
#if Typecheck
      if (v389->Type3.TypeName != TypeInt32)
         {
            Repmsgp->RetCode = WRONG_ARGS;
            return;
         }
#endif
      Baseaddr = (v389->Arg3);
#if Typecheck
      if (v389->Type4.TypeName != TypeInt16)
         {
            Repmsgp->RetCode = WRONG_ARGS;
            return;
         }
#endif
      Scanwidth = (v389->Arg4);
#if Typecheck
      if (v389->Type5.TypeName != TypeInt16)
         {
            Repmsgp->RetCode = WRONG_ARGS;
            return;
         }
#endif
      Basex = (v389->Arg5);
#if Typecheck
      if (v389->Type6.TypeName != TypeInt16)
         {
            Repmsgp->RetCode = WRONG_ARGS;
            return;
         }
#endif
      Basey = (v389->Arg6);
#if Typecheck
      if (v389->Type7.TypeName != TypeInt16)
         {
            Repmsgp->RetCode = WRONG_ARGS;
            return;
         }
#endif
      Maxx = (v389->Arg7);
#if Typecheck
      if (v389->Type8.TypeName != TypeInt16)
         {
            Repmsgp->RetCode = WRONG_ARGS;
            return;
         }
#endif
      Maxy = (v389->Arg8);
#if Typecheck
      if (v389->Type9.TypeName != TypeBoolean)
         {
            Repmsgp->RetCode = WRONG_ARGS;
            return;
         }
#endif
      Isfont = (v389->Arg9);
   }

   Repmsgp->RetCode = CreateRectangle(ServPort, RectPort, Baseaddr, Scanwidth, Basex, Basey, Maxx, Maxy, Isfont);
   {
      register struct RepMessage *v390 = Repmsgp;

   }
}



/* function xdestroyrectangle (inp, rep) */
XDestroyRectangle (Inp, Rep)
ptr_ Inp, Rep;

{
   struct MyMessage
        {
           msg_	Head;
           type_	Type2;
           port_	Arg2;
        };

   struct RepMessage
        {
           msg_	Head;
           type_	RetCodeType;
           short	RetCode;
        };
   typedef struct MyMessage *ptrMyMsg;
   typedef struct RepMessage *ptrRepMsg;
   ptrMyMsg MyMsgp;
   ptrRepMsg Repmsgp;

   port_ ServPort;
   port_ RectPort;


   MyMsgp = (ptrMyMsg )(Inp);
   Repmsgp = (ptrRepMsg )(Rep);
   Repmsgp->RetCode = SUCCESS;
   {
      register msg_	 *v391 = &Repmsgp->Head;

      v391->SimpleMsg = TRUE;
      v391->MsgSize = sizeof (struct RepMessage);
   }
   {
      register struct MyMessage *v393 = MyMsgp;

      ServPort = (v393->Head.RemotePort);
#if Typecheck
      if (v393->Type2.TypeName != TypePt)
         {
            Repmsgp->RetCode = WRONG_ARGS;
            return;
         }
#endif
      RectPort = (v393->Arg2);
   }

   Repmsgp->RetCode = DestroyRectangle(ServPort, RectPort);
   {
      register struct RepMessage *v394 = Repmsgp;

   }
}



/* function xavailablevm (inp, rep) */
XAvailableVM (Inp, Rep)
ptr_ Inp, Rep;

{
   struct MyMessage
        {
           msg_	Head;
        };

   struct RepMessage
        {
           msg_	Head;
           type_	RetCodeType;
           short	RetCode;
           type_	Type2;
           long Arg2;
        };
   typedef struct MyMessage *ptrMyMsg;
   typedef struct RepMessage *ptrRepMsg;
   ptrMyMsg MyMsgp;
   ptrRepMsg Repmsgp;

   port_ ServPort;
   long Numbytes;


   MyMsgp = (ptrMyMsg )(Inp);
   Repmsgp = (ptrRepMsg )(Rep);
   Repmsgp->RetCode = SUCCESS;
   {
      register msg_	 *v395 = &Repmsgp->Head;

      v395->SimpleMsg = TRUE;
      v395->MsgSize = sizeof (struct RepMessage);
   }
   {
      register struct MyMessage *v397 = MyMsgp;

      ServPort = (v397->Head.RemotePort);
   }

   Repmsgp->RetCode = AvailableVM(ServPort, Numbytes);
   {
      register struct RepMessage *v398 = Repmsgp;

#if Fastassign
      v398->Type2.Lng = 02000220002;
#else
      v398->Type2.InLine = TRUE;
      v398->Type2.Deallocate = FALSE;
      v398->Type2.LongForm = FALSE;
      v398->Type2.TypeName = TypeInt32;
      v398->Type2.TypeSizeInBits = 32;
      v398->Type2.NumObjects = 1;
#endif
      /* Because of nothing leftnot */
      v398->Arg2 = (Numbytes);
   }
}



/* function xenablerectangles (inp, rep) */
XEnableRectangles (Inp, Rep)
ptr_ Inp, Rep;

{
   struct MyMessage
        {
           msg_	Head;
           type_	Type2;
           short	Tname2;
           short	Tsize2;
           long Numelts2;
           ptrPortArray Arg2;
           type_	Type3;
           long_bool Arg3;
        };

   struct RepMessage
        {
           msg_	Head;
           type_	RetCodeType;
           short	RetCode;
        };
   typedef struct MyMessage *ptrMyMsg;
   typedef struct RepMessage *ptrRepMsg;
   ptrMyMsg MyMsgp;
   ptrRepMsg Repmsgp;

   port_ ServPort;
   ptrPortArray Rectlist;
   long Rectlist_cnt;
   long_bool Enable;


   MyMsgp = (ptrMyMsg )(Inp);
   Repmsgp = (ptrRepMsg )(Rep);
   Repmsgp->RetCode = SUCCESS;
   {
      register msg_	 *v402 = &Repmsgp->Head;

      v402->SimpleMsg = TRUE;
      v402->MsgSize = sizeof (struct RepMessage);
   }
   {
      register struct MyMessage *v404 = MyMsgp;

      ServPort = (v404->Head.RemotePort);
#if Typecheck
      if (v404->Tname2 != TypePt)
         {
            Repmsgp->RetCode = WRONG_ARGS;
            return;
         }
#endif
      Rectlist_cnt = v404->Numelts2;
      Rectlist = (v404->Arg2);
#if Typecheck
      if (v404->Type3.TypeName != TypeBoolean)
         {
            Repmsgp->RetCode = WRONG_ARGS;
            return;
         }
#endif
      Enable = (v404->Arg3);
   }

   Repmsgp->RetCode = EnableRectangles(ServPort, Rectlist, Rectlist_cnt, Enable);
   {
      register struct RepMessage *v405 = Repmsgp;

   }
}



/* function xsetkernelwindow (inp, rep) */
XSetKernelWindow (Inp, Rep)
ptr_ Inp, Rep;

{
   struct MyMessage
        {
           msg_	Head;
           type_	Type2;
           short	Arg2;
           type_	Type3;
           short	Arg3;
           type_	Type4;
           short	Arg4;
           type_	Type5;
           short	Arg5;
           type_	Type6;
           long_bool Arg6;
        };

   struct RepMessage
        {
           msg_	Head;
           type_	RetCodeType;
           short	RetCode;
        };
   typedef struct MyMessage *ptrMyMsg;
   typedef struct RepMessage *ptrRepMsg;
   ptrMyMsg MyMsgp;
   ptrRepMsg Repmsgp;

   port_ ServPort;
   short Leftx;
   short Topy;
   short Width;
   short Height;
   long_bool Inverted;


   MyMsgp = (ptrMyMsg )(Inp);
   Repmsgp = (ptrRepMsg )(Rep);
   Repmsgp->RetCode = SUCCESS;
   {
      register msg_	 *v406 = &Repmsgp->Head;

      v406->SimpleMsg = TRUE;
      v406->MsgSize = sizeof (struct RepMessage);
   }
   {
      register struct MyMessage *v408 = MyMsgp;

      ServPort = (v408->Head.RemotePort);
#if Typecheck
      if (v408->Type2.TypeName != TypeInt16)
         {
            Repmsgp->RetCode = WRONG_ARGS;
            return;
         }
#endif
      Leftx = (v408->Arg2);
#if Typecheck
      if (v408->Type3.TypeName != TypeInt16)
         {
            Repmsgp->RetCode = WRONG_ARGS;
            return;
         }
#endif
      Topy = (v408->Arg3);
#if Typecheck
      if (v408->Type4.TypeName != TypeInt16)
         {
            Repmsgp->RetCode = WRONG_ARGS;
            return;
         }
#endif
      Width = (v408->Arg4);
#if Typecheck
      if (v408->Type5.TypeName != TypeInt16)
         {
            Repmsgp->RetCode = WRONG_ARGS;
            return;
         }
#endif
      Height = (v408->Arg5);
#if Typecheck
      if (v408->Type6.TypeName != TypeBoolean)
         {
            Repmsgp->RetCode = WRONG_ARGS;
            return;
         }
#endif
      Inverted = (v408->Arg6);
   }

   Repmsgp->RetCode = SetKernelWindow(ServPort, Leftx, Topy, Width, Height, Inverted);
   {
      register struct RepMessage *v409 = Repmsgp;

   }
}



/* procedure accintserver (inp, repp) */
long_bool AccIntServer (Inp, Repp)
ptr_ Inp, Repp;
 
/* --------> */
{
   long_bool _AccIntServer;
   struct RepMessage
        {
           msg_	Head;
           type_	RetCodeType;
           short	RetCode;
        };
   typedef struct RepMessage *ptrRepMessage;
   msgp_ Inmsgp;
   ptrRepMessage Repmsgp;




   Inmsgp = (msgp_ )(Inp);
   Repmsgp = (ptrRepMessage )(Repp);
   {
      register msg_	 *v410 = &Repmsgp->Head;

      v410->LocalPort = Inmsgp->RemotePort;
      v410->RemotePort = Inmsgp->LocalPort;
      v410->ID = Inmsgp->ID + (long)(100);
      v410->MsgType = NormalMsg;
   }
   {
      register type_	*v412 = &Repmsgp->RetCodeType;

      v412->TypeName = TypeInt16;
      v412->TypeSizeInBits = 16;
      v412->NumObjects = 1;
      v412->InLine = TRUE;
      v412->LongForm = FALSE;
      v412->Deallocate = FALSE;
   }

   {
      register msg_	 *v415 = Inmsgp;

      if ((v415->ID > (long)(143)) || (v415->ID < (long)(100)))
           {
              _AccIntServer = FALSE;
              Repmsgp->RetCode = BadMsgID;
           }
      else {
              _AccIntServer = TRUE;
              switch((short)(v415->ID - (long)(100)))
                   {
                      case 0:
                              XSetBackLog(Inp, Repp);
                              break;

                      case 1:
                              XAllocatePort(Inp, Repp);
                              break;

                      case 2:
                              XDeallocatePort(Inp, Repp);
                              break;

                      case 3:
                              XIndexInterpose(Inp, Repp);
                              break;

                      case 4:
                              XPortInterpose(Inp, Repp);
                              break;

                      case 5:
                              XFork(Inp, Repp);
                              break;

                      case 6:
                              XStatus(Inp, Repp);
                              break;

                      case 7:
                              XTerminate(Inp, Repp);
                              break;

                      case 8:
                              XSetPriority(Inp, Repp);
                              break;

                      case 9:
                              XSetLimit(Inp, Repp);
                              break;

                      case 10:
                              XSuspend(Inp, Repp);
                              break;

                      case 11:
                              XRESUME(Inp, Repp);
                              break;

                      case 12:
                              XExamine(Inp, Repp);
                              break;

                      case 13:
                              XDeposit(Inp, Repp);
                              break;

                      case 14:
                              XSoftInterrupt(Inp, Repp);
                              break;

                      case 15:
                              XCreateSegment(Inp, Repp);
                              break;

                      case 16:
                              XTruncateSegment(Inp, Repp);
                              break;

                      case 17:
                              XDestroySegment(Inp, Repp);
                              break;

                      case 18:
                              XReadSegment(Inp, Repp);
                              break;

                      case 19:
                              XWriteSegment(Inp, Repp);
                              break;

                      case 20:
                              XValidateMemory(Inp, Repp);
                              break;

                      case 21:
                              XInvalidateMemory(Inp, Repp);
                              break;

                      case 22:
                              XSetProtection(Inp, Repp);
                              break;

                      case 23:
                              XReadProcessMemory(Inp, Repp);
                              break;

                      case 24:
                              XWriteProcessMemory(Inp, Repp);
                              break;

                      case 25:
                              XGetDiskPartitions(Inp, Repp);
                              break;

                      case 26:
                              XPartMount(Inp, Repp);
                              break;

                      case 27:
                              XPartDismount(Inp, Repp);
                              break;

                      case 28:
                              XSetTempSegPartition(Inp, Repp);
                              break;

                      case 29:
                              XSetDebugPort(Inp, Repp);
                              break;

                      case 30:
                              XTouch(Inp, Repp);
                              break;

                      case 31:
                              XGetPortIndexStatus(Inp, Repp);
                              break;

                      case 32:
                              XGetPortStatus(Inp, Repp);
                              break;

                      case 33:
                              XExtractAllRights(Inp, Repp);
                              break;

                      case 34:
                              XInsertAllRights(Inp, Repp);
                              break;

                      case 35:
                              XCreateProcess(Inp, Repp);
                              break;

                      case 36:
                              XInterceptSegmentCalls(Inp, Repp);
                              break;

                      case 37:
                              XDirectIO(Inp, Repp);
                              break;

                      case 38:
                              XSetPagingSegment(Inp, Repp);
                              break;

                      case 39:
                              XCreateRectangle(Inp, Repp);
                              break;

                      case 40:
                              XDestroyRectangle(Inp, Repp);
                              break;

                      case 41:
                              XAvailableVM(Inp, Repp);
                              break;

                      case 42:
                              XEnableRectangles(Inp, Repp);
                              break;

                      case 43:
                              XSetKernelWindow(Inp, Repp);
                              break;

                   }
           }
   }
   return(_AccIntServer);
}

