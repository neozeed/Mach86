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
 *	@(#)if_ether.c	6.13 (Berkeley) 9/16/85
 */

#if	CMU
/*
 **********************************************************************
 * HISTORY
 * 17-Feb-86  Bill Bolosky (bolosky) at Carnegie-Mellon University
 *	Added IBM code for Sailboat under switch romp.
 *
 * 25-Jan-86  Avadis Tevanian (avie) at Carnegie-Mellon University
 *	Upgraded to 4.3.
 *
 * 13-Aug-85  Mike Accetta (mja) at Carnegie-Mellon University
 *	CS_INET:  Parameterized to support multiple ethernet types and
 *	allow multiple heterogenous interfaces to co-exist.
 *	NEN:  Added support for 3Mb ethernet.
 *	[V1(1)]
 *
 * 27-Jun-85  Mike Accetta (mja) at Carnegie-Mellon University
 *	CS_INET: disabled silly ARP bypass by local host address check
 *	by setting local host upper limit above the maximum legal local
 *	host.
 *	[V1(1)].
 *
 **********************************************************************
 */
 
#include "cs_bugfix.h"
#include "cs_inet.h"
 
#include "en.h"
#endif	CMU

/*
 * Ethernet address resolution protocol.
 */

#include "param.h"
#include "systm.h"
#include "mbuf.h"
#include "socket.h"
#include "time.h"
#include "kernel.h"
#include "errno.h"
#include "ioctl.h"

#include "../net/if.h"
#include "in.h"
#include "in_systm.h"
#include "ip.h"
#include "if_ether.h"
#if	NEN > 0
#include "../vaxif/if_en.h"
#endif	NEN

#if	CS_INET
#define	ARPTAB_BSIZ	5		/* bucket size */
#define	ARPTAB_NB	19		/* number of buckets */

int arptab_bsiz = ARPTAB_BSIZ;
int arptab_nb = ARPTAB_NB;
#else	CS_INET
#define	ARPTAB_BSIZ	9		/* bucket size */

/*
 * Internet to ethernet address resolution table.
 */
struct	arptab {
	struct	in_addr at_iaddr;	/* internet address */
	u_char	at_enaddr[6];		/* ethernet address */
	struct	mbuf *at_hold;		/* last packet until resolved/timeout */
	u_char	at_timer;		/* minutes since last reference */
	u_char	at_flags;		/* flags */
};
#define	ARPTAB_NB	19		/* number of buckets */
#endif	CS_INET
#define	ARPTAB_SIZE	(ARPTAB_BSIZ * ARPTAB_NB)
struct	arptab arptab[ARPTAB_SIZE];
int	arptab_size = ARPTAB_SIZE;	/* for arp command */

#define	ARPTAB_HASH(a) \
	((short)((((a) >> 16) ^ (a)) & 0x7fff) % ARPTAB_NB)

/*
 *	The CS_INET extensions are not yet complete.  The following two
 *	lines should be removed when they are.
 */
#undef	CS_INET
#define	CS_INET 0

#if	CS_INET
/*
 *  Change to permit multiple heterogenous interfaces to co-exist.
 */
#define	ARPTAB_LOOK(at,addr,ifp) { \
	register n; \
	at = &arptab[ARPTAB_HASH(addr) * ARPTAB_BSIZ]; \
	for (n = 0 ; n < ARPTAB_BSIZ ; n++,at++) \
		if (at->at_if == (ifp) && at->at_iaddr.s_addr == addr) \
			break; \
	if (n >= ARPTAB_BSIZ) \
		at = 0; }
#else	CS_INET
#define	ARPTAB_LOOK(at,addr) { \
	register n; \
	at = &arptab[ARPTAB_HASH(addr) * ARPTAB_BSIZ]; \
	for (n = 0 ; n < ARPTAB_BSIZ ; n++,at++) \
		if (at->at_iaddr.s_addr == addr) \
			break; \
	if (n >= ARPTAB_BSIZ) \
		at = 0; }
#endif	CS_INET

int	arpt_age;		/* aging timer */

/* timer values */
#define	ARPT_AGE	(60*1)	/* aging timer, 1 min. */
#define	ARPT_KILLC	20	/* kill completed entry in 20 mins. */
#define	ARPT_KILLI	3	/* kill incomplete entry in 3 minutes */

#if	CS_INET

#define arpisen(ac)	\
	(*((short *)(ac)->ac_if.if_name) == ((('n'<<8))+'e'))

struct ether_arp arpethertempl =
{
  
    ARPHRD_ETHER,  ETHERTYPE_IP, 6, 4, ARPOP_REQUEST,
    0x0,  0x0,  0x0,  0x0,  0x0 , 0x0 ,
    0x0,  0x0,  0x0 , 0x0,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0x0,  0x0,  0x0 , 0x0,
};
#if	NEN > 0
struct ether_arp arpentempl =
{
    ARPHRD_XETHER, ENTYPE_IP,       1, 4, ARPOP_REQUEST,
    0x0,
    0x0, 0x0, 0x0, 0x0,
    0x0,
    0x0, 0x0, 0x0, 0x0,
};
#endif	NEN > 0
#endif	CS_INET
u_char	etherbroadcastaddr[6] = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };
extern struct ifnet loif;

/*
 * Timeout routine.  Age arp_tab entries once a minute.
 */
arptimer()
{
	register struct arptab *at;
	register i;

	timeout(arptimer, (caddr_t)0, hz);
	if (++arpt_age > ARPT_AGE) {
		arpt_age = 0;
		at = &arptab[0];
		for (i = 0; i < ARPTAB_SIZE; i++, at++) {
			if (at->at_flags == 0 || (at->at_flags & ATF_PERM))
				continue;
			if (++at->at_timer < ((at->at_flags&ATF_COM) ?
			    ARPT_KILLC : ARPT_KILLI))
				continue;
			/* timer has expired, clear entry */
			arptfree(at);
		}
	}
}

/*
 * Broadcast an ARP packet, asking who has addr on interface ac.
 */
arpwhohas(ac, addr)
	register struct arpcom *ac;
	struct in_addr *addr;
{
	register struct mbuf *m;
#if	CS_INET
	register struct ether_arp *eat;
#else	CS_INET
	register struct ether_header *eh;
#endif	CS_INET
	register struct ether_arp *ea;
	struct sockaddr sa;

	if ((m = m_get(M_DONTWAIT, MT_DATA)) == NULL)
		return (1);
	m->m_len = sizeof *ea;
#if	CS_INET
#if	NEN >  0
	if (arpisen(ac))
	{
	    /*
	     *  3Mb ethernet.
	     *
	     *  The ARP packet is smaller by twice the difference in the
	     *  hardware lengths of the 3Mb ethernet from the 10Mb ethernet
	     *  (from the source and target hardware address fields).
	     */
	    eat = &arpentempl;
	    sa.sa_family = ENTYPE_ARP;	/* if_output will swap */
	    m->m_len = sizeof *ea - (2*(6-1));
        }
	else
#endif	NEN >  0
	{
	    /*
	     *  10Mb ethernet.
	     *
	     *  Note: the old code included the hardware header in
	     *  the mbuf size although it was never used here and
	     *  a new one is going to be tacked on by the output
	     *  routine.  This was probably a bug, we'll see...
	     */
	    eat = &arpethertempl;
	    sa.sa_family = ETHERTYPE_ARP;	/* if_output will swap */
	    m->m_len = sizeof *ea;
	}
	/*
	 *  Set packet type and hardware destination in socket address.  For
	 *  both 3Mb and 10Mb ethernet, the type is a short word preceded by a
	 *  destination and source hardware address.  The destination address
	 *  is actually not the first byte of the 3Mb packet header but the
	 *  socket address is interpreted by the output routine and not passed
	 *  directly to the hardware so this doesn't matter.
	 */
	*((u_short *)&sa.sa_data[2*eat->arp_hln]) = sa.sa_family;
	bcopy((caddr_t)&eat->arp_sha[eat->arp_hln+eat->arp_pln],
	      (caddr_t)sa.sa_data, eat->arp_hln);
	/* reate pointer to beginning of ARP packet. */
	m->m_off = MMAXOFF - m->m_len;
	ea = mtod(m, struct ether_arp *);
	/* copy in the template packet. */
	bcopy((caddr_t)eat, (caddr_t)ea, (unsigned)m->m_len);
	/* copy the source hardware address (from the if) */
	bcopy((caddr_t)&ac->ac_enaddr[0],
	      (caddr_t)&ea->arp_sha[0],
	      eat->arp_hln);
	/* copy the source protocol address (from the if) */
	bcopy((caddr_t)&(ac->ac_ipaddr),
	      (caddr_t)&ea->arp_sha[eat->arp_hln],
	      eat->arp_pln);
	/* keep the target hardware address (from the template) */
	/* copy the target protocol address (parameter) */
	bcopy((caddr_t)addr,
	      (caddr_t)&ea->arp_sha[2*(eat->arp_hln)+eat->arp_pln],
	      eat->arp_pln);
	/* byte swap the appopriate quantities depending on the ethernet type */
	if (eat == &arpethertempl)
	/*
	 *  10Mb ethernet - byte oriented
	 *
	 *  Swap all word quantities.
	 */
	{
	     ea->arp_hrd = htons(ea->arp_hrd);
	     ea->arp_pro = htons(ea->arp_pro);
	     ea->arp_op  = htons(ea->arp_op);
	}
        else
	/*
	 *  3Mb ethernet - word oriented
	 *
	 *  Swap all byte quantities.
	 */
	{
	    register int i;
	
	    *((u_short *)&ea->arp_hln) = htons(*((u_short *)&ea->arp_hln));
	    for (i=2*(eat->arp_pln+eat->arp_hln); i; i-=2)
	        *((u_short *)&ea->arp_sha[i-2]) = htons(*((u_short *)&ea->arp_sha[i-2]));
	}
#else	CS_INET
	m->m_len = sizeof *ea + sizeof *eh;
	m->m_off = MMAXOFF - m->m_len;
	ea = mtod(m, struct ether_arp *);
	eh = (struct ether_header *)sa.sa_data;
	bzero((caddr_t)ea, sizeof (*ea));
	bcopy((caddr_t)etherbroadcastaddr, (caddr_t)eh->ether_dhost,
	    sizeof(eh->ether_dhost));
	eh->ether_type = ETHERTYPE_ARP;		/* if_output will swap */
	ea->arp_hrd = htons(ARPHRD_ETHER);
	ea->arp_pro = htons(ETHERTYPE_IP);
	ea->arp_hln = sizeof(ea->arp_sha);	/* hardware address length */
	ea->arp_pln = sizeof(ea->arp_spa);	/* protocol address length */
	ea->arp_op = htons(ARPOP_REQUEST);
	bcopy((caddr_t)ac->ac_enaddr, (caddr_t)ea->arp_sha,
	   sizeof(ea->arp_sha));
	bcopy((caddr_t)&ac->ac_ipaddr, (caddr_t)ea->arp_spa,
	   sizeof(ea->arp_spa));
	bcopy((caddr_t)addr, (caddr_t)ea->arp_tpa, sizeof(ea->arp_tpa));
#endif	CS_INET
	sa.sa_family = AF_UNSPEC;
	return ((*ac->ac_if.if_output)(&ac->ac_if, m, &sa));
}

/*
 * Resolve an IP address into an ethernet address.  If success, 
 * desten is filled in and 1 is returned.  If there is no entry
 * in arptab, set one up and broadcast a request 
 * for the IP address;  return 0.  Hold onto this mbuf and 
 * resend it once the address is finally resolved.
 *
 * We do some (conservative) locking here at splimp, since
 * arptab is also altered from input interrupt service (ecintr/ilintr
 * calls arpinput when ETHERTYPE_ARP packets come in).
 */
arpresolve(ac, m, destip, desten)
	register struct arpcom *ac;
	struct mbuf *m;
	register struct in_addr *destip;
	register u_char *desten;
{
	register struct arptab *at;
	register struct ifnet *ifp;
	struct sockaddr_in sin;
	int s, lna;

	if (in_broadcast(*destip)) {	/* broadcast address */
#if	CS_INET
		register struct ether_arp *eat;

#if	NEN > 0
		if (arpisen(ac))
			eat = &arpentempl;
		else
#endif	NEN > 0
			eat = &arpethertempl;
		bcopy((caddr_t)&eat->arp_sha[eat->arp_hln+eat->arp_pln],
		      (caddr_t)desten,
		      eat->arp_hln);
#else	CS_INET
		bcopy((caddr_t)etherbroadcastaddr, (caddr_t)desten,
		    sizeof(etherbroadcastaddr));
		return (1);
#endif	CS_INET
	}
	lna = in_lnaof(*destip);
	ifp = &ac->ac_if;
	/* if for us, then use software loopback driver */
	if (destip->s_addr == ac->ac_ipaddr.s_addr &&
	    (loif.if_flags & IFF_UP)) {
		sin.sin_family = AF_INET;
		sin.sin_addr = *destip;
		(void) looutput(&loif, m, (struct sockaddr *)&sin);
		/*
		 * We really don't want to indicate failure,
		 * but the packet has already been sent and freed.
		 */
		return (0);
	}
	s = splimp();
#if	CS_INET
	ARPTAB_LOOK(at, destip->s_addr, ifp);
#else	CS_INET
	ARPTAB_LOOK(at, destip->s_addr);
#endif	CS_INET
	if (at == 0) {			/* not found */
#if	CS_INET
		at = arptnew(ifp, destip);
#else	CS_INET
		at = arptnew(destip);
#endif	CS_INET
		at->at_hold = m;
		arpwhohas(ac, destip);
		splx(s);
		return (0);
	}
	at->at_timer = 0;		/* restart the timer */
	if (at->at_flags & ATF_COM) {	/* entry IS complete */
		bcopy((caddr_t)at->at_enaddr, (caddr_t)desten, 6);
		splx(s);
		return (1);
	}
	/*
	 * There is an arptab entry, but no ethernet address
	 * response yet.  Replace the held mbuf with this
	 * latest one.
	 */
	if (at->at_hold)
		m_freem(at->at_hold);
	at->at_hold = m;
	arpwhohas(ac, destip);		/* ask again */
	splx(s);
	return (0);
}

/*
 * Called from 10 Mb/s Ethernet interrupt handlers
 * when ether packet type ETHERTYPE_ARP
 * is received.  Algorithm is that given in RFC 826.
 * In addition, a sanity check is performed on the sender
 * protocol address, to catch impersonators.
 */
arpinput(ac, m)
	register struct arpcom *ac;
	struct mbuf *m;
{
	register struct ether_arp *ea;
#if	CS_INET
	register struct ether_arp *eat;
#else	CS_INET
	struct ether_header *eh;
#endif	CS_INET
	register struct arptab *at;  /* same as "merge" flag */
	struct sockaddr_in sin;
	struct sockaddr sa;
	struct in_addr isaddr,itaddr,myaddr;

#if	CS_INET
#if	NEN > 0
	if (arpisen(ac))
	{
		eat = &arpentempl;
		sa.sa_family = ENTYPE_ARP;
	}
	else
#endif	NEN > 0
	{
		eat = &arpethertempl;
		sa.sa_family = ETHERTYPE_ARP;
	}
	if (m->m_len < (sizeof ea->arp_hrd +
		        sizeof ea->arp_hln +
			sizeof ea->arp_pln +
			sizeof ea->arp_pro +
			sizeof ea->arp_op  +
			2*(eat->arp_hln+eat->arp_pln)))
	myaddr = ac->ac_ipaddr;
	ea = mtod(m, struct ether_arp *);
	if (eat == &arpethertempl)
	{
		/* we don't use HRD */
		ea->arp_pro = ntohs(ea->arp_pro);
		ea->arp_op  = ntohs(ea->arp_op);
	}
	else
	{
		register int i;
	    
		/*
		 *  Note: The HLN and PLN fields were used above before being
		 *  swapped but since they were used together this still
		 *  worked.
		 */
		*((u_short *)&ea->arp_hln) = htons(*((u_short *)&ea->arp_hln));
		for (i=2*(eat->arp_hln+eat->arp_pln); i; i-=2)
			*((u_short *)(&ea->arp_sha[i-2])) = ntohs(*((u_short *)(&ea->arp_sha[i-2])));
		
	}
	if (ea->arp_pro != eat->arp_pro ||
	    ea->arp_hln != eat->arp_hln ||
	    ea->arp_pln != eat->arp_pln)
		goto out;
	isaddr.s_addr = ((struct in_addr *)&ea->arp_sha[eat->arp_hln])->s_addr;
	itaddr.s_addr = ((struct in_addr *)&ea->arp_sha[eat->arp_hln*2+eat->arp_pln])->s_addr;
	if (!bcmp((caddr_t)ea->arp_sha, (caddr_t)ac->ac_enaddr,
	  eat->arp_hln))
		goto out;	/* it's from me, ignore it. */
	if (isaddr.s_addr == myaddr.s_addr) {
		printf("duplicate IP address!! sent from ethernet address: ");
		if (eat == &arpethertempl)
			printf("%x %x %x %x %x %x\n",
			    ea->arp_sha[0], ea->arp_sha[1],
			    ea->arp_sha[2], ea->arp_sha[3],
			    ea->arp_sha[4], ea->arp_sha[5]);
		else
			printf("%o\n", ea->arp_sha[0]);
		if (ea->arp_op == ARPOP_REQUEST)
			goto reply;
		goto out;
	}
	ARPTAB_LOOK(at, isaddr.s_addr, &ac->ac_if);
	if (at) {
		bcopy((caddr_t)ea->arp_sha, (caddr_t)at->at_enaddr,
		   eat->arp_hln);
		if (eat->arp_hln < sizeof at->at_enaddr)
			bzero((caddr_t)&at->at_enaddr[eat->arp_hln],
			      sizeof(at->at_enaddr)-eat->arp_hln);
		at->at_flags |= ATF_COM;
		if (at->at_hold) {
			sin.sin_family = AF_INET;
			sin.sin_addr = isaddr;
			(*ac->ac_if.if_output)(&ac->ac_if, 
			    at->at_hold, (struct sockaddr *)&sin);
			at->at_hold = 0;
		}
	}
	if (itaddr.s_addr != myaddr.s_addr)
		goto out;	/* if I am not the target */
	if (at == 0) {		/* ensure we have a table entry */
		at = arptnew(&ac->ac_if, &isaddr);
		bcopy((caddr_t)ea->arp_sha, (caddr_t)at->at_enaddr,
		   eat->arp_hln);
		if (eat->arp_hln < sizeof at->at_enaddr)
			bzero((caddr_t)&at->at_enaddr[eat->arp_hln],
			      sizeof(at->at_enaddr)-eat->arp_hln);
		at->at_flags |= ATF_COM;
	}
	if (ea->arp_op != ARPOP_REQUEST)
		goto out;
reply:
	bcopy((caddr_t)&ea->arp_sha[0],
	      (caddr_t)&ea->arp_sha[eat->arp_hln+eat->arp_pln],
	      eat->arp_hln);
	bcopy((caddr_t)&ea->arp_sha[eat->arp_hln],
	      (caddr_t)&ea->arp_sha[eat->arp_hln*2+eat->arp_pln],
	      eat->arp_pln);
	bcopy((caddr_t)&ac->ac_enaddr[0],
	      (caddr_t)&ea->arp_sha[0],
	      eat->arp_hln);
	bcopy((caddr_t)&myaddr,
	      (caddr_t)&ea->arp_sha[eat->arp_hln],
	      eat->arp_pln);
	ea->arp_op = ARPOP_REPLY;
	bcopy((caddr_t)&ea->arp_sha[eat->arp_hln+eat->arp_pln],
	      (caddr_t)sa.sa_data,
	      eat->arp_hln);
	*((u_short *)&sa.sa_data[2*eat->arp_hln]) = sa.sa_family;
	if (eat == &arpethertempl)
	/*
	 *  10Mb ethernet - byte oriented
	 *
	 *  Swap all word quantities.
	 */
	{
	     ea->arp_pro = htons(ea->arp_pro);
	     ea->arp_op  = htons(ea->arp_op);
	}
        else
	/*
	 *  3Mb ethernet - word oriented
	 *
	 *  Swap all byte quantities.
	 */
	{
	    register int i;
	
	    *((u_short *)&ea->arp_hln) = htons(*((u_short *)&ea->arp_hln));
	    for (i=2*(eat->arp_pln+eat->arp_hln); i; i-=2)
	        *((u_short *)&ea->arp_sha[i-2]) = htons(*((u_short *)&ea->arp_sha[i-2]));
	}
#else	CS_INET
	IF_ADJ(m);
	at = 0;
	if (m->m_len < sizeof *ea)
		goto out;
	if (ac->ac_if.if_flags & IFF_NOARP)
		goto out;
	myaddr = ac->ac_ipaddr;
	ea = mtod(m, struct ether_arp *);
	if (ntohs(ea->arp_pro) != ETHERTYPE_IP)
		goto out;
#ifdef romp
	bcopy((caddr_t)(struct in_addr *)ea->arp_spa,
		(caddr_t)&isaddr.s_addr, sizeof (ea->arp_spa));
	bcopy((caddr_t)(struct in_addr *)ea->arp_tpa,
		(caddr_t)&itaddr.s_addr, sizeof (ea->arp_tpa));
#else romp
	isaddr.s_addr = ((struct in_addr *)ea->arp_spa)->s_addr;
	itaddr.s_addr = ((struct in_addr *)ea->arp_tpa)->s_addr;
#endif romp
	if (!bcmp((caddr_t)ea->arp_sha, (caddr_t)ac->ac_enaddr,
	  sizeof (ea->arp_sha)))
		goto out;	/* it's from me, ignore it. */
	if (isaddr.s_addr == myaddr.s_addr) {
		printf("duplicate IP address!! sent from ethernet address: ");
		printf("%x %x %x %x %x %x\n", ea->arp_sha[0], ea->arp_sha[1],
			ea->arp_sha[2], ea->arp_sha[3],
			ea->arp_sha[4], ea->arp_sha[5]);
		itaddr = myaddr;
		if (ntohs(ea->arp_op) == ARPOP_REQUEST) {
			bcopy((caddr_t)ac->ac_enaddr, (caddr_t)ea->arp_sha,
			    sizeof(ea->arp_sha));
			goto reply;
		}
		goto out;
	}
	ARPTAB_LOOK(at, isaddr.s_addr);
	if (at && (at->at_flags & ATF_COM) == 0) {
		bcopy((caddr_t)ea->arp_sha, (caddr_t)at->at_enaddr,
		    sizeof(ea->arp_sha));
		at->at_flags |= ATF_COM;
		if (at->at_hold) {
			sin.sin_family = AF_INET;
			sin.sin_addr = isaddr;
			(*ac->ac_if.if_output)(&ac->ac_if, 
			    at->at_hold, (struct sockaddr *)&sin);
			at->at_hold = 0;
		}
	}
	if (at == 0 && itaddr.s_addr == myaddr.s_addr) {
		/* ensure we have a table entry */
		at = arptnew(&isaddr);
		bcopy((caddr_t)ea->arp_sha, (caddr_t)at->at_enaddr,
		    sizeof(ea->arp_sha));
		at->at_flags |= ATF_COM;
	}
	if (ntohs(ea->arp_op) != ARPOP_REQUEST)
		goto out;
	ARPTAB_LOOK(at, itaddr.s_addr);
	if (at == NULL) {
		if (itaddr.s_addr != myaddr.s_addr)
			goto out;	/* if I am not the target */
		at = arptnew(&myaddr);
		bcopy((caddr_t)ac->ac_enaddr, (caddr_t)at->at_enaddr,
		   sizeof(at->at_enaddr));
		at->at_flags |= ATF_COM;
	} 
	if (itaddr.s_addr != myaddr.s_addr && (at->at_flags & ATF_PUBL) == 0)
		goto out;
		
reply:
	bcopy((caddr_t)ea->arp_sha, (caddr_t)ea->arp_tha,
	    sizeof(ea->arp_sha));
	bcopy((caddr_t)ea->arp_spa, (caddr_t)ea->arp_tpa,
	    sizeof(ea->arp_spa));
	if (at)		/* done above if at == 0 */
		bcopy((caddr_t)at->at_enaddr, (caddr_t)ea->arp_sha,
		    sizeof(ea->arp_sha));
	bcopy((caddr_t)&itaddr, (caddr_t)ea->arp_spa,
	    sizeof(ea->arp_spa));
	ea->arp_op = htons(ARPOP_REPLY);
	eh = (struct ether_header *)sa.sa_data;
	bcopy((caddr_t)ea->arp_tha, (caddr_t)eh->ether_dhost,
	    sizeof(eh->ether_dhost));
	eh->ether_type = ETHERTYPE_ARP;
#endif	CS_INET
	sa.sa_family = AF_UNSPEC;
	(*ac->ac_if.if_output)(&ac->ac_if, m, &sa);
	return;
out:
	m_freem(m);
	return;
}

/*
 * Free an arptab entry.
 */
arptfree(at)
	register struct arptab *at;
{
	int s = splimp();

	if (at->at_hold)
		m_freem(at->at_hold);
	at->at_hold = 0;
	at->at_timer = at->at_flags = 0;
	at->at_iaddr.s_addr = 0;
	splx(s);
}

/*
 * Enter a new address in arptab, pushing out the oldest entry 
 * from the bucket if there is no room.
 * This always succeeds since no bucket can be completely filled
 * with permanent entries (except from arpioctl when testing whether
 * another permanent entry will fit).
 */
#if	CS_INET
struct arptab * 
arptnew(ifp, addr)
	struct ifnet *ifp;
#else	CS_INET
struct arptab *
arptnew(addr)
#endif	CS_INET
	struct in_addr *addr;
{
	register n;
	int oldest = -1;
	register struct arptab *at, *ato = NULL;
	static int first = 1;

	if (first) {
		first = 0;
		timeout(arptimer, (caddr_t)0, hz);
	}
	at = &arptab[ARPTAB_HASH(addr->s_addr) * ARPTAB_BSIZ];
	for (n = 0; n < ARPTAB_BSIZ; n++,at++) {
		if (at->at_flags == 0)
			goto out;	 /* found an empty entry */
		if (at->at_flags & ATF_PERM)
			continue;
#if	CS_BUGFIX
		if ((int)at->at_timer > oldest) {
#else	CS_BUGFIX
		if (at->at_timer > oldest) {
#endif	CS_BUGFIX
			oldest = at->at_timer;
			ato = at;
		}
	}
	if (ato == NULL)
		return (NULL);
	at = ato;
	arptfree(at);
out:
	at->at_iaddr = *addr;
	at->at_flags = ATF_INUSE;
#if	CS_INET
	at->at_if = ifp;
#endif	CS_INET
	return (at);
}

arpioctl(cmd, data)
	int cmd;
	caddr_t data;
{
	register struct arpreq *ar = (struct arpreq *)data;
	register struct arptab *at;
	register struct sockaddr_in *sin;
	int s;

	if (ar->arp_pa.sa_family != AF_INET ||
	    ar->arp_ha.sa_family != AF_UNSPEC)
		return (EAFNOSUPPORT);
	sin = (struct sockaddr_in *)&ar->arp_pa;
	s = splimp();
#if	CS_INET
	ARPTAB_LOOK(at, sin->sin_addr.s_addr, at->at_if);	/* XXX */
#else	CS_INET
	ARPTAB_LOOK(at, sin->sin_addr.s_addr);
#endif	CS_INET
	if (at == NULL) {		/* not found */
		if (cmd != SIOCSARP) {
			splx(s);
			return (ENXIO);
		}
		if (ifa_ifwithnet(&ar->arp_pa) == NULL) {
			splx(s);
			return (ENETUNREACH);
		}
	}
	switch (cmd) {

	case SIOCSARP:		/* set entry */
		if (at == NULL) {
			at = arptnew(&sin->sin_addr);
			if (ar->arp_flags & ATF_PERM) {
			/* never make all entries in a bucket permanent */
				register struct arptab *tat;
				
				/* try to re-allocate */
				tat = arptnew(&sin->sin_addr);
				if (tat == NULL) {
					arptfree(at);
					splx(s);
					return (EADDRNOTAVAIL);
				}
				arptfree(tat);
			}
		}
		bcopy((caddr_t)ar->arp_ha.sa_data, (caddr_t)at->at_enaddr,
		    sizeof(at->at_enaddr));
		at->at_flags = ATF_COM | ATF_INUSE |
			(ar->arp_flags & (ATF_PERM|ATF_PUBL));
		at->at_timer = 0;
		break;

	case SIOCDARP:		/* delete entry */
		arptfree(at);
		break;

	case SIOCGARP:		/* get entry */
		bcopy((caddr_t)at->at_enaddr, (caddr_t)ar->arp_ha.sa_data,
		    sizeof(at->at_enaddr));
		ar->arp_flags = at->at_flags;
		break;
	}
	splx(s);
	return (0);
}
