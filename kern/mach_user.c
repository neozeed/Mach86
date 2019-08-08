#include "mach.h"
#include "mach_msg.h"

#define FastAssign 1
#define TypeCheck 1

static port_t	mach_reply_port;
static GeneralReturn	GR;


void InitMach (reply_port)
port_t		reply_port;
{
port_t		tmp_reply_port;
kern_return_t	result;

if (reply_port != PORT_NULL)
mach_reply_port = reply_port;
else if ((result = port_allocate(task_self(), &tmp_reply_port)) == KERN_SUCCESS)
mach_reply_port = tmp_reply_port;
}

kern_return_t	port_allocate ( ServPort,my_port)
task_t	ServPort;
port_t	 * my_port;
 { 

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
MyMessage  MyMsg;
RepMessage RepMsg;
kern_return_t	port_allocate;

MyMsg.Head.SimpleMsg =	1;
MyMsg.Head.MsgSize = sizeof (MyMsg);
MyMsg.Head.MsgType = NormalMsg;
MyMsg.Head.RemotePort = ServPort;
MyMsg.Head.LocalPort = mach_reply_port;
MyMsg.Head.ID =     1000;
 { 
MyMessage *MyMsgPtr;
MyMsgPtr = & MyMsg;
 } 
RepMsg.Head.MsgSize = sizeof (RepMsg);
RepMsg.Head.LocalPort = mach_reply_port;
if ( (GR = Send (& MyMsg.Head, 0, Wait)) != Success)
MsgError(GR);

if((GR = Receive( & (RepMsg.Head),0,LocalPt,ReceiveIt))!= Success)
MsgError(GR);
 { 
 if (RepMsg.Head.ID !=     1100)
MsgError(BadReply);
#if TypeCheck
 if (_X_TypeName(RepMsg.RetCodeType) != TypeInt16)
MsgError(BadReply);
#endif
	 if (RepMsg.RetCode != Success)
MsgError(BadReply);

#if TypeCheck
	 if (_X_TypeName(RepMsg.IPCNam2) != TypePt)
MsgError(BadReply);
#endif TypeCheck
	 * my_port = (RepMsg.Arg2);

#if TypeCheck
	 if (_X_TypeName(RepMsg.IPCNam3) != TypeInt32)
MsgError(BadReply);
#endif TypeCheck
	port_allocate = (RepMsg.Arg3);
 return (port_allocate);
 } 

 } 

kern_return_t	port_deallocate ( ServPort,my_port,reason)
task_t	ServPort;
port_t	my_port;
int	reason;
 { 

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
MyMessage  MyMsg;
RepMessage RepMsg;
kern_return_t	port_deallocate;

MyMsg.Head.SimpleMsg =	0;
MyMsg.Head.MsgSize = sizeof (MyMsg);
MyMsg.Head.MsgType = NormalMsg;
MyMsg.Head.RemotePort = ServPort;
MyMsg.Head.LocalPort = mach_reply_port;
MyMsg.Head.ID =     1001;
 { 
MyMessage *MyMsgPtr;
MyMsgPtr = & MyMsg;
#if FastAssign
	MyMsgPtr->IPCNam2 = 02000220006L;
#else FastAssign
	MyMsgPtr->IPCNam2 =        0;
	MyMsgPtr->IPCNam2 |= _InLine(  1);
	MyMsgPtr->IPCNam2 |= _Deallocate( 0);
	MyMsgPtr->IPCNam2 |= _LongForm( 0);
	MyMsgPtr->IPCNam2 |= _TypeName(TypePt);
	MyMsgPtr->IPCNam2 |= _TypeSizeInBits(32);
	MyMsgPtr->IPCNam2 |= _NumObjects(1);
#endif FastAssign
	MyMsgPtr->Arg2 = (my_port);
#if FastAssign
	MyMsgPtr->IPCNam3 = 02000220002L;
#else FastAssign
	MyMsgPtr->IPCNam3 =        0;
	MyMsgPtr->IPCNam3 |= _InLine(  1);
	MyMsgPtr->IPCNam3 |= _Deallocate( 0);
	MyMsgPtr->IPCNam3 |= _LongForm( 0);
	MyMsgPtr->IPCNam3 |= _TypeName(TypeInt32);
	MyMsgPtr->IPCNam3 |= _TypeSizeInBits(32);
	MyMsgPtr->IPCNam3 |= _NumObjects(1);
#endif FastAssign
	MyMsgPtr->Arg3 = (reason);
 } 
RepMsg.Head.MsgSize = sizeof (RepMsg);
RepMsg.Head.LocalPort = mach_reply_port;
if ( (GR = Send (& MyMsg.Head, 0, Wait)) != Success)
MsgError(GR);

if((GR = Receive( & (RepMsg.Head),0,LocalPt,ReceiveIt))!= Success)
MsgError(GR);
 { 
 if (RepMsg.Head.ID !=     1101)
MsgError(BadReply);
#if TypeCheck
 if (_X_TypeName(RepMsg.RetCodeType) != TypeInt16)
MsgError(BadReply);
#endif
	 if (RepMsg.RetCode != Success)
MsgError(BadReply);

#if TypeCheck
	 if (_X_TypeName(RepMsg.IPCNam4) != TypeInt32)
MsgError(BadReply);
#endif TypeCheck
	port_deallocate = (RepMsg.Arg4);
 return (port_deallocate);
 } 

 } 

kern_return_t	port_enable ( ServPort,my_port)
task_t	ServPort;
port_t	my_port;
 { 

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
MyMessage  MyMsg;
RepMessage RepMsg;
kern_return_t	port_enable;

MyMsg.Head.SimpleMsg =	0;
MyMsg.Head.MsgSize = sizeof (MyMsg);
MyMsg.Head.MsgType = NormalMsg;
MyMsg.Head.RemotePort = ServPort;
MyMsg.Head.LocalPort = mach_reply_port;
MyMsg.Head.ID =     1002;
 { 
MyMessage *MyMsgPtr;
MyMsgPtr = & MyMsg;
#if FastAssign
	MyMsgPtr->IPCNam2 = 02000220006L;
#else FastAssign
	MyMsgPtr->IPCNam2 =        0;
	MyMsgPtr->IPCNam2 |= _InLine(  1);
	MyMsgPtr->IPCNam2 |= _Deallocate( 0);
	MyMsgPtr->IPCNam2 |= _LongForm( 0);
	MyMsgPtr->IPCNam2 |= _TypeName(TypePt);
	MyMsgPtr->IPCNam2 |= _TypeSizeInBits(32);
	MyMsgPtr->IPCNam2 |= _NumObjects(1);
#endif FastAssign
	MyMsgPtr->Arg2 = (my_port);
 } 
RepMsg.Head.MsgSize = sizeof (RepMsg);
RepMsg.Head.LocalPort = mach_reply_port;
if ( (GR = Send (& MyMsg.Head, 0, Wait)) != Success)
MsgError(GR);

if((GR = Receive( & (RepMsg.Head),0,LocalPt,ReceiveIt))!= Success)
MsgError(GR);
 { 
 if (RepMsg.Head.ID !=     1102)
MsgError(BadReply);
#if TypeCheck
 if (_X_TypeName(RepMsg.RetCodeType) != TypeInt16)
MsgError(BadReply);
#endif
	 if (RepMsg.RetCode != Success)
MsgError(BadReply);

#if TypeCheck
	 if (_X_TypeName(RepMsg.IPCNam3) != TypeInt32)
MsgError(BadReply);
#endif TypeCheck
	port_enable = (RepMsg.Arg3);
 return (port_enable);
 } 

 } 

kern_return_t	port_disable ( ServPort,my_port)
task_t	ServPort;
port_t	my_port;
 { 

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
MyMessage  MyMsg;
RepMessage RepMsg;
kern_return_t	port_disable;

MyMsg.Head.SimpleMsg =	0;
MyMsg.Head.MsgSize = sizeof (MyMsg);
MyMsg.Head.MsgType = NormalMsg;
MyMsg.Head.RemotePort = ServPort;
MyMsg.Head.LocalPort = mach_reply_port;
MyMsg.Head.ID =     1003;
 { 
MyMessage *MyMsgPtr;
MyMsgPtr = & MyMsg;
#if FastAssign
	MyMsgPtr->IPCNam2 = 02000220006L;
#else FastAssign
	MyMsgPtr->IPCNam2 =        0;
	MyMsgPtr->IPCNam2 |= _InLine(  1);
	MyMsgPtr->IPCNam2 |= _Deallocate( 0);
	MyMsgPtr->IPCNam2 |= _LongForm( 0);
	MyMsgPtr->IPCNam2 |= _TypeName(TypePt);
	MyMsgPtr->IPCNam2 |= _TypeSizeInBits(32);
	MyMsgPtr->IPCNam2 |= _NumObjects(1);
#endif FastAssign
	MyMsgPtr->Arg2 = (my_port);
 } 
RepMsg.Head.MsgSize = sizeof (RepMsg);
RepMsg.Head.LocalPort = mach_reply_port;
if ( (GR = Send (& MyMsg.Head, 0, Wait)) != Success)
MsgError(GR);

if((GR = Receive( & (RepMsg.Head),0,LocalPt,ReceiveIt))!= Success)
MsgError(GR);
 { 
 if (RepMsg.Head.ID !=     1103)
MsgError(BadReply);
#if TypeCheck
 if (_X_TypeName(RepMsg.RetCodeType) != TypeInt16)
MsgError(BadReply);
#endif
	 if (RepMsg.RetCode != Success)
MsgError(BadReply);

#if TypeCheck
	 if (_X_TypeName(RepMsg.IPCNam3) != TypeInt32)
MsgError(BadReply);
#endif TypeCheck
	port_disable = (RepMsg.Arg3);
 return (port_disable);
 } 

 } 

kern_return_t	port_select ( ServPort,port_list,port_list_Cnt)
task_t	ServPort;
port_array_t	 * port_list;
 Long  * port_list_Cnt;
 { 

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
MyMessage  MyMsg;
RepMessage RepMsg;
kern_return_t	port_select;

MyMsg.Head.SimpleMsg =	1;
MyMsg.Head.MsgSize = sizeof (MyMsg);
MyMsg.Head.MsgType = NormalMsg;
MyMsg.Head.RemotePort = ServPort;
MyMsg.Head.LocalPort = mach_reply_port;
MyMsg.Head.ID =     1004;
 { 
MyMessage *MyMsgPtr;
MyMsgPtr = & MyMsg;
 } 
RepMsg.Head.MsgSize = sizeof (RepMsg);
RepMsg.Head.LocalPort = mach_reply_port;
if ( (GR = Send (& MyMsg.Head, 0, Wait)) != Success)
MsgError(GR);

if((GR = Receive( & (RepMsg.Head),0,LocalPt,ReceiveIt))!= Success)
MsgError(GR);
 { 
 if (RepMsg.Head.ID !=     1104)
MsgError(BadReply);
#if TypeCheck
 if (_X_TypeName(RepMsg.RetCodeType) != TypeInt16)
MsgError(BadReply);
#endif
	 if (RepMsg.RetCode != Success)
MsgError(BadReply);

#if TypeCheck
	 if (RepMsg.TName2 != TypePt)
MsgError(BadReply);
#endif TypeCheck
	( * port_list_Cnt) = RepMsg.NumElts2;
	 * port_list = ((port_array_t)RepMsg.Arg2);

#if TypeCheck
	 if (_X_TypeName(RepMsg.IPCNam3) != TypeInt32)
MsgError(BadReply);
#endif TypeCheck
	port_select = (RepMsg.Arg3);
 return (port_select);
 } 

 } 

kern_return_t	port_set_backlog ( ServPort,my_port,backlog)
task_t	ServPort;
port_t	my_port;
int	backlog;
 { 

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
MyMessage  MyMsg;
RepMessage RepMsg;
kern_return_t	port_set_backlog;

MyMsg.Head.SimpleMsg =	0;
MyMsg.Head.MsgSize = sizeof (MyMsg);
MyMsg.Head.MsgType = NormalMsg;
MyMsg.Head.RemotePort = ServPort;
MyMsg.Head.LocalPort = mach_reply_port;
MyMsg.Head.ID =     1005;
 { 
MyMessage *MyMsgPtr;
MyMsgPtr = & MyMsg;
#if FastAssign
	MyMsgPtr->IPCNam2 = 02000220006L;
#else FastAssign
	MyMsgPtr->IPCNam2 =        0;
	MyMsgPtr->IPCNam2 |= _InLine(  1);
	MyMsgPtr->IPCNam2 |= _Deallocate( 0);
	MyMsgPtr->IPCNam2 |= _LongForm( 0);
	MyMsgPtr->IPCNam2 |= _TypeName(TypePt);
	MyMsgPtr->IPCNam2 |= _TypeSizeInBits(32);
	MyMsgPtr->IPCNam2 |= _NumObjects(1);
#endif FastAssign
	MyMsgPtr->Arg2 = (my_port);
#if FastAssign
	MyMsgPtr->IPCNam3 = 02000220002L;
#else FastAssign
	MyMsgPtr->IPCNam3 =        0;
	MyMsgPtr->IPCNam3 |= _InLine(  1);
	MyMsgPtr->IPCNam3 |= _Deallocate( 0);
	MyMsgPtr->IPCNam3 |= _LongForm( 0);
	MyMsgPtr->IPCNam3 |= _TypeName(TypeInt32);
	MyMsgPtr->IPCNam3 |= _TypeSizeInBits(32);
	MyMsgPtr->IPCNam3 |= _NumObjects(1);
#endif FastAssign
	MyMsgPtr->Arg3 = (backlog);
 } 
RepMsg.Head.MsgSize = sizeof (RepMsg);
RepMsg.Head.LocalPort = mach_reply_port;
if ( (GR = Send (& MyMsg.Head, 0, Wait)) != Success)
MsgError(GR);

if((GR = Receive( & (RepMsg.Head),0,LocalPt,ReceiveIt))!= Success)
MsgError(GR);
 { 
 if (RepMsg.Head.ID !=     1105)
MsgError(BadReply);
#if TypeCheck
 if (_X_TypeName(RepMsg.RetCodeType) != TypeInt16)
MsgError(BadReply);
#endif
	 if (RepMsg.RetCode != Success)
MsgError(BadReply);

#if TypeCheck
	 if (_X_TypeName(RepMsg.IPCNam4) != TypeInt32)
MsgError(BadReply);
#endif TypeCheck
	port_set_backlog = (RepMsg.Arg4);
 return (port_set_backlog);
 } 

 } 

kern_return_t	port_status ( ServPort,my_port,enabled,num_msgs,backlog)
task_t	ServPort;
port_t	my_port;
boolean_t	 * enabled;
int	 * num_msgs;
int	 * backlog;
 { 

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
MyMessage  MyMsg;
RepMessage RepMsg;
kern_return_t	port_status;

MyMsg.Head.SimpleMsg =	0;
MyMsg.Head.MsgSize = sizeof (MyMsg);
MyMsg.Head.MsgType = NormalMsg;
MyMsg.Head.RemotePort = ServPort;
MyMsg.Head.LocalPort = mach_reply_port;
MyMsg.Head.ID =     1006;
 { 
MyMessage *MyMsgPtr;
MyMsgPtr = & MyMsg;
#if FastAssign
	MyMsgPtr->IPCNam2 = 02000220006L;
#else FastAssign
	MyMsgPtr->IPCNam2 =        0;
	MyMsgPtr->IPCNam2 |= _InLine(  1);
	MyMsgPtr->IPCNam2 |= _Deallocate( 0);
	MyMsgPtr->IPCNam2 |= _LongForm( 0);
	MyMsgPtr->IPCNam2 |= _TypeName(TypePt);
	MyMsgPtr->IPCNam2 |= _TypeSizeInBits(32);
	MyMsgPtr->IPCNam2 |= _NumObjects(1);
#endif FastAssign
	MyMsgPtr->Arg2 = (my_port);
 } 
RepMsg.Head.MsgSize = sizeof (RepMsg);
RepMsg.Head.LocalPort = mach_reply_port;
if ( (GR = Send (& MyMsg.Head, 0, Wait)) != Success)
MsgError(GR);

if((GR = Receive( & (RepMsg.Head),0,LocalPt,ReceiveIt))!= Success)
MsgError(GR);
 { 
 if (RepMsg.Head.ID !=     1106)
MsgError(BadReply);
#if TypeCheck
 if (_X_TypeName(RepMsg.RetCodeType) != TypeInt16)
MsgError(BadReply);
#endif
	 if (RepMsg.RetCode != Success)
MsgError(BadReply);

#if TypeCheck
	 if (_X_TypeName(RepMsg.IPCNam3) != TypeBoolean)
MsgError(BadReply);
#endif TypeCheck
	 * enabled = (RepMsg.Arg3);

#if TypeCheck
	 if (_X_TypeName(RepMsg.IPCNam4) != TypeInt32)
MsgError(BadReply);
#endif TypeCheck
	 * num_msgs = (RepMsg.Arg4);

#if TypeCheck
	 if (_X_TypeName(RepMsg.IPCNam5) != TypeInt32)
MsgError(BadReply);
#endif TypeCheck
	 * backlog = (RepMsg.Arg5);

#if TypeCheck
	 if (_X_TypeName(RepMsg.IPCNam6) != TypeInt32)
MsgError(BadReply);
#endif TypeCheck
	port_status = (RepMsg.Arg6);
 return (port_status);
 } 

 } 

kern_return_t	task_create ( ServPort,inherit_memory,child_task,child_data)
task_t	ServPort;
boolean_t	inherit_memory;
task_t	 * child_task;
port_t	 * child_data;
 { 

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
task_t	Arg3;
TypeType IPCNam4;
port_t	Arg4;
TypeType IPCNam5;
kern_return_t	Arg5;
 } RepMessage;
MyMessage  MyMsg;
RepMessage RepMsg;
kern_return_t	task_create;

MyMsg.Head.SimpleMsg =	1;
MyMsg.Head.MsgSize = sizeof (MyMsg);
MyMsg.Head.MsgType = NormalMsg;
MyMsg.Head.RemotePort = ServPort;
MyMsg.Head.LocalPort = mach_reply_port;
MyMsg.Head.ID =     1007;
 { 
MyMessage *MyMsgPtr;
MyMsgPtr = & MyMsg;
#if FastAssign
	MyMsgPtr->IPCNam2 = 02000210000L;
#else FastAssign
	MyMsgPtr->IPCNam2 =        0;
	MyMsgPtr->IPCNam2 |= _InLine(  1);
	MyMsgPtr->IPCNam2 |= _Deallocate( 0);
	MyMsgPtr->IPCNam2 |= _LongForm( 0);
	MyMsgPtr->IPCNam2 |= _TypeName(TypeBoolean);
	MyMsgPtr->IPCNam2 |= _TypeSizeInBits(16);
	MyMsgPtr->IPCNam2 |= _NumObjects(1);
#endif FastAssign
	MyMsgPtr->Arg2 = (inherit_memory);
 } 
RepMsg.Head.MsgSize = sizeof (RepMsg);
RepMsg.Head.LocalPort = mach_reply_port;
if ( (GR = Send (& MyMsg.Head, 0, Wait)) != Success)
MsgError(GR);

if((GR = Receive( & (RepMsg.Head),0,LocalPt,ReceiveIt))!= Success)
MsgError(GR);
 { 
 if (RepMsg.Head.ID !=     1107)
MsgError(BadReply);
#if TypeCheck
 if (_X_TypeName(RepMsg.RetCodeType) != TypeInt16)
MsgError(BadReply);
#endif
	 if (RepMsg.RetCode != Success)
MsgError(BadReply);

#if TypeCheck
	 if (_X_TypeName(RepMsg.IPCNam3) != TypePt)
MsgError(BadReply);
#endif TypeCheck
	 * child_task = (RepMsg.Arg3);

#if TypeCheck
	 if (_X_TypeName(RepMsg.IPCNam4) != TypePt)
MsgError(BadReply);
#endif TypeCheck
	 * child_data = (RepMsg.Arg4);

#if TypeCheck
	 if (_X_TypeName(RepMsg.IPCNam5) != TypeInt32)
MsgError(BadReply);
#endif TypeCheck
	task_create = (RepMsg.Arg5);
 return (task_create);
 } 

 } 

kern_return_t	task_terminate ( ServPort,reason)
task_t	ServPort;
int	reason;
 { 

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
MyMessage  MyMsg;
RepMessage RepMsg;
kern_return_t	task_terminate;

MyMsg.Head.SimpleMsg =	1;
MyMsg.Head.MsgSize = sizeof (MyMsg);
MyMsg.Head.MsgType = NormalMsg;
MyMsg.Head.RemotePort = ServPort;
MyMsg.Head.LocalPort = mach_reply_port;
MyMsg.Head.ID =     1008;
 { 
MyMessage *MyMsgPtr;
MyMsgPtr = & MyMsg;
#if FastAssign
	MyMsgPtr->IPCNam2 = 02000220002L;
#else FastAssign
	MyMsgPtr->IPCNam2 =        0;
	MyMsgPtr->IPCNam2 |= _InLine(  1);
	MyMsgPtr->IPCNam2 |= _Deallocate( 0);
	MyMsgPtr->IPCNam2 |= _LongForm( 0);
	MyMsgPtr->IPCNam2 |= _TypeName(TypeInt32);
	MyMsgPtr->IPCNam2 |= _TypeSizeInBits(32);
	MyMsgPtr->IPCNam2 |= _NumObjects(1);
#endif FastAssign
	MyMsgPtr->Arg2 = (reason);
 } 
RepMsg.Head.MsgSize = sizeof (RepMsg);
RepMsg.Head.LocalPort = mach_reply_port;
if ( (GR = Send (& MyMsg.Head, 0, Wait)) != Success)
MsgError(GR);

if((GR = Receive( & (RepMsg.Head),0,LocalPt,ReceiveIt))!= Success)
MsgError(GR);
 { 
 if (RepMsg.Head.ID !=     1108)
MsgError(BadReply);
#if TypeCheck
 if (_X_TypeName(RepMsg.RetCodeType) != TypeInt16)
MsgError(BadReply);
#endif
	 if (RepMsg.RetCode != Success)
MsgError(BadReply);

#if TypeCheck
	 if (_X_TypeName(RepMsg.IPCNam3) != TypeInt32)
MsgError(BadReply);
#endif TypeCheck
	task_terminate = (RepMsg.Arg3);
 return (task_terminate);
 } 

 } 

kern_return_t	task_suspend ( ServPort)
task_t	ServPort;
 { 

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
MyMessage  MyMsg;
RepMessage RepMsg;
kern_return_t	task_suspend;

MyMsg.Head.SimpleMsg =	1;
MyMsg.Head.MsgSize = sizeof (MyMsg);
MyMsg.Head.MsgType = NormalMsg;
MyMsg.Head.RemotePort = ServPort;
MyMsg.Head.LocalPort = mach_reply_port;
MyMsg.Head.ID =     1009;
 { 
MyMessage *MyMsgPtr;
MyMsgPtr = & MyMsg;
 } 
RepMsg.Head.MsgSize = sizeof (RepMsg);
RepMsg.Head.LocalPort = mach_reply_port;
if ( (GR = Send (& MyMsg.Head, 0, Wait)) != Success)
MsgError(GR);

if((GR = Receive( & (RepMsg.Head),0,LocalPt,ReceiveIt))!= Success)
MsgError(GR);
 { 
 if (RepMsg.Head.ID !=     1109)
MsgError(BadReply);
#if TypeCheck
 if (_X_TypeName(RepMsg.RetCodeType) != TypeInt16)
MsgError(BadReply);
#endif
	 if (RepMsg.RetCode != Success)
MsgError(BadReply);

#if TypeCheck
	 if (_X_TypeName(RepMsg.IPCNam2) != TypeInt32)
MsgError(BadReply);
#endif TypeCheck
	task_suspend = (RepMsg.Arg2);
 return (task_suspend);
 } 

 } 

kern_return_t	task_resume ( ServPort)
task_t	ServPort;
 { 

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
MyMessage  MyMsg;
RepMessage RepMsg;
kern_return_t	task_resume;

MyMsg.Head.SimpleMsg =	1;
MyMsg.Head.MsgSize = sizeof (MyMsg);
MyMsg.Head.MsgType = NormalMsg;
MyMsg.Head.RemotePort = ServPort;
MyMsg.Head.LocalPort = mach_reply_port;
MyMsg.Head.ID =     1010;
 { 
MyMessage *MyMsgPtr;
MyMsgPtr = & MyMsg;
 } 
RepMsg.Head.MsgSize = sizeof (RepMsg);
RepMsg.Head.LocalPort = mach_reply_port;
if ( (GR = Send (& MyMsg.Head, 0, Wait)) != Success)
MsgError(GR);

if((GR = Receive( & (RepMsg.Head),0,LocalPt,ReceiveIt))!= Success)
MsgError(GR);
 { 
 if (RepMsg.Head.ID !=     1110)
MsgError(BadReply);
#if TypeCheck
 if (_X_TypeName(RepMsg.RetCodeType) != TypeInt16)
MsgError(BadReply);
#endif
	 if (RepMsg.RetCode != Success)
MsgError(BadReply);

#if TypeCheck
	 if (_X_TypeName(RepMsg.IPCNam2) != TypeInt32)
MsgError(BadReply);
#endif TypeCheck
	task_resume = (RepMsg.Arg2);
 return (task_resume);
 } 

 } 

kern_return_t	task_threads ( ServPort,thread_list,thread_list_Cnt)
task_t	ServPort;
thread_array_t	 * thread_list;
 Long  * thread_list_Cnt;
 { 

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
MyMessage  MyMsg;
RepMessage RepMsg;
kern_return_t	task_threads;

MyMsg.Head.SimpleMsg =	1;
MyMsg.Head.MsgSize = sizeof (MyMsg);
MyMsg.Head.MsgType = NormalMsg;
MyMsg.Head.RemotePort = ServPort;
MyMsg.Head.LocalPort = mach_reply_port;
MyMsg.Head.ID =     1011;
 { 
MyMessage *MyMsgPtr;
MyMsgPtr = & MyMsg;
 } 
RepMsg.Head.MsgSize = sizeof (RepMsg);
RepMsg.Head.LocalPort = mach_reply_port;
if ( (GR = Send (& MyMsg.Head, 0, Wait)) != Success)
MsgError(GR);

if((GR = Receive( & (RepMsg.Head),0,LocalPt,ReceiveIt))!= Success)
MsgError(GR);
 { 
 if (RepMsg.Head.ID !=     1111)
MsgError(BadReply);
#if TypeCheck
 if (_X_TypeName(RepMsg.RetCodeType) != TypeInt16)
MsgError(BadReply);
#endif
	 if (RepMsg.RetCode != Success)
MsgError(BadReply);

#if TypeCheck
	 if (RepMsg.TName2 != TypePt)
MsgError(BadReply);
#endif TypeCheck
	( * thread_list_Cnt) = RepMsg.NumElts2;
	 * thread_list = ((thread_array_t)RepMsg.Arg2);

#if TypeCheck
	 if (_X_TypeName(RepMsg.IPCNam3) != TypeInt32)
MsgError(BadReply);
#endif TypeCheck
	task_threads = (RepMsg.Arg3);
 return (task_threads);
 } 

 } 

kern_return_t	task_ports ( ServPort,port_list,port_list_Cnt)
task_t	ServPort;
port_array_t	 * port_list;
 Long  * port_list_Cnt;
 { 

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
MyMessage  MyMsg;
RepMessage RepMsg;
kern_return_t	task_ports;

MyMsg.Head.SimpleMsg =	1;
MyMsg.Head.MsgSize = sizeof (MyMsg);
MyMsg.Head.MsgType = NormalMsg;
MyMsg.Head.RemotePort = ServPort;
MyMsg.Head.LocalPort = mach_reply_port;
MyMsg.Head.ID =     1012;
 { 
MyMessage *MyMsgPtr;
MyMsgPtr = & MyMsg;
 } 
RepMsg.Head.MsgSize = sizeof (RepMsg);
RepMsg.Head.LocalPort = mach_reply_port;
if ( (GR = Send (& MyMsg.Head, 0, Wait)) != Success)
MsgError(GR);

if((GR = Receive( & (RepMsg.Head),0,LocalPt,ReceiveIt))!= Success)
MsgError(GR);
 { 
 if (RepMsg.Head.ID !=     1112)
MsgError(BadReply);
#if TypeCheck
 if (_X_TypeName(RepMsg.RetCodeType) != TypeInt16)
MsgError(BadReply);
#endif
	 if (RepMsg.RetCode != Success)
MsgError(BadReply);

#if TypeCheck
	 if (RepMsg.TName2 != TypePt)
MsgError(BadReply);
#endif TypeCheck
	( * port_list_Cnt) = RepMsg.NumElts2;
	 * port_list = ((port_array_t)RepMsg.Arg2);

#if TypeCheck
	 if (_X_TypeName(RepMsg.IPCNam3) != TypeInt32)
MsgError(BadReply);
#endif TypeCheck
	task_ports = (RepMsg.Arg3);
 return (task_ports);
 } 

 } 

kern_return_t	task_status ( ServPort,notify_port,resumed)
task_t	ServPort;
port_t	 * notify_port;
boolean_t	 * resumed;
 { 

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
MyMessage  MyMsg;
RepMessage RepMsg;
kern_return_t	task_status;

MyMsg.Head.SimpleMsg =	1;
MyMsg.Head.MsgSize = sizeof (MyMsg);
MyMsg.Head.MsgType = NormalMsg;
MyMsg.Head.RemotePort = ServPort;
MyMsg.Head.LocalPort = mach_reply_port;
MyMsg.Head.ID =     1013;
 { 
MyMessage *MyMsgPtr;
MyMsgPtr = & MyMsg;
 } 
RepMsg.Head.MsgSize = sizeof (RepMsg);
RepMsg.Head.LocalPort = mach_reply_port;
if ( (GR = Send (& MyMsg.Head, 0, Wait)) != Success)
MsgError(GR);

if((GR = Receive( & (RepMsg.Head),0,LocalPt,ReceiveIt))!= Success)
MsgError(GR);
 { 
 if (RepMsg.Head.ID !=     1113)
MsgError(BadReply);
#if TypeCheck
 if (_X_TypeName(RepMsg.RetCodeType) != TypeInt16)
MsgError(BadReply);
#endif
	 if (RepMsg.RetCode != Success)
MsgError(BadReply);

#if TypeCheck
	 if (_X_TypeName(RepMsg.IPCNam2) != TypePt)
MsgError(BadReply);
#endif TypeCheck
	 * notify_port = (RepMsg.Arg2);

#if TypeCheck
	 if (_X_TypeName(RepMsg.IPCNam3) != TypeBoolean)
MsgError(BadReply);
#endif TypeCheck
	 * resumed = (RepMsg.Arg3);

#if TypeCheck
	 if (_X_TypeName(RepMsg.IPCNam4) != TypeInt32)
MsgError(BadReply);
#endif TypeCheck
	task_status = (RepMsg.Arg4);
 return (task_status);
 } 

 } 

kern_return_t	task_set_notify ( ServPort,notify_port)
task_t	ServPort;
port_t	notify_port;
 { 

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
MyMessage  MyMsg;
RepMessage RepMsg;
kern_return_t	task_set_notify;

MyMsg.Head.SimpleMsg =	0;
MyMsg.Head.MsgSize = sizeof (MyMsg);
MyMsg.Head.MsgType = NormalMsg;
MyMsg.Head.RemotePort = ServPort;
MyMsg.Head.LocalPort = mach_reply_port;
MyMsg.Head.ID =     1014;
 { 
MyMessage *MyMsgPtr;
MyMsgPtr = & MyMsg;
#if FastAssign
	MyMsgPtr->IPCNam2 = 02000220006L;
#else FastAssign
	MyMsgPtr->IPCNam2 =        0;
	MyMsgPtr->IPCNam2 |= _InLine(  1);
	MyMsgPtr->IPCNam2 |= _Deallocate( 0);
	MyMsgPtr->IPCNam2 |= _LongForm( 0);
	MyMsgPtr->IPCNam2 |= _TypeName(TypePt);
	MyMsgPtr->IPCNam2 |= _TypeSizeInBits(32);
	MyMsgPtr->IPCNam2 |= _NumObjects(1);
#endif FastAssign
	MyMsgPtr->Arg2 = (notify_port);
 } 
RepMsg.Head.MsgSize = sizeof (RepMsg);
RepMsg.Head.LocalPort = mach_reply_port;
if ( (GR = Send (& MyMsg.Head, 0, Wait)) != Success)
MsgError(GR);

if((GR = Receive( & (RepMsg.Head),0,LocalPt,ReceiveIt))!= Success)
MsgError(GR);
 { 
 if (RepMsg.Head.ID !=     1114)
MsgError(BadReply);
#if TypeCheck
 if (_X_TypeName(RepMsg.RetCodeType) != TypeInt16)
MsgError(BadReply);
#endif
	 if (RepMsg.RetCode != Success)
MsgError(BadReply);

#if TypeCheck
	 if (_X_TypeName(RepMsg.IPCNam3) != TypeInt32)
MsgError(BadReply);
#endif TypeCheck
	task_set_notify = (RepMsg.Arg3);
 return (task_set_notify);
 } 

 } 

kern_return_t	thread_create ( ServPort,child_thread,child_data)
task_t	ServPort;
thread_t	 * child_thread;
port_t	 * child_data;
 { 

 typedef  struct { 
Msg	Head;
 } MyMessage;

 typedef  struct { 
Msg	Head;
TypeType	RetCodeType;
 Integer 	RetCode;
TypeType IPCNam2;
thread_t	Arg2;
TypeType IPCNam3;
port_t	Arg3;
TypeType IPCNam4;
kern_return_t	Arg4;
 } RepMessage;
MyMessage  MyMsg;
RepMessage RepMsg;
kern_return_t	thread_create;

MyMsg.Head.SimpleMsg =	1;
MyMsg.Head.MsgSize = sizeof (MyMsg);
MyMsg.Head.MsgType = NormalMsg;
MyMsg.Head.RemotePort = ServPort;
MyMsg.Head.LocalPort = mach_reply_port;
MyMsg.Head.ID =     1015;
 { 
MyMessage *MyMsgPtr;
MyMsgPtr = & MyMsg;
 } 
RepMsg.Head.MsgSize = sizeof (RepMsg);
RepMsg.Head.LocalPort = mach_reply_port;
if ( (GR = Send (& MyMsg.Head, 0, Wait)) != Success)
MsgError(GR);

if((GR = Receive( & (RepMsg.Head),0,LocalPt,ReceiveIt))!= Success)
MsgError(GR);
 { 
 if (RepMsg.Head.ID !=     1115)
MsgError(BadReply);
#if TypeCheck
 if (_X_TypeName(RepMsg.RetCodeType) != TypeInt16)
MsgError(BadReply);
#endif
	 if (RepMsg.RetCode != Success)
MsgError(BadReply);

#if TypeCheck
	 if (_X_TypeName(RepMsg.IPCNam2) != TypePt)
MsgError(BadReply);
#endif TypeCheck
	 * child_thread = (RepMsg.Arg2);

#if TypeCheck
	 if (_X_TypeName(RepMsg.IPCNam3) != TypePt)
MsgError(BadReply);
#endif TypeCheck
	 * child_data = (RepMsg.Arg3);

#if TypeCheck
	 if (_X_TypeName(RepMsg.IPCNam4) != TypeInt32)
MsgError(BadReply);
#endif TypeCheck
	thread_create = (RepMsg.Arg4);
 return (thread_create);
 } 

 } 

kern_return_t	thread_terminate ( ServPort)
thread_t	ServPort;
 { 

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
MyMessage  MyMsg;
RepMessage RepMsg;
kern_return_t	thread_terminate;

MyMsg.Head.SimpleMsg =	1;
MyMsg.Head.MsgSize = sizeof (MyMsg);
MyMsg.Head.MsgType = NormalMsg;
MyMsg.Head.RemotePort = ServPort;
MyMsg.Head.LocalPort = mach_reply_port;
MyMsg.Head.ID =     1016;
 { 
MyMessage *MyMsgPtr;
MyMsgPtr = & MyMsg;
 } 
RepMsg.Head.MsgSize = sizeof (RepMsg);
RepMsg.Head.LocalPort = mach_reply_port;
if ( (GR = Send (& MyMsg.Head, 0, Wait)) != Success)
MsgError(GR);

if((GR = Receive( & (RepMsg.Head),0,LocalPt,ReceiveIt))!= Success)
MsgError(GR);
 { 
 if (RepMsg.Head.ID !=     1116)
MsgError(BadReply);
#if TypeCheck
 if (_X_TypeName(RepMsg.RetCodeType) != TypeInt16)
MsgError(BadReply);
#endif
	 if (RepMsg.RetCode != Success)
MsgError(BadReply);

#if TypeCheck
	 if (_X_TypeName(RepMsg.IPCNam2) != TypeInt32)
MsgError(BadReply);
#endif TypeCheck
	thread_terminate = (RepMsg.Arg2);
 return (thread_terminate);
 } 

 } 

kern_return_t	thread_suspend ( ServPort)
thread_t	ServPort;
 { 

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
MyMessage  MyMsg;
RepMessage RepMsg;
kern_return_t	thread_suspend;

MyMsg.Head.SimpleMsg =	1;
MyMsg.Head.MsgSize = sizeof (MyMsg);
MyMsg.Head.MsgType = NormalMsg;
MyMsg.Head.RemotePort = ServPort;
MyMsg.Head.LocalPort = mach_reply_port;
MyMsg.Head.ID =     1017;
 { 
MyMessage *MyMsgPtr;
MyMsgPtr = & MyMsg;
 } 
RepMsg.Head.MsgSize = sizeof (RepMsg);
RepMsg.Head.LocalPort = mach_reply_port;
if ( (GR = Send (& MyMsg.Head, 0, Wait)) != Success)
MsgError(GR);

if((GR = Receive( & (RepMsg.Head),0,LocalPt,ReceiveIt))!= Success)
MsgError(GR);
 { 
 if (RepMsg.Head.ID !=     1117)
MsgError(BadReply);
#if TypeCheck
 if (_X_TypeName(RepMsg.RetCodeType) != TypeInt16)
MsgError(BadReply);
#endif
	 if (RepMsg.RetCode != Success)
MsgError(BadReply);

#if TypeCheck
	 if (_X_TypeName(RepMsg.IPCNam2) != TypeInt32)
MsgError(BadReply);
#endif TypeCheck
	thread_suspend = (RepMsg.Arg2);
 return (thread_suspend);
 } 

 } 

kern_return_t	thread_resume ( ServPort)
thread_t	ServPort;
 { 

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
MyMessage  MyMsg;
RepMessage RepMsg;
kern_return_t	thread_resume;

MyMsg.Head.SimpleMsg =	1;
MyMsg.Head.MsgSize = sizeof (MyMsg);
MyMsg.Head.MsgType = NormalMsg;
MyMsg.Head.RemotePort = ServPort;
MyMsg.Head.LocalPort = mach_reply_port;
MyMsg.Head.ID =     1018;
 { 
MyMessage *MyMsgPtr;
MyMsgPtr = & MyMsg;
 } 
RepMsg.Head.MsgSize = sizeof (RepMsg);
RepMsg.Head.LocalPort = mach_reply_port;
if ( (GR = Send (& MyMsg.Head, 0, Wait)) != Success)
MsgError(GR);

if((GR = Receive( & (RepMsg.Head),0,LocalPt,ReceiveIt))!= Success)
MsgError(GR);
 { 
 if (RepMsg.Head.ID !=     1118)
MsgError(BadReply);
#if TypeCheck
 if (_X_TypeName(RepMsg.RetCodeType) != TypeInt16)
MsgError(BadReply);
#endif
	 if (RepMsg.RetCode != Success)
MsgError(BadReply);

#if TypeCheck
	 if (_X_TypeName(RepMsg.IPCNam2) != TypeInt32)
MsgError(BadReply);
#endif TypeCheck
	thread_resume = (RepMsg.Arg2);
 return (thread_resume);
 } 

 } 

kern_return_t	thread_status ( ServPort,resumed,old_state)
thread_t	ServPort;
boolean_t	 * resumed;
thread_status_t	 * old_state;
 { 

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
MyMessage  MyMsg;
RepMessage RepMsg;
kern_return_t	thread_status;

MyMsg.Head.SimpleMsg =	1;
MyMsg.Head.MsgSize = sizeof (MyMsg);
MyMsg.Head.MsgType = NormalMsg;
MyMsg.Head.RemotePort = ServPort;
MyMsg.Head.LocalPort = mach_reply_port;
MyMsg.Head.ID =     1019;
 { 
MyMessage *MyMsgPtr;
MyMsgPtr = & MyMsg;
 } 
RepMsg.Head.MsgSize = sizeof (RepMsg);
RepMsg.Head.LocalPort = mach_reply_port;
if ( (GR = Send (& MyMsg.Head, 0, Wait)) != Success)
MsgError(GR);

if((GR = Receive( & (RepMsg.Head),0,LocalPt,ReceiveIt))!= Success)
MsgError(GR);
 { 
 if (RepMsg.Head.ID !=     1119)
MsgError(BadReply);
#if TypeCheck
 if (_X_TypeName(RepMsg.RetCodeType) != TypeInt16)
MsgError(BadReply);
#endif
	 if (RepMsg.RetCode != Success)
MsgError(BadReply);

#if TypeCheck
	 if (_X_TypeName(RepMsg.IPCNam2) != TypeBoolean)
MsgError(BadReply);
#endif TypeCheck
	 * resumed = (RepMsg.Arg2);

#if TypeCheck
	 if (_X_TypeName(RepMsg.IPCNam3) != TypeInt32)
MsgError(BadReply);
#endif TypeCheck
	 * old_state = (RepMsg.Arg3);

#if TypeCheck
	 if (_X_TypeName(RepMsg.IPCNam4) != TypeInt32)
MsgError(BadReply);
#endif TypeCheck
	thread_status = (RepMsg.Arg4);
 return (thread_status);
 } 

 } 

kern_return_t	thread_mutate ( ServPort,new_state)
thread_t	ServPort;
thread_status_t	new_state;
 { 

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
MyMessage  MyMsg;
RepMessage RepMsg;
kern_return_t	thread_mutate;

MyMsg.Head.SimpleMsg =	1;
MyMsg.Head.MsgSize = sizeof (MyMsg);
MyMsg.Head.MsgType = NormalMsg;
MyMsg.Head.RemotePort = ServPort;
MyMsg.Head.LocalPort = mach_reply_port;
MyMsg.Head.ID =     1020;
 { 
MyMessage *MyMsgPtr;
MyMsgPtr = & MyMsg;
#if FastAssign
	MyMsgPtr->IPCNam2 = 02000220002L;
#else FastAssign
	MyMsgPtr->IPCNam2 =        0;
	MyMsgPtr->IPCNam2 |= _InLine(  1);
	MyMsgPtr->IPCNam2 |= _Deallocate( 0);
	MyMsgPtr->IPCNam2 |= _LongForm( 0);
	MyMsgPtr->IPCNam2 |= _TypeName(TypeInt32);
	MyMsgPtr->IPCNam2 |= _TypeSizeInBits(32);
	MyMsgPtr->IPCNam2 |= _NumObjects(1);
#endif FastAssign
	MyMsgPtr->Arg2 = (new_state);
 } 
RepMsg.Head.MsgSize = sizeof (RepMsg);
RepMsg.Head.LocalPort = mach_reply_port;
if ( (GR = Send (& MyMsg.Head, 0, Wait)) != Success)
MsgError(GR);

if((GR = Receive( & (RepMsg.Head),0,LocalPt,ReceiveIt))!= Success)
MsgError(GR);
 { 
 if (RepMsg.Head.ID !=     1120)
MsgError(BadReply);
#if TypeCheck
 if (_X_TypeName(RepMsg.RetCodeType) != TypeInt16)
MsgError(BadReply);
#endif
	 if (RepMsg.RetCode != Success)
MsgError(BadReply);

#if TypeCheck
	 if (_X_TypeName(RepMsg.IPCNam3) != TypeInt32)
MsgError(BadReply);
#endif TypeCheck
	thread_mutate = (RepMsg.Arg3);
 return (thread_mutate);
 } 

 } 

kern_return_t	vm_allocate ( ServPort,address,size,anywhere)
vm_task_t	ServPort;
vm_address_t	 * address;
vm_size_t	size;
boolean_t	anywhere;
 { 

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
MyMessage  MyMsg;
RepMessage RepMsg;
kern_return_t	vm_allocate;

MyMsg.Head.SimpleMsg =	1;
MyMsg.Head.MsgSize = sizeof (MyMsg);
MyMsg.Head.MsgType = NormalMsg;
MyMsg.Head.RemotePort = ServPort;
MyMsg.Head.LocalPort = mach_reply_port;
MyMsg.Head.ID =     1021;
 { 
MyMessage *MyMsgPtr;
MyMsgPtr = & MyMsg;
#if FastAssign
	MyMsgPtr->IPCNam2 = 02000220002L;
#else FastAssign
	MyMsgPtr->IPCNam2 =        0;
	MyMsgPtr->IPCNam2 |= _InLine(  1);
	MyMsgPtr->IPCNam2 |= _Deallocate( 0);
	MyMsgPtr->IPCNam2 |= _LongForm( 0);
	MyMsgPtr->IPCNam2 |= _TypeName(TypeInt32);
	MyMsgPtr->IPCNam2 |= _TypeSizeInBits(32);
	MyMsgPtr->IPCNam2 |= _NumObjects(1);
#endif FastAssign
	MyMsgPtr->Arg2 =  * (address);
#if FastAssign
	MyMsgPtr->IPCNam3 = 02000220002L;
#else FastAssign
	MyMsgPtr->IPCNam3 =        0;
	MyMsgPtr->IPCNam3 |= _InLine(  1);
	MyMsgPtr->IPCNam3 |= _Deallocate( 0);
	MyMsgPtr->IPCNam3 |= _LongForm( 0);
	MyMsgPtr->IPCNam3 |= _TypeName(TypeInt32);
	MyMsgPtr->IPCNam3 |= _TypeSizeInBits(32);
	MyMsgPtr->IPCNam3 |= _NumObjects(1);
#endif FastAssign
	MyMsgPtr->Arg3 = (size);
#if FastAssign
	MyMsgPtr->IPCNam4 = 02000210000L;
#else FastAssign
	MyMsgPtr->IPCNam4 =        0;
	MyMsgPtr->IPCNam4 |= _InLine(  1);
	MyMsgPtr->IPCNam4 |= _Deallocate( 0);
	MyMsgPtr->IPCNam4 |= _LongForm( 0);
	MyMsgPtr->IPCNam4 |= _TypeName(TypeBoolean);
	MyMsgPtr->IPCNam4 |= _TypeSizeInBits(16);
	MyMsgPtr->IPCNam4 |= _NumObjects(1);
#endif FastAssign
	MyMsgPtr->Arg4 = (anywhere);
 } 
RepMsg.Head.MsgSize = sizeof (RepMsg);
RepMsg.Head.LocalPort = mach_reply_port;
if ( (GR = Send (& MyMsg.Head, 0, Wait)) != Success)
MsgError(GR);

if((GR = Receive( & (RepMsg.Head),0,LocalPt,ReceiveIt))!= Success)
MsgError(GR);
 { 
 if (RepMsg.Head.ID !=     1121)
MsgError(BadReply);
#if TypeCheck
 if (_X_TypeName(RepMsg.RetCodeType) != TypeInt16)
MsgError(BadReply);
#endif
	 if (RepMsg.RetCode != Success)
MsgError(BadReply);

#if TypeCheck
	 if (_X_TypeName(RepMsg.IPCNam2) != TypeInt32)
MsgError(BadReply);
#endif TypeCheck
	 * address = (RepMsg.Arg2);

#if TypeCheck
	 if (_X_TypeName(RepMsg.IPCNam5) != TypeInt32)
MsgError(BadReply);
#endif TypeCheck
	vm_allocate = (RepMsg.Arg5);
 return (vm_allocate);
 } 

 } 

kern_return_t	vm_allocate_with_pager ( ServPort,address,size,anywhere,pager_port,pager_space,pager_offset)
vm_task_t	ServPort;
vm_address_t	 * address;
vm_size_t	size;
boolean_t	anywhere;
port_t	pager_port;
vm_pager_id_t	pager_space;
vm_offset_t	pager_offset;
 { 

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
MyMessage  MyMsg;
RepMessage RepMsg;
kern_return_t	vm_allocate_with_pager;

MyMsg.Head.SimpleMsg =	0;
MyMsg.Head.MsgSize = sizeof (MyMsg);
MyMsg.Head.MsgType = NormalMsg;
MyMsg.Head.RemotePort = ServPort;
MyMsg.Head.LocalPort = mach_reply_port;
MyMsg.Head.ID =     1022;
 { 
MyMessage *MyMsgPtr;
MyMsgPtr = & MyMsg;
#if FastAssign
	MyMsgPtr->IPCNam2 = 02000220002L;
#else FastAssign
	MyMsgPtr->IPCNam2 =        0;
	MyMsgPtr->IPCNam2 |= _InLine(  1);
	MyMsgPtr->IPCNam2 |= _Deallocate( 0);
	MyMsgPtr->IPCNam2 |= _LongForm( 0);
	MyMsgPtr->IPCNam2 |= _TypeName(TypeInt32);
	MyMsgPtr->IPCNam2 |= _TypeSizeInBits(32);
	MyMsgPtr->IPCNam2 |= _NumObjects(1);
#endif FastAssign
	MyMsgPtr->Arg2 =  * (address);
#if FastAssign
	MyMsgPtr->IPCNam3 = 02000220002L;
#else FastAssign
	MyMsgPtr->IPCNam3 =        0;
	MyMsgPtr->IPCNam3 |= _InLine(  1);
	MyMsgPtr->IPCNam3 |= _Deallocate( 0);
	MyMsgPtr->IPCNam3 |= _LongForm( 0);
	MyMsgPtr->IPCNam3 |= _TypeName(TypeInt32);
	MyMsgPtr->IPCNam3 |= _TypeSizeInBits(32);
	MyMsgPtr->IPCNam3 |= _NumObjects(1);
#endif FastAssign
	MyMsgPtr->Arg3 = (size);
#if FastAssign
	MyMsgPtr->IPCNam4 = 02000210000L;
#else FastAssign
	MyMsgPtr->IPCNam4 =        0;
	MyMsgPtr->IPCNam4 |= _InLine(  1);
	MyMsgPtr->IPCNam4 |= _Deallocate( 0);
	MyMsgPtr->IPCNam4 |= _LongForm( 0);
	MyMsgPtr->IPCNam4 |= _TypeName(TypeBoolean);
	MyMsgPtr->IPCNam4 |= _TypeSizeInBits(16);
	MyMsgPtr->IPCNam4 |= _NumObjects(1);
#endif FastAssign
	MyMsgPtr->Arg4 = (anywhere);
#if FastAssign
	MyMsgPtr->IPCNam5 = 02000220006L;
#else FastAssign
	MyMsgPtr->IPCNam5 =        0;
	MyMsgPtr->IPCNam5 |= _InLine(  1);
	MyMsgPtr->IPCNam5 |= _Deallocate( 0);
	MyMsgPtr->IPCNam5 |= _LongForm( 0);
	MyMsgPtr->IPCNam5 |= _TypeName(TypePt);
	MyMsgPtr->IPCNam5 |= _TypeSizeInBits(32);
	MyMsgPtr->IPCNam5 |= _NumObjects(1);
#endif FastAssign
	MyMsgPtr->Arg5 = (pager_port);
#if FastAssign
	MyMsgPtr->IPCNam6 = 02000220002L;
#else FastAssign
	MyMsgPtr->IPCNam6 =        0;
	MyMsgPtr->IPCNam6 |= _InLine(  1);
	MyMsgPtr->IPCNam6 |= _Deallocate( 0);
	MyMsgPtr->IPCNam6 |= _LongForm( 0);
	MyMsgPtr->IPCNam6 |= _TypeName(TypeInt32);
	MyMsgPtr->IPCNam6 |= _TypeSizeInBits(32);
	MyMsgPtr->IPCNam6 |= _NumObjects(1);
#endif FastAssign
	MyMsgPtr->Arg6 = (pager_space);
#if FastAssign
	MyMsgPtr->IPCNam7 = 02000220002L;
#else FastAssign
	MyMsgPtr->IPCNam7 =        0;
	MyMsgPtr->IPCNam7 |= _InLine(  1);
	MyMsgPtr->IPCNam7 |= _Deallocate( 0);
	MyMsgPtr->IPCNam7 |= _LongForm( 0);
	MyMsgPtr->IPCNam7 |= _TypeName(TypeInt32);
	MyMsgPtr->IPCNam7 |= _TypeSizeInBits(32);
	MyMsgPtr->IPCNam7 |= _NumObjects(1);
#endif FastAssign
	MyMsgPtr->Arg7 = (pager_offset);
 } 
RepMsg.Head.MsgSize = sizeof (RepMsg);
RepMsg.Head.LocalPort = mach_reply_port;
if ( (GR = Send (& MyMsg.Head, 0, Wait)) != Success)
MsgError(GR);

if((GR = Receive( & (RepMsg.Head),0,LocalPt,ReceiveIt))!= Success)
MsgError(GR);
 { 
 if (RepMsg.Head.ID !=     1122)
MsgError(BadReply);
#if TypeCheck
 if (_X_TypeName(RepMsg.RetCodeType) != TypeInt16)
MsgError(BadReply);
#endif
	 if (RepMsg.RetCode != Success)
MsgError(BadReply);

#if TypeCheck
	 if (_X_TypeName(RepMsg.IPCNam2) != TypeInt32)
MsgError(BadReply);
#endif TypeCheck
	 * address = (RepMsg.Arg2);

#if TypeCheck
	 if (_X_TypeName(RepMsg.IPCNam8) != TypeInt32)
MsgError(BadReply);
#endif TypeCheck
	vm_allocate_with_pager = (RepMsg.Arg8);
 return (vm_allocate_with_pager);
 } 

 } 

kern_return_t	vm_deallocate ( ServPort,address,size)
vm_task_t	ServPort;
vm_address_t	address;
vm_size_t	size;
 { 

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
MyMessage  MyMsg;
RepMessage RepMsg;
kern_return_t	vm_deallocate;

MyMsg.Head.SimpleMsg =	1;
MyMsg.Head.MsgSize = sizeof (MyMsg);
MyMsg.Head.MsgType = NormalMsg;
MyMsg.Head.RemotePort = ServPort;
MyMsg.Head.LocalPort = mach_reply_port;
MyMsg.Head.ID =     1023;
 { 
MyMessage *MyMsgPtr;
MyMsgPtr = & MyMsg;
#if FastAssign
	MyMsgPtr->IPCNam2 = 02000220002L;
#else FastAssign
	MyMsgPtr->IPCNam2 =        0;
	MyMsgPtr->IPCNam2 |= _InLine(  1);
	MyMsgPtr->IPCNam2 |= _Deallocate( 0);
	MyMsgPtr->IPCNam2 |= _LongForm( 0);
	MyMsgPtr->IPCNam2 |= _TypeName(TypeInt32);
	MyMsgPtr->IPCNam2 |= _TypeSizeInBits(32);
	MyMsgPtr->IPCNam2 |= _NumObjects(1);
#endif FastAssign
	MyMsgPtr->Arg2 = (address);
#if FastAssign
	MyMsgPtr->IPCNam3 = 02000220002L;
#else FastAssign
	MyMsgPtr->IPCNam3 =        0;
	MyMsgPtr->IPCNam3 |= _InLine(  1);
	MyMsgPtr->IPCNam3 |= _Deallocate( 0);
	MyMsgPtr->IPCNam3 |= _LongForm( 0);
	MyMsgPtr->IPCNam3 |= _TypeName(TypeInt32);
	MyMsgPtr->IPCNam3 |= _TypeSizeInBits(32);
	MyMsgPtr->IPCNam3 |= _NumObjects(1);
#endif FastAssign
	MyMsgPtr->Arg3 = (size);
 } 
RepMsg.Head.MsgSize = sizeof (RepMsg);
RepMsg.Head.LocalPort = mach_reply_port;
if ( (GR = Send (& MyMsg.Head, 0, Wait)) != Success)
MsgError(GR);

if((GR = Receive( & (RepMsg.Head),0,LocalPt,ReceiveIt))!= Success)
MsgError(GR);
 { 
 if (RepMsg.Head.ID !=     1123)
MsgError(BadReply);
#if TypeCheck
 if (_X_TypeName(RepMsg.RetCodeType) != TypeInt16)
MsgError(BadReply);
#endif
	 if (RepMsg.RetCode != Success)
MsgError(BadReply);

#if TypeCheck
	 if (_X_TypeName(RepMsg.IPCNam4) != TypeInt32)
MsgError(BadReply);
#endif TypeCheck
	vm_deallocate = (RepMsg.Arg4);
 return (vm_deallocate);
 } 

 } 

kern_return_t	vm_protect ( ServPort,address,size,set_maximum,new_protection)
vm_task_t	ServPort;
vm_address_t	address;
vm_size_t	size;
boolean_t	set_maximum;
vm_prot_t	new_protection;
 { 

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
MyMessage  MyMsg;
RepMessage RepMsg;
kern_return_t	vm_protect;

MyMsg.Head.SimpleMsg =	1;
MyMsg.Head.MsgSize = sizeof (MyMsg);
MyMsg.Head.MsgType = NormalMsg;
MyMsg.Head.RemotePort = ServPort;
MyMsg.Head.LocalPort = mach_reply_port;
MyMsg.Head.ID =     1024;
 { 
MyMessage *MyMsgPtr;
MyMsgPtr = & MyMsg;
#if FastAssign
	MyMsgPtr->IPCNam2 = 02000220002L;
#else FastAssign
	MyMsgPtr->IPCNam2 =        0;
	MyMsgPtr->IPCNam2 |= _InLine(  1);
	MyMsgPtr->IPCNam2 |= _Deallocate( 0);
	MyMsgPtr->IPCNam2 |= _LongForm( 0);
	MyMsgPtr->IPCNam2 |= _TypeName(TypeInt32);
	MyMsgPtr->IPCNam2 |= _TypeSizeInBits(32);
	MyMsgPtr->IPCNam2 |= _NumObjects(1);
#endif FastAssign
	MyMsgPtr->Arg2 = (address);
#if FastAssign
	MyMsgPtr->IPCNam3 = 02000220002L;
#else FastAssign
	MyMsgPtr->IPCNam3 =        0;
	MyMsgPtr->IPCNam3 |= _InLine(  1);
	MyMsgPtr->IPCNam3 |= _Deallocate( 0);
	MyMsgPtr->IPCNam3 |= _LongForm( 0);
	MyMsgPtr->IPCNam3 |= _TypeName(TypeInt32);
	MyMsgPtr->IPCNam3 |= _TypeSizeInBits(32);
	MyMsgPtr->IPCNam3 |= _NumObjects(1);
#endif FastAssign
	MyMsgPtr->Arg3 = (size);
#if FastAssign
	MyMsgPtr->IPCNam4 = 02000210000L;
#else FastAssign
	MyMsgPtr->IPCNam4 =        0;
	MyMsgPtr->IPCNam4 |= _InLine(  1);
	MyMsgPtr->IPCNam4 |= _Deallocate( 0);
	MyMsgPtr->IPCNam4 |= _LongForm( 0);
	MyMsgPtr->IPCNam4 |= _TypeName(TypeBoolean);
	MyMsgPtr->IPCNam4 |= _TypeSizeInBits(16);
	MyMsgPtr->IPCNam4 |= _NumObjects(1);
#endif FastAssign
	MyMsgPtr->Arg4 = (set_maximum);
#if FastAssign
	MyMsgPtr->IPCNam5 = 02000220002L;
#else FastAssign
	MyMsgPtr->IPCNam5 =        0;
	MyMsgPtr->IPCNam5 |= _InLine(  1);
	MyMsgPtr->IPCNam5 |= _Deallocate( 0);
	MyMsgPtr->IPCNam5 |= _LongForm( 0);
	MyMsgPtr->IPCNam5 |= _TypeName(TypeInt32);
	MyMsgPtr->IPCNam5 |= _TypeSizeInBits(32);
	MyMsgPtr->IPCNam5 |= _NumObjects(1);
#endif FastAssign
	MyMsgPtr->Arg5 = (new_protection);
 } 
RepMsg.Head.MsgSize = sizeof (RepMsg);
RepMsg.Head.LocalPort = mach_reply_port;
if ( (GR = Send (& MyMsg.Head, 0, Wait)) != Success)
MsgError(GR);

if((GR = Receive( & (RepMsg.Head),0,LocalPt,ReceiveIt))!= Success)
MsgError(GR);
 { 
 if (RepMsg.Head.ID !=     1124)
MsgError(BadReply);
#if TypeCheck
 if (_X_TypeName(RepMsg.RetCodeType) != TypeInt16)
MsgError(BadReply);
#endif
	 if (RepMsg.RetCode != Success)
MsgError(BadReply);

#if TypeCheck
	 if (_X_TypeName(RepMsg.IPCNam6) != TypeInt32)
MsgError(BadReply);
#endif TypeCheck
	vm_protect = (RepMsg.Arg6);
 return (vm_protect);
 } 

 } 

kern_return_t	vm_inherit ( ServPort,address,size,new_inheritance)
vm_task_t	ServPort;
vm_address_t	address;
vm_size_t	size;
vm_inherit_t	new_inheritance;
 { 

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
MyMessage  MyMsg;
RepMessage RepMsg;
kern_return_t	vm_inherit;

MyMsg.Head.SimpleMsg =	1;
MyMsg.Head.MsgSize = sizeof (MyMsg);
MyMsg.Head.MsgType = NormalMsg;
MyMsg.Head.RemotePort = ServPort;
MyMsg.Head.LocalPort = mach_reply_port;
MyMsg.Head.ID =     1025;
 { 
MyMessage *MyMsgPtr;
MyMsgPtr = & MyMsg;
#if FastAssign
	MyMsgPtr->IPCNam2 = 02000220002L;
#else FastAssign
	MyMsgPtr->IPCNam2 =        0;
	MyMsgPtr->IPCNam2 |= _InLine(  1);
	MyMsgPtr->IPCNam2 |= _Deallocate( 0);
	MyMsgPtr->IPCNam2 |= _LongForm( 0);
	MyMsgPtr->IPCNam2 |= _TypeName(TypeInt32);
	MyMsgPtr->IPCNam2 |= _TypeSizeInBits(32);
	MyMsgPtr->IPCNam2 |= _NumObjects(1);
#endif FastAssign
	MyMsgPtr->Arg2 = (address);
#if FastAssign
	MyMsgPtr->IPCNam3 = 02000220002L;
#else FastAssign
	MyMsgPtr->IPCNam3 =        0;
	MyMsgPtr->IPCNam3 |= _InLine(  1);
	MyMsgPtr->IPCNam3 |= _Deallocate( 0);
	MyMsgPtr->IPCNam3 |= _LongForm( 0);
	MyMsgPtr->IPCNam3 |= _TypeName(TypeInt32);
	MyMsgPtr->IPCNam3 |= _TypeSizeInBits(32);
	MyMsgPtr->IPCNam3 |= _NumObjects(1);
#endif FastAssign
	MyMsgPtr->Arg3 = (size);
#if FastAssign
	MyMsgPtr->IPCNam4 = 02000220002L;
#else FastAssign
	MyMsgPtr->IPCNam4 =        0;
	MyMsgPtr->IPCNam4 |= _InLine(  1);
	MyMsgPtr->IPCNam4 |= _Deallocate( 0);
	MyMsgPtr->IPCNam4 |= _LongForm( 0);
	MyMsgPtr->IPCNam4 |= _TypeName(TypeInt32);
	MyMsgPtr->IPCNam4 |= _TypeSizeInBits(32);
	MyMsgPtr->IPCNam4 |= _NumObjects(1);
#endif FastAssign
	MyMsgPtr->Arg4 = (new_inheritance);
 } 
RepMsg.Head.MsgSize = sizeof (RepMsg);
RepMsg.Head.LocalPort = mach_reply_port;
if ( (GR = Send (& MyMsg.Head, 0, Wait)) != Success)
MsgError(GR);

if((GR = Receive( & (RepMsg.Head),0,LocalPt,ReceiveIt))!= Success)
MsgError(GR);
 { 
 if (RepMsg.Head.ID !=     1125)
MsgError(BadReply);
#if TypeCheck
 if (_X_TypeName(RepMsg.RetCodeType) != TypeInt16)
MsgError(BadReply);
#endif
	 if (RepMsg.RetCode != Success)
MsgError(BadReply);

#if TypeCheck
	 if (_X_TypeName(RepMsg.IPCNam5) != TypeInt32)
MsgError(BadReply);
#endif TypeCheck
	vm_inherit = (RepMsg.Arg5);
 return (vm_inherit);
 } 

 } 

kern_return_t	vm_read ( ServPort,address,size,data,data_Cnt)
vm_task_t	ServPort;
vm_address_t	address;
vm_size_t	size;
pointer_t	 * data;
 Long  * data_Cnt;
 { 

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
MyMessage  MyMsg;
RepMessage RepMsg;
kern_return_t	vm_read;

MyMsg.Head.SimpleMsg =	1;
MyMsg.Head.MsgSize = sizeof (MyMsg);
MyMsg.Head.MsgType = NormalMsg;
MyMsg.Head.RemotePort = ServPort;
MyMsg.Head.LocalPort = mach_reply_port;
MyMsg.Head.ID =     1026;
 { 
MyMessage *MyMsgPtr;
MyMsgPtr = & MyMsg;
#if FastAssign
	MyMsgPtr->IPCNam2 = 02000220002L;
#else FastAssign
	MyMsgPtr->IPCNam2 =        0;
	MyMsgPtr->IPCNam2 |= _InLine(  1);
	MyMsgPtr->IPCNam2 |= _Deallocate( 0);
	MyMsgPtr->IPCNam2 |= _LongForm( 0);
	MyMsgPtr->IPCNam2 |= _TypeName(TypeInt32);
	MyMsgPtr->IPCNam2 |= _TypeSizeInBits(32);
	MyMsgPtr->IPCNam2 |= _NumObjects(1);
#endif FastAssign
	MyMsgPtr->Arg2 = (address);
#if FastAssign
	MyMsgPtr->IPCNam3 = 02000220002L;
#else FastAssign
	MyMsgPtr->IPCNam3 =        0;
	MyMsgPtr->IPCNam3 |= _InLine(  1);
	MyMsgPtr->IPCNam3 |= _Deallocate( 0);
	MyMsgPtr->IPCNam3 |= _LongForm( 0);
	MyMsgPtr->IPCNam3 |= _TypeName(TypeInt32);
	MyMsgPtr->IPCNam3 |= _TypeSizeInBits(32);
	MyMsgPtr->IPCNam3 |= _NumObjects(1);
#endif FastAssign
	MyMsgPtr->Arg3 = (size);
 } 
RepMsg.Head.MsgSize = sizeof (RepMsg);
RepMsg.Head.LocalPort = mach_reply_port;
if ( (GR = Send (& MyMsg.Head, 0, Wait)) != Success)
MsgError(GR);

if((GR = Receive( & (RepMsg.Head),0,LocalPt,ReceiveIt))!= Success)
MsgError(GR);
 { 
 if (RepMsg.Head.ID !=     1126)
MsgError(BadReply);
#if TypeCheck
 if (_X_TypeName(RepMsg.RetCodeType) != TypeInt16)
MsgError(BadReply);
#endif
	 if (RepMsg.RetCode != Success)
MsgError(BadReply);

#if TypeCheck
	 if (RepMsg.TName4 != TypeInt8)
MsgError(BadReply);
#endif TypeCheck
	( * data_Cnt) = RepMsg.NumElts4;
	 * data = ((pointer_t)RepMsg.Arg4);

#if TypeCheck
	 if (_X_TypeName(RepMsg.IPCNam5) != TypeInt32)
MsgError(BadReply);
#endif TypeCheck
	vm_read = (RepMsg.Arg5);
 return (vm_read);
 } 

 } 

kern_return_t	vm_write ( ServPort,address,data,data_Cnt)
vm_task_t	ServPort;
vm_address_t	address;
pointer_t	data;
 Long data_Cnt;
 { 

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
MyMessage  MyMsg;
RepMessage RepMsg;
kern_return_t	vm_write;

MyMsg.Head.SimpleMsg =	0;
MyMsg.Head.MsgSize = sizeof (MyMsg);
MyMsg.Head.MsgType = NormalMsg;
MyMsg.Head.RemotePort = ServPort;
MyMsg.Head.LocalPort = mach_reply_port;
MyMsg.Head.ID =     1027;
 { 
MyMessage *MyMsgPtr;
MyMsgPtr = & MyMsg;
#if FastAssign
	MyMsgPtr->IPCNam2 = 02000220002L;
#else FastAssign
	MyMsgPtr->IPCNam2 =        0;
	MyMsgPtr->IPCNam2 |= _InLine(  1);
	MyMsgPtr->IPCNam2 |= _Deallocate( 0);
	MyMsgPtr->IPCNam2 |= _LongForm( 0);
	MyMsgPtr->IPCNam2 |= _TypeName(TypeInt32);
	MyMsgPtr->IPCNam2 |= _TypeSizeInBits(32);
	MyMsgPtr->IPCNam2 |= _NumObjects(1);
#endif FastAssign
	MyMsgPtr->Arg2 = (address);
#if FastAssign
	MyMsgPtr->IPCNam3 = 04000000000L;
#else FastAssign
	MyMsgPtr->IPCNam3 =        0;
	MyMsgPtr->IPCNam3 |= _InLine( 0);
	MyMsgPtr->IPCNam3 |= _Deallocate( 0);
	MyMsgPtr->IPCNam3 |= _LongForm(  1);
#endif FastAssign
	MyMsgPtr->TName3 = TypeInt8;
	MyMsgPtr->TSize3 = 8;
	MyMsgPtr->NumElts3 = (data_Cnt);
	MyMsgPtr->Arg3 = ( caddr_t )(data);
 } 
RepMsg.Head.MsgSize = sizeof (RepMsg);
RepMsg.Head.LocalPort = mach_reply_port;
if ( (GR = Send (& MyMsg.Head, 0, Wait)) != Success)
MsgError(GR);

if((GR = Receive( & (RepMsg.Head),0,LocalPt,ReceiveIt))!= Success)
MsgError(GR);
 { 
 if (RepMsg.Head.ID !=     1127)
MsgError(BadReply);
#if TypeCheck
 if (_X_TypeName(RepMsg.RetCodeType) != TypeInt16)
MsgError(BadReply);
#endif
	 if (RepMsg.RetCode != Success)
MsgError(BadReply);

#if TypeCheck
	 if (_X_TypeName(RepMsg.IPCNam4) != TypeInt32)
MsgError(BadReply);
#endif TypeCheck
	vm_write = (RepMsg.Arg4);
 return (vm_write);
 } 

 } 

kern_return_t	vm_copy ( ServPort,source_address,size,dest_address)
vm_task_t	ServPort;
vm_address_t	source_address;
vm_size_t	size;
vm_address_t	dest_address;
 { 

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
MyMessage  MyMsg;
RepMessage RepMsg;
kern_return_t	vm_copy;

MyMsg.Head.SimpleMsg =	1;
MyMsg.Head.MsgSize = sizeof (MyMsg);
MyMsg.Head.MsgType = NormalMsg;
MyMsg.Head.RemotePort = ServPort;
MyMsg.Head.LocalPort = mach_reply_port;
MyMsg.Head.ID =     1028;
 { 
MyMessage *MyMsgPtr;
MyMsgPtr = & MyMsg;
#if FastAssign
	MyMsgPtr->IPCNam2 = 02000220002L;
#else FastAssign
	MyMsgPtr->IPCNam2 =        0;
	MyMsgPtr->IPCNam2 |= _InLine(  1);
	MyMsgPtr->IPCNam2 |= _Deallocate( 0);
	MyMsgPtr->IPCNam2 |= _LongForm( 0);
	MyMsgPtr->IPCNam2 |= _TypeName(TypeInt32);
	MyMsgPtr->IPCNam2 |= _TypeSizeInBits(32);
	MyMsgPtr->IPCNam2 |= _NumObjects(1);
#endif FastAssign
	MyMsgPtr->Arg2 = (source_address);
#if FastAssign
	MyMsgPtr->IPCNam3 = 02000220002L;
#else FastAssign
	MyMsgPtr->IPCNam3 =        0;
	MyMsgPtr->IPCNam3 |= _InLine(  1);
	MyMsgPtr->IPCNam3 |= _Deallocate( 0);
	MyMsgPtr->IPCNam3 |= _LongForm( 0);
	MyMsgPtr->IPCNam3 |= _TypeName(TypeInt32);
	MyMsgPtr->IPCNam3 |= _TypeSizeInBits(32);
	MyMsgPtr->IPCNam3 |= _NumObjects(1);
#endif FastAssign
	MyMsgPtr->Arg3 = (size);
#if FastAssign
	MyMsgPtr->IPCNam4 = 02000220002L;
#else FastAssign
	MyMsgPtr->IPCNam4 =        0;
	MyMsgPtr->IPCNam4 |= _InLine(  1);
	MyMsgPtr->IPCNam4 |= _Deallocate( 0);
	MyMsgPtr->IPCNam4 |= _LongForm( 0);
	MyMsgPtr->IPCNam4 |= _TypeName(TypeInt32);
	MyMsgPtr->IPCNam4 |= _TypeSizeInBits(32);
	MyMsgPtr->IPCNam4 |= _NumObjects(1);
#endif FastAssign
	MyMsgPtr->Arg4 = (dest_address);
 } 
RepMsg.Head.MsgSize = sizeof (RepMsg);
RepMsg.Head.LocalPort = mach_reply_port;
if ( (GR = Send (& MyMsg.Head, 0, Wait)) != Success)
MsgError(GR);

if((GR = Receive( & (RepMsg.Head),0,LocalPt,ReceiveIt))!= Success)
MsgError(GR);
 { 
 if (RepMsg.Head.ID !=     1128)
MsgError(BadReply);
#if TypeCheck
 if (_X_TypeName(RepMsg.RetCodeType) != TypeInt16)
MsgError(BadReply);
#endif
	 if (RepMsg.RetCode != Success)
MsgError(BadReply);

#if TypeCheck
	 if (_X_TypeName(RepMsg.IPCNam5) != TypeInt32)
MsgError(BadReply);
#endif TypeCheck
	vm_copy = (RepMsg.Arg5);
 return (vm_copy);
 } 

 } 

kern_return_t	vm_regions ( ServPort,start,size,regions,regions_Cnt)
vm_task_t	ServPort;
vm_address_t	start;
vm_size_t	size;
vm_region_array_t	 * regions;
 Long  * regions_Cnt;
 { 

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
MyMessage  MyMsg;
RepMessage RepMsg;
kern_return_t	vm_regions;

MyMsg.Head.SimpleMsg =	1;
MyMsg.Head.MsgSize = sizeof (MyMsg);
MyMsg.Head.MsgType = NormalMsg;
MyMsg.Head.RemotePort = ServPort;
MyMsg.Head.LocalPort = mach_reply_port;
MyMsg.Head.ID =     1029;
 { 
MyMessage *MyMsgPtr;
MyMsgPtr = & MyMsg;
#if FastAssign
	MyMsgPtr->IPCNam2 = 02000220002L;
#else FastAssign
	MyMsgPtr->IPCNam2 =        0;
	MyMsgPtr->IPCNam2 |= _InLine(  1);
	MyMsgPtr->IPCNam2 |= _Deallocate( 0);
	MyMsgPtr->IPCNam2 |= _LongForm( 0);
	MyMsgPtr->IPCNam2 |= _TypeName(TypeInt32);
	MyMsgPtr->IPCNam2 |= _TypeSizeInBits(32);
	MyMsgPtr->IPCNam2 |= _NumObjects(1);
#endif FastAssign
	MyMsgPtr->Arg2 = (start);
#if FastAssign
	MyMsgPtr->IPCNam3 = 02000220002L;
#else FastAssign
	MyMsgPtr->IPCNam3 =        0;
	MyMsgPtr->IPCNam3 |= _InLine(  1);
	MyMsgPtr->IPCNam3 |= _Deallocate( 0);
	MyMsgPtr->IPCNam3 |= _LongForm( 0);
	MyMsgPtr->IPCNam3 |= _TypeName(TypeInt32);
	MyMsgPtr->IPCNam3 |= _TypeSizeInBits(32);
	MyMsgPtr->IPCNam3 |= _NumObjects(1);
#endif FastAssign
	MyMsgPtr->Arg3 = (size);
 } 
RepMsg.Head.MsgSize = sizeof (RepMsg);
RepMsg.Head.LocalPort = mach_reply_port;
if ( (GR = Send (& MyMsg.Head, 0, Wait)) != Success)
MsgError(GR);

if((GR = Receive( & (RepMsg.Head),0,LocalPt,ReceiveIt))!= Success)
MsgError(GR);
 { 
 if (RepMsg.Head.ID !=     1129)
MsgError(BadReply);
#if TypeCheck
 if (_X_TypeName(RepMsg.RetCodeType) != TypeInt16)
MsgError(BadReply);
#endif
	 if (RepMsg.RetCode != Success)
MsgError(BadReply);

#if TypeCheck
	 if (RepMsg.TName4 != TypeInt32)
MsgError(BadReply);
#endif TypeCheck
	( * regions_Cnt) = RepMsg.NumElts4;
	 * regions = ((vm_region_array_t)RepMsg.Arg4);

#if TypeCheck
	 if (_X_TypeName(RepMsg.IPCNam5) != TypeInt32)
MsgError(BadReply);
#endif TypeCheck
	vm_regions = (RepMsg.Arg5);
 return (vm_regions);
 } 

 } 

kern_return_t	vm_statistics ( ServPort,vm_stats)
vm_task_t	ServPort;
vm_statistics_data_t	 * vm_stats;
 { 

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
MyMessage  MyMsg;
RepMessage RepMsg;
kern_return_t	vm_statistics;

MyMsg.Head.SimpleMsg =	1;
MyMsg.Head.MsgSize = sizeof (MyMsg);
MyMsg.Head.MsgType = NormalMsg;
MyMsg.Head.RemotePort = ServPort;
MyMsg.Head.LocalPort = mach_reply_port;
MyMsg.Head.ID =     1030;
 { 
MyMessage *MyMsgPtr;
MyMsgPtr = & MyMsg;
 } 
RepMsg.Head.MsgSize = sizeof (RepMsg);
RepMsg.Head.LocalPort = mach_reply_port;
if ( (GR = Send (& MyMsg.Head, 0, Wait)) != Success)
MsgError(GR);

if((GR = Receive( & (RepMsg.Head),0,LocalPt,ReceiveIt))!= Success)
MsgError(GR);
 { 
 if (RepMsg.Head.ID !=     1130)
MsgError(BadReply);
#if TypeCheck
 if (_X_TypeName(RepMsg.RetCodeType) != TypeInt16)
MsgError(BadReply);
#endif
	 if (RepMsg.RetCode != Success)
MsgError(BadReply);

#if TypeCheck
	 if (_X_TypeName(RepMsg.IPCNam2) != TypeInt32)
MsgError(BadReply);
#endif TypeCheck
	 * vm_stats = (RepMsg.Arg2);

#if TypeCheck
	 if (_X_TypeName(RepMsg.IPCNam3) != TypeInt32)
MsgError(BadReply);
#endif TypeCheck
	vm_statistics = (RepMsg.Arg3);
 return (vm_statistics);
 } 

 } 

kern_return_t	pager_clean ( ServPort,pager_space,offset,size)
port_t	ServPort;
vm_pager_id_t	pager_space;
vm_offset_t	offset;
vm_size_t	size;
 { 

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
MyMessage  MyMsg;
RepMessage RepMsg;
kern_return_t	pager_clean;

MyMsg.Head.SimpleMsg =	1;
MyMsg.Head.MsgSize = sizeof (MyMsg);
MyMsg.Head.MsgType = NormalMsg;
MyMsg.Head.RemotePort = ServPort;
MyMsg.Head.LocalPort = mach_reply_port;
MyMsg.Head.ID =     1031;
 { 
MyMessage *MyMsgPtr;
MyMsgPtr = & MyMsg;
#if FastAssign
	MyMsgPtr->IPCNam2 = 02000220002L;
#else FastAssign
	MyMsgPtr->IPCNam2 =        0;
	MyMsgPtr->IPCNam2 |= _InLine(  1);
	MyMsgPtr->IPCNam2 |= _Deallocate( 0);
	MyMsgPtr->IPCNam2 |= _LongForm( 0);
	MyMsgPtr->IPCNam2 |= _TypeName(TypeInt32);
	MyMsgPtr->IPCNam2 |= _TypeSizeInBits(32);
	MyMsgPtr->IPCNam2 |= _NumObjects(1);
#endif FastAssign
	MyMsgPtr->Arg2 = (pager_space);
#if FastAssign
	MyMsgPtr->IPCNam3 = 02000220002L;
#else FastAssign
	MyMsgPtr->IPCNam3 =        0;
	MyMsgPtr->IPCNam3 |= _InLine(  1);
	MyMsgPtr->IPCNam3 |= _Deallocate( 0);
	MyMsgPtr->IPCNam3 |= _LongForm( 0);
	MyMsgPtr->IPCNam3 |= _TypeName(TypeInt32);
	MyMsgPtr->IPCNam3 |= _TypeSizeInBits(32);
	MyMsgPtr->IPCNam3 |= _NumObjects(1);
#endif FastAssign
	MyMsgPtr->Arg3 = (offset);
#if FastAssign
	MyMsgPtr->IPCNam4 = 02000220002L;
#else FastAssign
	MyMsgPtr->IPCNam4 =        0;
	MyMsgPtr->IPCNam4 |= _InLine(  1);
	MyMsgPtr->IPCNam4 |= _Deallocate( 0);
	MyMsgPtr->IPCNam4 |= _LongForm( 0);
	MyMsgPtr->IPCNam4 |= _TypeName(TypeInt32);
	MyMsgPtr->IPCNam4 |= _TypeSizeInBits(32);
	MyMsgPtr->IPCNam4 |= _NumObjects(1);
#endif FastAssign
	MyMsgPtr->Arg4 = (size);
 } 
RepMsg.Head.MsgSize = sizeof (RepMsg);
RepMsg.Head.LocalPort = mach_reply_port;
if ( (GR = Send (& MyMsg.Head, 0, Wait)) != Success)
MsgError(GR);

if((GR = Receive( & (RepMsg.Head),0,LocalPt,ReceiveIt))!= Success)
MsgError(GR);
 { 
 if (RepMsg.Head.ID !=     1131)
MsgError(BadReply);
#if TypeCheck
 if (_X_TypeName(RepMsg.RetCodeType) != TypeInt16)
MsgError(BadReply);
#endif
	 if (RepMsg.RetCode != Success)
MsgError(BadReply);

#if TypeCheck
	 if (_X_TypeName(RepMsg.IPCNam5) != TypeInt32)
MsgError(BadReply);
#endif TypeCheck
	pager_clean = (RepMsg.Arg5);
 return (pager_clean);
 } 

 } 

kern_return_t	pager_fetch ( ServPort,pager_space,offset,size)
port_t	ServPort;
vm_pager_id_t	pager_space;
vm_offset_t	offset;
vm_size_t	size;
 { 

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
MyMessage  MyMsg;
RepMessage RepMsg;
kern_return_t	pager_fetch;

MyMsg.Head.SimpleMsg =	1;
MyMsg.Head.MsgSize = sizeof (MyMsg);
MyMsg.Head.MsgType = NormalMsg;
MyMsg.Head.RemotePort = ServPort;
MyMsg.Head.LocalPort = mach_reply_port;
MyMsg.Head.ID =     1032;
 { 
MyMessage *MyMsgPtr;
MyMsgPtr = & MyMsg;
#if FastAssign
	MyMsgPtr->IPCNam2 = 02000220002L;
#else FastAssign
	MyMsgPtr->IPCNam2 =        0;
	MyMsgPtr->IPCNam2 |= _InLine(  1);
	MyMsgPtr->IPCNam2 |= _Deallocate( 0);
	MyMsgPtr->IPCNam2 |= _LongForm( 0);
	MyMsgPtr->IPCNam2 |= _TypeName(TypeInt32);
	MyMsgPtr->IPCNam2 |= _TypeSizeInBits(32);
	MyMsgPtr->IPCNam2 |= _NumObjects(1);
#endif FastAssign
	MyMsgPtr->Arg2 = (pager_space);
#if FastAssign
	MyMsgPtr->IPCNam3 = 02000220002L;
#else FastAssign
	MyMsgPtr->IPCNam3 =        0;
	MyMsgPtr->IPCNam3 |= _InLine(  1);
	MyMsgPtr->IPCNam3 |= _Deallocate( 0);
	MyMsgPtr->IPCNam3 |= _LongForm( 0);
	MyMsgPtr->IPCNam3 |= _TypeName(TypeInt32);
	MyMsgPtr->IPCNam3 |= _TypeSizeInBits(32);
	MyMsgPtr->IPCNam3 |= _NumObjects(1);
#endif FastAssign
	MyMsgPtr->Arg3 = (offset);
#if FastAssign
	MyMsgPtr->IPCNam4 = 02000220002L;
#else FastAssign
	MyMsgPtr->IPCNam4 =        0;
	MyMsgPtr->IPCNam4 |= _InLine(  1);
	MyMsgPtr->IPCNam4 |= _Deallocate( 0);
	MyMsgPtr->IPCNam4 |= _LongForm( 0);
	MyMsgPtr->IPCNam4 |= _TypeName(TypeInt32);
	MyMsgPtr->IPCNam4 |= _TypeSizeInBits(32);
	MyMsgPtr->IPCNam4 |= _NumObjects(1);
#endif FastAssign
	MyMsgPtr->Arg4 = (size);
 } 
RepMsg.Head.MsgSize = sizeof (RepMsg);
RepMsg.Head.LocalPort = mach_reply_port;
if ( (GR = Send (& MyMsg.Head, 0, Wait)) != Success)
MsgError(GR);

if((GR = Receive( & (RepMsg.Head),0,LocalPt,ReceiveIt))!= Success)
MsgError(GR);
 { 
 if (RepMsg.Head.ID !=     1132)
MsgError(BadReply);
#if TypeCheck
 if (_X_TypeName(RepMsg.RetCodeType) != TypeInt16)
MsgError(BadReply);
#endif
	 if (RepMsg.RetCode != Success)
MsgError(BadReply);

#if TypeCheck
	 if (_X_TypeName(RepMsg.IPCNam5) != TypeInt32)
MsgError(BadReply);
#endif TypeCheck
	pager_fetch = (RepMsg.Arg5);
 return (pager_fetch);
 } 

 } 
