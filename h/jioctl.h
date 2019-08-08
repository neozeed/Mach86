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
**	Unix to Blit I/O control codes
*/

#ifndef _JIOCTL_
#define	_JIOCTL_
#ifdef KERNEL
#include "ioctl.h"
#else
#include <sys/ioctl.h>
#endif

/*
#define	JBOOT		_IO(j, 1)
#define	JTERM		_IO(j, 2)
#define	JMPX		_IO(j, 3)
#define	JTIMO		_IO(j, 4)
#define	JTIMOM		_IO(j, 6)
#define	JZOMBOOT	_IO(j, 7)
#define JSMPX		_IOW(j, 9, int)
*/
#define JSMPX		TIOCUCNTL
#define JMPX		_IO(u,0)
#define	JBOOT		_IO(u, 1)
#define	JTERM		_IO(u, 2)
#define	JTIMO		_IO(u, 4)	/* Timeouts in seconds */
#define	JTIMOM		_IO(u, 6)	/* Timeouts in millisecs */
#define	JZOMBOOT	_IO(u, 7)
#define JWINSIZE	TIOCGWINSZ
#define JSWINSIZE	TIOCSWINSZ

/**	Channel 0 control message format **/

struct jerqmesg
{
	char	cmd;		/* A control code above */
	char	chan;		/* Channel it refers to */
};

/*
**	Character-driven state machine information for Blit to Unix communication.
*/

#define	C_SENDCHAR	1	/* Send character to layer process */
#define	C_NEW		2	/* Create new layer process group */
#define	C_UNBLK		3	/* Unblock layer process */
#define	C_DELETE	4	/* Delete layer process group */
#define	C_EXIT		5	/* Exit */
#define	C_BRAINDEATH	6	/* Send terminate signal to proc. group */
#define	C_SENDNCHARS	7	/* Send several characters to layer proc. */
#define	C_RESHAPE	8	/* Layer has been reshaped */
#define C_JAGENT	9	/* Jagent return (What do they mean? */

/*
 * Map to new window structure
 */
#define bitsx	ws_xpixel
#define bitsy	ws_ypixel
#define	bytesx	ws_col
#define	bytesy	ws_row
#define jwinsize winsize

/*
**	Usual format is: [command][data]
*/
#endif _JIOCTL_
