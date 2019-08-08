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
 * 5799-CGZ (C) COPYRIGHT IBM CORPORATION 1986
 * LICENSED MATERIALS - PROPERTY OF IBM
 * REFER TO COPYRIGHT INSTRUCTIONS FORM NUMBER G120-2083
 */
/* $Header: apvar.h,v 5.2 86/02/25 21:46:58 katherin Exp $ */
/* $Source: /ibm/acis/usr/sys_ca/caio/RCS/apvar.h,v $ */


/*
 * Major compilation parameters in most versions are:
 * 1. which compiler/system (MICROC, PCIX, XENIX, BERK)
 * 2. debugging flags APDEBUG and APIDEBUG
 * 3. number of ports opened simultaneously (NUMPORTS)
 * 4. maximum legal device number (MAXDEV)
 *
 * In the line discipline version of this driver, most of the above
 * parameters are no used.
 */


/* #define APDEBUG */  /* debug trace info */
#ifdef APDEBUG
#define APIDEBUG /* interupt debug trace info */
#endif


#ifdef APIDEBUG
#define iqtrc(s) qtrc(s)      /* queued trace call */
#define iqtrcx(s,i) qtrcx(s,i) /* queued trace with hex call */
#else
#define iqtrc(s)		/* maps to nothing if not interrupt debug */
#define iqtrcx(s,i)		/* maps to nothing if not interrupt debug */
#endif

#ifdef APDEBUG
#define SLOWIT	/* run real low baud rate */
extern qtpdm(); 		/* queued trace routine */
#define qtrc(s) qtpdm(a->devno,s)	/* queued trace call */
extern qtpdmx();		/* queued trace with hex int */
#define qtrcx(s,i) qtpdmx(a->devno,s,i) /* queued trace with hex call */
#else
#define qtrc(s) 		/* maps to nothing if not debug */
#define qtrcx(s,i)		/* maps to nothing if not debug */
#endif
/*
 * Includes and declarations of a general nature.
 */

typedef unsigned char byte;

typedef byte Boolean;

/*#define TRUE	1
  #define FALSE	0
 */

/*
 * handy characters
 */
#define NULLCHAR '\0'

/*
 * Definitions of ap driver "extern"s, to allow separate compilation.
 */

#include "../caio/apdefs.h"
#include "../h/param.h"
#include "../h/conf.h"
#include "../h/dir.h"
#include "../h/user.h"
#include "../h/proc.h"
#include "../h/ioctl.h"
#include "../h/tty.h"
#include "../h/systm.h"
#include "../h/uio.h"
#include "../h/file.h"
#include "../ca/debug.h"
#include "../h/buf.h"
#include "../caio/asyregs.h"

#define output(port,c) (asyadr->port=(c))
#define WRTBUF ASY_TXB
#define LNCTL ASY_LCR

#define reterrno(val) return(val)
#define lockintrs spl5()
#define unlockintrs(m) (void)splx(m)
#define usercount (uio->uio_resid)
typedef caddr_t useraddr;
#define getubyte(ka,ua) (*(ka)=fubyte(ua))
#define putubyte(ua,ka) subyte(ua,*(ka))
#define getwrtdata(buf,cnt) uiomove(buf,cnt,UIO_WRITE,uio)
#define putrddata(buf,cnt) uiomove(buf,cnt,UIO_READ,uio)
#define retstatus(ua,st) (((struct apioinfo *)ua)->apstatus = (st))
#define retcmd(ua,cmd) (((struct apioinfo *)ua)->apcmd = (cmd))
#define getcmd(ka,ua) (*(ka)=(((struct apioinfo *)ua)->apcmd))
#define tty2apstr(tp) (struct apstr *)(tp->t_bufp->b_un.b_addr)
#define getwo(f) (FALSE) /*temporary!!!*/
/*
 *!!! For now we define getwo, get write only, as FALSE, since
 * 4.2 does not pass the read and write flags to a line discipline.
 * We should be able to see it when the TIOCSETD ioctl calls apopen
 * or when a real open comes to apopen (which may not be possible).
 */

#define initxmtcnt (a->xmtcnt = 0)
#define rsxmtcnt (a->xmtcnt = 0)
#define rsxmtbuf (a->xmtbuf = a->xmtbufsv)
#define svxmtbuf (a->xmtbufsv = a->xmtbuf)
/*
 *
 * Driver and Protocol specific declarations and definitions.
 *
 * Conventions regarding names:
 * Rdxxx and wrtxxx are read and write at the user level.
 *   This refers primarily to read and write operations (and ioctl command
 *   read and write) at the user level, and to buffers shared between
 *   the user level and the interupt level for messages sent and received.
 * Rcvxxx and xmtxxx are receive and transmit at the interupt level.
 *   This refers primarily to character by character operations to send and
 *   receive breaks and messages, whatever their contents.
 * Sndxxx refers to protocol information and a finite state machine at the
 *   "send protocol" level, which is primarily driven by events such as
 *   arrival of timeouts, messages from the others side, and user level
 *   operations on this side. The main purpose is to determine what to send
 *   next, if anything.
 */

/*
 * read operation substates - values of a->rdopstate
 */
#define RDNOINFO 0	/* no data in read buffer or received command */
#define RDGOTDATA 1	/* data is in read buffer */
#define RDGOTCMD 2	/* command has been received from other side */

/*
 * write operation substates - values of a-> wrtopstate
 */
#define WRTIDLE 0	/* no user data or command specified */
#define WRTWAIT 1	/* user data or command specified; no free ptr buf */
#define WRTBUSY 2	/* sending user data or command to ptr buffer */
#define WRTDONE 3	/* data or cmd was sent; see if more to send */

/*
 * sndstate values represent the device driver send protocol state.
 * They are related to the transmission states (xmtstate) and the
 * write operations states (wrtopstate). sndstate also reflects
 * the overall state of the driver since the states fall into
 * four classes: "driver closed", "trying to synchronize with 3812",
 * "running normally, i.e. open", and "shutting down".
 *
 * There are three fields in the sndstate values:
 *   - overall state refered to above, extractable by DVMASK.
 *   - transmission state, extractable by MSGMASK:
 *	corresponds to whether the state is transmitting or not
 *	and if it is transmitting, what sort of message it is.
 *   - the subgroup code, extractable by GRPMASK:
 *	If a pair of states have the same group number, they
 *	are closely related.
 *	Normally, if they are the same number, one is a state where
 *	a message is transmitting, and the other is a wait state that
 *	is entered after transmission of that message.
 */

#define DVMASK		0x03	/* defines major state class */
#define dvstate (a->sndstate & DVMASK)	/* extracts major state class */
/* The major states classes are: */
#define DVCLOSED	0x00	/* driver is closed */
#define DVSYNCING	0x01	/* driver is trying to sync with 3812 */
#define DVOPEN		0x02	/* driver is open for business with printer */
#define DVUNSYNCED	0x03	/* driver is shutting down */

#define MSGMASK 	0x0C	/* defines what is being transmitted */
#define msgtype (a->sndstate & MSGMASK)  /* extracts transmission info */
#define NOMSG		0x00	/* nothing being transmitted right now */
#define NORMALMSG	0x04	/* command, data, or "tickle" (yoohoo) msg */
#define PRMSG		0x08	/* pacing receipt message */
#define SQMSG		0x0C	/* sequence reset message */

/*
 * There is no encoding of group codes with mnemonics now because they are
 * not useful, at least so far. GRPMASK is defined only for completeness.
 */
#define GRPMASK 	0xF0	/* defines group code field */

/*	name of state	group	msg, if any	major state class */
#define SNDCLOSED	(0x00			|DVCLOSED)
#define SNDUNSYNCED	(0x10			|DVUNSYNCED)
#define SNDINIT		(0x20			|DVSYNCING)
#define SNDSQ		(0x30	|SQMSG		|DVSYNCING)
#define SNDSQWT 	(0x30			|DVSYNCING)
#define SNDSQPR 	(0x40	|PRMSG		|DVSYNCING)
#define SNDSQPRWT	(0x40			|DVSYNCING)
#define SNDIDLE 	(0x50			|DVOPEN)
#define SNDIDLPR	(0x50	|PRMSG		|DVOPEN)
#define SNDBLKWT	(0x60			|DVOPEN)
#define SNDBWPR 	(0x60	|PRMSG		|DVOPEN)
#define SNDYOOHOO	(0x70	|NORMALMSG	|DVOPEN)
#define SNDYHWT 	(0x80			|DVOPEN)
#define SNDYHPR 	(0x80	|PRMSG		|DVOPEN)
#define SNDMSG		(0x90	|NORMALMSG	|DVOPEN)
#define SNDACKWT	(0xA0			|DVOPEN)
#define SNDWTPR 	(0xA0	|PRMSG		|DVOPEN)

/*
 * Interupt receive states - values of a->rcvstate
 */
#define RCVIDLE  0 /* ignore all interupts ... closed */
#define RCVFIRST 1 /* look for first char - ignore initial break */
#define RCVBREAK 2 /* wait for next break before looking at chars */
#define RCVCHAR  3 /* receive chars until full data buffer or break */
#define RCVEND	 4 /* no space left in non-data buffer, wait for brk */

/*
 * Interupt transmit states - values of a->xmtstate
 */
#define XMTIDLE  0 /* not transmitting */
#define XMTFIRST 1 /* wait for hold reg empty, to begin break */
#define XMTSETON 2 /* null in THR, wait to turn on break bit */
#define XMTBREAK 3 /* transmitting break */
#define XMT1STDATA 4 /* wait to clear break bit and write first data */
#define XMTDATA  5 /* writing data */
#define XMTEND 6 /* last char in THR, wait untill it empties */
/*
 * Maximum number of retransmisstions without acknowledgement
 * before we assume the line is down. The printer bahaves in the
 * same fashion.
 */
#define MAXRETRY 10 /* max number of retrys without closing*/
/*
 * We resend a pacing receipt once per PRMAX data or command messages
 * that we send. This is a part of the error recovery we do since the
 * printer has not implemented the "tickle" (yoohoo) message for
 * recovery of lost pacing receipts.
 */
#define PRMAX 5

#define SLEEPPRI (PZERO+10) /* sleep priority, system dependent*/
#define syncchan (&(a->sndstate))	/* ops sleep here waiting for sync */
#define rdchan (&(a->rdopstate))	/* read ops wait here if DVOPEN */
#define wrtchan (&(a->wrtopstate))  /* data/cmd write waits here if DVOPEN */
#define closechan (&(a->ticksleft)) /* close waits here for interupts to die */

#define INTBUFSZ 4	/* internal non-data message size */
			/* if no data byte, it is one less than above */

#ifdef APDEBUG
#define MAXDATA 16	/* For debugging, make it a little block */
/* Note that the other side must also be using a small block ! */
#else
#define MAXDATA 512	/* maximum data per block transmitted */
#endif

#define DATABUFSZ (MAXDATA+3)	/* maximum legal message size */
#define PREFSIZE 1	/* prefix size, displacement to data */

/*
 * masks and codes for message types and message fields
 */
#define SEQMASK 0x0F	/* sequence number mask */
#define PREFMASK 0xF0
#define SEQRESET 0xF0
#define SR2ND 0x0F
#define PACINGRCT 0x50
#define DATAMSG 0x00
#define COMMANDMSG 0xA0
#define MOD16MASK 0x000F

/*
 * Storechar(c) stores c in the current read buffer and leaves a
 * value that spaceleft can use for a test.
 */
#define storechar(c)  (*(a->rcvbuf++) = c, ++a->rcvcnt)

/*
 * spaceleft(storechar(c)) stores a char and tests to see if any space
 * is left in the current read buffer, returning TRUE if space is left
 * and FALSE otherwise.
 */
#define spaceleft(cnt)	((cnt) < a->rcvcmax)

/*
 * initrcvbuf - set up read buffer and counts
 */
#define initrcvbuf(p,s) (a->rcvbuf = p, a->rcvcnt = 0, a->rcvcmax = s)

/*
 * nqpacing -
 *   enqueue on the send queue that a pacing receipt is to be sent
 */
#define nqpacing(a) (a->sendpr = TRUE, apstxmt(a))
/*
 * The apstr structure is the main container of state information about
 * the line driver, and there is one per port. It is normally addressed
 * via a pointer from a (local) variable a. Thus a->rcvcnt accesses the
 * rcvcnt field of the structure pointed to by a.
 * We pass the pointer everywhere, and keep it in a register variable.
 */

struct apstr
{

/*
 * For the line discipline version, we keep a pointer to a struct tty,
 * and in in turn points to this structure (see the #define tty2apstr
 * above in this file).
 */
struct tty * ttyp;	/* pointer to the tty structure */

/* device info */
#ifdef APDEBUG
short devno;	/* to identify port for debugging */
/* such numbers are assigned 0...9 in order of open */
#endif

/* timer information */
int ticksleft;	     /* number of UNIX(TM) ticks left */
int fudgetime;	/* approx time in ticks of max message received */
Boolean ticking;	/* says if a timeout has been scheduled */
Boolean gottimeout;  /* flag for send protocol saying we timed out */

/* receive interupt level information */
byte *rcvbuf;	/* points to where to put next char */
short rcvcnt;	/* how many chars have been read */
short rcvcmax;	/* how many can be read into this buffer */
unsigned int rcvcksum; /* keep running check sum here */
byte rcvstate;	/* state of the read protocol, interupt level */
Boolean writeonly;   /* this says we throw away all read data */
byte rcvintbuf[INTBUFSZ];     /* The internal read buffer */

/* user and interupt level read information */
byte *rddataout; /* pointer to next byte to give to user */
short rddatacnt;	/* count of bytes left to give to user */
byte rdcmdval;	/* command value from other side for user to read */

/* transmit interupt level information */
byte *xmtbuf;	/* pointer to where to get next char */
byte * xmtbufsv; /* initial value of xmtbuf */
short xmtcnt;	/* how many have been written from this buffer */
short xmtcmax;	/* how many to write from this buffer */
byte xmtstate;	/* state of write protocol, interupt level */
byte xmtintbuf[INTBUFSZ]; /* internal write buffer */

/* user and interupt level write information */
short wrtdatacnt;      /* Amount of data in wrtdatabuf */

/* protocol specific information */
byte sndstate;	/* state of the send protocol finite state machine */
byte rdopstate; /* read operation state */
byte wrtopstate; /* write operation state */
Boolean senddata;    /* signal to send protocol to write data or command */
Boolean sendpr; /* signal to send protocol to send a pacing receipt */
Boolean gotpr;	/* signal to send protocol that we got a pacing receipt */
Boolean gotsq;	/* got sequence reset from other side */
Boolean gotdorc;  /* got data or cmd message from other side */
Boolean initialbrk;  /* is the break we are sending an initial one? */

/* other stuff */
short nullsperbreak;
short nullcount;	/* number of nulls sent for current break */

/* Higher level protocol variables. */
short myseq;	/* my currently busy or next available sequence num */
short mynblks;	/* my number of read buffers available */
short otherseq; /* other side's last used sequence number */
		/* ignore messages not otherseq+1 mod 16 */
short othernblks; /* other side's number of read blocks available */
short ackseq;	/* last of my sequence numbers acknowledged */
short retrycnt; /* times to resend without ack before saying line down */
short prretry;	/* decr 1 per data or cmd msg sent; send pr when 0 */

/*
 * if ackseq != myseq, the values inbetween have been sent but not
 * acknowledged. Outside of this range, ignore the acks. If those
 * in-between time out without an ack, we will resend them.
 */

byte rddatabuf[DATABUFSZ];	/* buffer for reading data msgs */
byte wrtdatabuf[DATABUFSZ];	/* buffer for writing data msgs */

};   /* end of struct apstr */

/*
 * various timeouts
 * A 4.2 tick is 1/64 second.
 */

/* What do I do about timeouts with 1/64 second varience?
 * A purported three ticks could actually be as short as two and average
 * 2 1/2. I actually use 4 ticks instead of 3 for the shortest timeout
 * so it will average 3 1/2, but that is okay.
 */
#ifdef SLOWIT	/* normally used only for debugging */
#define SHORTTIME 192		/* 3 seconds */
#define COMMANDTIME (2*192)	/* 6 seconds */
#define YOOHOOTIME (3*192)	/* 9 seconds */
#else
#define SHORTTIME 4	/* 3 to 4 ticks, 3 1/2 average */
#define COMMANDTIME 192 /* 3 seconds */
#define YOOHOOTIME 640	/* 10 seconds */
#endif
#define SQTIME 128	/* sequence reset resend time (two seconds) */
#define CLOSETIME 3	/* close time, two ticks, to let interupts die */
