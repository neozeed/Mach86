/*
 * Copyright (c) 1982, 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)ikreg.h	7.1 (Berkeley) 6/5/86
 */

struct ikdevice {
	short	ik_wc;			/* Unibus word count reg */
	u_short	ik_ubaddr;		/* Unibus address register */
	u_short	ik_ustat;		/* Unibus status/command reg */
	u_short	ik_data;		/* Data register */
	u_short	ik_xaddr;		/* X address in frame buffer */
	u_short	ik_yaddr;		/* Y address in frame buffer */
	u_short	ik_istat;		/* Ikonas status/command reg */
	u_short	ik_chan;		/* Channel control register */
};

#define IK_GETADDR 	IKIOGETADDR
#define IK_WAITINT 	IKIOWAITINT
#define	IKIOGETADDR	_IOR(i, 0, caddr_t)	/* get Unibus device address */
#define	IKIOWAITINT	_IO(i, 1)		/* await device interrupt */

/*
 * Unibus status/command register bits
 */

#define IK_GO		01
#define IK_IENABLE	0100
#define IK_READY	0200
#define IK_IKONAS_INTR	0100000

/*
 * Ikonas status/command register bits
 */

#define WORD32		0
#define RES512		2
#define RES1024		3
#define READ_SELECT	0
#define WRITE_MASK	010
#define WRITE_SELECT	020
#define HALFWORD	040
#define DMAENABLE	0100
#define INVISIBLE_IO	0200
#define AUTOINCREMENT	0400
#define RUN_PROCESSOR	01000
#define CLEAR		02000
#define BYTE_MODE	04000
#define FRAME_ENABLE	010000
#define PROC_ENABLE	020000
#define RED_SELECT	0
#define GREEN_SELECT	040000
#define BLUE_SELECT	0100000
#define ALPHA_SELECT	0140000

/*
 * Frame buffer controller
 */

#define FBC0		060000000
#define FBC1		062000000

#define VIEWPORT_LOC	0
#define VIEWPORT_SIZE	1
#define WINDOW_LOC	2
#define ZOOM		3
#define DISPLAY_RATE	4
#define VIDEO_CONTROL	5
#define		FORMAT_CONTROL_MASK	03
#define		CURSOR_ON		04
#define		LOW_RESOL		0
#define		HIGH_RESOL		010
#define		AUTO_CLEAR		040
#define		EXT_SYNC		0100
#define		COLOR_MAP_PAGES		0600
#define		HIGH_RESOL_SYNC		01000
#define		REPEAT_FIELD		02000
#define		PIXEL_CLOCK_RATE_MASK	077
#define CURSOR_LOC	6
#define CURSOR_SHADE	7

#define CURSOR_MAP	0400

/*
 * Color map lookup table
 */

#define CMAP0		040600000
#define CMAP1		040610000

#define CHAN_SELECT	02000

/*
 * Frame buffer memories
 */

#define MEM0		000000000
#define MEM1		004000000

/*
 * Bit-slice processor
 */

#define UMEM		040000000
#define SCRPAD		040400000
#define PROC		041200000

/*
 * Frame grabber
 */

#define FMG0		060200000
