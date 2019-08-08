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
/*CMU:	%M%	%I%	%G%	*/

/*
 **************************************************************************
 * HISTORY
 * 11-Oct-85  Robert V Baron (rvb) at Carnegie-Mellon University
 *	Added 6 and 7
 *
 * 18-Dec-84  Robert V Baron (rvb) at Carnegie-Mellon University
 *	Created symbolic names for nm trap indicator names
 **************************************************************************
 */

#define NM_MPS 0
#define NM_MACH_STAT 1
#define NM_CPU_NO 2
#define NM_PIDX 3
#define NM_PPTR 4
#define NM_KPORT 5
#define NM_IPC 6
#define NM_SHMRMAP 7
#define NM_SHM 8
#define NM_SHM_BYTES 9
#define NM_AAREA 10
