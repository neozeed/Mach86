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
 * 5799-CGZ (C) COPYRIGHT IBM CORPORATION  1986
 * LICENSED MATERIALS - PROPERTY OF IBM
 * REFER TO COPYRIGHT INSTRUCTIONS FORM NUMBER G120-2083
 */
/* $Header: ud.c,v 5.0 86/01/31 18:16:06 ibmacis ibm42a $ */
/* $Source: /ibm/acis/usr/sys_ca/caio/RCS/ud.c,v $ */

/*
 ***********************************************************************
 * HISTORY
 *  8-May-86  Bill Bolosky (bolosky) at Carnegie-Mellon University
 *	MACH_VM: Fixed ud_vtop to call real_buf_addr so that it will work
 *	with the new VM scheme.
 *
 * 28-Feb-86  Bill Bolosky (bolosky) at Carnegie-Mellon University
 *	Copied defintion of dkblock from the ibm param.h to right before
 *	it is used.
 *
 ***********************************************************************
 */

#include "mach_vm.h"
#endif CMU

#include "ud.h"
#if NUD > 0
 /*
  * UDP disk driver -- ddj
  */

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/dk.h"
#include "../h/buf.h"
#include "../h/conf.h"
#include "../h/dir.h"
#include "../h/user.h"
#include "../h/proc.h"
#include "../h/map.h"
#include "../h/vm.h"
#include "../h/uio.h"
#include "../h/mbuf.h"
#include "../h/socketvar.h"
#include "../h/socket.h"
#include "../netinet/in.h"
#include "../net/if.h"

#include "../ca/debug.h"

#include "../caio/ioccvar.h"
#include "../ca/pte.h"

/* Defines to set and get the MMU TCR */
#include	"../ca/mmu.h"

#define	IPPORT_UDPDISK	541

struct ud_softc {
	int ud_alive;
	struct socket *ud_so;
	struct buf *ud_buf;
	struct sockaddr_in ud_sin;
} ud;

struct buf *udbuf;
struct buf rudbuf;
int	uddebug;		/* debug flag */
#define SHOW_ENQ	0x01	/* show Q'ing requests */

caddr_t ud_vtop();
struct socket *udsock();

#define AOK 0
#define BAD -1
#define ERR -2
#define TMO -3

#define	UD_READ_REQ	1
#define	UD_READ_RESP	2
#define	UD_WRITE_REQ	3
#define	UD_WRITE_RESP	4
#define	UD_BREATH	5
#define	UD_ACK_RESP	6

struct breath_of_life {
	int bol_type;
};
static struct breath_of_life udbol;

caddr_t udstd[] = {
	0
};					  /* no vectors used */

struct iocc_driver uddriver = {
	0, 0, 0,
	0, udstd, "ud", 0, 0, 0, 0
};


/*
 * udinit -  Setup routine for UDP network disk driver - This routine
 *           initializes the disk io buf and UDP socket structures.
 */
udinit()
{
	register int error;

	printf("udinit: starting udp disk driver\n");
	ud.ud_so = udsock();
	if (ud.ud_so == (struct socket *)NULL) {
		printf("udinit: couldn't get socket\n");
		return 0;
	}
	if (error = udbreath()) {
		printf("udinit: udbreath returns errno=%d\n", error);
		return 0;
	}
	printf("udinit: alive!\n");
	printf("udinit: sin_port=%x, sin_addr=%x\n",
	    ud.ud_sin.sin_port, ud.ud_sin.sin_addr.s_addr);
	ud.ud_alive = 1;
	return 1;
}


/*
 * udstrategy - Schedule one buf struct for disk I/O by placing struct
 *	       in device active queue
 */
udstrategy(bp)
	register struct buf *bp;
{
	register int s;

	if (ud.ud_alive == 0) {
		if (udinit() == 0) {
			bp->b_error = ENODEV;
			bp->b_flags |= B_ERROR;
			iodone(bp);
			return;
		}
	}
	/* Test for invalid I/O request */

	if (bp->b_blkno < 0 || bp->b_bcount <= 0) {
		bp->b_error = ENXIO;
		bp->b_flags |= B_ERROR;
		iodone(bp);
		return;
	}
	s = spl5();
	while (udbuf)
		sleep((caddr_t) &udbuf,PZERO+1);
	udrequest(udbuf = bp);
	udbuf = 0;
	wakeup((caddr_t) &udbuf);
	splx(s);
}


udopen(dev)
	dev_t dev;
{
	return 0;
}


udread(dev, uio)
	dev_t dev;
	struct uio *uio;
{
	return (physio(udstrategy, &rudbuf, dev, B_READ, minphys, uio));
}


udwrite(dev, uio)
	dev_t dev;
	struct uio *uio;
{
	return (physio(udstrategy, &rudbuf, dev, B_WRITE, minphys, uio));
}


udrequest(bp)
	register struct buf *bp;
{
	register int sz, bn;
	register caddr_t addr;
	register int status;
	int timedout = 0;

	sz = (bp->b_bcount + 511) >> 9;
#define	dkblock(bp)	((bp)->b_blkno)
	bn = dkblock(bp);
#undef dkblock
	addr = bp->b_un.b_addr;

	while (sz-- > 0) {
		do {
			if (bp->b_flags & B_READ)
				status = udreadsect(bp, addr, bn);
			else
				status = udwritesect(bp, addr, bn);
			switch (status) {
			case AOK:
				addr += 512;
				bn++;
				break;
			case ERR:
				printf("udrequest: errno=%d at bn=%d\n",
				    bp->b_error, bn);
				break;
			case BAD:
				printf("udrequest: bad packet\n");
				break;
			case TMO:
				if (timedout == 0) {
					timedout++;
					printf("ud: server forgot about us\n");
				}
				break;
			}
		}
		while (status == TMO || status == BAD);
		if (timedout) {
			timedout = 0;
			printf("ud: server's up again\n");
		}
	}
	iodone(bp);
	return;
}


struct up_hdr {
	int uh_req;
	int uh_part;
	int uh_bn;
	int uh_error;
};

static struct up_hdr header;

struct ud_packet {
	struct up_hdr up_hdr;
	char up_data[512];
};

static struct ud_packet inpacket, outpacket;

udwritesect(bp, addr, bn)
	register struct buf *bp;
	register caddr_t addr;
	int bn;
{
	register struct up_hdr *uh;
	register int error;
	int part;

	part = minor(bp->b_dev);

	uh = &outpacket.up_hdr;
	uh->uh_req = UD_WRITE_REQ;
	uh->uh_part = part;
	uh->uh_bn = bn;
	uh->uh_error = 0;
	udfromuser(addr, outpacket.up_data, 512, bp);

	uh = &header;
	do {
		udflushrecv();
		error = udsend((caddr_t) & outpacket, sizeof outpacket);
		if (error)
			goto err;

		error = udrecv((caddr_t) & header, sizeof header, 0);
		if (error)
			goto err;

	}
	while (uh->uh_req == UD_BREATH);

	if (uh->uh_req != UD_WRITE_RESP)
		return BAD;
	if (uh->uh_part != part || uh->uh_bn != bn)
		return BAD;
	header.uh_req = UD_ACK_RESP;
	error = udsend((caddr_t) & header, sizeof header);
	if (error)
		goto err;
	error = uh->uh_error;
	if (!error)
		return AOK;

err:
	bp->b_error = error;
	bp->b_flags |= B_ERROR;
	return ERR;
}


udreadsect(bp, addr, bn)
	register struct buf *bp;
	register caddr_t addr;
	int bn;
{
	register struct up_hdr *uh;
	register int error;
	int part;

	part = minor(bp->b_dev);

	uh = &header;
	uh->uh_req = UD_READ_REQ;
	uh->uh_part = part;
	uh->uh_bn = bn;
	uh->uh_error = 0;

	uh = &inpacket.up_hdr;
	do {
		udflushrecv();
		error = udsend((caddr_t) & header, sizeof header);
		if (error)
			goto err;

		error = udrecv((caddr_t) & inpacket, sizeof inpacket, 0);
		if (error)
			goto err;

	}
	while (uh->uh_req == UD_BREATH);

	if (uh->uh_req != UD_READ_RESP)
		return BAD;
	if (uh->uh_part != part || uh->uh_bn != bn)
		return BAD;
	header.uh_req = UD_ACK_RESP;
	error = udsend((caddr_t) & header, sizeof header);
	if (error)
		goto err;
	error = uh->uh_error;
	if (!error) {
		udtouser(inpacket.up_data, addr, 512, bp);
		return AOK;
	}
err:
	bp->b_error = error;
	bp->b_flags |= B_ERROR;
	return ERR;
}


udflushrecv()
{
	register struct socket *so = ud.ud_so;
	struct sockaddr_in sin;

	while (soreadable(so)) {
		udrecv((caddr_t) & inpacket, sizeof inpacket, &sin);
		if (bcmp((caddr_t) & sin, (caddr_t) & ud.ud_sin, sizeof sin) != 0) {
			bcopy((caddr_t) & sin, (caddr_t) & ud.ud_sin, sizeof sin);
			printf("udflush: new sin_port=%x, sin_addr=%x\n",
			    ud.ud_sin.sin_port, ud.ud_sin.sin_addr.s_addr);
		}
	}
}


static struct uio uuio;
static struct iovec uiov;

udsend(addr, length)
	caddr_t addr;
	int length;
{
	register struct uio *uio = &uuio;
	register struct iovec *iov = &uiov;
	register struct socket *so = ud.ud_so;
	register int error;
	struct mbuf *to;

	iov->iov_base = addr;
	iov->iov_len = length;

	uio->uio_iov = iov;
	uio->uio_iovcnt = 1;
	uio->uio_resid = iov->iov_len;
	uio->uio_segflg = 1;		  /* KERNEL SEGMENT */
	uio->uio_offset = 0;

	to = m_get(M_WAIT, MT_SONAME);
	to->m_len = sizeof ud.ud_sin;
	bcopy((caddr_t) & ud.ud_sin, mtod(to, caddr_t), sizeof ud.ud_sin);

	error = sosend(so, to, uio, 0, (struct mbuf **)0);
	m_freem(to);
	if (error)
		printf("udsend: sosend returns %d\n", error);
	return error;
}


udrecv(addr, length, save_from)
	caddr_t addr;
	int length;
	struct sockaddr_in *save_from;
{
	register struct uio *uio = &uuio;
	register struct iovec *iov = &uiov;
	register struct socket *so = ud.ud_so;
	struct mbuf *from;
	register int error;

	iov->iov_base = addr;
	iov->iov_len = length;

	uio->uio_iov = iov;
	uio->uio_iovcnt = 1;
	uio->uio_resid = iov->iov_len;
	uio->uio_segflg = 1;		  /* KERNEL SEGMENT */
	uio->uio_offset = 0;

	error = soreceive(so, &from, uio, 0, (struct mbuf **)0);
	if (from) {
		if (save_from)
			bcopy(mtod(from, caddr_t),
			    (caddr_t)save_from, from->m_len);
		m_freem(from);
	}
	if (error)
		printf("udrecv: soreceive returns %d\n", error);
	return error;
}


udtouser(from, to, len, bp)
	register caddr_t from, to;
	register int len;
	register struct buf *bp;
{
	register int s = spl5();	  /* paranoia */
	register int i;
	register caddr_t udrbaddr;

	SET_VR0;			  /* Set flag seg 0 (Virt = Real) */


	if (((int)to & PGOFSET) == 0) {
		for (i = len; i > 0; i -= NBPG, from += NBPG, to += NBPG) {
			udrbaddr = ud_vtop(bp, to);
			bcopy(from, udrbaddr, (i < NBPG) ? i : NBPG);
		}
	} else
	{
		while (len-- > 0) {
			*(ud_vtop(bp, to++)) = *from++;
		}
	}
	CLR_VR0;			  /* Clear flag seg 0 (Virt -> Real) */
	splx(s);
}


udfromuser(from, to, len, bp)
	register caddr_t from, to;
	register int len;
	register struct buf *bp;
{
	register int s = spl5();	  /* paranoia */
	register int i;
	register caddr_t udrbaddr;

	SET_VR0;			  /* Set flag seg 0 (Virt = Real) */


	if (((int)from & PGOFSET) == 0) {
		for (i = len; i > 0; i -= NBPG, to += NBPG, from += NBPG) {
			udrbaddr = ud_vtop(bp, from);
			bcopy(udrbaddr, to, (i < NBPG) ? i : NBPG);
		}
	} else
	{
		while (len-- > 0) {
			*to++ = *(ud_vtop(bp, from++));
		}
	}
	CLR_VR0;			  /* Clear flag seg 0 (Virt -> Real) */
	splx(s);

}


/*
 * ud_vtop - translate a virtual address
 * into a physical memory address.
 * The virtual address need not be
 * mapped into an address space.
 */
caddr_t
ud_vtop(bp, bufaddr)
	register struct buf *bp;
	register char *bufaddr;
{
#if	MACH_VM
	return(real_buf_addr(bp,bufaddr));
#else	MACH_VM
	register v, o;
	register struct pte *pte;
	struct proc *p;
	extern caddr_t vtop();

	v = btop(bufaddr);
	o = (int)bufaddr & PGOFSET;
	p = bp->b_flags & B_DIRTY ? &proc[2] : bp->b_proc;

	if ((bp->b_flags & B_PHYS) == 0)
		return (vtop(bufaddr));
	else if (bp->b_flags & B_UAREA)
		pte = &p->p_addr[v];
	else if (bp->b_flags & B_PAGET)
		pte = &Usrptmap[btokmx((struct pte *)bufaddr)];
	else if ((pte = vtopte(p, v)) == 0)
		panic("ud_vtop");

	return ((caddr_t)((pte->pg_pfnum << PGSHIFT) | o));
#endif	MACH_VM
}


struct socket *udsock()
{
	struct sockaddr_in sin;
	struct socket *so;
	struct mbuf *nam;
	int error;

	error = socreate(AF_INET, &so, SOCK_DGRAM, 0);
	if (error) {
		printf("udsock: socreate returns errno %d\n", error);
		return (struct socket *)NULL;
	}
	bzero((caddr_t) & sin, sizeof sin);
	sin.sin_family = AF_INET;
	sin.sin_port = htons(IPPORT_UDPDISK);

	nam = m_get(M_WAIT, MT_SONAME);
	nam->m_len = sizeof sin;
	bcopy((caddr_t) & sin, mtod(nam, caddr_t), sizeof sin);

	error = sobind(so, nam);
	m_freem(nam);
	if (error) {
		printf("udsock: sobind returns errno %d\n", error);
		return (struct socket *)NULL;
	}
	return so;
}


udbreath()
{
	int error;

	for (;;) {
		error = udrecv((caddr_t) & udbol, sizeof udbol, &ud.ud_sin);
		if (error)
			return error;
		if (udbol.bol_type == UD_BREATH)
			return 0;
		printf("udbreath: got type %d\n", udbol.bol_type);
	}
}


#endif NUD

