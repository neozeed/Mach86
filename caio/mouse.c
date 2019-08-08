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
/* $Header: mouse.c,v 5.1 86/02/10 10:27:47 derrick Exp $ */
/* $Source: /ibm/acis/usr/sys_ca/caio/RCS/mouse.c,v $ */

/*
 * ibm032 Mouse driver, which works closely with the keyboard
 * code in cons.c
 */
#include "ms.h"
#if NMS > 0

#include "../h/param.h"
#include "../h/conf.h"
#include "../h/dir.h"
#include "../h/user.h"
#include "../h/proc.h"
#include "../h/ioctl.h"
#include "../h/tty.h"
#include "../h/systm.h"
#include "../h/uio.h"
#include "../ca/debug.h"
#include "../ca/io.h"
#include "../h/buf.h"
#include "../caio/mouseio.h"
#include "../caio/mousereg.h"

/* following taken from hd.c */
#define	SHOW_CHARS	0x01
#define SHOW_RDWR	0x02
#define SHOW_INTR	0x04
#define SHOW_INIT	0x08
#define SHOW_IO		0x10
#define SHOW_TIMEOUT	0x20		/* show wait */
#define SHOW_REGS	0x40
#define SHOW_OPEN	0x80

#define MOUSE_TIMEOUT	200000		/* time to do it if possible */
#define ISPEED B9600
struct tty ms[NMS];
int msdebug;
int msenabled;

/*ARGSUSED*/
msopen(dev, flag)
	dev_t dev;
{
	register struct tty *tp;
	register int unit = minor(dev);
	register int s;
	register int error;
	int ldisc = MSLINEDISC;

	DEBUGF(msdebug & SHOW_OPEN, printf("In PMSOPEN unit=%x\n", unit));
	tp = &ms[unit];
	tp->t_oproc = 0;
	if ((tp->t_state & TS_ISOPEN) == 0) {
		ttychars(tp);
		tp->t_ospeed = tp->t_ispeed = ISPEED;
		tp->t_state = TS_ISOPEN | TS_CARR_ON;
		tp->t_flags = ODDP;
		s = spl5();
		msparam(unit);
		if (reset_mouse()) {
			tp->t_state = 0;
			splx(s);
			return (EIO);
		}
		splx(s);
		DEBUGF(msdebug & SHOW_OPEN,
			printf("MSOPEN: call ttyopen (%x)\n",
				linesw[tp->t_line].l_open));
		if (error = (*linesw[tp->t_line].l_open)(dev, tp))
			return (error);
		return (msioctl(dev, TIOCSETD, (caddr_t) & ldisc, 0));
	}
	if (tp->t_state & TS_XCLUDE && u.u_uid != 0)
		return (EBUSY);
	return (0);
}

/*ARGSUSED*/
msclose(dev)
	dev_t dev;
{
	register int unit = minor(dev);
	register struct tty *tp = &ms[unit];
	register int s;

	DEBUGF(msdebug & SHOW_OPEN, printf("In MSCLOSE\n"));

	s = spl5();
	flush_uart(10);
	ms_cmd(UART_WR_CONTROL, MS_DISABLE);
	msenabled = 0;
	splx(s);
	(*linesw[tp->t_line].l_close)(tp);
	ttyclose(tp);
	tp->t_state = 0;
	DEBUGF(ttydebug & SHOW_OPEN, printf("ASYCLOSE end\n"));
}

/*ARGSUSED*/
msread(dev, uio)
	register dev_t dev;
	register struct uio *uio;
{
	register struct tty *tp = &ms[minor(dev)];

	DEBUGF(msdebug & SHOW_RDWR, printf("In MSREAD\n"));
	return ((*linesw[tp->t_line].l_read)(tp, uio));
}

/*
 * The Mouse wants to give us a character.
 * Catch the character, and see who it goes to.
 */
/*ARGSUSED*/
msrint(unit)
	register int unit;
{
	register int c, x;
	register struct tty *tp = &ms[unit];
	int iid;

	DEBUGF(msdebug & SHOW_INTR, printf("In MSRINT\n"));
	for (;;) {
		x = get_uart(&iid, 10);
		if (iid == KBD_TIMEOUT)
			break;
		if (iid == KBD_BLOCK)
			while (x--) {
				c = get_uart(&iid, 10);

				if (iid == KBD_UART) {
					DEBUGF(msdebug & SHOW_CHARS,
						printf("GOT -> (%x)\n", c));
					(*linesw[tp->t_line].l_rint)(c, tp);
					continue;
				}
				if (iid == KBD_TIMEOUT)
					break;
				if ((iid == KBD_ERROR) && (x > MS_ERROR)) {
					printf("msrint: ERROR -- %s\n",
						MS_ERROR_MSG[x - MS_ERROR]);
				}
		} else {
			DEBUGF(msdebug & SHOW_CHARS,
				printf("GOT -> (%x)\n", x));
			(*linesw[tp->t_line].l_rint)(x, tp);
		}
	}
	DEBUGF(msdebug & SHOW_INTR, printf("msrint end\n"));
}

/*ARGSUSED*/
msioctl(dev, cmd, addr, flag)
	dev_t dev;
	caddr_t addr;
{
	register int unit = minor(dev);
	register struct tty *tp = &ms[unit];
	register int error = 0;
	register int s;

	s = spl5();

	DEBUGF(msdebug,printf("in MSIOCTL(dev=%x, cmd=%x, addr=%x, flag=%x)\n",
		dev, cmd, addr, flag));
	switch (cmd) {
	case MSIC_STREAM:
		/* Set To STREAM mode (Two byte command) */
		ms_cmd(UART_WR_CONTROL, MS_STREAM);
		break;
	case MSIC_REMOTE:
		/* Set To REMOTE mode (Two byte command) */
		ms_cmd(UART_WR_CONTROL, MS_REMOTE);
		break;
	case MSIC_STATUS:
		/* Give the user the current raw status of the mouse */
		msstatus(addr);
		break;
	case MSIC_READXY:
		{
			register int reenable = msenabled;

			/* Force a read request to the mouse and go get the data */
			if (msenabled) {
				flush_uart(5);
				ms_cmd(UART_WR_CONTROL, MS_DISABLE);
			}
			ms_cmd(UART_WR_QUERY, MS_RD_DATA);
			msrint(unit);
			if (reenable)
				ms_cmd(UART_WR_CONTROL, MS_ENABLE);
			break;
		}
	case MSIC_ENABLE:
		/* Disable the mouse from sending data in stream mode */
		ms_cmd(UART_WR_CONTROL, MS_ENABLE);
		msenabled = 1;
		break;
	case MSIC_DISABLE:
		/* Disable the mouse from sending data in stream mode */
		flush_uart(10);
		ms_cmd(UART_WR_CONTROL, MS_DISABLE);
		msenabled = 0;
		break;
	case MSIC_EXP:
		/* Turn on Exponential scalling on the mouse */
		ms_cmd(UART_WR_CONTROL, MS_SET_EXP);
		break;
	case MSIC_LINEAR:
		/* Turn on Linear scalling on the mouse */
		ms_cmd(UART_WR_CONTROL, MS_RESET_EXP);
		break;
	case MSIC_SAMP:
		/* Set the sampling rate used for the mouse */
		DEBUGF(msdebug,
			printf("Set samp (%x)\n", MS_SET_SAMP | *(long *)addr);
		);
		ms_cmd(UART_WR_CONTROL, MS_SET_SAMP | *(long *)addr);
		break;
	case MSIC_RESL:
		/* Set the resolution the mouse uses */
		DEBUGF(msdebug,
			printf("Set resl (%x)\n", MS_SET_RESL | *(long *)addr);
		);
		ms_cmd(UART_WR_CONTROL, MS_SET_RESL | *(long *)addr);
		break;
	default:
		/* TTY ioctls */
		splx(s);
		error = (*linesw[tp->t_line].l_ioctl)(tp, cmd, addr);
		if (error >= 0)
			return (error);
		error = ttioctl(tp, cmd, addr, flag);
		if (error >= 0) {
			if (cmd == TIOCSETP || cmd == TIOCSETN) {
				s = spl5();
				msparam(unit);
				splx(s);
			}
			return (error);
		}
		if (error < 0)
			error = ENOTTY;
	}
	splx(s);
	return (error);
}

ms_cmd(u_cmd, m_cmd)
	register int u_cmd;
	register int m_cmd;
{
	register int x;
	    	 int iid;
	register int i;

	if (m_cmd & 0xFF00) {
		/* repeat calling uart_cmd until it returns 0 - cmd accepted */ 
		for (i=MOUSE_TIMEOUT ; uart_cmd(LCMD(u_cmd, m_cmd), 0) ; )
			if (--i == 0) {
				DEBUGF(msdebug&SHOW_TIMEOUT,printf("ms_cmd: timeout\n"));
				return(1);
			}
		do {       		/* make sure UART is not busy */
			x = uart_cmd(CMD(EXTCMD, READ_STAT), 0);
		} while ((((x >> 8) & IID_MASK) != KBD_REQ) 
			|| (x & UART_BUSY) != 0); 
	}
	for (i=MOUSE_TIMEOUT ; uart_cmd(CMD(u_cmd, m_cmd), 0) ; )
		if (--i == 0) {
			DEBUGF(msdebug&SHOW_TIMEOUT,printf("ms_cmd: timeout2\n"));
			return(1);
		}
	return (0);
}

msparam(unit)
	register int unit;
{
	register int x;
	register struct tty *tp = &ms[unit];

	DEBUGF(msdebug & SHOW_IO, {
		printf("In MSPARAM\n");
		printf("tp->t_flags = (%x)\n", tp->t_flags);
	}
	);

	/* Enable UART interface with a clear */
	x = uart_cmd(CMD(EXTCMD, UART_ENABLE), 0);
	if (x != 0x0000) {
		printf("msparam: uart did not enable (%x)\n", x);
	}
	/* Set the baud rate and initial mouse settings */
	DEBUGF(msdebug & SHOW_IO, printf("MSPARAM: Setting baud (%d)\n",
		MSBAUD(tp->t_ispeed));
	);
	x = uart_cmd(CMD(UART_SET_BAUD, MSBAUD(tp->t_ispeed)), 0);
	if (x != 0x0000) {
		printf("msparam: uart didn't Set baud rate (%x)\n", x);
	}
	/* Set the baud rate and initial mouse settings */
	x = uart_cmd(CMD(UART_INIT_FRM,
		((tp->t_flags & ODDP) ? UART_FRM_ODDP : UART_FRM_EVENP)),
		0);
	if (x != 0x0000) {
		printf("msparam: uart didn't init framing (%x)\n", x);
	}
	DEBUGF(msdebug & SHOW_IO, printf(" msparam end\n"));
}

msstatus(arg)
	register char *arg;
{
	register int x;
	register int reenable = msenabled;
	int iid;

	if (msenabled) {
		flush_uart(10);
		ms_cmd(UART_WR_CONTROL, MS_DISABLE);
	}
	ms_cmd(UART_WR_QUERY, MS_STATUS);

	x = get_uart(&iid, 40000);
	if ((iid != KBD_BLOCK) || (x != 4)) {
		printf("msstatus: Bad block iid (%x) bc (%d)\n", iid, x);

		/* Flush what it wanted to give us */
		if (iid == KBD_BLOCK)
			while (x--)
				printf(" (%x)", get_uart(&iid, 400000));
		return (0);
	}
	*arg = get_uart(0, 40000);
	DEBUGF(msdebug & SHOW_REGS, {
		printf("STATUS BYTE 1: (%b)", *arg, MS_STATUS_B1);
	}
	);
	*++arg = get_uart(0, 40000);
	DEBUGF(msdebug & SHOW_REGS, {
		printf("STATUS BYTE 2: (%b)", *arg, MS_STATUS_B2);
	}
	);
	*++arg = get_uart(0, 40000);
	DEBUGF(msdebug & SHOW_REGS, {
		printf("Resolution: (%d)", *arg);
	}
	);
	*++arg = get_uart(0, 40000);
	DEBUGF(msdebug & SHOW_REGS, {
		printf("Sampling rate: (%d)", *arg);
	}
	);
	if (reenable)
		ms_cmd(UART_WR_CONTROL, MS_ENABLE);
}

#define MS_MAX_RETRY 3
char ms_reset_ack[] = { 0xff, 0x08, 0x00, 0x00 };

reset_mouse()
{
	register int i;
	register int retry = 0;
	register char x;
	int iid;

	/* Enable Blocking mode on UART */
	if (ms_cmd(EXTCMD, UART_SET_BK)) {
		printf("reset_mouse: uart didn't Set block mode\n");
		return (1);
	}
	/* Reset the MOUSE */
	do {
		if (ms_cmd(UART_WR_QUERY, MS_RESET)) {
			printf("reset_mouse: mouse didn't accept reset\n");
		}
		x = get_uart(&iid, 400000);
		if (iid == KBD_TIMEOUT) {
			printf("Uart timed out waiting for RESET\n");
			continue;
		}
		if ((iid != KBD_BLOCK) || (x != 4)) {
			printf("reset_mouse: Bad block iid (%x) bc (%d)\n",
				iid, x);
			/* Flush what it wanted to give us */
			if (iid == KBD_BLOCK)
				for (i = 0; i < x; i++)
					printf("(%x) ", get_uart(&iid, 400000));
			continue;
		}
		for (i = 0; i < 4; i++) {
			x = get_uart(&iid, 400000);
			if ((iid == KBD_UART) && (x == ms_reset_ack[i]))
				continue;
			if ((iid == KBD_ERROR) && (x > MS_ERROR)) {
				printf("reset_mouse: ERROR -- %s\n",
					MS_ERROR_MSG[x - MS_ERROR]);
			}
			/* Flush out anything left arround */
			do {
				get_uart(&iid, 100000);
			} while (iid != KBD_TIMEOUT);
			break;
		}
		if (i == 4)
			break;
	} while (retry++ < MS_MAX_RETRY);

	if (retry >= MS_MAX_RETRY) {
		printf("reset_mouse: Mouse didn't reset (%x)\n", x);
		return (1);
	}
	/* Disable Blocking mode on UART */
	if (ms_cmd(EXTCMD, UART_RESET_BK)) {
		printf("reset_mouse: uart didn't Reset block mode\n");
		return (1);
	}
	/* Read Configuration */
	if (ms_cmd(UART_WR_QUERY, MS_RD_CONF)) {
		printf("reset_mouse: mouse didn't accept read config\n");
		return (1);
	}
	if ((x = get_uart(0, 4000)) != MS_CONFIGURED) {
		printf("reset_mouse: Wrong configuration resp (%x)\n", x);
		return (1);
	}

	/* Enable Blocking mode on UART */
	if (ms_cmd(EXTCMD, UART_SET_BK)) {
		printf("reset_mouse: uart didn't Set block mode\n");
		return (1);
	}
	/* Enable data transmissions */
	if (ms_cmd(UART_WR_CONTROL, MS_ENABLE)) {
		printf("reset_mouse: mouse didn't accept enable cmd\n");
		return (1);
	}
	msenabled = 1;

	return (0);

}

uart_cmd(cmd, timeout)
	register int cmd, timeout;
{
	/*
	 * wait for PA (8255) to be able to accept a command and then
	 * send it.
	 * wait for the response.
	 */
	register int i;
	int iid;

	cmd &= 0xffff;			/* mask off any junk */
	DEBUGF(msdebug & SHOW_CHARS,
		printf("uart_cmd(%x,%d)\n", cmd, timeout));
	for (i = 0; (in(KYB_CNTIR) & KYB_OBF) == 0; ++i)
		if (i > MOUSE_TIMEOUT) {
			DEBUGF(msdebug&SHOW_TIMEOUT,printf("uart_cmd: timeout\n"));
			break;
		}
					/* For output buffer to empty */
	if (i > 100)
		DEBUGF(msdebug,
			printf("kbd_cmd took %d iterations for busy to clear\n",
				i));
	outw(KYB_WRITE, cmd);
	i = get_uart(&iid, timeout);
	return ((iid << 8) + i);	/* return the resulting character */
}

/*
 * wait for something to appear at the keyboard, then return it
 * and the associated status byte.
 */
get_uart(iidptr, timeout)
	register int *iidptr;
	register int timeout;
{
	register int c, iid;
	static char *iidtypes[] = {
	"INFO", "DATA", "UART", "REQ", "BLOCK", "05", "TEST", "ERROR" };

	DEBUGF(msdebug & SHOW_CHARS, printf("waiting for Uart\n"));
	while (((iid = in(KYB_CNTIR)) & KYB_IBF) == 0) {
		if (timeout && --timeout == 0) {
			if (iidptr != 0)
				*iidptr = KBD_TIMEOUT;
			return (0);
		}
	}
	c = in(KYB_READ) & 0377;
	DEBUGF(msdebug & SHOW_CHARS,
		printf("got %x (IID=%x %s) from uart\n", c, iid, iidtypes[iid&07]));
	if (iidptr != 0)
		*iidptr = iid & IID_MASK; /* return it if requested */
	return (c);
}

flush_uart(n)
	register int n;
{
	int iid;

	while (n--) {
		get_uart(&iid, 10000);
		if (iid == KBD_TIMEOUT)
			break;
	}
}
#endif NMS
