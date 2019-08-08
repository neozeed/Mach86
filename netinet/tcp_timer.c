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
 * Copyright (c) 1982 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)tcp_timer.c	6.8 (Berkeley) 9/16/85
 */
#if	CMU
/*
 **********************************************************************
 * HISTORY
 * 25-Jan-86  Avadis Tevanian (avie) at Carnegie-Mellon University
 *	Upgraded to 4.3.
 *
 * 05-Oct-85  Mike Accetta (mja) at Carnegie-Mellon University
 *	Added inactivity time-out check in tcp_timers() in the absence of
 *	any keep-alive action.
 *	[v1(1)]
 *
 **********************************************************************
 */
 
#include "cs_tcp.h"
#endif	CMU

#include "param.h"
#include "systm.h"
#include "mbuf.h"
#include "socket.h"
#include "socketvar.h"
#include "protosw.h"
#include "errno.h"

#include "../net/if.h"
#include "../net/route.h"

#include "in.h"
#include "in_pcb.h"
#include "in_systm.h"
#include "ip.h"
#include "ip_var.h"
#include "tcp.h"
#include "tcp_fsm.h"
#include "tcp_seq.h"
#include "tcp_timer.h"
#include "tcp_var.h"
#include "tcpip.h"

int	tcpnodelack = 0;
/*
 * Fast timeout routine for processing delayed acks
 */
tcp_fasttimo()
{
	register struct inpcb *inp;
	register struct tcpcb *tp;
	int s = splnet();

	inp = tcb.inp_next;
	if (inp)
	for (; inp != &tcb; inp = inp->inp_next)
		if ((tp = (struct tcpcb *)inp->inp_ppcb) &&
		    (tp->t_flags & TF_DELACK)) {
			tp->t_flags &= ~TF_DELACK;
			tp->t_flags |= TF_ACKNOW;
			(void) tcp_output(tp);
		}
	splx(s);
}

/*
 * Tcp protocol timeout routine called every 500 ms.
 * Updates the timers in all active tcb's and
 * causes finite state machine actions if timers expire.
 */
tcp_slowtimo()
{
	register struct inpcb *ip, *ipnxt;
	register struct tcpcb *tp;
	int s = splnet();
	register int i;

	/*
	 * Search through tcb's and update active timers.
	 */
	ip = tcb.inp_next;
	if (ip == 0) {
		splx(s);
		return;
	}
	for (; ip != &tcb; ip = ipnxt) {
		ipnxt = ip->inp_next;
		tp = intotcpcb(ip);
		if (tp == 0)
			continue;
		for (i = 0; i < TCPT_NTIMERS; i++) {
			if (tp->t_timer[i] && --tp->t_timer[i] == 0) {
				(void) tcp_usrreq(tp->t_inpcb->inp_socket,
				    PRU_SLOWTIMO, (struct mbuf *)0,
				    (struct mbuf *)i, (struct mbuf *)0);
				if (ipnxt->inp_prev != ip)
					goto tpgone;
			}
		}
		tp->t_idle++;
		if (tp->t_rtt)
			tp->t_rtt++;
tpgone:
		;
	}
	tcp_iss += TCP_ISSINCR/PR_SLOWHZ;		/* increment iss */
	splx(s);
}

/*
 * Cancel all timers for TCP tp.
 */
tcp_canceltimers(tp)
	struct tcpcb *tp;
{
	register int i;

	for (i = 0; i < TCPT_NTIMERS; i++)
		tp->t_timer[i] = 0;
}

float	tcp_backoff[TCP_MAXRXTSHIFT] =
    { 1.0, 1.2, 1.4, 1.7, 2.0, 3.0, 5.0, 8.0, 16.0, 32.0, 32.0, 32.0 };
int	tcpexprexmtbackoff = 0;
/*
 * TCP timer processing.
 */
struct tcpcb *
tcp_timers(tp, timer)
	register struct tcpcb *tp;
	int timer;
{

	switch (timer) {

	/*
	 * 2 MSL timeout in shutdown went off.  If we're closed but
	 * still waiting for peer to close and connection has been idle
	 * too long, or if 2MSL time is up from TIME_WAIT, delete connection
	 * control block.  Otherwise, check again in a bit.
	 */
	case TCPT_2MSL:
		if (tp->t_state != TCPS_TIME_WAIT &&
		    tp->t_idle <= TCPTV_MAXIDLE)
			tp->t_timer[TCPT_2MSL] = TCPTV_KEEP;
		else
			tp = tcp_close(tp);
		break;

	/*
	 * Retransmission timer went off.  Message has not
	 * been acked within retransmit interval.  Back off
	 * to a longer retransmit interval and retransmit one segment.
	 */
	case TCPT_REXMT:
		tp->t_rxtshift++;
		if (tp->t_rxtshift > TCP_MAXRXTSHIFT) {
			tp = tcp_drop(tp, ETIMEDOUT);
			break;
		}
		/*
		 * If losing, let the lower level know
		 * and try for a better route.
		 */
		if (tp->t_rxtshift > TCP_MAXRXTSHIFT / 3)
			in_rtchange(tp->t_inpcb);
		TCPT_RANGESET(tp->t_timer[TCPT_REXMT],
		    (int)tp->t_srtt, TCPTV_MIN, TCPTV_MAX);
		if (tcpexprexmtbackoff) {
			TCPT_RANGESET(tp->t_timer[TCPT_REXMT],
			    tp->t_timer[TCPT_REXMT] << tp->t_rxtshift,
			    TCPTV_MIN, TCPTV_MAX);
		} else {
			TCPT_RANGESET(tp->t_timer[TCPT_REXMT],
			    tp->t_timer[TCPT_REXMT] *
			        tcp_backoff[tp->t_rxtshift - 1],
			    TCPTV_MIN, TCPTV_MAX);
		}
		tp->snd_nxt = tp->snd_una;
		/*
		 * If timing a segment in this window,
		 * and we have already gotten some timing estimate,
		 * stop the timer.
		 */
		if (tp->t_rtt && tp->t_srtt)
			tp->t_rtt = 0;
		(void) tcp_output(tp);
		break;

	/*
	 * Persistance timer into zero window.
	 * Force a byte to be output, if possible.
	 */
	case TCPT_PERSIST:
		tcp_setpersist(tp);
		tp->t_force = 1;
		(void) tcp_output(tp);
		tp->t_force = 0;
		break;

	/*
	 * Keep-alive timer went off; send something
	 * or drop connection if idle for too long.
	 */
	case TCPT_KEEP:
		if (tp->t_state < TCPS_ESTABLISHED)
			goto dropit;
		if (tp->t_inpcb->inp_socket->so_options & SO_KEEPALIVE) {
		    	if (tp->t_idle >= TCPTV_MAXIDLE)
				goto dropit;
			/*
			 * Saying tp->rcv_nxt-1 lies about what
			 * we have received, and by the protocol spec
			 * requires the correspondent TCP to respond.
			 * Saying tp->snd_una-1 causes the transmitted
			 * byte to lie outside the receive window; this
			 * is important because we don't necessarily
			 * have a byte in the window to send (consider
			 * a one-way stream!)
			 */
			tcp_respond(tp,
			    tp->t_template, tp->rcv_nxt-1, tp->snd_una-1, 0);
		} else
#if	CS_TCP
			/*
			 *  If we aren't doing the (questionable) keep-alive
			 *  hack, we can use the more reasonable inactivity
			 *  time-out.  This value is currently set only by the
			 *  BBN emulation code.  It would probably be useful to
			 *  add it to the standard network interface as well by
			 *  implementing it as a protocol specific option.
			 */
			if (tp->t_noact)
			{
				if (tp->t_idle >= tp->t_noact)
					goto dropit;
			}
			else
#endif	CS_TCP
			tp->t_idle = 0;
		tp->t_timer[TCPT_KEEP] = TCPTV_KEEP;
		break;
	dropit:
		tp = tcp_drop(tp, ETIMEDOUT);
		break;
	}
	return (tp);
}
