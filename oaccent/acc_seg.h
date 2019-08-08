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
 * acc_seg.h
 *
 * Types for Accent Segment calls.
 *
 * Michael B. Jones
 *
 * Copyright (C) 1981, 1982, 1983, 1984, 1985
 *	Richard F. Rashid, Michael B. Jones, and Carnegie-Mellon University
 *
 * History:
 *
 *  15-Aug-85	Michael B. Jones
 *		Merged in Perq definitions for s5.b2 release.
 *		Added missing types from Perq accent's vmtypes.pas.
 *
 *  13-Jun-85	Michael B. Jones
 *		Started.
 */

#ifndef _Acc_Seg
#define _Acc_Seg


/*
 * Accent Segment Identifier Type
 */

typedef VirtualAddress	SegID;
typedef VirtualAddress	DiskAddr;

/*
 * Kinds of segments supported by Accent.
 */

typedef short	SpiceSegKind;

#define SegTemporary	0
#define SegPermanent	1
#define SegBad		2
#define	SegPhysical	3
#define SegImaginary	4
#define SegShadow	5
#define SegRecoverable	6
#define SegPaging	7

/*
 * Device/Partition name string.
 */

DefineString(PartString,    MaxPartChars);
DefineString(DevPartString, MaxDPChars);

typedef unsigned short PartitionType;

#define PartRoot	0
#define PartUnUsed	1
#define PartSegment	2
#define PartPLX		3

typedef PartInfo PartList[MaxPartitions];
typedef PartInfo *ptrPartList;

#endif	_Acc_Seg
