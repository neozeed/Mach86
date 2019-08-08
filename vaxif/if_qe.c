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
 * if_qe.c
 */

#ifndef lint
static	char	*sccsid = "@(#)if_qe.c	1.6 (ULTRIX) 4/24/85";
#endif lint

/************************************************************************
 *									*
 *			Copyright (c) 1985 by				*
 *		Digital Equipment Corporation, Maynard, MA		*
 *			All rights reserved.				*
 *									*
 *   This software is furnished under a license and may be used and	*
 *   copied  only  in accordance with the terms of such license and	*
 *   with the  inclusion  of  the  above  copyright  notice.   This	*
 *   software  or  any  other copies thereof may not be provided or	*
 *   otherwise made available to any other person.  No title to and	*
 *   ownership of the software is hereby transferred.			*
 *									*
 *   This software is  derived  from  software  received  from  the	*
 *   University	of   California,   Berkeley,   and   from   Bell	*
 *   Laboratories.  Use, duplication, or disclosure is  subject  to	*
 *   restrictions  under  license  agreements  with  University  of	*
 *   California and with AT&T.						*
 *									*
 *   The information in this software is subject to change  without	*
 *   notice  and should not be construed as a commitment by Digital	*
 *   Equipment Corporation.						*
 *									*
 *   Digital assumes no responsibility for the use  or  reliability	*
 *   of its software on equipment which is not supplied by Digital.	*
 *									*
 ************************************************************************/
#if	CMU
 
/*
 **********************************************************************
 * HISTORY
 * 28-Jan-86  Avadis Tevanian (avie) at Carnegie-Mellon University
 *	Upgraded to 4.3.
 *
 *  3-Dec-85  Robert V Baron (rvb) at Carnegie-Mellon University
 *	People have grown weary of seeing the qerestart message.  So we
 *	just count.
 *
 *  6-Nov-85  Mike Accetta (mja) at Carnegie-Mellon University
 *	Perform qerestart sooner to prevent connections from being lost.
 *
 * 17-Jun-85  Mike Accetta (mja) at Carnegie-Mellon University
 *	CS_VAXM:  imported from ULTRIX-32 to run in standard CMU 4.2BSD
 *	environment.  ULTRIX specific features have been conditionally
 *	compiled out and a few ULTRIX specific structure defintion
 *	differences replicated here to make as few changes to the rest
 *	of the system and to this driver as possible as required to
 *	install it in the standard system (although who knows if we will
 *	ever see any never ULTRIX sources to make this worthwhile).
 *	[V1(1)]
 *
 **********************************************************************
 */
 
#include "cs_lint.h"
#include "cs_vaxm.h"
#include "cs_generic.h"
#endif	CMU

/* ---------------------------------------------------------------------
 * Modification History 
 *
 *  27 Feb. 85 -- ejf
 *	Return default hardware address on ioctl request.
 *
 *  12 Feb. 85 -- ejf
 *	Added internal extended loopback capability.
 *
 *  27 Dec. 84 -- rjl
 *	Fixed bug that caused every other transmit descriptor to be used
 *	instead of every descriptor.
 *
 *  21 Dec. 84 -- rjl
 *	Added watchdog timer to mask hardware bug that causes device lockup.
 *
 *  18 Dec. 84 -- rjl
 *	Reworked driver to use q-bus mapping routines.  MicroVAX-I now does
 *	copying instead of m-buf shuffleing.
 *	A number of deficencies in the hardware/firmware were compensated
 *	for. See comments in qestart and qerint.
 *
 *  14 Nov. 84 -- jf
 *	Added usage counts for multicast addresses.
 *	Updated general protocol support to allow access to the Ethernet
 *	header.
 *
 *  04 Oct. 84 -- jf
 *	Added support for new ioctls to add and delete multicast addresses
 *	and set the physical address.
 *	Add support for general protocols.
 *
 *  14 Aug. 84 -- rjl
 *	Integrated Shannon changes. (allow arp above 1024 and ? )
 *
 *  13 Feb. 84 -- rjl
 *
 *	Initial version of driver. derived from IL driver.
 * 
 * ---------------------------------------------------------------------
 */

#include "qe.h"
#if	NQE > 0 || defined(BINARY)
/*
 * Digital Q-BUS to NI Adapter
 */
#include "../machine/pte.h"

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/mbuf.h"
#include "../h/buf.h"
#include "../h/protosw.h"
#include "../h/socket.h"
#include "../h/vmmac.h"
#include "../h/ioctl.h"
#include "../h/errno.h"
#include "../h/time.h"
#include "../h/kernel.h"

#include "../net/if.h"
#include "../net/netisr.h"
#include "../net/route.h"
#include "../netinet/in.h"
#include "../netinet/in_systm.h"
#if	CS_VAXM
#include "../netinet/in_var.h"
#endif	CS_VAXM
#include "../netinet/ip.h"
#include "../netinet/ip_var.h"
#if	CS_VAXM
/*
 *  The following standard structures have incompatible defintions
 *  in ULTRIX and are provided below.
 */
#define	ether_header	Xether_header
#define	arpcom		Xarpcom
#endif	CS_VAXM
#include "../netinet/if_ether.h"
#ifdef	PUP
#include "../netpup/pup.h"
#endif	PUP

#include "../vax/cpu.h"
#include "../vax/mtpr.h"
#include "../vaxif/if_qereg.h"
#include "../vaxif/if_uba.h"
#include "../vaxuba/ubareg.h"
#include "../vaxuba/ubavar.h"

#if	CS_VAXM
/* 
 *  Provide our names for these symbols.
 */
#ifdef	VAXM
#define	MVAX	VAXM
#define	MVAX_I	VAX_MI
#define	MVAX_II	VAX_MII
#endif	VAXM
 
/* 
 *  These flags are specific to ULTRIX-32 and have no meaning here.
 */
#define	IFF_DYNPROTO	0
#define	IFF_LOOPBACK	0
 
/* 
 *  An ethernet address (a la ULTRIX).
 */
struct ether_addr
{
	u_char	ether_addr_octet[6];
};
 
/* 
 *  These definitions need the ethernet address structure to
 *  rather than an array o6 6 bytes to support ULTRIX device
 *  drivers.
 */
 
#undef	ether_header
struct	ether_header
{
	struct ether_addr ether_dhost;
	struct ether_addr ether_shost;
	u_short	ether_type;
};
 
#undef	arpcom
struct	arpcom
{
	struct 	ifnet ac_if;	/* network-visible interface */
	struct ether_addr ac_enaddr;	/* ethernet hardware address */
	struct	in_addr ac_ipaddr;	/* copy of ip address */
};
 
/* 
 *  Configuration variables from ULTRIX which are not used here.
 */
static int nINET = 1;
static int nETHER = 1;
#endif	CS_VAXM
/*
 * The number of recieve packets is set to minimize the possibility
 * of incuring a race condition between the hardware and software.
 */
#define NRCV	8	 		/* Receive descriptors		*/
#define NXMT	5	 		/* Transmit descriptors		*/
#define NTOT	(NXMT + NRCV)
#define NMULTI	12			/* Number of multicast addresses*/
/*
 * The deqna shouldn't recieve more than ETHERMTU + sizeof(struct ether_header)
 * but will actually take in up to 2048 bytes. To guard against the receiver
 * chaining buffers (which we aren't prepared to handle) we allocate 2kb 
 * size buffers.
 */
#define MAXPACKETSIZE 2048		/* Should really be ETHERMTU	*/

/*
 * This constant should really be 60 because the qna adds 4 bytes of crc.
 * However when set to 60 our packets are ignored by deuna's , 3coms are
 * okay ??????????????????????????????????????????
 */
#define MINDATA 64

#if	CS_GENERIC
/*
 *	This structure is defined in if_uba.h in 4.3.
 */
#undef	ifu_uban
#undef	ifu_hlen
#undef	ifu_uba
#undef	ifu_flags
#undef	ifu_w
/*
 * The qeuba structures generalizes the ifuba structure
 * to an arbitrary number of receive and transmit buffers.
 */
struct	qe_ifxmt {
#else	CS_GENERIC
struct	ifxmt {
#endif	CS_GENERIC
	struct	ifrw x_ifrw;			/* mapping information */
	struct	pte x_map[IF_MAXNUBAMR];	/* output base pages */
	short	x_xswapd;			/* mask of clusters swapped */
	struct	mbuf *x_xtofree;		/* pages being dma'ed out */
};

struct	qeuba {
	short	ifu_uban;		/* uba number */
	short	ifu_hlen;		/* local net heaqer length */
	struct	uba_regs *ifu_uba;	/* uba regs, in vm */
	struct	ifrw ifu_r[NRCV];	/* receive information */
#if	CS_GENERIC
	struct	qe_ifxmt ifu_w[NXMT];	/* transmit information */
#else	CS_GENERIC
	struct	ifxmt ifu_w[NXMT];	/* transmit information */
#endif	CS_GENERIC
	short	ifu_flags;		/* used during uballoc's */
};
struct qe_multi {
	u_char	qm_char[6];
};
#define MULTISIZE sizeof(struct qe_multi)

/*
 * Ethernet software status per interface.
 *
 * Each interface is referenced by a network interface structure,
 * is_if, which the routing code uses to locate the interface.
 * This structure contains the output queue for the interface, its address, ...
 */
#define	is_if	is_ac.ac_if		/* network-visible interface 	*/
#define	is_addr	is_ac.ac_enaddr		/* hardware Ethernet address 	*/

struct	qe_softc {
	struct	arpcom is_ac;		/* Ethernet common part 	*/
	struct	qeuba qeuba;		/* Q-bus resources 		*/
	int	setupaddr;		/* mapping info for setup pkts  */
	struct	qe_ring *rringaddr;	/* mapping info for rings	*/
	struct	qe_ring *tringaddr;	/*       ""			*/
	struct	qe_ring rring[NRCV+1];	/* Receive ring descriptors 	*/
	struct	qe_ring tring[NXMT+1];	/* Transmit ring descriptors 	*/
	u_char	setup_pkt[16][8];	/* Setup packet			*/
	struct	qe_multi multi[NMULTI];	/* Multicast address list	*/
#if	CS_VAXM
#else	CS_VAXM
	struct	estat ctrblk;		/* Counter block		*/
#endif	CS_VAXM
	u_char	muse[NMULTI];		/* Multicast address usage count*/
	long	ztime;			/* Time counters last zeroed	*/
	int	rindex;			/* Receive index		*/
	int	tindex;			/* Transmit index		*/
	int	otindex;		/* Old transmit index		*/
	int	qe_intvec;		/* Interrupt vector 		*/
	struct	qedevice *addr;		/* device addr			*/
	int	setupqueued;		/* Setup packet queued		*/
	int	nxmit;			/* Transmits in progress	*/
	char	*buffers;		/* Buffers for packets		*/
	int	timeout;		/* watchdog timeout		*/
};

#ifdef BINARY

extern	struct	qe_softc qe_softc[];
extern	struct	uba_device *qeinfo[];
extern	int	nNQE;
extern  int	nNRCV;
extern	int	nNXMT;
extern	int	nNTOT;

#else BINARY

struct	qe_softc  qe_softc[NQE];
struct	uba_device *qeinfo[NQE];
int	nNQE = NQE;
int	nNRCV = NRCV;
int	nNXMT = NXMT;
int	nNTOT = NTOT;

#endif

#if	CS_LINT
#else	CS_LINT
extern struct protosw *iftype_to_proto(), *iffamily_to_proto();
#endif	CS_LINT
extern struct timeval time;
extern timeout();

#if	CS_LINT
int	qeprobe(), qeattach(), qeintr(), qewatch();
#else	CS_LINT
int	qeprobe(), qeattach(), qeint(), qewatch();
#endif	CS_LINT
int	qeinit(),qeoutput(),qeioctl(),qereset(),qewatch();
struct mbuf *qeget();

u_short qestd[] = { 0 };
struct	uba_driver qedriver =
	{ qeprobe, 0, qeattach, 0, qestd, "qe", qeinfo };

u_char unused_multi[6] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

#define	QEUNIT(x)	minor(x)
static int mask = 0x3ffff;		/* address mask		*/
int qewatchrun = 0;			/* watchdog running	*/
#if	CS_GENERIC
int qerestart_count = 0;
#endif	CS_GENERIC
/*
 * Probe the QNA to see if it's there
 */
qeprobe(reg)
	caddr_t reg;
{
	register int br, cvec;		/* r11, r10 value-result */

	register struct qedevice *addr = (struct qedevice *)reg;
	register struct qe_ring *rp; 
	register struct qe_ring *prp; 	/* physical rp 		*/
	register int i, j, ncl;
	static int next=0;		/* softc index		*/
	register struct qe_softc *sc = &qe_softc[next++];

#ifdef lint
	br = 0; cvec = br; br = cvec;
#if	CS_LINT
	qeintr(0);
#endif	CS_LINT
#endif
	/*
	 * Set the address mask for the particular cpu
	 */
	if( cpu == MVAX_I )
		mask = 0x3fffff;
	else
		mask = 0x3ffff;

	/*
	 * The QNA interrupts on i/o operations. To do an I/O operation 
	 * we have to setup the interface by transmitting a setup  packet.
	 */
	addr->qe_csr = QE_RESET;
	addr->qe_vector = (uba_hd[numuba].uh_lastiv -= 4);

	/*
	 * Map the communications area and the setup packet.
	 */
	sc->setupaddr =
#if	CS_LINT
		uballoc(0, (caddr_t)sc->setup_pkt, sizeof(sc->setup_pkt), 0);
	sc->rringaddr = (struct qe_ring *) 
		uballoc(0, (caddr_t)sc->rring, sizeof(struct qe_ring)*(nNTOT+2),0);
#else	CS_LINT
		uballoc(0, sc->setup_pkt, sizeof(sc->setup_pkt), 0);
	sc->rringaddr = (struct qe_ring *)
		uballoc(0, sc->rring, sizeof(struct qe_ring)*(nNTOT+2),0);
#endif	CS_LINT
	prp = (struct qe_ring *)((int)sc->rringaddr & mask);

	/*
	 * The QNA will loop the setup packet back to the receive ring
	 * for verification, therefore we initialize the first 
	 * receive & transmit ring descriptors and link the setup packet
	 * to them.
	 */
	qeinitdesc( sc->tring, sc->setupaddr & mask, sizeof(sc->setup_pkt));
	qeinitdesc( sc->rring, sc->setupaddr & mask, sizeof(sc->setup_pkt));

	rp = (struct qe_ring *)sc->tring;
	rp->qe_setup = 1;
	rp->qe_eomsg = 1;
	rp->qe_flag = rp->qe_status1 = QE_NOTYET;
	rp->qe_valid = 1;

	rp = (struct qe_ring *)sc->rring;
	rp->qe_flag = rp->qe_status1 = QE_NOTYET;
	rp->qe_valid = 1;

	/*
	 * Get the addr off of the interface and place it into the setup
	 * packet. This code looks strange due to the fact that the address
	 * is placed in the setup packet in col. major order. 
	 */
	for( i = 0 ; i < 6 ; i++ )
		sc->setup_pkt[i][1] = addr->qe_sta_addr[i];

	qesetup( sc );
	/*
	 * Start the interface and wait for the packet.
	 */
	j = cvec;
	addr->qe_csr = QE_INT_ENABLE | QE_XMIT_INT | QE_RCV_INT;
	addr->qe_rcvlist_lo = (short)prp;
	addr->qe_rcvlist_hi = (short)((int)prp >> 16);
	prp += nNRCV+1;
	addr->qe_xmtlist_lo = (short)prp;
	addr->qe_xmtlist_hi = (short)((int)prp >> 16);
	DELAY(10000);
	/*
	 * All done with the bus resources. If it's a uVAX-I they weren't
	 * really allocated otherwise deallocated them.
	 */
	if( cpu != MVAX_I ) {
		ubarelse(0, &sc->setupaddr);
#if	CS_LINT
		ubarelse(0, (int *)&sc->rringaddr);
#else	CS_LINT
		ubarelse(0, &sc->rringaddr);
#endif	CS_LINT
	}
	if( cvec == j ) 
		return 0;		/* didn't interrupt	*/

	/*
	 * Allocate page size buffers now. If we wait until the network
	 * is setup they have already fragmented. By doing it here in
	 * conjunction with always copying on uVAX-I processors we obtain
	 * physically contigous buffers for dma transfers.
	 */
	ncl = clrnd((int)btoc(MAXPACKETSIZE) + CLSIZE) / CLSIZE;
	sc->buffers = m_clalloc(nNTOT * ncl, MPG_SPACE);
	return( sizeof(struct qedevice) );
}
 
/*
 * Interface exists: make available by filling in network interface
 * record.  System will initialize the interface when it is ready
 * to accept packets.
 */
qeattach(ui)
	struct uba_device *ui;
{
	register struct qe_softc *sc = &qe_softc[ui->ui_unit];
	register struct ifnet *ifp = &sc->is_if;
	register struct qedevice *addr = (struct qedevice *)ui->ui_addr;
	register int i;
	struct sockaddr_in *sin;

	ifp->if_unit = ui->ui_unit;
	ifp->if_name = "qe";
	ifp->if_mtu = ETHERMTU;
	ifp->if_flags |= IFF_BROADCAST | IFF_DYNPROTO;

	/*
	 * Read the address from the prom and save it.
	 */
	for( i=0 ; i<6 ; i++ )
		sc->setup_pkt[i][1] = sc->is_addr.ether_addr_octet[i] = addr->qe_sta_addr[i] & 0xff;  

	/*
	 * Save the vector for initialization at reset time.
	 */
	sc->qe_intvec = addr->qe_vector;

#if	CS_GENERIC
#else	CS_GENERIC
	sin = (struct sockaddr_in *)&ifp->if_addr;
	sin->sin_family = AF_INET;
#endif	CS_GENERIC
	ifp->if_init = qeinit;
	ifp->if_output = qeoutput;
	ifp->if_ioctl = qeioctl;
	ifp->if_reset = qereset;
	if_attach(ifp);
}

/*
 * Reset of interface after UNIBUS reset.
 * If interface is on specified uba, reset its state.
 */
qereset(unit, uban)
	int unit, uban;
{
	register struct uba_device *ui;

	if (unit >= nNQE || (ui = qeinfo[unit]) == 0 || ui->ui_alive == 0 ||
		ui->ui_ubanum != uban)
		return;
	printf(" qe%d", unit);
	qeinit(unit);
}
 
/*
 * Initialization of interface. 
 */
qeinit(unit)
	int unit;
{
	register struct qe_softc *sc = &qe_softc[unit];
	register struct uba_device *ui = qeinfo[unit];
	register struct qedevice *addr = (struct qedevice *)ui->ui_addr;
	register struct ifnet *ifp = &sc->is_if;
	register struct sockaddr_in *sin;
	register i;
	int s;

#if	CS_GENERIC
	if (ifp->if_addrlist == (struct ifaddr *)0)
		return;

	if (ifp->if_flags & IFF_RUNNING)
		return;
#else	CS_GENERIC
	sin = (struct sockaddr_in *)&ifp->if_addr;
	if (sin->sin_addr.s_addr == 0)		/* address still unknown */
		return;
	if( ifp->if_flags & IFF_RUNNING )
		goto justarp;
#endif	CS_GENERIC

	/*
	 * map the communications area onto the device 
	 */
	sc->rringaddr = (struct qe_ring *)((int)uballoc(0,
#if	CS_LINT
		(caddr_t)sc->rring, sizeof(struct qe_ring)*(nNTOT+2),0)&mask);
	sc->tringaddr = sc->rringaddr+nNRCV+1; 
	sc->setupaddr =	uballoc(0, (caddr_t)sc->setup_pkt, sizeof(sc->setup_pkt), 0) & mask;
#else	CS_LINT
		sc->rring, sizeof(struct qe_ring)*(nNTOT+2),0)&mask);
	sc->tringaddr = sc->rringaddr+nNRCV+1;
	sc->setupaddr =	uballoc(0, sc->setup_pkt, sizeof(sc->setup_pkt), 0) & mask;
#endif	CS_LINT
	/*
	 * init buffers and maps
	 */
	if (qe_ubainit(&sc->qeuba, ui->ui_ubanum,
	    sizeof (struct ether_header), (int)btoc(MAXPACKETSIZE), sc->buffers) == 0) { 
		printf("qe%d: can't initialize\n", unit);
		sc->is_if.if_flags &= ~IFF_UP;
		return;
	}
	/*
	 * Init the buffer descriptors and indexes for each of the lists and
	 * loop them back to form a ring.
	 */
	for( i = 0 ; i < nNRCV ; i++ ){
		qeinitdesc( &sc->rring[i],
			sc->qeuba.ifu_r[i].ifrw_info & mask, MAXPACKETSIZE);
		sc->rring[i].qe_flag = sc->rring[i].qe_status1 = QE_NOTYET;
		sc->rring[i].qe_valid = 1;
	}
	qeinitdesc( &sc->rring[i], NULL, 0 );

	sc->rring[i].qe_addr_lo = (short)sc->rringaddr;
	sc->rring[i].qe_addr_hi = (short)((int)sc->rringaddr >> 16);
	sc->rring[i].qe_chain = 1;
	sc->rring[i].qe_flag = sc->rring[i].qe_status1 = QE_NOTYET;
	sc->rring[i].qe_valid = 1;

	for( i = 0 ; i <= nNXMT ; i++ )
		qeinitdesc( &sc->tring[i], NULL, 0 );
	i--;

	sc->tring[i].qe_addr_lo = (short)sc->tringaddr;
	sc->tring[i].qe_addr_hi = (short)((int)sc->tringaddr >> 16);
	sc->tring[i].qe_chain = 1;
	sc->tring[i].qe_flag = sc->tring[i].qe_status1 = QE_NOTYET;
	sc->tring[i].qe_valid = 1;

	sc->nxmit = sc->otindex = sc->tindex = sc->rindex = 0;

	/*
	 * Take the interface out of reset, program the vector, 
	 * enable interrupts, and tell the world we are up.
	 */
	s = splimp();
	addr->qe_vector = sc->qe_intvec;
	sc->addr = addr;
	if ( ifp->if_flags & IFF_LOOPBACK )
		addr->qe_csr = QE_RCV_ENABLE | QE_INT_ENABLE | QE_XMIT_INT | QE_RCV_INT | QE_ELOOP;
	else
		addr->qe_csr = QE_RCV_ENABLE | QE_INT_ENABLE | QE_XMIT_INT | QE_RCV_INT | QE_ILOOP;
	addr->qe_rcvlist_lo = (short)sc->rringaddr;
	addr->qe_rcvlist_hi = (short)((int)sc->rringaddr >> 16);
	ifp->if_flags |= IFF_UP | IFF_RUNNING;
	qesetup( sc );
	qestart( unit );
	sc->ztime = time.tv_sec;
	splx( s );
#if	CS_GENERIC
#else	CS_GENERIC
justarp:
	if_rtinit(&sc->is_if, RTF_UP);
	arpwhohas(&sc->is_ac, &sin->sin_addr);
#endif	CS_GENERIC
}
 
/*
 * Start output on interface.
 *
 */
qestart(dev)
	dev_t dev;
{
	int unit = QEUNIT(dev);
	struct uba_device *ui = qeinfo[unit];
	register struct qe_softc *sc = &qe_softc[unit];
	register struct qedevice *addr;
	register struct qe_ring *rp;
	register index;
#if	CS_VAXM
	struct mbuf *m;
	int buf_addr, len, s;
#else	CS_VAXM
	struct mbuf *m, *m0;
	int buf_addr, len, j,  s;
#endif	CS_VAXM

	s = splimp();
	addr = (struct qedevice *)ui->ui_addr;
	/*
	 * The deqna doesn't look at anything but the valid bit
	 * to determine if it should transmit this packet. If you have
	 * a ring and fill it the device will loop indefinately on the
	 * packet and continue to flood the net with packets until you
	 * break the ring. For this reason we never queue more than n-1
	 * packets in the transmit ring. 
	 *
	 * The microcoders should have obeyed their own defination of the
	 * flag and status words, but instead we have to compensate.
	 */
	for( index = sc->tindex; 
		sc->tring[index].qe_valid == 0 && sc->nxmit < (nNXMT-1) ;
		sc->tindex = index = ++index % nNXMT){
		rp = &sc->tring[index];
		if( sc->setupqueued ) {
			buf_addr = sc->setupaddr;
			len = 128;
			rp->qe_setup = 1;
			sc->setupqueued = 0;
		} else {
			IF_DEQUEUE(&sc->is_if.if_snd, m);
			if( m == 0 ){
				splx(s);
				return;
			}
			buf_addr = sc->qeuba.ifu_w[index].x_ifrw.ifrw_info;
			len = qeput(&sc->qeuba, index, m);
		}
		/*
		 *  Does buffer end on odd byte ? 
		 */
		if( len & 1 ) {
			len++;
			rp->qe_odd_end = 1;
		}
		if( len < MINDATA )
			len = MINDATA;
		rp->qe_buf_len = -(len/2);
		buf_addr &= mask;
		rp->qe_flag = rp->qe_status1 = QE_NOTYET;
		rp->qe_addr_lo = (short)buf_addr;
		rp->qe_addr_hi = (short)(buf_addr >> 16);
		rp->qe_eomsg = 1;
		rp->qe_flag = rp->qe_status1 = QE_NOTYET;
		rp->qe_valid = 1;
		sc->nxmit++;
		/*
		 * If the watchdog time isn't running kick it.
		 */
		sc->timeout=1;
		if( !qewatchrun++ ) 
#if	CS_LINT
			timeout(qewatch,(caddr_t)0,10*hz);
#else	CS_LINT
			timeout(qewatch,0,10*hz);
#endif	CS_LINT
			
		/*
		 * See if the xmit list is invalid.
		 */
		if( addr->qe_csr & QE_XL_INVALID ) {
			buf_addr = (int)(sc->tringaddr+index);
			addr->qe_xmtlist_lo = (short)buf_addr;
			addr->qe_xmtlist_hi = (short)(buf_addr >> 16);
		}
#if	CS_VAXM
#else	CS_VAXM
		/*
		 * Accumulate statistics for DECnet
		 */
		if ((sc->ctrblk.est_bytesent + len) > sc->ctrblk.est_bytesent)
			sc->ctrblk.est_bytesent += len;
		if (sc->ctrblk.est_bloksent != 0xffffffff)
			sc->ctrblk.est_bloksent++;
#endif	CS_VAXM
	}
	splx( s );
}
 
/*
 * Ethernet interface interrupt processor
 */
qeintr(unit)
	int unit;
{
	register struct qe_softc *sc = &qe_softc[unit];
	register struct ifnet *ifp = &sc->is_if;
	struct qedevice *addr = (struct qedevice *)qeinfo[unit]->ui_addr;
	int s, buf_addr, csr;

	s = splimp();
	csr = addr->qe_csr;
	if ( ifp->if_flags & IFF_LOOPBACK )
		addr->qe_csr = QE_RCV_ENABLE | QE_INT_ENABLE | QE_XMIT_INT | QE_RCV_INT | QE_ELOOP;
	else
		addr->qe_csr = QE_RCV_ENABLE | QE_INT_ENABLE | QE_XMIT_INT | QE_RCV_INT | QE_ILOOP;
	if( csr & QE_RCV_INT ) 
		qerint( unit );
	if( csr & QE_XMIT_INT )
		qetint( unit );
	if( csr & QE_NEX_MEM_INT )
		printf("qe: Non existant memory interrupt\n");
	
	if( addr->qe_csr & QE_RL_INVALID && sc->rring[sc->rindex].qe_status1 == QE_NOTYET ) {
		buf_addr = (int)&sc->rringaddr[sc->rindex];
		addr->qe_rcvlist_lo = (short)buf_addr;
		addr->qe_rcvlist_hi = (short)(buf_addr >> 16);
	}
	splx( s );
}
 
/*
 * Ethernet interface transmit interrupt.
 */
qetint(unit)
	int unit;
{
	register struct qe_softc *sc = &qe_softc[unit];
#if	CS_VAXM
#else	CS_VAXM
	register struct mbuf *mp, *mp0;
	register first, index;
#endif	CS_VAXM
	register struct qe_ring *rp;
	register struct ifrw *ifrw;
	struct ether_header *eh;
#if	CS_VAXM
	int i, status1, setupflag;
#else	CS_VAXM
	int i, status1, status2, setupflag;
#endif	CS_VAXM
	short len;


	while( sc->otindex != sc->tindex && sc->tring[sc->otindex].qe_status1 != QE_NOTYET && sc->nxmit > 0 ) {
		/*
		 * Save the status words from the descriptor so that it can
		 * be released.
		 */
		rp = &sc->tring[sc->otindex];
		status1 = rp->qe_status1;
#if	CS_VAXM
#else	CS_VAXM
		status2 = rp->qe_status2;
#endif	CS_VAXM
		setupflag = rp->qe_setup;
		len = (-rp->qe_buf_len) * 2;
		if( rp->qe_odd_end )
			len++;
		/*
		 * Init the buffer descriptor
		 */
#if	CS_LINT
		bzero( (caddr_t)rp, sizeof(struct qe_ring));
#else	CS_LINT
		bzero( rp, sizeof(struct qe_ring));
#endif	CS_LINT
		if( --sc->nxmit == 0 )
			sc->timeout = 0;
		if( !setupflag ) {
			/*
			 * Do some statistics.
			 */
			sc->is_if.if_opackets++;
			sc->is_if.if_collisions += ( status1 & QE_CCNT ) >> 4;
#if	CS_VAXM
#else	CS_VAXM
			/*
			 * Accumulate DECnet statistics
			 */
			if (status1 & QE_CCNT) {
				if (((status1 & QE_CCNT) >> 4) == 1) {
					if (sc->ctrblk.est_single != 0xffffffff)
						sc->ctrblk.est_single++;
				} else {
					if (sc->ctrblk.est_multiple != 0xffffffff)
						sc->ctrblk.est_multiple++;
				}
			}
			if (status1 & QE_FAIL)
				if (sc->ctrblk.est_collis != 0xffff)
					sc->ctrblk.est_collis++;
#endif	CS_VAXM
			if( status1 & QE_ERROR ) { 
				sc->is_if.if_oerrors++;
#if	CS_VAXM
#else	CS_VAXM
				if (sc->ctrblk.est_sendfail != 0xffff) {
					sc->ctrblk.est_sendfail++;
					if (status1 & QE_ABORT)
						sc->ctrblk.est_sendfail_bm |= 1;
					if (status1 & QE_NOCAR)
						sc->ctrblk.est_sendfail_bm |= 2;
				}
#endif	CS_VAXM
			}
			/*
			 * If this was a broadcast packet loop it
			 * back because the hardware can't hear it's own
			 * transmits and the rwho deamon expects to see them.
			 * This code will have to be expanded to include multi-
			 * cast if the same situation developes.
			 */
			ifrw = &sc->qeuba.ifu_w[sc->otindex].x_ifrw;
			eh = (struct ether_header *)ifrw->ifrw_addr;
			for( i=0 ; i < 6 && eh->ether_dhost.ether_addr_octet[i] == 0xff ; i++)
					;
			if( i == 6 )
				qeread(sc, ifrw, len);
		}
		sc->otindex = ++sc->otindex % nNXMT;
	}
	qestart( unit );
}
 
/*
 * Ethernet interface receiver interrupt.
 * If can't determine length from type, then have to drop packet.  
 * Othewise decapsulate packet based on type and pass to type specific 
 * higher-level input routine.
 */
qerint(unit)
	int unit;
{
	register struct qe_softc *sc = &qe_softc[unit];
	register struct ifnet *ifp = &sc->is_if;
	register struct qe_ring *rp;
	int len, status1, status2;
	int bufaddr;
	struct ether_header *eh;

	/*
	 * Traverse the receive ring looking for packets to pass back.
	 * The search is complete when we find a descriptor not in use.
	 *
	 * As in the transmit case the deqna doesn't honor it's own protocols
	 * so there exists the possibility that the device can beat us around
	 * the ring. The proper way to guard against this is to insure that
	 * there is always at least one invalid descriptor. We chose instead
	 * to make the ring large enough to minimize the problem. With a ring
	 * size of 4 we haven't been able to see the problem. To be safe we
	 * doubled that to 8.
	 *
	 */
	for( ; sc->rring[sc->rindex].qe_status1 != QE_NOTYET ; sc->rindex = ++sc->rindex % nNRCV ){
		rp = &sc->rring[sc->rindex];
		status1 = rp->qe_status1;
		status2 = rp->qe_status2;
#if	CS_LINT
		bzero( (caddr_t)rp, sizeof(struct qe_ring));
#else	CS_LINT
		bzero( rp, sizeof(struct qe_ring));
#endif	CS_LINT
		len = ((status1 & QE_RBL_HI) | (status2 & QE_RBL_LO)) + 60;
		sc->is_if.if_ipackets++;
 
		if( ! (ifp->if_flags & IFF_LOOPBACK) ) {
			if( status1 & QE_ERROR ) {
				sc->is_if.if_ierrors++;
#if	CS_VAXM
#else	CS_VAXM
				if ((status1 & (QE_OVF | QE_CRCERR | QE_FRAME)) &&
					(sc->ctrblk.est_recvfail != 0xffff)) {
					sc->ctrblk.est_recvfail++;
					if (status1 & QE_OVF)
						sc->ctrblk.est_recvfail_bm |= 4;
					if (status1 & QE_CRCERR)
						sc->ctrblk.est_recvfail_bm |= 1;
					if (status1 & QE_FRAME)
						sc->ctrblk.est_recvfail_bm |= 2;
				}
#endif	CS_VAXM
			} else {
				/*
				 * We don't process setup packets.
				 */
				if( !(status1 & QE_ESETUP) )
					qeread(sc, &sc->qeuba.ifu_r[sc->rindex],
						len - sizeof(struct ether_header));
			}
		} else {
			eh = (struct ether_header *)sc->qeuba.ifu_r[sc->rindex].ifrw_addr;
#if	CS_LINT
			if ( bcmp((caddr_t)eh->ether_dhost.ether_addr_octet, (caddr_t)sc->is_addr.ether_addr_octet, 6) == NULL )
#else	CS_LINT
			if ( bcmp(eh->ether_dhost.ether_addr_octet, sc->is_addr.ether_addr_octet, 6) == NULL )
#endif	CS_LINT
					qeread(sc, &sc->qeuba.ifu_r[sc->rindex],
						len - sizeof(struct ether_header));
		}
		/*
		 * Return the buffer to the ring
		 */
		bufaddr = sc->qeuba.ifu_r[sc->rindex].ifrw_info & mask;
		rp->qe_buf_len = -((MAXPACKETSIZE)/2);
		rp->qe_addr_lo = (short)bufaddr;
		rp->qe_addr_hi = (short)((int)bufaddr >> 16);
		rp->qe_flag = rp->qe_status1 = QE_NOTYET;
		rp->qe_valid = 1;
	}
}
/*
 * Ethernet output routine.
 * Encapsulate a packet of type family for the local net.
 * Use trailer local net encapsulation if enough data in first
 * packet leaves a multiple of 512 bytes of data in remainder.
 */
qeoutput(ifp, m0, dst)
	struct ifnet *ifp;
	struct mbuf *m0;
	struct sockaddr *dst;
{
	int type, s, error;
	struct ether_addr edst;
	struct in_addr idst;
#if	CS_VAXM
#else	CS_VAXM
	struct protosw *pr;
#endif	CS_VAXM
	register struct qe_softc *is = &qe_softc[ifp->if_unit];
	register struct mbuf *m = m0;
	register struct ether_header *eh;
	register int off;

	switch (dst->sa_family) {

#ifdef INET
	case AF_INET:
		if (nINET == 0) {
			printf("qe%d: can't handle af%d\n", ifp->if_unit,
				dst->sa_family);
			error = EAFNOSUPPORT;
			goto bad;
		}
		idst = ((struct sockaddr_in *)dst)->sin_addr;
#if	CS_LINT
		if (!arpresolve((struct arpcom *)&is->is_ac, m, &idst, (u_char *)&edst))
#else	CS_LINT
		if (!arpresolve(&is->is_ac, m, &idst, &edst))
#endif	CS_LINT
			return (0);	/* if not yet resolved */
		off = ntohs((u_short)mtod(m, struct ip *)->ip_len) - m->m_len;
		/* need per host negotiation */
		if ((ifp->if_flags & IFF_NOTRAILERS) == 0)
		if (off > 0 && (off & 0x1ff) == 0 &&
			m->m_off >= MMINOFF + 2 * sizeof (u_short)) {
			type = ETHERTYPE_TRAIL + (off>>9);
			m->m_off -= 2 * sizeof (u_short);
			m->m_len += 2 * sizeof (u_short);
			*mtod(m, u_short *) = htons((u_short)ETHERTYPE_IP);
			*(mtod(m, u_short *) + 1) = htons((u_short)m->m_len);
			goto gottraqeertype;
		}
		type = ETHERTYPE_IP;
		off = 0;
		goto gottype;
#endif

	case AF_UNSPEC:
		eh = (struct ether_header *)dst->sa_data;
		edst = eh->ether_dhost;
		type = eh->ether_type;
		goto gottype;

	default:
#if	CS_VAXM
		{
#else	CS_VAXM
		/*
		 * Try to find other address families and call protocol
		 * specific output routine.
		 */
		if (pr = iffamily_to_proto(dst->sa_family)) {
			(*pr->pr_ifoutput)(ifp, m0, dst, &type, (char *)&edst);
			goto gottype;
		} else {
#endif	CS_VAXM
			printf("qe%d: can't handle af%d\n", ifp->if_unit,
				dst->sa_family);
			error = EAFNOSUPPORT;
			goto bad;
		}
	}

gottraqeertype:
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
	if (m->m_off > MMAXOFF || MMINOFF + sizeof (struct ether_header) > m->m_off) {
		m = m_get(M_DONTWAIT, MT_HEADER);
		if (m == 0) {
			error = ENOBUFS;
			goto bad;
		}
		m->m_next = m0;
		m->m_off = MMINOFF;
		m->m_len = sizeof (struct ether_header);
	} else {
		m->m_off -= sizeof (struct ether_header);
		m->m_len += sizeof (struct ether_header);
	}
	eh = mtod(m, struct ether_header *);
	eh->ether_type = htons((u_short)type);
	eh->ether_dhost = edst;
	eh->ether_shost = is->is_addr;

	/*
	 * Queue message on interface, and start output if interface
	 * not yet active.
	 */
	s = splimp();
	if (IF_QFULL(&ifp->if_snd)) {
		IF_DROP(&ifp->if_snd);
		splx(s);
		m_freem(m);
		return (ENOBUFS);
	}
	IF_ENQUEUE(&ifp->if_snd, m);
	qestart(ifp->if_unit);
	splx(s);
	return (0);

bad:
	m_freem(m0);
	return (error);
}
 

/*
 * Process an ioctl request.
 */
qeioctl(ifp, cmd, data)
	register struct ifnet *ifp;
	int cmd;
	caddr_t data;
{
	struct qe_softc *sc = &qe_softc[ifp->if_unit];
#if	CS_VAXM
	register struct ifaddr *ifa = (struct ifaddr *)data;
#else	CS_VAXM
	struct uba_device *ui = qeinfo[ifp->if_unit];
	struct qedevice *addr = (struct qedevice *)ui->ui_addr;
#endif	CS_VAXM
	struct sockaddr *sa;
	struct sockaddr_in *sin;
	struct ifreq *ifr = (struct ifreq *)data;
#if	CS_VAXM
	int s = splimp(), error = 0;
#else	CS_VAXM
	struct ifdevea *ifd = (struct ifdevea *)data;
	struct ctrreq *ctr = (struct ctrreq *)data;
	struct protosw *pr;
	int i,j = -1,s = splimp(), error = 0;
#endif	CS_VAXM

	switch (cmd) {

#if	CS_VAXM
#else	CS_VAXM
	case SIOCENABLBACK:
		printf("qe%d: internal loopback enable requested\n", ifp->if_unit);
		if((ifp->if_flags |= IFF_LOOPBACK) & IFF_RUNNING)
			if_rtinit(ifp, -1);
		qerestart( sc );
		break;
 
	case SIOCDISABLBACK:
		printf("qe%d: internal loopback disable requested\n", ifp->if_unit);
		if((ifp->if_flags &= ~IFF_LOOPBACK) & IFF_RUNNING)
			if_rtinit(ifp, -1);
		qerestart( sc );
		qeinit( ifp->if_unit );
		break;
 
	case SIOCRPHYSADDR:
		bcopy(sc->is_addr.ether_addr_octet, ifd->current_pa, 6);
		for( i = 0; i < 6; i++ )
			ifd->default_pa[i] = addr->qe_sta_addr[i] & 0xff;
		break;
 
	case SIOCSPHYSADDR:
		bcopy(ifr->ifr_addr.sa_data,sc->is_addr.ether_addr_octet,MULTISIZE);
		for ( i = 0; i < 6; i++ )
			sc->setup_pkt[i][1] = sc->is_addr.ether_addr_octet[i];
		if (ifp->if_flags & IFF_RUNNING) {
			qesetup( sc );
			if_rtinit(ifp, -1);
		}
		qeinit(ifp->if_unit);
		break;

	case SIOCDELMULTI:
	case SIOCADDMULTI:
		if (cmd == SIOCDELMULTI) {
			for (i = 0; i < NMULTI; i++)
				if (bcmp(&sc->multi[i],ifr->ifr_addr.sa_data,MULTISIZE) == 0) {
					if (--sc->muse[i] == 0)
						bcopy(unused_multi,&sc->multi[i],MULTISIZE);
				}
		} else {
			for (i = 0; i < NMULTI; i++) {
				if (bcmp(&sc->multi[i],ifr->ifr_addr.sa_data,MULTISIZE) == 0) {
					sc->muse[i]++;
					goto done;
				}
				if (bcmp(&sc->multi[i],unused_multi,MULTISIZE) == 0)
					j = i;
			}
			if (j == -1) {
				printf("qe%d: SIOCADDMULTI failed, multicast list full: %d\n",ui->ui_unit,NMULTI);
				error = ENOBUFS;
				goto done;
			}
			bcopy(ifr->ifr_addr.sa_data, &sc->multi[j], MULTISIZE);
			sc->muse[j]++;
		}
		for ( i = 0; i < 6; i++ )
			sc->setup_pkt[i][1] = sc->is_addr.ether_addr_octet[i];
		if (ifp->if_flags & IFF_RUNNING) {
			qesetup( sc );
		}
		break;

	case SIOCRDCTRS:
	case SIOCRDZCTRS:
		ctr->ctr_ether = sc->ctrblk;
		ctr->ctr_type = CTR_ETHER;
		ctr->ctr_ether.est_seconds = (time.tv_sec - sc->ztime) > 0xfffe ? 0xffff : (time.tv_sec - sc->ztime);
		if (cmd == SIOCRDZCTRS) {
			sc->ztime = time.tv_sec;
			bzero(&sc->ctrblk, sizeof(struct estat));
		}
		break;
#endif	CS_VAXM

	case SIOCSIFADDR:
#if	CS_VAXM
		ifp->if_flags |= IFF_UP;
		qeinit(ifp->if_unit);

		switch (ifa->ifa_addr.sa_family) {
#ifdef INET
		case AF_INET:
			((struct arpcom *)ifp)->ac_ipaddr =
				IA_SIN(ifa)->sin_addr;
			arpwhohas((struct arpcom *)ifp, &IA_SIN(ifa)->sin_addr);
/*			qesetaddr(ifa, sin);*/
			break;
#endif
		default:
			error = EINVAL;
		}
#else	CS_VAXM
		sa = (struct sockaddr *)data;
		if( sa->sa_family == AF_UNSPEC) {
			if( sa->sa_data[0] & 1) {  /*Broad or Multicast*/
				splx(s);
				return (EINVAL);
			}
			sc->is_addr = *(struct ether_addr *)sa->sa_data;
			qeinit(ifp->if_unit);
			break;
		}
		if (sa->sa_family != AF_INET) {
			if (pr = iffamily_to_proto(sa->sa_family))
				error = (*pr->pr_ifioctl)(ifp, cmd, data);
			splx(s);
			return (error);
		}
		sin = (struct sockaddr_in *)data;
		if (ifp->if_flags & IFF_RUNNING)
			if_rtinit(ifp, -1);	/* delete previous route */
		qesetaddr(ifp, sin);
		qeinit(ifp->if_unit);
#endif	CS_VAXM
		break;

	default:
#if	CS_VAXM
		error = EINVAL;
	}
	splx(s);
#else	CS_VAXM
		if (pr = iffamily_to_proto(sa->sa_family))
			error = (*pr->pr_ifioctl)(ifp, cmd, data);
		else
			error = EINVAL;
	}
done:	splx(s);
#endif	CS_VAXM
	return (error);
}

#if	CS_VAXM
qesetaddr(ifa, sin)
	register struct ifaddr *ifa;
#else	CS_VAXM
qesetaddr(ifp, sin)
	register struct ifnet *ifp;
#endif	CS_VAXM
	register struct sockaddr_in *sin;
{

#if	CS_GENERIC
	ifa->ifa_addr = *(struct sockaddr *)sin;
#else	CS_GENERIC
	ifp->if_addr = *(struct sockaddr *)sin;
	ifp->if_net = in_netof(sin->sin_addr);
	ifp->if_host[0] = in_lnaof(sin->sin_addr);
	sin = (struct sockaddr_in *)&ifp->if_broadaddr;
	sin->sin_family = AF_INET;
	sin->sin_addr = if_makeaddr(ifp->if_net, INADDR_ANY);
#endif	CS_GENERIC
}
 

/*
 * Initialize a ring descriptor with mbuf allocation side effects
 */
#if	CS_LINT
qeinitdesc(rp,cbuf,len)
#else	CS_LINT
qeinitdesc( rp, buf, len )
#endif	CS_LINT
	register struct qe_ring *rp;
#if	CS_LINT
	int cbuf; 			/* mapped address	*/
#else	CS_LINT
	char *buf; 			/* mapped address	*/
#endif	CS_LINT
	int len;
{
	/*
	 * clear the entire descriptor
	 */
#if	CS_LINT
	bzero( (caddr_t)rp, sizeof(struct qe_ring));
#else	CS_LINT
	bzero( rp, sizeof(struct qe_ring));
#endif	CS_LINT

	if( len ) {
		rp->qe_buf_len = -(len/2);
#if	CS_LINT
		rp->qe_addr_lo = (short)cbuf;
		rp->qe_addr_hi = (short)((int)cbuf >> 16);
#else	CS_LINT
		rp->qe_addr_lo = (short)buf;
		rp->qe_addr_hi = (short)((int)buf >> 16);
#endif	CS_LINT
	}
}
/*
 * Build a setup packet - the physical address will already be present
 * in first column.
 */
qesetup( sc )
struct qe_softc *sc;
{
	int i, j, offset = 0, next = 3;

	/*
	 * Copy the target address to the rest of the entries in this row.
	 */
	 for ( j = 0; j < 6 ; j++ )
		for ( i = 2 ; i < 8 ; i++ )
			sc->setup_pkt[j][i] = sc->setup_pkt[j][1];
	/*
	 * Duplicate the first half.
	 */
#if	CS_LINT
	bcopy((caddr_t)sc->setup_pkt, (caddr_t)(sc->setup_pkt[8]), 64);
#else	CS_LINT
	bcopy(sc->setup_pkt, sc->setup_pkt[8], 64);
#endif	CS_LINT
	/*
	 * Fill in the broadcast address.
	 */
	for ( i = 0; i < 6 ; i++ )
		sc->setup_pkt[i][2] = 0xff;
	/*
	 * If the device structure is available fill in the multicast address
	 * in the rest of the setup packet.
	 */
	for ( i = 0; i < NMULTI; i++ ) {
#if	CS_LINT
		if (bcmp((caddr_t)(&sc->multi[i]),(caddr_t)unused_multi,MULTISIZE) != 0) {
#else	CS_LINT
		if (bcmp(&sc->multi[i],unused_multi,MULTISIZE) != 0) {
#endif	CS_LINT
			for ( j = 0; j < 6; j++ )
				sc->setup_pkt[offset+j][next] = sc->multi[i].qm_char[j];
			if (++next == 8) {
				next = 1;
				offset = 8;
			}
		}
	}
	sc->setupqueued++;
}
/*
 * Routines supporting Q-BUS network interfaces.
 */

/*
 * Init Q-BUS for interface on uban whose headers of size hlen are to
 * end on a page boundary.  We allocate a Q-BUS map register for the page
 * with the header, and nmr more Q-BUS map registers for i/o on the adapter,
 * doing this for each receive and transmit buffer.  We also
 * allocate page frames in the mbuffer pool for these pages.
 */
qe_ubainit(ifu, uban, hlen, nmr, mptr)
	register struct qeuba *ifu;
	int uban, hlen, nmr;
	char *mptr;
{
	register caddr_t cp, dp;
	register struct ifrw *ifrw;
#if	CS_GENERIC
	register struct qe_ifxmt *ifxp;
#else	CS_GENERIC
	register struct ifxmt *ifxp;
#endif	CS_GENERIC
	int i, ncl;

	ncl = clrnd(nmr + CLSIZE) / CLSIZE;
	if (ifu->ifu_r[0].ifrw_addr)
		/*
		 * If the first read buffer has a non-zero
		 * address, it means we have already allocated core
		 */
		cp = ifu->ifu_r[0].ifrw_addr - (CLBYTES - hlen);
	else {
		cp = mptr;
		if (cp == 0)
			return (0);
		ifu->ifu_hlen = hlen;
		ifu->ifu_uban = uban;
		ifu->ifu_uba = uba_hd[uban].uh_uba;
		dp = cp + CLBYTES - hlen;
		for (ifrw = ifu->ifu_r; ifrw < &ifu->ifu_r[nNRCV]; ifrw++) {
			ifrw->ifrw_addr = dp;
			dp += ncl * CLBYTES;
		}
		for (ifxp = ifu->ifu_w; ifxp < &ifu->ifu_w[nNXMT]; ifxp++) {
			ifxp->x_ifrw.ifrw_addr = dp;
			dp += ncl * CLBYTES;
		}
	}
	/* allocate for receive ring */
	for (ifrw = ifu->ifu_r; ifrw < &ifu->ifu_r[nNRCV]; ifrw++) {
		if (qe_ubaalloc(ifu, ifrw, nmr) == 0) {
			struct ifrw *rw;

			for (rw = ifu->ifu_r; rw < ifrw; rw++)
				ubarelse(ifu->ifu_uban, &rw->ifrw_info);
			goto bad;
		}
	}
	/* and now transmit ring */
	for (ifxp = ifu->ifu_w; ifxp < &ifu->ifu_w[nNXMT]; ifxp++) {
		ifrw = &ifxp->x_ifrw;
		if (qe_ubaalloc(ifu, ifrw, nmr) == 0) {
#if	CS_GENERIC
			struct qe_ifxmt *xp;
#else	CS_GENERIC
			struct ifxmt *xp;
#endif	CS_GENERIC

			for (xp = ifu->ifu_w; xp < ifxp; xp++)
				ubarelse(ifu->ifu_uban, &xp->x_ifrw.ifrw_info);
			for (ifrw = ifu->ifu_r; ifrw < &ifu->ifu_r[nNRCV]; ifrw++)
				ubarelse(ifu->ifu_uban, &ifrw->ifrw_info);
			goto bad;
		}
		for (i = 0; i < nmr; i++)
			ifxp->x_map[i] = ifrw->ifrw_mr[i];
		ifxp->x_xswapd = 0;
	}
	return (1);
bad:
	m_pgfree(cp, nNTOT * ncl);
	ifu->ifu_r[0].ifrw_addr = 0;
	return(0);
}

/*
 * Setup either a ifrw structure by allocating Q-BUS map registers,
 * possibly a buffered data path, and initializing the fields of
 * the ifrw structure to minimize run-time overhead.
 */
static
qe_ubaalloc(ifu, ifrw, nmr)
	struct qeuba *ifu;
	register struct ifrw *ifrw;
	int nmr;
{
	register int info;

	info = uballoc(ifu->ifu_uban, ifrw->ifrw_addr,
			nmr*NBPG + ifu->ifu_hlen, ifu->ifu_flags);
	if (info == 0){
		return (0);
	}
	ifrw->ifrw_info = info;
	ifrw->ifrw_bdp = UBAI_BDP(info);
	ifrw->ifrw_proto = UBAMR_MRV | (UBAI_BDP(info) << UBAMR_DPSHIFT);
	ifrw->ifrw_mr = &ifu->ifu_uba->uba_map[UBAI_MR(info) + 1];
	return (1);
}

/*
 * Pull read data off a interface.
 * Len is length of data, with local net header stripped.
 * Off is non-zero if a trailer protocol was used, and
 * gives the offset of the trailer information.
 * We copy the trailer information and then all the normal
 * data into mbufs.  When full cluster sized units are present
 * on the interface on cluster boundaries we can get them more
 * easily by remapping, and take advantage of this here.
 */
#if	CS_GENERIC
struct mbuf *
qeget(ifu, ifrw, totlen, off0, ifp)
#else	CS_GENERIC
struct mbuf *
qeget(ifu, ifrw, totlen, off0)
#endif	CS_GENERIC
	register struct qeuba *ifu;
	register struct ifrw *ifrw;
	int totlen, off0;
#if	CS_GENERIC
	struct ifnet *ifp;
#endif	CS_GENERIC
{
	struct mbuf *top, **mp, *m;
	int off = off0, len;
	register caddr_t cp = ifrw->ifrw_addr + ifu->ifu_hlen;

	top = 0;
	mp = &top;
	while (totlen > 0) {
		MGET(m, M_DONTWAIT, MT_DATA);
		if (m == 0)
			goto bad;
		if (off) {
			len = totlen - off;
			cp = ifrw->ifrw_addr + ifu->ifu_hlen + off;
		} else
			len = totlen;
		if (len >= CLBYTES) {
			struct mbuf *p;
			struct pte *cpte, *ppte;
			int x, *ip, i;

#if	CS_GENERIC
			if (ifp) {
				len = 0;
				goto nopage;
			}
#endif	CS_GENERIC
			MCLGET(p, 1);
			if (p == 0)
				goto nopage;
			len = m->m_len = CLBYTES;
			m->m_off = (int)p - (int)m;
			if(cpu == MVAX_I || !claligned(cp))
				goto copy;

			/*
			 * Switch pages mapped to Q-BUS with new page p,
			 * as quick form of copy.  Remap Q-BUS and invalidate.
			 */
			cpte = &Mbmap[mtocl(cp)*CLSIZE];
			ppte = &Mbmap[mtocl(p)*CLSIZE];
			x = btop(cp - ifrw->ifrw_addr);
			ip = (int *)&ifrw->ifrw_mr[x];
			for (i = 0; i < CLSIZE; i++) {
				struct pte t;
				t = *ppte; *ppte++ = *cpte; *cpte = t;
				*ip++ =
				    cpte++->pg_pfnum|ifrw->ifrw_proto;
				mtpr(TBIS, cp);
				cp += NBPG;
				mtpr(TBIS, (caddr_t)p);
				p += NBPG / sizeof (*p);
			}
			goto nocopy;
		}
nopage:
#if	CS_GENERIC
		m->m_off = MMINOFF;
		if (ifp) {
			m->m_len = MIN(MLEN - sizeof(ifp), len);
			m->m_off += sizeof(ifp);
		} else
			m->m_len = MIN(MLEN, len);
#else	CS_GENERIC
		m->m_len = MIN(MLEN, len);
		m->m_off = MMINOFF;
#endif	CS_GENERIC
copy:
		bcopy(cp, mtod(m, caddr_t), (unsigned)m->m_len);
		cp += m->m_len;
nocopy:
		*mp = m;
		mp = &m->m_next;
		if (off) {
			/* sort of an ALGOL-W style for statement... */
			off += m->m_len;
			if (off == totlen) {
				cp = ifrw->ifrw_addr + ifu->ifu_hlen;
				off = 0;
				totlen = off0;
			}
		} else
			totlen -= m->m_len;
#if	CS_GENERIC
		if (ifp) {
			m->m_len += sizeof(ifp);
			m->m_off -= sizeof(ifp);
			*(mtod(m, struct ifnet **)) = ifp;
			ifp = (struct ifnet *)0;
		}
#endif	CS_GENERIC
	}
	return (top);
bad:
	m_freem(top);
	return (0);
}

/*
 * Map a chain of mbufs onto a network interface
 * in preparation for an i/o operation.
 * The argument chain of mbufs includes the local network
 * header which is copied to be in the mapped, aligned
 * i/o space.
 */
qeput(ifu, n, m)
	struct qeuba *ifu;
	int n;
	register struct mbuf *m;
{
	register struct mbuf *mp;
	register caddr_t cp;
#if	CS_GENERIC
	register struct qe_ifxmt *ifxp;
#else	CS_GENERIC
	register struct ifxmt *ifxp;
#endif	CS_GENERIC
	register struct ifrw *ifrw;
	register int i;
	int xswapd = 0;
	int x, cc, t;
	caddr_t dp;

	ifxp = &ifu->ifu_w[n];
	ifrw = &ifxp->x_ifrw;
	cp = ifrw->ifrw_addr;
	while (m) {
		dp = mtod(m, char *);
		if (cpu != MVAX_I && claligned(cp) && claligned(dp) && m->m_len == CLBYTES) {
			struct pte *pte; int *ip;
			pte = &Mbmap[mtocl(dp)*CLSIZE];
			x = btop(cp - ifrw->ifrw_addr);
			ip = (int *)&ifrw->ifrw_mr[x];
			for (i = 0; i < CLSIZE; i++)
				*ip++ =
				    ifrw->ifrw_proto | pte++->pg_pfnum;
			xswapd |= 1 << (x>>(CLSHIFT-PGSHIFT));
			mp = m->m_next;
			m->m_next = ifxp->x_xtofree;
			ifxp->x_xtofree = m;
			cp += m->m_len;
		} else {
			bcopy(mtod(m, caddr_t), cp, (unsigned)m->m_len);
			cp += m->m_len;
			MFREE(m, mp);
		}
		m = mp;
	}

	/*
	 * Xswapd is the set of clusters we just mapped out.  Ifxp->x_xswapd
	 * is the set of clusters mapped out from before.  We compute
	 * the number of clusters involved in this operation in x.
	 * Clusters mapped out before and involved in this operation
	 * should be unmapped so original pages will be accessed by the device.
	 */
	cc = cp - ifrw->ifrw_addr;
	x = ((cc - ifu->ifu_hlen) + CLBYTES - 1) >> CLSHIFT;
	ifxp->x_xswapd &= ~xswapd;
	while (i = ffs(ifxp->x_xswapd)) {
		i--;
		if (i >= x)
			break;
		ifxp->x_xswapd &= ~(1<<i);
		i *= CLSIZE;
		for (t = 0; t < CLSIZE; t++) {
			ifrw->ifrw_mr[i] = ifxp->x_map[i];
			i++;
		}
	}
	ifxp->x_xswapd |= xswapd;
	return (cc);
}
/*
 * Pass a packet to the higher levels.
 * We deal with the trailer protocol here.
 */
qeread(sc, ifrw, len)
	register struct qe_softc *sc;
	struct ifrw *ifrw;
	int len;
{
	struct ether_header *eh;
    	struct mbuf *m;
#if	CS_LINT
#else	CS_LINT
	struct protosw *pr;
#endif	CS_LINT
	int off, resid;
	struct ifqueue *inq;

	/*
	 * Deal with trailer protocol: if type is PUP trailer
	 * get true type from first 16-bit word past data.
	 * Remember that type was trailer by setting off.
	 */
	eh = (struct ether_header *)ifrw->ifrw_addr;
	eh->ether_type = ntohs((u_short)eh->ether_type);
#define	qedataaddr(eh, off, type)	((type)(((caddr_t)((eh)+1)+(off))))
	if (eh->ether_type >= ETHERTYPE_TRAIL &&
	    eh->ether_type < ETHERTYPE_TRAIL+ETHERTYPE_NTRAILER) {
		off = (eh->ether_type - ETHERTYPE_TRAIL) * 512;
		if (off >= ETHERMTU)
			return;		/* sanity */
		eh->ether_type = ntohs(*qedataaddr(eh, off, u_short *));
		resid = ntohs(*(qedataaddr(eh, off+2, u_short *)));
		if (off + resid > len)
			return;		/* sanity */
		len = off + resid;
	} else
		off = 0;
	if (len == 0)
		return;

	/*
	 * Pull packet off interface.  Off is nonzero if packet
	 * has trailing header; qeget will then force this header
	 * information to be at the front, but we still have to drop
	 * the type and length which are at the front of any trailer data.
	 */
#if	CS_GENERIC
	m = qeget(&sc->qeuba, ifrw, len, off, &sc->is_if);
#else	CS_GENERIC
	m = qeget(&sc->qeuba, ifrw, len, off);
#endif	CS_GENERIC
	if (m == 0)
		return;
	if (off) {
		m->m_off += 2 * sizeof (u_short);
		m->m_len -= 2 * sizeof (u_short);
	}
	switch (eh->ether_type) {

#ifdef INET
	case ETHERTYPE_IP:
		if (nINET==0) {
			m_freem(m);
			return;
		}
		schednetisr(NETISR_IP);
		inq = &ipintrq;
		break;

	case ETHERTYPE_ARP:
		if (nETHER==0) {
			m_freem(m);
			return;
		}
#if	CS_LINT
		arpinput((struct arpcom *)&sc->is_ac, m);
#else	CS_LINT
		arpinput(&sc->is_ac, m);
#endif	CS_LINT
		return;
#endif
	default:
#if	CS_VAXM
		{
#else	CS_VAXM
		/*
		 * see if other protocol families defined
		 * and call protocol specific routines.
		 * If no other protocols defined then dump message.
		 */
		if (pr=iftype_to_proto(eh->ether_type))  {
			if ((m = (struct mbuf *)(*pr->pr_ifinput)(m, &sc->is_if, &inq, eh)) == 0)
				return;
		} else {
			if (sc->ctrblk.est_unrecog != 0xffff)
				sc->ctrblk.est_unrecog++;
#endif	CS_VAXM
			m_freem(m);
			return;
		}
	}

	if (IF_QFULL(inq)) {
		IF_DROP(inq);
		m_freem(m);
		return;
	}
	IF_ENQUEUE(inq, m);
}
/*
 * Watchdog timer routine. There is a condition in the hardware that
 * causes the board to lock up under heavy load. This routine detects
 * the hang up and restarts the device.
 */
qewatch()
{
	register struct qe_softc *sc;
	register int i;
	int inprogress=0;

	for( i=0 ; i<nNQE ; i++ ) {
		sc = &qe_softc[i];
		if( sc->timeout ) 
#if	CS_GENERIC
			if( ++sc->timeout > 1 )
#else	CS_GENERIC
			if( ++sc->timeout > 3 )
#endif	CS_GENERIC
				qerestart( sc );
			else
				inprogress++;
	}
	if( inprogress ){
#if	CS_LINT
		timeout(qewatch, (caddr_t)0, 10*hz);
#else	CS_LINT
		timeout(qewatch, 0, 10*hz);
#endif	CS_LINT
		qewatchrun++;
	} else
		qewatchrun=0;
}
/*
 * Restart for board lockup problem.
 */
qerestart( sc )
	register struct qe_softc *sc;
{
	register struct ifnet *ifp = &sc->is_if;
	register struct qedevice *addr = sc->addr;
	register struct qe_ring *rp;
	register i;

	addr->qe_csr = QE_RESET;
	sc->timeout = 0;
	qesetup( sc );
	for(i = 0, rp = sc->tring; i<nNXMT ; rp++, i++ ){
		rp->qe_flag = rp->qe_status1 = QE_NOTYET;
		rp->qe_valid = 0;
	}
	sc->nxmit = sc->otindex = sc->tindex = sc->rindex = 0;
	if ( ifp->if_flags & IFF_LOOPBACK )
		addr->qe_csr = QE_RCV_ENABLE | QE_INT_ENABLE | QE_XMIT_INT | QE_RCV_INT | QE_ELOOP;
	else
		addr->qe_csr = QE_RCV_ENABLE | QE_INT_ENABLE | QE_XMIT_INT | QE_RCV_INT | QE_ILOOP;
	addr->qe_rcvlist_lo = (short)sc->rringaddr;
	addr->qe_rcvlist_hi = (short)((int)sc->rringaddr >> 16);
	for( i = 0 ; sc != &qe_softc[i] ; i++ )
		;
	qestart( i );
#if	CS_GENERIC
	qerestart_count++;
#else	CS_GENERIC
	printf("qerestart: restarted qe%d \n", i);
#endif	CS_GENERIC
}
#endif
