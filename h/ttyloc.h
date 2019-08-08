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
/*	ttyloc.h	CMU	03/28/83	*/

/*
 **********************************************************************
 * HISTORY
 * 15-May-85  Mike Accetta (mja) at Carnegie-Mellon University
 *	Made conditional on _TTYLOC_ symbol to permit recursive
 *	includes.
 *	[V1(1)]
 *
 * 09-Sep-83  Mike Accetta (mja) at Carnegie-Mellon University
 *	Added TLC_TACTTY and TLC_RANDOMTTY definitions.
 *
 * 28-Mar-83  Mike Accetta (mja) at Carnegie-Mellon University
 *	Created (V3.06h).
 *
 **********************************************************************
 */
 
#ifndef	_TTYLOC_
#define	_TTYLOC_
struct ttyloc
{
    long tlc_hostid;		/* host identifier of location (on Internet) */
    long tlc_ttyid;		/* terminal identifier of location (on host) */
};

/*
 *  Pseudo host location of Front Ends
 */
#define	TLC_FEHOST	((128<<24)|(2<<16)|(254<<8)|255)
#define	TLC_SPECHOST	((0<<24)|(0<<16)|(0<<8)|0)

/*
 *  Pseudo terminal index of console
 */
#define	TLC_CONSOLE	((unsigned short)0177777)
#define	TLC_DISPLAY	((unsigned short)0177776)

/*
 *  Constants
 */
#define	TLC_UNKHOST	((long)(0))
#define	TLC_UNKTTY	((long)(-1))
#define	TLC_DETACH	((long)(-2))
#define	TLC_TACTTY	((long)(-3))		/* unused by kernel */
#define	TLC_RANDOMTTY	((long)(-4))		/* unused by kernel */

#ifdef	KERNEL
/*
 *  IP address of local host (as determined by the network module)
 */
extern long myipaddr();
#define	TLC_MYHOST	(myipaddr())

#endif
#endif	_TTYLOC_
