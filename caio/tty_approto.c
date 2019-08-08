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
/* $Header: tty_approto.c,v 5.2 86/02/25 21:48:21 katherin Exp $ */
/* $Source: /ibm/acis/usr/sys_ca/caio/RCS/tty_approto.c,v $ */

/*
/*
 * With the possible exception of the following "include", this
 * module should be the same for all versions of the driver, and
 * contains most of the code for the async data mode protocol
 * that could be device independent.
 */

#include "../h/types.h"
#include "../caio/apvar.h"

/*
 *
 * aprdprotocol
 *
 * read logic of the link protocol
 */

aprdprotocol(a)
register struct apstr *a;
{
unsigned int sum;
byte *p;

/* calculate block checksum */
p = a->rcvbuf - 1;
sum = a->rcvcksum - *p + (a->rcvcnt - 3) + (*p << 8);
if ((sum & 0xFFFF) != 0)
   {
   qtrc("vBM");
   return;	/* ignore message */
   }
qtrc("vGM");
p = (a->rcvbuf -= a->rcvcnt);	/* adjust buffer ptr back to beginning */
switch (*p & PREFMASK)
   {

   case SEQRESET:
      if ((*p == SEQRESET) && (*(p+1) == SR2ND))  /* validity check */
	 {
	 switch (dvstate)
	    {
	    case DVSYNCING:
	       a->gotsq = TRUE;
	       a->otherseq = 0;
	       apstxmt(a);	/* tell send protocol about it */
	       break;
	    case DVOPEN:
	       /*
		* Assume for now that the 3812 died, and came back up.
		* We will go into the unsynced state, waiting for things
		* to die down (interupts, timeouts) and we will tell the
		* user that the line went down, if he has an operation
		* or two in progress. He should then close the driver.
		* If he wants to restart, he can open after that.
		*/
	       a->sndstate = SNDUNSYNCED;	/* we are unsynced first */
	       qtrc("vsU");
	       wakeup(rdchan);	/* tell the user about line down */
	       wakeup(wrtchan);
	       apsettimer(a, 0);	/* shut off timer */
	    }	/* end of switch */
	 }
      break;

   case DATAMSG:
   case COMMANDMSG:
      /*
       * Normal checking of received data or command messages is done here.
       * Specifically, checksum computation is finished, and if we are
       * running write only, we discard the message. If we are running
       * read-write, then the send protocol needs to be told about the
       * message received if we are still synchronizing, and it needs to
       * simply send a pacing receipt if we are already synchronized.
       * In both cases, we set up the read operation state to indicate
       * that there is data or command.
       * If we are synchronized, we should send a pacing receipt for any
       * data or command message received, even if it is a retry and out
       * of date, etc.
       */
      if ((SEQMASK & *p) == ((1 + a->otherseq) & MOD16MASK)
	     && a->rdopstate == RDNOINFO)
	 {	/* the message was valid */
	 if (dvstate == DVOPEN)
	    {
	    aprdmsg(a); /* process the message we read */
	    wakeup(rdchan);	/* tell the user about it */
	    wakeup(wrtchan);
	    nqpacing(a);	/* and acknowledge the message */
	    }
	 else
	    {	/* dvstate == DVSYNCING */
	    a->gotdorc = TRUE;	/* got data or command message */
	    apstxmt(a); 	/* tell send protocol about message */
	    }
	 }
      else if (dvstate == DVOPEN)
	 nqpacing(a);	/* It might be a retry; give it an ack */
      break;

   case PACINGRCT:
      if (*(p + 1) < 16)	/* validity check */
	 { /* valid rct - enqueue info saying my block was acked */
	 a->gotpr = TRUE;
	 /*
	  * Save the sequence number it was acking for a later check against
	  * what we have sent, if we have something not acknowledged.
	  */
	 a->ackseq = *p & SEQMASK;
	 /* Save the number of buffers the printer says it has. */
	 if ((a->othernblks = *(p + 1)) > 0 && a->wrtopstate == WRTWAIT)
	    wakeup(wrtchan);	/* wake up apwrite or apioctl; needs buffer */
	 apstxmt(a);   /* look for more xmt work */
	 }

   }	/* end of switch */

}	/* end of aprdprotocol */
/*
 * aprdmsg -
 *	This routine prepares a message that came in to be seen by the
 * user, or discards it if the driver is in write-only mode.
 * This routine is called by aprdprotocol when a data or command message
 * comes in, if the driver is open.
 * If the driver is still syncing with the printer, then aprdprotocol
 * calls apsendanything so that the send protocol can see the message
 * first. It may ignore the message or consider it valid. If it decides that
 * the message means that synchronization has been achieved, then it calls
 * this routine to give it to the user, or to discard it if the driver is in
 * write-only mode.
 */

aprdmsg(a)
register struct apstr * a;
{

if (! a->writeonly)
   {   /* we want to keep the message */
   if ((*a->rcvbuf & PREFMASK) == DATAMSG)
      {
      if ((a->rddatacnt = a->rcvcnt-3) > 0)
	 {     /* not a null message */
	 -- a->mynblks; /* say we now have one less buffer */
	 a->rddataout = a->rcvbuf+1;	/* remember where the data is */
	 a->rdopstate = RDGOTDATA;	/* tell user it is data msg */
	 qtrc("vrD");
	 }
      /*
       * else the message had no data, and is a "tickle" (yoohoo)
       * message, which we discard.
       */
      }
   else
      {        /* it was an expedited command message */
      -- a->mynblks;
      a->rdcmdval = *(a->rcvbuf+1);
      a->rdopstate = RDGOTCMD;
      qtrc("vrC");
      }
   }
a->otherseq = (SEQMASK & *a->rcvbuf);	/* update the sequence number */

}	/* end aprdmsg */
/*
 * apsendanything - send logic -
 *   This is called when the xmtstate is XMTIDLE or is about to go
 * XMTIDLE.
 * It checks if there is anything to be sent (i.e. on the send queue)
 * and if so, it sets it up to be sent. Specifically xmtbuf, xmtcmax,
 * and xmtcnt are set up. If the appropriate message has to be
 * constructed then it is, e.g. pacing receipts. If there is something
 * to send, TRUE is returned, otherwise nothing is done and FALSE
 * is returned.
 *   If something is to be sent, sndstate is changed from a non-xmt
 * to an xmt state as well.
 */

int apsendanything(a)
register struct apstr *a;
{

/* The following sets a new send state */
#define sst(s) {a->sndstate=(s);qtrcx("sst",(s));}

if ((a->sndstate & MSGMASK) != NOMSG)
   {	/* we are busy sending something */
   qtrc("s1");
   return(FALSE);	/* tell caller there is nothing to send */
   }

/* switch only to states where we are not transmitting */
switch (a->sndstate)
   {

   case SNDCLOSED:
      /* Closed, so of course we can't send anything. */
      return(FALSE);	/* tell caller, nothing to send */

   case SNDUNSYNCED:
      /*
       * We are shutting down now, and waiting for the user to tell
       * us to close. So we will tell him the line is down now.
       */
      return (FALSE);	/* tell caller, nothing to send */

   /*
    * The following section of states is associated with synchronization
    * with the printer. Every attempt has been made to match what the
    * printer does in "real life", and also meet the specifications for
    * the protocol.
    */

   case SNDINIT:
      /*
       * This is where we first do any transmission to the 3812.
       * We will call on the machine dependent code to do whatever
       * initialization is still needed, and then begin sending
       * sequence reset messages.
       */
      apinit(a);	/* hardware/system dependent initialization */
      apstsq(a);	/* build a sequence reset message */
      sst(SNDSQ);	/* set the state we will be in */
      return(TRUE);	/* and tell the caller to send it */
      break;

   case SNDSQWT:
      /*
       * We have sent a sequence reset (in SNDSQ state) and are now
       * looking for a sequence reset, a pacing receipt, a timeout or
       * a data message.
       */
      if (a->gotdorc)
	 {    /* we did get a data or command message */
	 goto sndsq;  /* resend sequence reset */
	 }
      if (a->gotsq)
	 {    /* received a sequence reset */
	 /*
	  * Start sending pacing in reply to first sequence reset.
	  * We will send it in SNDSQPR state.
	  * After sending that, we go to SNDSQPRWT state below.
	  * Note that we will be remembering any valid pacing receipt
	  * that we may have received.
	  */
	 a->gotsq = FALSE;
	 apsettimer(a, 0);	/* set no timeout */
	 sst(SNDSQPR);	/* set the transmission state */
	 apstpr(a);	/* build the pacing receipt to send */
	 return(TRUE);	/* and tell caller to send */
	 }
      if (a->gotpr && a->ackseq != a->myseq)
	 {    /* got a pacing receipt but not valid for our seq reset */
	 goto sndsq; /* so restart the whole mess */
	 }
      /* else if gotpr is set then it stays set, for the SNDSQPRWT state */
      if (a->gottimeout)
	 {    /* haven't gotten anything, so resend sequence reset */
	 goto sndsq;
	 }
      /* stay in this state, doing nothing */
      return(FALSE);	/* tell caller there is nothing to send */

   sndsq:	/* go into transmission of sequence reset */
      /*
       * In this state, we are starting over from scratch in
       * trying to sequence with the other side.
       */
      sst(SNDSQ);
      apsettimer(a, 0); /* turn off timer */
      a->gotpr = FALSE; 	/* forget pacing receipt, if any */
      a->gotsq = FALSE; 	/* forget sequence reset, if any */
      a->gotdorc = FALSE;	/* forget received data, if any */
      /* reset all sequence numbers or buffer counts. */
      a->myseq = 0;
      a->mynblks = 1;
      a->otherseq = 0;
      a->othernblks = 0;
      a->ackseq = 0;
      apstsq(a);	/* build a sequence reset */
      return(TRUE);	/* and tell caller to start sending it */

   case SNDSQPRWT:
      /*
       * We have basically sent and received sequence resets, and
       * here, we are waiting for a pacing receipt for our sequence
       * reset(s) we sent. If we get one, we are done with syncing,
       * and open for normal business.	If we get another sequence
       * reset while here, we just acknowledge it with a pacing
       * receipt and come back here. but if we get a timeout, we
       * assume that things have been missed too much ine either or
       * both directions, and go back to SNDSQ (square one of the
       * synchronization process). As a special thing (is that the
       * right phrase?) if we have a validly received data message
       * here, we assume that the printer thinks we are already
       * synced, and so we will acknowledge the message and then go
       * to the normal open state. (I have seen the printer do this.
       * It does happen.)
       */
      if (a->gotsq)
	 {	/* got sequence reset */
	 /* appears he may not have heard my pacing receipt */
	 goto sndsq;	/* start over completely fresh */
	 }
      if (a->gotpr)
	 {	/* we got a pacing receipt */
	 if (a->ackseq == a->myseq)	/* was it for our seq reset ? */
	    {	/* It was */
	    a->gotpr = FALSE;	/* forget pacing receipt */
	    a->myseq += a->myseq < 15 ? 1 : -15;   /* set to next seq num */
	    /* sndidle will take care of timer */
	    if (a->gotdorc)
	       {	/* got data or cmd message */
	       a->gotdorc = FALSE;
	       aprdmsg(a);	/* finish processing message */
	       a->sendpr = TRUE;	/* and acknowledge it */
	       }
	    wakeup(syncchan);	/* wake up any user operations waiting */
	    goto sndidle;	/* go look for work, if any */
	    }
	 else
	    goto sndsq; /* invalid pacing receipt; start over completely */
	 }
      if (a->gottimeout || a->gotdorc)
	 {	/* timed out without getting something, start over */
	 /* or got data or cmd without getting pacing rct for our seq res */
	 goto sndsq;
	 }
      return (FALSE);	/* tell caller, nothing to send */


   sndidle:	/* we can come here from other "wait" states */
      apsettimer(a, YOOHOOTIME);	/* timer for sending pacing receipt */
      sst(SNDIDLE);
   case SNDIDLE:
      /*
       * This is the main state where we look for some data or command
       * message to send. One major path from here is where we see there
       * is some data or command to send, but no buffer available in the
       * printer. In that case we end up in SNDBLKWT and subsequent states.
       * Once a buffer is available in the printer, when we have data or
       * command to send, we end up on SNDMSG and subsequent states.
       */
      /*
       * We also start a timer in this state, as an error recovery procedure
       * in case the printer has missed our last pacing receipt that had
       * told it that there was a buffer available on this side to send to.
       * Since the printer does not implement the "tickle" (yoohoo) message
       * to recover from this situation, I must insure that he hears my
       * latest pacing receipt. Thus this kludge.
       */
      a->gotpr = FALSE; /* we look at effects of pacing receipt below */
      if (a->sendpr || a->gottimeout)
	 {	/* need to send pacing receipt */
	 a->gottimeout = FALSE;
	 apstpr(a);	/* prepare pacing receipt to send */
	 sst(SNDIDLPR);
	 return(TRUE);	/* tell caller to send it */
	 }
      if (a->senddata)
	 {	/* apwrite or apioctl has prepared a message to send */
	 /* put block number in msg and increment crc */
	 apfixwrtbuf(a, a->wrtdatabuf, a->wrtdatacnt, a->myseq);
	 a->senddata = FALSE;
	 apstdata(a);	/* finish preparing the message */
	 a->retrycnt = MAXRETRY;	/* set maximum retransmit count */
	 /*
	  * Send a pacing receipt every MAXPR data or command messages that
	  * we send. This is part of the error recovery for missed pacing
	  * receipts discussed above.
	  */
	 if (--a->prretry <= 0)
	    {	/* count ran out, do a pacing receipt after data/cmd msg */
	    a->prretry = PRMAX;	/* refresh the counter */
	    a->sendpr = TRUE;	/* remember to send pacing receipt */
	    }
	 a->wrtopstate = WRTBUSY;	/* indicate we are now sending */
	 qtrc("swB");
	 sst(SNDMSG);	/* set the new state */
	 return(TRUE);	/* tell the caller to send the message */
	 }
      if (a->wrtopstate == WRTWAIT)
	 {	/* there is a write op wanting to send */
	 if (a->othernblks <= 0)	/* check if 3812 has buffer */
	    {	/* 3812 has no buffer */
	    sst(SNDBLKWT);	/* set to state to wait for one */
	    apsettimer(a, YOOHOOTIME);	/* wait to send yoohoo (tickle) msg */
	    }
	 else
	    {	/* there is a buffer; tell apwrite or apioctl */
	    wakeup(wrtchan);
	    }
	 }
      return(FALSE);	/* tell caller, nothing to send */

   case SNDACKWT:
      /*
       * We have sent a data or command message one or more times,
       * and are waiting for an acknowledgement of it. If we time out,
       * we will send it again, unless the retry count has run out,
       * in which case we declare the line is down.
       */
      /*
       * As explained under the sndidle section above, we need to send
       * a pacing receipt periodically to the other side. If we are
       * busy sending a lot of data to the other side, the timeout in
       * the sndidle state will not happen, and so we need another way
       * of reminding ourselves periodically to send the pacing receipt.
       * We maintain a count called prretry, which is reduced by 1
       * for every data message, and wraps back up to some nice number
       * like five. Each time it wraps, send a pacing receipt.
       */
      if (a->sendpr)
	 {	/* we should send a pacing receipt */
	 apstpr(a);	/* prepare one to send */
	 sst(SNDWTPR);	/* set up state change */
	 return(TRUE);	/* tell caller to send it */
	 }
      a->gotpr = FALSE; /* just clear pacing receipt flag */
      if (a->myseq == a->ackseq) /* was last sent msg acked? */
	 {	/* yes, it was, tell apwrite or apioctl */
	 /* sndidle will take care of the timer */
	 a->myseq += a->myseq < 15 ? 1 : -15;	/* set to next seq num */
	 a->wrtopstate = WRTDONE;  /* report that bufferful was sent */
	 qtrc("swD");
	 wakeup(wrtchan);	/* wake up apwrite or apioctl */
	 goto sndidle;	/* go to idle state where there may be work */
	 }
      if (a->gottimeout)
	 {	/* got retry timeout; msg not acknowledged yet */
	 a->gottimeout = FALSE;
	 if (a->retrycnt-- <= 0)	/* step and test retry count */
	    {	/* we have resent the maximum number of times */
	    sst(SNDUNSYNCED);	/* tell user line is down */
	    wakeup(rdchan);
	    wakeup(wrtchan);
	    return(FALSE);	/* and there is nothing to send */
	    }
	 else
	    {	/* try sending message again */
	    /* a->initialbrk = TRUE; */
	    apstdata(a);	/* set it up to send it */
	    sst(SNDMSG);
	    return(TRUE);	/* tell caller to send it */
	    }
	 }
      return(FALSE);	/* tell user, nothing to send */

   sndblkwt:
      sst(SNDBLKWT);
   case SNDBLKWT:
      /*
       * In this state we have a timer running for a long period. If it
       * runs out, we have not seen a pacing receipt for that period
       * and we will send a "tickle" (yoohoo) message to evoke a pacing
       * receipt from the other side. The message is a data message with
       * no data, and with a sequence number of the last acknowledged message
       * we sent. The printer will do nothing but send a pacing receipt,
       * presuming it is still up. This whole process is in case we
       * somehow missed a pacing receipt which said there was a buffer
       * available on the other side. It could catch a line-down case too.
       */
      if (a->sendpr)
	 {	/* send a pacing receipt */
	 apstpr(a);	/* build one to send */
	 sst(SNDBWPR);	/* set up the transmission state */
	 return(TRUE);	/* tell caller to send it */
	 }
      if (a->senddata)
	 {	/* the user op is ready for writing data or command */
	 /* sndidle will take care of the timer */
	 goto sndidle;	/* sndidle will know what to do next */
	 }
      a->gotpr = FALSE;
      if (a->othernblks > 0)
	 {	/* there is now a printer buffer available */
	 /* sndidle will take care of the timer */
	 goto sndidle;	/* go look for work */
	 /* We may need to wake up write or ioctl. sndidle should do it */
	 }
      if (a->gottimeout)
	 {	/* now we should send a yoohoo message */
	 a->gottimeout = FALSE;
	 a->retrycnt = MAXRETRY;	/* max number of times to try */
	 apstyoohoo(a); 	/* build a yoohoo message */
	 sst(SNDYOOHOO);	/* set the transmission state */
	 return(TRUE);		/* tell caller to send the message */
	 }
      return(FALSE);	/* tell caller, nothing to send */

   case SNDYHWT:
      /*
       * This is where we come after sending a yoohoo message. We should
       * get a pacing receipt fairly soon. If we do not, we retry, until
       * the retry count runs out, at which point the line is declared down.
       * If we a pacing receipt, we either go to SNDIDLE (if there is a
       * 3812 buffer available) or we go to SNDBLKWT again (if there is
       * no buffer available yet). In either case, at least we have heard
       * from the printer, and will set the long wait time again.
       */
      if (a->sendpr)
	 {
	 apstpr(a);	/* build one to send */
	 sst(SNDYHPR);	/* set up the transmission state */
	 return(TRUE);	/* tell caller to send it */
	 }
      if (a->senddata)
	 {	/* the user op is ready for writing data or command */
	 /* sndidle will take care of the timer */
	 goto sndidle;	/* sndidle will know what to do next */
	 }
      if (a->gotpr)
	 {	/* got a pacing receipt so printer is there */
	 a->gotpr = FALSE;
	 if (a->othernblks > 0)
	    {	   /* there is now a printer buffer available */
	    /* sndidle will take care of the timer */
	    goto sndidle;  /* go look for work */
	    /* We may need to wake up write or ioctl. sndidle should do it */
	    }
	 else
	    {	/* no buffer yet available in 3812 */
	    /* refer to sndidle for the reason for the following */
	    a->sendpr = TRUE;	/* to cause resend of pacing receipt */
	    apsettimer(a, YOOHOOTIME);
	    goto sndblkwt;	/* we will go back to long timeout wait */
	    }
	 }
      if (a->gottimeout)
	 {	/* did not hear pacing receipt for yoohoo message */
	 a->gottimeout = FALSE;
	 if (a->retrycnt-- <= 0)	/* step and test retry count */
	    {	/* we have resent the maximum number of times */
	    sst(SNDUNSYNCED);	/* tell user line is down */
	    wakeup(rdchan);
	    wakeup(wrtchan);
	    return(FALSE);	/* and there is nothing to send */
	    }
	 else
	    {	/* try sending yoohoo message again */
	    /* a->initialbrk = TRUE; */
	    apstyoohoo(a);	  /* set it up to send it */
	    sst(SNDYOOHOO);
	    return(TRUE);	/* tell caller to send it */
	    }
	 }
      return(FALSE);	/* tell caller, there is nothing to send */

   }	     /* end switch a->sndstate */

} /* end of apsendanything */

/*
 * apxmtending -
 *   run the part of the "send protocol" where the sndstate has been
 * transmitting something (message or pacing receipt). This routine restores
 * xmtcnt and xmtbuf to their starting values, and moves sndstate to an
 * appropriate wait state. A timer is started, if appropriate.
 */

apxmtending(a)
register struct apstr *a;
{
int t;

rsxmtcnt;
rsxmtbuf;
switch (a->sndstate)
   {
   case SNDSQ:	/* finished sending sequence reset */
      apsettimer(a, SQTIME);
      sst(SNDSQWT);	/* wait for pacing receipt or sequence reset */
      break;
   case SNDSQPR:	/* finished sending pacing receipt */
      apsettimer(a, SQTIME);
      sst(SNDSQPRWT);	/* wait for pacing receipt acking our seq. res. */
      break;
   case SNDIDLPR:	/* finished sending pacing receipt */
      /* Refer to sndidle writeup for reason for this timer call */
      apsettimer(a, YOOHOOTIME);	/* causes resending of pacing rct */
      sst(SNDIDLE);	/* go to idle state, wait for work to do */
      break;
   case SNDBWPR:	/* finished sending pacing receipt */
      sst(SNDBLKWT);	/* still waiting for 3812 to become available */
      break;
   case SNDYOOHOO:	/* finished sending yoohoo (tickle) message */
      apsettimer(a, SHORTTIME + a->fudgetime);
      sst(SNDYHWT);	/* wait for pacing receipt as reply */
      break;
   case SNDYHPR:   /* fninished sending a pacing receipt */
      sst(SNDYHWT);	/* still waiting for pacing receipt for yoohoo */
      break;
   case SNDMSG: /* just finished sending data or command message */
      apsettimer(a,SHORTTIME + a->fudgetime);
      sst(SNDACKWT);	/* wait for pacing receipt to ack it */
      break;
   case SNDWTPR:	/* just finished sending pacing receipt */
      sst(SNDACKWT);	/* still waiting for ack of our data or cmd msg */
      break;
   }	/* end switch */

}	/* end of apxmtending */

/*
 * apstsq -
 *     Basically this routine sets up a sequence reset message to send.
 */

apstsq(a)
register struct apstr *a;
{
byte *p;

p = a->xmtbuf = a->xmtintbuf;	/* addr of xmtintbuf in xmtbuf and p */
svxmtbuf;	/* save xmtbuf, if necessary in this version */
initxmtcnt;	/* init xmtcnt, if necessary in this version */
a->xmtcmax = 4;
*(p+1) = SR2ND;
a->myseq = 0;
apfmtwrtbuf(a, p, 1, SEQRESET);

}	/* end of apstsq */
/*
 * apstpr -
 *    Basically this routine sets up a pacing receipt to send.
 */

apstpr(a)
register struct apstr *a;
{
byte *p;

a->sendpr = FALSE;
p = a->xmtbuf = a->xmtintbuf;	/* addr of xmtintbuf in xmtbuf and p */
svxmtbuf;	/* save xmtbuf, if necessary in this version */
initxmtcnt;	/* init xmtcnt, if necessary in this version */
a->xmtcmax = 4;
*(p+1) = a->mynblks;
apfmtwrtbuf(a, p, 1, PACINGRCT | a->otherseq);

}	/* end of apstpr */
/*
 * apstdata -
 *     This routine sets up the message that is in the user write buffer
 * to be sent. A separate routine should format it in its final form, with
 * proper crc etc, since this routine can be called several times to resend
 * the message, if necessary.
 */

apstdata(a)
register struct apstr *a;
{

a->xmtbuf = a->wrtdatabuf;	/* addr of wrtdatabuf in xmtbuf */
svxmtbuf;	/* save xmtbuf, if necessary in this version */
initxmtcnt;	/* init xmtcnt, if necessary in this version */
a->xmtcmax = a->wrtdatacnt+3;

}	/* end of apstdata */
/*
 * apstyoohoo -
 *    sets up the yoohoo message to be sent.
 * Currently, the yoohoo message is a data message, with zero data,
 * which gets formed in the internal buffer, since, of course, the real
 * data buffer is occupied with a message that we have not been able to
 * send since we have seen no indication that there is space over there.
 * The yoohoo message has the sequence number of the last sequence number
 * that has been acknowledged, and since the other side sees it out of
 * sequence, and knows it is acknowledged, he ignores it, but is obligated
 * to resend the pacing receipt. Thus we get the pacing receipt we have
 * wanted.
 */

apstyoohoo(a)
register struct apstr *a;
{
byte *p;
int i;

p = a->xmtbuf = a->xmtintbuf;	/* addr of xmtintbuf in xmtbuf and p */
svxmtbuf;	/* save xmtbuf, if necessary in this version */
initxmtcnt;	/* init xmtcnt, if necessary in this version */
/* get invalid seq num for zero data msg ... other side must reply with
   pacing receipt. */
i = a->myseq == 0 ? 15 : a->myseq - 1;
a->xmtcmax = 3;
apfmtwrtbuf(a, p, 0, DATAMSG | i);

}	/* end of apstyoohoo */
/*
 * apfmtwrtbuf -
 *    format the data in the write buffer with crc, etc.
 */

apfmtwrtbuf(a, buf, cnt, byte0)
register struct apstr *a;
byte *buf;  /* pointer to buffer to format */
int cnt;	/* amount of data in buffer, starting at byte 1, not 0 */
byte byte0;	/* initial value of byte 0 for buffer */
{
unsigned int ui;
byte *p;
byte *endp;

*buf = byte0;
endp = buf + (cnt + 1);
p = buf;
ui = -cnt;
while (p < endp)
   ui -= *p++;
*endp++ = ui & 0xFF;
*endp = (ui >> 8) & 0xFF;

}	/* end of apfmtwrtbuf */
/*
 * apfixwrtbuf -
 *    adjust the write message in the buffer by or-ing in the block number
 * and subtracting it from the crc in the buffer, making it ready to send.
 */

apfixwrtbuf(a, buf, cnt, wrtblkno)
register struct apstr *a;
byte *buf;  /* pointer to buffer */
int cnt;	/* amount of data in buffer (prefix and crc are extra) */
int wrtblkno;	/* block number to put in message */
{
unsigned int ui;
byte *p;

p = buf + (cnt + 1);
ui = *p | *(p + 1) << 8;
ui -= wrtblkno;
*p = ui & 0xFF;
*(p + 1) = (ui >> 8) & 0xFF;
*buf |= wrtblkno & 0x0F;

}	/* end of apfixwrtbuf */

