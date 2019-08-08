#include "Pager.h"
#include "AccCall.h"
#include "AccentUser.h"
#define FastAssign 1
#define TypeCheck 1
 static Port	ReplyPort;
 static GeneralReturn	GR;


void InitPager (RPort) Port RPort;
 { 
if (RPort == NullPort)
 { 
if ((GR = AllocatePort (KernelPort, &ReplyPort, 5)) != Success)
ReplyPort = DataPort;
 } 
 else 
ReplyPort = RPort;
 } 

void pager_init ( ServPort,pager_space,request_port,name_port)
port_t	ServPort;
vm_pager_id_t	pager_space;
port_t	request_port;
port_t	name_port;
 { 

 typedef  struct { 
Msg	Head;
TypeType IPCNam2;
vm_pager_id_t	Arg2;
TypeType IPCNam3;
port_t	Arg3;
TypeType IPCNam4;
port_t	Arg4;
 } MyMessage;

 typedef  struct { 
Msg	Head;
TypeType	RetCodeType;
 Integer 	RetCode;
 } RepMessage;
MyMessage  MyMsg;
RepMessage RepMsg;

MyMsg.Head.SimpleMsg =	FALSE;
MyMsg.Head.MsgSize = sizeof (MyMsg);
MyMsg.Head.MsgType = NormalMsg;
MyMsg.Head.RemotePort = ServPort;
MyMsg.Head.LocalPort = ReplyPort;
MyMsg.Head.ID =     1200;
 { 
MyMessage *MyMsgPtr;
MyMsgPtr = & MyMsg;
#if FastAssign
	MyMsgPtr->IPCNam2 = 02000220002L;
#else FastAssign
	MyMsgPtr->IPCNam2 =        0;
	MyMsgPtr->IPCNam2 |= _InLine(  TRUE);
	MyMsgPtr->IPCNam2 |= _Deallocate( FALSE);
	MyMsgPtr->IPCNam2 |= _LongForm( FALSE);
	MyMsgPtr->IPCNam2 |= _TypeName(TypeInt32);
	MyMsgPtr->IPCNam2 |= _TypeSizeInBits(32);
	MyMsgPtr->IPCNam2 |= _NumObjects(1);
#endif FastAssign
	MyMsgPtr->Arg2 = (pager_space);
#if FastAssign
	MyMsgPtr->IPCNam3 = 02000220006L;
#else FastAssign
	MyMsgPtr->IPCNam3 =        0;
	MyMsgPtr->IPCNam3 |= _InLine(  TRUE);
	MyMsgPtr->IPCNam3 |= _Deallocate( FALSE);
	MyMsgPtr->IPCNam3 |= _LongForm( FALSE);
	MyMsgPtr->IPCNam3 |= _TypeName(TypePt);
	MyMsgPtr->IPCNam3 |= _TypeSizeInBits(32);
	MyMsgPtr->IPCNam3 |= _NumObjects(1);
#endif FastAssign
	MyMsgPtr->Arg3 = (request_port);
#if FastAssign
	MyMsgPtr->IPCNam4 = 02000220006L;
#else FastAssign
	MyMsgPtr->IPCNam4 =        0;
	MyMsgPtr->IPCNam4 |= _InLine(  TRUE);
	MyMsgPtr->IPCNam4 |= _Deallocate( FALSE);
	MyMsgPtr->IPCNam4 |= _LongForm( FALSE);
	MyMsgPtr->IPCNam4 |= _TypeName(TypePt);
	MyMsgPtr->IPCNam4 |= _TypeSizeInBits(32);
	MyMsgPtr->IPCNam4 |= _NumObjects(1);
#endif FastAssign
	MyMsgPtr->Arg4 = (name_port);
 } 
if ( (GR = Send (& MyMsg.Head, 0, Wait)) != Success)
MsgError(GR);
 } 

boolean_t	pager_read ( ServPort,pager_space,pager_offset,size,data,data_Cnt)
port_t	ServPort;
vm_pager_id_t	pager_space;
vm_offset_t	pager_offset;
vm_size_t	size;
pointer_t	 * data;
 Long  * data_Cnt;
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
 Integer 	TName5;
 Integer 	TSize5;
 Long 	NumElts5;
 caddr_t 	Arg5;
TypeType IPCNam6;
boolean_t	Arg6;
 } RepMessage;
MyMessage  MyMsg;
RepMessage RepMsg;
boolean_t	pager_read;

MyMsg.Head.SimpleMsg =	TRUE;
MyMsg.Head.MsgSize = sizeof (MyMsg);
MyMsg.Head.MsgType = NormalMsg;
MyMsg.Head.RemotePort = ServPort;
MyMsg.Head.LocalPort = ReplyPort;
MyMsg.Head.ID =     1201;
 { 
MyMessage *MyMsgPtr;
MyMsgPtr = & MyMsg;
#if FastAssign
	MyMsgPtr->IPCNam2 = 02000220002L;
#else FastAssign
	MyMsgPtr->IPCNam2 =        0;
	MyMsgPtr->IPCNam2 |= _InLine(  TRUE);
	MyMsgPtr->IPCNam2 |= _Deallocate( FALSE);
	MyMsgPtr->IPCNam2 |= _LongForm( FALSE);
	MyMsgPtr->IPCNam2 |= _TypeName(TypeInt32);
	MyMsgPtr->IPCNam2 |= _TypeSizeInBits(32);
	MyMsgPtr->IPCNam2 |= _NumObjects(1);
#endif FastAssign
	MyMsgPtr->Arg2 = (pager_space);
#if FastAssign
	MyMsgPtr->IPCNam3 = 02000220002L;
#else FastAssign
	MyMsgPtr->IPCNam3 =        0;
	MyMsgPtr->IPCNam3 |= _InLine(  TRUE);
	MyMsgPtr->IPCNam3 |= _Deallocate( FALSE);
	MyMsgPtr->IPCNam3 |= _LongForm( FALSE);
	MyMsgPtr->IPCNam3 |= _TypeName(TypeInt32);
	MyMsgPtr->IPCNam3 |= _TypeSizeInBits(32);
	MyMsgPtr->IPCNam3 |= _NumObjects(1);
#endif FastAssign
	MyMsgPtr->Arg3 = (pager_offset);
#if FastAssign
	MyMsgPtr->IPCNam4 = 02000220002L;
#else FastAssign
	MyMsgPtr->IPCNam4 =        0;
	MyMsgPtr->IPCNam4 |= _InLine(  TRUE);
	MyMsgPtr->IPCNam4 |= _Deallocate( FALSE);
	MyMsgPtr->IPCNam4 |= _LongForm( FALSE);
	MyMsgPtr->IPCNam4 |= _TypeName(TypeInt32);
	MyMsgPtr->IPCNam4 |= _TypeSizeInBits(32);
	MyMsgPtr->IPCNam4 |= _NumObjects(1);
#endif FastAssign
	MyMsgPtr->Arg4 = (size);
 } 
RepMsg.Head.MsgSize = sizeof (RepMsg);
RepMsg.Head.LocalPort = ReplyPort;
if ( (GR = Send (& MyMsg.Head, 0, Wait)) != Success)
MsgError(GR);

if((GR = Receive( & (RepMsg.Head),0,LocalPt,ReceiveIt))!= Success)
MsgError(GR);
 { 
 if (RepMsg.Head.ID !=     1301)
MsgError(BadReply);
#if TypeCheck
 if (_X_TypeName(RepMsg.RetCodeType) != TypeInt16)
MsgError(BadReply);
#endif
	 if (RepMsg.RetCode != Success)
MsgError(BadReply);

#if TypeCheck
	 if (RepMsg.TName5 != TypeInt8)
MsgError(BadReply);
#endif TypeCheck
	( * data_Cnt) = RepMsg.NumElts5;
	 * data = ((pointer_t)RepMsg.Arg5);

#if TypeCheck
	 if (_X_TypeName(RepMsg.IPCNam6) != TypeInt32)
MsgError(BadReply);
#endif TypeCheck
	pager_read = (RepMsg.Arg6);
 return (pager_read);
 } 

 } 

boolean_t	pager_write ( ServPort,pager_space,pager_offset,data,data_Cnt)
port_t	ServPort;
vm_pager_id_t	pager_space;
vm_offset_t	pager_offset;
pointer_t	data;
 Long data_Cnt;
 { 

 typedef  struct { 
Msg	Head;
TypeType IPCNam2;
vm_pager_id_t	Arg2;
TypeType IPCNam3;
vm_offset_t	Arg3;
TypeType IPCNam4;
 Integer 	TName4;
 Integer 	TSize4;
 Long 	NumElts4;
 caddr_t 	Arg4;
 } MyMessage;

 typedef  struct { 
Msg	Head;
TypeType	RetCodeType;
 Integer 	RetCode;
TypeType IPCNam5;
boolean_t	Arg5;
 } RepMessage;
MyMessage  MyMsg;
RepMessage RepMsg;
boolean_t	pager_write;

MyMsg.Head.SimpleMsg =	FALSE;
MyMsg.Head.MsgSize = sizeof (MyMsg);
MyMsg.Head.MsgType = NormalMsg;
MyMsg.Head.RemotePort = ServPort;
MyMsg.Head.LocalPort = ReplyPort;
MyMsg.Head.ID =     1202;
 { 
MyMessage *MyMsgPtr;
MyMsgPtr = & MyMsg;
#if FastAssign
	MyMsgPtr->IPCNam2 = 02000220002L;
#else FastAssign
	MyMsgPtr->IPCNam2 =        0;
	MyMsgPtr->IPCNam2 |= _InLine(  TRUE);
	MyMsgPtr->IPCNam2 |= _Deallocate( FALSE);
	MyMsgPtr->IPCNam2 |= _LongForm( FALSE);
	MyMsgPtr->IPCNam2 |= _TypeName(TypeInt32);
	MyMsgPtr->IPCNam2 |= _TypeSizeInBits(32);
	MyMsgPtr->IPCNam2 |= _NumObjects(1);
#endif FastAssign
	MyMsgPtr->Arg2 = (pager_space);
#if FastAssign
	MyMsgPtr->IPCNam3 = 02000220002L;
#else FastAssign
	MyMsgPtr->IPCNam3 =        0;
	MyMsgPtr->IPCNam3 |= _InLine(  TRUE);
	MyMsgPtr->IPCNam3 |= _Deallocate( FALSE);
	MyMsgPtr->IPCNam3 |= _LongForm( FALSE);
	MyMsgPtr->IPCNam3 |= _TypeName(TypeInt32);
	MyMsgPtr->IPCNam3 |= _TypeSizeInBits(32);
	MyMsgPtr->IPCNam3 |= _NumObjects(1);
#endif FastAssign
	MyMsgPtr->Arg3 = (pager_offset);
#if FastAssign
	MyMsgPtr->IPCNam4 = 04000000000L;
#else FastAssign
	MyMsgPtr->IPCNam4 =        0;
	MyMsgPtr->IPCNam4 |= _InLine( FALSE);
	MyMsgPtr->IPCNam4 |= _Deallocate( FALSE);
	MyMsgPtr->IPCNam4 |= _LongForm(  TRUE);
#endif FastAssign
	MyMsgPtr->TName4 = TypeInt8;
	MyMsgPtr->TSize4 = 8;
	MyMsgPtr->NumElts4 = (data_Cnt);
	MyMsgPtr->Arg4 = ( caddr_t )(data);
 } 
RepMsg.Head.MsgSize = sizeof (RepMsg);
RepMsg.Head.LocalPort = ReplyPort;
if ( (GR = Send (& MyMsg.Head, 0, Wait)) != Success)
MsgError(GR);

if((GR = Receive( & (RepMsg.Head),0,LocalPt,ReceiveIt))!= Success)
MsgError(GR);
 { 
 if (RepMsg.Head.ID !=     1302)
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
	pager_write = (RepMsg.Arg5);
 return (pager_write);
 } 

 } 
