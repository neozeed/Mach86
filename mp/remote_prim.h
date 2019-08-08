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
 *	File:	remote_prim.h
 *
 *	Copyright (C) 1984, Avadis Tevanian, Jr.
 *
 *	Machine independent header file for processor-to-processor
 *	communications at the lowest level.  (Just above the hardware)
 *
 * HISTORY
 * 20-Sep-84  Avadis Tevanian (avie) at Carnegie-Mellon University
 *	Created.
 *
 */

/*	number of bytes to use for communication area */

#define MSG_SPACE 1024*2

/*
 * Struct msg_buffer MUST have a length that is a multiple of 8 bytes,
 * This is so that consecutive buffers are quad-word aligned.
 */

struct msg_buffer {
	struct	mpqueue msg_q;		/* queue information */
	short	to;			/* destination processor */
	short	from;			/* source processor */
	long	type;			/* message type */
	long	param1;			/* 4 parameters */
	long	param2;
	long	param3;
	long	param4;
};

#define MP_MSG_MAGIC	14470		/* magic number for message passing */
