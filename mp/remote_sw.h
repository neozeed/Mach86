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
 *	File:	remote_sw.h
 *	Author:	Avadis Tevanian, Jr.
 *
 *	Copyright (C) 1984, Avadis Tevanian (rvb)		CMU
 *
 *	Define type codes for interprocessor communication.
 *
 * HISTORY
 * 24-Feb-85	Robert V Baron
 *	add UNASLEEP and UNTIMEOUT
 *
 * 23-Sep-84  Avadis Tevanian (avie) at Carnegie-Mellon University
 *	Created
 *
 */

struct mp_comm_entry {
	int	(*mp_call)();		/* handler */
};

#define TYPE_ML		0		/* ml device driver */
#define TYPE_WAKEUP	1		/* remote wakeup */
#define TYPE_PSIGNAL	2		/* remote psignal */
#define TYPE_IMC	3		/* IMC handler */
#define TYPE_UNASLEEP	4		/* cancel asleep */
#define TYPE_UNTIMEOUT	5		/* remote untimeout */

#define NCOMMTYPES	6		/* number of codes */

