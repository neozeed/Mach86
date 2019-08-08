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
 * MachDep.h -- Vax version
 *
 * Machine/architecture dependent types exported by Accent.
 *
 * Michael B. Jones
 *
 * Copyright (C) 1984 Michael B. Jones, and Carnegie-Mellon University
 *
 * History:
 *
 *  15-Aug-85	Michael B. Jones
 *		Moved page size parameters to acc_impl.h.
 *
 *  27-Jun-85	Mark Hjelm
 *		Added machine names.  Removed Pointer.
 *
 *  17-Jun-85	Michael B. Jones
 *		Added Align_Type for structure/parameter aligment boundary.
 *
 *  13-Jun-85   Michael B. Jones
 *		Fixed the "2 << PageBits" to be "1 << PageBits".
 *
 *  06-Dec-84	Michael B. Jones
 *		Added Units_Per and Unit_Size macros.
 *
 *  16-Oct-84	Michael B. Jones
 *		Started by defining Page* and Pointer values.
 */

#ifndef _MachDep
#define _MachDep


typedef char		Unit_Type;	/* Basic addressing unit of machine */

#define	BytesPerUnit	sizeof(Unit_Type) /* Bytes per basic addressing unit */
#define Units_Per(x)	(((x) + BytesPerUnit - 1) / BytesPerUnit)
#define Unit_Size(x)	Units_Per(sizeof(x))


typedef long		Align_Type;	/* Structure/parameter aligment boundary type */


#define	MSB_First	0		/* Byte order in integers */
#define LSB_First	(! MSB_First)


#define VAX	1


#endif	_MachDep
