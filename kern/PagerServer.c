/* 
 * Module PagerServer
 */
#include 	"AccentType.h"

#include "mach_if.h"
#include "accent.h"


 static void Xpager_init(InP, ReP) caddr_t InP, ReP;
 { 

/* Define local messages and types */

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
 typedef MyMessage * ptrMyMsg;
 typedef RepMessage * ptrRepMsg;
ptrMyMsg MyMsgP;
ptrRepMsg RepMsgP;

port_t	ServPort;
vm_pager_id_t	pager_space;
port_t	request_port;
port_t	name_port;
 extern void pager_init ( );


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
	 if (_X_TypeName(MyMsgP->IPCNam3) != TypePt)
		 { 
		RepMsg.RetCode = WRONGARGS;
		 return ;
		 } 
#endif TypeCheck
	request_port = (MyMsgP->Arg3);

#if TypeCheck
	 if (_X_TypeName(MyMsgP->IPCNam4) != TypePt)
		 { 
		RepMsg.RetCode = WRONGARGS;
		 return ;
		 } 
#endif TypeCheck
	name_port = (MyMsgP->Arg4);


/* Call service routine */

pager_init (ServPort, pager_space, request_port, name_port);

/* Gather reply arguments */

RepMsgP->RetCode = NoReply;
 } 

 static void Xpager_read(InP, ReP) caddr_t InP, ReP;
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
 Integer 	TName5;
 Integer 	TSize5;
 Long 	NumElts5;
 caddr_t 	Arg5;
TypeType IPCNam6;
boolean_t	Arg6;
 } RepMessage;
 typedef MyMessage * ptrMyMsg;
 typedef RepMessage * ptrRepMsg;
ptrMyMsg MyMsgP;
ptrRepMsg RepMsgP;

port_t	ServPort;
vm_pager_id_t	pager_space;
vm_offset_t	pager_offset;
vm_size_t	size;
pointer_t	data;
 Long data_Cnt;
 extern boolean_t	pager_read ( );


MyMsgP = (ptrMyMsg) (InP);
RepMsgP = (ptrRepMsg) (ReP);
RepMsgP->RetCode = Success;
RepMsgP->Head.SimpleMsg = FALSE;
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
	pager_offset = (MyMsgP->Arg3);

#if TypeCheck
	 if (_X_TypeName(MyMsgP->IPCNam4) != TypeInt32)
		 { 
		RepMsg.RetCode = WRONGARGS;
		 return ;
		 } 
#endif TypeCheck
	size = (MyMsgP->Arg4);

RepMsgP->Arg6 = (
/* Call service routine */

pager_read (ServPort, pager_space, pager_offset, size,  & data,  & data_Cnt));

/* Gather reply arguments */

#if FastAssign
	RepMsgP->IPCNam5 = 014000000000L;
#else FastAssign
	RepMsgP->IPCNam5 =        0;
	RepMsgP->IPCNam5 |= _InLine( FALSE);
	RepMsgP->IPCNam5 |= _Deallocate(  TRUE);
	RepMsgP->IPCNam5 |= _LongForm(  TRUE);
#endif FastAssign
	RepMsgP->TName5 = TypeInt8;
	RepMsgP->TSize5 = 8;
	RepMsgP->NumElts5 = (data_Cnt);
	RepMsgP->Arg5 = 	( caddr_t )(data);
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

 static void Xpager_write(InP, ReP) caddr_t InP, ReP;
 { 

/* Define local messages and types */

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
 typedef MyMessage * ptrMyMsg;
 typedef RepMessage * ptrRepMsg;
ptrMyMsg MyMsgP;
ptrRepMsg RepMsgP;

port_t	ServPort;
vm_pager_id_t	pager_space;
vm_offset_t	pager_offset;
pointer_t	data;
 Long data_Cnt;
 extern boolean_t	pager_write ( );


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
	pager_offset = (MyMsgP->Arg3);

#if TypeCheck
	 if (MyMsgP->TName4 != TypeInt8)
		 { 
		RepMsg.RetCode = WRONGARGS;
		 return ;
		 } 
#endif TypeCheck
	data_Cnt = MyMsgP->NumElts4;
	data = ((pointer_t)MyMsgP->Arg4);

RepMsgP->Arg5 = (
/* Call service routine */

pager_write (ServPort, pager_space, pager_offset, data, data_Cnt));

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
 Boolean PagerServer(InP, RepP)  caddr_t InP, RepP;
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

if ((InMsgP->ID >     1202) || (InMsgP->ID <     1200))
 { 
RepMsgP->RetCode = BadMsgID;
 return (FALSE);
 } 
 else  { 
 switch (InMsgP->ID -     1200)
 { 
 case        0:
Xpager_init(InP, RepP);
 break; 

 case        1:
Xpager_read(InP, RepP);
 break; 

 case        2:
Xpager_write(InP, RepP);
 break; 

 } 
 } 
 return (TRUE);
 } 
