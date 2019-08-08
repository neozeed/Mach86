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
 *  File: ipc.h (Copyright 1980 (c) Richard F. Rashid)
 *
 **********************************************************************
 * HISTORY
 *  9-Nov-85  Avadis Tevanian (avie) at Carnegie-Mellon University
 *	Changed to redefine some procedure names for coexistence with
 *	the newer IPC facility.
 *
 * 21-May-85  Mike Accetta (mja) at Carnegie-Mellon University
 *	Upgraded to 4.2BSD.
 *	[V1(1)]
 *
 * 31-Jan-85  Glenn Marcy (gm0w) at Carnegie-Mellon University
 *	Changed MsgType struct to be unsigned (V3.07n).
 *
 * 15-Jan-85  Glenn Marcy (gm0w) at Carnegie-Mellon University
 *	Remove inappropriate definitions of tracing flags (V3.07n).
 *
 * 17-Dec-84  Glenn Marcy (gm0w) at Carnegie-Mellon University
 *	Removed definitions for true and false.  Changed definition of
 *	boolean to use new bool enumerated type in <sys/types.h>.
 *
 * 18-Oct-84  Glenn Marcy (gm0w) at Carnegie-Mellon University
 *	Defined MsgType union to match ipcio(5).  Allows fixes for some
 *	kernel indexing bugs.
 *
 * 21-Aug-81  Mike Accetta (mja) at Carnegie-Mellon University
 *	Upgraded for 4.1BSD by changing the  "ProcTable" definition to a
 *	pointer, and by changing "NPROC" to "nproc" in the definitions
 *	of "NOONE" and "INTRANSIT" (V3.00).
 *
 **********************************************************************
 *
 *  Purpose: 
 *      Header file for the UNIX IPC facility.
 *
 *  Author:
 *      Richard F. Rashid 
 *
 */

#ifdef	KERNEL
#include "mach_acc.h"
#else	KERNEL
#include <sys/features.h>
#endif	KERNEL

#ifdef	KERNEL
#if	MACH_ACC
#define NewPort			OLD_NewPort
#define RemPort			OLD_RemPort
#define GiveReceiveRights	OLD_GiveReceiveRights
#define SendKernelMsg		OLD_SendKernelMsg
#define GiveSendRights		OLD_GiveSendRights
#define SendAMsg		OLD_SendAMsg
#define QueueMsg		OLD_QueueMsg
#define GiveMsgToUser		OLD_GiveMsgToUser
#define DQMsg			OLD_DQMsg
#define FreeKMsgQ		OLD_FreeKMsgQ
#define FreePortQ		OLD_FreePortQ
#endif	MACH_ACC
#endif	KERNEL

/*
 *  Constants:
 *      integer,boolean,endcase
 *
 *  Purpose:
 *      Various macros for making the text read more easily.
 *
 */

#define integer                 int
#define boolean                 bool
#define endcase                 break;

/*
 *  Types:
 *      process,localport,routineaddr,counter,
 *      porttype
 *
 *  Purpose:
 *      Various types used by the IPC.
 *
 */

#define process			short
#define counter                 unsigned char
#define localport               unsigned char
#define porttype                unsigned char

#define routineaddr             caddr_t

#define MAXPORTS                256

/*
 *  Constants:
 *      IPCERRORS,
 *      NOTAPORT, NOTPORTRECEIVER, PORTFULL,
 *      UNRECOGNISEDMSGTYPE, NOTENOUGHROOM,
 *      TOOMANYPORTS, NOTANIPCALL, BADMSGTYPE
 *      BADIPCNAME, BADIPCNAME, NOTYOURCHILD
 *      BADMSG, OUTOFIPCSPACE
 *
 *  Purpose:
 *      IPC error codes.  Note that a user process
 *    will see all of these error codes as negative
 *    numbers starting at -128 and increasing. Thus
 *    NOTAPORT will be -128 and NOTPORTRECEIVER will
 *    be -127, etc.  This is the result of sign
 *    extension in converting a character to an
 *    integer. 
 */

#define IPCERRORS               0200

#define NOTAPORT                0 + IPCERRORS
#define NOTPORTRECEIVER         1 + IPCERRORS
#define PORTFULL                2 + IPCERRORS
#define UNRECOGNISEDMSGTYPE     3 + IPCERRORS
#define NOTENOUGHROOM           4 + IPCERRORS
#define TOOMANYPORTS            5 + IPCERRORS
#define NOTANIPCCALL            6 + IPCERRORS
#define BADMSGTYPE              7 + IPCERRORS
#define BADIPCNAME              8 + IPCERRORS
#define MSGTOOBIG               9 + IPCERRORS
#define BADRIGHTS               10+ IPCERRORS
#define NOTYOURCHILD            11+ IPCERRORS
#define BADMSG                  12+ IPCERRORS
#define OUTOFIPCSPACE           13+ IPCERRORS  


/*
 *  Constants:
 *      NORMALMSG,EMERGENCYMSG
 *
 *  Purpose:
 *      Possible values of MsgType field in a message header.
 *
 */

#define NORMALMSG               0
#define EMERGENCYMSG            1
#define NUMMSGTYPES             2

#ifndef	MACHDEP
struct MsgTypeField
  {
    unsigned	AttributeCode:1,
		UnusedAttributeCode:7, /* for matching with NUMMSGTYPES */
		Priority:4,
		UnReliable:1,
		UnOrdered:1,
		Secure:1,
		UnFlowControlled:1,
		MaxAge:16; /* In seconds, 0 = no maximum */
  };

/*
 *  Constants:
 *      WAIT,DONTWAIT,REPLY
 *
 *  Purpose:
 *      Possible sending options.
 *
 */

#define WAIT                    0
#define DONTWAIT                1
#define REPLY                   2

/*
 *  Constants:
 *      NULLPORT,KERNELPORT,DATAPORT,FIRSTNONRESERVEDPORT,
 *      ALLPORTS
 *
 *  Purpose:
 *      Distinguished local port numbers (or in the case
 *    of ALLPORTS a number which implies all ports).
 *
 */

#define NULLPORT                NULL
#define KERNELPORT              1
#define DATAPORT                2
#define FIRSTNONRESERVEDPORT    3
#define ALLPORTS               -1

/*
 *  Constants:
 *      ARRAYOFPTRS,PTRTOARRAY,INLINE
 *
 *  Purpose:
 *      Values of Descriptor.DescKind.
 *
 */

#define PTRTOARRAY      0
#define ARRAYOFPTRS     1
#define INLINE          2

/*
 *  Structure:
 *      Msg
 *
 *  Purpose:
 *      Defines format of message header in user area.
 *
 */

struct Msg
  {
    union {
	unsigned MU_Int;
	struct MsgTypeField MU_Field;
    } Msg_U;
#define MsgType Msg_U.MU_Int
#define MsgCode Msg_U.MU_Field.AttributeCode
    porttype    LPType;
    localport   LocalPort;
    porttype    RPType;
    localport   RemotePort;
    integer     ID;
    integer     Type;
    integer     NumElts;
    caddr_t     DataPtr;
  };

/*
 *  Structure:
 *      LS
 *
 *  Purpose:
 *      Linear structure descriptor.
 */

struct LS
  {
    integer     LSSize;
    caddr_t     LSPtr;
    integer     LSDataSize;
    caddr_t     LSDataPtr;
  };

/*
 *  Structure:
 *      LSArray
 *
 *  Purpose:
 *      Array of linear structures.
 */

struct LSArray
  {
    struct LS LSEntry[1];/* Undefined number of entries intended. */
  };

/*
 *  Structure:
 *      Descriptor
 *
 *  Purpose:
 *      General structure declaration for descriptor.
 *
 */

struct Descriptor
  {
    unsigned integer    DescName:7,
                        DescKind:2,
                        NumBitsPerElt:6,
                        LeftToRight:1,
                        NumEltsPerBlk:6,
                        PrefixBits:5,
                        SuffixBits:5;
    integer             NumObjects;
    union               
      {
        caddr_t                 DPtr;
        integer                 DescData;
        struct Descriptor *     DescPtrs[1]; /* Undefined number intended. */
        struct LS *             LSPtrs[1];   /* Undefined number intended. */
      } DataPart;
  };

/*
 *  Structure:
 *      DescArray
 *
 *  Purpose:
 *      Array of descriptors.
 */

struct DescArray
  {
    struct Descriptor DescEntry[1];/* Undefined number of entries intended. */
  };

#endif	MACHDEP
/*
 *  Structure:
 *      SetOfPorts
 *
 *  Purpose:
 *      A bit array describing the ports on which a process
 *    is currently waiting.
 */

struct SetOfPorts
  {
    integer PortBits[(MAXPORTS+31)/32];
  };
#ifndef	MACHDEP

/*
 *  Structure:
 *      LSDescriptor
 *
 *  Purpose:
 *      The structure of a message data descriptor.
 *
 */

struct LSDescriptor
  {
    integer     DType;
    integer     DSize;
    integer     DOffset;
  };

/*
 *  Constants:
 *      LINEARSTRUCTURE,TYPEINTEGER,TYPEPTOWNERSHIP,
 *      TYPEPTRECEIVERIGHTS,TYPEPTALLRIGHTS,
 *      TYPEPT,TYPEUNSTRUCTURED
 *
 *  Purpose:
 *      Types of message descriptors.
 *
 */

#define TYPEUNSTRUCTURED        0
#define TYPELINEARSTRUCTURE     1
#define TYPEINTEGER             2
#define TYPEPTOWNERSHIP         3
#define TYPEPTRECEIVERIGHTS     4
#define TYPEPTALLRIGHTS         5
#define TYPEPT                  6
#define TYPEDESCRIPTOR          7
#define TYPECHAR                8
#define TYPEBYTE                9
#define TYPEREAL               10
        
/*
 *  Constants:
 *      ALLOCATEPORT,DEALLOCATEPORT,
 *      SEND,RECEIVE,PREVIEW,
 *      MESSAGEWAIT,PORTSWITHMESSAGESWAITING,
 *      ASSERTNAME,LOCATE,REMOVENAME,
 *      INCREMENTUSECOUNT,DECREMENTUSECOUNT,
 *      SETBACKLOG,CHILDPORTS,GETSIGNAL,
 *      DISMISSSIGNAL,DEFINESIGNAL,CLEARSIGNAL
 *
 *  Purpose:
 *      Define IPC commands.
 *
 */

#define ALLOCATEPORT             0
#define DEALLOCATEPORT           1
#define SEND                     2
#define RECEIVE                  3
#define PREVIEW                  4
#define MESSAGEWAIT              5
#define PORTSWITHMESSAGESWAITING 6
#define ASSERTNAME               7
#define REMOVENAME               8
#define LOCATE                   9
#define INCREMENTUSECOUNT       10
#define DECREMENTUSECOUNT       11
#define SETBACKLOG              12
#define CHILDPORTS              13
#define GETSIGNAL               14
#define DISMISSSIGNAL           15
#define DEFINESIGNAL            16
#define CLEARSIGNAL             17

/*
 *  Constants:
 *      PORTDELETED,MSGACCEPTED,OWNERSHIPRIGHTS,
 *      RECEIVERIGHTS,INTERPOSE,INTERPOSEDONE,
 *      KERNELMSGERROR
 *
 *  Purpose:
 *      Kernel generated messages ids.
 */

#define PORTDELETED               1
#define MSGACCEPTED               2
#define OWNERSHIPRIGHTS           3
#define RECEIVERIGHTS             4
#define INTERPOSE                 5
#define INTERPOSEDONE             6
#define KERNELMSGERROR            7

/*
 *  Constants:
 *      MAXIPCARGS
 *
 *  Purpose:
 *      Maximum number of arguments to an IPC call.
 */

#define MAXIPCARGS                4

/*
 *  Constants:
 *      DEFAULTBACKLOG,MAXUSECOUNT,
 *      KERNELPROC,MAXBACKLOG,MAXMSGSIZE,
 *      MAXDESCDEPTH,NOONE,INTRANSIT,MAXDESCDEPTH
 *
 *  Purpose:
 *      Miscellaneous constants of the implementation.
 *
 */

#define KERNELPROC                0
#define NOONE                     (nproc)       /* Illegal process numbers */
#define INTRANSIT                 (nproc+1)     /* used for intermediate */
                                                /* port right states. */
#define DEFAULTBACKLOG            1
#define MAXBACKLOG                4
#define MAXUSECOUNT             255
#define MAXMSGSIZE             4096             /* bytes */
#define MAXDESCDEPTH             20

#endif	MACHDEP
#ifdef KERNEL
#ifndef	MACHDEP
/*
 *******************************************************************
 *******************************************************************
 *                      IPC constants                              *
 *******************************************************************
 *******************************************************************
 */

/*
 *  Constants:
 *      NOTWAITING,TIMEDOUT,MSGWAIT,FULLWAIT
 *
 *  Purpose:
 *      States of a process may be in when using the
 *    IPC.  
 *
 */

#define NOTWAITING                0
#define TIMEDOUT                  1
#define FULLWAIT                  2
#define ALLPORTWAIT		  3
#define ONEPORTWAIT		  4
#define MULTIPLEPORTWAIT	  5

/*
 *  Constants:
 *      DONTWAKE,SENDACK,WAKEME
 *
 *  Purpose:
 *      Values of KMsg.What which indicate what a receiver
 *    should do to the process which sent a message which is
 *    being moved from the secondary to the primary queue of
 *    a port.
 *
 */

#define DONTWAKE                  0
#define SENDACK                   1
#define WAKEME                    2


/*
 *******************************************************************
 *******************************************************************
 *                      IPC structures                             *
 *******************************************************************
 *******************************************************************
 */

/*
 *  Structure:
 *      IPCName
 *
 *  Purpose:
 *      Holds strings for name service facility.
 */

struct IPCName
  {
    struct Queue Link;
    char String[120];
  };
  
/*
 *  Structure:
 *      KMsg
 *
 *  Purpose:
 *      Basic data structure for a message stored in the kernel.
 */

struct KMsg
  {
    struct Queue  Link;
    short integer WakeupProc;
    short integer What;
    union {
	integer KMU_Int;
	struct MsgTypeField KMU_Field;
    } KMsg_U;
#define KMsgType KMsg_U.KMU_Int
#define KMsgCode KMsg_U.KMU_Field.AttributeCode
    porttype      KLPType;
    struct Port  *KLocalPort;
    porttype      KRPType;
    struct Port  *KRemotePort;
    integer       KID;
    integer       StructSize;
    caddr_t       LinearStructure;
    integer       DataSize;
    caddr_t       LinearData;
    integer       CurrentByte;
    integer       StructAlloc;
    integer       DataAlloc;
  };

/*
 *  Constant:
 *      TABLESIZE
 *
 *  Structure:
 *      HashTable
 *
 *  Purpose:
 *      Defines format of hash table used both for looking up 
 *    the local port name which corresponds to a global port 
 *    name for a particular process and for looking up the 
 *    global port name associated with a local port for a
 *    given process.  Bucket hashing is used.
 */

#define TABLESIZE                128

struct HashTable
  {
    struct Queue HashHead[TABLESIZE];
  };
  
/*
 *  Constant:
 *      NUMDATASIZES
 *
 *  Structure:
 *      FreeDataTable
 *
 *  Purpose:
 *      An array of queues for data blocks of different sizes.
 */

#define NUMDATASIZES             10

struct FreeDataTable
  {
    struct Queue DataHead[NUMDATASIZES];
  };

/*
 *  Structure:
 *      PortEntry
 *
 *  Purpose:
 *      Defines the structure of entries in hash table buckets.
 * 
 */

struct PortEntry
  {
    struct Queue   Link;
    integer        Key1;
    integer        Key2;
    integer        Value1;
    integer        Value2;
  };

/*
 *  Aliases:
 *      LTGValue,GTLValue,UseCount
 *
 *  Purpose:
 *      These names are used to refer to specific fields
 *    in the general purpose hash table entry defined above.
 *    LTGValue is the global name associated with a process local
 *    name for a port.   GTLValue is the process local name 
 *    associated with the global name of a port.
 *
 */

#define LTGValue                Value1
#define UseCount                Value2
#define GTLValue                Value1

/*
 *  Structure:
 *      Port
 *
 *  Purpose:
 *      Defines the port data structure.
 */

struct Port
  {
    struct Queue  Link;
    struct Queue  SecondaryMsgQ;
    unsigned char NumQueued;
    unsigned char Backlog;
    process       Owner;
    process       Receiver;
  };
#endif	MACHDEP

/*
 *  Structure:
 *      Proc
 *
 *  Purpose:
 *      Describes per process IPC information.
 */

struct Proc
  {
    boolean	      Active;
    unsigned char     State;
    boolean	      TimerSet;
    localport         WaitPort;
    struct Queue      MsgsWaiting;
    struct SetOfPorts WaitPorts;
    struct SetOfPorts PortsInUse;
    struct KMsg      *PreviewMsg;
    struct 
      {

        boolean Want;
        unsigned char Which;
        boolean Set;
        unsigned char HowMany;    

      } Signal[NUMMSGTYPES];
  };

#ifndef	MACHDEP
/*
 *******************************************************************
 *******************************************************************
 *              Foward declarations and globals                    *
 *******************************************************************
 *******************************************************************
 */

/*
 *  Forward declarations:
 *      HashLookup,GetFree,KMsgAllocate
 *      GetIPCName,DQMsg,NameHash,NameEqual
 *      PortHash,PortEqual,TranGP,TranLP
 *
 *  Purpose:
 *      To satisfy the compiler which is too stupid to defer
 *   type-checking of return values from subroutines in the
 *   same file.
 *
 */

IPCInit();
IPCTimer();
struct IPCName   *GetIPCName();
struct IPCName   *GetName();
struct Port      *GetPort();
struct PortEntry *GetHashEntry();
caddr_t           GetMemory();
struct PortEntry *HashLookup();
struct KMsg      *GetKMsg();
integer           NameHash();
boolean           NameEqual();
integer           PortHash();
boolean           PortEqual();
struct KMsg      *DQMsg();
integer           TranGP();
struct Port      *TranLP();
struct Port      *FindName();
boolean		  TestBit();
boolean		  IPCClearSignal();
boolean		  IPCDefineSignal();
boolean		  IPCSetBacklog();
boolean		  IPCDecrementUseCount();
boolean		  IPCIncrementUseCount();
boolean		  IPCRemoveName();
boolean		  IPCAssertName();
boolean		  IPCPortsWithMessagesWaiting();
boolean		  IPCDeallocatePort();
boolean		  IPCPortsWithMessagesWaiting();
boolean		  LegalPort();

/*  Macros:
 *      Me, MePtr, GTPort, LTPort *
 *  Purpose:
 *      These macros define short names for commonly used
 *    information.  'Me' is the
 *    current process index; 'MePtr' is a pointer to the
 *    Proc structure for the current process. GTPort and
 *    LTPort determine the correspondence between local
 *    and global port names. 
 */

#define Me        ((u.u_procp)->p_ndx)

#define MePtr     (&ProcTable[Me])

/*
 *  Data structures:
 *      FreeData
 *      FreeKMsgQ
 *      ProcTable
 *      FreePortQ
 *      FreeEntryQ
 *      FreeNameQ
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

extern struct FreeDataTable FreeData;
extern struct Queue FreeKMsgQ;
extern struct Queue FreeNameQ;
extern struct Queue FreePortQ;
extern struct Queue FreeEntryQ;
#endif	MACHDEP	
extern struct Proc *ProcTable;			/* added 8/21 for 4.1 by mja */
#ifndef	MACHDEP	
extern struct HashTable LocalToGlobal;
extern struct HashTable GlobalToLocal;
extern struct HashTable IPCNameTable;

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

extern boolean IPCStarted;


#endif	MACHDEP
#endif
