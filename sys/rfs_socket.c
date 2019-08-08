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
/*	rfs_socket.c	CMU	85/12/13	*/

/*
 *  Remote file system - socket interface module
 *
 **********************************************************************
 * HISTORY
 * 13-Dec-85  Mike Accetta (mja) at Carnegie-Mellon University
 *	Created during reorganization for new RFS name.
 *
 **********************************************************************
 */

#include "cs_rfs.h"

#if	CS_RFS
#include "../h/param.h"
#include "../h/systm.h"
#include "../h/buf.h"
#include "../h/mbuf.h"
#include "../h/protosw.h"
#include "../h/socket.h"
#include "../h/socketvar.h"
#include "../h/ioctl.h"
#include "../h/tty.h"
#include "../h/dir.h"
#include "../h/file.h"
#include "../h/inode.h"
#include "../h/user.h"
#include "../h/map.h"
#include "../h/proc.h"
#include "../h/uio.h"
#include "../h/stat.h"
#include "../h/rfs.h"

#include "../net/route.h"
#include "../netinet/in_pcb.h"



/*
 *  rfsEstablish - establish TCP connection to port on remote host
 *
 *  rcbp   = remote control block for connection
 *  fcon   = IP address of remote host
 *  fport  = remot port number of connection
 *  lport  = local port number for connection
 *  active = active/passive open flag
 *  retry  = times to attempt to connect if refused before giving up
 *
 *  Establishes a connection between the indicated local port number and the
 *  specified port on the remote host.  The active flag indicates whether or
 *  not an active or passive socket should be created. When a connection
 *  attempt fails because it is refused at the remote end (i.e. there was no
 *  corresponding remot port), it will be retried until the requested number of
 *  attempts have been made.  This is to insure that new connections may be
 *  established to port numbers sent on already established connections because
 *  it may take time on each end to initialize the new ports.
 *
 *  The socket and local port fields of the remote control block are filled in
 *  from the established connection.
 *
 *  Return: 0 or an error number as appropriate.
 */

rfsEstablish(rcbp, fcon, fport, lport, active, retry)
    register struct rfsConnectionBlock *rcbp;
    struct in_addr fcon;
    u_short fport;
    u_short lport;
    int active;
    int retry;
{
    struct socket *so = 0;
    int error = 0;
    label_t tsave;

    rfs_printf4(RFSD_P_TRACEIN, "<Estb fa=%X fp=%x lp=%x\n", fcon, fport, lport);

    bcopy((caddr_t)&u.u_qsave, (caddr_t)&tsave, sizeof(label_t));
    for (;;)
    {
	struct mbuf *m;

	if (error = socreate(AF_INET, &so, SOCK_STREAM, 0))
	    goto out;
	m = rfsSocketName((u_long)0, lport);
	if (m == NULL)
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
	error = sobind(so, m);
	m_freem(m);
	if (error)
	    goto oops;
	if (setjmp(&u.u_qsave) != 0)
	{
	    error = EINTR;
	    goto oops;
	}
	if (active)
	{
	    m = rfsSocketName(fcon.s_addr, fport);
	    if (m == NULL)
	    {
		error = ENOBUFS;
		goto oops;
	    }
	    error = soconnect(so, m);
	    m_freem(m);
	    if (error)
		goto oops;
	}
	else
	    error = solisten(so, 0);
	if (error == 0)
	{
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
			    panic("rfsEstablish accept");
			m = m_get(M_WAIT, MT_SONAME);
			(void)soaccept(nso, m);
			m_freem(m);
			if (soclose(so))
			    ;
			so = nso;
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
	}

	switch (error)
	{
	    case 0:
		rcbp->rcb_so = so;
		rcbp->rcb_lport = ntohs(sotoinpcb(so)->inp_lport);
		goto out;

	    case ECONNREFUSED:
		if (--retry <= 0)
		    goto oops;
		if (soclose(so))
		    ;
		so = 0;
		break;

	    default:
		goto oops;
	}
    }

oops:
    if (so)
	(void) soclose(so);
out:
    bcopy((caddr_t)&tsave, (caddr_t)&u.u_qsave, sizeof(label_t));
    rfs_printf2(RFSD_P_TRACEOUT, ">Estb %d\n", error);
    return(error);
}



/*
 *  rfsSocketName - create a socket name in an mbuf
 *
 *  addr = IP address
 *  port = TCP port number
 *
 *  Return: an AF_INET socket name mbuf with the specified address or NULL if
 *  an mbuf acnnot be allocated.
 */
 
struct mbuf *
rfsSocketName(addr, port)
    u_long addr;
    u_short port;
{
    register struct mbuf *m;
    struct sockaddr_in *sin;

    m = m_get(M_WAIT, MT_SONAME);
    if (m != 0)
    {
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
 *  rfsUnEstablish - release a remote connection
 *
 *  rcbp = remote connection block
 *
 *  Close the socket (if any) and clear any error flags so that
 *  the remote connection block appears uninitialized.
 */

void
rfsUnEstablish(rcbp)
    register struct rfsConnectionBlock *rcbp;
{
    if (rcbp->rcb_so)
    {
	(void) soclose(rcbp->rcb_so);
	rcbp->rcb_so = NULL;
	rcbp->rcb_flags &= ~(RFSF_ERROR|RFSF_RETRY);
    }
}



/*
 *  rfsSend - send a message to remote host
 *
 *  rcbp = control block for connection
 *  msgp = pointer to message to be sent
 *  len  = length of message to send
 *
 *  Send the supplied message with the specified length on the indicated
 *  connection. All signals are masked for the duration of the socket send
 *  operation so that the protocol will not be interrupted at an inconvenient
 *  point during the request/reply dialogue.
 *
 *  Return: 0 or an error number as appropriate.  
 */

rfsSend(rcbp, msgp, len)
register struct rfsConnectionBlock *rcbp;
caddr_t msgp;
int len;
{
    register struct proc *p = u.u_procp;
    struct uio uio;
    struct iovec iov;
    int signalmask;
    int error;

    rfs_printf3(RFSD_P_TRACEIN, "<Send t=%d[%d]\n", *((u_char *)msgp), len);
    iov.iov_base = msgp;
    iov.iov_len = len;
    uio.uio_iov = &iov;
    uio.uio_iovcnt = 1;
    uio.uio_resid = len;
    uio.uio_segflg = 1;

    signalmask = p->p_sigmask;
    p->p_sigmask = -1;
    error = sosend(rcbp->rcb_so, (struct mbuf *)0, &uio, 0, (struct mbuf *)0);
    p->p_sigmask = signalmask;
    if (error)
	rcbp->rcb_flags |= RFSF_RETRY;
    rfs_printf2(RFSD_P_TRACEOUT, ">Send %d\n", error);
    return(error);
}



/*
 *  rfsSendName - send a pathname on a connection
 *
 *  rcbp = remote control block
 *  len  = length of name
 *
 *  u.u_dirp = pathname to send (in system space)
 *
 *  Send the specified number of bytes of the supplied pathname on the
 *  indicated connection.  If the pathname length is zero, no bytes
 *  need be sent.  All signals are masked for the duration of the socket
 *  send operation so that the protocol will not be interrupted at an
 *  inconvenient point during the request/reply dialogue.
 *
 *  Return: 0 or an error number as appropriate.  
 */

rfsSendName(rcbp, len)
    register struct rfsConnectionBlock *rcbp;
{
    register struct proc *p = u.u_procp;
    struct uio uio;
    struct iovec iov;
    int error = 0;

    rfs_printf3(RFSD_P_TRACEIN, "<SndN \"%s\"[%d]\n", u.u_nd.ni_dirp, len);
    if (len != 0)
    {
	int signalmask;

	uio.uio_segflg = 1;
	iov.iov_base = u.u_nd.ni_dirp;
	uio.uio_resid = iov.iov_len = len;
	uio.uio_iov = &iov;
	uio.uio_iovcnt = 1;

	signalmask = p->p_sigmask;
	p->p_sigmask = -1;
	error = sosend(rcbp->rcb_so, (struct mbuf *)0, &uio, 0, (struct mbuf *)0);
	p->p_sigmask = signalmask;
	if (error)
	    rcbp->rcb_flags |= RFSF_RETRY;
    }
    rfs_printf2(RFSD_P_TRACEOUT, ">SndN %d\n", error);
    return (error);
}



/*
 *  rfsReceive - receive a message from remote connection
 *
 *  rcbp = remote control block
 *  repp = pointer to structure for reply
 *  len  = len of reply buffer
 *  type = reply type expected
 *
 *  Wait for a reply to a previously sent message on the remote connection.
 *  The reply must be of the indicated type and the expected number of bytes
 *  long.  All signals are masked for the duration of the socket receive
 *  operation so that the protocol will not be interrupted at an inconvenient
 *  point during the request/reply dialogue.
 *
 *  Return: 0 or an error number as appropriate.
 *
 *  TODO:  fix for interruptability.
 */

rfsReceive(rcbp, repp, len, type)
register struct rfsConnectionBlock *rcbp;
caddr_t repp;
u_int len;
int type;
{
    register struct proc *p = u.u_procp;
    struct uio uio;
    struct iovec iov;
    int error = 0;

    rfs_printf3(RFSD_P_TRACEIN, "<Recv t=%d[%d]\n", type, len);
    iov.iov_base = repp;
    iov.iov_len = len;
    uio.uio_iov = &iov;
    uio.uio_iovcnt = 1;
    uio.uio_resid = len;
    uio.uio_segflg = 1;
    while (uio.uio_resid)
    {
	unsigned oldcount;
	int signalmask;

	oldcount = uio.uio_resid;
	signalmask = p->p_sigmask;
	p->p_sigmask = -1;
	error = soreceive(rcbp->rcb_so, (struct mbuf **)0, &uio, 0, (struct mbuf **)0);
	p->p_sigmask = signalmask;
	if (error)
	    break;
	if (uio.uio_resid == oldcount)
	{
	    rfs_printf1(RFSD_PROTOCOL, "*Recv EOF\n");
	    error = EIO;
	    goto out;
	}
    }
    if (error)
    {
	rcbp->rcb_flags |= RFSF_RETRY;
	goto out;
    }
    if (*((u_short *)repp) != type)
    {
	rfs_printf2(RFSD_PROTOCOL, "*Recv t=%d\n", *((u_short *)repp));
	error = EIO;
    }
out:
    rfs_printf2(RFSD_P_TRACEOUT, ">Recv %d\n", error);
    return(error);
}



/*
 *  rfsShutdown - prevent any further sends on remote connection
 *
 *  rcbp = remote connection block
 *
 */

void
rfsShutdown(rcbp)
register struct rfsConnectionBlock *rcbp;
{
    (void) soshutdown(rcbp->rcb_so, 1);
}
#endif	CS_RFS
