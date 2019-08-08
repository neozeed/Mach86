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
 *	@(#)ns_output.c	6.6 (Berkeley) 9/13/85
 */

#include "param.h"
#include "mbuf.h"
#include "errno.h"
#include "socket.h"
#include "socketvar.h"

#include "../net/if.h"
#include "../net/route.h"

#include "ns.h"
#include "ns_if.h"
#include "idp.h"
#include "idp_var.h"

#ifdef vax
#include "../vax/mtpr.h"
#endif
int ns_hold_output = 0;
int ns_copy_output = 0;
int ns_output_cnt = 0;
struct mbuf *ns_lastout;

ns_output(m0, ro, flags)
	struct mbuf *m0;
	struct route *ro;
	int flags;
{
	register struct idp *idp = mtod(m0, struct idp *);
	register struct ifnet *ifp;
	int error = 0;
	struct route idproute;
	struct sockaddr_ns *dst;
	extern int idpcksum;

	if (ns_hold_output) {
		if (ns_lastout) {
			m_free(ns_lastout);
		}
		ns_lastout = m_copy(m0, 0, (int)M_COPYALL);
	}
	if (ns_copy_output) {
		ns_watch_output(m0);
	}

	/*
	 * Route packet.
	 */
	if (ro == 0) {
		ro = &idproute;
		bzero((caddr_t)ro, sizeof (*ro));
	}
	dst = (struct sockaddr_ns *)&ro->ro_dst;
	if (ro->ro_rt == 0) {
		dst->sns_family = AF_NS;
		dst->sns_addr = idp->idp_dna;
		dst->sns_addr.x_port = 0;
		/*
		 * If routing to interface only,
		 * short circuit routing lookup.
		 */
		if (flags & NS_ROUTETOIF) {
			struct ns_ifaddr *ia;
			struct ifaddr *ifa_ifwithdstaddr();

			ia = ns_iaonnetof(idp->idp_dna.x_net);
			if (ia == 0) {
				error = ENETUNREACH;
				goto bad;
			}
			ifp = ia->ia_ifp;
			if (ifp->if_flags & IFF_POINTOPOINT) {
				ia = (struct ns_ifaddr *)
					ifa_ifwithdstaddr(&ro->ro_dst);
				if (ia == 0) {
					error = ENETUNREACH;
					goto bad;
				}
				ifp = ia->ia_ifp;
			}
			goto gotif;
		}
		rtalloc(ro);
	} else if ((ro->ro_rt->rt_flags & RTF_UP) == 0) {
		/*
		 * The old route has gone away; try for a new one.
		 */
		rtfree(ro->ro_rt);
		rtalloc(ro);
	}
	if (ro->ro_rt == 0 || (ifp = ro->ro_rt->rt_ifp) == 0) {
		error = ENETUNREACH;
		goto bad;
	}
	ro->ro_rt->rt_use++;
	if (ro->ro_rt->rt_flags & (RTF_GATEWAY|RTF_HOST))
		dst = (struct sockaddr_ns *)&ro->ro_rt->rt_gateway;
gotif:

	/*
	 * Look for multicast addresses and
	 * and verify user is allowed to send
	 * such a packet.
	 */
	if (dst->sns_addr.x_host.c_host[0]&1) {
		if ((ifp->if_flags & IFF_BROADCAST) == 0) {
			error = EADDRNOTAVAIL;
			goto bad;
		}
		if ((flags & NS_ALLOWBROADCAST) == 0) {
			error = EACCES;
			goto bad;
		}
	}

	if (htons(idp->idp_len) <= ifp->if_mtu) {
		ns_output_cnt++;
		error = (*ifp->if_output)(ifp, m0, (struct sockaddr *)dst);
		goto done;
	} else error = EMSGSIZE;


bad:
	m_freem(m0);
done:
	if (ro == &idproute && (flags & NS_ROUTETOIF) == 0 && ro->ro_rt)
		RTFREE(ro->ro_rt);
	return (error);
}
