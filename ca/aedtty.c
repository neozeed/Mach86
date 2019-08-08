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
#include "aed.h"
#if NAED > 0
#include "../machine/param.h"
#include "../machine/aed_tty.h"
#include "../machine/aed_tty_mcode.h"
#ifdef	VIRTUE
#include "../machineio/aeddefs.h"
#endif

aed_screen_init ()
{
#ifndef AED_SET
	register int len;
	unsigned short aedcmd[10];
	short poll;		   /* semaphore word */

#ifdef	VIRTUE
	/*
	 * XXX - this means that panics while the wm is running
	 * will be a bit inscrutable,  but c'est la vie.......
	 */
	extern AED aed_info;

	if (aed_info.state & AED_WM_MODE)
		return;
#endif
	/* read control store to reset AED */
	aedrd(&poll,1,0X0000);

	/* copy microcode file into control store */
	aedwr(aed_tty_mcode,AEDTTY_MC_LEN/2,0x0000); 

	/* send a null command to the AED to start and wait for ready */
	aedrd(&poll,1,0X0002);
	aed_tty_command(0, 0);

#endif AED_SET

	/* Initialize AED sram pointers */
	vinit();
}


aed_tty_command(buff, nwords)
register char *buff;		/* Buffer to give to AED */
register short nwords;		/* number of words to transfer */

{
	short poll;		   /* semaphore word */

	/* copy the AED buffer to shared ram */
	aedwr(buff,nwords,0X4002);

	/* Wake up microcode */
	poll = 1;
	aedwr(&poll,1,0X4000);

	/* wait for completion */
	do {
		AED_DELAY;
		aedrd(&poll,1,0X4000);
	}
	while (poll != 0);

}
char aedbuff[4];		/* buffered escape sequences */
int aedstate;
#define AED_NORMAL	0
#define AED_ESC		4	/* read ESC */
#define AED_ESC1	1	/* read next 1 character */
#define AED_ESC2	2	/* read next 2 character */

/*
 * output a character to the AED
 * we look at the characters passing thru to see if 
 * they are a complete escape sequence - we only send it
 * when it is complete to protect the status line
 */
aed_screen_putc(c)
register char c;
{
	register int count;
	register int i;
	register int s;

	vwait();		/* wait for previous operation to complete at low priority */
	switch(aedstate)
		{
	case AED_NORMAL:
		if (c == '\033')
			{
			aedstate = AED_ESC;
			aedbuff[0] = c;
			return;
			}
		else if (c == 07)
			{
			beep();		/* ring the bell */
			return;
			}
		vterm(c);
		return;
	case AED_ESC:
		aedbuff[1] = c;
		switch(c)
			{
		case 'Y':
			aedstate = AED_ESC1;
			return;		/* need 2 more characters */
		case 'G':
			aedbuff[0] = 07;
			count = 1 ;
			break;
		default:
			count = 2;
			break;
			}
		break;
	case AED_ESC1:
		aedbuff[2] = c;
		aedstate = AED_ESC2;
		return;
	case AED_ESC2:
		aedbuff[3] = c;
		count = 4;
		break;
	default:
		aedstate = AED_NORMAL;
		return;
		}
		
/* inhibit interrupts so that escape sequence can be output in one piece */
	s = spl5();
	for (i=0; i<count; ++i)
		vterm(aedbuff[i]);
	splx(s);
	aedstate = AED_NORMAL;

}

aed_put_status(pos,str)
	register int pos;
	register char *str;
{
	register int c;

	vterm('\033');		/* send out ESC */
	vterm('4');		/* 	     4  */
	vterm(pos+' ');		/* column position plus offset */

	do
		{
		c = *str++;
		vterm(c);
		}
	while (c);


}

aed_screen_print()
{
}

#endif NAEDTTY
