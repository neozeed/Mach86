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
/* $Header: dkdata.h,v 4.0 85/07/15 00:41:59 ibmacis GAMMA $ */
/* $Source: /ibm/acis/usr/sys_ca/ca/RCS/dkdata.h,v $ */

#if !defined(lint) && !defined(LOCORE) && defined(RCS_HDRS)
static char *rcsiddkdata = "$Header: dkdata.h,v 4.0 85/07/15 00:41:59 ibmacis GAMMA $";
#endif


/* Macros for adding and removing I/O request from active queue */

					/* Insert buf struct in device */
					/*   active queue              */
#define ENQDKACT(bp) binsheadfree(bp, (struct buf *)&dk )

#define DEQDKACT(bp) bremfree(bp) 	/* Remove buf struct from device */
					/*   active queue                */

/* Macro to file system index from minor device number */

#define DKFS(dev) (dev & 0x07)		/* Get file system index */
					/*   (partition number)  */

/* Active queue header */

struct {                                /* thru actl must match buf struct */
	struct bufhd header;		/* Buf header */
	struct buf *actf, *actl;        /* Pntrs for device active queue */
	struct buf *nowserving;         /* Address of active buf struct */
	char   	*rb_addr;               /* Real address of I/O buffer */
	char   	*vb_addr;               /* Virtual address of I/O buffer */
	union {
		u_short lastack;	/* # of last pkt recieved correctly */
		u_short pkt_number;	/* # of last packet sent correctly */
	} un_pkt;

	char 	*getc_buffer_pntr;	/* Pointer used by getc to access */
					/*   input buffer                 */
	char 	*intr_buffer_pntr;	/* Pointer used by interrupt rtn */
					/*   to access input buffer      */
	u_char	time_out;		/* Time out indicator */
	char   	input_buffer[(3*PACKET_SIZE) + 6]; /* Input buffer for getc   */
						/*   and interrupt routines */
}dk;

#define LASTACK		dk.un_pkt.lastack
#define PKT_NUMBER	dk.un_pkt.pkt_number

/* Defines for iodebug */

#define	MIN_TRACE	0x01
#define	MAX_TRACE	0x02
#define	SHOW_RQP	0x04
#define	SHOW_PKTHD	0x08
#define	SHOW_INTR	0x10
#define	SHOW_INIT	0x20
#define	SHOW_ENQ	0x40
#define	SHOW_DEQ	0x80
