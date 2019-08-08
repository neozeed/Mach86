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
/* $Header: cons.c,v 4.18 85/09/05 22:49:16 webb Exp $ */
/* $Source: /ibm/acis/usr/sys_ca/ca/RCS/cons.c,v $ */

/*	cons.c	6.1	83/07/29	*/

#if	CMU
/*
 * ROMP console driver
 ***********************************************************************
 * HISTORY
 * 13-Feb-86  Bill Bolosky (bolosky) at Carnegie-Mellon University
 *	Added switch LOCK_KEY which causes Caps Lock to function as
 *	another control key (and consequently makes caps lock
 *	impossible).
 *
 */
#include "debounce.h"
#include "lock_key.h"
#endif	CMU

#include "../h/param.h"
#include "../h/types.h"
#include "../h/conf.h"
#include "../h/dir.h"
#include "../h/user.h"
#include "../h/proc.h"
#include "../h/ioctl.h"
#include "../h/tty.h"
#include "../h/systm.h"
#include "../h/uio.h"
#include "../h/file.h"
#include "../machine/debug.h"

#include "../caio/screen_conf.h"
#include "../machine/monocons.h"
#include "../machine/io.h"

#include "ms.h"

#include "aed.h"

#if NAED > 0
#include "../caio/aeddefs.h"
#include "../ca/scr.h"
extern struct tty aedsotty;
extern AED aed_info;
extern char aeddebug;
#endif NAED

#define CNDLEVEL 0
#define screen_put mono_screen_putc	  /* kludge */

#ifdef CNDEBUG
#define CNDEBUGF(cond,stmt) DEBUGF(cond,stmt)
#else
#define CNDEBUGF(cond,stmt)
#endif

struct tty cons;
#define ISPEED B9600
#define TEST_PAT 0x65

int cnstart();
int ttrstrt();
int debug_key = 0;


cnatch(dev)
	register dev_t dev;
{
	register struct tty *tp;

	tp = &cons;
	tp->t_state = 0;
	ttychars(tp);
	tp->t_ospeed = tp->t_ispeed = ISPEED;
	tp->t_flags = EVENP | ECHO | CRMOD;
	screen_init();
}


/*ARGSUSED*/
cnopen(dev, flag)
	dev_t dev;
{
	register struct tty *tp;
	register int unit = minor(dev);
	register int s;

	tp = &cons;
	tp->t_oproc = cnstart;
	if ((tp->t_state & TS_ISOPEN) == 0) {
		ttychars(tp);
		tp->t_ospeed = tp->t_ispeed = ISPEED;
		tp->t_state = TS_ISOPEN | TS_CARR_ON;
		tp->t_flags = EVENP | ECHO | CRMOD;
	}
	if (!_init_kbd) {
		register int s = spl5();
		init_kbd();	/* not normally needed */
		splx(s);
	}
	if (tp->t_state & TS_XCLUDE && u.u_uid != 0)
		return (EBUSY);

	tp->t_state |= TS_CARR_ON;
	CNDEBUGF (cndebug > CNDLEVEL, printf("CNOPEN: call ttyopen (%x)\n", linesw[tp->t_line].l_open));
	return ((*linesw[tp->t_line].l_open)(dev, tp));
}


/*ARGSUSED*/
cnclose(dev)
	dev_t dev;
{
	register int unit = minor(dev);
	register struct tty *tp = &cons;

	CNDEBUGF (cndebug > CNDLEVEL, printf("In CNCLOSE\n"));

	(*linesw[tp->t_line].l_close)(tp);
	ttyclose(tp);

}


/*ARGSUSED*/
cnread(dev, uio)
	register dev_t dev;
	register struct uio *uio;
{
	register struct tty *tp = &cons;
	CNDEBUGF (cndebug > CNDLEVEL, printf("In CNREAD\n"));
	return ((*linesw[tp->t_line].l_read)(tp, uio));
}


/*ARGSUSED*/
cnwrite(dev, uio)
	dev_t dev;
	struct uio *uio;
{
	register struct tty *tp = &cons;

	CNDEBUGF (cndebug > CNDLEVEL, printf("In CNWRITE\n"));
	return ((*linesw[tp->t_line].l_write)(tp, uio));
}


/*
 * Got a level-1 receive interrupt -
 * the Keyboard wants to give us a character.
 * Catch the character,  see who it goes to.
 */
/*ARGSUSED*/
cnrint(dev)
	dev_t dev;
{
	register int c;
	register int ch;
	register int unit = minor(dev);
	register struct tty *tp = &cons;
	int iid;


	CNDEBUGF(cndebug & 0x40, printf("In CNRINT\n"));


	for (;;) {
		if ((ch = get_kbd(&iid, 10)) <= 0)
			break;
		if ((c = key_scan(ch)) == NONE)
			continue;
		CNDEBUGF(cndebug & 0x80,
		    printf("got character %x from scan code %x\n", c, ch));
		if (c & ESC_MARK)
			(*linesw[tp->t_line].l_rint)(033, tp);
		(*linesw[tp->t_line].l_rint)(c, tp);

	}
	CNDEBUGF(cndebug & 0x20, printf("cnrint end\n"));
}


/*ARGSUSED*/
cnioctl(dev, cmd, addr, flag)
	dev_t dev;
	caddr_t addr;
{
	register int unit = minor(dev);
	register struct tty *tp = &cons;
	register int error;

	if (cmd == CON_SELECT_SCREEN) {
		register int save_screen;

		CNDEBUGF(cndebug, printf("which_screen (%d), arg (%d)\n",
		    which_screen, *(int *)addr));
		save_screen = which_screen;
		which_screen = *(int *)addr;
		error = switch_screen(*(int *)addr);
		*(int *)addr = save_screen;
		return (error < 0 ? EINVAL : 0);
	}
	error = (*linesw[tp->t_line].l_ioctl)(tp, cmd, addr);
	if (error >= 0)
		return (error);
	error = ttioctl(tp, cmd, addr, flag);
	if (error > 0) {
		if (cmd == TIOCSETP || cmd == TIOCSETN) {
			init_kbd();
		}
		return (error);
	}
	if (error < 0)
		error = ENOTTY;
	return (error);
}


/*
 * for now none of the console routines are interrupt driven
 * we will simulate an interrupt driven console via a timeout.
 * the timeout will invoke the actual start routine which will
 * output characters at low enough priority to not hold up 
 * the rest of the system during normal console output.
 */
cnstart(tp)
	register struct tty *tp;
{
	register int c;
	register int unit = minor(tp->t_dev);
#if NAED > 0
	register struct tty *aedtp = &aedsotty;
	register AED * aed = &aed_info;
	int fromuser;
#endif NAED
	int cnostart();
	register int s = spl5();

	if (tp->t_state & (TS_TIMEOUT | TS_BUSY | TS_TTSTOP))
		goto lout;
#if NAED > 0
	if ((mfsr(SCR_ICS) & ICSCS_PRIORITY / 0x10000) >= 0x7 &&
			(aed->state & AED_WM_MODE)) {
		while ((tp->t_outq.c_cc > 0) && ((c = getc(&tp->t_outq)) != -1)) {
				(*linesw[aedtp->t_line].l_rint)(c, aedtp, 2);
			}
		} else
#endif NAED
		{
		tp->t_state |= TS_BUSY;			/* we are now busy */
		timeout(cnostart, tp, 1);		/* minimal delay */
	}
lout:
	splx(s);
}

/*
 * cnostart is entered at low priority from timeout routine
 * it then outputs the characters either via the aedsout or
 * via the actual output routine for this screen.
 */
cnostart(tp)
	register struct tty *tp;
{
	register int c;
	register int unit = minor(tp->t_dev);
#if NAED > 0
	register struct tty *aedtp = &aedsotty;
	register AED * aed = &aed_info;
	int fromuser;
#endif NAED
	register int s = spl5();		/* protect data structures */
	while ((tp->t_outq.c_cc > 0) && ((c = getc(&tp->t_outq)) != -1)) {
#if NAED > 0
		/* was cnstart called from interrupt level? */
		if ((mfsr(SCR_ICS) & ICSCS_PRIORITY / 0x10000) < 0x7)
			fromuser = 0;	  /* from interrupt level */
		else
			fromuser = 2;
		if (aed->state & AED_WM_MODE) {
			(*linesw[aedtp->t_line].l_rint)(c, aedtp, fromuser);
		} else
#endif NAED

		if ((tp->t_flags&(RAW|LITOUT)) == 0 && c > 0177) {
			timeout(ttrstrt, (caddr_t)tp, (c&0177));
			tp->t_state |= TS_TIMEOUT;
			goto lout;	/* clear busy, return */
		}
		else {
			splx(s);		/* do output at low priority */
			(*screen_sw[which_screen].putc)(c);
			s = spl5();		/* raise priority again */
			if (tp->t_state & TS_TTSTOP)
				break;		/* stop output for now */
		}
	}
	if (tp->t_outq.c_cc <= TTLOWAT(tp)) {
		if (tp->t_state & TS_ASLEEP) {
			tp->t_state &= ~TS_ASLEEP;
			wakeup((caddr_t) & tp->t_outq);
		}
		if (tp->t_wsel) {
			selwakeup(tp->t_wsel, tp->t_state & TS_WCOLL);
			tp->t_wsel = 0;
			tp->t_state &= ~TS_WCOLL;
		}
	}
lout:
	tp->t_state &= ~TS_BUSY;		/* clear busy */
	splx(s);
}


/*
 * Print a character on console.
 * Attempts to save and restore device
 * status.
 */
cnputc(c)
	register int c;
{
#if NAED > 0
	register int fromuser = 0;	  /* Not from user */
	register struct tty *tp = &aedsotty;
	register AED * aed = &aed_info;
#endif NAED


	/*
	 * Try waiting for the console tty to come ready,
	 * otherwise give up after a reasonable time.
	 */
	if (c == 0)
		return;

	if (which_screen < 0)
		screen_init();		/* in case of debugging messages before cnatch */
#if NAED > 0
	if (aed->state & AED_WM_MODE) {
		if (c == '\n')
			(*linesw[tp->t_line].l_rint)('\r', tp, fromuser);
		(*linesw[tp->t_line].l_rint)(c, tp, fromuser);
#ifdef AEDDEBUG
		if (aeddebug) {
			if (c == '\n')
				screen_put('\r');
			screen_put(c);
		}
#endif AEDDEBUG
	} else
#endif NAED
	{
		if (c == '\n')
			(*screen_sw[which_screen].putc)('\r');
		(*screen_sw[which_screen].putc)(c);
	}
	cnputc(0);
}


#define CN_MAJOR 0

cnint(unit, icscs)
	register int unit;
	register int icscs;
{
	register dev_t dev;
	register unsigned char saveiic;

	CNDEBUGF (cndebug > CNDLEVEL + 5, printf("In cnint unit (%x)\n", unit));

	/* Create the dev fromt Major and minor numbers */
	dev = makedev(CN_MAJOR, unit);


	saveiic = in(KYB_CNTIR) & KYB_IIC_MASK;

	CNDEBUGF (cndebug > CNDLEVEL + 5, printf("cnint: IIC (%x)", saveiic));

	if (!(saveiic & KYB_INT)) {
		CNDEBUGF (cndebug > CNDLEVEL + 5, printf("cnint: NO INTS pending\n"));
		return (1);
	}
	switch (saveiic) {
	case KYB_DATA:
	case KYB_EC:
	case KYB_INFO:
	case KYB_REQD:
		cnrint (dev);
		return (0);
		break;
	case UART_DATA:
	case UART_BLK:
#if NMS > 0
		msrint (0);
		return (0);
#endif NMS
		/* fall thru if no MS defined */
	case KYB_UNASS:
	case KYB_SR:
		{
		register int c = in(KYB_READ) & 0377;	 /* clear interrupt */
		return (0);
		}
	default:
		break;
	}
	/* NO INTERRUPT FROM THE CONSOLE Keyboard!!!!!! */
	CNDEBUGF (cndebug > CNDLEVEL + 5, printf("cnint: no ints\n"));
	return (1);
}


#define FULL_TEST	1		/* force full keyboard init'n */
#define FAIL(n)	TRACEF(("return %d\n",n)) /* ; return(n)	/* */

/* following allows us to get rid of bell and keyboard clicks easily */
#ifndef NOBELL
#define NOBELL 0
#endif

int nobell = NOBELL;

init_kbd()
{
	register int x, i;
	int iid;

	++_init_kbd;		       /* mark as done */
	TRACEF(("init init_kbd\n"));

#ifdef NOTIMEOUT
	getchar_timeout = 0;
#endif

	make_break = state = 0;
	out(KYB_CNTIW, 0x09);	       /* enable interrupt request */
#ifdef FULL_TEST
	kbd_reset();		       /* reset the keyboard */

	out(KYB_CNTIW, KYB_CONFIG);    /* configure 8255 */
	delay(200);		       /* delay 200 ms */

	clear_kbd(40);		       /* clear pipeline */
	if (in(KYB_CNTIR) & KYB_IBF) {
		kbd_not_empty();
		FAIL(1);
	}
	x = kbd_cmd(CMD(EXTCMD, RD1C), 0); /* issue read shared RAM command */
	if (x != (0xAE | (KBD_REQ << 8))) {
		printf("keyboard self test failed: IID/data = %x\n", x);
		FAIL(3);
	}
	clear_kbd(10);		       /* do 10 reads to empty pipeline */
	if (in(KYB_CNTIR) & KYB_IBF) {
		kbd_not_empty();
		FAIL(3);
	}
	x = kbd_cmd(CMD(EXTCMD, ENKYBD), 0); /* issue set mode 11 command */
	if (x != 0x0000) {
		printf("keyboard failed to clear: IID/data = %x\n", x);

		FAIL(4);
	}
	clear_kbd(10);		       /* do 10 reads to empty pipeline */
	if (in(KYB_CNTIR) & KYB_IBF) {
		kbd_not_empty();
		FAIL(5);
	}
	x = kbd_cmd(CMD(KYBDCMD, KRESET), 0); /* issue reset keyboard command */
	if (x != 0x0000) {
		printf("keyboard failed to reset: IID/data = %x\n", x);

		FAIL(6);
	}
	/* kbd_cmd(CMD(KYBDCMD,KDEFDS),0);	/* disable keyboard default */
	clear_kbd(10);
	kbd_cmd(CMD(KYBDCMD, KSCAN), 0); /* enable scanning */
	for (i = 0; i < 4; ++i) {
		x = get_kbd(&iid, 2000); /* pick up the BAT etc. */
		if (x == 0xAA)
			break;
	}
	beep();			       /* ring my chimes ! */
	clear_kbd(10);
	out(KYB_CNTIW, 0x09);	       /* enable interrupt request */
#endif				       /* FULL_TEST */
	clear_kbd(10);
	TRACEF(("kbd_init normal return\n"));
	kbd_status();
	return (0);
}


/*
 * wait for something to appear at the keyboard, then return it
 * and the associated status byte.
 */
get_kbd(iidptr, timeout)
register int *iidptr;
register int timeout;
{
	register int c, iid;

	TRACEF(("waiting for keyboard\n"));
	while (((iid = in(KYB_CNTIR)) & KYB_IBF) == 0) {
		if (timeout && --timeout == 0) {
			if (iidptr != 0)
				*iidptr = KBD_TIMEOUT;
			return (0);
		}
		last_scan = 0;	       /* reset last_scan if we have to wait */
	}
	c = in(KYB_READ) & 0377;
	TRACEF(("got %x (IID=%x) from keyboard\n", c, iid));
	if (iidptr != 0)
		*iidptr = iid & IID_MASK; /* return it if requested */
	return (c);
}


LOCAL clear_kbd(n)
register int n;
{
	register int c;

	while (--n >= 0) {
		c = in(KYB_READ);
		delay(1);
	}
}


/* flag bits for state variable */
#define CAPS_MODE	01	       /* must be 1. caps mode */
#define ALT_MODE	02	       /* alt mode */
#define CTL_MODE	04	       /* control mode */


#ifdef CLICK_CHAR
#define CLICK_FREQ 0x8c0	       /* click frequency */
#define CLICK_TIME 0x00		       /* minimal time */
int click_freq = CLICK_FREQ;
int click_time = CLICK_TIME;
#endif CLICK_CHAR

static	int shift_lock_down = 0;

key_scan(code)
register int code;
{
	register int n = (state & CAPS_MODE) + (code << 1); /* CAPS_MODE == 1 */
	register int c = NONE;
	static int click = 1;

#if	DEBOUNCE
	if (last_scan == code)
		return (c);	       /* ignore 'BOUNCES' */
	last_scan = code;
#endif

	if (make_break) {
		register int old_state = state;

		make_break = 0;
		switch (code) {
		case SHIFT_LOCK:       /* lock key let go */
#if	LOCK_KEY
			state &= ~CTL_MODE; /* control key released */
#else 	LOCK_KEY
			shift_lock_down = 0;
#endif 	LOCK_KEY
			break;
		case ALT1:
		case ALT2:
			state &= ~ALT_MODE; /* alt key released */
			break;
		case SHIFT1:
		case SHIFT2:
			state &= ~CAPS_MODE; /* shift key released */
			set_kbd_led(~CAPS_LED);	/* turn off cap lock led */
			break;
		case CNTRL:
			state &= ~CTL_MODE; /* control key released */
			break;
		}
		if (state != old_state)
			kbd_status();  /* reflect state change */
	} 
	else if (code >= MAX_CODES || (c = codes[n]) == NONE) {
		switch (code) {
		case SHIFT_LOCK:       /* caps lock key pressed */
#if	LOCK_KEY
			state |= CTL_MODE; /* key held down */
			kbd_status();
#else 	LOCK_KEY
			if (shift_lock_down == 0)
			{
				state ^= CAPS_MODE; /* invert caps state each time */
				set_kbd_led(state & CAPS_MODE ? CAPS_LED : ~CAPS_LED);
				shift_lock_down = 1;
				kbd_status();
			} 
#endif	LOCK_KEY
			break;
		case ALT1:
		case ALT2:
			state |= ALT_MODE; /* key held down */
			kbd_status();
			break;
		case SHIFT1:
		case SHIFT2:
			state |= CAPS_MODE; /* key held down */
			kbd_status();
			break;
		case CNTRL:
			state |= CTL_MODE; /* key held down */
			kbd_status();
			break;
#ifdef DEBUG_CHAR
		case DEBUG_CHAR:
			if (state == 0)
				debug_key = !debug_key;
			else if (state == ALT_MODE)
				cnhangup();
			kbd_status();
			break;
#endif
		case BREAK:
			make_break = 1;	/* released a make/break key */
			break;
#ifdef SWITCH_CHAR
		case SWITCH_CHAR:
			switch_screen((state&CTL_MODE) ? SCREEN_SWITCH_RELOAD : SCREEN_SWITCH ); /* to next screen */
			break;
#endif

#ifdef PRINT_CHAR
		case PRINT_CHAR:
			print_screen(state & CTL_MODE);	/* print it */
			break;
#endif

#ifdef CLICK_CHAR
		case CLICK_CHAR:
			{
				click = (click + 1) & 03;
				kbd_cmd(CMD(EXTCMD, ((click & 1) ? 0x2e : 0x3e)), 0);
			}
			break;

#endif CLICK_CHAR
		}
	} 
	else {
		if (state == (ALT_MODE | CTL_MODE) && c == 0177)
			_reboot();     /* start execution again */
		if (state & CTL_MODE)
			c &= 037;
		if (c & ESC_FLAG)
			c ^= ESC_FLAG | ESC_MARK; /* turn on extra bit for ESC */
		if (state & ALT_MODE)
			c |= ALT_MARK; /* flag as alt hit */
	}
	TRACEF(("code = %x state=%x ==>%x\n", code, state, c));
#ifdef CLICK_CHAR
	if (c != NONE && (click & 2))
		speaker(click_freq, click_time);
#endif
	return (c);
}


LOCAL kbd_status()
{
	static int xdebug_key = -1;

	if (xdebug_key != debug_key) {
debug_key_loop:
		put_status(60, debug_key ? "DEBUG" : "     ");
		xdebug_key = debug_key;
#if defined(KERNEL) && defined(RDB)
		if (debug_key) {
			debug_key = 0;
			asm(" setsb r12,16-16 "); /* level 0 interrupt */
			goto debug_key_loop; /* redisplay it */
		}
#endif
	}
	put_status(68, (state & ALT_MODE) ? "ALT" : "   ");
	put_status(71, (state & CAPS_MODE) ? "CAPS" : "    ");
	put_status(75, (state & CTL_MODE) ? "CTRL" : "    ");
}



set_kbd_led(leds)
char leds;
{
	register int x;
	int iid;
	if (leds <= 7)
		kbd_leds_state = kbd_leds_state | leds;	/* turn leds on */
	else
		kbd_leds_state = kbd_leds_state & leds;	/* turn leds off */
	do 
	    x=kbd_cmd(CMD(KYBDCMD, SETLED), 0);
	while (x !=0);

	do {				/* wait for command complete */
		x=kbd_cmd(CMD(EXTCMD,READ_STAT),0);
		while (x & IID_MASK <<8 != KBD_REQ <<8) {
			x=get_kbd(&iid,0);
			x+= iid << 8;
		}
	}
	while ( x & KYB_BUSY != 0);

	kbd_cmd(CMD(KYBDCMD, kbd_leds_state), 0);
}



#ifdef FULL_TEST
static kbd_not_empty()
{
	printf("keyboard pipeline won't empty\n");
}


kbd_reset()
{
	register int x;

	TRACEF(("kbd_reset called\n"));

	x = in(CRRB);
	x &= KBD_ADAPTOR_RESET;
	out(CRRB, x);

	delay(200);
	x = in(CRRB);
	x |= KBD_ADAPTOR_RELEASE;
	out(CRRB, x);

}


#endif	FULL_TEST

LOCAL
kbd_cmd(cmd, timeout)
register int cmd, timeout;
{
	/*
	 * wait for PA (8255) to be able to accept a command and then
	 * send it.
	 * wait for the response.
	 */
	register int i;
	int iid;

	cmd &= 0xffff;		       /* mask off any junk */
	TRACEF(("kbd_cmd(%x,%d)\n", cmd, timeout));
	for (i = 0; (in(KYB_CNTIR) & KYB_OBF) == 0; ++i);
	/* for for output buffer to empty */
	if (i > 100)
		TRACEF(("kbd_cmd took %d iterations for busy to clear\n", i));
	outw(KYB_WRITE, cmd);
	i = get_kbd(&iid, timeout);
	return ((iid << 8) + i);       /* return the resulting character */
}


int beep_freq = (FCHI << 8) + FCLO;
int beep_time = SPTIME;

beep()
{
	speaker(beep_freq, beep_time);
}


speaker(beep_freq, beep_time)
register int beep_freq, beep_time;
{
#ifdef KERNEL
	register int s = spl5();
#else
	if (!_init_kbd)
		return;		       /* oops */
#endif KERNEL
	if (nobell)
		return;
	TRACEF(("speaker(%d,%d) entered\n", beep_freq, beep_time));
	kbd_cmd(CMD(SETFCHI, (beep_freq >> 8)), 100);
	kbd_cmd(CMD(SETFCLO, (beep_freq)), 100);
	/* kbd_cmd(CMD(EXTCMD,SP_MEDIUM),100);	/* set volume to medium */
	kbd_cmd(CMD(SPKRON, beep_time), 100);
	TRACEF(("speaker done\n"));
#ifdef KERNEL
	splx(s);
#endif
}


_reboot()
{
	/*
	 * generate a reboot request via the keyboard interface
	 */
	TRACEF(("reboot in progress..."));
	kbd_cmd(CMD(EXTCMD, 0x3a), 10000); /* turn on diagnostic mode */
	kbd_cmd(CMD(EXTCMD, 0x61), 10000); /* request soft reset */
	kbd_cmd(CMD(EXTCMD, 0x2a), 10000); /* turn off diagnostic mode */
	delay(5000);		       /* should be long enough */
	/* NOTREACHED */
	TRACEF(("reboot failed\n"));   /* can't get here ! */
}


cnhangup()
{
#ifdef KERNEL
	register struct tty *tp = &cons;
	if (tp->t_state&TS_ISOPEN) {
		gsignal (tp-> t_pgrp, SIGHUP);
		gsignal (tp-> t_pgrp, SIGCONT);
		ttyflush(tp,FREAD|FWRITE);
		tp->t_state &= ~ TS_CARR_ON;
	}
#else
	beep();
#endif
}


slave_cnenable()
{}

