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
 * acc_errors.h
 *
 * Types exported by Accent except those for kernel traps.
 *
 * Michael B. Jones
 *
 * Copyright (C) 1981, 1982, 1983, 1984, 1985
 *	Carnegie-Mellon University
 *
 * HISTORY:
 * 11-Dec-85  Robert V Baron (rvb) at Carnegie-Mellon University
 *	Added error values for timing facility
 *
 *
 *  15-Aug-85	Michael B. Jones
 *		Merged in GeneralReturns from s5.b2 system release, moving
 *		new multi-processor errors from offset 60 to 70.
 *
 *  14-Aug-85	Michael B. Jones
 *		Split off from accenttype.h.
 */

#ifndef _Acc_Errors
#define _Acc_Errors

/*  
 *	 GeneralReturn
 *
 *   Purpose:
 *	 Values returned from system calls and system messages.
 *
 */

#ifndef	LOCORE
typedef short GeneralReturn;
#endif	LOCORE

#define AccErr 100

#define Dummy	 		(AccErr+0)
#define Success 		(AccErr+1)
#define SUCCESS 		(AccErr+1)
#define TimeOut 		(AccErr+2)
#define PortFull 		(AccErr+3)
#define WillReply 		(AccErr+4)
#define TooManyReplies 		(AccErr+5)
#define MemFault 		(AccErr+6)
#define NotAPort 		(AccErr+7)
#define BadRights 		(AccErr+8)
#define NoMorePorts 		(AccErr+9)
#define IllegalBacklog 		(AccErr+10)
#define NetFail 		(AccErr+11)
#define Intr 			(AccErr+12)
#define Other 			(AccErr+13)
#define NotPortReceiver 	(AccErr+14)
#define UnrecognizedMsgType 	(AccErr+15)
#define NotEnoughRoom 		(AccErr+16)
#define NotAnIPCCall 		(AccErr+17)
#define BadMsgType 		(AccErr+18)
#define BadIPCName 		(AccErr+19)
#define MsgTooBig 		(AccErr+20)
#define NotYourChild 		(AccErr+21)
#define BadMsg 			(AccErr+22)
#define OutOfIPCSpace 		(AccErr+23)
#define Failure 		(AccErr+24)
#define MapFull 		(AccErr+25)
#define WriteFault 		(AccErr+26)
#define BadKernelMsg 		(AccErr+27)
#define NotCurrentProcess	(AccErr+28)
#define CantFork 		(AccErr+29)
#define BadPriority 		(AccErr+30)
#define BadTrap 		(AccErr+31)
#define DiskErr 		(AccErr+32)
#define BadSegType 		(AccErr+33)
#define BadSegment 		(AccErr+34)
#define IsParent 		(AccErr+35)
#define IsChild 		(AccErr+36)
#define NoAvailablePages 	(AccErr+37)
#define FiveDeep 		(AccErr+38)
#define BadVPTable 		(AccErr+39)
#define VPExclusionFailure 	(AccErr+40)
#define MicroFailure 		(AccErr+41)
#define EStackTooDeep 		(AccErr+42)
#define MsgInterrupt 		(AccErr+43)
#define UncaughtException 	(AccErr+44)
#define BreakPointTrap 		(AccErr+45)
#define ASTInconsistency 	(AccErr+46)
#define InactiveSegment 	(AccErr+47)
#define SegmentAlreadyExists 	(AccErr+48)
#define OutOfImagSegments 	(AccErr+49)
#define NotASystemAddress 	(AccErr+50)
#define NotAUserAddress 	(AccErr+51)
#define BadCreateMask 		(AccErr+52)
#define BadRectangle 		(AccErr+53)
#define OutOfRectangleBounds 	(AccErr+54)
#define IllegalScanWidth 	(AccErr+55)
#define CoveredRectangle 	(AccErr+56)
#define BusyRectangle 		(AccErr+57)
#define NotAFont 		(AccErr+58)
#define PartitionFull 		(AccErr+59)

#define NoSuchPage		(AccErr+60)
#define NoRecoveryManager	(AccErr+61)
#define HaveRecManPortAlready	(AccErr+62)
#define RecovSegError		(AccErr+63)
#define FlushAborted		(AccErr+64)
#define PageNotRecoverable	(AccErr+65)
#define RecovQueueFull		(AccErr+66)

#define BadInitPort		(AccErr+70)	/* Was AccErr+60 */
#define InitPortInUse		(AccErr+71)
#define NotSuperUser		(AccErr+72)
#define OutOfSharedMemory	(AccErr+73)
#define ProcessSharedMemoryExceded	(AccErr+74)
#define BadSharedMemoryArg	(AccErr+75)
#define SharedMemoryHuh		(AccErr+76)
#define BadSharedMemorySize	(AccErr+77)
#define BadShmfreeRange		(AccErr+78)
#define BadReceiveSize		(AccErr+79)
#define TypeZero		(AccErr+80)
#define TypeTooBig		(AccErr+81)
#define AccErrEnd		(AccErr+84)


/*
 *   General error codes to be used
 *   by all modules that pass messages.
 */

#define MchmkErr	     0
#define	BadMsgID             1
#define	WrongArgs            2
#define	BadReply             3
#define	NoReply              4
#define	UnspecException      5
#define MchmkErrEnd	     5
	/* Message is an exception on behalf of a server */


#endif _Acc_Errors
