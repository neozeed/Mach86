/*
 * Copyright (c) 1984, 1985, 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)ns_error.c	7.1 (Berkeley) 6/5/86
 */

#include "param.h"
#include "systm.h"
#include "mbuf.h"
#include "protosw.h"
#include "socket.h"
#include "time.h"
#include "kernel.h"

#include "../net/route.h"

#include "ns.h"
#include "ns_pcb.h"
#include "idp.h"
#include "ns_error.h"

#ifdef lint
#define NS_ERRPRINTFS 1
#endif

#ifdef NS_ERRPRINTFS
/*
 * NS_ERR routines: error generation, receive packet processing, and
 * routines to turnaround packets back to the originator.
 */
int	ns_errprintfs = 0;
#endif

/*
 * Generate an error packet of type error
 * in response to bad packet.
 */

ns_error(om, type, param)
	struct mbuf *om;
	int type;
{
	register struct ns_epidp *ep;
	struct mbuf *m;
	struct idp *nip;
	register struct idp *oip = mtod(om, struct idp *);
	extern int idpcksum;

	/*
	 * If this packet was sent to the echo port,
	 * and nobody was there, just echo it.
	 * (Yes, this is a wart!)
	 */
	if (type==NS_ERR_NOSOCK &&
	    oip->idp_dna.x_port==htons(2) &&
	    (type = ns_echo(oip)==0))
		return;

#ifdef NS_ERRPRINTFS
	if (ns_errprintfs)
		printf("ns_err_error(%x, %d, %d)\n", oip, type, param);
#endif
	/*
	 * Don't Generate error packets in response to multicasts.
	 */
	if (oip->idp_dna.x_host.c_host[0] & 1)
		goto free;

	ns_errstat.ns_es_error++;
	/*
	 * Make sure that the old IDP packet had 30 bytes of data to return;
	 * if not, don't bother.  Also don't EVER error if the old
	 * packet protocol was NS_ERR.
	 */
	if (oip->idp_len < sizeof(struct idp)) {
		ns_errstat.ns_es_oldshort++;
		goto free;
	}
	if (oip->idp_pt == NSPROTO_ERROR) {
		ns_errstat.ns_es_oldns_err++;
		goto free;
	}

	/*
	 * First, formulate ns_err message
	 */
	m = m_get(M_DONTWAIT, MT_HEADER);
	if (m == NULL)
		goto free;
	m->m_len = sizeof(*ep);
	m->m_off = MMAXOFF - m->m_len;
	ep = mtod(m, struct ns_epidp *);
	if ((u_int)type > NS_ERR_TOO_BIG)
		panic("ns_err_error");
	ns_errstat.ns_es_outhist[ns_err_x(type)]++;
	ep->ns_ep_errp.ns_err_num = htons((u_short)type);
	ep->ns_ep_errp.ns_err_param = htons((u_short)param);
	bcopy((caddr_t)oip, (caddr_t)&ep->ns_ep_errp.ns_err_idp, 42);
	nip = &ep->ns_ep_idp;
	nip->idp_len = sizeof(*ep);
	nip->idp_len = htons((u_short)nip->idp_len);
	nip->idp_pt = NSPROTO_ERROR;
	nip->idp_tc = 0;
	nip->idp_dna = oip->idp_sna;
	nip->idp_sna = oip->idp_dna;
	if (idpcksum) {
		nip->idp_sum = 0;
		nip->idp_sum = ns_cksum(dtom(nip), sizeof(*ep));
	} else 
		nip->idp_sum = 0xffff;
	(void) ns_output(dtom(nip), (struct route *)0, 0);

free:
	m_freem(dtom(oip));
}

ns_printhost(p)
register struct ns_addr *p;
{

	printf("<net:%x%x,host:%x%x%x,port:%x>",
			p->x_net.s_net[0],
			p->x_net.s_net[1],
			p->x_host.s_host[0],
			p->x_host.s_host[1],
			p->x_host.s_host[2],
			p->x_port);

}

/*
 * Process a received NS_ERR message.
 */
ns_err_input(m)
	struct mbuf *m;
{
	register struct ns_errp *ep;
	register struct ns_epidp *epidp = mtod(m, struct ns_epidp *);
	register int i;
	int type, code, param;

	/*
	 * Locate ns_err structure in mbuf, and check
	 * that not corrupted and of at least minimum length.
	 */
#ifdef NS_ERRPRINTFS
	if (ns_errprintfs) {
		printf("ns_err_input from ");
		ns_printhost(&epidp->ns_ep_idp.idp_sna);
		printf("len %d\n", ntohs(epidp->ns_ep_idp.idp_len));
	}
#endif
	i = sizeof (struct ns_epidp);
 	if ((m->m_off > MMAXOFF || m->m_len < i) &&
 		(m = m_pullup(m, i)) == 0)  {
		ns_errstat.ns_es_tooshort++;
		return;
	}
	ep = &(mtod(m, struct ns_epidp *)->ns_ep_errp);
	type = ntohs(ep->ns_err_num);
	param = ntohs(ep->ns_err_param);
	ns_errstat.ns_es_inhist[ns_err_x(type)]++;

#ifdef NS_ERRPRINTFS
	/*
	 * Message type specific processing.
	 */
	if (ns_errprintfs)
		printf("ns_err_input, type %d param %d\n", type, param);
#endif
	if (type >= NS_ERR_TOO_BIG) {
		goto badcode;
	}
	ns_errstat.ns_es_outhist[ns_err_x(type)]++;
	switch (type) {

	case NS_ERR_UNREACH_HOST:
		code = PRC_UNREACH_NET;
		goto deliver;

	case NS_ERR_TOO_OLD:
		code = PRC_TIMXCEED_INTRANS;
		goto deliver;

	case NS_ERR_TOO_BIG:
		code = PRC_MSGSIZE;
		goto deliver;

	case NS_ERR_FULLUP:
		code = PRC_QUENCH;
		goto deliver;

	case NS_ERR_NOSOCK:
		code = PRC_UNREACH_PORT;
		goto deliver;

	case NS_ERR_UNSPEC_T:
	case NS_ERR_BADSUM_T:
	case NS_ERR_BADSUM:
	case NS_ERR_UNSPEC:
		code = PRC_PARAMPROB;
		goto deliver;

	deliver:
		/*
		 * Problem with datagram; advise higher level routines.
		 */
#ifdef NS_ERRPRINTFS
		if (ns_errprintfs)
			printf("deliver to protocol %d\n",
				       ep->ns_err_idp.idp_pt);
#endif
		switch(ep->ns_err_idp.idp_pt) {
		case NSPROTO_SPP:
			spp_ctlinput(code, (caddr_t)ep);
			break;

		default:
			idp_ctlinput(code, (caddr_t)ep);
		}
		
		goto free;

	default:
	badcode:
		ns_errstat.ns_es_badcode++;
		goto free;

	}
free:
	m_freem(m);
}

#ifdef notdef
u_long
nstime()
{
	int s = spl6();
	u_long t;

	t = (time.tv_sec % (24*60*60)) * 1000 + time.tv_usec / 1000;
	splx(s);
	return (htonl(t));
}
#endif

ns_echo(idp)
register struct idp *idp;
{
	struct mbuf *m = dtom(idp);
	register struct echo {
	    struct idp	ec_idp;
	    u_short		ec_op; /* Operation, 1 = request, 2 = reply */
	} *ec = (struct echo *)idp;
	struct ns_addr temp;

	if (idp->idp_pt!=NSPROTO_ECHO) return(NS_ERR_NOSOCK);
	if (ec->ec_op!=htons(1)) return(NS_ERR_UNSPEC);

	ec->ec_op = htons(2);

	temp = idp->idp_dna;
	idp->idp_dna = idp->idp_sna;
	idp->idp_sna = temp;

	if (idp->idp_sum != 0xffff) {
		idp->idp_sum = 0;
		idp->idp_sum = ns_cksum(m,
		    (int)(((ntohs(idp->idp_len) - 1)|1)+1));
	}
	(void) ns_output(m, (struct route *)0, NS_FORWARDING);
	return(0);
}
