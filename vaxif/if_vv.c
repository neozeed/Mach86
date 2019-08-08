/*
 * Copyright (c) 1982, 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)if_vv.c	7.1 (Berkeley) 6/5/86
 */

#include "vv.h"
#if NVV > 0

/*
 * Proteon proNET-10 and proNET-80 token ring driver.
 * The name of this device driver derives from the old MIT
 * name of V2LNI for the proNET hardware, would would abbreviate
 * to "v2", but this won't work right. Thus the name is "vv".
 *
 * This driver is compatible with the proNET 10 meagbit and
 * 80 megabit token ring interfaces (models p1000 and p1080).
 * A unit may be marked as 80 megabit using "flags 1" in the
 * config file.
 *
 * TRAILERS: This driver has a new implementation of trailers that
 * is at least a tolerable neighbor on the ring. The offset is not
 * stored in the protocol type, but instead only in the vh_info
 * field. Also, the vh_info field, and the two shorts before the
 * trailing header, are in network byte order, not VAX byte order.
 *
 * Of course, nothing but BSD UNIX supports trailers on ProNET.
 * If you need interoperability with anything else, turn off
 * trailers using the -trailers option to /etc/ifconfig!
 *
 * HARDWARE COMPATABILITY: This driver prefers that the HSBU (p1001)
 * have a serial number >= 040, which is about March, 1982. Older
 * HSBUs do not carry across 64kbyte boundaries. They can be supported
 * by adding "| UBA_NEED16" to the vs_ifuba.ifu_flags initialization
 * in vvattach().
 *
 * The old warning about use without Wire Centers applies only to CTL
 * (p1002) cards with serial <= 057, which have not received ECO 176-743,
 * which was implemented in March, 1982. Most such CTLs have received
 * this ECO.
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

#include "../net/if.h"
#include "../net/netisr.h"
#include "../net/route.h"

#ifdef	INET
#include "../netinet/in.h"
#include "../netinet/in_systm.h"
#include "../netinet/in_var.h"
#include "../netinet/ip.h"
#endif

#include "../vax/cpu.h"
#include "../vax/mtpr.h"
#include "if_vv.h"
#include "if_uba.h"
#include "../vaxuba/ubareg.h"
#include "../vaxuba/ubavar.h"

/*
 *    maximum transmission unit definition --
 *        you can set VVMTU at anything from 576 to 2024.
 *        1536 is a popular "large" value, because it is a multiple
 *	  of 512, which the trailer scheme likes.
 *        The absolute maximum size is 2024, which is enforced.
 */

#define VVMTU (1536)

#define VVMRU (VVMTU + 16)
#define VVBUFSIZE (VVMRU + sizeof(struct vv_header))
#if VVMTU>2024
#undef VVMTU
#undef VVMRU
#undef VVBUFSIZE
#define VVBUFSIZE (2046)
#define VVMRU (VVBUFSIZE - sizeof (struct vv_header))
#define VVMTU (VVMRU - 16)
#endif

/*
 *   debugging and tracing stuff
 */
int	vv_tracehdr = 0;	/* 1 => trace headers (slowly!!) */

#define vvtracehdr  if (vv_tracehdr) vvprt_hdr
#define vvprintf    if (vs->vs_if.if_flags & IFF_DEBUG) printf

/*
 * externals, types, etc.
 */
int	vvprobe(), vvattach(), vvreset(), vvinit();
int	vvidentify(), vvstart(), vvxint(), vvwatchdog();
int	vvrint(), vvoutput(), vvioctl();
struct	uba_device *vvinfo[NVV];
u_short vvstd[] = { 0 };
struct	uba_driver vvdriver =
	{ vvprobe, 0, vvattach, 0, vvstd, "vv", vvinfo };
#define	VVUNIT(x)	minor(x)

#define LOOPBACK		/* use loopback for packets meant for us */
#ifdef	LOOPBACK
extern struct ifnet loif;
#endif

/*
 * Software status of each interface.
 *
 * Each interface is referenced by a network interface structure,
 * vs_if, which the routing code uses to locate the interface.
 * This structure contains the output queue for the interface, its address, ...
 * We also have, for each interface, a UBA interface structure, which
 * contains information about the UNIBUS resources held by the interface:
 * map registers, buffered data paths, etc.  Information is cached in this
 * structure for use by the if_uba.c routines in running the interface
 * efficiently.
 */
struct	vv_softc {
	struct	ifnet vs_if;		/* network-visible interface */
	struct	ifuba vs_ifuba;		/* UNIBUS resources */
	u_short	vs_host;		/* this interface address */
	short	vs_oactive;		/* is output active */
	short	vs_is80;		/* is 80 megabit version */
	short	vs_olen;		/* length of last output */
	u_short	vs_lastx;		/* address of last packet sent */
	u_short	vs_lastr;		/* address of last packet received */
	short	vs_tries;		/* transmit current retry count */
	short	vs_init;		/* number of ring inits */
	short	vs_refused;		/* number of packets refused */
	short	vs_timeouts;		/* number of transmit timeouts */
	short	vs_otimeout;		/* number of output timeouts */
	short	vs_ibadf;		/* number of input bad formats */
	short	vs_parity;		/* number of parity errors on 10 meg, */
					/* link data errors on 80 meg */
} vv_softc[NVV];

#define	NOHOST	0xffff			/* illegal host number */

/*
 * probe the interface to see that the registers exist, and then
 * cause an interrupt to find its vector
 */
vvprobe(reg)
	caddr_t reg;
{
	register int br, cvec;
	register struct vvreg *addr;

#ifdef lint
	br = 0; cvec = br; br = cvec;
#endif
	addr = (struct vvreg *)reg;

	/* reset interface, enable, and wait till dust settles */
	addr->vvicsr = VV_RST;
	addr->vvocsr = VV_RST;
	DELAY(100000);

	/* generate interrupt by doing 1 word DMA from 0 in uba space!! */
	addr->vvoba = 0;		/* low 16 bits */
	addr->vvoea = 0;		/* extended bits */
	addr->vvowc = -1;		/* for 1 word */
	addr->vvocsr = VV_IEN | VV_DEN;	/* start the DMA, with interrupt */
	DELAY(100000);
	addr->vvocsr = VV_RST;		/* clear out the CSR */
	if (cvec && cvec != 0x200)
		cvec -= 4;		/* backup so vector => receive */
	return(1);
}

/*
 * Interface exists: make available by filling in network interface
 * record.  System will initialize the interface when it is ready
 * to accept packets.
 */
vvattach(ui)
	struct uba_device *ui;
{
	register struct vv_softc *vs;

	vs = &vv_softc[ui->ui_unit];
	vs->vs_if.if_unit = ui->ui_unit;
	vs->vs_if.if_name = "vv";
	vs->vs_if.if_mtu = VVMTU;
	vs->vs_if.if_flags = IFF_BROADCAST;
	vs->vs_if.if_init = vvinit;
	vs->vs_if.if_ioctl = vvioctl;
	vs->vs_if.if_output = vvoutput;
	vs->vs_if.if_reset = vvreset;
	vs->vs_if.if_timer = 0;
	vs->vs_if.if_watchdog = vvwatchdog;
	vs->vs_ifuba.ifu_flags = UBA_CANTWAIT | UBA_NEEDBDP;

	/* use flag to determine if this is proNET-80 */
	vs->vs_is80 = (short)(ui->ui_flags & 01);

#if defined(VAX750)
	/* don't chew up 750 bdp's */
	if (cpu == VAX_750 && ui->ui_unit > 0)
		vs->vs_ifuba.ifu_flags &= ~UBA_NEEDBDP;
#endif
	if_attach(&vs->vs_if);
}

/*
 * Reset of interface after UNIBUS reset.
 * If interface is on specified uba, reset its state.
 */
vvreset(unit, uban)
	int unit, uban;
{
	register struct uba_device *ui;

	if (unit >= NVV || (ui = vvinfo[unit]) == 0 || ui->ui_alive == 0 ||
	    ui->ui_ubanum != uban)
		return;
	printf(" vv%d", unit);
	vvinit(unit);
}

/*
 * Initialization of interface; clear recorded pending
 * operations, and reinitialize UNIBUS usage.
 */
vvinit(unit)
	int unit;
{
	register struct vv_softc *vs;
	register struct uba_device *ui;
	register struct vvreg *addr;
	register int ubainfo, s;

	vs = &vv_softc[unit];
	ui = vvinfo[unit];

	if (vs->vs_if.if_addrlist == (struct ifaddr *)0)
		return;

	addr = (struct vvreg *)ui->ui_addr;
	if (if_ubainit(&vs->vs_ifuba, ui->ui_ubanum,
	    sizeof (struct vv_header), (int)btoc(VVMTU)) == 0) {
		printf("vv%d: can't initialize, if_ubainit() failed\n", unit);
		vs->vs_if.if_flags &= ~IFF_UP;
		return;
	}

	/*
	 * Now that the uba is set up, figure out our address and
	 * update complete our host address.
	 */
	if ((vs->vs_host = vvidentify(unit)) == NOHOST) {
		vs->vs_if.if_flags &= ~IFF_UP;
		return;
	}
	printf("vv%d: host %u\n", unit, vs->vs_host);

	/*
	 * Reset the interface, and stay in the ring
	 */
	addr->vvocsr = VV_RST;			/* take over output */
	addr->vvocsr = VV_CPB;			/* clear packet buffer */
	addr->vvicsr = VV_RST | VV_HEN;		/* take over input, */
						/* keep relay closed */
	DELAY(500000);				/* let contacts settle */

	vs->vs_init = 0;			/* clear counters, etc. */
	vs->vs_refused = 0;
	vs->vs_timeouts = 0;
	vs->vs_otimeout = 0;
	vs->vs_ibadf = 0;
	vs->vs_parity = 0;
	vs->vs_lastx = 256;			/* an invalid address */
	vs->vs_lastr = 256;			/* an invalid address */

	/*
	 * Hang a receive and start any
	 * pending writes by faking a transmit complete.
	 */
	s = splimp();
	ubainfo = vs->vs_ifuba.ifu_r.ifrw_info;
	addr->vviba = (u_short)ubainfo;
	addr->vviea = (u_short)(ubainfo >> 16);
	addr->vviwc = -(VVBUFSIZE) >> 1;
	addr->vvicsr = VV_IEN | VV_HEN | VV_DEN | VV_ENB;
	vs->vs_oactive = 1;
	vs->vs_if.if_flags |= IFF_RUNNING;
	vvxint(unit);
	splx(s);
}

/*
 * Do a moderately thorough self-test in all three modes. Mostly
 * to keeps defective nodes off the ring, rather than to be especially
 * thorough. The key issue is to detect any cable breaks before joining
 * the ring. Return our node address on success, return -1 on failure.
 *
 */

/* the three self-test modes */
static u_short vv_modes[] = {
	VV_STE|VV_LPB,			/* digital loopback */
	VV_STE,				/* analog loopback */
	VV_HEN				/* network mode */
};

vvidentify(unit)
	int unit;
{
	register struct vv_softc *vs;
	register struct uba_device *ui;
	register struct vvreg *addr;
	register struct mbuf *m;
	register struct vv_header *v;
	register int ubainfo;
	register int i, successes, failures, waitcount;
	u_short shost = NOHOST;

	vs = &vv_softc[unit];
	ui = vvinfo[unit];
	addr = (struct vvreg *)ui->ui_addr;

	/*
	 * Build a multicast message to identify our address
	 * We need do this only once, since nobody else is about to use
	 * the intermediate transmit buffer (ifu_w.ifrw_addr) that
	 * if_ubainit() aquired for us.
	 */
	m = m_get(M_DONTWAIT, MT_HEADER);
	if (m == NULL) {
		printf("vv%d: can't initialize, m_get() failed\n", unit);
		return (0);
	}
	m->m_next = 0;
	m->m_off = MMINOFF;
	m->m_len = sizeof(struct vv_header);
	v = mtod(m, struct vv_header *);
	v->vh_dhost = VV_BROADCAST;	/* multicast destination address */
	v->vh_shost = 0;		/* will be overwritten with ours */
	v->vh_version = RING_VERSION;
	v->vh_type = RING_DIAGNOSTICS;
	v->vh_info = 0;
	/* map xmit message into uba, copying to intermediate buffer */
	vs->vs_olen = if_wubaput(&vs->vs_ifuba, m);

	/*
	 * For each of the modes (digital, analog, network), go through
	 * a self-test that requires me to send VVIDENTSUCC good packets
	 * in VVIDENTRETRY attempts. Use broadcast destination to find out
	 * who I am, then use this as my address to check my address match
	 * logic. Only data checked is the vh_type field.
	 */

	for (i = 0; i < 3; i++) {
		successes = 0;	/* clear successes for this mode */
		failures = 0;	/* and clear failures, too */

		/* take over device, and leave ring */
		addr->vvicsr = VV_RST;
		addr->vvocsr = VV_RST;
		addr->vvicsr = vv_modes[i];	/* test mode */

		/*
		 * let the flag and token timers pop so that the init ring bit
		 * will be allowed to work, by waiting about 1 second
		 */
		DELAY(1000000L);

		/*
		 * retry loop
 		 */
		while ((successes < VVIDENTSUCC) && (failures < VVIDENTRETRY))
		{
			/* start a receive */
			ubainfo = vs->vs_ifuba.ifu_r.ifrw_info;
			addr->vvicsr = VV_RST | vv_modes[i]; /* abort last */
			addr->vviba = (u_short) ubainfo;
			addr->vviea = (u_short) (ubainfo >> 16);
			addr->vviwc = -(VVBUFSIZE) >> 1;
			addr->vvicsr = vv_modes[i] | VV_DEN | VV_ENB;

			/* purge stale data from BDP */
			if (vs->vs_ifuba.ifu_flags & UBA_NEEDBDP)
				UBAPURGE(vs->vs_ifuba.ifu_uba,
				    vs->vs_ifuba.ifu_w.ifrw_bdp);

			/* do a transmit */
			ubainfo = vs->vs_ifuba.ifu_w.ifrw_info;
			addr->vvocsr = VV_RST;	/* abort last try */
			addr->vvoba = (u_short) ubainfo;
			addr->vvoea = (u_short) (ubainfo >> 16);
			addr->vvowc = -((vs->vs_olen + 1) >> 1);
			addr->vvocsr = VV_CPB | VV_DEN | VV_INR | VV_ENB;

			/* poll receive side for completion */
			DELAY(10000);		/* give it a chance */
			for (waitcount = 0; waitcount < 10; waitcount++) {
				if (addr->vvicsr & VV_RDY)
					goto gotit;
				DELAY(1000);
			}
			failures++;		/* no luck */
			continue;

gotit:			/* we got something--is it any good? */
			if ((addr->vvicsr & (VVRERR|VV_LDE)) ||
			    (addr->vvocsr & (VVXERR|VV_RFS))) {
				failures++;
				continue;
			}

			/* Purge BDP before looking at received packet */
			if (vs->vs_ifuba.ifu_flags & UBA_NEEDBDP)
				UBAPURGE(vs->vs_ifuba.ifu_uba,
				    vs->vs_ifuba.ifu_r.ifrw_bdp);
			m = if_rubaget(&vs->vs_ifuba, sizeof(struct vv_header),
				0, &vs->vs_if);
			if (m != NULL)
				m_freem(m);
			
			v = (struct vv_header *)(vs->vs_ifuba.ifu_r.ifrw_addr);

			/* check message type, catch our node address */
			if ((v->vh_type & 0xff) == RING_DIAGNOSTICS) {
				if (shost == NOHOST) {
					shost = v->vh_shost & 0xff;
					/* send to ourself now */
					((struct vv_header *)
					    (vs->vs_ifuba.ifu_r.ifrw_addr))
					    ->vh_dhost = shost;
				}
				successes++;
			} else {
				failures++;
			}
			v->vh_type = 0;  /* clear to check again */
		}

		if (failures >= VVIDENTRETRY)
		{
			printf("vv%d: failed self-test after %d tries \
in %s mode\n",
			    unit, VVIDENTRETRY, i == 0 ? "digital loopback" :
			    (i == 1 ? "analog loopback" : "network"));
			printf("vv%d: icsr = %b, ocsr = %b\n",
			    unit, 0xffff & addr->vvicsr, VV_IBITS,
			    0xffff & addr->vvocsr, VV_OBITS);
			addr->vvicsr = VV_RST;	/* kill the sick board */
			addr->vvocsr = VV_RST;
			shost = NOHOST;
			goto done;
		}
	}

done:
	/* deallocate mbuf used for send packet (won't be one, anyways) */
	if (vs->vs_ifuba.ifu_xtofree) {
		m_freem(vs->vs_ifuba.ifu_xtofree);
		vs->vs_ifuba.ifu_xtofree = 0;
	}

	return(shost);
}

/*
 * Start or restart output on interface.
 * If interface is active, this is a retransmit, so just
 * restuff registers and go.
 * If interface is not already active, get another datagram
 * to send off of the interface queue, and map it to the interface
 * before starting the output.
 */
vvstart(dev)
	dev_t dev;
{
	register struct uba_device *ui;
	register struct vv_softc *vs;
	register struct vvreg *addr;
	register struct mbuf *m;
	register int unit, ubainfo, dest, s;

	unit = VVUNIT(dev);
	ui = vvinfo[unit];
	vs = &vv_softc[unit];
	if (vs->vs_oactive)
		goto restart;
	/*
	 * Not already active: dequeue another request
	 * and map it to the UNIBUS.  If no more requests,
	 * just return.
	 */
	s = splimp();
	IF_DEQUEUE(&vs->vs_if.if_snd, m);
	splx(s);
	if (m == NULL) {
		vs->vs_oactive = 0;
		return;
	}
	dest = mtod(m, struct vv_header *)->vh_dhost;
	vs->vs_olen = if_wubaput(&vs->vs_ifuba, m);
	vs->vs_lastx = dest;
restart:
	/*
	 * Have request mapped to UNIBUS for transmission.
	 * Purge any stale data from this BDP, and start the output.
	 *
	 * Make sure this packet will fit in the interface.
	 */
	if (vs->vs_olen > VVBUFSIZE) {
		printf("vv%d vs_olen: %d > VVBUFSIZE\n", unit, vs->vs_olen);
		panic("vvdriver vs_olen botch");
	}

	vs->vs_if.if_timer = VVTIMEOUT;
	vs->vs_oactive = 1;

	/* ship it */
	if (vs->vs_ifuba.ifu_flags & UBA_NEEDBDP)
		UBAPURGE(vs->vs_ifuba.ifu_uba, vs->vs_ifuba.ifu_w.ifrw_bdp);
	addr = (struct vvreg *)ui->ui_addr;
	ubainfo = vs->vs_ifuba.ifu_w.ifrw_info;
	addr->vvoba = (u_short) ubainfo;
	addr->vvoea = (u_short) (ubainfo >> 16);
	addr->vvowc = -((vs->vs_olen + 1) >> 1);
	addr->vvowc = -((vs->vs_olen + 1) >> 1); /* extra byte is garbage */
	if (addr->vvocsr & VV_NOK)
		vs->vs_init++;			/* count ring inits */
	addr->vvocsr = VV_IEN | VV_CPB | VV_DEN | VV_INR | VV_ENB;
}

/*
 * proNET transmit interrupt
 * Start another output if more data to send.
 */
vvxint(unit)
	int unit;
{
	register struct uba_device *ui;
	register struct vv_softc *vs;
	register struct vvreg *addr;
	register int oc;

	ui = vvinfo[unit];
	vs = &vv_softc[unit];
	vs->vs_if.if_timer = 0;
	addr = (struct vvreg *)ui->ui_addr;
	oc = 0xffff & (addr->vvocsr);
	if (vs->vs_oactive == 0) {
		vvprintf("vv%d: stray interrupt vvocsr = %b\n", unit,
		    oc, VV_OBITS);
		return;
	}

	/*
	 * we retransmit on soft error
	 * TODO: sort retransmits to end of queue if possible!
	 */
	if (oc & (VV_OPT | VV_RFS)) {
		if (vs->vs_tries++ < VVRETRY) {
			if (oc & VV_OPT)
				vs->vs_otimeout++;
			if (oc & VV_RFS) {
				vs->vs_if.if_collisions++;
				vs->vs_refused++;
			}
			vvstart(unit);		/* restart this message */
			return;
		}
	}
	vs->vs_if.if_opackets++;
	vs->vs_oactive = 0;
	vs->vs_tries = 0;

	if (oc & VVXERR) {
		vs->vs_if.if_oerrors++;
		vvprintf("vv%d: error vvocsr = %b\n", unit, 0xffff & oc,
		    VV_OBITS);
	}
	if (vs->vs_ifuba.ifu_xtofree) {
		m_freem(vs->vs_ifuba.ifu_xtofree);
		vs->vs_ifuba.ifu_xtofree = 0;
	}
	vvstart(unit);
}

/*
 * Transmit watchdog timer routine.
 * This routine gets called when we lose a transmit interrupt.
 * The best we can do is try to restart output.
 */
vvwatchdog(unit)
	int unit;
{
	register struct vv_softc *vs;
	register int s;

	vs = &vv_softc[unit];
	vvprintf("vv%d: lost a transmit interrupt.\n", unit);
	vs->vs_timeouts++;
	s = splimp();
	vvstart(unit);
	splx(s);
}

/*
 * proNET interface receiver interrupt.
 * If input error just drop packet.
 * Otherwise purge input buffered data path and examine
 * packet to determine type.  If can't determine length
 * from type, then have to drop packet.  Otherwise decapsulate
 * packet based on type and pass to type specific higher-level
 * input routine.
 */
vvrint(unit)
	int unit;
{
	register struct vv_softc *vs;
	register struct vvreg *addr;
	register struct vv_header *vv;
	register struct ifqueue *inq;
	register struct mbuf *m;
	int ubainfo, len, off, s;
	short resid;

	vs = &vv_softc[unit];
	vs->vs_if.if_ipackets++;
	addr = (struct vvreg *)vvinfo[unit]->ui_addr;

	/*
	 * Purge BDP
	 */
	if (vs->vs_ifuba.ifu_flags & UBA_NEEDBDP)
		UBAPURGE(vs->vs_ifuba.ifu_uba, vs->vs_ifuba.ifu_r.ifrw_bdp);

	/*
	 * receive errors?
	 */
	if (addr->vvicsr & VVRERR) {
		vvprintf("vv%d: receive error, vvicsr = %b\n", unit,
		    0xffff&(addr->vvicsr), VV_IBITS);
		if (addr->vvicsr & VV_BDF)
			vs->vs_ibadf++;
		goto dropit;
	}

	/*
	 * parity errors?
	 */
	if (addr->vvicsr & VV_LDE) {
		/* we don't have to clear it because the receive command */
		/* writes 0 to parity bit */
		vs->vs_parity++;

		/*
		 * only on 10 megabit proNET is VV_LDE an end-to-end parity
		 * bit. On 80 megabit, it returns to the intended use of
		 * node-to-node parity. End-to-end parity errors on 80 megabit
		 * give VV_BDF.
		 */
		if (vs->vs_is80 == 0)
		    goto dropit;
	}

	/*
	 * Get packet length from residual word count
	 *
	 * Compute header offset if trailer protocol
	 *
	 * Pull packet off interface.  Off is nonzero if packet
	 * has trailing header; if_rubaget will then force this header
	 * information to be at the front.  The vh_info field
	 * carries the offset to the trailer data in trailer
	 * format packets.
	 */
	vv = (struct vv_header *)(vs->vs_ifuba.ifu_r.ifrw_addr);
	vvtracehdr("vi", vv);
	resid = addr->vviwc & 01777;	/* only low 10 bits valid */
	if (resid)
		resid |= 0176000;	/* high 6 bits are undefined */
	len = ((VVBUFSIZE >> 1) + resid) << 1;
	len -= sizeof(struct vv_header);

	if ((addr->vvicsr & VV_DPR) || len > VVMRU || len <= 0) {
		vvprintf("vv%d: len too long or short, \
len = %d, vvicsr = %b\n",
			    unit, len, 0xffff&(addr->vvicsr), VV_IBITS);
		goto dropit;
	}

	/* check the protocol header version */
	if (vv->vh_version != RING_VERSION) {
		vvprintf("vv%d: bad protocol header version %d\n",
		    unit, vv->vh_version & 0xff);
		goto dropit;
	}

#define	vvdataaddr(vv, off, type)	((type)(((caddr_t)((vv)+1)+(off))))
	if (vv->vh_type == RING_TRAILER ) {
		off = ntohs((u_short)vv->vh_info);
		if (off > VVMTU) {
			vvprintf("vv%d: off > VVMTU, off = %d, vvicsr = %b\n",
				    unit, off, 0xffff&(addr->vvicsr), VV_IBITS);
			goto dropit;
		}
		vv->vh_type = ntohs(*vvdataaddr(vv, off, u_short *));
		resid = ntohs(*(vvdataaddr(vv, off+sizeof(u_short), u_short *)));
		if (off + resid > len) {
			vvprintf("vv%d: trailer packet too short\n", unit);
			vvprintf("vv%d: off = %d, resid = %d, vvicsr = %b\n",
				    unit, off, resid,
				    0xffff&(addr->vvicsr), VV_IBITS);
			goto dropit;
		}
		len = off + resid;
	} else
		off = 0;

	if (len == 0) {
		vvprintf("vv%d: len is zero, vvicsr = %b\n", unit,
			    0xffff&(addr->vvicsr), VV_IBITS);
		goto dropit;
	}

	m = if_rubaget(&vs->vs_ifuba, len, off, &vs->vs_if);
	if (m == NULL) {
		vvprintf("vv%d: if_rubaget() failed, vvicsr = %b\n", unit,
			    0xffff&(addr->vvicsr), VV_IBITS);
		goto dropit;
	}
	if (off) {
		struct ifnet *ifp;

		ifp = *(mtod(m, struct ifnet **));
		m->m_off += 2 * sizeof (u_short);
		m->m_len -= 2 * sizeof (u_short);
		*(mtod(m, struct ifnet **)) = ifp;
	}

	/* Keep track of source address of this packet */
	vs->vs_lastr = vv->vh_shost;

	/*
	 * Demultiplex on packet type
	 */
	switch (vv->vh_type) {

#ifdef INET
	case RING_IP:
		schednetisr(NETISR_IP);
		inq = &ipintrq;
		break;
#endif
	default:
		vvprintf("vv%d: unknown pkt type 0x%x\n", unit, vv->vh_type);
		m_freem(m);
		goto setup;
	}
	s = splimp();
	if (IF_QFULL(inq)) {
		IF_DROP(inq);
		m_freem(m);
	} else
		IF_ENQUEUE(inq, m);

	splx(s);
	/*
	 * Reset for the next packet.
	 */
setup:
	ubainfo = vs->vs_ifuba.ifu_r.ifrw_info;
	addr->vviba = (u_short) ubainfo;
	addr->vviea = (u_short) (ubainfo >> 16);
	addr->vviwc = -(VVBUFSIZE) >> 1;
	addr->vvicsr = VV_HEN | VV_IEN | VV_DEN | VV_ENB;
	return;

	/*
	 * Drop packet on floor -- count them!!
	 */
dropit:
	vs->vs_if.if_ierrors++;
	goto setup;
}

/*
 * proNET output routine.
 * Encapsulate a packet of type family for the local net.
 * Use trailer local net encapsulation if enough data in first
 * packet leaves a multiple of 512 bytes of data in remainder.
 */
vvoutput(ifp, m0, dst)
	struct ifnet *ifp;
	struct mbuf *m0;
	struct sockaddr *dst;
{
	register struct mbuf *m;
	register struct vv_header *vv;
	register int off;
	register int unit;
	register struct vvreg *addr;
	register struct vv_softc *vs;
	register int s;
	int type, dest, error;

	m = m0;
	unit = ifp->if_unit;
	addr = (struct vvreg *)vvinfo[unit]->ui_addr;
	vs = &vv_softc[unit];

	/*
	 * Check to see if the input side has wedged due the UBA
	 * vectoring through 0.
	 *
	 * We are lower than device ipl when we enter this routine,
	 * so if the interface is ready with an input packet then
	 * an input interrupt must have slipped through the cracks.
	 *
	 * Avoid the race with an input interrupt by watching to see
	 * if any packets come in.
	 */
	s = vs->vs_if.if_ipackets;
	if (addr->vvicsr & VV_RDY && s == vs->vs_if.if_ipackets) {
		vvprintf("vv%d: lost a receive interrupt, icsr = %b\n",
			    unit, 0xffff&(addr->vvicsr), VV_IBITS);
		s = splimp();
		vvrint(unit);
		splx(s);
	}

	switch (dst->sa_family) {

#ifdef INET
	case AF_INET:
		if (in_broadcast(((struct sockaddr_in *)dst)->sin_addr))
			dest = VV_BROADCAST;
		else
			dest = in_lnaof(((struct sockaddr_in *)dst)->sin_addr);
#ifdef LOOPBACK
		if (dest == vs->vs_host && (loif.if_flags & IFF_UP))
			return (looutput(&loif, m0, dst));
#endif LOOPBACK
		if (dest >= 0x100) {
			error = EPERM;
			goto bad;
		}
		off = ntohs((u_short)mtod(m, struct ip *)->ip_len) - m->m_len;
		/*
		 * Trailerize, if the configuration allows it.
		 * TODO: Need per host negotiation.
		 */
		if ((ifp->if_flags & IFF_NOTRAILERS) == 0)
		if (off > 0 && (off & 0x1ff) == 0 &&
		    m->m_off >= MMINOFF + 2 * sizeof (u_short)) {
			type = RING_TRAILER;
			m->m_off -= 2 * sizeof (u_short);
			m->m_len += 2 * sizeof (u_short);
			*mtod(m, u_short *) = htons((short)RING_IP);
			*(mtod(m, u_short *) + 1) = htons((u_short)m->m_len);
			goto gottrailertype;
		}
		type = RING_IP;
		off = 0;
		goto gottype;
#endif
	default:
		printf("vv%d: can't handle af%d\n", unit, dst->sa_family);
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
	    MMINOFF + sizeof (struct vv_header) > m->m_off) {
		m = m_get(M_DONTWAIT, MT_HEADER);
		if (m == NULL) {
			error = ENOBUFS;
			goto bad;
		}
		m->m_next = m0;
		m->m_off = MMINOFF;
		m->m_len = sizeof (struct vv_header);
	} else {
		m->m_off -= sizeof (struct vv_header);
		m->m_len += sizeof (struct vv_header);
	}
	vv = mtod(m, struct vv_header *);
	vv->vh_shost = vs->vs_host;
	vv->vh_dhost = dest;
	vv->vh_version = RING_VERSION;
	vv->vh_type = type;
	vv->vh_info = htons((u_short)off);
	vvtracehdr("vo", vv);

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
	if (vs->vs_oactive == 0)
		vvstart(unit);
	splx(s);
	return (0);
qfull:
	m0 = m;
	splx(s);
bad:
	m_freem(m0);
	return(error);
}

/*
 * Process an ioctl request.
 */
vvioctl(ifp, cmd, data)
	register struct ifnet *ifp;
	int cmd;
	caddr_t data;
{
	struct ifaddr *ifa = (struct ifaddr *) data;
	int s = splimp(), error = 0;

	switch (cmd) {

	case SIOCSIFADDR:
		ifp->if_flags |= IFF_UP;
		if ((ifp->if_flags & IFF_RUNNING) == 0)
			vvinit(ifp->if_unit);
		/*
		 * Did self-test succeed?
		 */
		if ((ifp->if_flags & IFF_UP) == 0)
			error = ENETDOWN;
                /*
                 * Attempt to check agreement of protocol address
                 * and board address.
                 */
		switch (ifa->ifa_addr.sa_family) {
                case AF_INET:
			if (in_lnaof(IA_SIN(ifa)->sin_addr) !=
			    vv_softc[ifp->if_unit].vs_host)
				error = EADDRNOTAVAIL;
			break;
		}
		break;

	default:
		error = EINVAL;
	}
	splx(s);
	return (error);
}

/*
 * vvprt_hdr(s, v) print the local net header in "v"
 *	with title is "s"
 */
vvprt_hdr(s, v)
	char *s;
	register struct vv_header *v;
{
	printf("%s: dsvti: 0x%x 0x%x 0x%x 0x%x 0x%x\n",
		s,
		0xff & (int)(v->vh_dhost), 0xff & (int)(v->vh_shost),
		0xff & (int)(v->vh_version), 0xff & (int)(v->vh_type),
		0xffff & (int)(v->vh_info));
}
#endif NVV
