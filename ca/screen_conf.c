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
#include "../caio/screen_conf.h"

/*
 * IMPORTANT NOTE:
 * the same source is used for both kernel and standalone - use care
 * to keep in sync.
 */

#ifdef KERNEL
#include "../machine/io.h"	       /* for DISPLAY */
#endif KERNEL

int nodev();

#include "mono.h"
#if NMONO > 0
int mono_screen_init();
int mono_screen_putc();
int mono_screen_print();
int mono_put_status();
#else
#define mono_screen_init nodev
#define mono_screen_putc nodev
#define mono_screen_print nodev
#define mono_put_status nodev
#endif NMONO

#include "aed.h"
#if NAED > 0
int aed_screen_init();
int aed_screen_putc();
int aed_screen_print();
int aed_put_status();
#else
#define	aed_screen_init	nodev
#define	aed_screen_putc	nodev
#define aed_screen_print nodev
#define aed_put_status nodev
#endif NAED

#include "apaetty.h"
#if NAPAETTY > 0
int apa8_screen_init();
int apa8_screen_putc();
int apa8_put_status();
int apa8_screen_print();
#else
#define	apa8_screen_init nodev
#define	apa8_screen_putc nodev
#define apa8_put_status nodev
#define apa8_screen_print nodev
#endif NAPATTY

#define apa16_screen_init nodev
#define apa16_screen_putc nodev
#define apa16_put_status nodev
#define apa16_screen_print nodev

int which_screen = -1;			  /* use whatever is available */
int screen_lines;
int nulldev();				  /* a dummy routine */

#define NSCREEN 5			  /* number of entries in following table */
struct screen_sw screen_sw[NSCREEN] = {
	/* ENTRY	INIT     	PUTC	 	RWADDR    lines flags    status    print             name */
	{  /* 0 */ aed_screen_init, aed_screen_putc, C 0xf40a4020, 52, 0 , aed_put_status, aed_screen_print, "AED"},
	{  /* 1 */ mono_screen_init, mono_screen_putc, C 0xf40b0000, 24, 0, mono_put_status, mono_screen_print, "MONOCHROME"},
	{  /* 2 */ apa8_screen_init, apa8_screen_putc, C 0x00000000, 24, 0, apa8_put_status, apa8_screen_print, "APA8"},
	{  /* 3 */ apa16_screen_init, apa16_screen_putc, C 0x00000000, 24, 0, apa16_put_status, apa16_screen_print, "APA16" },
	{  /* 4 */ nulldev, nulldev, C 0x00000000, 24, 0, nulldev, nulldev, "DUMMY" }
};

int screen_inited;			  /* must be in BSS! */

#ifdef SBMODEL
#define AED_MMAP	0xf40a0000
#define AED_DELAY	delay(2)
#else SBPROTO
#define AED_MMAP	0xf10a0000
#define AED_DELAY
#endif SBPROTO

#define aedrd(X,Y,Z) bcopy(AED_MMAP+Z, X, Y+Y)
#define aedwr(X,Y,Z) bcopy(X, (AED_MMAP+Z), Y+Y)

#define TEST_PAT 0x65

screen_init()
{
	register int i;
	register char *rwaddr;
	short poll;

	/* read control store to reset AED */
	aedrd(&poll, 1, 0X0000);
	AED_DELAY;

	for (i = 0; i < NSCREEN; i++) {
/*		DISPLAY(i);		/* debugging */
		if (screen_sw[i].init && screen_sw[i].init != nodev
				&& (rwaddr = screen_sw[i].rwaddr) != 0) {
			*rwaddr = TEST_PAT;
			delay(3);
			if (*rwaddr == TEST_PAT) {
				screen_sw[i].flags |= SCREEN_ALIVE;
				(*screen_sw[i].init)();
				screen_sw[i].flags |= SCREEN_INIT;
				if (which_screen < 0)
					which_screen = i;
			}
			delay(3);
		}
	}
/*
 * if we haven't made a choice, or if our choice does not exist then
 * look for any alive screen.
 */
	if (which_screen < 0 || (screen_sw[which_screen].flags & SCREEN_ALIVE) == 0) {
		which_screen = -1;	  /* just in case */
		for (i = 0; i < NSCREEN; i++)
			if (screen_sw[i].flags & SCREEN_ALIVE) {
				which_screen = i;
				break;
			}
	}
	if (which_screen < 0) {
		DISPLAY(0x99);		  /* no screen on system! */
		delay(1000);
		which_screen = CONS_DUMMY; /* keep going anyway */
		screen_sw[CONS_DUMMY].flags |= SCREEN_ALIVE;
	}
	screen_lines = screen_sw[which_screen].lines;
	delay(3);
	screen_inited = 1;

/*
 * loop thru the other screens and print message for the screen we are
 * going to use just in case the user is looking at the wrong one.
 */
	for (i = 0; i < NSCREEN; i++) {
		if (i != which_screen && (screen_sw[i].flags&SCREEN_ALIVE)) {
			register int save = which_screen;
			which_screen = i;
			printf("Console is %s screen\n", screen_sw[save].name);
			which_screen = save;
		}
	}
	init_kbd();
}


/*
 * switch to screen "n" or to next screen (if n < 0)
 */
switch_screen(n)
	register int n;
{
	register int i;

	if (n >= NSCREEN)
		return (-1);		  /* can't do it */
	if (n == SCREEN_SWITCH || n == SCREEN_SWITCH_RELOAD) {
		for (i = 0; i < NSCREEN; ++i) {
			if (++which_screen >= NSCREEN)
				which_screen = 0;
			if (screen_sw[which_screen].flags & SCREEN_ALIVE) {
				if (n == SCREEN_SWITCH_RELOAD)
					screen_sw[which_screen].flags &= ~SCREEN_INIT;
				break;
			}
		}
	} else if (n < 0)
		return(-1);		/* not valid request */
	else {
		if (screen_sw[n].flags & SCREEN_ALIVE)
			which_screen = n;
		else
			return (-1);
	}
	screen_lines = screen_sw[which_screen].lines;
/*
 * screen_init can be cleared to force an init which (might) force
 * a micro-code reload
 */
	if ((screen_sw[which_screen].flags & SCREEN_INIT) == 0) {
		(*screen_sw[which_screen].init)();
		screen_sw[which_screen].flags |= SCREEN_INIT;
	}
	return (which_screen);
}

/*
 * put up a status indicator from the keyboard
 * don't atempt it if the device is not initialized 
 */
put_status(pos,str)
	register int pos;
	register char *str;
{
	if (screen_sw[which_screen].flags & SCREEN_INIT)
		(*screen_sw[which_screen].putstatus)(pos,str);
}

/*
 * print the current screen (if possible)
 */
print_screen(flag)
	register int flag;
{
	(*screen_sw[which_screen].printscreen)(flag);
}


/*
 * reset the screen (by forcing a microcode reload etc. next time it is used
 * mainly used to reload aed microcode.
 */

reset_screen(n)
{
screen_sw[n].flags &= SCREEN_INIT;
}
