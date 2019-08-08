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
/*	pty.h	CMU	6/4/80	*/

/*
 **********************************************************************
 * HISTORY
 * 15-May-85  Mike Accetta (mja) at Carnegie-Mellon University
 *	Upgraded to 4.2BSD.  Fixed bug which omitted ATTACHFLG from
 *	ptynread() macro.
 *	[V1(1)]
 *
 * 19-Nov-84  Mike Accetta (mja) at Carnegie-Mellon University
 *	Added new definitions to support detaching of pseudo-
 *	terminals: ATTACHMSG, and ATTACHFLG to implement new control
 *	message, PTYLOGGEDIN, PTYDETACHED, and PTYDETHUP to record this
 *	status for a pseudo terminal, and new pt_ldev field in the
 *	control structure as a back link to the index of the logical
 *	control file mapping table; added new PIOCMGET definition
 *	(V3.07l).
 *
 * 10-May-83  Mike Accetta (mja) at Carnegie-Mellon University
 *	Added ptioctlbuf_t definition for the ioctl reply buffer (V3.06j).
 *
 * 28-Mar-83  Mike Accetta (mja) at Carnegie-Mellon University
 *	Removed defunct PIOCGLOC definition;  added new PIOCSLOC definition
 *	(V3.06h).
 *
 * 10-Aug-82  Mike Accetta (mja) at Carnegie-Mellon University
 *	Added PIOCMBIS and PIOCMBIC definitions, new PTYNOBLOCK,
 *	PTYNEWSIG and PTYHOLDSIG mode bits and PTYPRIVMODES definitions;
 *	made the non-privileged mode bit definitions accessible outside
 *	of the KERNEL conditional (V3.05e).
 *
 * 10-Jul-82  David Nichols (nichols) at Carnegie-Mellon University
 *	Changed ptynread() macro to check tp->t_state as well as
 *	tp->t_outq.c_cc to tell if a WRITEMSG would be given to a
 *	reader of the control pty (V3.05c).
 *
 * 07-Jul-82  Mike Accetta (mja) at Carnegie-Mellon University
 *	Added PIOCGLOC call (currently unused by kernel) for returning
 *	terminal connection location information from servers running
 *	pseudo terminals (V3.05c).
 *
 * 29-Jun-82  Mike Accetta (mja) at Carnegie-Mellon University
 *	Added fields for signal processing to control structure, PIOCENBS
 *	definition and ptynread() macro to test if input is pending on
 *	the control pty; increased NPTY from 20 to 26 (V3.05b).
 *
 * 15-Oct-80  Mike Accetta (mja) at Carnegie-Mellon University
 *	Added message types and structure definitions for multiple control and
 *	new ioctl fetatures (V1.09h).
 *
 * 03-Jul-80  Mike Accetta (mja) at Carnegie-Mellon University
 *	Created (V1.08).
 *
 **********************************************************************
 */

#ifdef	KERNEL
#include "../h/ioctl.h"	
#include "../h/ttyloc.h"	
#else	KERNEL
#include <sys/ioctl.h>
#include <sys/ttyloc.h>
#endif	KERNEL
 
#undef	NPTY
#define	NPTY	32
#define	NPTYL	5		/* maximum of 16 (4-bit mask) */

#define	OPENMSG	  1
#define	CLOSEMSG  2
#define	STATEMSG  3
#define	IOCTLMSG  4
#define	WRITEMSG  5
#define	CCMSG	  6
#define	ATTACHMSG 7

#define	OPENREPLY  50
#define	CLOSEREPLY 51
#define	IOCTLREPLY 52

#define	IOCTLDATA  75
#define	READDATA   76

struct ptymsg
{
    unsigned char pt_msg;		/* message identifier */
    unsigned char pt_arg;		/* message argument */
    unsigned char pt_aux;		/* auxiliary message argument */
    unsigned char pt_line;		/* line generating message */
};

struct ptyreply
{
    unsigned char pt_reply;		/* reply identifier */
    unsigned char pt_error;		/* reply error code */
    unsigned char pt_len;		/* reply non-data length */
    unsigned char pt_line;		/* line to receive reply */
};


#define	PTYLBITS	(4)		/* bits in ioctl line */
#define	PTYLMASK	(017)		/* PTYLBITS of line mask */

/*
 *  Note: The PTYLBITS definition above limits the number of PTY
 *  ioctl calls to 16 (the remaining 4 bits in the low byte of
 *  the code).
 */
#define	PIOCXIOC	_IO  (P, ( 0<<PTYLBITS))
#define	PIOCSEOF	_IO  (P, ( 1<<PTYLBITS))
#define	PIOCSSIG	_IOW (P, ( 3<<PTYLBITS), int)
#define	PIOCCONN	_IOR (P, ( 4<<PTYLBITS), struct ptymsg)
#define	PIOCSIM		_IO  (P, ( 5<<PTYLBITS))	/* obsolete */
#define	PIOCNOSIM	_IO  (P, ( 6<<PTYLBITS))	/* obsolete */
#define	PIOCCCMSG	_IO  (P, ( 7<<PTYLBITS))	/* obsolete */
#define	PIOCNOCCMSG	_IO  (P, ( 8<<PTYLBITS))	/* obsolete */
#define	PIOCENBS	_IOW (P, ( 9<<PTYLBITS), int)
#define	PIOCMBIS	_IOW (P, (10<<PTYLBITS), int)
#define	PIOCMBIC	_IOW (P, (11<<PTYLBITS), int)
#define	PIOCSLOC	_IOW (P, (12<<PTYLBITS), struct ttyloc)
#define	PIOCMGET	_IOR (P, (13<<PTYLBITS), int)

#ifdef	KERNEL
typedef struct sgttyb	ptioctlbuf_t;

struct ptyctrl
{
    int pt_state;			/* pty control state bits */
    int pt_ostate;			/* previous tty state bits */
    int pt_orcc;			/* previous raw character count */
    char pt_openbuf;			/* open reply buffer */
    char pt_closebuf;			/* close reply buffer */
    short pt_buflen;			/* ioctl reply length */
    int pt_cmdbuf;			/* ioctl cmd word (must be here) */
    ptioctlbuf_t pt_ioctlbuf;		/* ioctl reply buffer */
    struct ptyctrl *pt_cpty;		/* master multiplex pty */
    struct ptyctrl *pt_ctrl[NPTYL];	/* multiplex line pointers */
    struct proc *pt_sigp;		/* signal process pointer */ 
    u_short pt_pid;			/* PID of signal process */
    u_short pt_sign;			/* signal number to send */
    unsigned char pt_next;		/* next line for multiplex read */
    unsigned char pt_high;		/* highest line assigned */
    dev_t pt_mdev;			/* device number of pty */
    dev_t pt_ldev;			/* device number of control file */
};

#define	OPENFLG	    0x1			/* waiting for open reply */
#define	OPENINPROG  0x2			/* open in progress */
#define	CLOSEFLG    0x4			/* waiting for close reply */
#define	CLOSEINPROG 0x8			/* close in progress */
#define	IOCTLFLG    0x10		/* waiting for ioctl reply */
#define	IOCTLINPROG 0x20		/* ioctl in progress */
#define	CCFLG	    0x40		/* character count */
#define	ATTACHFLG   0x80		/* newly attached */
#endif	KERNEL
#define	PTYDETHUP   0x200000		/* send HANGUP on detach condition */
#define	PTYLOGGEDIN 0x400000		/* application terminal "logged-in" */
#define	PTYDETACHED 0x800000		/* application terminal "detached" */
#define	PTYNEWSIG   0x1000000		/* return 1 byte on new signals */
#define	PTYHOLDSIG  0x2000000		/* keep signal enabled after sending */
#define	PTYNOBLOCK  0x4000000		/* do not block on control reads */
#define	PTYCCMSG    0x8000000		/* generate character count messages */
#define	PTYSIM	    0x10000000		/* force read to fill buffer */
#ifdef	KERNEL
#define	PTYEOF	    0x20000000		/* force read to return EOF */
#define OUTPUTWAIT  0x40000000		/* pty output wait */
#define	PTYINUSE    0x80000000		/* pty control in use */

/*
 *  State (mode) bit masks.
 *
 *  RW (read/write) can be examined and changed by the control process.
 *  RO (read-only) can only be examined by the control process.
 */
#define	PTYRWMODES (PTYNEWSIG|PTYHOLDSIG|PTYNOBLOCK|PTYCCMSG|PTYSIM|PTYDETHUP)
#define	PTYROMODES (PTYLOGGEDIN)


#define	ptynread(cp, tp) \
    ((cp)->pt_state&(ATTACHFLG|OPENFLG|IOCTLFLG|CLOSEFLG|CCFLG) || \
     (tp)->t_state != (cp)->pt_ostate || \
     ((tp)->t_outq.c_cc > 0 && !((tp)->t_state&(TS_TIMEOUT|TS_BUSY|TS_TTSTOP))))
#endif	KERNEL
