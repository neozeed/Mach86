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
 * perq_compat.h
 *
 * WARNING:  THE CONTENTS OF THIS FILE ARE OBSOLETE.
 *
 * Types for compatibility with the old perq native_c accenttype.h file.
 * No new code should be written which depends on anything in this file.
 *
 * Michael B. Jones
 *
 * History:
 *
 *  23-Oct-85	Michael B. Jones, Robert V. Baron
 *		Copied relevant pieces from native_c accenttype.h file.
 */

#ifndef _Perq_Compat
#define _Perq_Compat

typedef short		Boolean;
typedef long		pointer;

typedef char		Character;
typedef char		Char;
typedef int16		Integer;
typedef int32		Long;

#define PageWordSize	(PageByteSize/2)
#define DiskBufSize	PageUnitSize

typedef unsigned char   Bit8;
typedef unsigned short  Bit16;
typedef int32           Bit32;
typedef Bit32           *pBit32;

typedef struct
		{
		Long            lsw;
		Long            msw;
		}
	Bit64;

#define Temporary	SegTemporary
#define Permanent	SegPermanent
#define Bad		SegBad
/*#define SegPhysical	SegPhysical */
#define Imaginary	SegImaginary
#define Shadow		SegShadow
#define Recoverable	SegRecoverable
#define Paging		SegPaging

typedef union
		{
		long            lng;
		char            byte[4];
		}
	OldDiskAddr;

#define MaxPorts	256 /* Code should not be using MaxPorts anymore */

#if ! BIT_STRUCTURES
typedef long            TypeType;


/*
 *	Can't use bit fields, thus we use hand-shifting.
 *	Definitions are as follows:
 *		_Foo            Function to insert into Foo
 *		_X_Foo          Function to extract from Foo
 *		_bFoo           Number of bits to shift Foo
 *		_sFoo           Size of Foo
 */

#define _sTypeName       8
#define _sTypeSizeInBits 8
#define _sNumObjects    12
#define _sInLine         1
#define _sLongForm       1
#define _sDeallocate     1

#define _bTypeName      0
#define _bTypeSizeInBits 8
#define _bNumObjects    16
#define _bInLine        28
#define _bLongForm      29
#define _bDeallocate    30

#define _TypeName(x)       ((x&(~(-1<< _sTypeName)))      << _bTypeName)
#define _TypeSizeInBits(x) ((x&(~(-1<< _sTypeSizeInBits)))<< _bTypeSizeInBits)
#define _NumObjects(x)     ((x&(~(-1<< _sNumObjects)))    << _bNumObjects)
#define _InLine(x)         ((x&(~(-1<< _sInLine)))        << _bInLine)
#define _LongForm(x)       ((x&(~(-1<< _sLongForm)))      << _bLongForm)
#define _Deallocate(x)     ((x&(~(-1<< _sDeallocate)))    << _bDeallocate)

#define _X_TypeName(x)       ((x>>_bTypeName)      &(~(-1<< _sTypeName)))
#define _X_TypeSizeInBits(x) ((x>>_bTypeSizeInBits)&(~(-1<< _sTypeSizeInBits)))
#define _X_NumObjects(x)     ((x>>_bNumObjects)    &(~(-1<< _sNumObjects)))
#define _X_InLine(x)         ((x>>_bInLine)        &(~(-1<< _sInLine)))
#define _X_LongForm(x)       ((x>>_bLongForm)      &(~(-1<< _sLongForm)))
#define _X_Deallocate(x)     ((x>>_bDeallocate)    &(~(-1<< _sDeallocate)))
#endif          BIT_STRUCTURES

/* ptrPortBitArray = ^PortBitArray;
   PortBitArray    = packed array [0..MaxPorts-1] of boolean;

typedef Boolean         PortBitArray[MaxPorts];
typedef PortBitArray    *ptrPortBitArray;
typedef Port            PortArray[MaxPorts];
*/

#define MAXPROCS 	(NumProcs-1)
#define NUMPRIORITIES	NumPriorities
#define NUMSLEEPQS	NumSleepQs
#define NUMQUEUES	NumQueues

#define Supervisor	SupervisorState
#define Privileged	PrivilegedState
/*#define BadSupervisor	BadSupervisor */
#define User		UserState

typedef Integer         *ptrInteger;
typedef Boolean         *ptrBoolean;

#define MAXPARTCHARS	MaxPartChars
#define MAXDPCHARS	MaxDPChars
#define MAXPARTITIONS	MaxPartitions
#define MAXDEVICES	MaxDevices

typedef PartString	_PartStringType;
typedef DevPartString   _DevPartType;

#define Root		PartRoot
#define UnUsed		PartUnUsed
#define Segment		PartSegment
#define PLX		PartPLX

#endif _Perq_Compat
