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
/*	table.h	CMU	03/30/83	*/

/*
 **********************************************************************
 * HISTORY
 * 30-Aug-85  Mike Accetta (mja) at Carnegie-Mellon University
 *	Added TBL_U_TTYD.
 *	[V1(1)]
 *
 * 30-Mar-83  Mike Accetta (mja) at Carnegie-Mellon University
 *	Created (V3.06h).
 *
 **********************************************************************
 */

#define	TBL_TTYLOC	0	/* index by device number */
#define	TBL_U_TTYD	1	/* index by process ID */
