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
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)raw_cb.c	6.7 (Berkeley) 9/16/85
 */
#if	CMU
/*
 **********************************************************************
 * HISTORY
 * 25-Jan-86  Avadis Tevanian (avie) at Carnegie-Mellon University
 *	Upgraded to 4.3.
 *
 * 01-Aug-85  Mike Accetta (mja) at Carnegie-Mellon University
 *	CS_LINT: cast additions for lint.
 *
 **********************************************************************
 */
 
#include "cs_lint.h"
#endif	CMU

#include "param.h"
#include "systm.h"
#include "mbuf.h"
#include "socket.h"
#include "socketvar.h"
#include "domain.h"
#include "protosw.h"
#include "errno.h"

#include "if.h"
#include "route.h"
#include "raw_cb.h"
#include "../netinet/in.h"
#ifdef PUP
#include "../netpup/pup.h"
#endif

#ifdef	CS_GENERIC || romp
#else	CS_GENERIC
#include "../vax/mtpr.h"
#endif	CS_GENERIC
/*
 * Routines to manage the raw protocol control blocks. 
 *
 * TODO:
 *	hash lookups by protocol family/protocol + address family
 *	take care of unique address problems per AF?
 *	redo address binding to allow wildcards
 */

/*
 * Allocate a control block and a nominal amount
 * of buffer space for the socket.
 */
raw_attach(so, proto)
	register struct socket *so;
	int proto;
{
	struct mbuf *m;
	register struct rawcb *rp;

	m = m_getclr(M_DONTWAIT, MT_PCB);
	if (m == 0)
		return (ENOBUFS);
	if (sbreserve(&so->so_snd, RAWSNDQ) == 0)
		goto bad;
	if (sbreserve(&so->so_rcv, RAWRCVQ) == 0)
		goto bad2;
	rp = mtod(m, struct rawcb *);
	rp->rcb_socket = so;
	so->so_pcb = (caddr_t)rp;
	rp->rcb_pcb = 0;
	rp->rcb_proto.sp_family = so->so_proto->pr_domain->dom_family;
	rp->rcb_proto.sp_protocol = proto;
	insque(rp, &rawcb);
	return (0);
bad2:
	sbrelease(&so->so_snd);
bad:
	(void) m_free(m);
	return (ENOBUFS);
}

/*
 * Detach the raw connection block and discard
 * socket resources.
 */
raw_detach(rp)
	register struct rawcb *rp;
{
	struct socket *so = rp->rcb_socket;

	if (rp->rcb_route.ro_rt)
		rtfree(rp->rcb_route.ro_rt);
	so->so_pcb = 0;
	sofree(so);
	remque(rp);
	m_freem(dtom(rp));
}

/*
 * Disconnect and possibly release resources.
 */
raw_disconnect(rp)
	struct rawcb *rp;
{

	rp->rcb_flags &= ~RAW_FADDR;
	if (rp->rcb_socket->so_state & SS_NOFDREF)
		raw_detach(rp);
}

raw_bind(so, nam)
	register struct socket *so;
	struct mbuf *nam;
{
	struct sockaddr *addr = mtod(nam, struct sockaddr *);
	register struct rawcb *rp;

	if (ifnet == 0)
		return (EADDRNOTAVAIL);
/* BEGIN DUBIOUS */
	/*
	 * Should we verify address not already in use?
	 * Some say yes, others no.
	 */
	switch (addr->sa_family) {

#if defined(INET) || defined(BBNNET)
	case AF_IMPLINK:
	case AF_INET: {
		if (((struct sockaddr_in *)addr)->sin_addr.s_addr &&
		    ifa_ifwithaddr(addr) == 0)
			return (EADDRNOTAVAIL);
		break;
	}
#endif

#ifdef PUP
	/*
	 * Curious, we convert PUP address format to internet
	 * to allow us to verify we're asking for an Ethernet
	 * interface.  This is wrong, but things are heavily
	 * oriented towards the internet addressing scheme, and
	 * converting internet to PUP would be very expensive.
	 */
	case AF_PUP: {
		struct sockaddr_pup *spup = (struct sockaddr_pup *)addr;
		struct sockaddr_in inpup;

		bzero((caddr_t)&inpup, (unsigned)sizeof(inpup));
		inpup.sin_family = AF_INET;
#if	CS_LINT
		inpup.sin_addr = in_makeaddr((int)spup->spup_net,
				(int)spup->spup_host);
#else	CS_LINT
		inpup.sin_addr = in_makeaddr(spup->spup_net, spup->spup_host);
#endif	CS_LINT
		if (inpup.sin_addr.s_addr &&
		    ifa_ifwithaddr((struct sockaddr *)&inpup) == 0)
			return (EADDRNOTAVAIL);
		break;
	}
#endif

	default:
		return (EAFNOSUPPORT);
	}
/* END DUBIOUS */
	rp = sotorawcb(so);
	bcopy((caddr_t)addr, (caddr_t)&rp->rcb_laddr, sizeof (*addr));
	rp->rcb_flags |= RAW_LADDR;
	return (0);
}

/*
 * Associate a peer's address with a
 * raw connection block.
 */
raw_connaddr(rp, nam)
	struct rawcb *rp;
	struct mbuf *nam;
{
	struct sockaddr *addr = mtod(nam, struct sockaddr *);

	bcopy((caddr_t)addr, (caddr_t)&rp->rcb_faddr, sizeof(*addr));
	rp->rcb_flags |= RAW_FADDR;
}
