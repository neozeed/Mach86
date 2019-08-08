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
 *	@(#)ns_input.c	6.10 (Berkeley) 9/6/85
 */

#include "param.h"
#include "systm.h"
#include "mbuf.h"
#include "domain.h"
#include "protosw.h"
#include "socket.h"
#include "socketvar.h"
#include "errno.h"
#include "time.h"
#include "kernel.h"

#include "../net/if.h"
#include "../net/route.h"
#include "../net/raw_cb.h"

#include "ns.h"
#include "ns_if.h"
#include "ns_pcb.h"
#include "idp.h"
#include "idp_var.h"
#include "ns_error.h"

/*
 * NS initialization.
 */
union ns_host	ns_thishost;
union ns_host	ns_zerohost;
union ns_host	ns_broadhost;

static char allones[] = {-1, -1, -1, -1, -1, -1};

struct nspcb nspcb;
struct nspcb nsrawpcb;

struct ifqueue	nsintrq;
int	nsqmaxlen = IFQ_MAXLEN;

int	idpcksum = 1;
long	ns_pexseq;

ns_init()
{
	extern struct timeval time;

	ns_broadhost = * (union ns_host *) allones;
	nspcb.nsp_next = nspcb.nsp_prev = &nspcb;
	nsrawpcb.nsp_next = nsrawpcb.nsp_prev = &nsrawpcb;
	nsintrq.ifq_maxlen = nsqmaxlen;
	ns_pexseq = time.tv_usec;
}

/*
 * Idp input routine.  Pass to next level.
 */
int nsintr_getpck = 0;
int nsintr_swtch = 0;
nsintr()
{
	register struct idp *idp;
	register struct mbuf *m;
	register struct nspcb *nsp;
	struct ifnet *ifp;
	struct mbuf *m0;
	register int i;
	int len, s, error;
	char oddpacketp;

next:
	/*
	 * Get next datagram off input queue and get IDP header
	 * in first mbuf.
	 */
	s = splimp();
	IF_DEQUEUEIF(&nsintrq, m, ifp);
	splx(s);
	nsintr_getpck++;
	if (m == 0)
		return;
	if ((m->m_off > MMAXOFF || m->m_len < sizeof (struct idp)) &&
	    (m = m_pullup(m, sizeof (struct idp))) == 0) {
		idpstat.idps_toosmall++;
		goto next;
	}

	/*
	 * Give any raw listeners a crack at the packet
	 */
	for (nsp = nsrawpcb.nsp_next; nsp != &nsrawpcb; nsp = nsp->nsp_next) {
		struct mbuf *m1 = m_copy(m, 0, (int)M_COPYALL);
		if (m1) idp_input(m1, nsp, ifp);
	}

	idp = mtod(m, struct idp *);
	len = ntohs(idp->idp_len);
	if (oddpacketp = len & 1) {
		len++;		/* If this packet is of odd length,
				   preserve garbage byte for checksum */
	}

	/*
	 * Check that the amount of data in the buffers
	 * is as at least much as the IDP header would have us expect.
	 * Trim mbufs if longer than we expect.
	 * Drop packet if shorter than we expect.
	 */
	i = -len;
	m0 = m;
	for (;;) {
		i += m->m_len;
		if (m->m_next == 0)
			break;
		m = m->m_next;
	}
	if (i != 0) {
		if (i < 0) {
			idpstat.idps_tooshort++;
			m = m0;
			goto bad;
		}
		if (i <= m->m_len)
			m->m_len -= i;
		else
			m_adj(m0, -i);
	}
	m = m0;
	if (idpcksum && ((i = idp->idp_sum)!=0xffff)) {
		idp->idp_sum = 0;
		if (i != (idp->idp_sum = ns_cksum(m,len))) {
			idpstat.idps_badsum++;
			idp->idp_sum = i;
			if (ns_hosteqnh(ns_thishost, idp->idp_dna.x_host))
				error = NS_ERR_BADSUM;
			else
				error = NS_ERR_BADSUM_T;
			ns_error(m, error, 0);
			goto next;
		}
	}
	/*
	 * Is this a directed broadcast?
	 */
	if (ns_hosteqnh(ns_broadhost,idp->idp_dna.x_host)) {
		if ((ns_netof(idp->idp_dna)!=ns_netof(idp->idp_sna)) &&
		   (ns_netof(idp->idp_dna)!=-1) && (ns_netof(idp->idp_sna)!=0)
		   && (ns_netof(idp->idp_dna)!=0)) {
			/*
			 * Look to see if I need to eat this packet.
			 * Algorithm is to forward all young packets
			 * and prematurely age any packets which will
			 * by physically broadcasted.
			 * Any very old packets eaten without forwarding
			 * would die anyway.
			 *
			 * Suggestion of Bill Nesheim, Cornell U.
			 */
			if (idp->idp_tc < NS_MAXHOPS) {
				idp_forward(idp);
				goto next;
			}
		}
	/*
	 * Is this our packet? If not, forward.
	 */
	} else if (!ns_hosteqnh(ns_thishost,idp->idp_dna.x_host)) {
		idp_forward(idp);
		goto next;
	}
	/*
	 * Locate pcb for datagram.
	 */
	nsp = ns_pcblookup(&idp->idp_sna, idp->idp_dna.x_port, NS_WILDCARD);
	/*
	 * Switch out to protocol's input routine.
	 */
	nsintr_swtch++;
	if (nsp) {
		if (oddpacketp) {
			m_adj(m0, -1);
		}
		if ((nsp->nsp_flags & NSP_ALL_PACKETS)==0)
			switch (idp->idp_pt) {

			    case NSPROTO_SPP:
				    spp_input(m, nsp, ifp);
				    goto next;

			    case NSPROTO_ERROR:
				    ns_err_input(m);
				    goto next;
			}
		idp_input(m, nsp, ifp);
	} else {
		ns_error(m, NS_ERR_NOSOCK, 0);
	}
	goto next;

bad:
	m_freem(m);
	goto next;
}

u_char nsctlerrmap[PRC_NCMDS] = {
	ECONNABORTED,	ECONNABORTED,	0,		0,
	0,		0,		EHOSTDOWN,	EHOSTUNREACH,
	ENETUNREACH,	EHOSTUNREACH,	ECONNREFUSED,	ECONNREFUSED,
	EMSGSIZE,	0,		0,		0,
	0,		0,		0,		0
};

idp_donosocks = 1;

idp_ctlinput(cmd, arg)
	int cmd;
	caddr_t arg;
{
	struct ns_addr *ns;
	struct nspcb *nsp;
	struct ns_errp *errp;
	int idp_abort();
	extern struct nspcb *idp_drop();
	int type;

	if (cmd < 0 || cmd > PRC_NCMDS)
		return;
	if (nsctlerrmap[cmd] == 0)
		return;		/* XXX */
	type = NS_ERR_UNREACH_HOST;
	switch (cmd) {
		struct sockaddr_ns *sns;

	case PRC_IFDOWN:
	case PRC_HOSTDEAD:
	case PRC_HOSTUNREACH:
		sns = (struct sockaddr_ns *)arg;
		if (sns->sns_family != AF_INET)
			return;
		ns = &sns->sns_addr;
		break;

	default:
		errp = (struct ns_errp *)arg;
		ns = &errp->ns_err_idp.idp_dna;
		type = errp->ns_err_num;
		type = ntohs((u_short)type);
	}
	switch (type) {

	case NS_ERR_UNREACH_HOST:
		ns_pcbnotify(ns, (int)nsctlerrmap[cmd], idp_abort, (long)0);
		break;

	case NS_ERR_NOSOCK:
		nsp = ns_pcblookup(ns, errp->ns_err_idp.idp_sna.x_port,
			NS_WILDCARD);
		if(nsp && idp_donosocks && ! ns_nullhost(nsp->nsp_faddr))
			(void) idp_drop(nsp, (int)nsctlerrmap[cmd]);
	}
}

int	idpprintfs = 0;
int	idpforwarding = 1;
/*
 * Forward a packet.  If some error occurs return the sender
 * an error packet.  Note we can't always generate a meaningful
 * error message because the NS errors don't have a large enough repetoire
 * of codes and types.
 */
struct route idp_droute;
struct route idp_sroute;

idp_forward(idp)
	register struct idp *idp;
{
	register int error, type, code;
	struct mbuf *mcopy = NULL;
	int agedelta = 1;
	int flags = NS_FORWARDING;
	int ok_there = 0;
	int ok_back = 0;

	if (idpprintfs) {
		printf("forward: src ");
		ns_printhost(&idp->idp_sna);
		printf(", dst ");
		ns_printhost(&idp->idp_dna);
		printf("hop count %d\n", idp->idp_tc);
	}
	if (idpforwarding == 0) {
		/* can't tell difference between net and host */
		type = NS_ERR_UNREACH_HOST, code = 0;
		goto senderror;
	}
	idp->idp_tc++;
	if (idp->idp_tc > NS_MAXHOPS) {
		type = NS_ERR_TOO_OLD, code = 0;
		goto senderror;
	}
	/*
	 * Save at most 42 bytes of the packet in case
	 * we need to generate an NS error message to the src.
	 */
	mcopy = m_copy(dtom(idp), 0, imin(ntohs(idp->idp_len), 42));

	if ((ok_there = idp_do_route(&idp->idp_dna,&idp_droute))==0) {
		type = NS_ERR_UNREACH_HOST, code = 0;
		goto senderror;
	}
	/*
	 * Here we think about  forwarding  broadcast packets,
	 * so we try to insure that it doesn't go back out
	 * on the interface it came in on.  Also, if we
	 * are going to physically broadcast this, let us
	 * age the packet so we can eat it safely the second time around.
	 */
	if (idp->idp_dna.x_host.c_host[0] & 0x1) {
		struct ns_ifaddr *ia = ns_iaonnetof(idp->idp_dna.x_net);
		struct ifnet *ifp;
		if (ia) {
			/* I'm gonna hafta eat this packet */
			agedelta += NS_MAXHOPS - idp->idp_tc;
			idp->idp_tc = NS_MAXHOPS;
		}
		if ((ok_back = idp_do_route(&idp->idp_sna,&idp_sroute))==0) {
			/* error = ENETUNREACH; He'll never get it! */
			m_freem(dtom(idp));
			goto cleanup;
		}
		if (idp_droute.ro_rt &&
		    (ifp=idp_droute.ro_rt->rt_ifp) &&
		    idp_sroute.ro_rt &&
		    (ifp!=idp_sroute.ro_rt->rt_ifp)) {
			flags |= NS_ALLOWBROADCAST;
		} else {
			type = NS_ERR_UNREACH_HOST, code = 0;
			goto senderror;
		}
	}
	/* need to adjust checksum */
	if (idp->idp_sum!=0xffff) {
		union bytes {
			u_char c[4];
			u_short s[2];
			long l;
		} x;
		register int shift;
		x.l = 0; x.c[0] = agedelta;
		shift = (((((int)ntohs(idp->idp_len))+1)>>1)-2) & 0xf;
		x.l = idp->idp_sum + (x.l << shift);
		x.l = x.s[0] + x.s[1];
		x.l = x.s[0] + x.s[1];
		if (x.l==0xffff) idp->idp_sum = 0; else idp->idp_sum = x.l;
	}
	if ((error = ns_output(dtom(idp), &idp_droute, flags)) && 
	    (mcopy!=NULL)) {
		idp = mtod(mcopy, struct idp *);
		type = NS_ERR_UNSPEC_T, code = 0;
		switch (error) {

		case ENETUNREACH:
		case EHOSTDOWN:
		case EHOSTUNREACH:
		case ENETDOWN:
		case EPERM:
			type = NS_ERR_UNREACH_HOST;
			break;

		case EMSGSIZE:
			type = NS_ERR_TOO_BIG;
			code = 576; /* too hard to figure out mtu here */
			break;

		case ENOBUFS:
			type = NS_ERR_UNSPEC_T;
			break;
		}
		mcopy = NULL;
	senderror:
		ns_error(dtom(idp), type, code);
	}
cleanup:
	if (ok_there)
		idp_undo_route(&idp_droute);
	if (ok_back)
		idp_undo_route(&idp_sroute);
	if (mcopy != NULL)
		m_freem(mcopy);
}

idp_do_route(src, ro)
struct ns_addr *src;
struct route *ro;
{
	
	struct sockaddr_ns *dst;

	bzero((caddr_t)ro, sizeof (*ro));
	dst = (struct sockaddr_ns *)&ro->ro_dst;

	dst->sns_family = AF_NS;
	dst->sns_addr = *src;
	dst->sns_addr.x_port = 0;
	rtalloc(ro);
	if (ro->ro_rt == 0 || ro->ro_rt->rt_ifp == 0) {
		return (0);
	}
	ro->ro_rt->rt_use++;
	return (1);
}

idp_undo_route(ro)
register struct route *ro;
{
	if (ro->ro_rt) {RTFREE(ro->ro_rt);}
}
ns_watch_output(m)
struct mbuf *m;
{
	register struct nspcb *nsp;
	/*
	 * Give any raw listeners a crack at the packet
	 */
	for (nsp = nsrawpcb.nsp_next; nsp != &nsrawpcb; nsp = nsp->nsp_next) {
		struct mbuf *m1 = m_copy(m, 0, (int)M_COPYALL);
		if (m1) idp_input(m1, nsp);
	}
}
