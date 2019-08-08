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

#ifndef lint
static	char	*sccsid = "@(#)qvcons.c	1.5 4/26/85 (decvax!rjl)";
#endif lint

/************************************************************************
 *									*
 *			Copyright (c) 1984 by				*
 *		Digital Equipment Corporation, Maynard, MA		*
 *			All rights reserved.				*
 *									*
 *   This software is furnished under a license and may be used and	*
 *   copied  only  in accordance with the terms of such license and	*
 *   with the  inclusion  of  the  above  copyright  notice.   This	*
 *   software  or  any  other copies thereof may not be provided or	*
 *   otherwise made available to any other person.  No title to and	*
 *   ownership of the software is hereby transferred.			*
 *									*
 *   This software is  derived  from  software  received  from  the	*
 *   University    of   California,   Berkeley,   and   from   Bell	*
 *   Laboratories.  Use, duplication, or disclosure is  subject  to	*
 *   restrictions  under  license  agreements  with  University  of	*
 *   California and with AT&T.						*
 *									*
 *   The information in this software is subject to change  without	*
 *   notice  and should not be construed as a commitment by Digital	*
 *   Equipment Corporation.						*
 *									*
 *   Digital assumes no responsibility for the use  or  reliability	*
 *   of its software on equipment which is not supplied by Digital.	*
 *									*
 ************************************************************************/

/* ---------------------------------------------------------------------
 * Modification History 
 *
 *  7 Jul 84 --  rjl
 *	Initial version to support the qvss as the system console
 *	during the boot process.
 *
 * ---------------------------------------------------------------------
 */

#if NQV <= 0	/* CMU	CS_QVSS */

#ifdef	CMU
/* CS_VAXM */
int vgetc() {return(-1);}
int vputc() {}
int qv_init() {}
/* CS_VAXM */
#endif	CMU

#else	NQV	/* CMU	CS_QVSS */

#include "../h/types.h"
#include "../vaxuba/qvioctl.h"
#include "../vaxuba/qvfont.c"
#include "../vax/cpu.h"

#ifdef	CMU
/* CS_VAXM */
#define CS_CONS 1
#ifdef	VAXM
#define	MVAX	VAXM
#define	MVAX_II	VAX_MII
#endif	VAXM

#define	qvgetc	vgetc
#define	qvputc	vputc
/* CS_VAXM */
#endif	CMU


/*
 * MicroVAX-II q-bus memory base
 */
#define QMEMBASE 0x30000000
#define QVMAXEVQ	64

#define LED_ON 0x13
#define LED_OFF 0x11
#define LED_3 0x84
#define BELL 0xa7

/*
 * Screen initialization tables. qv_def_scn is used as an index into the
 * table to select the proper initialization parameters.
 */
int qv_def_scn = 1;			/* Screen initialization flag	*/

char	qv_scrn_15[]= {
	31,25,27,0142,31,13,30,31,4,15,040,0,0,0,0,0
};

char	qv_scrn_19s[]= {
	39,30,31,0264,55,5,54,54,4,15,040,0,0,0,0,0
};

char	*qv_init_tbl[]= {
	qv_scrn_15,
	qv_scrn_19s,
};

struct qv_info qv_scn_defaults[] = {
	{0, {0, 0}, 0, {0, 0}, 0, 0, 30, 80, 768, 480, 768-16, 480-16,
	 0, 0, 0, 0, 0, QVMAXEVQ, 0, 0, {0, 0}, {0, 0, 0, 0}},
	{0, {0, 0}, 0, {0, 0}, 0, 0, 55, 120, 960, 864, 960-16, 864-16,
	 0, 0, 0, 0, 0, QVMAXEVQ, 0, 0, {0, 0}, {0, 0, 0, 0}}
};

struct qv_info  qv_scn;

struct qv_keyboard {
	int shift;			/* state variables	*/
	int cntrl;
	int lock;
	char last;			/* last character	*/
} qv_keyboard;


qv_console=0;				/* qvss console flag	*/

/*
 * Routine called to init a qvss.
 */
qv_init(qvaddr)
	struct qvdevice *qvaddr;
{
	char *qvssmem;
	short *scanline;
	int i;
	short scan;
	char *ptr;
	extern int cpu;

        if( qvaddr->qv_csr & QV_19INCH )
                qv_def_scn = 1;
        else
                qv_def_scn = 0;
        qv_scn = qv_scn_defaults[ qv_def_scn ];
	qv_scn.qvaddr = qvaddr;

	/*
	 * Initialize the screen.
	 */
	ptr = qv_init_tbl[ qv_def_scn ];
	for( i=0 ; i<16 ; i++ ) {
		qvaddr->qv_crtaddr = i;
		qvaddr->qv_crtdata = *ptr++;
	}

	/*
	 * Turn on the keyboard. 
	 */
	qvaddr->qv_uartcmd = 0x15;	/* set mode pntr/enable rx/tx	*/
	qvaddr->qv_uartmode = 0x17;	/* noparity, 8-bit		*/
	qvaddr->qv_uartmode = 0x07;	/* 1 stop bit			*/
	qvaddr->qv_uartstatus = 0x99;	/* 4800 baud xmit/recv 		*/

	/*
	 * Size the screen and setup screen structure.
	 */
	if( qvaddr->qv_csr & QV_19INCH ) {
		qv_scn.max_x = 960;
		qv_scn.max_y = 864;
		qv_scn.max_col = 120;
		qv_scn.max_row = 80;
		qv_scn.max_cur_x = 960-16;
		qv_scn.max_cur_y = 864-16;
	}
	qvssmem = (char *)((qvaddr->qv_csr & QV_MEM_BANK) << 7);
	if( cpu == MVAX_II )
		qvssmem += QMEMBASE;

	qv_scn.bitmap = qvssmem;
	qv_scn.scanmap = (short *)((int)qvssmem + ( 254 * 1024 ));
	qv_scn.cursorbits = (short *)((int)qvssmem + ( 256 * 1024 ) - 32);

	/*
	 * Setup the cursor.
	 */
	for( i=0 ; i<16 ; i++ )
		qv_scn.cursorbits[i] = qv_cursor[i];

	/*
	 * Clear the bit map
	 */
	for( i=0 , ptr = qv_scn.bitmap ; i<254 ; i += 2 , ptr += 2048)
		bzero( ptr, 2048 );
	
	/*
	 * Reinitialize the scanmap
	 */
	scan = qv_scn.qvaddr->qv_csr & QV_MEM_BANK;
	scanline = qv_scn.scanmap;
	for(i = 0 ; i < qv_scn.max_y ; i++ )
		*scanline++ = scan++;

	/*
	 * Home the cursor
	 */
	qv_scn.row = qv_scn.col = 0;

	/*
	 * Turn it on.
	 */
	qv_console = 1;
	qvaddr->qv_csr |= QV_CUR_MODE | QV_VIDEO_ENA;
}

/*
 * Routine to display a character on the screen.  The model used is a 
 * glass tty.  It is assummed that the user will only use this emulation
 * during system boot and that the screen will be eventually controlled
 * by a window manager.
 */
qvputc( c )
char c;
{

	char *b_row, *f_row;
	int i, j;
	short *scanline;

#if	CS_CONS
	if (qv_console == 0)
		return;
#endif	CS_CONS
	c &= 0x7f;

	switch ( c ) {
	case '\t':				/* tab		*/
		for( j = 8 - (qv_scn.col & 0x7) ; j > 0 ; j-- )
			qvputc( ' ' );
		break;

	case '\r':				/* return	*/
		qv_scn.col = 0;
		break;

	case '\010':				/* backspace	*/
		if( --qv_scn.col < 0 )
			qv_scn.col = 0;
		break;

	case '\n':				/* linefeed	*/
		if( qv_scn.row+1 >= qv_scn.max_row )
			qvscroll();
		else
			qv_scn.row++;
		break;

	case '\007':				/* bell		*/
		if( qv_scn.qvaddr )
			qv_key_out( BELL );
		return;

	default:
		if( c >= ' ' && c <= '~' ) {
			scanline = qv_scn.scanmap;
			b_row = qv_scn.bitmap+(scanline[qv_scn.row*15]&0x3ff)*128+qv_scn.col;
			i = c - ' ';
			if( i < 0 || i > 95 )
				i = 0;
			else
				i *= 15;
			f_row = (char *)((int)qv_font + i);
		
			for( i=0 ; i<15 ; i++ , b_row += 128, f_row++ )
				*b_row = *f_row;

			if( ++qv_scn.col >= qv_scn.max_col ) {
				qv_scn.col = 0 ;
				if( qv_scn.row+1 >= qv_scn.max_row )
					qvscroll();
				else
					qv_scn.row++;
			}
		}
		break;
	}
	/*
	 * Position the cursor to the next character location.
	 */
	qv_pos_cur( qv_scn.col*8, qv_scn.row*15 );
}

/*
 * Position the cursor to a particular spot.
 */
qv_pos_cur( x, y)
int x,y;
{
	struct qvdevice *qvaddr;

	if( qvaddr = qv_scn.qvaddr ) {
		if( y < 0 || y > qv_scn.max_cur_y )
			y = qv_scn.max_cur_y;
		if( x < 0 || x > qv_scn.max_cur_x )
			x = qv_scn.max_cur_x;

		qvaddr->qv_crtaddr = 10;	/* select cursor start reg */
		qvaddr->qv_crtdata = y & 0xf;
		qvaddr->qv_crtaddr = 11;	/* select cursor end reg */
		qvaddr->qv_crtdata = y & 0xf;
		qvaddr->qv_crtaddr = 14;	/* select cursor y pos. */
		qvaddr->qv_crtdata = y >> 4;
		qvaddr->qv_xcur = x;		/* pos x axis	*/
	}
}
/*
 * Scroll the bitmap by moving the scanline map words. This could
 * be done by moving the bitmap but it's much too slow for a full screen.
 * The only drawback is that the scanline map must be reset when the user 
 * wants to do graphics.
 */
qvscroll()
{
	int i;
	short tmpscanlines[15];
	char *b_row;
	short *scanline;


	/*
	 * Save the first 15 scanlines so that we can put them at
	 * the bottom when done.
	 */
	bcopy( qv_scn.scanmap, tmpscanlines, sizeof tmpscanlines );

	/*
	 * Clear the wrapping line so that it won't flash on the bottom
	 * of the screen.
	 */
	scanline = qv_scn.scanmap;
	b_row = qv_scn.bitmap+(*scanline&0x3ff)*128;
	bzero( b_row, 1280 );

	/*
	 * Now move the scanlines down 
	 */
	bcopy( qv_scn.scanmap+15, qv_scn.scanmap, (qv_scn.row * 15) * sizeof (short) );

	/*
	 * Now put the other lines back
	 */
	bcopy( tmpscanlines, qv_scn.scanmap+(qv_scn.row * 15), sizeof tmpscanlines );

}

/*
 * QVSS keyboard interrupt.
 */
qvgetc()
{
	int c;
	struct qvdevice *qvaddr;
	char *string;
	int j;

	qvaddr = qv_scn.qvaddr;
	/*
	 * Get a character from the keyboard.
	 */
loop:
	while( (qvaddr->qv_uartstatus & 0x01) == 0 )
#ifdef	CMU
		/* CS_CONS */
		return(-1)
		/* CS_CONS */
#endif	CMU
		;
	j = qvaddr->qv_uartdata & 0xff;
	/*
	 * See if its a state change key
	 */
	switch ( j ) {
	case LOCK:
		qv_keyboard.lock ^= 0xffff;	/* toggle */
		if( qv_keyboard.lock )
			qv_key_out( LED_ON );
		else
			qv_key_out( LED_OFF );
		qv_key_out( LED_3 );
		goto loop;
	case SHIFT:
		qv_keyboard.shift ^= 0xffff;
		goto loop;
	case CNTRL:
		qv_keyboard.cntrl ^= 0xffff;
		goto loop;
	case ALLUP:
		qv_keyboard.cntrl = qv_keyboard.shift = 0;
		goto loop;
	case REPEAT:
		c = qv_keyboard.last;
		break;
	default:
		/*
		 * Test for control characters. If set, see if the character
		 * is elligible to become a control character.
		 */
		if( qv_keyboard.cntrl ) {
			c = qv_key[ j ];
			if( c >= ' ' && c <= '~' )
				c &= 0x1f;
		} else if( qv_keyboard.lock || qv_keyboard.shift )
			c = qv_shift_key[ j ];
		else
			c = qv_key[ j ];
		break;
	}

	qv_keyboard.last = c;

	/*
	 * Check for special function keys
	 */
	if( c & 0x80 ) 
		return 0;
	else
		return c;
}

/*
 * Output to the keyboard. This routine status polls the transmitter on the
 * keyboard to output a code. The timer is to avoid hanging on a bad device.
 */
qv_key_out( c )
char c;
{
	int timer = 30000;

	if( qv_scn.qvaddr ) {
		while( (qv_scn.qvaddr->qv_uartstatus & 0x4) == 0  && timer-- )
			;
		qv_scn.qvaddr->qv_uartdata = c;
	}
}
#endif	NQV	/* CMU	CS_QVSS */
