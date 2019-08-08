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
 * Copyright (c) 1982 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)tty_conf.c	6.5 (Berkeley) 6/8/85
 */
#if	CMU
/*
 **********************************************************************
 * HISTORY
 * 16-Feb-86  Bill Bolosky (bolosky) at Carnegie-Mellon University
 *	Added support for Sailboat MS and AED devices under switch romp.
 *
 * 25-Jan-86  Avadis Tevanian (avie) at Carnegie-Mellon University
 *	Upgraded to 4.3.
 *
 * 30-Jul-85  Mike Accetta (mja) at Carnegie-Mellon University
 *	NFE:  Added Front End line discipline.
 *	CS_COMPAT:  Added MPX line discipline for now.
 *	[V1(1)]
 *
 **********************************************************************
 */
 
#include "cs_compat.h"
 
#include "fe.h"
#endif	CMU

#include "param.h"
#include "systm.h"
#include "buf.h"
#include "ioctl.h"
#include "tty.h"
#include "conf.h"

int	nodev();
int	nulldev();

int	ttyopen(),ttyclose(),ttread(),ttwrite(),nullioctl(),ttstart();
int	ttyinput();

#include "bk.h"
#if NBK > 0
int	bkopen(),bkclose(),bkread(),bkinput(),bkioctl();
#endif

#include "tb.h"
#if NTB > 0
int	tbopen(),tbclose(),tbread(),tbinput(),tbioctl();
#endif
#ifdef romp /* This is a kludge; supposed to be machine independent -- sac */
#include "ms.h"
#if NMS > 0
int	msdopen(),msdclose(),msdread(),msdinput(),msdioctl();
#endif NMS

#include "aed.h"
#if NAED > 0
int	aed_ttread(), aed_ttyinput();
#endif NAED
#endif romp

#if	NFE > 0
int	felopen(), felclose(), felrint(), felstart();
 
#endif	NFE
#if	CS_COMPAT
int	mxopen(), mxclose(), mcread(), mcwrite(), mxioctl();
 
#endif	CS_COMPAT
struct	linesw linesw[] =
{
	ttyopen, nulldev, ttread, ttwrite, nullioctl,
	ttyinput, nodev, nulldev, ttstart, nulldev,
#if NBK > 0
	bkopen, bkclose, bkread, ttwrite, bkioctl,
	bkinput, nodev, nulldev, ttstart, nulldev,
#else
	nodev, nodev, nodev, nodev, nodev,
	nodev, nodev, nodev, nodev, nodev,
#endif
	ttyopen, ttyclose, ttread, ttwrite, nullioctl,
	ttyinput, nodev, nulldev, ttstart, nulldev,
#if NTB > 0
	tbopen, tbclose, tbread, nodev, tbioctl,
	tbinput, nodev, nulldev, ttstart, nulldev,		/* 3 */
#else
	nodev, nodev, nodev, nodev, nodev,
	nodev, nodev, nodev, nodev, nodev,
#endif
#if NTB > 0
	tbopen, tbclose, tbread, nodev, tbioctl,
	tbinput, nodev, nulldev, ttstart, nulldev,		/* 4 */
#else
	nodev, nodev, nodev, nodev, nodev,
	nodev, nodev, nodev, nodev, nodev,
#endif
#if	NFE > 0
#if defined(romp) && NMS > 0
#include "Error...front and support and romp MS support are mutually exclusive"
#endif error stuff....
	felopen, felclose, ttread, ttwrite, nullioctl,
	felrint, nodev, nulldev, felstart, nulldev,		/* 5 */
#else	NFE
#if defined(romp) && NMS > 0
	msdopen, msdclose, msdread, nodev, msdioctl,
	msdinput, nodev, nulldev, ttstart, nulldev,		/* 5 */
#else	romp && NMS
	nodev, nodev, nodev, nodev, nodev,
	nodev, nodev, nodev, nodev, nodev,
#endif	romp && NMS
#endif	NFE
#if	defined(romp) && CS_COMPAT
#include "Error: backward compatibility not supported on the romp..."
#endif
#if	CS_COMPAT
	mxopen, mxclose, mcread, mcwrite, mxioctl,
	nulldev, nulldev, nulldev, nulldev, nulldev,		/* 6 */
#else	CS_COMPAT
#if defined(romp) && NAED > 0
	ttyopen, nodev, aed_ttread, nodev, nodev,		/* 6 */
	aed_ttyinput, nodev, nodev, nodev, nodev,
#else	romp && NAED
	nodev, nodev, nodev, nodev, nodev,
	nodev, nodev, nodev, nodev, nodev,
#endif	romp && NAED
#endif	CS_COMPAT
};

int	nldisp = sizeof (linesw) / sizeof (linesw[0]);

/*
 * Do nothing specific version of line
 * discipline specific ioctl command.
 */
/*ARGSUSED*/
nullioctl(tp, cmd, data, flags)
	struct tty *tp;
	char *data;
	int flags;
{

#ifdef lint
	tp = tp; data = data; flags = flags;
#endif
	return (-1);
}
