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
#include "mono.h"

#if NMONO > 0

/************************************************************************/
/*									*/
/*	Routines to Handle the MONOcrome Screen (memory mapped).	*/
/* We implement a simple termcap for now in software.			*/
/*									*/
/************************************************************************/

#define IBM3101
#define LOCAL static
#define screen_put mono_screen_putc	  /* kludge */

#include "../machine/mono_tcap.h"
/* $Header: mono.c,v 4.3 85/08/31 13:03:48 webb Exp $ */
/* $Source: /ibm/acis/usr/sys_ca/ca/RCS/mono.c,v $ */

#ifndef lint
#endif

/*
 * Output Driver for a PC MONOCROME screen adapter on a SAILBOAT.
 * designed for standalone use.
 * currently only single character control characters or the single letter
 * ESCAPE sequences of the IBM3101 terminal.
 * the goal - minimal support needed for Vi - later we can support proper ANSI
 * escape sequences.
 * the following functions are supported:
 * CR
 * LF - with scrolling
 * clear screen
 * erase to end of line
 * erase to end of screen
 * inverse video
 * status line on/off
 *
 */


#ifdef SBPROTO
#define SCREEN_BUFFER (char *)  0xf10b0000
#endif

#ifdef SBMODEL
#define SCREEN_BUFFER (char *)  0xf40b0000
#endif

#define SCREEN_END	(SCREEN_BUFFER + SCREEN_SIZE)

#define NORMAL_VIDEO 0x07
#define REVERSE_VIDEO 0x70
#define UNDERLINE_VIDEO 0x01	       /* under line */
#define HI_INTENSITY	0x08	       /* High Intensity */
#define BLINK		0x80	       /* blink bit */

#define SCREEN_WIDTH 80
#define SCREEN_LENGTH 25
#define STATUS_LINE (SCREEN_LENGTH-1)  /* line to put status on */
#define SCREEN_SIZE (SCREEN_LENGTH * SCREEN_WIDTH * 2)
#define set_ptr() screen_ptr = lines[screen_y] + (screen_x<<1)	/* where we are */
#define blank_line(line) screen_blank(line,0,SCREEN_WIDTH)	/* blank a line */

#if !defined(LF_DELAY)
#define LF_DELAY 0		       /* by default no mono_delay required */
#endif

#if !defined(LP_LOG)
#define LP_LOG 0		       /* by default no loggin on line printer */
#endif

char *screen_buffer = SCREEN_BUFFER;
int screen_size = SCREEN_SIZE;
static lpstatus = -1;
int print_log = LP_LOG;		       /* log screen output */
LOCAL lf_mono_delay = LF_DELAY;	       /* for slowing down the output */
LOCAL char *lines[SCREEN_LENGTH + 1] = {
	0
};				       /* pointer to start of lines */
LOCAL int screen_x = 0,		       /* screen column 0=start of line */
screen_y = 0;			       /* screen line 0 = first line */
LOCAL char *screen_ptr = 0;
LOCAL char screen_attr = NORMAL_VIDEO, screen_reverse = REVERSE_VIDEO;
static int old_x, old_y;
int monopagemode;		       /* non-scroll mode */
#define PAGEEOLCLR	0x02	       /* if we are to clear to eol in page mode */
#define PAGEMODE	0x01	       /* if in page mode */

int screen_length = SCREEN_LENGTH;     /* allow for status line */
#define move_block bcopy
 /* #define rmove_block bcopy		/* removed as bcopy does not work */


#ifdef SBMODEL
#define DELAY_COUNT 1000
#define DELAY_ADDRESS 0x80E0	       /* dummy write I/O mono_delay */
#else
#define DELAY_COUNT 500
#endif

int mono_delay_count = DELAY_COUNT;

mono_delay(n)
	register int n;
{
	register int i;

	while (--n >= 0)
		for (i = mono_delay_count; --i >= 0;)
#ifdef SBMODEL
			out(DELAY_ADDRESS, 0xFF) /* use hardware if we have it */
#endif
			;
}


mono_screen_init()
{
	register char *p = SCREEN_BUFFER;
	register int i;

	screen_ptr = p;
	/* note the <=. This so that lines[screen_length] is the end of the buffer */
	for (i = 0; i <= SCREEN_LENGTH; ++i) {
		lines[i] = p;
		p += SCREEN_WIDTH << 1;
	}
	screen_length = SCREEN_LENGTH; /* reserve it */
	screen_clear();		       /* clear entire screen */
	screen_length = SCREEN_LENGTH - 1; /* reserve it */
}


LOCAL
screen_clear()
{
	screen_blank(0, 0, (lines[screen_length] - SCREEN_BUFFER) >> 1);
	screen_x = screen_y = 0;
	set_ptr();
	screen_cursor();	       /* put cursor on right spot */
}


/* internal version of mono_putc - used to output cr and nl distinctly */
#define NORMAL 0
#define CURSOR_Y	1
#define CURSOR_X	2
#define ESC_1		3	       /* first letter of escape sequence */

mono_screen_putc(c)
	register int c;
{
	static put_state;	       /* internal state for cursor motion etc. */
	static cursor_base;	       /* base for cursor motion */
	register int temp;

	if (screen_ptr == 0)
		mono_screen_init();

	switch (put_state) {
	case CURSOR_Y:
		c -= cursor_base;      /* less the offset */
		screen_y = min(c, SCREEN_LENGTH);
		put_state = CURSOR_X;
		break;
	case CURSOR_X:		       /* was 1 now 0 */
		c -= cursor_base;      /* less the offset */
		screen_x = min(c, SCREEN_WIDTH);
		put_state = NORMAL;
		set_ptr();
		break;
#ifdef ESC_FOUND
	case ESC_1:
		put_state = NORMAL;    /* revert to normal mode */
		c |= ESC_FOUND;
		/* fall thru */
#endif
	default:		       /* NORMAL state */
		switch (c) {
		case SH_CHAR:
			if (screen_length == SCREEN_LENGTH)
				screen_length--;
			else
				screen_length++;
			blank_line(SCREEN_LENGTH - 1);
			break;
#ifdef SO_CHAR
		case SO_CHAR:
			screen_attr = REVERSE_VIDEO;
			screen_reverse = NORMAL_VIDEO;
			break;
		case SE_CHAR:
			screen_reverse = REVERSE_VIDEO;
			screen_attr = NORMAL_VIDEO;
			break;
#endif SO_CHAR
		case HO_CHAR:
			screen_x = screen_y = 0;
			set_ptr();
			break;
#ifdef US_CHAR
		case US_CHAR:
			screen_attr |= UNDERLINE_VIDEO;
			break;
		case UE_CHAR:
			screen_attr &= ~UNDERLINE_VIDEO;
			break;
#endif
#ifdef HI_CHAR
		case HI_CHAR:
			screen_attr |= HI_INTENSITY;
			break;
		case LO_CHAR:
			screen_attr &= ~HI_INTENSITY;
			break;
#endif HI_CHAR
		case UP_CHAR:	       /* index */
			screen_index();
			break;
		case 07:	       /* bell */
#ifdef SBMODEL
			beep();	       /* ring bell */
#endif SBMODEL
			break;
		case '\n':
			screen_lf();
			break;
		case '\t':
			screen_tab();
			return;	       /* cursor updated already */
		case '\r':
			screen_cr();
			break;
		case '\b':
			screen_bs();
			break;
#ifdef ESC_FOUND
		case 033:	       /* if emulating a terminal with ESC sequences */
			put_state = ESC_1;
			break;
#endif
		case CL_CHAR:
			screen_clear();
			break;
		case ND_CHAR:
			screen_ptr += 2;
			if (++screen_x >= SCREEN_WIDTH) {
				screen_cr();
				screen_lf();
			}
			break;
#ifdef CM_CHAR			       /* primary cursor is directly generated */
		case CM_CHAR:
			put_state = CURSOR_Y; /* number of cursor characters */
			cursor_base = 0; /* no base value */
			break;
#endif

#ifdef CM_CHAR2			       /* secondary cursor addressing uses an offset */
		case CM_CHAR2:
			put_state = CURSOR_Y; /* number of cursor characters */
			cursor_base = CURSOR_OFFSET; /* blank is base value */
			break;
#endif

		case CD_CHAR:	       /* clear to end of screen */
			screen_blank(screen_y, screen_x, (lines[screen_length] - screen_ptr) >> 1);
			break;

		case CE_CHAR:	       /* clear to end of line */
			screen_blank(screen_y, screen_x, SCREEN_WIDTH - screen_x);
			break;

		case 0:
			return;	       /* ignore nulls (fast) */
#ifdef SAVE_CM
		case SAVE_CM:	       /* save current cursor */
			old_x = screen_x;
			old_y = screen_y;
			break;
		case RESTORE_CM:       /* restore saved cursor */
			screen_x = old_x;
			screen_y = old_y;
			set_ptr();
			break;
#endif SAVE_CM
#ifdef IGN_CHAR
		case IGN_CHAR:
			put_state = ESC_1; /* ignore it */
			break;
#endif

#ifdef AL_CHAR:
			/* 
			 * insert a line (blank) by moving the current line and following lines
			 * down the screen.
			 */
		case AL_CHAR:	       /* insert line */
			if (screen_y < screen_length - 1)
				screen_move(screen_y, screen_length - 2, screen_y + 1);
			/* move the lines down */
			blank_line(screen_y); /* blank current line */
			break;
		case DL_CHAR:	       /* delete line */
			screen_move(screen_y + 1, screen_length - 1, screen_y);
			/* move lines up */
			blank_line(screen_length - 1); /* blank last line */
			break;
#endif
		default:
#ifdef ESC_FOUND
			if ((c & ESC_FOUND) == 0)
#endif
			screen_out(c); /* normal character */
			break;
		}
		if (print_log && lp_put(c)) /* log to printer */
			print_log = 0; /* error - stop logging */
	}
	screen_cursor();	       /* put cursor on right spot */
}


LOCAL
screen_out(c)
	register c;
{
	/*
	 * output the given character at the current screen position and then
	 * update the pointer to the new position.
	 */
	*screen_ptr++ = c;
	*screen_ptr++ = screen_attr;
	if (++screen_x >= SCREEN_WIDTH) {
		screen_cr();
		screen_lf();
	}
}


/*
 * do a line feed on the screen.
 * two cases:
 * 1. we are at the bottom of the screen
 * 2. we are not
 * we will do a scroll if we are at the bottom, and then fall thru to
 * the other case.
 */
LOCAL
screen_lf()
{
	if (++screen_y >= screen_length) {
		if (monopagemode & PAGEMODE)
			screen_y = 0;
		else {
			screen_y = screen_length - 1;
			screen_move(1, screen_length - 1, 0); /* move the lines */
			blank_line(screen_length - 1);
		}
	}
	set_ptr();
	if (lf_mono_delay)
		mono_delay(lf_mono_delay);       /* mono_delay for readability */
}


LOCAL
screen_index()
{
	if (--screen_y < 0) {
		screen_move(0, screen_length - 2, 1); /* move the lines */
		blank_line(0);
		screen_y = 0;
	}
	set_ptr();
}


/*
 * blank the given line (line), starting at the given position (position)
 * giving proper number of blanks (width)
 */
LOCAL
screen_blank(line, position, width)
	register int line, position, width;
{
	register short *ptr = (short *)(lines[line] + (position << 1)), *end = ptr + width;
#define fill line

	fill = (' ' << 8) + screen_attr;
	fill = (fill << 16) + fill;
	if ((int)ptr & 02) {
		*ptr++ = fill;	       /* align to word bdy */
		--width;
	}
	while ((width -= 8) >= 0) {
		register int *wptr = (int *)ptr;

		wptr[0] = fill;
		wptr[1] = fill;
		wptr[2] = fill;
		wptr[3] = fill;
		ptr = (short *)(wptr + 4);
	}
	while (ptr < end)
		*ptr++ = fill;
}


#undef fill


/*
 * go to start of line 
 * and do a clear to eol if we are in page mode 
 */
LOCAL
screen_cr()
{
	if (monopagemode & PAGEEOLCLR)
		screen_blank(screen_y, screen_x, SCREEN_WIDTH - screen_x);
	screen_x = 0;
	set_ptr();
}


LOCAL
screen_bs()
{
	if (--screen_x < 0) {
		screen_x = SCREEN_WIDTH - 1;
		if (--screen_y < 0)
			screen_y = screen_length - 1;
	}
	set_ptr();
}


LOCAL
screen_tab()
{
	register int n = 8 - (screen_x & 07);

	while (--n >= 0)
		mono_screen_putc(ND_CHAR);     /* non destructive tab */
}


/*
 * move line1 ... line2 to dest
 */
LOCAL
screen_move(line1, line2, dest)
	register int line1, line2, dest;
{
	register char *start = lines[line1], *end = lines[line2 + 1];
	if (line1 > dest)
		move_block(start, lines[dest], end - start);
	else if (line1 != dest)
		rmove_block(start, lines[dest], end - start);
}


#ifndef move_block
LOCAL
move_block(from, to, length)
	register int *from, *to, length;
{
	length >>= 2;		       /* get as int's */
	while (--length >= 0)
		*to++ = *from++;
}


#endif

#ifndef rmove_block
LOCAL
rmove_block(from, to, length)
	register int *from, *to, length;
{
	from = (int *)(((int)from) + length);
	to = (int *)(((int)to) + length);
	length >>= 2;		       /* get as int's */
	while (--length >= 0)
		*--to = *--from;
}


#endif

LOCAL
screen_cursor()
{
	register int x = (screen_ptr - SCREEN_BUFFER) >> 1;

	PUT_SCR_REG(14, x >> 8);
	PUT_SCR_REG(15, x);
}


mono_put_status(offset, string)
	register char *string;
{
	register x = screen_x, y = screen_y;
	register char *ptr = screen_ptr;

	if (screen_length == SCREEN_LENGTH)
		return;		       /* no status display */

	screen_x = offset;
	screen_y = STATUS_LINE;
	set_ptr();
	while (*string)
		screen_out(*string++);
	screen_x = x;
	screen_y = y;
	screen_ptr = ptr;
}


/*
 * screen printing functions:
 * flag == 0	print the current screen contents
 * flag != 0	invert the log output flag
 */
mono_screen_print(flag)
	register int flag;
{
	register int l, i;
	register char *p = SCREEN_BUFFER;
	register int x = screen_x, y = screen_y;

	if (flag) {
		print_log = !print_log;
		return;
	}
	/* print out the monochrome screen buffer on the printer */

	for (l = 0; l < SCREEN_LENGTH; ++l) {
		p = lines[l];
		for (i = 0; i < SCREEN_WIDTH; ++i, p += 2) {
			screen_x = i;
			screen_y = l;
			set_ptr();
			screen_cursor();
			if (lp_put(p[0])) {
				mono_delay(250);
				goto done; /* quit if error */
			}
		}
		lp_put('\r');
		lp_put('\n');
	}
	lp_put('\f');		       /* skip to top of form */
done:
	screen_x = x;
	screen_y = y;
	set_ptr();
	screen_cursor();
	mono_put_status(20, "        ");
	lpstatus = -1;
}


/*
 * simple non-interrupt drive line printer output routine
 */

#define OUT_DATA     0x3bc	       /* Data register    */
#define OUT_CMD      0x3be	       /* Command register */
#define IN_STAT      0x3bd	       /* status  register */
#define STROB_HIGH   0x0d	       /* strobe high      */
#define STROB_LOW    0x0c	       /* strobe low       */
#define RESET_LOW    0x8	       /* reset line LOW   */
#define RESET_HIGH   0x0c	       /* no interrupt, no auto LF */

#define LP_NOTBUSY	0x80
#define LP_ACK		0x40
#define LP_NOPAPER	0x20
#define LP_SELECT	0x10
#define LP_OK		0x08
#define LP_TIMEOUT 	0x01

#define LP_BITS	LP_NOTBUSY+LP_ACK+LP_NOPAPER+LP_SELECT+LP_OK

#define LP_DELAY 500000
#define LP_INVERT LP_ACK + LP_OK + LP_SELECT

LOCAL int lp_mono_delay = LP_DELAY;

LOCAL lp_status(n)
	register int n;
{
	register int i = ((in(IN_STAT) & LP_BITS) ^ LP_INVERT) | n;
	static char msg[10] = "ERROR";

	if (i != lpstatus) {
#ifdef STANDALONE
		sprintf(msg, "ERROR%02X", i);
#endif
		mono_put_status(20, i ? (n ? "TIMEOUT" : msg) : "       ");
	}
	return (lpstatus = i);
}


LOCAL lp_put(c)
	register int c;
{
	register int i;
	static char first_time = 0;

	if (c == ND_CHAR)
		c = ' ';	       /* so that tabs will work */
	if (first_time == 0) {
		first_time++;
		lp_reset();
	}
	/* reset */
	for (i = 0; i < lp_mono_delay; i++) { /* time out loop */
		if ((in(IN_STAT) & 0x80) != 0) {
			out(OUT_DATA, c); /* output character */
			i = i;
			i = i;	       /* at least .5 us */
			out(OUT_CMD, STROB_HIGH); /* hardware handshake */
			i = i;
			i = i;	       /* at least .5 us */
			out(OUT_CMD, STROB_LOW);
			i = i;
			i = i;	       /* at least .5 us */
			return (lp_status(0));
		}
	}
	return (lp_status(1));	       /* timed out */

}


LOCAL lp_reset()
{
	int i;
	out(OUT_CMD, RESET_LOW);
	mono_delay(2);
	out(OUT_CMD, RESET_HIGH);
	mono_delay(200);
	lpstatus = -1;
	lp_put(0);
	return (lp_status(0));
}
#endif NMONO
