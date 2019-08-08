/* 
 * Module mach_server
 */
#include 	"../accent/accenttype.h"

#include "../kern/mach_if.h"
#include "../accent/accent.h"

 static void Xport_allocate(InP, ReP) caddr_t InP, ReP;
 { 

/* Define local messages and types */

 typedef  struct { 
Msg	Head;
 } MyMessage;

 typedef  struct { 
Msg	Head;
TypeType	RetCodeType;
 Integer 	RetCode;
TypeType IPCNam2;
port_t	Arg2;
TypeType IPCNam3;
kern_return_t	Arg3;
 } RepMessage;
 typedef MyMessage * ptrMyMsg;
 typedef RepMessage * ptrRepMsg;
ptrMyMsg MyMsgP;
ptrRepMsg RepMsgP;

task_t	ServPort;
port_t	my_port;
 extern kern_return_t	port_allocate ( );


MyMsgP = (ptrMyMsg) (InP);
RepMsgP = (ptrRepMsg) (ReP);
RepMsgP->RetCode = Success;
RepMsgP->Head.SimpleMsg = FALSE;
RepMsgP->Head.MsgSize =  sizeof (RepMessage);
ServPort = convert_port_to_task(MyMsgP->Head.RemotePort);

/* Extract function arguments from message */


RepMsgP->Arg3 = (
/* Call service routine */

port_allocate (ServPort,  & my_port));

/* Gather reply arguments */

#if FastAssign
	RepMsgP->IPCNam2 = 02000220006L;
#else FastAssign
	RepMsgP->IPCNam2 =        0;
	RepMsgP->IPCNam2 |= _InLine(  TRUE);
	RepMsgP->IPCNam2 |= _Deallocate( FALSE);
	RepMsgP->IPCNam2 |= _LongForm( FALSE);
	RepMsgP->IPCNam2 |= _TypeName(TypePt);
	RepMsgP->IPCNam2 |= _TypeSizeInBits(32);
	RepMsgP->IPCNam2 |= _NumObjects(1);
#endif FastAssign
	RepMsgP->Arg2 = (my_port);
#if FastAssign
	RepMsgP->IPCNam3 = 02000220002L;
#else FastAssign
	RepMsgP->IPCNam3 =        0;
	RepMsgP->IPCNam3 |= _InLine(  TRUE);
	RepMsgP->IPCNam3 |= _Deallocate( FALSE);
	RepMsgP->IPCNam3 |= _LongForm( FALSE);
	RepMsgP->IPCNam3 |= _TypeName(TypeInt32);
	RepMsgP->IPCNam3 |= _TypeSizeInBits(32);
	RepMsgP->IPCNam3 |= _NumObjects(1);
#endif FastAssign
 } 
 static void Xport_deallocate(InP, ReP) caddr_t InP, ReP;
 { 

/* Define local messages and types */

 typedef  struct { 
Msg	Head;
TypeType IPCNam2;
port_t	Arg2;
TypeType IPCNam3;
int	Arg3;
 } MyMessage;

 typedef  struct { 
Msg	Head;
TypeType	RetCodeType;
 Integer 	RetCode;
TypeType IPCNam4;
kern_return_t	Arg4;
 } RepMessage;
 typedef MyMessage * ptrMyMsg;
 typedef RepMessage * ptrRepMsg;
ptrMyMsg MyMsgP;
ptrRepMsg RepMsgP;

task_t	ServPort;
port_t	my_port;
int	reason;
 extern kern_return_t	port_deallocate ( );


MyMsgP = (ptrMyMsg) (InP);
RepMsgP = (ptrRepMsg) (ReP);
RepMsgP->RetCode = Success;
RepMsgP->Head.SimpleMsg = TRUE;
RepMsgP->Head.MsgSize =  sizeof (RepMessage);
ServPort = convert_port_to_task(MyMsgP->Head.RemotePort);

/* Extract function arguments from message */


#if TypeCheck
	 if (_X_TypeName(MyMsgP->IPCNam2) != TypePt)
		 { 
		RepMsg.RetCode = WRONGARGS;
		 return ;
		 } 
#endif TypeCheck
	my_port = (MyMsgP->Arg2);

#if TypeCheck
	 if (_X_TypeName(MyMsgP->IPCNam3) != TypeInt32)
		 { 
		RepMsg.RetCode = WRONGARGS;
		 return ;
		 } 
#endif TypeCheck
	reason = (MyMsgP->Arg3);

RepMsgP->Arg4 = (
/* Call service routine */

port_deallocate (ServPort, my_port, reason));

/* Gather reply arguments */

#if FastAssign
	RepMsgP->IPCNam4 = 02000220002L;
#else FastAssign
	RepMsgP->IPCNam4 =        0;
	RepMsgP->IPCNam4 |= _InLine(  TRUE);
	RepMsgP->IPCNam4 |= _Deallocate( FALSE);
	RepMsgP->IPCNam4 |= _LongForm( FALSE);
	RepMsgP->IPCNam4 |= _TypeName(TypeInt32);
	RepMsgP->IPCNam4 |= _TypeSizeInBits(32);
	RepMsgP->IPCNam4 |= _NumObjects(1);
#endif FastAssign
 } 
 static void Xport_enable(InP, ReP) caddr_t InP, ReP;
 { 

/* Define local messages and types */

 typedef  struct { 
Msg	Head;
TypeType IPCNam2;
port_t	Arg2;
 } MyMessage;

 typedef  struct { 
Msg	Head;
TypeType	RetCodeType;
 Integer 	RetCode;
TypeType IPCNam3;
kern_return_t	Arg3;
 } RepMessage;
 typedef MyMessage * ptrMyMsg;
 typedef RepMessage * ptrRepMsg;
ptrMyMsg MyMsgP;
ptrRepMsg RepMsgP;

task_t	ServPort;
port_t	my_port;
 extern kern_return_t	port_enable ( );


MyMsgP = (ptrMyMsg) (InP);
RepMsgP = (ptrRepMsg) (ReP);
RepMsgP->RetCode = Success;
RepMsgP->Head.SimpleMsg = TRUE;
RepMsgP->Head.MsgSize =  sizeof (RepMessage);
ServPort = convert_port_to_task(MyMsgP->Head.RemotePort);

/* Extract function arguments from message */


#if TypeCheck
	 if (_X_TypeName(MyMsgP->IPCNam2) != TypePt)
		 { 
		RepMsg.RetCode = WRONGARGS;
		 return ;
		 } 
#endif TypeCheck
	my_port = (MyMsgP->Arg2);

RepMsgP->Arg3 = (
/* Call service routine */

port_enable (ServPort, my_port));

/* Gather reply arguments */

#if FastAssign
	RepMsgP->IPCNam3 = 02000220002L;
#else FastAssign
	RepMsgP->IPCNam3 =        0;
	RepMsgP->IPCNam3 |= _InLine(  TRUE);
	RepMsgP->IPCNam3 |= _Deallocate( FALSE);
	RepMsgP->IPCNam3 |= _LongForm( FALSE);
	RepMsgP->IPCNam3 |= _TypeName(TypeInt32);
	RepMsgP->IPCNam3 |= _TypeSizeInBits(32);
	RepMsgP->IPCNam3 |= _NumObjects(1);
#endif FastAssign
 } 
 static void Xport_disable(InP, ReP) caddr_t InP, ReP;
 { 

/* Define local messages and types */

 typedef  struct { 
Msg	Head;
TypeType IPCNam2;
port_t	Arg2;
 } MyMessage;

 typedef  struct { 
Msg	Head;
TypeType	RetCodeType;
 Integer 	RetCode;
TypeType IPCNam3;
kern_return_t	Arg3;
 } RepMessage;
 typedef MyMessage * ptrMyMsg;
 typedef RepMessage * ptrRepMsg;
ptrMyMsg MyMsgP;
ptrRepMsg RepMsgP;

task_t	ServPort;
port_t	my_port;
 extern kern_return_t	port_disable ( );


MyMsgP = (ptrMyMsg) (InP);
RepMsgP = (ptrRepMsg) (ReP);
RepMsgP->RetCode = Success;
RepMsgP->Head.SimpleMsg = TRUE;
RepMsgP->Head.MsgSize =  sizeof (RepMessage);
ServPort = convert_port_to_task(MyMsgP->Head.RemotePort);

/* Extract function arguments from message */


#if TypeCheck
	 if (_X_TypeName(MyMsgP->IPCNam2) != TypePt)
		 { 
		RepMsg.RetCode = WRONGARGS;
		 return ;
		 } 
#endif TypeCheck
	my_port = (MyMsgP->Arg2);

RepMsgP->Arg3 = (
/* Call service routine */

port_disable (ServPort, my_port));

/* Gather reply arguments */

#if FastAssign
	RepMsgP->IPCNam3 = 02000220002L;
#else FastAssign
	RepMsgP->IPCNam3 =        0;
	RepMsgP->IPCNam3 |= _InLine(  TRUE);
	RepMsgP->IPCNam3 |= _Deallocate( FALSE);
	RepMsgP->IPCNam3 |= _LongForm( FALSE);
	RepMsgP->IPCNam3 |= _TypeName(TypeInt32);
	RepMsgP->IPCNam3 |= _TypeSizeInBits(32);
	RepMsgP->IPCNam3 |= _NumObjects(1);
#endif FastAssign
 } 
 static void Xport_select(InP, ReP) caddr_t InP, ReP;
 { 

/* Define local messages and types */

 typedef  struct { 
Msg	Head;
 } MyMessage;

 typedef  struct { 
Msg	Head;
TypeType	RetCodeType;
 Integer 	RetCode;
TypeType IPCNam2;
 Integer 	TName2;
 Integer 	TSize2;
 Long 	NumElts2;
 caddr_t 	Arg2;
TypeType IPCNam3;
kern_return_t	Arg3;
 } RepMessage;
 typedef MyMessage * ptrMyMsg;
 typedef RepMessage * ptrRepMsg;
ptrMyMsg MyMsgP;
ptrRepMsg RepMsgP;

task_t	ServPort;
port_array_t	port_list;
 Long port_list_Cnt;
 extern kern_return_t	port_select ( );


MyMsgP = (ptrMyMsg) (InP);
RepMsgP = (ptrRepMsg) (ReP);
RepMsgP->RetCode = Success;
RepMsgP->Head.SimpleMsg = FALSE;
RepMsgP->Head.MsgSize =  sizeof (RepMessage);
ServPort = convert_port_to_task(MyMsgP->Head.RemotePort);

/* Extract function arguments from message */


RepMsgP->Arg3 = (
/* Call service routine */

port_select (ServPort,  & port_list,  & port_list_Cnt));

/* Gather reply arguments */

#if FastAssign
	RepMsgP->IPCNam2 = 04000000000L;
#else FastAssign
	RepMsgP->IPCNam2 =        0;
	RepMsgP->IPCNam2 |= _InLine( FALSE);
	RepMsgP->IPCNam2 |= _Deallocate( FALSE);
	RepMsgP->IPCNam2 |= _LongForm(  TRUE);
#endif FastAssign
	RepMsgP->TName2 = TypePt;
	RepMsgP->TSize2 = 32;
	RepMsgP->NumElts2 = (port_list_Cnt);
	RepMsgP->Arg2 = 	( caddr_t )(port_list);
#if FastAssign
	RepMsgP->IPCNam3 = 02000220002L;
#else FastAssign
	RepMsgP->IPCNam3 =        0;
	RepMsgP->IPCNam3 |= _InLine(  TRUE);
	RepMsgP->IPCNam3 |= _Deallocate( FALSE);
	RepMsgP->IPCNam3 |= _LongForm( FALSE);
	RepMsgP->IPCNam3 |= _TypeName(TypeInt32);
	RepMsgP->IPCNam3 |= _TypeSizeInBits(32);
	RepMsgP->IPCNam3 |= _NumObjects(1);
#endif FastAssign
 } 
 static void Xport_set_backlog(InP, ReP) caddr_t InP, ReP;
 { 

/* Define local messages and types */

 typedef  struct { 
Msg	Head;
TypeType IPCNam2;
port_t	Arg2;
TypeType IPCNam3;
int	Arg3;
 } MyMessage;

 typedef  struct { 
Msg	Head;
TypeType	RetCodeType;
 Integer 	RetCode;
TypeType IPCNam4;
kern_return_t	Arg4;
 } RepMessage;
 typedef MyMessage * ptrMyMsg;
 typedef RepMessage * ptrRepMsg;
ptrMyMsg MyMsgP;
ptrRepMsg RepMsgP;

task_t	ServPort;
port_t	my_port;
int	backlog;
 extern kern_return_t	port_set_backlog ( );


MyMsgP = (ptrMyMsg) (InP);
RepMsgP = (ptrRepMsg) (ReP);
RepMsgP->RetCode = Success;
RepMsgP->Head.SimpleMsg = TRUE;
RepMsgP->Head.MsgSize =  sizeof (RepMessage);
ServPort = convert_port_to_task(MyMsgP->Head.RemotePort);

/* Extract function arguments from message */


#if TypeCheck
	 if (_X_TypeName(MyMsgP->IPCNam2) != TypePt)
		 { 
		RepMsg.RetCode = WRONGARGS;
		 return ;
		 } 
#endif TypeCheck
	my_port = (MyMsgP->Arg2);

#if TypeCheck
	 if (_X_TypeName(MyMsgP->IPCNam3) != TypeInt32)
		 { 
		RepMsg.RetCode = WRONGARGS;
		 return ;
		 } 
#endif TypeCheck
	backlog = (MyMsgP->Arg3);

RepMsgP->Arg4 = (
/* Call service routine */

port_set_backlog (ServPort, my_port, backlog));

/* Gather reply arguments */

#if FastAssign
	RepMsgP->IPCNam4 = 02000220002L;
#else FastAssign
	RepMsgP->IPCNam4 =        0;
	RepMsgP->IPCNam4 |= _InLine(  TRUE);
	RepMsgP->IPCNam4 |= _Deallocate( FALSE);
	RepMsgP->IPCNam4 |= _LongForm( FALSE);
	RepMsgP->IPCNam4 |= _TypeName(TypeInt32);
	RepMsgP->IPCNam4 |= _TypeSizeInBits(32);
	RepMsgP->IPCNam4 |= _NumObjects(1);
#endif FastAssign
 } 
 static void Xport_status(InP, ReP) caddr_t InP, ReP;
 { 

/* Define local messages and types */

 typedef  struct { 
Msg	Head;
TypeType IPCNam2;
port_t	Arg2;
 } MyMessage;

 typedef  struct { 
Msg	Head;
TypeType	RetCodeType;
 Integer 	RetCode;
TypeType IPCNam3;
boolean_t	Arg3;
TypeType IPCNam4;
int	Arg4;
TypeType IPCNam5;
int	Arg5;
TypeType IPCNam6;
kern_return_t	Arg6;
 } RepMessage;
 typedef MyMessage * ptrMyMsg;
 typedef RepMessage * ptrRepMsg;
ptrMyMsg MyMsgP;
ptrRepMsg RepMsgP;

task_t	ServPort;
port_t	my_port;
boolean_t	enabled;
int	num_msgs;
int	backlog;
 extern kern_return_t	port_status ( );


MyMsgP = (ptrMyMsg) (InP);
RepMsgP = (ptrRepMsg) (ReP);
RepMsgP->RetCode = Success;
RepMsgP->Head.SimpleMsg = TRUE;
RepMsgP->Head.MsgSize =  sizeof (RepMessage);
ServPort = convert_port_to_task(MyMsgP->Head.RemotePort);

/* Extract function arguments from message */


#if TypeCheck
	 if (_X_TypeName(MyMsgP->IPCNam2) != TypePt)
		 { 
		RepMsg.RetCode = WRONGARGS;
		 return ;
		 } 
#endif TypeCheck
	my_port = (MyMsgP->Arg2);

RepMsgP->Arg6 = (
/* Call service routine */

port_status (ServPort, my_port,  & enabled,  & num_msgs,  & backlog));

/* Gather reply arguments */

#if FastAssign
	RepMsgP->IPCNam3 = 02000210000L;
#else FastAssign
	RepMsgP->IPCNam3 =        0;
	RepMsgP->IPCNam3 |= _InLine(  TRUE);
	RepMsgP->IPCNam3 |= _Deallocate( FALSE);
	RepMsgP->IPCNam3 |= _LongForm( FALSE);
	RepMsgP->IPCNam3 |= _TypeName(TypeBoolean);
	RepMsgP->IPCNam3 |= _TypeSizeInBits(16);
	RepMsgP->IPCNam3 |= _NumObjects(1);
#endif FastAssign
	RepMsgP->Arg3 = (enabled);
#if FastAssign
	RepMsgP->IPCNam4 = 02000220002L;
#else FastAssign
	RepMsgP->IPCNam4 =        0;
	RepMsgP->IPCNam4 |= _InLine(  TRUE);
	RepMsgP->IPCNam4 |= _Deallocate( FALSE);
	RepMsgP->IPCNam4 |= _LongForm( FALSE);
	RepMsgP->IPCNam4 |= _TypeName(TypeInt32);
	RepMsgP->IPCNam4 |= _TypeSizeInBits(32);
	RepMsgP->IPCNam4 |= _NumObjects(1);
#endif FastAssign
	RepMsgP->Arg4 = (num_msgs);
#if FastAssign
	RepMsgP->IPCNam5 = 02000220002L;
#else FastAssign
	RepMsgP->IPCNam5 =        0;
	RepMsgP->IPCNam5 |= _InLine(  TRUE);
	RepMsgP->IPCNam5 |= _Deallocate( FALSE);
	RepMsgP->IPCNam5 |= _LongForm( FALSE);
	RepMsgP->IPCNam5 |= _TypeName(TypeInt32);
	RepMsgP->IPCNam5 |= _TypeSizeInBits(32);
	RepMsgP->IPCNam5 |= _NumObjects(1);
#endif FastAssign
	RepMsgP->Arg5 = (backlog);
#if FastAssign
	RepMsgP->IPCNam6 = 02000220002L;
#else FastAssign
	RepMsgP->IPCNam6 =        0;
	RepMsgP->IPCNam6 |= _InLine(  TRUE);
	RepMsgP->IPCNam6 |= _Deallocate( FALSE);
	RepMsgP->IPCNam6 |= _LongForm( FALSE);
	RepMsgP->IPCNam6 |= _TypeName(TypeInt32);
	RepMsgP->IPCNam6 |= _TypeSizeInBits(32);
	RepMsgP->IPCNam6 |= _NumObjects(1);
#endif FastAssign
 } 
 static void Xtask_create(InP, ReP) caddr_t InP, ReP;
 { 

/* Define local messages and types */

 typedef  struct { 
Msg	Head;
TypeType IPCNam2;
boolean_t	Arg2;
 } MyMessage;

 typedef  struct { 
Msg	Head;
TypeType	RetCodeType;
 Integer 	RetCode;
TypeType IPCNam3;
port_t	Arg3;
TypeType IPCNam4;
port_t	Arg4;
TypeType IPCNam5;
kern_return_t	Arg5;
 } RepMessage;
 typedef MyMessage * ptrMyMsg;
 typedef RepMessage * ptrRepMsg;
ptrMyMsg MyMsgP;
ptrRepMsg RepMsgP;

task_t	ServPort;
boolean_t	inherit_memory;
task_t	child_task;
port_t	child_data;
 extern kern_return_t	task_create ( );


MyMsgP = (ptrMyMsg) (InP);
RepMsgP = (ptrRepMsg) (ReP);
RepMsgP->RetCode = Success;
RepMsgP->Head.SimpleMsg = FALSE;
RepMsgP->Head.MsgSize =  sizeof (RepMessage);
ServPort = convert_port_to_task(MyMsgP->Head.RemotePort);

/* Extract function arguments from message */


#if TypeCheck
	 if (_X_TypeName(MyMsgP->IPCNam2) != TypeBoolean)
		 { 
		RepMsg.RetCode = WRONGARGS;
		 return ;
		 } 
#endif TypeCheck
	inherit_memory = (MyMsgP->Arg2);

RepMsgP->Arg5 = (
/* Call service routine */

task_create (ServPort, inherit_memory,  & child_task,  & child_data));

/* Gather reply arguments */

#if FastAssign
	RepMsgP->IPCNam3 = 02000220006L;
#else FastAssign
	RepMsgP->IPCNam3 =        0;
	RepMsgP->IPCNam3 |= _InLine(  TRUE);
	RepMsgP->IPCNam3 |= _Deallocate( FALSE);
	RepMsgP->IPCNam3 |= _LongForm( FALSE);
	RepMsgP->IPCNam3 |= _TypeName(TypePt);
	RepMsgP->IPCNam3 |= _TypeSizeInBits(32);
	RepMsgP->IPCNam3 |= _NumObjects(1);
#endif FastAssign
	RepMsgP->Arg3 = convert_task_to_port(child_task);
#if FastAssign
	RepMsgP->IPCNam4 = 02000220006L;
#else FastAssign
	RepMsgP->IPCNam4 =        0;
	RepMsgP->IPCNam4 |= _InLine(  TRUE);
	RepMsgP->IPCNam4 |= _Deallocate( FALSE);
	RepMsgP->IPCNam4 |= _LongForm( FALSE);
	RepMsgP->IPCNam4 |= _TypeName(TypePt);
	RepMsgP->IPCNam4 |= _TypeSizeInBits(32);
	RepMsgP->IPCNam4 |= _NumObjects(1);
#endif FastAssign
	RepMsgP->Arg4 = (child_data);
#if FastAssign
	RepMsgP->IPCNam5 = 02000220002L;
#else FastAssign
	RepMsgP->IPCNam5 =        0;
	RepMsgP->IPCNam5 |= _InLine(  TRUE);
	RepMsgP->IPCNam5 |= _Deallocate( FALSE);
	RepMsgP->IPCNam5 |= _LongForm( FALSE);
	RepMsgP->IPCNam5 |= _TypeName(TypeInt32);
	RepMsgP->IPCNam5 |= _TypeSizeInBits(32);
	RepMsgP->IPCNam5 |= _NumObjects(1);
#endif FastAssign
 } 
 static void Xtask_terminate(InP, ReP) caddr_t InP, ReP;
 { 

/* Define local messages and types */

 typedef  struct { 
Msg	Head;
TypeType IPCNam2;
int	Arg2;
 } MyMessage;

 typedef  struct { 
Msg	Head;
TypeType	RetCodeType;
 Integer 	RetCode;
TypeType IPCNam3;
kern_return_t	Arg3;
 } RepMessage;
 typedef MyMessage * ptrMyMsg;
 typedef RepMessage * ptrRepMsg;
ptrMyMsg MyMsgP;
ptrRepMsg RepMsgP;

task_t	ServPort;
int	reason;
 extern kern_return_t	task_terminate ( );


MyMsgP = (ptrMyMsg) (InP);
RepMsgP = (ptrRepMsg) (ReP);
RepMsgP->RetCode = Success;
RepMsgP->Head.SimpleMsg = TRUE;
RepMsgP->Head.MsgSize =  sizeof (RepMessage);
ServPort = convert_port_to_task(MyMsgP->Head.RemotePort);

/* Extract function arguments from message */


#if TypeCheck
	 if (_X_TypeName(MyMsgP->IPCNam2) != TypeInt32)
		 { 
		RepMsg.RetCode = WRONGARGS;
		 return ;
		 } 
#endif TypeCheck
	reason = (MyMsgP->Arg2);

RepMsgP->Arg3 = (
/* Call service routine */

task_terminate (ServPort, reason));

/* Gather reply arguments */

#if FastAssign
	RepMsgP->IPCNam3 = 02000220002L;
#else FastAssign
	RepMsgP->IPCNam3 =        0;
	RepMsgP->IPCNam3 |= _InLine(  TRUE);
	RepMsgP->IPCNam3 |= _Deallocate( FALSE);
	RepMsgP->IPCNam3 |= _LongForm( FALSE);
	RepMsgP->IPCNam3 |= _TypeName(TypeInt32);
	RepMsgP->IPCNam3 |= _TypeSizeInBits(32);
	RepMsgP->IPCNam3 |= _NumObjects(1);
#endif FastAssign
 } 
 static void Xtask_suspend(InP, ReP) caddr_t InP, ReP;
 { 

/* Define local messages and types */

 typedef  struct { 
Msg	Head;
 } MyMessage;

 typedef  struct { 
Msg	Head;
TypeType	RetCodeType;
 Integer 	RetCode;
TypeType IPCNam2;
kern_return_t	Arg2;
 } RepMessage;
 typedef MyMessage * ptrMyMsg;
 typedef RepMessage * ptrRepMsg;
ptrMyMsg MyMsgP;
ptrRepMsg RepMsgP;

task_t	ServPort;
 extern kern_return_t	task_suspend ( );


MyMsgP = (ptrMyMsg) (InP);
RepMsgP = (ptrRepMsg) (ReP);
RepMsgP->RetCode = Success;
RepMsgP->Head.SimpleMsg = TRUE;
RepMsgP->Head.MsgSize =  sizeof (RepMessage);
ServPort = convert_port_to_task(MyMsgP->Head.RemotePort);

/* Extract function arguments from message */


RepMsgP->Arg2 = (
/* Call service routine */

task_suspend (ServPort));

/* Gather reply arguments */

#if FastAssign
	RepMsgP->IPCNam2 = 02000220002L;
#else FastAssign
	RepMsgP->IPCNam2 =        0;
	RepMsgP->IPCNam2 |= _InLine(  TRUE);
	RepMsgP->IPCNam2 |= _Deallocate( FALSE);
	RepMsgP->IPCNam2 |= _LongForm( FALSE);
	RepMsgP->IPCNam2 |= _TypeName(TypeInt32);
	RepMsgP->IPCNam2 |= _TypeSizeInBits(32);
	RepMsgP->IPCNam2 |= _NumObjects(1);
#endif FastAssign
 } 
 static void Xtask_resume(InP, ReP) caddr_t InP, ReP;
 { 

/* Define local messages and types */

 typedef  struct { 
Msg	Head;
 } MyMessage;

 typedef  struct { 
Msg	Head;
TypeType	RetCodeType;
 Integer 	RetCode;
TypeType IPCNam2;
kern_return_t	Arg2;
 } RepMessage;
 typedef MyMessage * ptrMyMsg;
 typedef RepMessage * ptrRepMsg;
ptrMyMsg MyMsgP;
ptrRepMsg RepMsgP;

task_t	ServPort;
 extern kern_return_t	task_resume ( );


MyMsgP = (ptrMyMsg) (InP);
RepMsgP = (ptrRepMsg) (ReP);
RepMsgP->RetCode = Success;
RepMsgP->Head.SimpleMsg = TRUE;
RepMsgP->Head.MsgSize =  sizeof (RepMessage);
ServPort = convert_port_to_task(MyMsgP->Head.RemotePort);

/* Extract function arguments from message */


RepMsgP->Arg2 = (
/* Call service routine */

task_resume (ServPort));

/* Gather reply arguments */

#if FastAssign
	RepMsgP->IPCNam2 = 02000220002L;
#else FastAssign
	RepMsgP->IPCNam2 =        0;
	RepMsgP->IPCNam2 |= _InLine(  TRUE);
	RepMsgP->IPCNam2 |= _Deallocate( FALSE);
	RepMsgP->IPCNam2 |= _LongForm( FALSE);
	RepMsgP->IPCNam2 |= _TypeName(TypeInt32);
	RepMsgP->IPCNam2 |= _TypeSizeInBits(32);
	RepMsgP->IPCNam2 |= _NumObjects(1);
#endif FastAssign
 } 
 static void Xtask_threads(InP, ReP) caddr_t InP, ReP;
 { 

/* Define local messages and types */

 typedef  struct { 
Msg	Head;
 } MyMessage;

 typedef  struct { 
Msg	Head;
TypeType	RetCodeType;
 Integer 	RetCode;
TypeType IPCNam2;
 Integer 	TName2;
 Integer 	TSize2;
 Long 	NumElts2;
 caddr_t 	Arg2;
TypeType IPCNam3;
kern_return_t	Arg3;
 } RepMessage;
 typedef MyMessage * ptrMyMsg;
 typedef RepMessage * ptrRepMsg;
ptrMyMsg MyMsgP;
ptrRepMsg RepMsgP;

task_t	ServPort;
thread_array_t	thread_list;
 Long thread_list_Cnt;
 extern kern_return_t	task_threads ( );


MyMsgP = (ptrMyMsg) (InP);
RepMsgP = (ptrRepMsg) (ReP);
RepMsgP->RetCode = Success;
RepMsgP->Head.SimpleMsg = FALSE;
RepMsgP->Head.MsgSize =  sizeof (RepMessage);
ServPort = convert_port_to_task(MyMsgP->Head.RemotePort);

/* Extract function arguments from message */


RepMsgP->Arg3 = (
/* Call service routine */

task_threads (ServPort,  & thread_list,  & thread_list_Cnt));

/* Gather reply arguments */

#if FastAssign
	RepMsgP->IPCNam2 = 04000000000L;
#else FastAssign
	RepMsgP->IPCNam2 =        0;
	RepMsgP->IPCNam2 |= _InLine( FALSE);
	RepMsgP->IPCNam2 |= _Deallocate( FALSE);
	RepMsgP->IPCNam2 |= _LongForm(  TRUE);
#endif FastAssign
	RepMsgP->TName2 = TypePt;
	RepMsgP->TSize2 = 32;
	RepMsgP->NumElts2 = (thread_list_Cnt);
	RepMsgP->Arg2 = 	( caddr_t )convert_thread_to_port(thread_list);
#if FastAssign
	RepMsgP->IPCNam3 = 02000220002L;
#else FastAssign
	RepMsgP->IPCNam3 =        0;
	RepMsgP->IPCNam3 |= _InLine(  TRUE);
	RepMsgP->IPCNam3 |= _Deallocate( FALSE);
	RepMsgP->IPCNam3 |= _LongForm( FALSE);
	RepMsgP->IPCNam3 |= _TypeName(TypeInt32);
	RepMsgP->IPCNam3 |= _TypeSizeInBits(32);
	RepMsgP->IPCNam3 |= _NumObjects(1);
#endif FastAssign
 } 
 static void Xtask_ports(InP, ReP) caddr_t InP, ReP;
 { 

/* Define local messages and types */

 typedef  struct { 
Msg	Head;
 } MyMessage;

 typedef  struct { 
Msg	Head;
TypeType	RetCodeType;
 Integer 	RetCode;
TypeType IPCNam2;
 Integer 	TName2;
 Integer 	TSize2;
 Long 	NumElts2;
 caddr_t 	Arg2;
TypeType IPCNam3;
kern_return_t	Arg3;
 } RepMessage;
 typedef MyMessage * ptrMyMsg;
 typedef RepMessage * ptrRepMsg;
ptrMyMsg MyMsgP;
ptrRepMsg RepMsgP;

task_t	ServPort;
port_array_t	port_list;
 Long port_list_Cnt;
 extern kern_return_t	task_ports ( );


MyMsgP = (ptrMyMsg) (InP);
RepMsgP = (ptrRepMsg) (ReP);
RepMsgP->RetCode = Success;
RepMsgP->Head.SimpleMsg = FALSE;
RepMsgP->Head.MsgSize =  sizeof (RepMessage);
ServPort = convert_port_to_task(MyMsgP->Head.RemotePort);

/* Extract function arguments from message */


RepMsgP->Arg3 = (
/* Call service routine */

task_ports (ServPort,  & port_list,  & port_list_Cnt));

/* Gather reply arguments */

#if FastAssign
	RepMsgP->IPCNam2 = 04000000000L;
#else FastAssign
	RepMsgP->IPCNam2 =        0;
	RepMsgP->IPCNam2 |= _InLine( FALSE);
	RepMsgP->IPCNam2 |= _Deallocate( FALSE);
	RepMsgP->IPCNam2 |= _LongForm(  TRUE);
#endif FastAssign
	RepMsgP->TName2 = TypePt;
	RepMsgP->TSize2 = 32;
	RepMsgP->NumElts2 = (port_list_Cnt);
	RepMsgP->Arg2 = 	( caddr_t )(port_list);
#if FastAssign
	RepMsgP->IPCNam3 = 02000220002L;
#else FastAssign
	RepMsgP->IPCNam3 =        0;
	RepMsgP->IPCNam3 |= _InLine(  TRUE);
	RepMsgP->IPCNam3 |= _Deallocate( FALSE);
	RepMsgP->IPCNam3 |= _LongForm( FALSE);
	RepMsgP->IPCNam3 |= _TypeName(TypeInt32);
	RepMsgP->IPCNam3 |= _TypeSizeInBits(32);
	RepMsgP->IPCNam3 |= _NumObjects(1);
#endif FastAssign
 } 
 static void Xtask_status(InP, ReP) caddr_t InP, ReP;
 { 

/* Define local messages and types */

 typedef  struct { 
Msg	Head;
 } MyMessage;

 typedef  struct { 
Msg	Head;
TypeType	RetCodeType;
 Integer 	RetCode;
TypeType IPCNam2;
port_t	Arg2;
TypeType IPCNam3;
boolean_t	Arg3;
TypeType IPCNam4;
kern_return_t	Arg4;
 } RepMessage;
 typedef MyMessage * ptrMyMsg;
 typedef RepMessage * ptrRepMsg;
ptrMyMsg MyMsgP;
ptrRepMsg RepMsgP;

task_t	ServPort;
port_t	notify_port;
boolean_t	resumed;
 extern kern_return_t	task_status ( );


MyMsgP = (ptrMyMsg) (InP);
RepMsgP = (ptrRepMsg) (ReP);
RepMsgP->RetCode = Success;
RepMsgP->Head.SimpleMsg = FALSE;
RepMsgP->Head.MsgSize =  sizeof (RepMessage);
ServPort = convert_port_to_task(MyMsgP->Head.RemotePort);

/* Extract function arguments from message */


RepMsgP->Arg4 = (
/* Call service routine */

task_status (ServPort,  & notify_port,  & resumed));

/* Gather reply arguments */

#if FastAssign
	RepMsgP->IPCNam2 = 02000220006L;
#else FastAssign
	RepMsgP->IPCNam2 =        0;
	RepMsgP->IPCNam2 |= _InLine(  TRUE);
	RepMsgP->IPCNam2 |= _Deallocate( FALSE);
	RepMsgP->IPCNam2 |= _LongForm( FALSE);
	RepMsgP->IPCNam2 |= _TypeName(TypePt);
	RepMsgP->IPCNam2 |= _TypeSizeInBits(32);
	RepMsgP->IPCNam2 |= _NumObjects(1);
#endif FastAssign
	RepMsgP->Arg2 = (notify_port);
#if FastAssign
	RepMsgP->IPCNam3 = 02000210000L;
#else FastAssign
	RepMsgP->IPCNam3 =        0;
	RepMsgP->IPCNam3 |= _InLine(  TRUE);
	RepMsgP->IPCNam3 |= _Deallocate( FALSE);
	RepMsgP->IPCNam3 |= _LongForm( FALSE);
	RepMsgP->IPCNam3 |= _TypeName(TypeBoolean);
	RepMsgP->IPCNam3 |= _TypeSizeInBits(16);
	RepMsgP->IPCNam3 |= _NumObjects(1);
#endif FastAssign
	RepMsgP->Arg3 = (resumed);
#if FastAssign
	RepMsgP->IPCNam4 = 02000220002L;
#else FastAssign
	RepMsgP->IPCNam4 =        0;
	RepMsgP->IPCNam4 |= _InLine(  TRUE);
	RepMsgP->IPCNam4 |= _Deallocate( FALSE);
	RepMsgP->IPCNam4 |= _LongForm( FALSE);
	RepMsgP->IPCNam4 |= _TypeName(TypeInt32);
	RepMsgP->IPCNam4 |= _TypeSizeInBits(32);
	RepMsgP->IPCNam4 |= _NumObjects(1);
#endif FastAssign
 } 
 static void Xtask_set_notify(InP, ReP) caddr_t InP, ReP;
 { 

/* Define local messages and types */

 typedef  struct { 
Msg	Head;
TypeType IPCNam2;
port_t	Arg2;
 } MyMessage;

 typedef  struct { 
Msg	Head;
TypeType	RetCodeType;
 Integer 	RetCode;
TypeType IPCNam3;
kern_return_t	Arg3;
 } RepMessage;
 typedef MyMessage * ptrMyMsg;
 typedef RepMessage * ptrRepMsg;
ptrMyMsg MyMsgP;
ptrRepMsg RepMsgP;

task_t	ServPort;
port_t	notify_port;
 extern kern_return_t	task_set_notify ( );


MyMsgP = (ptrMyMsg) (InP);
RepMsgP = (ptrRepMsg) (ReP);
RepMsgP->RetCode = Success;
RepMsgP->Head.SimpleMsg = TRUE;
RepMsgP->Head.MsgSize =  sizeof (RepMessage);
ServPort = convert_port_to_task(MyMsgP->Head.RemotePort);

/* Extract function arguments from message */


#if TypeCheck
	 if (_X_TypeName(MyMsgP->IPCNam2) != TypePt)
		 { 
		RepMsg.RetCode = WRONGARGS;
		 return ;
		 } 
#endif TypeCheck
	notify_port = (MyMsgP->Arg2);

RepMsgP->Arg3 = (
/* Call service routine */

task_set_notify (ServPort, notify_port));

/* Gather reply arguments */

#if FastAssign
	RepMsgP->IPCNam3 = 02000220002L;
#else FastAssign
	RepMsgP->IPCNam3 =        0;
	RepMsgP->IPCNam3 |= _InLine(  TRUE);
	RepMsgP->IPCNam3 |= _Deallocate( FALSE);
	RepMsgP->IPCNam3 |= _LongForm( FALSE);
	RepMsgP->IPCNam3 |= _TypeName(TypeInt32);
	RepMsgP->IPCNam3 |= _TypeSizeInBits(32);
	RepMsgP->IPCNam3 |= _NumObjects(1);
#endif FastAssign
 } 
 static void Xthread_create(InP, ReP) caddr_t InP, ReP;
 { 

/* Define local messages and types */

 typedef  struct { 
Msg	Head;
 } MyMessage;

 typedef  struct { 
Msg	Head;
TypeType	RetCodeType;
 Integer 	RetCode;
TypeType IPCNam2;
port_t	Arg2;
TypeType IPCNam3;
port_t	Arg3;
TypeType IPCNam4;
kern_return_t	Arg4;
 } RepMessage;
 typedef MyMessage * ptrMyMsg;
 typedef RepMessage * ptrRepMsg;
ptrMyMsg MyMsgP;
ptrRepMsg RepMsgP;

task_t	ServPort;
thread_t	child_thread;
port_t	child_data;
 extern kern_return_t	thread_create ( );


MyMsgP = (ptrMyMsg) (InP);
RepMsgP = (ptrRepMsg) (ReP);
RepMsgP->RetCode = Success;
RepMsgP->Head.SimpleMsg = FALSE;
RepMsgP->Head.MsgSize =  sizeof (RepMessage);
ServPort = convert_port_to_task(MyMsgP->Head.RemotePort);

/* Extract function arguments from message */


RepMsgP->Arg4 = (
/* Call service routine */

thread_create (ServPort,  & child_thread,  & child_data));

/* Gather reply arguments */

#if FastAssign
	RepMsgP->IPCNam2 = 02000220006L;
#else FastAssign
	RepMsgP->IPCNam2 =        0;
	RepMsgP->IPCNam2 |= _InLine(  TRUE);
	RepMsgP->IPCNam2 |= _Deallocate( FALSE);
	RepMsgP->IPCNam2 |= _LongForm( FALSE);
	RepMsgP->IPCNam2 |= _TypeName(TypePt);
	RepMsgP->IPCNam2 |= _TypeSizeInBits(32);
	RepMsgP->IPCNam2 |= _NumObjects(1);
#endif FastAssign
	RepMsgP->Arg2 = convert_thread_to_port(child_thread);
#if FastAssign
	RepMsgP->IPCNam3 = 02000220006L;
#else FastAssign
	RepMsgP->IPCNam3 =        0;
	RepMsgP->IPCNam3 |= _InLine(  TRUE);
	RepMsgP->IPCNam3 |= _Deallocate( FALSE);
	RepMsgP->IPCNam3 |= _LongForm( FALSE);
	RepMsgP->IPCNam3 |= _TypeName(TypePt);
	RepMsgP->IPCNam3 |= _TypeSizeInBits(32);
	RepMsgP->IPCNam3 |= _NumObjects(1);
#endif FastAssign
	RepMsgP->Arg3 = (child_data);
#if FastAssign
	RepMsgP->IPCNam4 = 02000220002L;
#else FastAssign
	RepMsgP->IPCNam4 =        0;
	RepMsgP->IPCNam4 |= _InLine(  TRUE);
	RepMsgP->IPCNam4 |= _Deallocate( FALSE);
	RepMsgP->IPCNam4 |= _LongForm( FALSE);
	RepMsgP->IPCNam4 |= _TypeName(TypeInt32);
	RepMsgP->IPCNam4 |= _TypeSizeInBits(32);
	RepMsgP->IPCNam4 |= _NumObjects(1);
#endif FastAssign
 } 
 static void Xthread_terminate(InP, ReP) caddr_t InP, ReP;
 { 

/* Define local messages and types */

 typedef  struct { 
Msg	Head;
 } MyMessage;

 typedef  struct { 
Msg	Head;
TypeType	RetCodeType;
 Integer 	RetCode;
TypeType IPCNam2;
kern_return_t	Arg2;
 } RepMessage;
 typedef MyMessage * ptrMyMsg;
 typedef RepMessage * ptrRepMsg;
ptrMyMsg MyMsgP;
ptrRepMsg RepMsgP;

thread_t	ServPort;
 extern kern_return_t	thread_terminate ( );


MyMsgP = (ptrMyMsg) (InP);
RepMsgP = (ptrRepMsg) (ReP);
RepMsgP->RetCode = Success;
RepMsgP->Head.SimpleMsg = TRUE;
RepMsgP->Head.MsgSize =  sizeof (RepMessage);
ServPort = convert_port_to_thread(MyMsgP->Head.RemotePort);

/* Extract function arguments from message */


RepMsgP->Arg2 = (
/* Call service routine */

thread_terminate (ServPort));

/* Gather reply arguments */

#if FastAssign
	RepMsgP->IPCNam2 = 02000220002L;
#else FastAssign
	RepMsgP->IPCNam2 =        0;
	RepMsgP->IPCNam2 |= _InLine(  TRUE);
	RepMsgP->IPCNam2 |= _Deallocate( FALSE);
	RepMsgP->IPCNam2 |= _LongForm( FALSE);
	RepMsgP->IPCNam2 |= _TypeName(TypeInt32);
	RepMsgP->IPCNam2 |= _TypeSizeInBits(32);
	RepMsgP->IPCNam2 |= _NumObjects(1);
#endif FastAssign
 } 
 static void Xthread_suspend(InP, ReP) caddr_t InP, ReP;
 { 

/* Define local messages and types */

 typedef  struct { 
Msg	Head;
 } MyMessage;

 typedef  struct { 
Msg	Head;
TypeType	RetCodeType;
 Integer 	RetCode;
TypeType IPCNam2;
kern_return_t	Arg2;
 } RepMessage;
 typedef MyMessage * ptrMyMsg;
 typedef RepMessage * ptrRepMsg;
ptrMyMsg MyMsgP;
ptrRepMsg RepMsgP;

thread_t	ServPort;
 extern kern_return_t	thread_suspend ( );


MyMsgP = (ptrMyMsg) (InP);
RepMsgP = (ptrRepMsg) (ReP);
RepMsgP->RetCode = Success;
RepMsgP->Head.SimpleMsg = TRUE;
RepMsgP->Head.MsgSize =  sizeof (RepMessage);
ServPort = convert_port_to_thread(MyMsgP->Head.RemotePort);

/* Extract function arguments from message */


RepMsgP->Arg2 = (
/* Call service routine */

thread_suspend (ServPort));

/* Gather reply arguments */

#if FastAssign
	RepMsgP->IPCNam2 = 02000220002L;
#else FastAssign
	RepMsgP->IPCNam2 =        0;
	RepMsgP->IPCNam2 |= _InLine(  TRUE);
	RepMsgP->IPCNam2 |= _Deallocate( FALSE);
	RepMsgP->IPCNam2 |= _LongForm( FALSE);
	RepMsgP->IPCNam2 |= _TypeName(TypeInt32);
	RepMsgP->IPCNam2 |= _TypeSizeInBits(32);
	RepMsgP->IPCNam2 |= _NumObjects(1);
#endif FastAssign
 } 
 static void Xthread_resume(InP, ReP) caddr_t InP, ReP;
 { 

/* Define local messages and types */

 typedef  struct { 
Msg	Head;
 } MyMessage;

 typedef  struct { 
Msg	Head;
TypeType	RetCodeType;
 Integer 	RetCode;
TypeType IPCNam2;
kern_return_t	Arg2;
 } RepMessage;
 typedef MyMessage * ptrMyMsg;
 typedef RepMessage * ptrRepMsg;
ptrMyMsg MyMsgP;
ptrRepMsg RepMsgP;

thread_t	ServPort;
 extern kern_return_t	thread_resume ( );


MyMsgP = (ptrMyMsg) (InP);
RepMsgP = (ptrRepMsg) (ReP);
RepMsgP->RetCode = Success;
RepMsgP->Head.SimpleMsg = TRUE;
RepMsgP->Head.MsgSize =  sizeof (RepMessage);
ServPort = convert_port_to_thread(MyMsgP->Head.RemotePort);

/* Extract function arguments from message */


RepMsgP->Arg2 = (
/* Call service routine */

thread_resume (ServPort));

/* Gather reply arguments */

#if FastAssign
	RepMsgP->IPCNam2 = 02000220002L;
#else FastAssign
	RepMsgP->IPCNam2 =        0;
	RepMsgP->IPCNam2 |= _InLine(  TRUE);
	RepMsgP->IPCNam2 |= _Deallocate( FALSE);
	RepMsgP->IPCNam2 |= _LongForm( FALSE);
	RepMsgP->IPCNam2 |= _TypeName(TypeInt32);
	RepMsgP->IPCNam2 |= _TypeSizeInBits(32);
	RepMsgP->IPCNam2 |= _NumObjects(1);
#endif FastAssign
 } 
 static void Xthread_status(InP, ReP) caddr_t InP, ReP;
 { 

/* Define local messages and types */

 typedef  struct { 
Msg	Head;
 } MyMessage;

 typedef  struct { 
Msg	Head;
TypeType	RetCodeType;
 Integer 	RetCode;
TypeType IPCNam2;
boolean_t	Arg2;
TypeType IPCNam3;
thread_status_t	Arg3;
TypeType IPCNam4;
kern_return_t	Arg4;
 } RepMessage;
 typedef MyMessage * ptrMyMsg;
 typedef RepMessage * ptrRepMsg;
ptrMyMsg MyMsgP;
ptrRepMsg RepMsgP;

thread_t	ServPort;
boolean_t	resumed;
thread_status_t	old_state;
 extern kern_return_t	thread_status ( );


MyMsgP = (ptrMyMsg) (InP);
RepMsgP = (ptrRepMsg) (ReP);
RepMsgP->RetCode = Success;
RepMsgP->Head.SimpleMsg = TRUE;
RepMsgP->Head.MsgSize =  sizeof (RepMessage);
ServPort = convert_port_to_thread(MyMsgP->Head.RemotePort);

/* Extract function arguments from message */


RepMsgP->Arg4 = (
/* Call service routine */

thread_status (ServPort,  & resumed,  & old_state));

/* Gather reply arguments */

#if FastAssign
	RepMsgP->IPCNam2 = 02000210000L;
#else FastAssign
	RepMsgP->IPCNam2 =        0;
	RepMsgP->IPCNam2 |= _InLine(  TRUE);
	RepMsgP->IPCNam2 |= _Deallocate( FALSE);
	RepMsgP->IPCNam2 |= _LongForm( FALSE);
	RepMsgP->IPCNam2 |= _TypeName(TypeBoolean);
	RepMsgP->IPCNam2 |= _TypeSizeInBits(16);
	RepMsgP->IPCNam2 |= _NumObjects(1);
#endif FastAssign
	RepMsgP->Arg2 = (resumed);
#if FastAssign
	RepMsgP->IPCNam3 = 02000220002L;
#else FastAssign
	RepMsgP->IPCNam3 =        0;
	RepMsgP->IPCNam3 |= _InLine(  TRUE);
	RepMsgP->IPCNam3 |= _Deallocate( FALSE);
	RepMsgP->IPCNam3 |= _LongForm( FALSE);
	RepMsgP->IPCNam3 |= _TypeName(TypeInt32);
	RepMsgP->IPCNam3 |= _TypeSizeInBits(32);
	RepMsgP->IPCNam3 |= _NumObjects(1);
#endif FastAssign
	RepMsgP->Arg3 = (old_state);
#if FastAssign
	RepMsgP->IPCNam4 = 02000220002L;
#else FastAssign
	RepMsgP->IPCNam4 =        0;
	RepMsgP->IPCNam4 |= _InLine(  TRUE);
	RepMsgP->IPCNam4 |= _Deallocate( FALSE);
	RepMsgP->IPCNam4 |= _LongForm( FALSE);
	RepMsgP->IPCNam4 |= _TypeName(TypeInt32);
	RepMsgP->IPCNam4 |= _TypeSizeInBits(32);
	RepMsgP->IPCNam4 |= _NumObjects(1);
#endif FastAssign
 } 
 static void Xthread_mutate(InP, ReP) caddr_t InP, ReP;
 { 

/* Define local messages and types */

 typedef  struct { 
Msg	Head;
TypeType IPCNam2;
thread_status_t	Arg2;
 } MyMessage;

 typedef  struct { 
Msg	Head;
TypeType	RetCodeType;
 Integer 	RetCode;
TypeType IPCNam3;
kern_return_t	Arg3;
 } RepMessage;
 typedef MyMessage * ptrMyMsg;
 typedef RepMessage * ptrRepMsg;
ptrMyMsg MyMsgP;
ptrRepMsg RepMsgP;

thread_t	ServPort;
thread_status_t	new_state;
 extern kern_return_t	thread_mutate ( );


MyMsgP = (ptrMyMsg) (InP);
RepMsgP = (ptrRepMsg) (ReP);
RepMsgP->RetCode = Success;
RepMsgP->Head.SimpleMsg = TRUE;
RepMsgP->Head.MsgSize =  sizeof (RepMessage);
ServPort = convert_port_to_thread(MyMsgP->Head.RemotePort);

/* Extract function arguments from message */


#if TypeCheck
	 if (_X_TypeName(MyMsgP->IPCNam2) != TypeInt32)
		 { 
		RepMsg.RetCode = WRONGARGS;
		 return ;
		 } 
#endif TypeCheck
	new_state = (MyMsgP->Arg2);

RepMsgP->Arg3 = (
/* Call service routine */

thread_mutate (ServPort, new_state));

/* Gather reply arguments */

#if FastAssign
	RepMsgP->IPCNam3 = 02000220002L;
#else FastAssign
	RepMsgP->IPCNam3 =        0;
	RepMsgP->IPCNam3 |= _InLine(  TRUE);
	RepMsgP->IPCNam3 |= _Deallocate( FALSE);
	RepMsgP->IPCNam3 |= _LongForm( FALSE);
	RepMsgP->IPCNam3 |= _TypeName(TypeInt32);
	RepMsgP->IPCNam3 |= _TypeSizeInBits(32);
	RepMsgP->IPCNam3 |= _NumObjects(1);
#endif FastAssign
 } 
 static void Xvm_allocate(InP, ReP) caddr_t InP, ReP;
 { 

/* Define local messages and types */

 typedef  struct { 
Msg	Head;
TypeType IPCNam2;
vm_address_t	Arg2;
TypeType IPCNam3;
vm_size_t	Arg3;
TypeType IPCNam4;
boolean_t	Arg4;
 } MyMessage;

 typedef  struct { 
Msg	Head;
TypeType	RetCodeType;
 Integer 	RetCode;
TypeType IPCNam2;
vm_address_t	Arg2;
TypeType IPCNam5;
kern_return_t	Arg5;
 } RepMessage;
 typedef MyMessage * ptrMyMsg;
 typedef RepMessage * ptrRepMsg;
ptrMyMsg MyMsgP;
ptrRepMsg RepMsgP;

vm_map_t	ServPort;
vm_address_t	address;
vm_size_t	size;
boolean_t	anywhere;
 extern kern_return_t	vm_allocate ( );


MyMsgP = (ptrMyMsg) (InP);
RepMsgP = (ptrRepMsg) (ReP);
RepMsgP->RetCode = Success;
RepMsgP->Head.SimpleMsg = TRUE;
RepMsgP->Head.MsgSize =  sizeof (RepMessage);
ServPort = convert_port_to_map(MyMsgP->Head.RemotePort);

/* Extract function arguments from message */


#if TypeCheck
	 if (_X_TypeName(MyMsgP->IPCNam2) != TypeInt32)
		 { 
		RepMsg.RetCode = WRONGARGS;
		 return ;
		 } 
#endif TypeCheck
	address = (MyMsgP->Arg2);

#if TypeCheck
	 if (_X_TypeName(MyMsgP->IPCNam3) != TypeInt32)
		 { 
		RepMsg.RetCode = WRONGARGS;
		 return ;
		 } 
#endif TypeCheck
	size = (MyMsgP->Arg3);

#if TypeCheck
	 if (_X_TypeName(MyMsgP->IPCNam4) != TypeBoolean)
		 { 
		RepMsg.RetCode = WRONGARGS;
		 return ;
		 } 
#endif TypeCheck
	anywhere = (MyMsgP->Arg4);

RepMsgP->Arg5 = (
/* Call service routine */

vm_allocate (ServPort,  & address, size, anywhere));

/* Gather reply arguments */

#if FastAssign
	RepMsgP->IPCNam2 = 02000220002L;
#else FastAssign
	RepMsgP->IPCNam2 =        0;
	RepMsgP->IPCNam2 |= _InLine(  TRUE);
	RepMsgP->IPCNam2 |= _Deallocate( FALSE);
	RepMsgP->IPCNam2 |= _LongForm( FALSE);
	RepMsgP->IPCNam2 |= _TypeName(TypeInt32);
	RepMsgP->IPCNam2 |= _TypeSizeInBits(32);
	RepMsgP->IPCNam2 |= _NumObjects(1);
#endif FastAssign
	RepMsgP->Arg2 = (address);
#if FastAssign
	RepMsgP->IPCNam5 = 02000220002L;
#else FastAssign
	RepMsgP->IPCNam5 =        0;
	RepMsgP->IPCNam5 |= _InLine(  TRUE);
	RepMsgP->IPCNam5 |= _Deallocate( FALSE);
	RepMsgP->IPCNam5 |= _LongForm( FALSE);
	RepMsgP->IPCNam5 |= _TypeName(TypeInt32);
	RepMsgP->IPCNam5 |= _TypeSizeInBits(32);
	RepMsgP->IPCNam5 |= _NumObjects(1);
#endif FastAssign
 } 
 static void Xvm_allocate_with_pager(InP, ReP) caddr_t InP, ReP;
 { 

/* Define local messages and types */

 typedef  struct { 
Msg	Head;
TypeType IPCNam2;
vm_address_t	Arg2;
TypeType IPCNam3;
vm_size_t	Arg3;
TypeType IPCNam4;
boolean_t	Arg4;
TypeType IPCNam5;
port_t	Arg5;
TypeType IPCNam6;
vm_pager_id_t	Arg6;
TypeType IPCNam7;
vm_offset_t	Arg7;
 } MyMessage;

 typedef  struct { 
Msg	Head;
TypeType	RetCodeType;
 Integer 	RetCode;
TypeType IPCNam2;
vm_address_t	Arg2;
TypeType IPCNam8;
kern_return_t	Arg8;
 } RepMessage;
 typedef MyMessage * ptrMyMsg;
 typedef RepMessage * ptrRepMsg;
ptrMyMsg MyMsgP;
ptrRepMsg RepMsgP;

vm_map_t	ServPort;
vm_address_t	address;
vm_size_t	size;
boolean_t	anywhere;
port_t	pager_port;
vm_pager_id_t	pager_space;
vm_offset_t	pager_offset;
 extern kern_return_t	vm_allocate_with_pager ( );


MyMsgP = (ptrMyMsg) (InP);
RepMsgP = (ptrRepMsg) (ReP);
RepMsgP->RetCode = Success;
RepMsgP->Head.SimpleMsg = TRUE;
RepMsgP->Head.MsgSize =  sizeof (RepMessage);
ServPort = convert_port_to_map(MyMsgP->Head.RemotePort);

/* Extract function arguments from message */


#if TypeCheck
	 if (_X_TypeName(MyMsgP->IPCNam2) != TypeInt32)
		 { 
		RepMsg.RetCode = WRONGARGS;
		 return ;
		 } 
#endif TypeCheck
	address = (MyMsgP->Arg2);

#if TypeCheck
	 if (_X_TypeName(MyMsgP->IPCNam3) != TypeInt32)
		 { 
		RepMsg.RetCode = WRONGARGS;
		 return ;
		 } 
#endif TypeCheck
	size = (MyMsgP->Arg3);

#if TypeCheck
	 if (_X_TypeName(MyMsgP->IPCNam4) != TypeBoolean)
		 { 
		RepMsg.RetCode = WRONGARGS;
		 return ;
		 } 
#endif TypeCheck
	anywhere = (MyMsgP->Arg4);

#if TypeCheck
	 if (_X_TypeName(MyMsgP->IPCNam5) != TypePt)
		 { 
		RepMsg.RetCode = WRONGARGS;
		 return ;
		 } 
#endif TypeCheck
	pager_port = (MyMsgP->Arg5);

#if TypeCheck
	 if (_X_TypeName(MyMsgP->IPCNam6) != TypeInt32)
		 { 
		RepMsg.RetCode = WRONGARGS;
		 return ;
		 } 
#endif TypeCheck
	pager_space = (MyMsgP->Arg6);

#if TypeCheck
	 if (_X_TypeName(MyMsgP->IPCNam7) != TypeInt32)
		 { 
		RepMsg.RetCode = WRONGARGS;
		 return ;
		 } 
#endif TypeCheck
	pager_offset = (MyMsgP->Arg7);

RepMsgP->Arg8 = (
/* Call service routine */

vm_allocate_with_pager (ServPort,  & address, size, anywhere, pager_port, pager_space, pager_offset));

/* Gather reply arguments */

#if FastAssign
	RepMsgP->IPCNam2 = 02000220002L;
#else FastAssign
	RepMsgP->IPCNam2 =        0;
	RepMsgP->IPCNam2 |= _InLine(  TRUE);
	RepMsgP->IPCNam2 |= _Deallocate( FALSE);
	RepMsgP->IPCNam2 |= _LongForm( FALSE);
	RepMsgP->IPCNam2 |= _TypeName(TypeInt32);
	RepMsgP->IPCNam2 |= _TypeSizeInBits(32);
	RepMsgP->IPCNam2 |= _NumObjects(1);
#endif FastAssign
	RepMsgP->Arg2 = (address);
#if FastAssign
	RepMsgP->IPCNam8 = 02000220002L;
#else FastAssign
	RepMsgP->IPCNam8 =        0;
	RepMsgP->IPCNam8 |= _InLine(  TRUE);
	RepMsgP->IPCNam8 |= _Deallocate( FALSE);
	RepMsgP->IPCNam8 |= _LongForm( FALSE);
	RepMsgP->IPCNam8 |= _TypeName(TypeInt32);
	RepMsgP->IPCNam8 |= _TypeSizeInBits(32);
	RepMsgP->IPCNam8 |= _NumObjects(1);
#endif FastAssign
 } 
 static void Xvm_deallocate(InP, ReP) caddr_t InP, ReP;
 { 

/* Define local messages and types */

 typedef  struct { 
Msg	Head;
TypeType IPCNam2;
vm_address_t	Arg2;
TypeType IPCNam3;
vm_size_t	Arg3;
 } MyMessage;

 typedef  struct { 
Msg	Head;
TypeType	RetCodeType;
 Integer 	RetCode;
TypeType IPCNam4;
kern_return_t	Arg4;
 } RepMessage;
 typedef MyMessage * ptrMyMsg;
 typedef RepMessage * ptrRepMsg;
ptrMyMsg MyMsgP;
ptrRepMsg RepMsgP;

vm_map_t	ServPort;
vm_address_t	address;
vm_size_t	size;
 extern kern_return_t	vm_deallocate ( );


MyMsgP = (ptrMyMsg) (InP);
RepMsgP = (ptrRepMsg) (ReP);
RepMsgP->RetCode = Success;
RepMsgP->Head.SimpleMsg = TRUE;
RepMsgP->Head.MsgSize =  sizeof (RepMessage);
ServPort = convert_port_to_map(MyMsgP->Head.RemotePort);

/* Extract function arguments from message */


#if TypeCheck
	 if (_X_TypeName(MyMsgP->IPCNam2) != TypeInt32)
		 { 
		RepMsg.RetCode = WRONGARGS;
		 return ;
		 } 
#endif TypeCheck
	address = (MyMsgP->Arg2);

#if TypeCheck
	 if (_X_TypeName(MyMsgP->IPCNam3) != TypeInt32)
		 { 
		RepMsg.RetCode = WRONGARGS;
		 return ;
		 } 
#endif TypeCheck
	size = (MyMsgP->Arg3);

RepMsgP->Arg4 = (
/* Call service routine */

vm_deallocate (ServPort, address, size));

/* Gather reply arguments */

#if FastAssign
	RepMsgP->IPCNam4 = 02000220002L;
#else FastAssign
	RepMsgP->IPCNam4 =        0;
	RepMsgP->IPCNam4 |= _InLine(  TRUE);
	RepMsgP->IPCNam4 |= _Deallocate( FALSE);
	RepMsgP->IPCNam4 |= _LongForm( FALSE);
	RepMsgP->IPCNam4 |= _TypeName(TypeInt32);
	RepMsgP->IPCNam4 |= _TypeSizeInBits(32);
	RepMsgP->IPCNam4 |= _NumObjects(1);
#endif FastAssign
 } 
 static void Xvm_protect(InP, ReP) caddr_t InP, ReP;
 { 

/* Define local messages and types */

 typedef  struct { 
Msg	Head;
TypeType IPCNam2;
vm_address_t	Arg2;
TypeType IPCNam3;
vm_size_t	Arg3;
TypeType IPCNam4;
boolean_t	Arg4;
TypeType IPCNam5;
vm_prot_t	Arg5;
 } MyMessage;

 typedef  struct { 
Msg	Head;
TypeType	RetCodeType;
 Integer 	RetCode;
TypeType IPCNam6;
kern_return_t	Arg6;
 } RepMessage;
 typedef MyMessage * ptrMyMsg;
 typedef RepMessage * ptrRepMsg;
ptrMyMsg MyMsgP;
ptrRepMsg RepMsgP;

vm_map_t	ServPort;
vm_address_t	address;
vm_size_t	size;
boolean_t	set_maximum;
vm_prot_t	new_protection;
 extern kern_return_t	vm_protect ( );


MyMsgP = (ptrMyMsg) (InP);
RepMsgP = (ptrRepMsg) (ReP);
RepMsgP->RetCode = Success;
RepMsgP->Head.SimpleMsg = TRUE;
RepMsgP->Head.MsgSize =  sizeof (RepMessage);
ServPort = convert_port_to_map(MyMsgP->Head.RemotePort);

/* Extract function arguments from message */


#if TypeCheck
	 if (_X_TypeName(MyMsgP->IPCNam2) != TypeInt32)
		 { 
		RepMsg.RetCode = WRONGARGS;
		 return ;
		 } 
#endif TypeCheck
	address = (MyMsgP->Arg2);

#if TypeCheck
	 if (_X_TypeName(MyMsgP->IPCNam3) != TypeInt32)
		 { 
		RepMsg.RetCode = WRONGARGS;
		 return ;
		 } 
#endif TypeCheck
	size = (MyMsgP->Arg3);

#if TypeCheck
	 if (_X_TypeName(MyMsgP->IPCNam4) != TypeBoolean)
		 { 
		RepMsg.RetCode = WRONGARGS;
		 return ;
		 } 
#endif TypeCheck
	set_maximum = (MyMsgP->Arg4);

#if TypeCheck
	 if (_X_TypeName(MyMsgP->IPCNam5) != TypeInt32)
		 { 
		RepMsg.RetCode = WRONGARGS;
		 return ;
		 } 
#endif TypeCheck
	new_protection = (MyMsgP->Arg5);

RepMsgP->Arg6 = (
/* Call service routine */

vm_protect (ServPort, address, size, set_maximum, new_protection));

/* Gather reply arguments */

#if FastAssign
	RepMsgP->IPCNam6 = 02000220002L;
#else FastAssign
	RepMsgP->IPCNam6 =        0;
	RepMsgP->IPCNam6 |= _InLine(  TRUE);
	RepMsgP->IPCNam6 |= _Deallocate( FALSE);
	RepMsgP->IPCNam6 |= _LongForm( FALSE);
	RepMsgP->IPCNam6 |= _TypeName(TypeInt32);
	RepMsgP->IPCNam6 |= _TypeSizeInBits(32);
	RepMsgP->IPCNam6 |= _NumObjects(1);
#endif FastAssign
 } 
 static void Xvm_inherit(InP, ReP) caddr_t InP, ReP;
 { 

/* Define local messages and types */

 typedef  struct { 
Msg	Head;
TypeType IPCNam2;
vm_address_t	Arg2;
TypeType IPCNam3;
vm_size_t	Arg3;
TypeType IPCNam4;
vm_inherit_t	Arg4;
 } MyMessage;

 typedef  struct { 
Msg	Head;
TypeType	RetCodeType;
 Integer 	RetCode;
TypeType IPCNam5;
kern_return_t	Arg5;
 } RepMessage;
 typedef MyMessage * ptrMyMsg;
 typedef RepMessage * ptrRepMsg;
ptrMyMsg MyMsgP;
ptrRepMsg RepMsgP;

vm_map_t	ServPort;
vm_address_t	address;
vm_size_t	size;
vm_inherit_t	new_inheritance;
 extern kern_return_t	vm_inherit ( );


MyMsgP = (ptrMyMsg) (InP);
RepMsgP = (ptrRepMsg) (ReP);
RepMsgP->RetCode = Success;
RepMsgP->Head.SimpleMsg = TRUE;
RepMsgP->Head.MsgSize =  sizeof (RepMessage);
ServPort = convert_port_to_map(MyMsgP->Head.RemotePort);

/* Extract function arguments from message */


#if TypeCheck
	 if (_X_TypeName(MyMsgP->IPCNam2) != TypeInt32)
		 { 
		RepMsg.RetCode = WRONGARGS;
		 return ;
		 } 
#endif TypeCheck
	address = (MyMsgP->Arg2);

#if TypeCheck
	 if (_X_TypeName(MyMsgP->IPCNam3) != TypeInt32)
		 { 
		RepMsg.RetCode = WRONGARGS;
		 return ;
		 } 
#endif TypeCheck
	size = (MyMsgP->Arg3);

#if TypeCheck
	 if (_X_TypeName(MyMsgP->IPCNam4) != TypeInt32)
		 { 
		RepMsg.RetCode = WRONGARGS;
		 return ;
		 } 
#endif TypeCheck
	new_inheritance = (MyMsgP->Arg4);

RepMsgP->Arg5 = (
/* Call service routine */

vm_inherit (ServPort, address, size, new_inheritance));

/* Gather reply arguments */

#if FastAssign
	RepMsgP->IPCNam5 = 02000220002L;
#else FastAssign
	RepMsgP->IPCNam5 =        0;
	RepMsgP->IPCNam5 |= _InLine(  TRUE);
	RepMsgP->IPCNam5 |= _Deallocate( FALSE);
	RepMsgP->IPCNam5 |= _LongForm( FALSE);
	RepMsgP->IPCNam5 |= _TypeName(TypeInt32);
	RepMsgP->IPCNam5 |= _TypeSizeInBits(32);
	RepMsgP->IPCNam5 |= _NumObjects(1);
#endif FastAssign
 } 
 static void Xvm_read(InP, ReP) caddr_t InP, ReP;
 { 

/* Define local messages and types */

 typedef  struct { 
Msg	Head;
TypeType IPCNam2;
vm_address_t	Arg2;
TypeType IPCNam3;
vm_size_t	Arg3;
 } MyMessage;

 typedef  struct { 
Msg	Head;
TypeType	RetCodeType;
 Integer 	RetCode;
TypeType IPCNam4;
 Integer 	TName4;
 Integer 	TSize4;
 Long 	NumElts4;
 caddr_t 	Arg4;
TypeType IPCNam5;
kern_return_t	Arg5;
 } RepMessage;
 typedef MyMessage * ptrMyMsg;
 typedef RepMessage * ptrRepMsg;
ptrMyMsg MyMsgP;
ptrRepMsg RepMsgP;

vm_map_t	ServPort;
vm_address_t	address;
vm_size_t	size;
pointer_t	data;
 Long data_Cnt;
 extern kern_return_t	vm_read ( );


MyMsgP = (ptrMyMsg) (InP);
RepMsgP = (ptrRepMsg) (ReP);
RepMsgP->RetCode = Success;
RepMsgP->Head.SimpleMsg = FALSE;
RepMsgP->Head.MsgSize =  sizeof (RepMessage);
ServPort = convert_port_to_map(MyMsgP->Head.RemotePort);

/* Extract function arguments from message */


#if TypeCheck
	 if (_X_TypeName(MyMsgP->IPCNam2) != TypeInt32)
		 { 
		RepMsg.RetCode = WRONGARGS;
		 return ;
		 } 
#endif TypeCheck
	address = (MyMsgP->Arg2);

#if TypeCheck
	 if (_X_TypeName(MyMsgP->IPCNam3) != TypeInt32)
		 { 
		RepMsg.RetCode = WRONGARGS;
		 return ;
		 } 
#endif TypeCheck
	size = (MyMsgP->Arg3);

RepMsgP->Arg5 = (
/* Call service routine */

vm_read (ServPort, address, size,  & data,  & data_Cnt));

/* Gather reply arguments */

#if FastAssign
	RepMsgP->IPCNam4 = 04000000000L;
#else FastAssign
	RepMsgP->IPCNam4 =        0;
	RepMsgP->IPCNam4 |= _InLine( FALSE);
	RepMsgP->IPCNam4 |= _Deallocate( FALSE);
	RepMsgP->IPCNam4 |= _LongForm(  TRUE);
#endif FastAssign
	RepMsgP->TName4 = TypeInt8;
	RepMsgP->TSize4 = 8;
	RepMsgP->NumElts4 = (data_Cnt);
	RepMsgP->Arg4 = 	( caddr_t )(data);
#if FastAssign
	RepMsgP->IPCNam5 = 02000220002L;
#else FastAssign
	RepMsgP->IPCNam5 =        0;
	RepMsgP->IPCNam5 |= _InLine(  TRUE);
	RepMsgP->IPCNam5 |= _Deallocate( FALSE);
	RepMsgP->IPCNam5 |= _LongForm( FALSE);
	RepMsgP->IPCNam5 |= _TypeName(TypeInt32);
	RepMsgP->IPCNam5 |= _TypeSizeInBits(32);
	RepMsgP->IPCNam5 |= _NumObjects(1);
#endif FastAssign
 } 
 static void Xvm_write(InP, ReP) caddr_t InP, ReP;
 { 

/* Define local messages and types */

 typedef  struct { 
Msg	Head;
TypeType IPCNam2;
vm_address_t	Arg2;
TypeType IPCNam3;
 Integer 	TName3;
 Integer 	TSize3;
 Long 	NumElts3;
 caddr_t 	Arg3;
 } MyMessage;

 typedef  struct { 
Msg	Head;
TypeType	RetCodeType;
 Integer 	RetCode;
TypeType IPCNam4;
kern_return_t	Arg4;
 } RepMessage;
 typedef MyMessage * ptrMyMsg;
 typedef RepMessage * ptrRepMsg;
ptrMyMsg MyMsgP;
ptrRepMsg RepMsgP;

vm_map_t	ServPort;
vm_address_t	address;
pointer_t	data;
 Long data_Cnt;
 extern kern_return_t	vm_write ( );


MyMsgP = (ptrMyMsg) (InP);
RepMsgP = (ptrRepMsg) (ReP);
RepMsgP->RetCode = Success;
RepMsgP->Head.SimpleMsg = TRUE;
RepMsgP->Head.MsgSize =  sizeof (RepMessage);
ServPort = convert_port_to_map(MyMsgP->Head.RemotePort);

/* Extract function arguments from message */


#if TypeCheck
	 if (_X_TypeName(MyMsgP->IPCNam2) != TypeInt32)
		 { 
		RepMsg.RetCode = WRONGARGS;
		 return ;
		 } 
#endif TypeCheck
	address = (MyMsgP->Arg2);

#if TypeCheck
	 if (MyMsgP->TName3 != TypeInt8)
		 { 
		RepMsg.RetCode = WRONGARGS;
		 return ;
		 } 
#endif TypeCheck
	data_Cnt = MyMsgP->NumElts3;
	data = ((pointer_t)MyMsgP->Arg3);

RepMsgP->Arg4 = (
/* Call service routine */

vm_write (ServPort, address, data, data_Cnt));

/* Gather reply arguments */

#if FastAssign
	RepMsgP->IPCNam4 = 02000220002L;
#else FastAssign
	RepMsgP->IPCNam4 =        0;
	RepMsgP->IPCNam4 |= _InLine(  TRUE);
	RepMsgP->IPCNam4 |= _Deallocate( FALSE);
	RepMsgP->IPCNam4 |= _LongForm( FALSE);
	RepMsgP->IPCNam4 |= _TypeName(TypeInt32);
	RepMsgP->IPCNam4 |= _TypeSizeInBits(32);
	RepMsgP->IPCNam4 |= _NumObjects(1);
#endif FastAssign
 } 
 static void Xvm_copy(InP, ReP) caddr_t InP, ReP;
 { 

/* Define local messages and types */

 typedef  struct { 
Msg	Head;
TypeType IPCNam2;
vm_address_t	Arg2;
TypeType IPCNam3;
vm_size_t	Arg3;
TypeType IPCNam4;
vm_address_t	Arg4;
 } MyMessage;

 typedef  struct { 
Msg	Head;
TypeType	RetCodeType;
 Integer 	RetCode;
TypeType IPCNam5;
kern_return_t	Arg5;
 } RepMessage;
 typedef MyMessage * ptrMyMsg;
 typedef RepMessage * ptrRepMsg;
ptrMyMsg MyMsgP;
ptrRepMsg RepMsgP;

vm_map_t	ServPort;
vm_address_t	source_address;
vm_size_t	size;
vm_address_t	dest_address;
 extern kern_return_t	vm_copy ( );


MyMsgP = (ptrMyMsg) (InP);
RepMsgP = (ptrRepMsg) (ReP);
RepMsgP->RetCode = Success;
RepMsgP->Head.SimpleMsg = TRUE;
RepMsgP->Head.MsgSize =  sizeof (RepMessage);
ServPort = convert_port_to_map(MyMsgP->Head.RemotePort);

/* Extract function arguments from message */


#if TypeCheck
	 if (_X_TypeName(MyMsgP->IPCNam2) != TypeInt32)
		 { 
		RepMsg.RetCode = WRONGARGS;
		 return ;
		 } 
#endif TypeCheck
	source_address = (MyMsgP->Arg2);

#if TypeCheck
	 if (_X_TypeName(MyMsgP->IPCNam3) != TypeInt32)
		 { 
		RepMsg.RetCode = WRONGARGS;
		 return ;
		 } 
#endif TypeCheck
	size = (MyMsgP->Arg3);

#if TypeCheck
	 if (_X_TypeName(MyMsgP->IPCNam4) != TypeInt32)
		 { 
		RepMsg.RetCode = WRONGARGS;
		 return ;
		 } 
#endif TypeCheck
	dest_address = (MyMsgP->Arg4);

RepMsgP->Arg5 = (
/* Call service routine */

vm_copy (ServPort, source_address, size, dest_address));

/* Gather reply arguments */

#if FastAssign
	RepMsgP->IPCNam5 = 02000220002L;
#else FastAssign
	RepMsgP->IPCNam5 =        0;
	RepMsgP->IPCNam5 |= _InLine(  TRUE);
	RepMsgP->IPCNam5 |= _Deallocate( FALSE);
	RepMsgP->IPCNam5 |= _LongForm( FALSE);
	RepMsgP->IPCNam5 |= _TypeName(TypeInt32);
	RepMsgP->IPCNam5 |= _TypeSizeInBits(32);
	RepMsgP->IPCNam5 |= _NumObjects(1);
#endif FastAssign
 } 
 static void Xvm_regions(InP, ReP) caddr_t InP, ReP;
 { 

/* Define local messages and types */

 typedef  struct { 
Msg	Head;
TypeType IPCNam2;
vm_address_t	Arg2;
TypeType IPCNam3;
vm_size_t	Arg3;
 } MyMessage;

 typedef  struct { 
Msg	Head;
TypeType	RetCodeType;
 Integer 	RetCode;
TypeType IPCNam4;
 Integer 	TName4;
 Integer 	TSize4;
 Long 	NumElts4;
 caddr_t 	Arg4;
TypeType IPCNam5;
kern_return_t	Arg5;
 } RepMessage;
 typedef MyMessage * ptrMyMsg;
 typedef RepMessage * ptrRepMsg;
ptrMyMsg MyMsgP;
ptrRepMsg RepMsgP;

vm_map_t	ServPort;
vm_address_t	start;
vm_size_t	size;
vm_region_array_t	regions;
 Long regions_Cnt;
 extern kern_return_t	vm_regions ( );


MyMsgP = (ptrMyMsg) (InP);
RepMsgP = (ptrRepMsg) (ReP);
RepMsgP->RetCode = Success;
RepMsgP->Head.SimpleMsg = FALSE;
RepMsgP->Head.MsgSize =  sizeof (RepMessage);
ServPort = convert_port_to_map(MyMsgP->Head.RemotePort);

/* Extract function arguments from message */


#if TypeCheck
	 if (_X_TypeName(MyMsgP->IPCNam2) != TypeInt32)
		 { 
		RepMsg.RetCode = WRONGARGS;
		 return ;
		 } 
#endif TypeCheck
	start = (MyMsgP->Arg2);

#if TypeCheck
	 if (_X_TypeName(MyMsgP->IPCNam3) != TypeInt32)
		 { 
		RepMsg.RetCode = WRONGARGS;
		 return ;
		 } 
#endif TypeCheck
	size = (MyMsgP->Arg3);

RepMsgP->Arg5 = (
/* Call service routine */

vm_regions (ServPort, start, size,  & regions,  & regions_Cnt));

/* Gather reply arguments */

#if FastAssign
	RepMsgP->IPCNam4 = 04000000000L;
#else FastAssign
	RepMsgP->IPCNam4 =        0;
	RepMsgP->IPCNam4 |= _InLine( FALSE);
	RepMsgP->IPCNam4 |= _Deallocate( FALSE);
	RepMsgP->IPCNam4 |= _LongForm(  TRUE);
#endif FastAssign
	RepMsgP->TName4 = TypeInt32;
	RepMsgP->TSize4 = 32;
	RepMsgP->NumElts4 = (regions_Cnt);
	RepMsgP->Arg4 = 	( caddr_t )(regions);
#if FastAssign
	RepMsgP->IPCNam5 = 02000220002L;
#else FastAssign
	RepMsgP->IPCNam5 =        0;
	RepMsgP->IPCNam5 |= _InLine(  TRUE);
	RepMsgP->IPCNam5 |= _Deallocate( FALSE);
	RepMsgP->IPCNam5 |= _LongForm( FALSE);
	RepMsgP->IPCNam5 |= _TypeName(TypeInt32);
	RepMsgP->IPCNam5 |= _TypeSizeInBits(32);
	RepMsgP->IPCNam5 |= _NumObjects(1);
#endif FastAssign
 } 
 static void Xvm_statistics(InP, ReP) caddr_t InP, ReP;
 { 

/* Define local messages and types */

 typedef  struct { 
Msg	Head;
 } MyMessage;

 typedef  struct { 
Msg	Head;
TypeType	RetCodeType;
 Integer 	RetCode;
TypeType IPCNam2;
vm_statistics_data_t	Arg2;
TypeType IPCNam3;
kern_return_t	Arg3;
 } RepMessage;
 typedef MyMessage * ptrMyMsg;
 typedef RepMessage * ptrRepMsg;
ptrMyMsg MyMsgP;
ptrRepMsg RepMsgP;

vm_map_t	ServPort;
vm_statistics_data_t	vm_stats;
 extern kern_return_t	vm_statistics ( );


MyMsgP = (ptrMyMsg) (InP);
RepMsgP = (ptrRepMsg) (ReP);
RepMsgP->RetCode = Success;
RepMsgP->Head.SimpleMsg = TRUE;
RepMsgP->Head.MsgSize =  sizeof (RepMessage);
ServPort = convert_port_to_map(MyMsgP->Head.RemotePort);

/* Extract function arguments from message */


RepMsgP->Arg3 = (
/* Call service routine */

vm_statistics (ServPort,  & vm_stats));

/* Gather reply arguments */

#if FastAssign
	RepMsgP->IPCNam2 = 02003220002L;
#else FastAssign
	RepMsgP->IPCNam2 =        0;
	RepMsgP->IPCNam2 |= _InLine(  TRUE);
	RepMsgP->IPCNam2 |= _Deallocate( FALSE);
	RepMsgP->IPCNam2 |= _LongForm( FALSE);
	RepMsgP->IPCNam2 |= _TypeName(TypeInt32);
	RepMsgP->IPCNam2 |= _TypeSizeInBits(32);
	RepMsgP->IPCNam2 |= _NumObjects(13);
#endif FastAssign
	RepMsgP->Arg2 = (vm_stats);
#if FastAssign
	RepMsgP->IPCNam3 = 02000220002L;
#else FastAssign
	RepMsgP->IPCNam3 =        0;
	RepMsgP->IPCNam3 |= _InLine(  TRUE);
	RepMsgP->IPCNam3 |= _Deallocate( FALSE);
	RepMsgP->IPCNam3 |= _LongForm( FALSE);
	RepMsgP->IPCNam3 |= _TypeName(TypeInt32);
	RepMsgP->IPCNam3 |= _TypeSizeInBits(32);
	RepMsgP->IPCNam3 |= _NumObjects(1);
#endif FastAssign
 } 
 static void Xpager_clean(InP, ReP) caddr_t InP, ReP;
 { 

/* Define local messages and types */

 typedef  struct { 
Msg	Head;
TypeType IPCNam2;
vm_pager_id_t	Arg2;
TypeType IPCNam3;
vm_offset_t	Arg3;
TypeType IPCNam4;
vm_size_t	Arg4;
 } MyMessage;

 typedef  struct { 
Msg	Head;
TypeType	RetCodeType;
 Integer 	RetCode;
TypeType IPCNam5;
kern_return_t	Arg5;
 } RepMessage;
 typedef MyMessage * ptrMyMsg;
 typedef RepMessage * ptrRepMsg;
ptrMyMsg MyMsgP;
ptrRepMsg RepMsgP;

port_t	ServPort;
vm_pager_id_t	pager_space;
vm_offset_t	offset;
vm_size_t	size;
 extern kern_return_t	pager_clean ( );


MyMsgP = (ptrMyMsg) (InP);
RepMsgP = (ptrRepMsg) (ReP);
RepMsgP->RetCode = Success;
RepMsgP->Head.SimpleMsg = TRUE;
RepMsgP->Head.MsgSize =  sizeof (RepMessage);
ServPort = (MyMsgP->Head.LocalPort);

/* Extract function arguments from message */


#if TypeCheck
	 if (_X_TypeName(MyMsgP->IPCNam2) != TypeInt32)
		 { 
		RepMsg.RetCode = WRONGARGS;
		 return ;
		 } 
#endif TypeCheck
	pager_space = (MyMsgP->Arg2);

#if TypeCheck
	 if (_X_TypeName(MyMsgP->IPCNam3) != TypeInt32)
		 { 
		RepMsg.RetCode = WRONGARGS;
		 return ;
		 } 
#endif TypeCheck
	offset = (MyMsgP->Arg3);

#if TypeCheck
	 if (_X_TypeName(MyMsgP->IPCNam4) != TypeInt32)
		 { 
		RepMsg.RetCode = WRONGARGS;
		 return ;
		 } 
#endif TypeCheck
	size = (MyMsgP->Arg4);

RepMsgP->Arg5 = (
/* Call service routine */

pager_clean (ServPort, pager_space, offset, size));

/* Gather reply arguments */

#if FastAssign
	RepMsgP->IPCNam5 = 02000220002L;
#else FastAssign
	RepMsgP->IPCNam5 =        0;
	RepMsgP->IPCNam5 |= _InLine(  TRUE);
	RepMsgP->IPCNam5 |= _Deallocate( FALSE);
	RepMsgP->IPCNam5 |= _LongForm( FALSE);
	RepMsgP->IPCNam5 |= _TypeName(TypeInt32);
	RepMsgP->IPCNam5 |= _TypeSizeInBits(32);
	RepMsgP->IPCNam5 |= _NumObjects(1);
#endif FastAssign
 } 
 static void Xpager_fetch(InP, ReP) caddr_t InP, ReP;
 { 

/* Define local messages and types */

 typedef  struct { 
Msg	Head;
TypeType IPCNam2;
vm_pager_id_t	Arg2;
TypeType IPCNam3;
vm_offset_t	Arg3;
TypeType IPCNam4;
vm_size_t	Arg4;
 } MyMessage;

 typedef  struct { 
Msg	Head;
TypeType	RetCodeType;
 Integer 	RetCode;
TypeType IPCNam5;
kern_return_t	Arg5;
 } RepMessage;
 typedef MyMessage * ptrMyMsg;
 typedef RepMessage * ptrRepMsg;
ptrMyMsg MyMsgP;
ptrRepMsg RepMsgP;

port_t	ServPort;
vm_pager_id_t	pager_space;
vm_offset_t	offset;
vm_size_t	size;
 extern kern_return_t	pager_fetch ( );


MyMsgP = (ptrMyMsg) (InP);
RepMsgP = (ptrRepMsg) (ReP);
RepMsgP->RetCode = Success;
RepMsgP->Head.SimpleMsg = TRUE;
RepMsgP->Head.MsgSize =  sizeof (RepMessage);
ServPort = (MyMsgP->Head.LocalPort);

/* Extract function arguments from message */


#if TypeCheck
	 if (_X_TypeName(MyMsgP->IPCNam2) != TypeInt32)
		 { 
		RepMsg.RetCode = WRONGARGS;
		 return ;
		 } 
#endif TypeCheck
	pager_space = (MyMsgP->Arg2);

#if TypeCheck
	 if (_X_TypeName(MyMsgP->IPCNam3) != TypeInt32)
		 { 
		RepMsg.RetCode = WRONGARGS;
		 return ;
		 } 
#endif TypeCheck
	offset = (MyMsgP->Arg3);

#if TypeCheck
	 if (_X_TypeName(MyMsgP->IPCNam4) != TypeInt32)
		 { 
		RepMsg.RetCode = WRONGARGS;
		 return ;
		 } 
#endif TypeCheck
	size = (MyMsgP->Arg4);

RepMsgP->Arg5 = (
/* Call service routine */

pager_fetch (ServPort, pager_space, offset, size));

/* Gather reply arguments */

#if FastAssign
	RepMsgP->IPCNam5 = 02000220002L;
#else FastAssign
	RepMsgP->IPCNam5 =        0;
	RepMsgP->IPCNam5 |= _InLine(  TRUE);
	RepMsgP->IPCNam5 |= _Deallocate( FALSE);
	RepMsgP->IPCNam5 |= _LongForm( FALSE);
	RepMsgP->IPCNam5 |= _TypeName(TypeInt32);
	RepMsgP->IPCNam5 |= _TypeSizeInBits(32);
	RepMsgP->IPCNam5 |= _NumObjects(1);
#endif FastAssign
 } 
 Boolean mach_server(InP, RepP)  caddr_t InP, RepP;
 { 
 typedef  struct { 
Msg	Head;
TypeType	RetCodeType;
 Integer 	RetCode;
 } RepMessage;
 typedef RepMessage  * ptrRepMessage;
ptrMsg	InMsgP;
ptrRepMessage	RepMsgP;

InMsgP = (ptrMsg) (InP);
RepMsgP = (ptrRepMessage) (RepP);
RepMsgP->Head.LocalPort = InMsgP->LocalPort;
RepMsgP->Head.RemotePort = InMsgP->RemotePort;
RepMsgP->Head.ID = InMsgP->ID + 100;
RepMsgP->Head.MsgType = NormalMsg;
#if	FastAssign
( * ( Long )( & RepMsgP->RetCodeType) =       268505089;
#else	FastAssign
RepMsgP->RetCodeType = 0;
RepMsgP->RetCodeType |= _TypeName (TypeInt16);
RepMsgP->RetCodeType |= _TypeSizeInBits (16);
RepMsgP->RetCodeType |= _NumObjects (1);
RepMsgP->RetCodeType |= _InLine (TRUE);
RepMsgP->RetCodeType |= _LongForm (FALSE);
RepMsgP->RetCodeType |= _Deallocate (FALSE);
#endif	FastAssign

if ((InMsgP->ID >     1032) || (InMsgP->ID <     1000))
 { 
RepMsgP->RetCode = BadMsgID;
 return (FALSE);
 } 
 else  { 
 switch (InMsgP->ID -     1000)
 { 
 case        0:
Xport_allocate(InP, RepP);
 break; 

 case        1:
Xport_deallocate(InP, RepP);
 break; 

 case        2:
Xport_enable(InP, RepP);
 break; 

 case        3:
Xport_disable(InP, RepP);
 break; 

 case        4:
Xport_select(InP, RepP);
 break; 

 case        5:
Xport_set_backlog(InP, RepP);
 break; 

 case        6:
Xport_status(InP, RepP);
 break; 

 case        7:
Xtask_create(InP, RepP);
 break; 

 case        8:
Xtask_terminate(InP, RepP);
 break; 

 case        9:
Xtask_suspend(InP, RepP);
 break; 

 case       10:
Xtask_resume(InP, RepP);
 break; 

 case       11:
Xtask_threads(InP, RepP);
 break; 

 case       12:
Xtask_ports(InP, RepP);
 break; 

 case       13:
Xtask_status(InP, RepP);
 break; 

 case       14:
Xtask_set_notify(InP, RepP);
 break; 

 case       15:
Xthread_create(InP, RepP);
 break; 

 case       16:
Xthread_terminate(InP, RepP);
 break; 

 case       17:
Xthread_suspend(InP, RepP);
 break; 

 case       18:
Xthread_resume(InP, RepP);
 break; 

 case       19:
Xthread_status(InP, RepP);
 break; 

 case       20:
Xthread_mutate(InP, RepP);
 break; 

 case       21:
Xvm_allocate(InP, RepP);
 break; 

 case       22:
Xvm_allocate_with_pager(InP, RepP);
 break; 

 case       23:
Xvm_deallocate(InP, RepP);
 break; 

 case       24:
Xvm_protect(InP, RepP);
 break; 

 case       25:
Xvm_inherit(InP, RepP);
 break; 

 case       26:
Xvm_read(InP, RepP);
 break; 

 case       27:
Xvm_write(InP, RepP);
 break; 

 case       28:
Xvm_copy(InP, RepP);
 break; 

 case       29:
Xvm_regions(InP, RepP);
 break; 

 case       30:
Xvm_statistics(InP, RepP);
 break; 

 case       31:
Xpager_clean(InP, RepP);
 break; 

 case       32:
Xpager_fetch(InP, RepP);
 break; 

 } 
 } 
 return (TRUE);
 } 
