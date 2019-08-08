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
/* $Header: sys_machdep.c,v 4.0 85/07/15 00:49:04 ibmacis GAMMA $ */
/* $Source: /ibm/acis/usr/sys_ca/ca/RCS/sys_machdep.c,v $ */

#ifndef lint
static char *rcsid = "$Header: sys_machdep.c,v 4.0 85/07/15 00:49:04 ibmacis GAMMA $";
#endif

/*     sys_machdep.c   6.1     83/07/29        */

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/dir.h"
#include "../h/user.h"
#include "../h/ioctl.h"
#include "../h/file.h"
#include "../h/proc.h"
#include "../h/uio.h"
#include "../h/kernel.h"
#include "../h/buf.h"
#include "../machine/pte.h"


#ifdef COMPAT
 /*
  * Note: these tables are sorted by
  * ioctl "code" (in ascending order).
  */
int dctls[] = {
	DKIOCHDR, 0
};
int fctls[] = {
	FIOCLEX, FIONCLEX, FIOASYNC, FIONBIO, FIONREAD, 0
};
int mctls[] = {
	MTIOCTOP, MTIOCGET, 0
};
int tctls[] = {
	TIOCGETD, TIOCSETD, TIOCHPCL, TIOCMODG, TIOCMODS,
	TIOCGETP, TIOCSETP, TIOCSETN, TIOCEXCL, TIOCNXCL,
	TIOCFLUSH, TIOCSETC, TIOCGETC, TIOCREMOTE, TIOCMGET,
	TIOCMBIC, TIOCMBIS, TIOCMSET, TIOCSTART, TIOCSTOP,
	TIOCPKT, TIOCNOTTY, TIOCSTI, TIOCOUTQ, TIOCGLTC,
	TIOCSLTC, TIOCSPGRP, TIOCGPGRP, TIOCCDTR, TIOCSDTR,
	TIOCCBRK, TIOCSBRK, TIOCLGET, TIOCLSET, TIOCLBIC,
	TIOCLBIS, 0
};

/*
 * Map an old style ioctl command to new.
 */
mapioctl(cmd)
	int cmd;
{
	register int *map, c;

	switch ((cmd >> 8) & 0xff) {

	case 'd':
		map = dctls;
		break;

	case 'f':
		map = fctls;
		break;

	case 'm':
		map = mctls;
		break;

	case 't':
		map = tctls;
		break;

	default:
		return (0);
	}
	while ((c = *map) && (c & 0xff) < (cmd & 0xff))
		map++;
	if (c && (c & 0xff) == (cmd & 0xff))
		return (c);
	return (0);
}


#endif
