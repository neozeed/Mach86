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
 *	@(#)tcp_timer.h	6.4 (Berkeley) 9/16/85
 */

/*
 * Definitions of the TCP timers.  These timers are counted
 * down PR_SLOWHZ times a second.
 */
#define	TCPT_NTIMERS	4

#define	TCPT_REXMT	0		/* retransmit */
#define	TCPT_PERSIST	1		/* retransmit persistance */
#define	TCPT_KEEP	2		/* keep alive */
#define	TCPT_2MSL	3		/* 2*msl quiet time timer */

/*
 * The TCPT_REXMT timer is used to force retransmissions.
 * The TCP has the TCPT_REXMT timer set whenever segments
 * have been sent for which ACKs are expected but not yet
 * received.  If an ACK is received which advances tp->snd_una,
 * then the retransmit timer is cleared (if there are no more
 * outstanding segments) or reset to the base value (if there
 * are more ACKs expected).  Whenever the retransmit timer goes off,
 * we retransmit one unacknowledged segment, and do a backoff
 * on the retransmit timer.
 *
 * The TCPT_PERSIST timer is used to keep window size information
 * flowing even if the window goes shut.  If all previous transmissions
 * have been acknowledged (so that there are no retransmissions in progress),
 * and the window is too small to bother sending anything, then we start
 * the TCPT_PERSIST timer.  When it expires, if the window is nonzero,
 * we go to transmit state.  Otherwise, at intervals send a single byte
 * into the peer's window to force him to update our window information.
 * We do this at most as often as TCPT_PERSMIN time intervals,
 * but no more frequently than the current estimate of round-trip
 * packet time.  The TCPT_PERSIST timer is cleared whenever we receive
 * a window update from the peer.
 *
 * The TCPT_KEEP timer is used to keep connections alive.  If an
 * connection is idle (no segments received) for TCPTV_KEEP amount of time,
 * but not yet established, then we drop the connection.  If the connection
 * is established, then we force the peer to send us a segment by sending:
 *	<SEQ=SND.UNA-1><ACK=RCV.NXT><CTL=ACK>
 * This segment is (deliberately) outside the window, and should elicit
 * an ack segment in response from the peer.  If, despite the TCPT_KEEP
 * initiated segments we cannot elicit a response from a peer in TCPT_MAXIDLE
 * amount of time, then we drop the connection.
 */

#define	TCP_TTL		15		/* time to live for TCP segs */
/*
 * Time constants.
 */
#define	TCPTV_MSL	( 15*PR_SLOWHZ)		/* max seg lifetime */
#define	TCPTV_SRTTBASE	0			/* base roundtrip time;
						   if 0, no idea yet */
#define	TCPTV_KEEP	( 45*PR_SLOWHZ)		/* keep alive - 45 secs */
#define	TCPTV_PERSMIN	(  5*PR_SLOWHZ)		/* retransmit persistance */

#define	TCPTV_MAXIDLE	(  8*TCPTV_KEEP)	/* maximum allowable idle
						   time before drop conn */

#define	TCPTV_MIN	(  1*PR_SLOWHZ)		/* minimum allowable value */
#define	TCPTV_MAX	( 30*PR_SLOWHZ)		/* maximum allowable value */

#define	TCP_LINGERTIME	120			/* linger at most 2 minutes */

#define	TCP_MAXRXTSHIFT	12			/* maximum retransmits */

#ifdef	TCPTIMERS
char *tcptimers[] =
    { "REXMT", "PERSIST", "KEEP", "2MSL" };
#endif

/*
 * Retransmission smoothing constants.
 * Smoothed round trip time is updated by
 *    tp->t_srtt = (tcp_alpha * tp->t_srtt) + ((1 - tcp_alpha) * tp->t_rtt)
 * each time a new value of tp->t_rtt is available.  The initial
 * retransmit timeout is then based on
 *    tp->t_timer[TCPT_REXMT] = tcp_beta * tp->t_srtt;
 * limited, however to be at least TCPTV_REXMTLO and at most TCPTV_REXMTHI.
 */
float	tcp_alpha, tcp_beta;

/*
 * Initial values of tcp_alpha and tcp_beta.
 * These are conservative: averaging over a long
 * period of time, and allowing for large individual deviations from
 * tp->t_srtt.
 */
#define	TCP_ALPHA	0.9
#define	TCP_BETA	2.0

/*
 * Force a time value to be in a certain range.
 */
#define	TCPT_RANGESET(tv, value, tvmin, tvmax) { \
	(tv) = (value); \
	if ((tv) < (tvmin)) \
		(tv) = (tvmin); \
	if ((tv) > (tvmax)) \
		(tv) = (tvmax); \
}
