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
 *	@(#)if_en.c	6.9 (Berkeley) 9/16/85
 */
#if	CMU
/*
 **********************************************************************
 * HISTORY
 * 26-Jan-86  Avadis Tevanian (avie) at Carnegie-Mellon University
 *	Upgraded to 4.3.
 *
 * 09-Aug-85  Mike Accetta (mja) at Carnegie-Mellon University
 *	NENET: Integrated filter changes from Jeff Mogul at Stanford.
 *	[V1(1)]
 *
 * 14 December 1984	Jeff Mogul	Stanford
 *	- Use if_mqenqueue instead of IF_ENQUEUE for IPs
 *
 * 20 July 1984	Jeff Mogul	Stanford
 *	- prohibit sending of AF_INET packets as broadcasts unless
 *	they really are broadcasts.
 *
 * 19 July 1984	Jeff Mogul	Stanford
 *	- Added watchdog timer because some boards lose transmitter
 *	interrupts.
 *
 * 13 February 1984	Jeff Mogul	Stanford
 *	- Added swabbing of all packets #ifdef	SWABALL
 *	- Use of trailer packets #ifdef	USE_TRAILERS instead of
 *		#ifdef	SWABIPS; should probably just use IFF_NOTRAILERS
 *		flag.
 *
 * 10 November 1983	Jeffrey Mogul
 *	- minor change to linkage mechanism so that filter can
 *	support a variety of hardware
 *
 * 25 April 1983	Mogul, Croft, Nowicki
 *	- linkage to CMU enet filter, #if NENET > 0
 *	- IP packets are swabbed on send/receive 
 *	- don't interfere with subnet part of IP address
 *
 **********************************************************************
 */
 
#include "cs_generic.h"
#include "cs_inet.h"
#include "cs_lint.h"
 
#include "enet.h"
#endif	CMU

#include "en.h"

/*
 * Xerox prototype (3 Mb) Ethernet interface driver.
 */
#include "../machine/pte.h"

#include "param.h"
#include "systm.h"
#include "mbuf.h"
#include "buf.h"
#include "protosw.h"
#include "socket.h"
#include "vmmac.h"
#include "errno.h"
#include "ioctl.h"
#if	NENET > 0
#include "../h/enet.h"
#endif	NENET > 0

#include "../net/if.h"
#include "../net/netisr.h"
#include "../net/route.h"

#ifdef	BBNNET
#define	INET
#endif
#ifdef	INET
#include "../netinet/in.h"
#include "../netinet/in_systm.h"
#include "../netinet/in_var.h"
#include "../netinet/ip.h"
#if	CS_INET
#include "../netinet/if_ether.h"
#endif	CS_INET
#endif

#ifdef PUP
#include "../netpup/pup.h"
#include "../netpup/ether.h"
#endif

#include "../vax/cpu.h"
#include "../vax/mtpr.h"
#include "if_en.h"
#include "if_enreg.h"
#include "if_uba.h"
#include "../vaxuba/ubareg.h"
#include "../vaxuba/ubavar.h"

#define	ENMTU	(1024+512)
#define	ENMRU	(1024+512+16)		/* 16 is enough to receive trailer */

int	enprobe(), enattach(), enrint(), enxint(), encollide();
struct	uba_device *eninfo[NEN];
#if	CS_GENERIC
u_short enstd[] = { 0164000, 0160020, 0 };
#else	CS_GENERIC
u_short enstd[] = { 0 };
#endif	CS_GENERIC
struct	uba_driver endriver =
	{ enprobe, 0, enattach, 0, enstd, "en", eninfo };
#define	ENUNIT(x)	minor(x)

int	eninit(),enoutput(),enreset(),enioctl();
#if	NENET > 0
int	enwatchdog();
#endif	NENET > 0

#ifdef notdef
/*
 * If you need to byte swap IP's in the system, define
 * this and do a SIOCSIFFLAGS at boot time.
 */
#define	ENF_SWABIPS	0x1000
#endif

/*
 * Ethernet software status per interface.
 *
 * Each interface is referenced by a network interface structure,
 * es_if, which the routing code uses to locate the interface.
 * This structure contains the output queue for the interface, its address, ...
 * We also have, for each interface, a UBA interface structure, which
 * contains information about the UNIBUS resources held by the interface:
 * map registers, buffered data paths, etc.  Information is cached in this
 * structure for use by the if_uba.c routines in running the interface
 * efficiently.
 */
struct	en_softc {
#if	CS_INET
	struct	arpcom es_ac;		/* Ethernet common part */
#define	es_if	es_ac.ac_if		/* network-visible interface */
#define	es_addr	es_ac.ac_enaddr		/* hardware Ethernet address */
#else	CS_INET
	struct	ifnet es_if;		/* network-visible interface */
#endif	CS_INET
	struct	ifuba es_ifuba;		/* UNIBUS resources */
	short	es_host;		/* hardware host number */
	short	es_delay;		/* current output delay */
	short	es_mask;		/* mask for current output delay */
	short	es_lastx;		/* host last transmitted to */
	short	es_oactive;		/* is output active? */
	short	es_olen;		/* length of last output */
#if	NENET > 0
	short	es_enetunit;		/* unit number for enet filtering */
#endif	NENET > 0
} en_softc[NEN];

/*
 * Do output DMA to determine interface presence and
 * interrupt vector.  DMA is too short to disturb other hosts.
 */
enprobe(reg)
	caddr_t reg;
{
	register int br, cvec;		/* r11, r10 value-result */
	register struct endevice *addr = (struct endevice *)reg;

#ifdef lint
	br = 0; cvec = br; br = cvec;
	enrint(0); enxint(0); encollide(0);
#endif
	addr->en_istat = 0;
	addr->en_owc = -1;
	addr->en_oba = 0;
	addr->en_ostat = EN_IEN|EN_GO;
	DELAY(100000);
	addr->en_ostat = 0;
#if	CS_GENERIC
	/*
	 *  Allow for a collision rather than transmit done interrupt by
	 *  masking out the low order bits.
	 */
	if (cvec != 0x200)
		cvec &= ~0xf;
#endif	CS_GENERIC	
	return (1);
}

/*
 * Interface exists: make available by filling in network interface
 * record.  System will initialize the interface when it is ready
 * to accept packets.
 */
enattach(ui)
	struct uba_device *ui;
{
	register struct en_softc *es = &en_softc[ui->ui_unit];

	es->es_if.if_unit = ui->ui_unit;
	es->es_if.if_name = "en";
	es->es_if.if_mtu = ENMTU;
	es->es_if.if_flags = IFF_BROADCAST;
	es->es_if.if_init = eninit;
	es->es_if.if_output = enoutput;
	es->es_if.if_ioctl = enioctl;
	es->es_if.if_reset = enreset;
#if	NENET > 0
	es->es_if.if_watchdog = enwatchdog;
	es->es_if.if_timer = 0;
#endif	NENET > 0
	es->es_ifuba.ifu_flags = UBA_NEEDBDP | UBA_NEED16 | UBA_CANTWAIT;
#if defined(VAX750)
	/* don't chew up 750 bdp's */
	if (cpu == VAX_750 && ui->ui_unit > 0)
		es->es_ifuba.ifu_flags &= ~UBA_NEEDBDP;
#endif
#if NENET > 0
	{
	    struct endevp enp;
	    struct en_header dummy;
	    struct endevice *enaddr = (struct endevice *)(ui->ui_addr);
	    
#ifdef	SWABALL
	    enp.end_dev_type = ENDT_3MB;
#else	SWABALL
	    enp.end_dev_type = ENDT_BS3MB;   /* packets appear byte-swapped */
#endif	SWABALL
 
	    enp.end_addr_len = sizeof(dummy.en_shost);
	    enp.end_hdr_len = sizeof(struct en_header);
	    enp.end_MTU = ENMTU;
	    enp.end_addr[0] = (~enaddr->en_addr) & 0xFF;
	    enp.end_broadaddr[0] = 0;
 
	    es->es_enetunit = enetattach(&es->es_if, &enp);
	}
#endif NENET > 0
#if	CS_INET
	es->es_addr[0] = (~((struct endevice *)(ui->ui_addr))->en_addr);
#endif	CS_INET
	if_attach(&es->es_if);
}

/*
 * Reset of interface after UNIBUS reset.
 * If interface is on specified uba, reset its state.
 */
enreset(unit, uban)
	int unit, uban;
{
	register struct uba_device *ui;

	if (unit >= NEN || (ui = eninfo[unit]) == 0 || ui->ui_alive == 0 ||
	    ui->ui_ubanum != uban)
		return;
	printf(" en%d", unit);
	eninit(unit);
}

/*
 * Initialization of interface; clear recorded pending
 * operations, and reinitialize UNIBUS usage.
 */
eninit(unit)
	int unit;
{
	register struct en_softc *es = &en_softc[unit];
	register struct uba_device *ui = eninfo[unit];
	register struct endevice *addr;
	int s;

	if (es->es_if.if_addrlist == (struct ifaddr *)0)
		return;
	if (if_ubainit(&es->es_ifuba, ui->ui_ubanum,
	    sizeof (struct en_header), (int)btoc(ENMRU)) == 0) { 
		printf("en%d: can't initialize\n", unit);
		es->es_if.if_flags &= ~IFF_UP;
		return;
	}
	addr = (struct endevice *)ui->ui_addr;
	addr->en_istat = addr->en_ostat = 0;

	/*
	 * Hang a receive and start any
	 * pending writes by faking a transmit complete.
	 */
	s = splimp();
	addr->en_iba = es->es_ifuba.ifu_r.ifrw_info;
	addr->en_iwc = -(sizeof (struct en_header) + ENMRU) >> 1;
	addr->en_istat = EN_IEN|EN_GO;
	es->es_oactive = 1;
	es->es_if.if_flags |= IFF_RUNNING;
	enxint(unit);
	splx(s);
}

int	enalldelay = 0;
int	enlastdel = 50;
int	enlastmask = (~0) << 5;

/*
 * Start or restart output on interface.
 * If interface is already active, then this is a retransmit
 * after a collision, and just restuff registers and delay.
#if	NENET > 0
 * (It might also be a restart after a watchdog timeout.)
#endif	NENET > 0
 * If interface is not already active, get another datagram
 * to send off of the interface queue, and map it to the interface
 * before starting the output.
 */
enstart(dev)
	dev_t dev;
{
        int unit = ENUNIT(dev);
	struct uba_device *ui = eninfo[unit];
	register struct en_softc *es = &en_softc[unit];
	register struct endevice *addr;
	register struct en_header *en;
	struct mbuf *m;
	int dest;

	if (es->es_oactive)
		goto restart;

	/*
	 * Not already active: dequeue another request
	 * and map it to the UNIBUS.  If no more requests,
	 * just return.
	 */
	IF_DEQUEUE(&es->es_if.if_snd, m);
	if (m == 0) {
		es->es_oactive = 0;
		return;
	}
	en = mtod(m, struct en_header *);
	dest = en->en_dhost;
	en->en_shost = es->es_host;
	es->es_olen = if_wubaput(&es->es_ifuba, m);
#ifdef ENF_SWABIPS
	/*
	 * The Xerox interface does word at a time DMA, so
	 * someone must do byte swapping of user data if high
	 * and low ender machines are to communicate.  It doesn't
	 * belong here, but certain people depend on it, so...
	 *
	 * Should swab everybody, but this is a kludge anyway.
	 */
	if (es->es_if.if_flags & ENF_SWABIPS) {
		en = (struct en_header *)es->es_ifuba.ifu_w.ifrw_addr;
		if (en->en_type == ENTYPE_IP)
			enswab((caddr_t)(en + 1), (caddr_t)(en + 1),
			    es->es_olen - sizeof (struct en_header) + 1);
	}
#endif

	/*
	 * Ethernet cannot take back-to-back packets (no
	 * buffering in interface.  To help avoid overrunning
	 * receivers, enforce a small delay (about 1ms) in interface:
	 *	* between all packets when enalldelay
	 *	* whenever last packet was broadcast
	 *	* whenever this packet is to same host as last packet
	 */
	if (enalldelay || es->es_lastx == 0 || es->es_lastx == dest) {
		es->es_delay = enlastdel;
		es->es_mask = enlastmask;
	}
	es->es_lastx = dest;

restart:
	/*
	 * Have request mapped to UNIBUS for transmission.
	 * Purge any stale data from this BDP, and start the otput.
	 */
	if (es->es_ifuba.ifu_flags & UBA_NEEDBDP)
		UBAPURGE(es->es_ifuba.ifu_uba, es->es_ifuba.ifu_w.ifrw_bdp);
	addr = (struct endevice *)ui->ui_addr;
	addr->en_oba = (int)es->es_ifuba.ifu_w.ifrw_info;
	addr->en_odelay = es->es_delay;
	addr->en_owc = -((es->es_olen + 1) >> 1);
	addr->en_ostat = EN_IEN|EN_GO;
	es->es_oactive = 1;
#if	NENET > 0
	es->es_if.if_timer = 1 + IFNET_SLOWHZ;
	/* 		     ^
	 * The "1" is to guarantee at least one complete timeout
	 * interval, since IFNET_SLOWHZ may be one.  If if_timer
	 * is started at one, the watchdog might go off in much
	 * less than 1/IFNET_SLOWHZ seconds.
	 */
#endif	NENET > 0
}

/*
 * Ethernet interface transmitter interrupt.
 * Start another output if more data to send.
 */
enxint(unit)
	int unit;
{
	register struct uba_device *ui = eninfo[unit];
	register struct en_softc *es = &en_softc[unit];
	register struct endevice *addr = (struct endevice *)ui->ui_addr;

	if (es->es_oactive == 0)
		return;
	if (es->es_mask && (addr->en_ostat&EN_OERROR)) {
		es->es_if.if_oerrors++;
		endocoll(unit);
		return;
	}
	es->es_if.if_opackets++;
#if	NENET > 0
	es->es_if.if_timer = 0;	/* don't want the timer going off */
#endif	NENET > 0
	es->es_oactive = 0;
	es->es_delay = 0;
	es->es_mask = ~0;
	if (es->es_ifuba.ifu_xtofree) {
		m_freem(es->es_ifuba.ifu_xtofree);
		es->es_ifuba.ifu_xtofree = 0;
	}
	if (es->es_if.if_snd.ifq_head == 0) {
		es->es_lastx = 256;		/* putatively illegal */
		return;
	}
	enstart(unit);
}

/*
 * Collision on ethernet interface.  Do exponential
 * backoff, and retransmit.  If have backed off all
 * the way print warning diagnostic, and drop packet.
 */
encollide(unit)
	int unit;
{
	struct en_softc *es = &en_softc[unit];

	es->es_if.if_collisions++;
	if (es->es_oactive == 0)
		return;
	endocoll(unit);
}

endocoll(unit)
	int unit;
{
	register struct en_softc *es = &en_softc[unit];

	/*
	 * Es_mask is a 16 bit number with n low zero bits, with
	 * n the number of backoffs.  When es_mask is 0 we have
	 * backed off 16 times, and give up.
	 */
	if (es->es_mask == 0) {
		printf("en%d: send error\n", unit);
		enxint(unit);
		return;
	}
	/*
	 * Another backoff.  Restart with delay based on n low bits
	 * of the interval timer.
	 */
	es->es_mask <<= 1;
	es->es_delay = mfpr(ICR) &~ es->es_mask;
	enstart(unit);
}

#if	NENET > 0
/*
 * Watchdog routine
 *
 * The timer should only be running while the interface
 * is active for output; it is always reset when enstart()
 * restarts the interface, and is set to be much longer than
 * the maximum time it should take to send a packet.  So, if
 * the timer does go off, we assume that the transmitter interrupt
 * was lost, and restart the transmission by pretending a collision
 * interrupt was seen.  This may lead to duplicated packets!
 */
enwatchdog(unit)
	int unit;
{
	printf("en%d: watchdog\n", unit);
	encollide(unit);
}

#endif	NENET > 0
#ifdef notdef
struct	sockproto enproto = { AF_ETHERLINK };
struct	sockaddr_en endst = { AF_ETHERLINK };
struct	sockaddr_en ensrc = { AF_ETHERLINK };
#endif
/*
 * Ethernet interface receiver interrupt.
 * If input error just drop packet.
 * Otherwise purge input buffered data path and examine 
 * packet to determine type.  If can't determine length
 * from type, then have to drop packet.  Othewise decapsulate
 * packet based on type and pass to type specific higher-level
 * input routine.
 */
enrint(unit)
	int unit;
{
	register struct en_softc *es = &en_softc[unit];
	struct endevice *addr = (struct endevice *)eninfo[unit]->ui_addr;
	register struct en_header *en;
    	struct mbuf *m;
	int len; short resid;
	register struct ifqueue *inq;
#if	NENET > 0
	int off = 0, s;
#else	NENET > 0
	int off, s;
#endif	NENET > 0

	es->es_if.if_ipackets++;

	/*
	 * Purge BDP; drop if input error indicated.
	 */
	if (es->es_ifuba.ifu_flags & UBA_NEEDBDP)
		UBAPURGE(es->es_ifuba.ifu_uba, es->es_ifuba.ifu_r.ifrw_bdp);
	if (addr->en_istat&EN_IERROR) {
		es->es_if.if_ierrors++;
		goto setup;
	}

	/*
	 * Calculate input data length.
	 * Get pointer to ethernet header (in input buffer).
	 * Deal with trailer protocol: if type is PUP trailer
	 * get true type from first 16-bit word past data.
	 * Remember that type was trailer by setting off.
	 */
	resid = addr->en_iwc;
	if (resid)
		resid |= 0176000;
	len = (((sizeof (struct en_header) + ENMRU) >> 1) + resid) << 1;
	len -= sizeof (struct en_header);
#if	NENET > 0
	if ((len > ENMRU) || (len <= 0))
#else	NENET > 0
	if (len > ENMRU)
#endif	NENET > 0
		goto setup;			/* sanity */
	en = (struct en_header *)(es->es_ifuba.ifu_r.ifrw_addr);
#if	CS_INET
#else	CS_INET
	en->en_type = ntohs(en->en_type);
#endif	CS_INET
#define	endataaddr(en, off, type)	((type)(((caddr_t)((en)+1)+(off))))
	if (en->en_type >= ENTYPE_TRAIL &&
	    en->en_type < ENTYPE_TRAIL+ENTYPE_NTRAILER) {
		off = (en->en_type - ENTYPE_TRAIL) * 512;
		if (off > ENMTU)
			goto setup;		/* sanity */
#if	CS_INET
		en->en_type = (*endataaddr(en, off, u_short *));
#else	CS_INET
		en->en_type = ntohs(*endataaddr(en, off, u_short *));
#endif	CS_INET
		resid = ntohs(*(endataaddr(en, off+2, u_short *)));
		if (off + resid > len)
			goto setup;		/* sanity */
		len = off + resid;
	} else
		off = 0;
	if (len == 0)
		goto setup;
#ifdef ENF_SWABIPS
	if (es->es_if.if_flags & ENF_SWABIPS && en->en_type == ENTYPE_IP)
		enswab((caddr_t)(en + 1), (caddr_t)(en + 1), len);
#endif
	/*
	 * Pull packet off interface.  Off is nonzero if packet
	 * has trailing header; if_rubaget will then force this header
	 * information to be at the front, but we still have to drop
	 * the type and length which are at the front of any trailer data.
	 */
	m = if_rubaget(&es->es_ifuba, len, off, &es->es_if);
	if (m == 0)
		goto setup;
	if (off) {
		struct ifnet *ifp;

		ifp = *(mtod(m, struct ifnet **));
		m->m_off += 2 * sizeof (u_short);
		m->m_len -= 2 * sizeof (u_short);
		*(mtod(m, struct ifnet **)) = ifp;

	}
	switch (en->en_type) {

#ifdef INET
	case ENTYPE_IP:
		schednetisr(NETISR_IP);
		inq = &ipintrq;
		break;
#if	CS_INET

	case ENTYPE_ARP:
		arpinput(&es->es_ac, m);
		goto setup;
#endif	CS_INET
#endif
#if NENET > 0
	default:
	{
		register struct mbuf *mtop;
		/*
		 * We need the local net header after all.  Oh well,
		 * this could be improved.
		 */
		MGET(mtop, M_DONTWAIT, MT_DATA);
		if (mtop == 0) {	/* no more mbufs? */
			m_freem(m);		/* wasted effort */
			goto setup;
		}
#ifdef	SWABALL
		/*
		 * the interface has put en_type in host order and we
		 * want it in net order.
		 */
		en->en_type = htons(en->en_type);
#endif	SWABALL
		bcopy((caddr_t)en, (caddr_t)mtod(mtop, struct en_header *),
						sizeof(struct en_header));
		mtop->m_len = sizeof(struct en_header);
		mtop->m_next = m;
		enetFilter(es->es_enetunit, mtop,
				(len + sizeof(struct en_header)) );
		goto setup;
	}
#else	NENET > 0
#ifdef PUP
	case ENTYPE_PUP:
		rpup_input(m);
		goto setup;
#endif
	default:
#ifdef notdef
		enproto.sp_protocol = en->en_type;
		endst.sen_host = en->en_dhost;
		endst.sen_net = ensrc.sen_net = es->es_if.if_net;
		ensrc.sen_host = en->en_shost;
		raw_input(m, &enproto,
		    (struct sockaddr *)&ensrc, (struct sockaddr *)&endst);
#else
		m_freem(m);
#endif
		goto setup;
#endif NENET > 0
	}

	s = splimp();
	if (IF_QFULL(inq)) {
		IF_DROP(inq);
		m_freem(m);
	} else
		IF_ENQUEUE(inq, m);
	splx(s);

setup:
	/*
	 * Reset for next packet.
	 */
	addr->en_iba = es->es_ifuba.ifu_r.ifrw_info;
	addr->en_iwc = -(sizeof (struct en_header) + ENMRU) >> 1;
	addr->en_istat = EN_IEN|EN_GO;
}

/*
 * Ethernet output routine.
 * Encapsulate a packet of type family for the local net.
 * Use trailer local net encapsulation if enough data in first
 * packet leaves a multiple of 512 bytes of data in remainder.
 */
enoutput(ifp, m0, dst)
	struct ifnet *ifp;
	struct mbuf *m0;
	struct sockaddr *dst;
{
	int type, dest, s, error;
	register struct mbuf *m = m0;
	register struct en_header *en;
#if	CS_INET
	register struct en_softc *es = &en_softc[ifp->if_unit];
#endif	CS_INET
	register int off;

	switch (dst->sa_family) {

#ifdef INET
	case AF_INET:
		{
		struct in_addr in;

		in = ((struct sockaddr_in *)dst)->sin_addr;
		if (in_broadcast(in))
			dest = EN_BROADCAST;
		else
			dest = in_lnaof(in);
		}
		if (dest >= 0x100) {
			error = EPERM;		/* ??? */
			goto bad;
		}
		off = ntohs((u_short)mtod(m, struct ip *)->ip_len) - m->m_len;
		/* need per host negotiation */
		if ((ifp->if_flags & IFF_NOTRAILERS) == 0)
		if (off > 0 && (off & 0x1ff) == 0 &&
		    m->m_off >= MMINOFF + 2 * sizeof (u_short)) {
			type = ENTYPE_TRAIL + (off>>9);
			m->m_off -= 2 * sizeof (u_short);
			m->m_len += 2 * sizeof (u_short);
			*mtod(m, u_short *) = htons((u_short)ENTYPE_IP);
			*(mtod(m, u_short *) + 1) = ntohs((u_short)m->m_len);
			goto gottrailertype;
		}
		type = ENTYPE_IP;
		off = 0;
		goto gottype;
#endif
#ifdef PUP
	case AF_PUP:
		dest = ((struct sockaddr_pup *)dst)->spup_host;
		type = ENTYPE_PUP;
		off = 0;
		goto gottype;
#endif
#if	CS_INET
	case AF_UNSPEC:
		dest = dst->sa_data[0];
		type = ((struct en_header *)dst->sa_data)->en_type;
		off = 0;
		goto gottype;
#endif	CS_INET

#ifdef notdef
	case AF_ETHERLINK:
		goto gotheader;
#endif

	default:
		printf("en%d: can't handle af%d\n", ifp->if_unit,
			dst->sa_family);
		error = EAFNOSUPPORT;
		goto bad;
	}

gottrailertype:
	/*
	 * Packet to be sent as trailer: move first packet
	 * (control information) to end of chain.
	 */
	while (m->m_next)
		m = m->m_next;
	m->m_next = m0;
	m = m0->m_next;
	m0->m_next = 0;
	m0 = m;

gottype:
	/*
	 * Add local net header.  If no space in first mbuf,
	 * allocate another.
	 */
	if (m->m_off > MMAXOFF ||
	    MMINOFF + sizeof (struct en_header) > m->m_off) {
		MGET(m, M_DONTWAIT, MT_HEADER);
		if (m == 0) {
			error = ENOBUFS;
			goto bad;
		}
		m->m_next = m0;
		m->m_off = MMINOFF;
		m->m_len = sizeof (struct en_header);
	} else {
		m->m_off -= sizeof (struct en_header);
		m->m_len += sizeof (struct en_header);
	}
	en = mtod(m, struct en_header *);
#if	CS_INET
	en->en_shost = es->es_addr[0];
	en->en_dhost = dest;
	en->en_type = (u_short)type;
#else	CS_INET
	/* add en_shost later */
	en->en_dhost = dest;
	en->en_type = htons((u_short)type);
#endif	CS_INET

#ifdef notdef
gotheader:
#endif
	/*
	 * Queue message on interface, and start output if interface
	 * not yet active.
	 */
	s = splimp();
	if (IF_QFULL(&ifp->if_snd)) {
		IF_DROP(&ifp->if_snd);
		error = ENOBUFS;
		goto qfull;
	}
	IF_ENQUEUE(&ifp->if_snd, m);
	if (en_softc[ifp->if_unit].es_oactive == 0)
		enstart(ifp->if_unit);
	splx(s);
	return (0);
qfull:
	m0 = m;
	splx(s);
bad:
	m_freem(m0);
	return (error);
}

/*
 * Process an ioctl request.
 */
enioctl(ifp, cmd, data)
	register struct ifnet *ifp;
	int cmd;
	caddr_t data;
{
	register struct en_softc *es = ((struct en_softc *)ifp);
	struct ifaddr *ifa = (struct ifaddr *) data;
	int s = splimp(), error = 0;
	struct endevice *enaddr;

	switch (cmd) {

	case SIOCSIFADDR:
		enaddr = (struct endevice *)eninfo[ifp->if_unit]->ui_addr;
		es->es_host = (~enaddr->en_addr) & 0xff;
		/*
		 * Attempt to check agreement of protocol address
		 * and board address.
		 */
		switch (ifa->ifa_addr.sa_family) {
		case AF_INET:
			if (in_lnaof(IA_SIN(ifa)->sin_addr) != es->es_host)
				return (EADDRNOTAVAIL);
			break;
		}
		ifp->if_flags |= IFF_UP;
		if ((ifp->if_flags & IFF_RUNNING) == 0)
			eninit(ifp->if_unit);
		break;

	default:
		error = EINVAL;
		break;
	}
	splx(s);
	return (error);
}

#ifdef ENF_SWABIPS
/*
 * Swab bytes
 * Jeffrey Mogul, Stanford
 */
enswab(from, to, n)
	register unsigned char *from, *to;
	register int n;
{
	register unsigned long temp;

	if ((n <= 0) || (n > 0xFFFF)) {
		printf("enswab: bad len %d\n", n);
		return;
	}
	
	n >>= 1; n++;
#define	STEP	{temp = *from++;*to++ = *from++;*to++ = temp;}
	/* round to multiple of 8 */
	while ((--n) & 07)
		STEP;
	n >>= 3;
	while (--n >= 0) {
		STEP; STEP; STEP; STEP;
		STEP; STEP; STEP; STEP;
	}
}
#endif
