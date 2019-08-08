/*
 * Copyright (c) 1982, 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)dkio.h	7.1 (Berkeley) 6/5/86
 */

/*
 * Structures and definitions for disk io control commands
 *
 * THIS WHOLE AREA NEEDS MORE THOUGHT.  FOR NOW JUST IMPLEMENT
 * ENOUGH TO READ AND WRITE HEADERS ON MASSBUS DISKS.  EVENTUALLY
 * SHOULD BE ABLE TO DETERMINE DRIVE TYPE AND DO OTHER GOOD STUFF.
 */

/* disk io control commands */
#define DKIOCHDR	_IO(d, 1)	/* next I/O will read/write header */
