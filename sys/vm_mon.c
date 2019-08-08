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
 *	@(#)vm_mon.c	6.4 (Berkeley) 6/8/85
 */

#if	CMU
/***********************************************************************
 * HISTORY
 * 16-Feb-86  Bill Bolosky (bolosky) at Carnegie-Mellon University
 *	Added change in trace1 from IBM to support compilation on
 *	Sailboat.  Under switch ROMP.
 *
 */

#include "mach_vm.h"
#endif	CMU

#if	MACH_VM
#else	MACH_VM

#include "param.h"
#include "systm.h"
#include "dir.h"
#include "user.h"
#include "vmmeter.h"
#include "trace.h"
#include "kernel.h"

#ifdef PGINPROF

int pmonmin = PMONMIN;
int pres = PRES;
int rmonmin = RMONMIN;
int rres = RRES;

vmsizmon()
{
	register int i;

	i = (u.u_dsize / DRES) < NDMON ? (u.u_dsize / DRES):NDMON;
	dmon[i] += u.u_ru.ru_utime.tv_sec - u.u_outime;

	i = (u.u_ssize / SRES) < NSMON ? (u.u_ssize / SRES):NSMON;
	smon[i] += u.u_ru.ru_utime.tv_sec - u.u_outime;
	u.u_outime = u.u_ru.ru_utime.tv_sec;
}

vmfltmon(hist, atime, amin, res, nmax)
	register unsigned int *hist;
	register int atime, amin, res, nmax;
{
	register int i;

	i = (atime - amin) / res;
	if (i>=0 && i<nmax)
		hist[i+1]++;
	else 
		i<0 ? hist[0]++ : hist[nmax+1]++;
}
#endif

#ifdef TRACE
/*VARARGS*/
trace1(args)
	int args;
{
	register int nargs;
	register int x;
	register int *argp, *tracep;

	nargs = 4;
	x = tracex % TRCSIZ;
	if (x + nargs >= TRCSIZ) {
		tracex += (TRCSIZ - x);
		x = 0;
	}
	argp = &args;
	tracep = &tracebuf[x];
	tracex += nargs;
#ifdef	romp
	*tracep++ = time.tv_sec;
#else	romp
	*tracep++ = (time.tv_sec%1000)*1000 + (time.tv_usec/1000);
#endif	romp
	nargs--;
	do
		*tracep++ = *argp++;
	while (--nargs > 0);
}
#endif
#endif	MACH_VM
