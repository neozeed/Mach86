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
 *	@(#)ns_pcb.c	6.4 (Berkeley) 8/9/85
 */

#include "param.h"
#include "systm.h"
#include "dir.h"
#include "user.h"
#include "mbuf.h"
#include "socket.h"
#include "socketvar.h"
#include "../net/if.h"
#include "../net/route.h"
#include "protosw.h"

#include "ns.h"
#include "ns_if.h"
#include "ns_pcb.h"

struct	ns_addr zerons_addr;

ns_pcballoc(so, head)
	struct socket *so;
	struct nspcb *head;
{
	struct mbuf *m;
	register struct nspcb *nsp;

	m = m_getclr(M_DONTWAIT, MT_PCB);
	if (m == NULL)
		return (ENOBUFS);
	nsp = mtod(m, struct nspcb *);
	nsp->nsp_socket = so;
	insque(nsp, head);
	so->so_pcb = (caddr_t)nsp;
	return (0);
}
	
ns_pcbbind(nsp, nam)
	register struct nspcb *nsp;
	struct mbuf *nam;
{
	register struct sockaddr_ns *sns;
	u_short lport = 0;

	if(nsp->nsp_lport || !ns_nullhost(nsp->nsp_laddr))
		return (EINVAL);
	if (nam == 0)
		goto noname;
	sns = mtod(nam, struct sockaddr_ns *);
	if (nam->m_len != sizeof (*sns))
		return (EINVAL);
	if (!ns_nullhost(sns->sns_addr)) {
		int tport = sns->sns_port;

		sns->sns_port = 0;		/* yech... */
		if (ifa_ifwithaddr((struct sockaddr *)sns) == 0)
			return (EADDRNOTAVAIL);
		sns->sns_port = tport;
	}
	lport = sns->sns_port;
	if (lport) {
		u_short aport = ntohs(lport);

		if (aport < NSPORT_RESERVED && u.u_uid != 0)
			return (EACCES);
		if (ns_pcblookup(&zerons_addr, lport, 0))
			return (EADDRINUSE);
	}
	nsp->nsp_laddr = sns->sns_addr;
noname:
	if (lport == 0)
		do {
			if (nspcb.nsp_lport++ < NSPORT_RESERVED)
				nspcb.nsp_lport = NSPORT_RESERVED;
			lport = htons(nspcb.nsp_lport);
		} while (ns_pcblookup(&zerons_addr, lport, 0));
	nsp->nsp_lport = lport;
	return (0);
}

/*
 * Connect from a socket to a specified address.
 * Both address and port must be specified in argument sns.
 * If don't have a local address for this socket yet,
 * then pick one.
 */
ns_pcbconnect(nsp, nam)
	struct nspcb *nsp;
	struct mbuf *nam;
{
	struct ns_ifaddr *ia;
	struct sockaddr_ns *ifaddr;
	register struct sockaddr_ns *sns = mtod(nam, struct sockaddr_ns *);

	if (nam->m_len != sizeof (*sns))
		return (EINVAL);
	if (sns->sns_family != AF_NS)
		return (EAFNOSUPPORT);
	if (sns->sns_port==0 || ns_nullhost(sns->sns_addr))
		return (EADDRNOTAVAIL);
	if (ns_nullhost(nsp->nsp_laddr) &&
	    ((*(long *)&nsp->nsp_lastnet)!=ns_netof(sns->sns_addr))) {
		ia = (struct ns_ifaddr *)ifa_ifwithnet((struct sockaddr *)sns);
		if (ia == 0) {
			register struct route *ro;
			struct ifnet *ifp;

			/* 
			 * If route is known or can be allocated now,
			 * our src addr is taken from the i/f, else punt.
			 */
			ro = &nsp->nsp_route;
#define	satosns(sa)	((struct sockaddr_ns *)(sa))
			if (ro->ro_rt &&
				!ns_hosteq(satosns(&ro->ro_dst)->sns_addr,
					sns->sns_addr)) {
				RTFREE(ro->ro_rt);
				ro->ro_rt = (struct rtentry *)0;
			}
			if ((ro->ro_rt == (struct rtentry *)0) ||
			    (ifp = ro->ro_rt->rt_ifp) == (struct ifnet *)0) {
				/* No route yet, so try to acquire one */
				ro->ro_dst.sa_family = AF_NS;
				satosns(&ro->ro_dst)->sns_addr =
					sns->sns_addr;
				rtalloc(ro);
				if (ro->ro_rt == 0)
					ifp = (struct ifnet *)0;
				else
					ifp = ro->ro_rt->rt_ifp;
			}
			if (ifp) {
				for (ia = ns_ifaddr; ia; ia = ia->ia_next)
					if (ia->ia_ifp == ifp)
					    break;
			}
			if (ia == 0)
				ia = ns_ifaddr;
			if (ia == 0)
				return (EADDRNOTAVAIL);
		}
		ifaddr = satosns(&ia->ia_addr);
		nsp->nsp_laddr.x_net = ifaddr->sns_addr.x_net;
		nsp->nsp_lastnet = sns->sns_addr.x_net;
	}
	if (ns_pcblookup(&sns->sns_addr, nsp->nsp_lport, 0))
		return (EADDRINUSE);
	if (ns_nullhost(nsp->nsp_laddr)) {
		if (nsp->nsp_lport == 0)
			(void) ns_pcbbind(nsp, (struct mbuf *)0);
		nsp->nsp_laddr.x_host = ns_thishost;
	}
	nsp->nsp_faddr = sns->sns_addr;
	/* Includes nsp->nsp_fport = sns->sns_port; */
	return (0);
}

ns_pcbdisconnect(nsp)
	struct nspcb *nsp;
{

	nsp->nsp_faddr = zerons_addr;
	if (nsp->nsp_socket->so_state & SS_NOFDREF)
		ns_pcbdetach(nsp);
}

ns_pcbdetach(nsp)
	struct nspcb *nsp;
{
	struct socket *so = nsp->nsp_socket;

	so->so_pcb = 0;
	sofree(so);
	if (nsp->nsp_route.ro_rt)
		rtfree(nsp->nsp_route.ro_rt);
	remque(nsp);
	(void) m_free(dtom(nsp));
}

ns_setsockaddr(nsp, nam)
	register struct nspcb *nsp;
	struct mbuf *nam;
{
	register struct sockaddr_ns *sns = mtod(nam, struct sockaddr_ns *);
	
	nam->m_len = sizeof (*sns);
	sns = mtod(nam, struct sockaddr_ns *);
	bzero((caddr_t)sns, sizeof (*sns));
	sns->sns_family = AF_NS;
	sns->sns_addr = nsp->nsp_laddr;
}

ns_setpeeraddr(nsp, nam)
	register struct nspcb *nsp;
	struct mbuf *nam;
{
	register struct sockaddr_ns *sns = mtod(nam, struct sockaddr_ns *);
	
	nam->m_len = sizeof (*sns);
	sns = mtod(nam, struct sockaddr_ns *);
	bzero((caddr_t)sns, sizeof (*sns));
	sns->sns_family = AF_NS;
	sns->sns_addr  = nsp->nsp_faddr;
}

/*
 * Pass some notification to all connections of a protocol
 * associated with address dst.  Call the
 * protocol specific routine to handle each connection.
 * Also pass an extra paramter via the nspcb. (which may in fact
 * be a parameter list!)
 */
ns_pcbnotify(dst, errno, notify, param)
	register struct ns_addr *dst;
	long param;
	int errno, (*notify)();
{
	register struct nspcb *nsp, *oinp;
	int s = splimp();

	for (nsp = (&nspcb)->nsp_next; nsp != (&nspcb);) {
		if (!ns_hosteq(*dst,nsp->nsp_faddr)) {
	next:
			nsp = nsp->nsp_next;
			continue;
		}
		if (nsp->nsp_socket == 0)
			goto next;
		if (errno) 
			nsp->nsp_socket->so_error = errno;
		oinp = nsp;
		nsp = nsp->nsp_next;
		oinp->nsp_notify_param = param;
		(*notify)(oinp);
	}
	splx(s);
}

/*
 * After a routing change, flush old routing
 * and allocate a (hopefully) better one.
 */
ns_rtchange(nsp)
	struct nspcb *nsp;
{
	if (nsp->nsp_route.ro_rt) {
		rtfree(nsp->nsp_route.ro_rt);
		nsp->nsp_route.ro_rt = 0;
		/*
		 * A new route can be allocated the next time
		 * output is attempted.
		 */
	}
	/* SHOULD NOTIFY HIGHER-LEVEL PROTOCOLS */
}

struct nspcb *
ns_pcblookup(faddr, lport, wildp)
	struct ns_addr *faddr;
	u_short lport;
{
	register struct nspcb *nsp, *match = 0;
	int matchwild = 3, wildcard;
	u_short fport;

	fport = faddr->x_port;
	for (nsp = (&nspcb)->nsp_next; nsp != (&nspcb); nsp = nsp->nsp_next) {
		if (nsp->nsp_lport != lport)
			continue;
		wildcard = 0;
		if (ns_nullhost(nsp->nsp_faddr)) {
			if (!ns_nullhost(*faddr))
				wildcard++;
		} else {
			if (ns_nullhost(*faddr))
				wildcard++;
			else {
				if (!ns_hosteq(nsp->nsp_faddr, *faddr))
					continue;
				if( nsp->nsp_fport != fport) {
					if(nsp->nsp_fport != 0)
						continue;
					else
						wildcard++;
				}
			}
		}
		if (wildcard && wildp==0)
			continue;
		if (wildcard < matchwild) {
			match = nsp;
			matchwild = wildcard;
			if (wildcard == 0)
				break;
		}
	}
	return (match);
}
