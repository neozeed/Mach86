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
 * BBN network implementation compatibility interface device driver
 *
 **********************************************************************
 * HISTORY
 **********************************************************************
 */

#include "cs_socket.h"

#ifdef	COMPAT

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/conf.h"
#include "../h/dir.h"
#include "../h/user.h"
#include "../h/proc.h"
#include "../h/file.h"
#include "../h/inode.h"
#include "../h/buf.h"
#include "../h/mbuf.h"
#include "../h/protosw.h"
#include "../h/socket.h"
#include "../h/socketvar.h"
#include "../h/uio.h"
#include "../h/ioctl.h"
#include "../h/fs.h"

#include "../net/route.h"
#include "../netinet/in.h"
#include "../netinet/in_pcb.h"
#include "../netinet/ip_var.h"
#include "../netinet/udp.h"
#include "../netinet/udp_var.h"
#include "../netinet/tcp.h"
#include "../netinet/tcp_timer.h"
#include "../netinet/tcp_var.h"
#include "../netinet/bbn.h"

/*
 *  Maximum number of minor devices.
 */
#define	NBBNDEV 256
/*
 *  Number of bits per minor device free table entry.
 */
#define	BBNBPF (sizeof(int)*8)
/*
 *  Initially all minor devices except 0 are available.
 */
int bbnfree[NBBNDEV/BBNBPF] = {-2, -1, -1, -1, -1, -1, -1, -1};

/*
 *  Active sockets.
 */
struct bbncb
{
    struct socket *bbncb_so;
    struct inode  *bbncb_oip;
    struct mbuf	  *bbncb_m;
    short	   bbncb_state;
    short	   bbncb_mode;
    u_short	   bbncb_fport;
    u_char	   bbncb_noact;
    char	   bbncb_rdnoblk;
} *bbncbtab[NBBNDEV] = {0};



/*
 *  bbngetsname - create the socket name in an mbuf
 */
struct mbuf *
bbngetsname(addr, port)
u_long addr;
u_short port;
{
    register struct mbuf *m;

    m = m_get(M_WAIT, MT_SONAME);
    if (m)
    {
	struct sockaddr_in *sin;

	m->m_len = sizeof(struct sockaddr_in);
	sin = mtod(m, struct sockaddr_in *);
	sin->sin_family = AF_INET;
	sin->sin_port = htons(port);
	sin->sin_addr.s_addr = addr;
	bzero((caddr_t)sin->sin_zero, sizeof(sin->sin_zero));
    }
    return(m);
}



/*
 *  bbnerrno - map 4.2 network error codes to BBN numbers
 */

bbnerrno(errno)
{
	switch(errno)
	{
	    case ECONNABORTED:
		return(ENETERR);
	    case ENOBUFS:
		return(ENETBUF);
	    case EADDRINUSE:
	    case EADDRNOTAVAIL:
		return(ENETRNG);
	    case ETIMEDOUT:
		return(ENETTIM);
	    case ECONNREFUSED:
		return(ENETREF);
	    case EHOSTUNREACH:
		return(ENETDED);
	    case ENETUNREACH:
		return(ENETUNR);
	    case EWOULDBLOCK:
		return(EBLOCK);
	    default:
		return(errno);
	}
}



/*
 *  BBN network interface device open routine
 */

/* ARGSUSED */
bbnopen(dev, flag)
	dev_t dev;
	int flag;
{
	struct a {
		char *name;
		struct con *con;
	} *uap = (struct a *)u.u_ap;
	register struct file *fp = u.u_ofile[u.u_r.r_val1];
	register struct con *cp;
	register struct bbncb *bbncb = 0;
	register struct inode *ip = 0;
	struct inode *oip;
	struct socket *so = 0;
	struct con const;
	struct mbuf *m = 0;
	int mdidx = -1;
	int md;
	int mdbit;
	int error;
	int type;

	if (setcompat("net open", 0) == 0)
		return(u.u_error);

	/*
	 * get user connection structure parm
	 */
	if (error=copyin((caddr_t)uap->con, (caddr_t)&const, sizeof(const)) < 0)
		return(error);
	cp = (struct con *)&const;

	/*
	 *  Allocate a minor device number for the connection.
	 */
	for (mdidx=0; mdidx<sizeof(bbnfree); mdidx++)
	{
	     mdbit = ffs(bbnfree[mdidx]);
	     if (mdbit-- > 0)
		break;
	}
	if (mdidx >= sizeof(bbnfree))
	    return(ENETBUF);
	/*
	 *  Show this minor device now in use and transform it into
	 *  the appropriate index number.
	 */
	bbnfree[mdidx] &= ~(1<<mdbit);
	md=(mdidx*BBNBPF)+mdbit;

	/*
	 *  Allocate a control block for this connection.  Initially most
	 *  fields are NULL since nothing need yet be released on error.
	 *  Remember the original inode, though, since we can't release it
	 *  until we know the open will complete without error (because the
	 *  code which calls us maintains a local pointer to this inode which
	 *  it is going to iput() if we return an error to it).
	 */
	m = m_get(M_WAIT, MT_PCB);
 	if (m == 0)
	{
	    error = ENOBUFS;
	    goto oops;
	}
	bbncb = mtod(m, struct bbncb *);
	bbncb->bbncb_mode = cp->c_mode;
	bbncb->bbncb_rdnoblk = 0;
	bbncb->bbncb_state = 0;
	bbncb->bbncb_so = 0;
	bbncb->bbncb_m = 0;
	bbncb->bbncb_fport = cp->c_fport;
	bbncb->bbncb_noact = cp->c_noact;
	bbncb->bbncb_oip = ((struct inode *)fp->f_data);

	/*
	 *  Allocate ourself a new unique inode to replace the
	 *  common one we were called with.  This ensures that
	 *  the close routine will be called when all references
	 *  to this connection go away (rather than when all
	 *  references to all connections go away).
	 */
	oip = iget(rootdev, getfs(rootdev), ROOTINO);
	if (oip == NULL)
	{
	    error = u.u_error;
	    goto oops;
	}
	ip = (struct inode *)ialloc(oip, ROOTINO, 0);
	iput(oip);
	if (ip == NULL)
	{
	    error = u.u_error;
	    goto oops;
	}

	/*
	 *  Substitute our newly allocated inode in place of the original
	 *  shared inode which all opens use.
	 */
	ip->i_rdev = dev+md;
	ip->i_flag |= IACC|IUPD|ICHG;
	ip->i_mode = IFCHR;
	ip->i_nlink = 0;
	fp->f_data = (caddr_t)ip;
	iunlock(ip);

	/*
	 *  We have now successfully changed the file inode pointer and
	 *  initialized the control block.  Remember the control block for
	 *  future use within the device close routine.
	 *
	 *  Up until this point, none of the preceding code could block
	 *  interruptably so we didn't have to worry about losing track of
	 *  intermediate allocated data structures.  Hereafter, we may block in
	 *  the network code at a number of points and must take care to insure
	 *  that anything we allocate is recorded via the control block so that
	 *  we don't lose track of it if interrupted.  This can happen either
	 *  by a signal (which is caught by the setjmp() below) or because the
	 *  system forces a process exit (which will call the device close
	 *  routine to clean up).
	 */
	bbncbtab[md] = bbncb;
	if (setjmp(&u.u_qsave))
	{
	    if (u.u_error)
		error = u.u_error;
	    else
		error = EINTR;
	    goto oops;
	}

	/*
	 *  Create and initialize a socket.
	 */
	switch(cp->c_mode&CONMASK)
	{
	    case CONTCP:
		type= SOCK_STREAM;
		break;
	    case CONUDP:
		type = SOCK_DGRAM;
		break;
	    default:
		if (cp->c_mode&CONCTL)
		    type = SOCK_STREAM;
		else
		{
		    error = ENETPARM;
		    goto oops;
		}
	}

	/*
	 *  Create a socket of the appropriate type.
	 */
	error = socreate(AF_INET, &so, type, 0);
	if (error)
	    goto oops;
	bbncb->bbncb_so = so;

	/*
	 *  The remaining socket/connection initialization occurs only if the
	 *  device is not a control device.
	 */
	if ((cp->c_mode&CONCTL) == 0)
	{
	    int priv;

	    /*
	     *  Bind the specified local address.
	     */
	    bbncb->bbncb_m = bbngetsname(cp->c_lcon.s_addr, cp->c_lport);
	    if (bbncb->bbncb_m == 0)
	    {
		error = ENOBUFS;
		goto oops;
	    }
	    so->so_options |= (
				SO_REUSEADDR
#if	CS_SOCKET
				|
				SO_CANTSIG
#endif	CS_SOCKET
			      );
	    priv = (so->so_state&SS_PRIV);
	    if (((cp->c_mode&CONACT) && cp->c_lport == 20) ||
		(cp->c_lport == 25 && u.u_uid == 7))
		so->so_state |= SS_PRIV;
	    error = sobind(so, bbncb->bbncb_m);
	    m_freem(bbncb->bbncb_m);
	    bbncb->bbncb_m = 0;
	    so->so_state = (so->so_state&~SS_PRIV)|priv;
	    if (error)
		goto oops;
	    if ((cp->c_mode&CONACT) || ((cp->c_mode&CONUDP) && cp->c_fcon.s_addr))
	    {
		bbncb->bbncb_m = bbngetsname(cp->c_fcon.s_addr, cp->c_fport);
		if (bbncb->bbncb_m == 0)
		{
		    error = ENOBUFS;
		    goto oops;
		}
		error = soconnect(so, bbncb->bbncb_m);
		m_freem(bbncb->bbncb_m);
		bbncb->bbncb_m = 0;
	    }
	    else if (cp->c_mode&CONTCP)
		error = solisten(so, 0);
	    if (error == 0 && (cp->c_mode&CONOBLOK) == 0)
	    {
		error = bbnawaitconn(bbncb);
		so = bbncb->bbncb_so;
	    }
	    bbncb->bbncb_mode ^= CONACT;
	}
	else
	    bbncb->bbncb_mode &= ~CONCTL;
	/*
	 *  Here without errors if socket has been successfully established.
	 *  It is now finally safe to release the original file inode pointer.
	 */
	if (error == 0)
	{
	    oip = bbncb->bbncb_oip;
	    bbncb->bbncb_oip = 0;
	    ilock(oip);
	    iput(oip);
	    return(0);
	}

oops:
	/*
	 *  Something went wrong, clean up all of our mess.
	 */
	if (so)
	{
	    if (soclose(so))
		;
	}
	if (bbncb)
	{
	    if (bbncb->bbncb_m)
		m_freem(bbncb->bbncb_m);
	    m_freem(dtom(bbncb));
	}
 	if (ip)
	{
	    ilock(ip);
	    iput(ip);
	}
	if (mdidx >= 0)
	    bbnfree[mdidx] |= (1<<mdbit);
	return(bbnerrno(error));

#ifdef	UNDEF
	snd = cp->c_sbufs*NMBPG;
	rcv = cp->c_rbufs*NMBPG;
	if (snd < NMBPG || snd > (NMBPG*8))
		snd = NMBPG;
	if (rcv < NMBPG || rcv > (NMBPG*8))
		rcv = NMBPG;
	up->uc_timeo = cp->c_timeo << 1;	/* overlays uc_ssize */
	up->uc_noact = cp->c_noact;		/* overlays uc_rsize */
#endif	UNDEF
}



/*
 *  Device close.
 */

bbnclose(dev)
	dev_t dev;
{
	register int md = minor(dev);
	register struct bbncb *bbncb = bbncbtab[md];

	if (bbncb == 0)
		panic("bbnclose");
	if (bbncb->bbncb_so);
	{
		if (soclose(bbncb->bbncb_so))
			;
	}
	if (bbncb->bbncb_oip)
	{
		ilock(bbncb->bbncb_oip);
		iput(bbncb->bbncb_oip);
	}
	if (bbncb->bbncb_m)
	{
	    m_freem(bbncb->bbncb_m);
	}
	m_freem(dtom(bbncb));
	bbncbtab[md] = 0;
	bbnfree[md/BBNBPF] |= 1<<(md%BBNBPF);
}  



/*
 * Device read routine.
 */

bbnread(dev, uio)
	dev_t dev;
	struct uio *uio;
{
	register struct bbncb *bbncb = bbncbtab[minor(dev)];
	register struct socket *so;
	struct mbuf *from = 0;
	int error;
	label_t tsave;

	bcopy((caddr_t)&u.u_qsave, (caddr_t)&tsave, sizeof(tsave));
	if (setjmp(&u.u_qsave) == 0)
        {
	    error = bbnawaitconn(bbncb);
	    if (error == 0)
	    {
		so = bbncb->bbncb_so;
		if (bbncb->bbncb_rdnoblk)
		    so->so_state |= SS_NBIO;
		else
		    so->so_state &= ~SS_NBIO;
		if (bbncb->bbncb_state && !soreadable(so) && so->so_error == 0)
		    return(ENETSTAT);
		if (bbncb->bbncb_mode&CONTCP)
		    error = soreceive(so, (struct mbuf **)0, uio, 0, (struct mbuf **)0);
		else
		{
		    struct udpiphdr ui;
		    char *base = uio->uio_iov->iov_base;
		    int resid;

		    if ((uio->uio_resid-=sizeof(ui)) < 0)
			return(0);
		    uio->uio_iov->iov_base += sizeof(ui);
		    uio->uio_iov->iov_len -= sizeof(ui);
		    resid = uio->uio_resid;
		    
		    error = soreceive(so, &from, uio, 0, (struct mbuf **)0);
		    if (error == 0)
		    {
			ui.ui_next = 0;
			ui.ui_prev = 0;
			ui.ui_x1 = 0;
			ui.ui_pr = IPPROTO_UDP;
			ui.ui_src = mtod(from, struct sockaddr_in *)->sin_addr;
			ui.ui_sport = mtod(from, struct sockaddr_in *)->sin_port;
			ui.ui_dst = sotoinpcb(so)->inp_laddr;
			ui.ui_dport = sotoinpcb(so)->inp_lport;
			ui.ui_len = ntohs((u_short)(resid-uio->uio_resid+sizeof(struct udphdr)));
			ui.ui_ulen = ui.ui_len;
			ui.ui_sum = 0;
			error = copyout((caddr_t)&ui, base, sizeof(ui));
		    }
		}
	    }
	}
	else
	{
	    if (from)
		m_freem(from);
	    longjmp(&tsave);
	    /*NOTREACHED*/
	}

	if (from)
	    m_freem(from);
	return(bbnerrno(error));
}



/*
 *  Device write.
 */

bbnwrite(dev, uio)
	dev_t dev;
	struct uio *uio;
{
	register struct bbncb *bbncb = bbncbtab[minor(dev)];
	register struct socket *so;
	struct mbuf *to = 0;
	int error;
	label_t tsave;

	bcopy((caddr_t)&u.u_qsave, (caddr_t)&tsave, sizeof(tsave));
	if (setjmp(&u.u_qsave) == 0)
        {
	    if (bbncb->bbncb_state)
		return(ENETSTAT);
	    error = bbnawaitconn(bbncb);
	    if (error == 0)
	    {
		so = bbncb->bbncb_so;
		so->so_state &= ~SS_NBIO;
		if (bbncb->bbncb_mode&CONTCP)
		    error = sosend(so, (struct mbuf *)0, uio, 0, (struct mbuf *)0);
		else
		{
		    char *base = uio->uio_iov->iov_base;

		    if ((bbncb->bbncb_mode&RAWCOMP) == 0)
		    {
			struct udpiphdr ui;

			if ((uio->uio_resid-=sizeof(ui)) < 0)
			    return(0);
			uio->uio_iov->iov_base += sizeof(ui);
			uio->uio_iov->iov_len -= sizeof(ui);
			/*
			 *  Zero length packets to 1 (kludge)
			 */
			if (uio->uio_resid == 0)
			{
			    uio->uio_resid++;
			    uio->uio_iov->iov_len++;
			    uio->uio_iov->iov_base--;
			}
			if ((so->so_state&SS_ISCONNECTED) == 0)
			{
			    error = copyin(base, (caddr_t)&ui, sizeof(ui));
			    if (error == 0)
				to = bbngetsname(ui.ui_dst.s_addr, (u_short)htons(ui.ui_dport));
			}
		    }
		    else
			to = 0;
		    error = sosend(so, to, uio, 0, (struct mbuf *)0);
		}
	    }
	}
	else
	{
	    if (to)
		m_freem(to);
	    longjmp(&tsave);
	    /*NOTREACHED*/
	}

	if (to)
	    m_freem(to);
	return(bbnerrno(error));
}       



/*
 *  Device ioctl.
 */

/* ARGSUSED */
bbnioctl(dev, cmd, data, flag)
	dev_t dev;
	caddr_t	data;
{
	register struct bbncb *bbncb = bbncbtab[minor(dev)];
	register struct socket *so = bbncb->bbncb_so;

	switch (cmd) {

	case NETGETS:                   /* get net status */
	{
		struct netstate *n = (struct netstate *)data;

		n->n_snd = (so->so_snd.sb_mbmax+MSIZE-1)/MSIZE;
		n->n_rcv = (so->so_rcv.sb_mbmax+MSIZE-1)/MSIZE;
		n->n_ssize = (so->so_snd.sb_mbcnt+MSIZE-1)/MSIZE;
		n->n_rsize = (so->so_rcv.sb_mbcnt+MSIZE-1)/MSIZE;
		n->n_xstat = 0;
		n->n_state = bbncb->bbncb_state;
		n->n_flags = bbncb->bbncb_mode;
		n->n_lport = ntohs(sotoinpcb(so)->inp_lport);
		n->n_fport = ntohs(sotoinpcb(so)->inp_fport);
		if (n->n_fport == 0)
			n->n_fport = bbncb->bbncb_fport;
		n->n_lcon = sotoinpcb(so)->inp_laddr;
		n->n_fcon = sotoinpcb(so)->inp_faddr;
		bbncb->bbncb_state = 0;
		break;
	}

	case NETSETU:                   /* set urgent mode */
		bbncb->bbncb_mode |= UURG;
		break;

	case NETRSETU:                  /* reset urgent mode */
		bbncb->bbncb_mode &= ~UURG;
		break;

	case NETSETE:                   /* set eol mode */
		bbncb->bbncb_mode |= UPUSH;
		break;

	case NETRSETE:                  /* reset eol mode */
		bbncb->bbncb_mode &= ~UPUSH;
		break;

	case NETCLOSE:                  /* tcp close but continue to rcv */
		(void) soshutdown(so, FWRITE-1);
		break;

	case NETABORT:                  /* tcp user abort */
	{
		int ipl;

		ipl = splnet();
		if (soabort(so))
		    ;
		splx(ipl);
		break;
	}

	case NETOWAIT:			/* tcp wait until connected */
		return(bbnerrno(bbnawaitconn(bbncb)));

	case NETINIT:			/* init net i/f */
	case NETDISAB:			/* disable net i/f */		
	case NETRESERVE:		/* reserve net i/f (for NU) */
	case NETAVAIL:			/* un-reserve net i/f */
	case NETGINIT:			/* reread the gateway file */
	case NETRESET:                  /* forced tcp reset */
	case NETECHO:			/* set the echo IP address */
		if (!suser())
			return(EPERM);
	case NETDEBUG:                  /* toggle debugging flag */
	case NETTCPOPT:			/* copy ip options to tcb */
	case NETPRADD:			/* add protocol numbers to ucb chain */
	case NETPRDEL:			/* delete protocol numbers from chain */
	case NETPRSTAT:			/* return current list of proto nos. */
		return(EINVAL);

	case NETROUTE:			/* change ip route */
	{
		break;
	}

	case NETFLUSH:			/* flush the TCP buffers */
	{
		break;
	}

	/*
	 *  Enable signal n for input notification
	 */
	case NETENBS:
	{
	    u_int signum = *((u_int *)data);

	    if (signum < NSIG)
	    {
		so->so_rcv.sb_sigp   = u.u_procp;
		so->so_rcv.sb_sigpid = u.u_procp->p_pid;
		so->so_rcv.sb_signum = signum;	/* This must be set last */
	    }
	    else
		return(EINVAL);
	    break;
	}

	/*
	 *  Disable signal for input notification
	 */
	case NETINHS:
	    so->so_rcv.sb_signum = 0;
	    break;

	/*
	 *  Toggle read block input mode
	 */
	case NETRDNOBLK:
	    u.u_r.r_val1 = bbncb->bbncb_rdnoblk = !bbncb->bbncb_rdnoblk;
	    break;

	default:
	    return(ENETPARM);
	}
	return(0);
}



/*
 *  Await connection completion on socket
 */

bbnawaitconn(bbncb)
register struct bbncb *bbncb;
{
    register struct socket *so = bbncb->bbncb_so;
    struct mbuf *m;
    int error = 0;
    int s;

    s = splnet();
    if ((so->so_state & SS_ISCONNECTED) == 0 &&
	(so->so_proto->pr_flags & PR_CONNREQUIRED))
    {
	if (so->so_options&SO_ACCEPTCONN)
	{
	    while (so->so_qlen == 0 && so->so_error == 0)
	    {
		if (so->so_state & SS_CANTRCVMORE)
		{
		    so->so_error = ECONNABORTED;
		    break;
		}
		sleep((caddr_t)&so->so_timeo, PZERO+1);
	    }
	    if (so->so_error == 0)
	    {
		struct socket *nso = so->so_q;

		if (soqremque(nso, 1) == 0)
		    panic("bbnnet accept");
		m = m_get(M_WAIT, MT_SONAME);
		(void)soaccept(nso, m);
		m_freem(m);
		if (soclose(so))
		    ;
		so = nso;
		bbncb->bbncb_so = so;
	    }
	}
	else
	{
	    while ((so->so_state & SS_ISCONNECTING) && so->so_error == 0)
		sleep((caddr_t)&so->so_timeo, PZERO+1);
	}
	error = so->so_error;
	so->so_error = 0;
    }
    splx(s);
    /*
     *  We can't save the inactivity time until we have a TCP control block to
     *  put it in.  Since this routine is potentially called many times, insure
     *  that we only save the time once and then only if we really have a place
     *  to save it.
     */
    if (bbncb->bbncb_noact && error == 0
        &&
        (bbncb->bbncb_mode&CONTCP) && (so->so_state&SS_ISCONNECTED))
    {
	register struct tcpcb *tp = intotcpcb(sotoinpcb(so));

	tp->t_noact = bbncb->bbncb_noact*60*PR_SLOWHZ;
	bbncb->bbncb_noact = 0;
    }
    return(error);
}
#endif	COMPAT
