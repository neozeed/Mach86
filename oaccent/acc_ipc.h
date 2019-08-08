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
 * acc_ipc.h
 *
 * Types exported by Accent except those for kernel traps.
 *
 * Michael B. Jones
 *
 * Copyright (C) 1981, 1982, 1983, 1984, 1985
 *	Carnegie-Mellon University
 *
 * HISTORY
 * 12-May-86  Rick Rashid (rfr) at Carnegie-Mellon University
 *	Added new Send/Receive options to Send call.
 *
 *
 *  15-Aug-85	Michael B. Jones
 *		Merged in kernel messages from s5.b2 system release
 *		for recoverable segment processing.
 *
 *  14-Aug-85	Michael B. Jones
 *		Added NoTimeout and ImmediateTimeout values for MicroSeconds
 *		argument of Receive, making explicit the special semantics.
 *
 *  14-Aug-85	Michael B. Jones
 *		Split off from accenttype.h.
 */

#ifndef _Acc_IPC
#define _Acc_IPC

/*
 * Port type.
 */

typedef long	Port;

/*
 *   Constants:
 *	 NullPort, KernelPort, DataPort, FirstNonReservedPort,
 *	 AllPorts
 * 
 *   Purpose:
 *	 Distinguished local port numbers (or in the case
 *	 of AllPorts a number which implies all ports).
 * 
 */

#define	NullPort              0L
#define	KernelPort            1L
#define	DataPort              2L
#define	AllPorts              (-1L)


/*
 *   Structure:
 *	 Msg, ptrMsg
 * 
 *   Purpose:
 *	 Defines format of message header in user area.
 */

typedef struct Msg {
#if PERQ
		short_bool	SimpleMsg;
#else PERQ
		byte		RESERVED[3];
		byte_bool	SimpleMsg;
#endif PERQ
		long		MsgSize;
		long		MsgType;
		Port		LocalPort;
		Port		RemotePort;
		long		ID;
} Msg, *ptrMsg;


/*
 *   Structure:
 *	 TypeType
 * 
 *   Purpose:
 *	 Data structure for a message field type descriptor.
 */

typedef struct type_ {
	byte		TypeName;     
	byte		TypeSizeInBits;
	unsigned int
			NumObjects	: 12,
			InLine		: 1,
			LongForm	: 1,
			Deallocate	: 1;
} type_;


#if BIT_STRUCTURES	/* The old Perq native_c TypeType was a long */
typedef type_	TypeType;
#endif


/*
 *    Structure
 *	 LongTypeType
 * 
 *    Purpose
 *	 Long form structure declaration for msg field type descriptor.
 * 
 */

typedef struct {			/* When DataType.LongForm */
	byte		FILLER1;
	byte		FILLER2;
	unsigned int
			FILLER3		: 12,
			InLine		: 1,
			LongForm	: 1,	/* Always true in this case */
			Deallocate	: 1;
	short		TypeName;
	short		TypeSizeInBits;
	long		NumObjects;
} LongTypeType;


/*
 *   Purpose:
 *	 Some types of message descriptors.
 * 
 */

#define	TypeUnstructured	0
#define	TypeBit			0
#define	TypeBoolean		0
#define TypeShort		1
#define	TypeInt16		1
#define TypeLong		2
#define	TypeInt32		2
#define	TypePtOwnership		3
#define	TypePtReceive		4
#define	TypePtAll		5
#define	TypePt			6
#define	TypeChar		8
#define	TypeInt8		9
#define	TypeByte		9
#define	TypeReal		10
#define	TypePStat		11
#define	TypeString		12
#define	TypeSegID		13
#define TypePage		14


/*
 *   Constant:
 *	 DefaultBacklog, MaxBacklog
 *
 *   Purpose:
 *	 Number of messages held per port.
 */

#define	DefaultBacklog	0

typedef	short	BackLogValue;	/* 0..MaxBackLog */


/*
 *   Constants:
 *	 NormalMsg, EmergencyMsg
 * 
 *   Purpose:
 *	 Possible values of MsgType field in a message header.
 * 
 */

#define	NormalMsg             0
#define	EmergencyMsg          1
#define	NumMsgTypes           2

/*
 *   Constants:
 *	 Wait, DontWait, Reply
 * 
 *   Purpose:
 *	 Possible sending options.
 * 
 */

#define	Wait                  0
#define	DontWait              1
#define	Reply                 2
#define Send_Rec_DefaultPts   5
#define Send_Rec_AllPts       6
#define Send_Rec_LocalPt      7

typedef	short         SendOption;             /* Wait..Reply */

/*
 *   Constants:
 *	 Preview, ReceiveIt, ReceiveWait
 * 
 *   Purpose:
 *	 Possible receiving options.
 * 
 */

#define	ReceiveIt               1
#define InternalCall		3

typedef	short         ReceiveOption;       /* Preview..ReceiveWait */

/*
 *   Constants:
 *	 DefaultPts, AllPts, LocalPt
 *   Purpose:
 *	 Possible port options on receive
 */

#define	DefaultPts            0
#define	AllPts                1
#define	LocalPt               2

typedef	short         PortOption;           /* DefaultPts..LocalPt */

/*
 *   Constants:
 *	NoTimeout, ImmediateTimeout
 *   Purpose:
 *	Number of microseconds to wait for a receive.
 */

#define NoTimeout		0L
#define ImmediateTimeout	(-1L)

typedef	long	MicroSeconds;

/*
 * Various kinds of port array references.
 */

typedef Port	*ptrPort;

typedef Port	*ptrPortArray;
typedef Port	*ptrAllPortArray;


/*
 *   Constants:
 *	 ExplicitDealloc, ProcessDeath, NetworkTrouble
 *
 *   Purpose:
 *	 Reason for a port to be deallocated and/or destroyed.
 */

#define	ExplicitDealloc       0L
#define	ProcessDeath          1L
#define	NetworkTrouble        2L

typedef	long	PortDeath;	/* ExplicitDealloc .. NetworkTrouble */


/*
 *   Purpose:
 *	 Kernel generated messages ids.
 */

#define M_KernID		   0100
#define	M_PortDeleted              ( M_KernID + 001 )
#define	M_MsgAccepted              ( M_KernID + 002 )
#define	M_OwnershipRights          ( M_KernID + 003 )
#define	M_ReceiveRights            ( M_KernID + 004 )
#define	M_GeneralKernelReply       ( M_KernID + 006 )
#define	M_KernelMsgError           ( M_KernID + 007 )
#define	M_ParentForkReply          ( M_KernID + 010 )
#define	M_ChildForkReply           ( M_KernID + 011 )
#define	M_DebugMsg                 ( M_KernID + 012 )
#define M_AboutToFlushPage	   ( M_KernID + 013 )
#define M_ModifiedPage		   ( M_KernID + 014 )
#define M_ErrorLogMsg		   ( M_KernID + 015 )
#define M_KernIDEnd		   ( M_KernID + 015 )


/*
 *	Setup Message Header for fields:
 *		Local Port, Remote Port, Message Total Size
 *		Message ID, Msg Type, Simple Msg flag
 *
 */
		
/*
	Template:
	MsgHdrMacro(<msg>,
		*LocalPort*, , *RemotePort*, , *ID*, ,
		*MsgSize*, , *MsgType*, , *SimpleMsg*, )
	or typically
	MsgHdrMacro(,
		*LocalPort*, , *RemotePort*, , *ID*, ,
		*MsgSize*, , *MsgType*, NORMALMSG, *SimpleMsg*, TRUE)
 */

#define MsgHdrMacro(mesg, fill1, local, fill2, remote, fill3, id, fill4, size, fill5, type, fill6, simple) \
{\
mesg.SimpleMsg = simple;\
mesg.MsgSize = size;\
mesg.MsgType = type;\
mesg.LocalPort = local;\
mesg.RemotePort = remote;\
mesg.ID = id;\
}

/*
 *	Fill in the TypeTypeMacro fields of the first argument:
 *		Name, SizeInBits, Number,
 *		InLine, LongForm, Deallocate
 *
 */

/*
	Template:
	TypeTypeMacro(<type>,
		*TypeName*, , *TypeSizeInBits*, , *NumObjects*, ,
		*InLine*, , *LongForm*, , *Deallocate*, )
	or typically
	TypeTypeMacro(,
		*TypeName*, , *TypeSizeInBits*, 32, *NumObjects*, 1,
		*InLine*, TRUE, *LongForm*, FALSE, *Deallocate*, FALSE)
 */

#define TypeTypeMacro(typ, fill1, name, fill2, size, fill3, num, fill4, inline, fill5, long, fill6, deal) \
{\
typ.TypeName = name;\
typ.TypeSizeInBits = size;\
typ.NumObjects = num;\
typ.InLine = inline;\
typ.LongForm = long;\
typ.Deallocate = deal;\
}

#endif _Acc_IPC
