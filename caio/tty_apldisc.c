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
/* $Header: tty_apldisc.c,v 5.2 86/02/25 21:49:20 katherin Exp $ */
/* $Source: /ibm/acis/usr/sys_ca/caio/RCS/tty_apldisc.c,v $ */

/*
/*
 * This module is unique to the line discipline version of the driver.
 */

#include "../ca/ioctl.h"
#include "../h/types.h"
#include "../caio/apio.h"
#include "../caio/apvar.h"

#ifdef APDEBUG
/*
 * Identifies the port that is open, for debugging purposes.
 * This value is cycled from '0'...'9' for each new port opened.
 */
static short tracenum = 0;
static short apstrcnt = 0;	/* number of allocated apstrs */
#endif

/*!!!
 * for possibly temporary apstart routine defined later below
 */
extern apstart();

/*
 * apopen -
 *    Driver entry point for opening the device specified, if it is not
 * already open. Generally, in the line discipline version, this gets
 * called when the user program switches to this line discipline, and
 * therefore the line is already open. Also, we are naively assuming
 * that the baud rate, parity, and number of stop bits are all okay.
 */

/*
 * Warning!!! The following flag parameter is not really passed to
 * me in 4.2 (vanilla version). This is something easily fixed, but
 * we are waiting until 4.3 to see if they fixed it. For now we will
 * assume that the user should always get "O_RDWR". As a separate
 * issue, we might provide (in all versions) an ioctl to allow
 * switching to and from read/write and write only mode.
 */
apopen(dev, tp, flag)
dev_t dev;	/* major and minor device number */
register struct tty *tp;	/* pointer to tty structure */
{
register struct apstr *a;
int i;
int intrinfo;
struct buf *bp;

if (tp->t_line == APLDISC)	/* new open for us? */
   return(0);	/* no, it is already our number */
/*!!!! does the above work as a way to tell if we have
 * already begun, gotten an apstr, etc.?
 */

/* get a fresh apstr */
bp = geteblk(sizeof(struct apstr));
if (bp == NULL)
   {
   reterrno(ENOMEM);
   }
tp->t_bufp = bp;
a = tty2apstr(tp);	/* point to the actual apstr now */
a->ttyp = tp;	/* and I should point to the tty too */
tp->t_line = APLDISC;	/*!!! kludge to get it started??? */
/*!!! note that tty ioctl also sets t_line to ours after open */

#ifdef APDEBUG
if (tracenum > 9 || apstrcnt == 0)
   tracenum = 0;
a->devno = tracenum++;
++ apstrcnt;
#endif
qtrcx("oa", a);
qtrcx("otp", tp);

/*
 * Now that we have a data structure to remember our state in, we
 * can proceed with initializing the device appropriately.
 *
 * Note that we are following the usual UNIX practice of keeping the
 * device driver simple, and assuming that the right parity, baud
 * rate, stop bits, etc., are set by the user program.
 */

a->writeonly = getwo(oflag);
a->rdopstate = RDNOINFO;
a->wrtopstate = WRTIDLE;
a->rcvstate = RCVFIRST;
a->xmtstate = XMTIDLE;
a->sndstate = SNDINIT;
a->gottimeout = FALSE;
a->ticking = FALSE;
a->ticksleft = 0;
a->senddata = FALSE;
a->sendpr = FALSE;
a->gotpr = FALSE;
a->gotsq = FALSE;
a->gotdorc = FALSE;
a->myseq = 0;
a->mynblks = 1;
a->otherseq = 0;
a->othernblks = 0;
a->ackseq = 0;
a->initialbrk = FALSE;
a->prretry = PRMAX;
/* code to derive the nulls per break from the speed */
switch (tp->t_ospeed)
   {
   case EXTA:	/* 19200 baud */
      a->nullsperbreak = 10;
      break;
   case B9600:
      a->nullsperbreak = 5;
      break;
   default:
      a->nullsperbreak = 3;
   }
switch (tp->t_ospeed)
   {	/* set maximum ticks per message */
   case B1200:
      a->fudgetime = 400;
      break;
   case B2400:
      a->fudgetime = 200;
      break;
   case B4800:
      a->fudgetime = 100;
      break;
   case B9600:
      a->fudgetime = 50;
      break;
   case EXTA:	/* 19200 baud */
      a->fudgetime = 25;
      break;
   }
a->xmtstate = XMTIDLE;  
qtrc("o1");
return(0);

}	/* end apopen */

/*
 * do system dependent initialization for the protocol code
 */
apinit(a)
register struct apstr *a;
{

(a->ttyp->t_oproc)(a->ttyp, APINTMASK);	/* set interupt mask */

}	/* end of apinit */


/*
 * apclose -
 *	This routine closes the device.
 * This will be called when an ioctl switches line discipline
 * from this one to another, or when a real close is done.
 */

apclose(tp)
register struct tty * tp;
{
register struct apstr *a;
int intrinfo;

a = tty2apstr(tp);
intrinfo = lockintrs;	   /* inhibit interupts from device */
if (dvstate != DVCLOSED)
   {
   /*
    * code to return apstr to pool
    */
   a->sndstate = SNDCLOSED;
   qtrc("csC");
   a->rcvstate = RCVIDLE;
   a->xmtstate = XMTIDLE;
   apsettimer(a, CLOSETIME);	/* wait to end timer ticks and interupts */
   sleep(closechan, SLEEPPRI);
   brelse(tp->t_bufp);	/* now it is safe to free the structure */
   tp->t_bufp = NULL;	/* clean up after myself */
   tp->t_line = 0;	/* be paranoid, just like tty_bk */
#ifdef APDEBUG
   --apstrcnt;
#endif
   unlockintrs(intrinfo);
   return(0);
   }
else
   {
   unlockintrs(intrinfo);
   reterrno(EBADF);
   }

}	/* end of apclose */

/*
 * apread -
 *	entry for reading data using async protocol
 */
/* We can be in two modes:
 *
 * Throwaway mode:
 * (writeonly==TRUE) means that we are always throwing away data as it comes
 * from the other side, and if the user actually tries to read, we reject
 * the attempt. I.e. it is appropriate for a user that thinks we are an
 * ordinary printer.
 *
 * Normal mode:
 * We will give the user whatever we have in the data read buffer,
 * up to the amount he requests, or the amount left in the buffer, whichever
 * is smaller. If there is nothing there, we will wait
 * for data to arrive. The user may kill the read safely (but not a write!)
 * upon a user timeout, in standard Unix signal fashion.
 */

apread(tp, uio)
register struct tty *tp;
register struct uio *uio;
{
register struct apstr *a;
int i;
int intrinfo;

a = tty2apstr(tp);
if (a->writeonly)
   {	/* reads are forbidden in throwaway mode */
   reterrno(EBADF);
   }
if (usercount <= 0)
   {
   reterrno(EINVAL);
   }

intrinfo = lockintrs;
while (TRUE)
   {

   switch (dvstate)
      {

      case DVCLOSED:
	 unlockintrs(intrinfo);
	 reterrno(EBADF);

      case DVSYNCING:
	 apstxmt(a);	/* start running, if we are not */
	 sleep(syncchan, SLEEPPRI);
	 continue;	/* go back to top of while loop */

      case DVUNSYNCED:
	 unlockintrs(intrinfo);
	 reterrno(EIO);

      case DVOPEN:
	 if (a->rdopstate == RDGOTCMD)
	    {
	    unlockintrs(intrinfo);
	    reterrno(EIO);
	    }
	 if (a->rdopstate == RDGOTDATA)
	    {
	    unlockintrs(intrinfo);
	    i = usercount < a->rddatacnt ? usercount : a->rddatacnt;
	    putrddata(a->rddataout, i);
	    a->rddatacnt -= i;
	    a->rddataout += i;
	    if (a->rddatacnt == 0)
	       {
	       intrinfo = lockintrs;
	       a->rdopstate = RDNOINFO;
	       qtrc("rrN");
	       ++ a->mynblks;	/* we can use to block to read again */
	       nqpacing(a);	/* so tell the other side */
	       unlockintrs(intrinfo);
	       }
	    return(0);	   /* give data back to the user */
	    }
	 sleep(rdchan, SLEEPPRI);
      } /* end of switch */
   }	/* end of while */

}	/* end of apread */

/*
 * apwrite -
 *	entry for writing user data using the async protocol
 */

/*
 * Currently, we donot reject a apwrite because we are not at the
 * WRTIDLE state. This implies it is possible to have two writes
 * at once for the driver, which is likely to be disasterous from
 * the user's view. On the other hand, if the user has a write
 * killed, and then does another, it will not hang because I
 * could not see the first write go away and therefore could not
 * set it to WRTIDLE before the second write.
 */
apwrite(tp, uio)
register struct tty *tp;
register struct uio *uio;
{
register struct apstr *a;
int intrinfo;

a = tty2apstr(tp);
if (usercount <= 0)
   {
   reterrno(EINVAL);
   }

intrinfo = lockintrs;
if (a->rdopstate != RDNOINFO)
   {
   unlockintrs(intrinfo);
   reterrno(EIO);
   }
while (TRUE)
   {

   switch (dvstate)
      {

      case DVCLOSED:
	 unlockintrs(intrinfo);
	 reterrno(EBADF);

      case DVSYNCING:
	 apstxmt(a);	/* start running, if we are not */
	 sleep(syncchan, SLEEPPRI);
	 continue;	/* go back to top of while loop */

      case DVUNSYNCED:
	 unlockintrs(intrinfo);
	 reterrno(EIO);

      case DVOPEN:
	 if (a->wrtopstate != WRTBUSY)
	    {
	    if (usercount <= 0 || a->rdopstate != RDNOINFO)
	       {	/* write done or data or a command byte to read */
	       a->wrtopstate = WRTIDLE;
	       qtrc("wwI");
	       unlockintrs(intrinfo);
	       return(0);	/* must report amount actually written */
	       }
	    if (a->othernblks > 0)
	       {	/* get data from user since printer can read it */
	       unlockintrs(intrinfo);
	       a->wrtdatacnt = usercount > MAXDATA ? MAXDATA : usercount;
	       getwrtdata(a->wrtdatabuf + PREFSIZE, a->wrtdatacnt);
	       /* Set prefix with zero block # for now, and compute crc. */
	       /* Later we will add the block # and adjust the crc. */
	       apfmtwrtbuf(a, a->wrtdatabuf, a->wrtdatacnt, DATAMSG);
	       intrinfo = lockintrs;
	       a->senddata = TRUE;
	       a->wrtopstate = WRTBUSY;
	       qtrc("wwB");
	       }
	    else
	       {	/* must wait until printer can read the data */
	       a->wrtopstate = WRTWAIT;
	       qtrc("wwW");
	       }
	    apstxmt(a);
	    }
	 sleep(wrtchan, SLEEPPRI);
	 /* continue looping */
      }      /* end of switch */
   }	     /* end of while */

}	/* end of apwrite */

/*
 * apioctl -
 *    driver control routine for async protocol
 */

apioctl(tp, cmd, uaddr, flag)
register struct tty * tp;
register int cmd;
register caddr_t uaddr;
int flag;
{
register struct apstr *a;
char c;
int speed;
int intrinfo;

a = tty2apstr(tp);

/*
 * Carry out one of the normal ioctl operations
 * Send "command byte" to other side.
 * Read command byte from other side.
 * Do test of line discipline state.
 * Tell user if there is something to read.
 * We may someday add ioctls to switch writeonly and
 *     read-write modes.
 */

switch (cmd)
   {

   case APWRTCMD:
      intrinfo = lockintrs;
      while (TRUE)
	 {
	 switch (dvstate)
	    {
	    case DVCLOSED:
	       unlockintrs(intrinfo);
	       reterrno(EBADF);
	    case DVSYNCING:
	       apstxmt(a);	/* start running, if we are not */
	       sleep(syncchan, SLEEPPRI);
	       continue;	/* go back to top of loop */
	    case DVUNSYNCED:
	       unlockintrs(intrinfo);
	       reterrno(EIO);
	    case DVOPEN:
	       if (a->wrtopstate != WRTBUSY)
		  {
		  if (a->wrtopstate == WRTDONE)
		     {
		     a->wrtopstate = WRTIDLE;
		     qtrc("iwI1");
		     unlockintrs(intrinfo);
		     return(0);
		     }
		  if (a->rdopstate != RDNOINFO)
		     {
		     a->wrtopstate = WRTIDLE;
		     qtrc("iwI2");
		     unlockintrs(intrinfo);
		     reterrno(EIO);
		     }
		  if (a->othernblks > 0)
		     {
		     getcmd(a->wrtdatabuf + 1, uaddr); /* get command byte */
		     a->wrtdatacnt = 1;
		     apfmtwrtbuf(a, a->wrtdatabuf, 1, COMMANDMSG);
		     a->senddata = TRUE;
		     a->wrtopstate = WRTBUSY;
		     qtrc("iwB");
		     }
		  else
		     {
		     a->wrtopstate = WRTWAIT;
		     qtrc("iwW");
		     }
		  apstxmt(a);
		  }
	       sleep(wrtchan, SLEEPPRI);
	    }	/* end of dvstate switch */
	 }	/* end of while loop */
      break;

   case APRDCMD:
      if (a->writeonly)
	 {	/* reads are forbidden in throwaway mode */
	 reterrno(EBADF);
	 }
      /* Fall through to APTESTRD and APTEST case */
   case APTEST:
   case APTESTRD:
      intrinfo = lockintrs;
      while (TRUE)
	 {
	 switch (dvstate)
	    {
	    case DVCLOSED:
	       unlockintrs(intrinfo);
	       reterrno(EBADF);
	    case DVSYNCING:
	       if (cmd != APRDCMD)
		  {	/* APTEST and APTESTRD should never hang */
		  qtrcx("a=",a);
		  qtrcx("tp=",tp);
		  retstatus(uaddr, APSYNCING);
		  unlockintrs(intrinfo);
		  return(0);
		  }
	       sleep(syncchan, SLEEPPRI);
	       continue;	/* go back to top of loop */
	    case DVUNSYNCED:
	       retstatus(uaddr, APDOWN);
	       unlockintrs(intrinfo);
	       return(0);
	    case DVOPEN:
	       switch (a->rdopstate)
		  {
		  case RDGOTDATA:
		     retstatus(uaddr, APGOTDATA);
		     unlockintrs(intrinfo);
		     return(0);
		  case RDNOINFO:
		     if (cmd != APRDCMD)
			{
			retstatus(uaddr, APNOINFO);
			unlockintrs(intrinfo);
			return(0);
			}
		     sleep(rdchan, SLEEPPRI);
		     continue;
		  case RDGOTCMD:
		     retstatus(uaddr, APGOTCMD);
		     if (cmd != APTEST)
			{
			a->rdopstate = RDNOINFO;
			qtrc("irN");
			++ a->mynblks;
			retcmd(uaddr, a->rdcmdval);
			nqpacing(a);
			}
		     unlockintrs(intrinfo);
		     return(0);
		  }	/* end of rdopstate switch */
	    }	/* end of dvstate switch */
	 }	/* end of while loop */

   case TIOCSETD:
   case TIOCGETD:
   case TIOCGETP:
   case TIOCEXCL:
   case TIOCNXCL:
      return(-1);	/* tell tty to do it */

   default:
      reterrno(EINVAL); /* tell tty that it is illegal */

   }	/* end of cmd switch */

}	/* end of apioctl */

/*
 * aprcvint -
 *    This routine is given control from the tty routines after every
 * receipt of a character, a break (APBREAK) or an error (APERROR).
 * Characters are values from 0...255. The others are negative codes.
 */

/*
 * Warning!!! The current version of asy.c actually ignores a break
 * unless there is also a character along with it, which is apparently
 * unusual. Thus I normally see a break come in as a framing error,
 * i.e. with APERROR instead of APBREAK. The framing error is seen
 * possibly before the break bit comes on? Good question. In any
 * case, I will treat either error code as if a break were indicated.
 * For the usual case, that should be okay.
 */

/* consider error or break as equivalent */
#define gotbreak (c < 0)

aprcvint(c, tp)
register int c;	/* negative error code or the character read in */
register struct tty *tp;

{
register struct apstr *a;

a = tty2apstr(tp);
#ifdef APDEBUG
   iqtrcx("!c",c);
#endif

switch (a->rcvstate)
   {

   case RCVIDLE: /* ignore interupt */
      return;

   case RCVFIRST:
      /* waiting for first character to come in */
      if (gotbreak)
	 return;
      if ((c & PREFMASK) == DATAMSG || (c & PREFMASK) == COMMANDMSG)
	 {	/* point to read buffer, if any */
	 if (a->mynblks <= 0)
	    {	/* ignore msg, sigh... */
	    a->rcvstate = RCVBREAK;
	    qtrc("vvB2");
	    /* Make sure to let printer see our latest buffer cnt. */
	    /* That may have been a "tickle" (yoohoo) msg he sent. */
	    if (dvstate == DVOPEN)
	       nqpacing(a);
	    return;
	    }
	 else
	    {
	    a->rcvbuf = a->rddatabuf;
	    a->rcvcmax = DATABUFSZ;
	    a->rcvcnt = 0;
	    }
	 }
      else
	 {
	 a->rcvbuf = a->rcvintbuf;
	 a->rcvcmax = INTBUFSZ;
	 a->rcvcnt = 0;
	 }
      a->rcvcksum = c;	/* initial value of checksum */
      storechar(c);	/* put char in read buffer */
      a->rcvstate = RCVCHAR;
      qtrc("vvC");
      return;

   case RCVBREAK:
      /* Waiting for a break to begin */
      if (gotbreak)
	 {
	 a->rcvstate = RCVFIRST;
	 qtrc("vvF1");
	 }
      return;

   case RCVCHAR:
      /* waiting for all characters after the first */
      if (gotbreak)
	 {
	 a->rcvstate = RCVFIRST;
	 qtrc("vvF2");
	 aprdprotocol(a);      /* run receive logic */
	 return;
	 }
      a->rcvcksum += c;	/* add each character into checksum */
      if (spaceleft(storechar(c)))
	 return;
      if (a->rcvcnt == DATABUFSZ)
	 {
	 a->rcvstate = RCVFIRST; /* Don't expect break, but allow one */
	 qtrc("vvF3");
	 aprdprotocol(a);
	 }
      else
	 {
	 a->rcvstate = RCVEND;
	 qtrc("vvE");
	 }
      return;

   case RCVEND:
      /* short msg; no more buffer; wait for break which must be next */
      if (gotbreak)
	 {
	 a->rcvstate = RCVFIRST;
	 qtrc("vvF4");
	 aprdprotocol(a);
	 }
      else
	 {
	 a->rcvstate = RCVBREAK;
	 qtrc("vvB4");
	 if (dvstate == DVOPEN)
	    nqpacing(a);
	 }
      return;

   }	/* end of switch */

}	/* end of aprcvint */

/*
 * apstxmt -
 *	starts a send (xmt) operation if not already under way.
 */

apstxmt(a)
register struct apstr *a;	/* driver state */
{

if (a->xmtstate == XMTIDLE)
   if (apsendanything(a))
      {
      if (a->initialbrk)
	 {    /* send an initial break before message */
	 a->xmtstate = XMTFIRST;
	 qtrc("xxF1");
	 }
      else
	 {    /* start sending a message */
	 a->xmtstate = XMT1STDATA;
	 qtrc("xxFD1");
	 }
#ifdef APDEBUG
      qtrc("xx!");
#endif
      apxmtint(a->ttyp);
      }

}	/* end of apstxmt */

/*
 * apxmtint -
 *	interupt level transmit routine
 *
 * This routine is given control for a "transmit holding reg empty"
 * interupt.
 *
 */

apxmtint(tp)
register struct tty * tp;
{
register struct apstr *a;	/* driver state */
#ifdef APDEBUG
char * pref = "!x";
#endif

a = tty2apstr(tp);

iqtrc(pref);

switch (a->xmtstate)
   {
   case XMTIDLE:	   /* ignore leftover interupt but remember it */
      return;

   case XMTFIRST:
      timeout(apxmtint, tp, 1);	/* wait to let xmt regs drain */
      /* Warning!!! If there is an extremely small amount of time
       * until the next UNIX tick, and nobody else waiting for it
       * and very little else to do, we just maybe might clobber
       * the last character sent. this is so rare a likelyhood that
       * I will still wait one tick. Error recovery will take care
       * of the very unusual case where we do clobber it.
       */
      a->xmtstate = XMTSETON;
      qtrc("xxS2");
      break;

   case XMTSETON:
      (tp->t_oproc)(tp, APBREAKON);	/* turn on break transmit bit */
      a->xmtstate = XMTEND;
      timeout(apxmtint, tp, 1);	/* wait for one tick to end the break */
      break;

   case XMT1STDATA:
         (tp->t_oproc)(tp, APBREAKOFF);	/* turn off break bit */
	 a->xmtstate = XMTDATA; 	/* fall through to data state */
	 /* fall through to XMTDATA */
   case XMTDATA:
      (tp->t_oproc)(tp, *(a->xmtbuf++));
      iqtrcx(pref,*(a->xmtbuf-1));
      if (++a->xmtcnt >= a->xmtcmax)
	 {	   /* message is done */
	 a->xmtstate = XMTFIRST;
	 qtrc("xxF2");
	 }
      break;

   case XMTEND:
      (tp->t_oproc)(tp, APBREAKOFF);	/* turn off break bit */
      apxmtending(a);
      if (apsendanything(a))
	 {
	 a->initialbrk = FALSE;
	 a->xmtstate = XMTDATA;
	 (tp->t_oproc)(tp, *(a->xmtbuf++));
	 iqtrcx(pref,*(a->xmtbuf-1));/*!!!debug*/
	 ++ a->xmtcnt;
	 /* assumes msgs have more than one character above*/
	 qtrc("xxD2");
	 }
      else
	 {
	 a->xmtstate = XMTIDLE;
	 qtrc("xxI");
	 return;	/* ignore leftover interupt, but remember it */
	 }

   }	/* end switch */

}	/* end of apxmtint */

/*
 * handletick -
 *   called on the interupt level for every UNIX tick, while a timer is
 * running logically. When it goes from 1 to 0, then appropriate send
 * protocol code is called.  If handletick is called with ticksleft <= 0,
 * timeouts are stopped.
 */

static handletick(a)
register struct apstr *a;
{
register int intrinfo;

intrinfo = lockintrs;
if (a->ticksleft > 0)
   {
   if ( -- a->ticksleft  > 0)
      timeout(handletick, a, 1);
   else
      {
      a->gottimeout = TRUE;
      a->ticking = FALSE;
      iqtrc("!T");
      if (dvstate == DVCLOSED)
	 wakeup(closechan);
      else
	 apstxmt(a);
      }
   }
else
   a->ticking = FALSE;
unlockintrs(intrinfo);

}	/* end of handletick */
/*
 * apsettimer -
 *    This sets a timeout interval, in terms of UNIX "ticks",
 * after which gottimeout is set and the appropriate part of the send
 * protocol is run. Apsettimer must be called with interupts off.
 * A value of zero given to it turns off the timer logically.
 */

apsettimer(a, t)
register struct apstr *a;
register int t;
{

a->gottimeout = FALSE;
a->ticksleft = t;
if (t > 0 && ! a->ticking)
   {
   timeout(handletick, a, 1);	/* start timer, one tick at a time */
   a->ticking = TRUE;
   }

}	/* end of apsettimer */
