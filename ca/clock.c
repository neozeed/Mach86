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
/* $Header: clock.c,v 4.0 85/07/15 00:40:31 ibmacis GAMMA $ */
/* $Source: /ibm/acis/usr/sys_ca/ca/RCS/clock.c,v $ */

#ifndef lint
static char *rcsid = "$Header: clock.c,v 4.0 85/07/15 00:40:31 ibmacis GAMMA $";
#endif

/*     clock.c 6.1     83/07/29        */

#include "../h/param.h"
#include "../h/time.h"
#include "../h/kernel.h"
#include "../machine/clock.h"
#include "../machine/scr.h"

#ifdef SBMODEL

#define XCLOCK_SEC	0xf0008800
#define XCLOCK_SAL	0xf0008801
#define XCLOCK_MIN	0xf0008802
#define XCLOCK_MAL	0xf0008803
#define XCLOCK_HRS	0xf0008804
#define XCLOCK_HAL	0xf0008805
#define XCLOCK_DOW	0xf0008806
#define XCLOCK_DOM	0xf0008807
#define XCLOCK_MON	0xf0008808
#define XCLOCK_YR	0xf0008809
#define XCLOCK_A	0xf000880A
#define XCLOCK_B	0xf000880B
#define XCLOCK_C	0xf000880C
#define XCLOCK_D	0xf000880D

struct XCLOCK {
	unsigned char sec;
	unsigned char sal;
	unsigned char min;
	unsigned char mal;
	unsigned char hrs;
	unsigned char hal;
	unsigned char dow;
	unsigned char dom;
	unsigned char mon;
	unsigned char year;
	unsigned char a;
	unsigned char b;
	unsigned char c;
	unsigned char d;
} *xclock = (struct XCLOCK *)XCLOCK_SEC;

/* Reset the clock XLOCK_A */
#define XCLOCK_RS	0x26
 /* Start the external clock for timer XCLOCK_B */
#define XCLOCK_SET	0x80		  /* 0 - Update normal, 1 - Halt and init */
#define XCLOCK_PIE	0x40		  /* 0/1 disable/enable periodic interrupt */
#define XCLOCK_AIE	0x20		  /* 0/1 disable/enable alarm interrupt */
#define XCLOCK_UIE	0x10		  /* 0/1 disable/enable update-ended interrupt */
#define XCLOCK_SQWE	0x08		  /* 0/1 disable/enable sqware-wave signal */
#define XCLOCK_DM	0x04		  /* data mode, 0 - BCD, 1 - Binary */
#define XCLOCK_24HR	0x02		  /* 0 - 12 hour mode, 1 - 24 hour mode */
#define XCLOCK_DSE	0x01		  /* 0/1 diable/enable daylight savings */
#endif SBMODEL

/*
 * Machine-dependent clock routines.
 *
 * Startrtclock restarts the real-time clock, which provides
 * hardclock interrupts to kern_clock.c.
 *
 * Inittodr initializes the time of day hardware which provides
 * date functions.  Its primary function is to use some file
 * system information in case the hardare clock lost state.
 *
 * Resettodr restores the time of day hardware after a time change.
 */

static unsigned timer_base;

/*
 * Start the real-time clock.
 */
startrtclock()
{
	if (tick * hz != 1000000)
		panic("1000000 % hz not 0");

	timer_base = ROMPHZ / hz;	  /* # of timer counts to short timer int */

#ifdef SBMODEL
	xclock->a = XCLOCK_RS;
	xclock->b = XCLOCK_SQWE | XCLOCK_DM | XCLOCK_24HR | XCLOCK_DSE;
#endif SBMODEL

	mtsr(SCR_TIMER, timer_base);	  /* start with a short tick */
	mtsr(SCR_TIMER_RESET, timer_base); /* 1st tick is a guess */
	mtsr(SCR_TIMER_STATUS, TS_ENABLE | TS_ILEVEL);
}


/*
 * This routine is called hz times per second (ie every hard clock interrupt).
 * Note that setting SCR_TIMER_RESET on the Nth call per second, determines the
 * duration of the (N+2)th tick for that second.  This routine behaves as if it
 * set the duration of the (N+1)th tick.  This does not cause a problem as long
 * as the number of short ticks per second is correct.
 */


/*
 * Initialze the time of day structure.
 */
inittodr(base)
	time_t base;
{
	register unsigned i;
	int secpast69 = 0;

	{
#ifdef CLOCKDEBUG
		printf("yr (%d) mon (%d) dom (%d) hrs (%d) min (%d) sec (%d)\n",
		    xclock->year, xclock->mon, xclock->dom, xclock->hrs,
		    xclock->min, xclock->sec);
#endif CLOCKDEBUG

		for (i = 70; i < xclock->year; i++)
			secpast69 += SECYR + (LEAPYEAR(i) ? SECDAY : 0);

		for (i = 1; i < xclock->mon; i++) {
			adjustmonth(i, &secpast69, xclock->year, 1);
		}
		secpast69 += SECDAY * (xclock->dom - 1);
		secpast69 += SECHR * xclock->hrs;
		secpast69 += SECMIN * xclock->min;
		secpast69 += xclock->sec;
	}
	time.tv_sec = secpast69;
	time.tv_usec = 0;

	if ((secpast69 < base) || (secpast69 > base + (SECDAY * 3)))
		printf("PREPOSTEROUS DATE,MAY BE WRONG -- CHECK AND RESET!\n");
}


/*
 * Reset the TODR based on the time value.  Used when:
 *   - the time is reset by the stime system call
 *   - on Jan 2 just after midnight
 */
resettodr()
{
	int s69 = time.tv_sec;
	register int i;

	xclock->sal = xclock->mal = xclock->hal = xclock->dow = 0;

#ifdef CLOCKDEBUG
	printf("s69 (%d) - ", s69);
#endif CLOCKDEBUG
	for (i = 70; s69 > SECYR + (LEAPYEAR(i) ? SECDAY : 0); i++)
		s69 -= SECYR + (LEAPYEAR(i) ? SECDAY : 0);

	/* Tell the MC146818 that we are going to reset it */
	*(unsigned char *)XCLOCK_B |= XCLOCK_SET;

	/* Set the year on the time of day clock */
	xclock->year = i;

/*
 * find the month by going past it (which will set s69 < 0)
 * and then backing off that extra month.
 */
	for (i = 1; s69 > 0; i++) {
		adjustmonth(i, &s69, xclock->year, 0);
	}
	/* Set the Month on time of day clock */
	xclock->mon = --i;
	adjustmonth(i, &s69, xclock->year, 1);

	/* Set the Day of Month on time of day clock */
/*	printf("s69=%d s69/SECDAY=%d ",s69,s69/SECDAY);	/* debug */
	xclock->dom = (s69 / SECDAY) + 1;

	s69 %= SECDAY;
	/* Set the Hours on the time of day clock */
	xclock->hrs = s69 / (SECHR);

	s69 %= SECHR;
	/* Set the Minutes on the time of day clock */
	xclock->min = s69 / (SECMIN);

	s69 %= SECMIN;
	/* Set the SEONDS on the time of day clock */
	xclock->sec = s69;

	/* Tell the MC146818 that we are done with reset */
	*(unsigned char *)XCLOCK_B &= ~XCLOCK_SET;
#ifdef CLOCKDEBUG
	printf("yr (%d) mon (%d) dom (%d) hrs (%d) min (%d) sec (%d)\n",
	    xclock->year, xclock->mon, xclock->dom, xclock->hrs,
	    xclock->min, xclock->sec);
#endif CLOCKDEBUG
	printf("TODR reset !\n");
}


/*
 * add/subtrace number of seconds in month 'i' to/from *s69.
 */
adjustmonth(i, s69, year, add)
	register int i;
	register int *s69;
	register int year;
	register int add;
{
	register int secs;

	switch (i) {
	case 1:				  /* Jan */
	case 3:				  /* Mar */
	case 5:				  /* May */
	case 7:				  /* Jul */
	case 8:				  /* Aug */
	case 10:			  /* Oct */
	case 12:			  /* Dec */
		secs = 31 * SECDAY;
		break;
	case 4:				  /* Apr */
	case 6:				  /* Jun */
	case 9:				  /* Sep */
	case 11:			  /* Nov */
		secs = 30 * SECDAY;
		break;
	case 2:				  /* Feb */
		secs = 28 * SECDAY + (LEAPYEAR(year) ? SECDAY : 0);
		break;
	}
	if (add)
		*s69 += secs;
	else
		*s69 -= secs;
}


/*  Kernel profiling system call routine to:
 *
 *  - Turn kernel profiling on or off
 *  - Reset the kernel profiling counters
 *  - Read the kernel profiling counters
 */
kernprof(parm1, parm2)
{
	extern int kpf_onoff;
	register int i;

	switch (parm1) {
	case 0:				  /* off */
		timer_base = ROMPHZ / hz;
		mtsr(SCR_TIMER, timer_base);
		mtsr(SCR_TIMER_RESET, timer_base);
		mtsr(SCR_TIMER_STATUS, TS_ENABLE | TS_ILEVEL);

		kpf_onoff = 0;
		break;
	case 1:				  /* on */
		i = 10 * hz;		  /* speed up clock 10x */
		timer_base = ROMPHZ / i;
		mtsr(SCR_TIMER, timer_base);
		mtsr(SCR_TIMER_RESET, timer_base);
		mtsr(SCR_TIMER_STATUS, TS_ENABLE); /* level 0 */

		kpf_onoff = 1;
		break;
	case 2:				  /* reset */
		break;
	case 3:				  /* read */
		break;
	}
}
