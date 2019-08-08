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
 * 5799-CGZ (C) COPYRIGHT IBM CORPORATION  1986
 * LICENSED MATERIALS - PROPERTY OF IBM
 * REFER TO COPYRIGHT INSTRUCTIONS FORM NUMBER G120-2083
 */
/* $Header: tty_mouse.c,v 5.0 86/01/31 18:14:26 ibmacis ibm42a $ */
/* $Source: /ibm/acis/usr/sys_ca/caio/RCS/tty_mouse.c,v $ */

/* tty_mouse.c -- mouse discipline routines */

#include "ms.h"
#if NMS > 0

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/dir.h"
#include "../h/user.h"
#include "../h/ioctl.h"
#include "../h/tty.h"
#include "../h/proc.h"
#include "../h/inode.h"
#include "../h/file.h"
#include "../h/conf.h"
#include "../h/buf.h"
#include "../h/uio.h"
#include "../caio/mouseio.h"
#include "../caio/mousereg.h"

#define	 MS_REPORT_SZ   sizeof(mousedata)

struct mb {
	int	   used;
	int 	   single_but;
	int	   inbuf;		/* count of bytes in buffer */
	char	   *bufp; 		/* buffer pointer (to ms_report) */
	mousedata  ms_report; 
} mb[NMS];

/*
 * Open as mouse discipline.  Called when discipline changed
 * with ioctl, and changes the interpretation of the information
 * in the tty structure.
 */
/*ARGSUSED*/
msdopen(dev, tp)
	dev_t dev;
	register struct tty *tp;
{
	register struct mb *mbp;

	if (tp->t_line == MSLINEDISC)
		return (ENODEV);
	ttywflush(tp);
	for (mbp = mb; mbp < &mb[NMS]; mbp++)
		if (!mbp->used)
			break;
	if (mbp >= &mb[NMS])
		return (EBUSY);
	mbp->used++;
	mbp->single_but = 0;
	mbp->inbuf = 0;
	mbp->bufp = (char *) &mbp->ms_report;
	mbp->ms_report.m_magic = 0;
	mbp->ms_report.m_flags = 0;
	mbp->ms_report.m_x  = 0;
	mbp->ms_report.m_y  = 0;
	tp->t_rawq.c_cc = 0;
	tp->t_canq.c_cc = 0;
	tp->T_LINEP = (caddr_t) mbp;
	return (0);
}

/*
 * Break down... called when discipline changed or from device
 * close routine.
 */
msdclose(tp)
	register struct tty *tp;
{
	register int s = MS_SPL;

	((struct mb *) tp->T_LINEP)->used = 0;
	tp->t_rawq.c_cc = 0;		/* clear queues -- paranoid */
	tp->t_line = 0;			/* paranoid: avoid races */
	splx(s);
}

/*
 * Read from a mouse.
 * Characters have been buffered in the raw queue. 
 */
msdread(tp, uio)
	register struct tty *tp;
	struct uio *uio;
{
	int	i, s, error = 0;
	char	c;

	s = MS_SPL;
	while (tp->t_rawq.c_cc <= 0) {
		if (tp->t_state & TS_NBIO) {
			splx(s);
			return (EWOULDBLOCK);
		}
		sleep((caddr_t)&tp->t_rawq, TTIPRI);
	}
	splx(s);

	/* Input present. */
	while ((c = getc(&tp->t_rawq)) >= 0) {
		error = ureadc(c, uio);		
		if (error || (uio->uio_resid == 0))
			break;
	}
	return (error);
}
	

/*
 * Low level character input routine.
 * Stuff the character in the buffer.
 *
 * This routine could be expanded in-line in the receiver
 * interrupt routine of the mouse adapter to make it run as fast as possible.
 * Logic:
 * 1. collect up mouse report characters with mbp->inbuf being the count
 *	of number of characters collected so far and mbp->bufp being the
 *	position to store the next character.
 * 2. when we have an entire report call msddecode.
 * 3. if msddecode indicates a we have a complete report we transfer from
 *	our buffer to the tty raw q and wakeup the tty.
 */
 
msdinput(c, tp)
	register int c;
	register struct tty *tp;
{
	register struct mb *mbp = (struct mb *) tp->T_LINEP;
	int	 s, i;

	if((c == MS_DATA_SYNC) || (mbp->inbuf > 0)) {
		*mbp->bufp++ = c;
		if (++mbp->inbuf == MS_REPORT_SZ) {
			if (msddecode(mbp->inbuf, mbp, tp)) {
				msreport(mbp, tp);
			}
			mbp->bufp  = (char *) &mbp->ms_report;
			mbp->inbuf = 0;		/* prepare for next report */
		}
	}
}

/*
 * copy mouse data into raw q and wakeup the tty
 */
msreport(mbp, tp)
	register struct tty *tp;
	register struct mb *mbp;
{
	register int i;
	register char *p;

	mbp->bufp = p = (char *) &mbp->ms_report;
	for (i = 0; i < MS_REPORT_SZ; i++)  
		putc(*p++, &tp->t_rawq);
	ttwakeup(tp);
}

#define RIGHT_BUT	0x80
#define MIDDLE_BUT	0x40
#define LEFT_BUT	0x20
#define NO_BUT		0x00
#define BOTH_BUT	0xA0
#define BUT_MASK 	0xE0
#define BUT_DEFS	"\20\06LEFT\07MIDDLE\08RIGHT"

#define SETTIMEOUT(a,b,c) {\
				ptp = tp;\
				timeout(a,b,c);\
			  }

struct tty *ptp;
char msd_debug = 0;

/* 
 * msddecode looks are the state of the buttons and handles the 
 * three button simulation using two buttons.
 * states (single_but):
 * 0	no buttons pressed
 * 1	single button pressed (left or right)
 *	
 * logic:
 * 	if both bottons are pressed then pretend that the middle
 *	button was pressed.
 * states:
 * 1.	not in middle of a single button test, and both buttons were
 *	down and exactly one was let up --> we ignore this transtion
 *	until both are let up.
 * 2.	not in middle of a single button test, and both buttons were
 *	up, and now one is pressed --> arrange to check again in
 *	5 ticks and now in single button test mode.
 * 3.	in the middle of the single button test, and the buttons
 *	are unchanged --> cancel the timeout. if we have been 
 *	gone thru this twice before then assume that the second button
 *	will not be pressed, otherwise arrange another poll.
 * 4.	otherwise, if in single button test (and buttons are different)
 *	then cancel the timeout and the single button mode (e.g.
 *	both buttons are up or both down.
 *
 */

msddecode(flag, mbp, tp)
	register int 	flag;
	register struct mb *mbp;
	struct   tty    *tp;
{
        register mousedata  *pms_report = &mbp->ms_report;
	register char   new_but;
	static		last_but = 0;
	int pollms();

	new_but  = pms_report->m_flags & BUT_MASK; 

	/* Check if this was a poll timeout */
	if (flag == 0) {	/* flag is 0 only when called from pollms */
		if (mbp->single_but) {
			if (++mbp->single_but > 2) 
				mbp->single_but = 0;
			else
				SETTIMEOUT (pollms, mbp, 2);
		}
		return(!mbp->single_but);
	}
	
	/* Check if both buttons are down and simulate a middle button */
	if (new_but == BOTH_BUT)
		new_but = MIDDLE_BUT;	

	if (msd_debug)
		printf ("sb (%x) last (%b) cur (%b)\n", mbp->single_but,
			last_but, BUT_DEFS, new_but, BUT_DEFS);
	if (!mbp->single_but && (last_but != new_but)) {
		if ((last_but == MIDDLE_BUT) && (new_but != NO_BUT)) {
		 	new_but = MIDDLE_BUT;
			mbp->single_but = 0;	/* (1) only one button let up */
		} else if ((last_but == NO_BUT) && (new_but != MIDDLE_BUT)) {
			SETTIMEOUT(pollms, mbp, 4);
			mbp->single_but++;	/* single button pressed down */
		}	/* otherwise single button let up */
	} else if ((mbp->single_but) && (last_but == new_but)) {
		untimeout(pollms, mbp);		/* no change in buttons */
		if (++mbp->single_but > 2)
			mbp->single_but = 0;	/* third time thru */
		else
			SETTIMEOUT (pollms, mbp, 4);
	} else if (mbp->single_but) {		/* change in buttons */
		untimeout(pollms, mbp);
		mbp->single_but = 0;
		if (new_but == NO_BUT) {
			pms_report->m_flags &= ~BUT_MASK; 		/* upate status */
			pms_report->m_flags |= last_but;	/* previous button */
			msreport(mbp,tp);	/* button let up - note both down & up reports */
		}
	}
	pms_report->m_flags &= ~BUT_MASK; 		/* upate status */
	pms_report->m_flags |= new_but;
	last_but = new_but;
	return(!mbp->single_but);
}

pollms (mbp)
register struct mb  *mbp;
{
	if (msd_debug)
		printf ("In pollms\n");
	
	if (msddecode(0, mbp, ptp)) 
		msreport(mbp, ptp);

	/* msioctl (0,MSIC_READXY,0,0); */
}

/*
 * This routine is called whenever a ioctl is about to be performed
 * and gets a chance to reject the ioctl.  We reject all teletype
 * oriented ioctl's except those which set the discipline, and
 * those which get parameters (gtty and get special characters).
 */
/*ARGSUSED*/
msdioctl(tp, cmd, data, flag)
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

msdselect(dev, rw)
	dev_t dev;
	int rw;
{
	register struct tty *tp = &cdevsw[major(dev)].d_ttys[minor(dev)];
	int s = MS_SPL;


	switch (rw) {

	case FREAD:
		if (tp->t_rawq.c_cc + tp->t_canq.c_cc > 0)     
			goto win;
		if (tp->t_rsel && tp->t_rsel->p_wchan == (caddr_t)&selwait)
			tp->t_state |= TS_RCOLL;
		else
			tp->t_rsel = u.u_procp;
		break;

	case FWRITE:
		goto win;
	}
	splx(s);
	return (0);
win:
	splx(s);
	return (1);
}
#endif
