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
 * acc_part.h -- Vax version
 *
 * Implementation dependent types for Accent partition calls.
 *
 * Michael B. Jones
 *
 * Copyright (C) 1981, 1982, 1983, 1984, 1985
 *	Richard F. Rashid, Michael B. Jones, and Carnegie-Mellon University
 *
 * History:
 *
 *  24-Oct-85	Michael B. Jones
 *		Created Vax version.  PartInfo is currently empty.
 */

#ifndef _Acc_Part
#define _Acc_Part

#if ! VAX
Wrong version of acc_part.h!
#endif

typedef         struct          /* entry in the PartTable */
  {
/*  DiskAddr       PartHeadFree;        /* pointer to Head of Free List */
/*  DiskAddr       PartTailFree;        /* pointer to tail of Free List */
/*  DiskAddr       PartInfoBlk;         /* pointer to PartInfoBlock */
/*  SegID          PartRootDir;         /* SegID of Root Directory */
/*  short	   PartNumOps;          /* Now many operations done since
/*					   last update of PartInfoBlock */
/*  long           PartNumFree;         /* HINT of how many free pages */
/*  short_bool     PartInUse;           /* this entry in PartTable is valid */
/*  short_bool     PartMounted;         /* this partition is mounted */
/*  short          PartDevice;          /* which disk this partition is in */
/*  DiskAddr       PartStart;           /* Disk Address of 1st page */
/*  DiskAddr       PartEnd;             /* Disk Address of last page */
/*  PartitionType  PartKind;            /* Root or Leaf */
/*  PartString     PartName;            /* name of this partition */
/*  short_bool     PartExUse;           /* Opened exclusively */
    long           Unused;              /* Port is not returned */
  }
	PartInfo;

#endif	_Acc_Part
