/*
 * Copyright (c) 1982, 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)sys_socket.c	7.1 (Berkeley) 6/5/86
 */

#include "param.h"
#include "systm.h"
#include "dir.h"
#include "user.h"
#include "file.h"
#include "mbuf.h"
#include "protosw.h"
#include "socket.h"
#include "socketvar.h"
#include "ioctl.h"
#include "uio.h"
#include "stat.h"

#include "../net/if.h"
#include "../net/route.h"

int	soo_rw(), soo_ioctl(), soo_select(), soo_close();
struct	fileops socketops =
    { soo_rw, soo_ioctl, soo_select, soo_close };

soo_rw(fp, rw, uio)
	struct file *fp;
	enum uio_rw rw;
	struct uio *uio;
{
	int soreceive(), sosend();

	return (
	    (*(rw==UIO_READ?soreceive:sosend))
	      ((struct socket *)fp->f_data, 0, uio, 0, 0));
}

soo_ioctl(fp, cmd, data)
	struct file *fp;
	int cmd;
	register caddr_t data;
{
	register struct socket *so = (struct socket *)fp->f_data;

	switch (cmd) {

	case FIONBIO:
		if (*(int *)data)
			so->so_state |= SS_NBIO;
		else
			so->so_state &= ~SS_NBIO;
		return (0);

	case FIOASYNC:
		if (*(int *)data)
			so->so_state |= SS_ASYNC;
		else
			so->so_state &= ~SS_ASYNC;
		return (0);

	case FIONREAD:
		*(int *)data = so->so_rcv.sb_cc;
		return (0);

	case SIOCSPGRP:
		so->so_pgrp = *(int *)data;
		return (0);

	case SIOCGPGRP:
		*(int *)data = so->so_pgrp;
		return (0);

	case SIOCATMARK:
		*(int *)data = (so->so_state&SS_RCVATMARK) != 0;
		return (0);
	}
	/*
	 * Interface/routing/protocol specific ioctls:
	 * interface and routing ioctls should have a
	 * different entry since a socket's unnecessary
	 */
#define	cmdbyte(x)	(((x) >> 8) & 0xff)
	if (cmdbyte(cmd) == 'i')
		return (ifioctl(so, cmd, data));
	if (cmdbyte(cmd) == 'r')
		return (rtioctl(cmd, data));
	return ((*so->so_proto->pr_usrreq)(so, PRU_CONTROL, 
	    (struct mbuf *)cmd, (struct mbuf *)data, (struct mbuf *)0));
}

soo_select(fp, which)
	struct file *fp;
	int which;
{
	register struct socket *so = (struct socket *)fp->f_data;
	register int s = splnet();

	switch (which) {

	case FREAD:
		if (soreadable(so)) {
			splx(s);
			return (1);
		}
		sbselqueue(&so->so_rcv);
		break;

	case FWRITE:
		if (sowriteable(so)) {
			splx(s);
			return (1);
		}
		sbselqueue(&so->so_snd);
		break;

	case 0:
		if (so->so_oobmark ||
		    (so->so_state & SS_RCVATMARK)) {
			splx(s);
			return (1);
		}
		sbselqueue(&so->so_rcv);
		break;
	}
	splx(s);
	return (0);
}

/*ARGSUSED*/
soo_stat(so, ub)
	register struct socket *so;
	register struct stat *ub;
{

	bzero((caddr_t)ub, sizeof (*ub));
	return ((*so->so_proto->pr_usrreq)(so, PRU_SENSE,
	    (struct mbuf *)ub, (struct mbuf *)0, 
	    (struct mbuf *)0));
}

soo_close(fp)
	struct file *fp;
{
	int error = 0;
	
	if (fp->f_data)
		error = soclose((struct socket *)fp->f_data);
	fp->f_data = 0;
	return (error);
}
