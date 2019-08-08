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
/*	fe.c	CMU	1/18/80	*/

/*
 *  Front End driver
 *
 *  Steven Rubin with help from Ken Greer and John Lowry
 *
 **********************************************************************
 * HISTORY
 * 24-Mar-86  Mike Accetta (mja) at Carnegie-Mellon University
 *	Added support for new TIOCCLOG, TIOCCSET, TIOCCHECK and
 *	TIOCATTACH versions of old FE specific calls; fixed bug in
 *	feioctl() which mis-interpreted negative return values from
 *	line discipline ioctl routines; fixed to do output select
 *	wakeup when queue drains.
 *
 * 06-Aug-85  Mike Accetta (mja) at Carnegie-Mellon University
 *	Upgraded from 4.1BSD.
 *	[V1(1)]
 *
 * 13-Dec-84  Mike Accetta (mja) at Carnegie-Mellon University
 *	Changed FEIOCCDET call to FEIOCCSET call which handles
 *	multiple special flag bits; changed BREAK message processing to
 *	check new FEBRKINH bit which can now be set with this call;
 *	fixed bug in attach processing which was neglecting to preserve
 *	permanent mode bits (V3.07l).
 *
 * 17-May-83  Mike Accetta (mja) at Carnegie-Mellon University
 *	Added FEIOCCDET call to alter action when a line is detached.  If
 *	FEDETHUP is set, a hangup signal is sent to the process group
 *	of a terminal whenever it is detached (V3.06j).
 *
 * 10-May-83  Mike Accetta (mja) at Carnegie-Mellon University
 *	Fixed to intialize data structures on an FEIOCSETS call if not
 *	already done so that setting the status of a never connected FE
 *	will no longer crash the system (V3.06j).
 *
 * 30-Mar-83  Mike Accetta (mja) at Carnegie-Mellon University
 *	Changed to track changes in the Front End line number for a terminal
 *	in its terminal location stored in the tty structure;  added
 *	nfetty definition for cdevsw[] (V3.06h).
 *
 * 07-Jul-82  Mike Accetta (mja) at Carnegie-Mellon University
 *	Added FEIOCLINEN call to return the line number for an open
 *	terminal file (V3.05c).
 *
 * 13-Apr-82  Mike Accetta (mja) at Carnegie-Mellon University
 *	Changed to generate even parity on output characters in normal
 *	mode (V3.04f).
 *
 * 23-Jan-82  Mike Accetta (mja) at Carnegie-Mellon University
 *	Fixed bug by moving FEIOCNFE call from feioctl() to fespioctl();
 *	added Front End reinitialization on inactivity (no SYNC requests); 
 *	added detached terminal state set for all terminals when Front End
 *	is initialized, disconnected, or connected when already connected
 *	or for a terminal when line is disconnected by FE or connected to
 *	when already connected;  added FEIOCCHECK ioctl call to check
 *	for disconnected terminal and FEIOCATTACH ioctl call to attach line
 *	to disconnected terminal (V3.03e).
 *
 * 16-Nov-81  Mike Accetta (mja) at Carnegie-Mellon University
 *	Fixed bug in feioctl() which failed to set open flag from
 *	parameters.  This became necessary since ioctl() neglects to do
 *      this in 4.1 (V3.03a).
 *
 * 21-Oct-81  Mike Accetta (mja) at Carnegie-Mellon University
 *	Added BREAK line message handling to disconnect terminal;  renamed
 *	reset routine fereinit() and added real fereset() routine for
 *	UBA resets (V3.02e).
 *
 * 20-Aug-81  Mike Accetta (mja) at Carnegie-Mellon University
 *	Upgraded for 4.1BSD;  changed processing of intro messages to allow
 *	insertion of system name at connect time rather than compile time;
 *	added new FEIOCCLOG ioctl call to change the login status of a line
 *	(this replaces old kludge of using zero input speed as logged out);
 *	added new FEIOCNFE ioctl call to return the number of Front Ends;
 *	fixed feinit() to initialize the t_dev and t_addr fields of the tty
 *	structure for each FE since they are now used by the relevant DZ
 *	driver routines;  fixed feioctl to grab read/write status a la ioctl()
 *	since it no longer pases it through to the device routine;  removed
 *	references to obsolete SNDCHR bit;  fixed longstanding bug in ASLEEP
 *	wakeups by moving it into the line scan loop rather than executing
 *	it only after a character had been found to output (V3.00).
 *
 * 23-Jul-81  Mike Accetta (mja) at Carnegie-Mellon University
 *	Expanded to three Front Ends and changed to use fettyletter()
 *	macro to map fet11 index to terminal letter (V2.00r).
 *
 * 14-May-81  Mike Accetta (mja) at Carnegie-Mellon University
 *	Fixed bug which failed to clear the line status after disconnecting a
 *	line and consequently generated spurious LME meesages at the Front
 *	End console (V2.00k).
 *
 * 13-May-81  Mike Accetta (mja) at Carnegie-Mellon University
 *	Fixed bug in fespwrite() which munged returned line number beyond
 *	recognition for all but the first FE;  changed reset print
 *	statement to print FE# rather than DZ#;  added new FEIOCDISC
 *	ioctl call which disconnects the line from the FE provided
 *	the invoker has the terminal open for reading (V2.00j).
 *
 * 02-Feb-81  Mike Accetta (mja) at Carnegie-Mellon University
 *	Commented out refuse connections response if ISOPEN bit clear 
 *	in terminal flags word until bug can be tracked down (V2.00b).
 *
 * 12-Nov-80  Mike Accetta (mja) at Carnegie-Mellon University
 *	Changed to use FEIOCSETS and FEIOCGETS ioctl call codes
 *	inplace of stty/gtty on control device (V2.00).
 *
 * 01-Nov-80  Mike Accetta (mja) at Carnegie-Mellon University
 *	Upgraded for Berkeley version 3.34;  extracted NFE onto dz.h
 *	and NLINE onto fe.h;  changed welcome message format for new
 *	getty.vm; modified to use t_iproc field to store input start
 *	routine for FE dz lines; changed to store terminal type in
 *	output speeds and only allow super-user to set input speed
 *	(logged-in status) (V2.00).
 *
 * 18-Aug-80  Mike Accetta (mja) at Carnegie-Mellon University
 *	Added welcome and reconnect messages for new ZOG, PS, CFS
 *	and CAD Vaxen (V1.09).
 *
 * 27-Jun-80  Mike Accetta (mja) at Carnegie-Mellon University
 *	Changed to support multiple Front End connections (V1.08a).
 *
 * 12-Feb-80  Mike Accetta (mja) at Carnegie-Mellon University
 *	Added fe_tty[] array of tty structures and changed the fet11
 *	structure so that ttyb field is only a pointer into the
 *	fe_tty[] array rather than being the actual structure itself.
 *	This was done so that the fe_tty[] array would be available for
 *	inclusion in the cdevsw[] table for use by the multiplexor
 *	(V1.03b).
 *
 * 24-Jan-80  Mike Accetta (mja) at Carnegie-Mellon University
 *	Modified for new FE protocol which transmits line messages
 *	as LINEMSG+msg# followed by line# (V1.01a).
 *
 * 18-Jan-80  Mike Accetta (mja) at Carnegie-Mellon University
 *	Created (V1.00).
 *
 **********************************************************************
 */

#include "cs_compat.h"
#include "cs_ttyloc.h"

#include "fe.h"

#if NFE > 0

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/ioctl.h"
#include "../h/tty.h"
#include "../h/dir.h"
#include "../h/user.h"
#include "../h/proc.h"
#include "../h/buf.h"
#include "../h/conf.h"
#include "../h/file.h"
#include "../h/uio.h"
#include "../h/kernel.h"
#if	CS_COMPAT
#include "../h/mx.h"
#undef	CONNECT
#undef	DISCON
#undef	DEBUG
#endif	CS_COMPAT
#include "../h/fe.h"

#include "../vax/mtpr.h"

#include "../vaxuba/ubavar.h"

#define	DEBUG	1
#define	FETRACE	0


/* control responses for the bits in flags */
char    feresponse[] =
{
    SYNC, DISCON, CACK, QINC, 0,
    CONNECT, CONREF, CLEAR, HALT, RESUME
};

/*
 *  Special control character coding convention for terminal messages:
 *
 *  000 - end of string, resume at oldintro pointer
 *  001 - insert system name
 *  002 - insert tty letter
 *  004 - end of message
 *
 *  177 - delay character
 */

char fedelay[] = "\177\177\177\177\177\177\177\177\004";

char    fewelcome[] =			/* message for un-logged-in ttys */
        "\004";
char    fereconct[] =			/* message for logged-in ttys */
        "Reconnected to \001  ttyf\002\r\n\004";

#define	FEIPL5	0x15		/* DZ interrupt priority level */
#define	FESPEED	12		/* B4800 */
#define	FEDZERR	070000		/* error bits in DZ rbuf */

#define	NFELINES (NFE+NLINE)

struct fet  fet11[NFELINES];
struct tty fe_tty[NFELINES];
int nfetty = NLINE;

extern fe1xstart(), fe2xstart(), fe3xstart();

struct fetab fetab[NFE] =
{
    0, fe1xstart, 0, {0},
    0, fe2xstart, 1, {0},
    0, fe3xstart, 2, {0},
};

/* routine to initialize the Front End line discipline */
felopen(dev, fetp)
    dev_t dev;
    struct tty *fetp;
{
    register struct fetab *fetabp;
    int unit = minor(dev);

    if (!suser() || unit >= NFE)
	return(EPERM);

    fetabp = &fetab[unit];
    fetabp->fetp = fetp;
    feinit(fetabp);
    fexstart(fetabp);
    return(0);
}


felclose(fetp)
    struct tty *fetp;
{
    register struct fetab *fetabp = &fetab[minor(fetp->t_dev)];

    fetabp->fe11.flags &= ~ALLOWC;
    fetabp->fe11.flags |= SNDISC;
    fexstart(fetabp);
}



/* routine to initialize the Front End line */
feinit (fetabp)
register struct fetab *fetabp;
{
    register int    i;
    register struct fet *p;
    register struct tty *tp;
    register int ipl;
    extern int fetimout();

#if	FETRACE
    printf("feinit: %x\n", fetabp);
#endif
    if ((ipl=mfpr(IPL)) < FEIPL5)
	ipl = spl5();
    fetabp->fe11.flags &= FEINIT;
    fetabp->fe11.flags |= SNDSYNC | SNDCON | ALLOWC;
    fetabp->fe11.iptr = 0;
    fetabp->fe11.optr = 0;
    fetabp->fe11.iquo = 0;
    fetabp->fe11.oquo = 0;
    fetabp->fe11.inactive = 0;
    for (p = &fet11[0]; p < &fet11[NFELINES]; p++)
	if (p->fetabp == 0 || p->fetabp == fetabp)
	{
	    p -> linf &= FEMODES;
	    p -> linqo = 0;
	    p -> linqi = 0;
	    if (p->line > FEDETACH)
	    {
		fetabp->fe11.lcbtab[p->line] = FENOLINE;
		p->line = FEDETACH;
		p->ttyb->t_ttyloc.tlc_ttyid = (unsigned short)FEDETACH;
		if (p->linf&FEDETHUP)
		    gsignal(p->ttyb->t_pgrp, SIGHUP);
	    }
	}
    ttyflush(fetabp->fetp, (FREAD|FWRITE));
    splx (ipl);

    if ((fetabp->fe11.flags&FEINIT) == 0)
    {
	fetabp->fe11.flags |= FEINIT;
	for (i = 0; i < MAXLINE; i++)
	    fetabp->fe11.lcbtab[i] = FENOLINE;
	for (p = &fet11[0], tp = &fe_tty[0]; p < &fet11[NFELINES]; p++, tp++)
	{
	    if (p->ttyb == 0)
	    {
		p -> line = FEFREE;
		p -> ttyb = tp;
		p -> fetabp = fetabp;		/* just in case */
	    }
#if	CS_TTYLOC
	    tp->t_ttyloc.tlc_hostid = TLC_FEHOST;
	    tp->t_ttyloc.tlc_ttyid = TLC_UNKTTY;
#endif	CS_TTYLOC
	}
	if (fetabp == &fetab[0])
	    timeout(fetimout, (caddr_t)0, IDLESECS*60);
    }
}

/* routine to open a UNIX line to the Front End */
/*ARGSUSED*/
feopen (dev, flag)
{
    register struct tty *tp;
    register int md;
    register struct fetab *fetabp;
    extern int fenostart();

#if	FETRACE
    printf("feopen: %o %d\n", dev, flag);
#endif
    if ((md=minor(dev)) >= NFELINES)
    {
	return(ENXIO);
    }
    tp = &fe_tty[md];
    if (tp->t_oproc == NULL)
	tp -> t_oproc = fenostart;
    if ((tp -> t_state & TS_ISOPEN) == 0)
    {
	ttychars(tp);
	tp->t_ispeed = NULL;
	tp->t_ospeed = NULL;
	tp -> t_flags |= ODDP | EVENP |XTABS | ECHO;
	tp -> t_erase = CERASE;
	tp -> t_kill = CKILL;
    }
    else if ((tp->t_state & TS_XCLUDE) && u.u_uid != 0)
    {
	return(EBUSY);
    }
    tp -> t_state |= TS_ISOPEN|TS_CARR_ON;
    (*linesw[tp->t_line].l_open)(dev, tp);
    for (fetabp = &fetab[0]; fetabp < &fetab[NFE]; fetabp++)
	if ((fetabp->fe11.flags&FEINIT) == 0) feinit (fetabp);
    return(0);
}

/* routine to close a UNIX line to the Front End */
feclose (dev)
{

    register struct tty *tp;

#if	FETRACE
    printf("feclose: %d\n", dev);
#endif
    tp = fet11[minor(dev)].ttyb;
    if ((fet11[minor(dev)].linf&FETHERE) == 0)
	ttyflush(tp, (FREAD|FWRITE));
    tp->t_state &= ~TS_ISOPEN;
    tp->t_pgrp = 0;
    ttywflush(tp);
    if ((tp->t_state&TS_ISOPEN) == 0)
    {
	(*linesw[tp->t_line].l_close)(tp);
	ttyclose(tp);
    }

}

/* routine to get characters from the Front End to a UNIX line */
feread (dev, uio)
    dev_t dev;
    struct uio *uio;
{

    register struct tty *tp;

    tp = fet11[minor(dev)].ttyb;
    return((*linesw[tp->t_line].l_read)(tp, uio));

}

/* routine to send characters from a UNIX line to the Front End */
fewrite (dev, uio)
    dev_t dev;
    struct uio *uio;
{
    register struct tty *tp;

    tp = fet11[minor(dev)].ttyb;
    return((*linesw[tp->t_line].l_write)(tp, uio));

}

/* routine to read front end line status */
/*ARGSUSED*/
fespread (dev, uio)
    dev_t dev;
    struct uio *uio;
{
    register struct fet *p;
    register int offs;
    struct fespbuf lbuf;
    int error = 0;

    if ((offs=uio->uio_offset / sizeof (struct fespbuf)) >= NFELINES)
	return(0);

    for (p = &fet11[offs]; p < &fet11[NFELINES]; p++)
    {
	if (uio->uio_resid <= 0 || error)
	    break;
	offs = uio->uio_offset % sizeof (struct fespbuf);
	lbuf.splinf = p->linf;
	lbuf.spline = p->line;
	if (lbuf.spline > FEDETACH)
	    lbuf.spline += (p->fetabp - &fetab[0])*LINEOFS;
	error = uiomove((caddr_t)&lbuf+offs, (int)min((u_int)uio->uio_resid, sizeof (struct fespbuf)) - offs, UIO_READ, uio);
    }
    return(error);
}

/*  routine to set line characteristics of line  */
/*ARGSUSED*/
fespwrite (dev, uio)
    dev_t dev;
    struct uio *uio;
{
    register struct fet *p;
    register struct tty *tp;
    register int offs;
    struct fespbuf lbuf;
    int error = 0;

    if ((offs=uio->uio_offset / sizeof (struct fespbuf)) >= NFELINES)
	return(0);

    for (p = &fet11[offs]; p < &fet11[NFELINES]; p++)
    {
	tp = p->ttyb;
	if (uio->uio_resid <= 0 || error)
	    break;
	offs = uio->uio_offset % sizeof (struct fespbuf);
	lbuf.splinf = p->linf;
	lbuf.spline = p->line;
	if (lbuf.spline > FEDETACH)
	    lbuf.spline += (p->fetabp - &fetab[0])*LINEOFS;
	error = uiomove((caddr_t)&lbuf+offs, (int)min((u_int)uio->uio_resid, sizeof (struct fespbuf)) - offs, UIO_WRITE, uio);
	if (lbuf.spline > FEDETACH)
	    lbuf.spline -= (p->fetabp - &fetab[0])*LINEOFS;
	if (lbuf.spline < 1 || lbuf.spline >= MAXLINE)
	    error = EIO;
	else
	{
	    lbuf.splinf &= SPWMASK;
	    p->linf &= ~SPWMASK;
	    p->linf |= lbuf.splinf;
	    p->line = lbuf.spline;
#if	CS_TTYLOC
	    tp->t_ttyloc.tlc_ttyid = lbuf.spline+(p->fetabp - &fetab[0])*LINEOFS;
#endif	CS_TTYLOC
	    p->fetabp->fe11.lcbtab[p->line] = (p-fet11);
	    fexstart(p->fetabp);
	}
    }
    return(error);
}

/*  routine to read or modify Front End status  */
/*ARGSUSED*/
fespioctl (dev, cmd, addr, flag)
caddr_t addr;
dev_t dev;
{
    register struct fetab *fetabp;
    register int allowc;

    switch (cmd)
    {
	case FEIOCNFE:
	{
	    *((int *)addr) = NFE;
	    break;
 	}

	case FEIOCSETS:
	{
	    struct feiocbuf *sgp = (struct feiocbuf *)addr;

	    if ((flag&FWRITE) == 0)
		return(EACCES);

	    for (fetabp = &fetab[0]; fetabp < &fetab[NFE]; fetabp++, sgp++)
	    {
		if ((fetabp->fe11.flags&FEINIT) == 0)
		    feinit (fetabp);
		fetabp->fe11.force1 = sgp->spfrc1;
		fetabp->fe11.force2 = sgp->spfrc2;
		allowc = (fetabp->fe11.flags&ALLOWC);
		sgp->spflags &= IOCMASK;
		fetabp->fe11.flags &= ~IOCMASK;
		fetabp->fe11.flags |= sgp->spflags;
		if (allowc == 0 && (fetabp->fe11.flags&ALLOWC))
		    feinit(fetabp);
		fexstart(fetabp);
	     }
	    break;
	}

	case FEIOCGETS:
	{
	    struct feiocbuf *sgp = (struct feiocbuf *)addr;

	    for (fetabp = &fetab[0]; fetabp < &fetab[NFE]; fetabp++, sgp++)
	    {
		sgp->spfrc1 = fetabp->fe11.force1;
		sgp->spfrc2 = fetabp->fe11.force2;
		sgp->spflags = fetabp->fe11.flags;
		sgp->spnline = NLINE;
	    }
	    break;
	}

	default:
	    return(ENOTTY);
    }
    return(0);
}

/*
 * routine to handle disconnects of inactive logged-out ttys and
 * Front End crash detection.
 *
 * Called every five seconds by the clock timeout mechanism.
 *
 */
fetimout ()
{
    register struct fetab *fetabp;
    register struct fet *p;
    register    line;

#if	FETRACE
    printf("fetimout:\n");
#endif
    for (line = 0; line < NFELINES; line++)
    {
	p = &fet11[line];
	if ((p -> linf & LOGGED) == 0)
	{
	    if (p->line == FEDETACH)
	    {
		p->line = FEFREE;
#if	CS_TTYLOC
		p->ttyb->t_ttyloc.tlc_ttyid = TLC_UNKTTY;
#endif	CS_TTYLOC
	    }
	    else if (p -> line > FEDETACH)
		if (++(p -> idle) > MAXIDLE)
		    p -> linf |= SNDISC;
	}
    }

    /*  Check each Front End for inactivity  */
    for (fetabp = &fetab[0]; fetabp < &fetab[NFE]; fetabp++)
    {
	if ((fetabp->fe11.flags & FETHERE) == 0)
	    continue;
	if (fetabp->fe11.flags & FEACTIVE)
	{
	    fetabp->fe11.flags &= ~FEACTIVE;
	    fetabp->fe11.inactive = 0;
	}
	else
	    if (++(fetabp->fe11.inactive) >= INACTIVE)
	    {
		fereinit(": inactive", fetabp);
	    }
    }
    timeout(fetimout, (caddr_t)0, IDLESECS*60);

}

/* routine to send characters to the Front End if possible */
fenostart()
{
}



felstart(fetp)
struct tty *fetp;
{
    fexstart(&fetab[minor(fetp->t_dev)]);
}


fe1xstart()
{

    fexstart(&fetab[0]);

}


fe2xstart()
{

    fexstart(&fetab[1]);

}


fe3xstart()
{

    fexstart(&fetab[2]);

}


fexstart (fetabp)
register struct fetab *fetabp;
{
    register struct tty *tp;
    register struct fet *p;
    register c,chr;
    register ttycount;
    register ipl;
    extern ttrstrt();
    extern char partab[];

#if	FETRACE
    printf("fexstart: %x\n", fetabp);
#endif
    if (fetabp->fetp == 0)
	return;
    if ((ipl=mfpr(IPL)) < FEIPL5)
	ipl = spl5 ();
    if ((fetabp->fe11.flags&FEXBUSY) || fetabp->fetp->t_outq.c_cc >=TTHIWAT(fetabp->fetp))
    {
	fetabp->fe11.flags |= FEXNEST;
	splx (ipl);
	(*(fetabp->fetp->t_oproc))(fetabp->fetp);
	return;
    }
    fetabp->fe11.flags |= FEXBUSY;

    /* handle the character transmission */
    for (;;)
    {

	fetabp->fe11.flags &= ~FEXNEST;
	splx (ipl);

	/* check for any system messages at all */
	if (fetabp->fe11.flags & SNDSYS)
	    for (c = 0; c < 10; c++)
		if (fetabp->fe11.flags & (1 << c))
		{
		    if (c == 1)
		    {
			/*  Disconnect, detach all lines  */
			for (p = &fet11[0]; p < &fet11[NLINE]; p++)
			    if (p->fetabp == fetabp)
			    {
				p -> linf &= FEMODES;
				if (p->line > FEDETACH)
				{
				    fetabp->fe11.lcbtab[p->line] = FENOLINE;
				    p->line = FEDETACH;
				    p->ttyb->t_ttyloc.tlc_ttyid = (unsigned short)FEDETACH;
				    if (p->linf&FEDETHUP)
					gsignal(p->ttyb->t_pgrp, SIGHUP);
				}
			    }
			fetabp->fe11.flags = FEINIT;
		    }
		    fetabp->fe11.flags &= ~(1 << c);
		    if (c == 4)
			fetabp->fe11.ochar = fetabp->fe11.force2;
		    else
			fetabp->fe11.ochar = feresponse[c];
		    if (c == 0)
			chr = SYNC;
		    else
			if (c == 4)
			    chr = fetabp->fe11.force1;
			else
			    chr = SYSMES;
		    goto send;
		}

	/* no system messages.  any quota left? */
	if (fetabp->fe11.oquo <= 0 || (fetabp->fe11.flags & FETHERE) == 0)
	    goto out;

	/* check for line messages */
	for (p = &fet11[0]; p < &fet11[NFELINES]; p++)
	{
	    if (p -> linf & SNDLINE && p->fetabp == fetabp)
		for (c = 0; c < 10; c++)
		    if (p -> linf & (1 << c))
		    {
			p -> linf &= ~(1 << c);
			chr = feresponse[c] + LINEMSG;
			fetabp->fe11.ochar = p -> line;
			fetabp->fe11.oquo--;
			if (c == 1 || c == 6)
			{
			    p -> linf &= FEMODES;
			    if ((p->linf&LOGGED) == 0)
			    {
				fetabp->fe11.lcbtab[p -> line] = FENOLINE;
				p -> line = FEFREE;
#if	CS_TTYLOC
				p->ttyb->t_ttyloc.tlc_ttyid = TLC_UNKTTY;
#endif	CS_TTYLOC
			    }
			}
			goto send;
		    }
	}

	/* ok, no line messages either, try a text character */
	for (ttycount = 0; ttycount < NLINE; ttycount++)
	{
	    if (++fetabp->fe11.linpos >= NLINE)
		fetabp->fe11.linpos = 0;
	    p = &fet11[fetabp->fe11.linpos];
	    tp = p -> ttyb;
	    if (p->fetabp != fetabp || p -> line <= FEDETACH ||
		p -> linqo <= 0 || (p -> linf & FETHERE) == 0 ||
		    (tp -> t_state & (TS_TIMEOUT|TS_TTSTOP)))
		continue;
	    /* wake up writer if he was sleeping */
	    if (tp -> t_outq.c_cc <= TTLOWAT(tp))
	    {
		if ((tp -> t_state & TS_ASLEEP))
		{
		    tp -> t_state &= ~TS_ASLEEP;
#if	CS_COMPAT
		    if (tp->t_chan)
			mcstart(tp->t_chan, (caddr_t)&tp->t_outq);
		    else
#endif	CS_COMPAT
		    wakeup ((caddr_t)&tp -> t_outq);
		}
		if (tp->t_wsel)
		{
			selwakeup(tp->t_wsel, tp->t_state&TS_WCOLL);
			tp->t_wsel = 0;
			tp->t_state &= ~TS_WCOLL;
		}
	    }
immed:
	    if (p->intro)
	    {
		c = *(p -> intro)++;
		switch (c)
		{
		    case 0:	/* end of insert */
			p->intro = p->oldintro;
			goto immed;
		    case 1:	/* insert system name */
			p->oldintro = p->intro;
			p->intro = hostname;
			goto immed;
		    case 2:	/* insert tty letter */
			c = fettyletter(fetabp->fe11.linpos);
			break;
		    case 4:	/* end of message */
			p->intro = 0;
			goto immed;
		}
		/*  Convert a host name to upper case.  */
		if (p->intro > hostname && p->intro <= &hostname[32])
		    if (c >= 'a' && c <= 'z')
			c += 'A' - 'a';
	    }
	    else
		c = getc (&tp -> t_outq);
	    if (c != -1)
	    {
		if ((tp->t_flags&(RAW|LITOUT)) == 0)
		    if (c >= 0200)
		    {
			if ((c = (sizeof fedelay-1-((c&0177)+7)/2)) < 0)
			    c = 0;
			p->intro = &fedelay[c];
			goto immed;
		    }
		    else
			c |= (partab[c]&0200);
		fetabp->fe11.oquo--;
		p -> linqo--;
		p -> idle = 0;	/* Clear inactivity */
		chr = p -> line;
		fetabp->fe11.ochar = c;

		goto send;
	    }
	}

out:
	if ((ipl=mfpr(IPL)) < FEIPL5)
	    ipl = spl5();
	if ((fetabp->fe11.flags&FEXNEST) == 0)
	{
	    fetabp->fe11.flags &= ~FEXBUSY;
	    splx (ipl);
	    (*(fetabp->fetp->t_oproc))(fetabp->fetp);
	    return;
	}

    }

send:
#if	DEBUG
    if (fetabp->fetp->t_flags&020000) printf("X%d:%o,%o\n", fetabp->fedzl+1,chr, fetabp->fe11.ochar);
#endif
    if (putc(chr, &(fetabp->fetp->t_outq)) || putc(fetabp->fe11.ochar, &(fetabp->fetp->t_outq)))
	fereinit("QOV", fetabp);
    if ((ipl=mfpr(IPL)) < FEIPL5)
	ipl = spl5 ();
    fetabp->fe11.flags &= ~FEXBUSY;
    splx (ipl);
    (*(fetabp->fetp->t_oproc))(fetabp->fetp);

}

/* Front End line discipline input interrupt routine */
felrint (rbuf, fetp)
short rbuf;
struct tty *fetp;
{
    unsigned char    linum, chr, i;
    int t_state;
    register int    lcb;
    register struct fet *p;
    register int ipl;
    register struct tty *tp;
    register struct fetab *fetabp;

#if	FETRACE
    printf("felrint: %x\n", rbuf);
#endif
    /* dump the character into the buffer */
    fetabp = &fetab[minor(fetp->t_dev)];
    fetabp->fe11.chrbuf[fetabp->fe11.iptr] = rbuf;

    /* check for error bits */
    if (rbuf & FEDZERR)
    {
	printf("%o-", (unsigned short)rbuf); fereinit("CSR", fetabp);
	return;
    }

#if	DEBUG
    if (fetabp->fetp->t_flags&010000) printf("R%d:%o\n", fetabp->fedzl+1, (char)rbuf);
#endif

    /* 
     * if the character is a SYNC, just ignore it, and set the
     * flag indicating that the SYNC was received
     */
    if ((fetabp->fe11.flags & FETHERE) == 0 &&
	    (fetabp->fe11.chrbuf[fetabp->fe11.iptr] & 0377) == SYNC)
    {
	fetabp->fe11.flags |= GOTSYNC;
	fexstart (fetabp);
	return;
    }

    /* ignore data until SYNC is sent (this aligns bytes) */
    if ((fetabp->fe11.flags & (GOTSYNC | FETHERE)) == 0)
    {
/*
	fexstart (fetabp);
 */
	return;
    }

    fetabp->fe11.iptr = (fetabp->fe11.iptr + 1) & FEIMASK;

    /* get the scanner */
    ipl = spl5 ();
    if (fetabp->fe11.flags & FERBUSY)
    {
	splx (ipl);
	return;
    }
    fetabp->fe11.flags |= FERBUSY;

    /* 
     * the front end scanner:
     * remove the first character from the buffer, which gives the
     * line to which the transmission is intended, and the second,
     * which gives the character or message.
     */
    for (;;)
    {

	/* see if there are at least two characters in the buffer */
	if (((fetabp->fe11.iptr - fetabp->fe11.optr) & FEIMASK) < 2)
	{
	    fetabp->fe11.flags &= ~FERBUSY;
	    splx (ipl);
	    fexstart (fetabp);
	    return;
	}
	splx (ipl);

	linum = fetabp->fe11.chrbuf[fetabp->fe11.optr];
	chr = fetabp->fe11.chrbuf[fetabp->fe11.optr + 1];
	fetabp->fe11.optr = (fetabp->fe11.optr + 2) & FEIMASK;

	/* first check for line 0, which is for system messages */
	/* (this used to be a switch statement) */
	if (linum == 0)
	{

	    /* increment system quota */
	    if (chr == QINC)
	    {
		fetabp->fe11.oquo += FEQUOTA;
	    }

	    /* connect: send connect acknowledgement and system quota */
	    else
		if (chr == CONNECT)
		{
		    /*
		     *  Detach all lines from this front end if we
		     *  get a CONNECT message while already connected.
		     *  This means that the front end has crashed and
		     *  rebooted.
		     */
		    if (fetabp->fe11.flags & FETHERE)
		    {
			for (p = &fet11[0]; p < &fet11[NLINE]; p++)
			{
			    if (p->fetabp != fetabp)
				continue;
			    p->linf &= FEMODES;
			    if (p->line > FEDETACH)
			    {
				fetabp->fe11.lcbtab[p->line] = FENOLINE;
				p->line = FEDETACH;
#if	CS_TTYLOC
				p->ttyb->t_ttyloc.tlc_ttyid = (unsigned short)FEDETACH;
#endif	CS_TTYLOC
				if (p->linf&FEDETHUP)
				    gsignal(p->ttyb->t_pgrp, SIGHUP);
			    }
			}
		    }
		    if (fetabp->fe11.flags & ALLOWC)
		    {
			fetabp->fe11.oquo = 0;
			fetabp->fe11.iquo = FEQUOTA;
			fetabp->fe11.inactive = 0;
			fetabp->fe11.flags |= SNDCACK | SNDINC | FETHERE;
		    }
		    else
			fetabp->fe11.flags |= SNDCONR;
		}

	    /* request for SYNC; set up to send the SYNC */
		else
		    if (chr == SYNCREQ)
		    {
			fetabp->fe11.flags |= (FEACTIVE|SNDSYNC);
		    }
	}
	else
	{

	    /* not a system message.  Decrement the system quota count */
	    fetabp->fe11.iquo--;

	    /* 
	     * See if a line message (linum >= MAXLINE).
	     */
	    if (linum >= MAXLINE)
	    {
		/* this swap should be done by changing identifiers */
		lcb = chr;
		chr = linum - LINEMSG;
		linum = lcb;
		lcb = fetabp->fe11.lcbtab[linum];
		if (lcb == FENOLINE && chr == CONNECT)
		{
		findfree:
		    for (i = 0; i < NLINE; i++)
			if (fet11[i].line == FEFREE)
			{
			    lcb = i;
			    break;
			}

		    /* allow (only temporarily) if out of lines */
		    if (lcb == FENOLINE)
			lcb = NLINE + (fetabp - &fetab[0]);;

		    /* setup the UNIX-to-FrontEnd tables */
		    fet11[lcb].fetabp = fetabp;
		    fet11[lcb].line = linum;
		    fetabp->fe11.lcbtab[linum] = lcb;
		}

		/* procede only if the line number is valid */
		if (lcb != FENOLINE)
		{
		    p = &fet11[lcb];
		    tp = p -> ttyb;

		    /* handle BREAK message (disconnect line) */
		    if (chr == BREAK && (p->linf&FEBRKINH) == 0)
		    {
			p->linf |= SNDISC;
		    } else

		    /* handle line connection */
		    if (chr == CONNECT)
		    {
			/*
			 *  If we get a connection message on a line we
			 *  think is already connected, first detach it
			 *  and assign a new slot since something has
			 *  gotten fouled up.
			 */
			if (p->linf&FETHERE)
			{
			    p->linf &= FEMODES;
			    fetabp->fe11.lcbtab[p->line] = FENOLINE;
			    p->line = FEDETACH;
#if	CS_TTYLOC
			    tp->t_ttyloc.tlc_ttyid = (unsigned short)FEDETACH;
#endif	CS_TTYLOC
			    if (p->linf&FEDETHUP)
				gsignal(tp->t_pgrp, SIGHUP);
			    goto findfree;
			}
			if (lcb >= NLINE || (fetabp->fe11.flags&REFUSEC)/* || *(tp -> t_state & TS_ISOPEN) == 0*/)
			    p -> linf |= SNDCONR;
			else
			{
			    p -> linqi = FEQUOTA;
			    p -> linqo = 0;
			    p -> linf |= SNDCACK | SNDINC | FETHERE;
#if	CS_TTYLOC
			    tp -> t_ttyloc.tlc_ttyid = p->line+(fetabp - &fetab[0])*LINEOFS;
#endif	CS_TTYLOC
			    tp -> t_oproc = fetabp->xstart;
#if	UNDEF
			    tp -> t_state &= ~CPTOGGLE;
#endif
			    if (p->linf&LOGGED)
			    {
				p -> intro = fereconct;
			    }
			    else
			    {
				p -> intro = fewelcome;
				ttyflush(tp, (FREAD|FWRITE));
			    }
			}
		    } else

		    /*  handle line disconnect  */
		    if (chr == DISCON)
		    {
			/*  Detach line  */
			p->linf &= FEMODES;
			fetabp->fe11.lcbtab[p->line] = FENOLINE;
			p->line = FEDETACH;
#if	CS_TTYLOC
			tp->t_ttyloc.tlc_ttyid = (unsigned short)FEDETACH;
#endif	CS_TTYLOC
			if (p->linf&FEDETHUP)
			    gsignal(tp->t_pgrp, SIGHUP);
		    } else

		    /* handle quota increment */
		    if (chr == QINC)
		    {
			p -> linqo += FEQUOTA;
		    } else

		    /* handle terminal types */
		    if (chr >= FETTYPE)
		    {
			if ((p->linf&LOGGED) == 0)
			{
			    gsignal(tp->t_pgrp, SIGHUP);
			    p -> ttype = (chr - FETTYPE);
			}
		    }
		}
	    }
	    else
		if ((lcb = fetabp->fe11.lcbtab[linum]) != FENOLINE)
		{

		    /* 
		     * OK, just a normal character for the line.  Just
		     * pass it to the system.
		     */
		    p = &fet11[lcb];
		    tp = p -> ttyb;
		    p -> idle = 0;    /* Clear inactivity */
		    t_state = tp -> t_state & TS_TTSTOP;

		    if ((tp -> t_flags & RAW) == 0)
		    {
			linum = chr & 0177;
			if (linum == 037)
			{
			    p -> linf |= SNDISC;
			    goto noinp;
			}
			if (linum == CINTR || linum == CQUIT)
/*
				(linum == CNOOUT && (tp -> t_flags & CNTRLO) == 0))
 */
			    p -> linf |= SNDCLR;
		    }
		    if ((tp -> t_state & TS_ISOPEN) != 0)
			(*linesw[tp->t_line].l_rint)(chr, tp);
	    noinp: 

		    /* Process output suspension/resumption */
		    if ((t_state ^ tp -> t_state) & TS_TTSTOP)
			if (tp -> t_state & TS_TTSTOP)
			    p -> linf |= SNDHALT;
			else
			    p -> linf |= SNDRSM;

		    /* decrement quota and see if it is near zero */
		    p -> linqi--;
		    ipl = spl5 ();
		    if (p -> linqi <= FEQLOW && (p -> linf & SNDINC) == 0)
		    {
			p -> linf |= SNDINC;
			p -> linqi += FEQUOTA;
		    }
		    splx (ipl);
		}
	}

	ipl = spl5 ();
	/* see if we can send a system quota-increment character */
	if (((fetabp->fe11.flags & SNDINC) == 0) && (fetabp->fe11.flags & FETHERE) &&
		(((fetabp->fe11.optr - fetabp->fe11.iptr - 1) & FEIMASK) >= (FEQUOTA + fetabp->fe11.iquo)))
	{
	    fetabp->fe11.iquo += FEQUOTA;
	    fetabp->fe11.flags |= SNDINC;
	}
    }

}

fereinit(str, fetabp)
char *str;
struct fetab *fetabp;
{

    printf("FE%d%s\n", fetabp->fedzl + 1, str);
    feinit(fetabp);

}

/*
 * Reset driver after UBA init
 */

/* ARGSUSED */
fereset(uban)
int uban;
{
    register int nfe;
    register struct fetab *fetabp;

    for (nfe = 0; nfe < NFE; nfe++)
    {
	fetabp = &fetab[nfe];
	printf(" fe%d", nfe+1);
	feinit(fetabp);
	fexstart(fetabp);
    }

}
		
/* routine to set the Front End line terminal characteristics */
feioctl (dev, cmd, addr, flag)
caddr_t addr;
dev_t dev;
{
    register struct fet *p;
    register struct tty *tp;
    int error = 0;

    p = &fet11[minor(dev)];
    tp = p->ttyb;
    switch(cmd)
    {
	case FEIOCLINEN:
	{
	    *((int *)addr) = p->line;
	    if (*((int *)addr) > FEDETACH)
		*((int *)addr) += (p->fetabp - &fetab[0])*LINEOFS;
	    break;
	}

	case TIOCCLOG:
#if	CS_COMPAT
	case FEIOCCLOG:
#endif	CS_COMPAT
	{
	    int l = *((int *)addr);

	    if (!suser())
		return(u.u_error);

	    /* implicit reset on login state change */
	    p->linf &= ~(FEDETHUP|FEBRKINH);
	    if (l)
		p->linf |= LOGGED;
	    else
		p->linf &= ~LOGGED;
	    break;
	}

	case TIOCCSET:
#if	CS_COMPAT
	case FEIOCCSET:
#endif	CS_COMPAT
	{
	    int l = *((int *)addr);

	    p->linf &= ~(FECSETMASK);
	    p->linf |= ((l<<FECSETPOS)&FECSETMASK);
	    break;
	}
	    
	case FEIOCGETT:
	    *((int *)addr) = p->ttype;
	    break;

	case FEIOCDISC:
	    if (flag&FREAD)
	    {
		p->linf |= SNDISC;
		fexstart(p->fetabp);
	    }
	    else
		return(EPERM);
	    break;

	case TIOCCHECK:
	case TIOCATTACH:
#if	CS_COMPAT
	case FEIOCCHECK:
	case FEIOCATTACH:
#endif	CS_COMPAT
	{
	    register int i;
	    register struct fet *pp;

#if	CS_COMPAT
	    if (cmd != TIOCCHECK && cmd != TIOCATTACH)
	    {
		i = felettertty(addr[1]);
		if (addr[0] != 'f' || addr[2] != 0 || i < 0 || i >= NLINE)
		    return(EINVAL);
	    }
	    else
#endif	CS_COMPAT
	    {
		i = minor(*((int *)addr));
		if (major(*((int *)addr)) != major(dev) || i < 0 || i >= NLINE)
		    return(EINVAL);
	    }
	    pp = &fet11[i];
 	    if (pp->line == FEDETACH)
	    {
		if (cmd == FEIOCATTACH)
		{
		    if (!suser())
			return(u.u_error);
		    pp->fetabp = p->fetabp;
		    pp->ttype = p->ttype;
		    pp->linqo = p->linqo;
		    pp->linqi = p->linqi;
		    pp->line = p->line;
#if	CS_TTYLOC
		    pp->ttyb->t_ttyloc.tlc_ttyid = p->ttyb->t_ttyloc.tlc_ttyid;
#endif	CS_TTYLOC
		    pp->fetabp->fe11.lcbtab[pp->line] = i;
		    pp->ttyb->t_oproc = pp->fetabp->xstart;
		    pp->linf = (p->linf&~FEMODES)|(pp->linf&FEMODES);
		    p->linf &= FEMODES;
		    p->line = FEDETACH;
#if	CS_TTYLOC
		    p->ttyb->t_ttyloc.tlc_ttyid = (unsigned short)FEDETACH;
#endif	CS_TTYLOC
		    if (p->linf&FEDETHUP)
			gsignal(p->ttyb->t_pgrp, SIGHUP);
		}
		u.u_r.r_val1 = 1;
	    }
	    break;
	}

	default:
	    error = (*linesw[tp->t_line].l_ioctl)(tp, cmd, addr, flag);
	    if (error < 0)
		error = ttioctl(tp, cmd, addr, flag);
	    if (error < 0)
	        error = ENOTTY;
    }
    return(error);
}
#endif
