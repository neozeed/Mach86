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
 *	@(#)tty_bk.c	6.3 (Berkeley) 6/8/85
 */

#if CMU
/***********************************************************************
 * HISTORY
 * 16-Feb-86  Bill Bolosky (bolosky) at Carnegie-Mellon University
 *	Changed spl5 calls to spl2 calls if ROMP is defined.
 *
 */
#endif CMU

#include "bk.h"

#if NBK > 0
#include "param.h"
#include "systm.h"
#include "dir.h"
#include "user.h"
#include "ioctl.h"
#include "tty.h"
#include "proc.h"
#include "inode.h"
#include "file.h"
#include "conf.h"
#include "buf.h"
#include "uio.h"

/*
 * Line discipline for Berkeley network.
 *
 * This supplies single lines to a user level program
 * with a minimum of fuss.  Lines are newline terminated.
 *
 * This discipline requires that tty device drivers call
 * the line specific l_ioctl routine from their ioctl routines,
 * assigning the result to cmd so that we can refuse most tty specific
 * ioctls which are unsafe because we have ambushed the
 * teletype input queues, overlaying them with other information.
 */

/*
 * Open as networked discipline.  Called when discipline changed
 * with ioctl, this assigns a buffer to the line for input, and
 * changing the interpretation of the information in the tty structure.
 */
/*ARGSUSED*/
bkopen(dev, tp)
	dev_t dev;
	register struct tty *tp;
{
	register struct buf *bp;

	if (tp->t_line == NETLDISC)
		return (EBUSY);	/* sometimes the network opens /dev/tty */
	bp = geteblk(1024);
	ttyflush(tp, FREAD|FWRITE);
	tp->t_bufp = bp;
	tp->t_cp = (char *)bp->b_un.b_addr;
	tp->t_inbuf = 0;
	tp->t_rec = 0;
	return (0);
}

/*
 * Break down... called when discipline changed or from device
 * close routine.
 */
bkclose(tp)
	register struct tty *tp;
{
	register int s;

#ifdef	romp
	s = spl2();
#else	romp
	s = spl5();
#endif	romp
	wakeup((caddr_t)&tp->t_rawq);
	if (tp->t_bufp) {
		brelse(tp->t_bufp);
		tp->t_bufp = 0;
	} else
		printf("bkclose: no buf\n");
	tp->t_cp = 0;
	tp->t_inbuf = 0;
	tp->t_rec = 0;
	tp->t_line = 0;		/* paranoid: avoid races */
	splx(s);
}

/*
 * Read from a network line.
 * Characters have been buffered in a system buffer and are
 * now dumped back to the user in one fell swoop, and with a
 * minimum of fuss.  Note that no input is accepted when a record
 * is waiting.  Our clearing tp->t_rec here allows further input
 * to accumulate.
 */
bkread(tp, uio)
	register struct tty *tp;
	struct uio *uio;
{
	register int s;
	int error;

	if ((tp->t_state&TS_CARR_ON)==0)
		return (-1);
#ifdef	romp
	s = spl2();
#else	romp
	s = spl5();
#endif	romp
	while (tp->t_rec == 0 && tp->t_line == NETLDISC)
		sleep((caddr_t)&tp->t_rawq, TTIPRI);
	splx(s);
	if (tp->t_line != NETLDISC)
		return (-1);
	error = uiomove(tp->t_bufp->b_un.b_addr, tp->t_inbuf, UIO_READ, uio);
	tp->t_cp = (char *)tp->t_bufp->b_un.b_addr;
	tp->t_inbuf = 0;
	tp->t_rec = 0;
	return (error);
}

/*
 * Low level character input routine.
 * Stuff the character in the buffer, and wake up the top
 * half after setting t_rec if this completes the record
 * or if the buffer is (ick!) full.
 *
 * Thisis where the formatting should get done to allow
 * 8 character data paths through escapes.
 *
 * This rutine should be expanded in-line in the receiver
 * interrupt routine of the dh-11 to make it run as fast as possible.
 */
bkinput(c, tp)
register c;
register struct tty *tp;
{

	if (tp->t_rec)
		return;
	*tp->t_cp++ = c;
	if (++tp->t_inbuf == 1024 || c == '\n') {
		tp->t_rec = 1;
		wakeup((caddr_t)&tp->t_rawq);
	}
}

/*
 * This routine is called whenever a ioctl is about to be performed
 * and gets a chance to reject the ioctl.  We reject all teletype
 * oriented ioctl's except those which set the discipline, and
 * those which get parameters (gtty and get special characters).
 */
/*ARGSUSED*/
bkioctl(tp, cmd, data, flag)
	struct tty *tp;
	caddr_t data;
{

	if ((cmd>>8) != 't')
		return (-1);
	switch (cmd) {

	case TIOCSETD:
	case TIOCGETD:
	case TIOCGETP:
	case TIOCGETC:
		return (-1);
	}
	return (ENOTTY);
}
#endif
