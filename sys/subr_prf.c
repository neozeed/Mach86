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
 *	@(#)subr_prf.c	6.9 (Berkeley) 9/17/85
 */
#if	CMU
/*
 **********************************************************************
 * HISTORY
 * 13-Apr-86  Avadis Tevanian (avie) at Carnegie-Mellon University
 *	CS_GENERIC:  print table full messages on console in addition to
 *	loging them.
 *
 * 15-Feb-86  Bill Bolosky (bolosky) at Carnegie-Mellon University
 *	Added different definitions of the parameter list for printf and
 *	uprintf under the assumption that the Sailboat c compiler needs
 *	them.  Switched on by romp.
 *
 * 25-Jan-86  Avadis Tevanian (avie) at Carnegie-Mellon University
 *	Upgraded to 4.3.
 *
 *  8-Sep-85  Avadis Tevanian (avie) at Carnegie-Mellon University
 *	On a panic, issue a break point trap if kernel debugger present
 *	(and active).
 *
 * 26-Jun-85  Mike Accetta (mja) at Carnegie-Mellon University
 *	CS_KDB:  Modified panic() to generate a breakpoint trap
 *	before rebooting.
 *	[V1(1)]
 *
 * 14-Jun-85  Avadis Tevanian (avie) at Carnegie-Mellon University
 *	Added (and modified) mprintf from Ultrix.
 *
 * 15-Sep-84  Robert V Baron (rvb) at Carnegie-Mellon University
 *	Spiffed up printf b format. you can also give an entry of the form
 *	\#1\#2Title
 *	will print Title then extract the bits from #1 to #2 as a field and
 *	print it -- some devices have fields in their csr. consider
 *	vaxmpm/mpmreg.h as an example.
 **********************************************************************
 */

#include "cs_bugfix.h"
#include "cs_generic.h"
#include "cs_kdb.h"
#endif	CMU

#include "param.h"
#include "systm.h"
#include "seg.h"
#include "buf.h"
#include "conf.h"
#include "reboot.h"
#include "vm.h"
#include "msgbuf.h"
#include "dir.h"
#include "user.h"
#include "proc.h"
#include "ioctl.h"
#include "tty.h"
#include "syslog.h"
#ifdef vax
#include "../vax/mtpr.h"
#endif

#define TOCONS	0x1
#define TOTTY	0x2
#define TOLOG	0x4

/*
 * In case console is off,
 * panicstr contains argument to last
 * call to panic.
 */
char	*panicstr;

/*
 * Scaled down version of C Library printf.
 * Used to print diagnostic information directly on console tty.
 * Since it is not interrupt driven, all system activities are
 * suspended.  Printf should not be used for chit-chat.
 *
 * One additional format: %b is supported to decode error registers.
 * Usage is:
 *	printf("reg=%b\n", regval, "<base><arg>*");
 * Where <base> is the output base expressed as a control character,
 * e.g. \10 gives octal; \20 gives hex.  Each arg is a sequence of
 * characters, the first of which gives the bit number to be inspected
 * (origin 1), and the next characters (up to a control character, i.e.
 * a character <= 32), give the name of the register.  Thus
 *	printf("reg=%b\n", 3, "\10\2BITTWO\1BITONE\n");
 * would produce output:
 *	reg=3<BITTWO,BITONE>
 */
/*VARARGS1*/
#ifndef	romp
printf(fmt, x1)
	char *fmt;
	unsigned x1;
#else	romp
printf(fmt, x1, x2)
	register char *fmt; /* REGISTER for locore */
	unsigned x1, x2;
/*I am including this on the assumption that x2 is necessary on the romp due
 *to lossage on the part of the romp c compiler.  -BJB
 */
#endif	romp
{

	prf(fmt, &x1, TOCONS | TOLOG, (struct tty *)0);
	logwakeup();
}

/*
 * Uprintf prints to the current user's terminal
 * and does no watermark checking - (so no verbose messages).
 */
/*VARARGS1*/
#ifndef	romp
uprintf(fmt, x1)
	char *fmt;
	unsigned x1;
#else	romp
uprintf(fmt, x1, x2)
	char *fmt;
	unsigned x1, x2;
#endif	romp
{

	prf(fmt, &x1, TOTTY, u.u_ttyp);
}

/*
 * tprintf prints on the specified terminal (console if none)
 * and logs the message.  It is designed for error messages from
 * single-open devices, and may be called from interrupt level.
 */
/*VARARGS2*/
tprintf(ttyp, fmt, x1)
	struct tty *ttyp;
	char *fmt;
	unsigned x1;
{

	prf(fmt, &x1, TOTTY | TOLOG, ttyp);
}

/*
 * Log writes to the log buffer,
 * and guarantees not to sleep (so can be called by interrupt routines).
 * If there is no process reading the log yet, it writes to the console also.
 */
/*VARARGS2*/
log(level, fmt, x1)
	char *fmt;
	unsigned x1;
{
	register s = splhigh();
	extern int log_open;

	putchar('<', TOLOG, (struct tty *)0);
	printn(level, 10, TOLOG, (struct tty *)0);
	putchar('>', TOLOG, (struct tty *)0);
	prf(fmt, &x1, TOLOG, (struct tty *)0);
	splx(s);
	if (!log_open)
		prf(fmt, &x1, TOCONS, (struct tty *)0);
	logwakeup();
}

prf(fmt, adx, flags, ttyp)
	register char *fmt;
	register u_int *adx;
	struct tty *ttyp;
{
	register int b, c, i;
	char *s;
	int any;

loop:
	while ((c = *fmt++) != '%') {
		if (c == '\0')
			return;
		putchar(c, flags, ttyp);
	}
again:
	c = *fmt++;
	/* THIS CODE IS VAX DEPENDENT IN HANDLING %l? AND %c */
	switch (c) {

	case 'l':
		goto again;
	case 'x': case 'X':
		b = 16;
		goto number;
	case 'd': case 'D':
	case 'u':		/* what a joke */
		b = 10;
		goto number;
	case 'o': case 'O':
		b = 8;
number:
		printn((u_long)*adx, b, flags, ttyp);
		break;
	case 'c':
		b = *adx;
		for (i = 24; i >= 0; i -= 8)
			if (c = (b >> i) & 0x7f)
				putchar(c, flags, ttyp);
		break;
	case 'b':
		b = *adx++;
		s = (char *)*adx;
		printn((u_long)b, *s++, flags, ttyp);
		any = 0;
		if (b) {
			while (i = *s++) {
#if	CS_GENERIC
				if (*s <= 32) {
					register int j;

					if (any++)
						putchar(',', flags, ttyp);
					j = *s++ ;
					for (; (c = *s) > 32 ; s++)
						putchar(c, flags, ttyp);
					printn( (u_long)( (b >> (j-1)) &
							 ( (2 << (i-j)) -1)),
						 8, flags, ttyp);
				} else if (b & (1 << (i-1))) {
#else	CS_GENERIC
				if (b & (1 << (i-1))) {
#endif	CS_GENERIC
					putchar(any? ',' : '<', flags, ttyp);
					any = 1;
					for (; (c = *s) > 32; s++)
						putchar(c, flags, ttyp);
				} else
					for (; *s > 32; s++)
						;
			}
#if	CS_BUGFIX
#else	CS_BUGFIX
			if (any)
#endif	CS_BUGFIX
				putchar('>', flags, ttyp);
		}
		break;

	case 's':
		s = (char *)*adx;
		while (c = *s++)
			putchar(c, flags, ttyp);
		break;

	case '%':
		putchar('%', flags, ttyp);
		break;
	}
	adx++;
	goto loop;
}

/*
 * Printn prints a number n in base b.
 * We don't use recursion to avoid deep kernel stacks.
 */
printn(n, b, flags, ttyp)
	u_long n;
	struct tty *ttyp;
{
	char prbuf[11];
	register char *cp;

	if (b == 10 && (int)n < 0) {
		putchar('-', flags, ttyp);
		n = (unsigned)(-(int)n);
	}
	cp = prbuf;
	do {
		*cp++ = "0123456789abcdef"[n%b];
		n /= b;
	} while (n);
	do
		putchar(*--cp, flags, ttyp);
	while (cp > prbuf);
}

/*
 * Panic is called on unresolvable fatal errors.
 * It prints "panic: mesg", and then reboots.
 * If we are called twice, then we avoid trying to
 * sync the disks as this often leads to recursive panics.
 */
panic(s)
	char *s;
{
	int bootopt = RB_AUTOBOOT;

	if (panicstr)
		bootopt |= RB_NOSYNC;
	else {
		panicstr = s;
	}
	printf("panic: %s\n", s);
#if	CS_KDB
#ifdef	vax
	if (boothowto&RB_KDB)
	    asm("bpt");
#endif	vax
#endif	CS_KDB
#if	defined(RDB) && defined(romp)
	Debugger("panic");
	printf("panic: calling boot...");
#endif	RDB && romp
#if	CS_GENERIC
	printf("K:  1-1A\n");
	printf("Sp: 1-1A-2B\n");
	printf("Sc: 1B-2B-3\n");
	printf("K:  0-0-0-Destruct-0\n");
#endif	CS_GENERIC
	boot(RB_PANIC, bootopt);
}

/*
 * Warn that a system table is full.
 */
tablefull(tab)
	char *tab;
{

#if	CS_GENERIC
	printf("%s: table is full\n", tab);
#endif	CS_GENERIC
	log(LOG_ERR, "%s: table is full\n", tab);
}

/*
 * Hard error is the preface to plaintive error messages
 * about failing disk transfers.
 */
harderr(bp, cp)
	struct buf *bp;
	char *cp;
{

	printf("%s%d%c: hard error sn%d ", cp,
	    minor(bp->b_dev) >> 3, 'a'+(minor(bp->b_dev)&07), bp->b_blkno);
}

/*
 * Print a character on console or users terminal.
 * If destination is console then the last MSGBUFS characters
 * are saved in msgbuf for inspection later.
 */
/*ARGSUSED*/
putchar(c, flags, tp)
	register int c;
	struct tty *tp;
{
	extern struct tty cons;

	if (flags & TOTTY) {
		if (tp == (struct tty *)NULL && (flags & TOCONS) == 0)
			tp = &cons;
		if (tp && (tp->t_state & TS_CARR_ON)) {
			register s = spl6();
			if (c == '\n')
				(void) ttyoutput('\r', tp);
			(void) ttyoutput(c, tp);
			ttstart(tp);
			splx(s);
		}
	}
	if ((flags & TOLOG) && c != '\0' && c != '\r' && c != 0177
#ifdef vax
	    && mfpr(MAPEN)
#endif
	    ) {
		if (msgbuf.msg_magic != MSG_MAGIC) {
			register int i;

			msgbuf.msg_magic = MSG_MAGIC;
			msgbuf.msg_bufx = msgbuf.msg_bufr = 0;
			for (i=0; i < MSG_BSIZE; i++)
				msgbuf.msg_bufc[i] = 0;
		}
		if (msgbuf.msg_bufx < 0 || msgbuf.msg_bufx >= MSG_BSIZE)
			msgbuf.msg_bufx = 0;
		msgbuf.msg_bufc[msgbuf.msg_bufx++] = c;
	}
	if ((flags & TOCONS) && c != '\0')
		cnputc(c);
}
