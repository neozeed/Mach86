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
/*CMU:	if_ml.c	7.2	6/12/84	*/

/*
 *	File:	if_ml.c
 *
 * HISTORY
 *  3-May-84  Avadis Tevanian (avie) at Carnegie-Mellon University
 *	Bug found... it wasn't my fault, it was Berkeley's.  Apparently,
 *	the mbuf code assumes that the page table entries for its
 *	clusters of pages are aligned properly by click size.
 *
 * 23-Apr-84  Avadis Tevanian (avie) at Carnegie-Mellon University
 *	Somewhat working version, although it appears that this
 *	interface is causing crashes by (somehow) trashing mclfree.
 */

#include "wb_ml.h"
#include "mach_mp.h"
#if	WB_ML

/*
 * Multiport Memory Pseudo-Network Interface Controller.
 */

#include "mach_mpm.h"

#include "../machine/pte.h"

#include "../h/map.h"
#include "../h/param.h"
#include "../h/systm.h"
#include "../h/mbuf.h"
#include "../h/buf.h"
#include "../h/protosw.h"
#include "../h/socket.h"
#include "../h/vmmac.h"
#include "../h/ioctl.h"
#include "../h/errno.h"

#include "../net/if.h"
#include "../net/netisr.h"
#include "../net/route.h"
#include "../netinet/in.h"
#include "../netinet/in_systm.h"
#include "../netinet/ip.h"
#include "../netinet/ip_var.h"
#include "../netinet/if_ether.h"
#include "../netpup/pup.h"

#include "../vax/cpu.h"
#include "../vax/mtpr.h"
#include "../vax/nexus.h"
#include "../vaxif/if_ml.h"
#include "../vaxif/if_mlreg.h"
#include "../vaxmpm/mpm_param.h"
#include "../mp/remote_sw.h"

#define MPMNET	126
#define HACKHOST 1		/* temporary until determination of port */

struct	ifnet mlif;
int	*ml_sem;
int	ml_initialized = 0;
int	ml_active = 0;
int	ml_printf = 0;
struct	mlbuf *mlbuf;

int	mlattach(),mlinit(),mlrint(),mloutput(),mlioctl();

/*
 *  Attach the interface.
 */
mlattach(addr)
caddr_t addr;
{
	register struct ifnet *ifp = &mlif;
	struct sockaddr_in *sin;
	int myport;

	mlbuf = (struct mlbuf *) addr;

	myport = CPU_NUMBER();

	ifp->if_name = "ml";
	ifp->if_mtu = WIRESIZE;

/*
 *	Set up our address.
 */

	ifp->if_net = MPMNET;
	ifp->if_host[0] = myport+1;

	sin = (struct sockaddr_in *)&ifp->if_addr;
	sin->sin_family = AF_INET;
	sin->sin_addr = if_makeaddr(MPMNET, ifp->if_host[0]);

	ifp->if_flags = IFF_UP | IFF_RUNNING;
	ifp->if_init = mlinit;
	ifp->if_ioctl = mlioctl;
	ifp->if_output = mloutput;
	if_attach(ifp);
	if_rtinit(ifp, RTF_UP);
}

mlinit()
{	register int mpmmem, i;
	register struct pte *pte;
	struct nexus *nexp = &nexus[2];
	struct mpm_regs *mp = (struct mpm_regs *) nexp;
	int myport;

	myport = CPU_NUMBER();

	ml_sem = &(mlbuf->ml_semaphore);
	*ml_sem = 0;
	ml_initialized = 1;
	printf("ml0 initialized at multiport %d.\n",myport);
}

/*
 * Pull a datagram off the queue and send it to where it wants to go.
 */
mlstart(ifp)
	struct ifnet *ifp;
{
	register struct ifqueue *ifq;
	register struct mbuf *m, *mp;
	register caddr_t cp;
	struct ml_header *ml;
	int dhost;
	int s;
	int count;

	ml_active = 1;
start:
#ifdef	MLDEBUG
	printf("mlstart.\n");
#endif
	s = splimp() ;
	IF_DEQUEUE(&ifp->if_snd, m);
	splx(s) ;
	if (m == 0) {
		ml_active = 0;
		return;
	}

/*	Move packet to multiport memory and cause an interrupt */

	cp = (caddr_t) mlbuf->ml_wire;
#ifdef	MLDEBUG
	printf("Call busyP.\n");
#endif
	busyP(ml_sem,0);
#ifdef	MLDEBUG
	printf("P successful... I've got the semaphore.\n");
#endif
	count = 10000;
	while (mlbuf->ml_wait) {	/* loop optimized for 0!! executions */
		s = splhdw();	/* dangerous, should be done some other way */
		if (--count == 0) {
			if (ml_printf)
			 printf("Machines possibly down: 0x%x ==> %s%s%s%s\n",
					mlbuf->ml_wait,
					mlbuf->ml_wait & 0x1 ? "WB1 " : "",
					mlbuf->ml_wait & 0x2 ? "WB2 " : "",
					mlbuf->ml_wait & 0x4 ? "WB3 " : "",
					mlbuf->ml_wait & 0x8 ? "WB4 " : "");
			mlbuf->ml_upmask &= ~mlbuf->ml_wait;
			mlbuf->ml_wait = 0;
		}
		splx(s);
#ifdef	MLDEBUG
	printf("Looping on 0x%x.\n",mlbuf->ml_wait);
#endif
	}
	s = splimp();
	mlbuf->ml_packetsize = 0;
#ifdef	MLDEBUG
	printf("Enter copy loop.\n");
#endif
	while (m) {
#ifdef	MLDEBUG
		printf("Copy %d bytes.\n",(unsigned)m->m_len);
#endif
		bcopy(mtod(m, caddr_t), cp, (unsigned)m->m_len);
		cp += m->m_len;
		mlbuf->ml_packetsize += m->m_len;
#ifdef	MLDEBUG
		printf("Packetsize = %d. Prepare to MFREE.\n",
				mlbuf->ml_packetsize);
#endif
		MFREE(m, mp);
		m = mp;
	}
#ifdef	MLDEBUG
	printf("Call busyV.\n");
#endif
	ml = (struct ml_header *) mlbuf->ml_wire;
	dhost = ml->ml_dhost;
	if (dhost == 0)
		mlbuf->ml_wait = mlbuf->ml_upmask;
	else	mlbuf->ml_wait = (1 << (dhost - 1));
	busyV(ml_sem,0);
#ifdef	MLDEBUG
	printf("Semaphore now = 0x%x.\n",*ml_sem);
	printf("Cause mpm interrupt on 0x%x.\n",mlbuf->ml_wait);
#endif
	if (dhost == 0)
		for (dhost = 0; dhost < 4; dhost++)
			request_processor(dhost, TYPE_ML);
	else
		request_processor(dhost - 1, TYPE_ML);
#ifdef	MLDEBUG
	printf("spl back to %d.\n",s);
#endif
	splx(s);
	goto start;
}

/*
 * Multiport causes an interrupt and requests us when sending a packet
 * to our port.  We pull the packet out of the multiport and send it
 * off to be processed.
 */

mlrint	()
{
	register struct ifqueue *ifq;
	register struct ifnet *ifp = &mlif;
	register struct ml_header *ml;
	struct mbuf *top, **mp, *m;
	register caddr_t cp;
	int size, myport, isr_type;
	int s;

	myport = CPU_NUMBER();
	if (myport != master_cpu) {
		bbcci(myport,&mlbuf->ml_wait);
		return;
	}

#ifdef	MLDEBUG
	printf("mlrint.\n");
#endif
	if (ml_initialized == 0) {
		printf("Cross interrupt before mapping.\n");
		return;
	}
	top = 0;
	mp = &top;
	cp = (caddr_t) mlbuf->ml_wire;
#ifdef	MLDEBUG
	printf("Packetsize = %d.\n",mlbuf->ml_packetsize);
#endif
	size = mlbuf->ml_packetsize;
	if (size == 0) {
		bbcci(myport,&mlbuf->ml_wait);
		return;
	}
	s = splimp();
	while (size > 0) {
#ifdef	MLDEBUG
		printf("Top of copyin loop, packetsize = %d.\n",size);
#endif
		MGET(m, M_DONTWAIT, MT_DATA);
		if (m == 0) {
			m_freem(top);
			bbcci(myport,&mlbuf->ml_wait);
			splx(s);
			return;
		}
		m->m_len = MIN(MLEN, size);
		m->m_off = MMINOFF;
#ifdef	MLDEBUG
		printf("Got an mbuf, now do copy.\n");
#endif
		bcopy(cp, mtod(m, caddr_t), (unsigned)m->m_len);
		cp += m->m_len;
		size -= m->m_len;
		*mp = m;
		mp = &m->m_next;
	}
	bbcci(myport,&mlbuf->ml_wait);
#ifdef	MLDEBUG
	printf("bbcci done... mlbuf->ml_wait = 0x%x.\n",mlbuf->ml_wait);
#endif
	ifp->if_ipackets++;

	/*
	 * Check that this packet is intended for us.
	 */

	ml = mtod(top, struct ml_header *);
	if ((ml->ml_dhost != ifp->if_host[0]) && (ml->ml_dhost != 0)) {
		m_freem(top);
		splx(s);
		return;
	}

	/*
	 * Fix up first mbuf so that it skips the header info.
	 * ml still points to it though.
	 */

	top->m_off += sizeof(struct ml_header);
	top->m_len -= sizeof(struct ml_header);

	switch (ml->ml_type) {
#ifdef	INET
		case MLTYPE_IP:
			isr_type = NETISR_IP;
			ifq = &ipintrq;
			break;
#endif
		default:
			printf("ml0: input packet not handled.\n");
			m_freem(top);
			splx(s);
			return;
	}

	if (IF_QFULL(ifq)) {
#ifdef	MLDEBUG
		printf("Queue was full.\n");
#endif
		IF_DROP(ifq);
		m_freem(top);
		return (ENOBUFS);
	}
	IF_ENQUEUE(ifq, top);
	schednetisr(isr_type);
	splx(s);
#ifdef	MLDEBUG
	printf("mlrint done.\n");
#endif
}

/*
 * Ethernet output routine.
 * Queue up packets to be send to another machine.
 */

mloutput(ifp, m0, dst)
	struct ifnet *ifp;
	struct mbuf *m0;
	struct sockaddr *dst;
{
	int dest, type, s;
	register struct mbuf *m = m0;
	register struct ml_header *ml;
	int myport;

	myport = CPU_NUMBER();

	/*
	 * Queue message on interface, and start output if interface
	 * not yet active.
	 */

#ifdef	MLDEBUG
	printf("Output a packet to ml.\n");
#endif
	bbssi(myport,&mlbuf->ml_upmask);		/* temporary hack */
	switch (dst->sa_family) {
#ifdef INET
	case AF_INET:
		dest = ((struct sockaddr_in *)dst)->sin_addr.s_addr;
		dest = (dest >> 24) & 0xff;
		type = MLTYPE_IP;
		break;
#endif
	default:
		printf("ml%d: can't handle af%d\n", ifp->if_unit,
			dst->sa_family);
		m_freem(m0);
		return (EAFNOSUPPORT);
	}

	/*
	 * Add local net header.  If no space in first mbuf,
	 * allocate another.
	 */

	if (m->m_off > MMAXOFF ||
	    MMINOFF + sizeof (struct ml_header) > m->m_off) {
		m = m_get(M_DONTWAIT, MT_HEADER);
		if (m == 0) {
			return (ENOBUFS);
		}
		m->m_next = m0;
		m->m_off = MMINOFF;
		m->m_len = sizeof (struct ml_header);
	} else {
		m->m_off -= sizeof (struct ml_header);
		m->m_len += sizeof (struct ml_header);
	}
	ml = mtod(m, struct ml_header *);
	ml->ml_shost = ifp->if_host[0];
	ml->ml_dhost = dest;
	ml->ml_type = type;

	/*
	 * Queue the datagram on the interface and start output if
	 * necessary.
	 */

	s = splimp();
	if (IF_QFULL(&ifp->if_snd)) {
		IF_DROP(&ifp->if_snd);
		splx(s);
		m_freem(m);
		printf("ml0: dropped packet destined for port %d.\n",dest);
		return (ENOBUFS);
	}
	IF_ENQUEUE(&ifp->if_snd, m);
	ifp->if_opackets++;
	splx(s);
	if (ml_active == 0) {
		mlstart(ifp);
	}
	return (0);
}

/*
 * Process an ioctl request.
 */
mlioctl(ifp, cmd, data)
	register struct ifnet *ifp;
	int cmd;
	caddr_t data;
{
	struct ifreq *ifr = (struct ifreq *)data;
	struct sockaddr_in *sin;
	int s = splimp(), error = 0;

	switch (cmd) {

	case SIOCSIFADDR:
		if (ifp->if_flags & IFF_RUNNING)
			if_rtinit(ifp, -1);	/* delete previous route */
		mlsetaddr(ifp, (struct sockaddr_in *)&ifr->ifr_addr);
		if_rtinit(ifp, RTF_UP);
		break;

	default:
		error = EINVAL;
	}
	splx(s);
	return (error);
}

mlsetaddr(ifp, sin)
register struct ifnet *ifp;
register struct sockaddr_in *sin;
{
	ifp->if_net = in_netof(sin->sin_addr);
	ifp->if_host[0] = CPU_NUMBER()+1;
	sin = (struct sockaddr_in *)&ifp->if_addr;
	sin->sin_family = AF_INET;
	sin->sin_addr = if_makeaddr(ifp->if_net, ifp->if_host[0]);
	sin = (struct sockaddr_in *)&ifp->if_broadaddr;
	sin->sin_family = AF_INET;
	sin->sin_addr = if_makeaddr(ifp->if_net, INADDR_ANY);
	ifp->if_flags |= IFF_BROADCAST;
}
#endif	WB_ML
