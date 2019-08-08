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
 *	@(#)raw_hy.c	6.2	9/16/85
 *
 * 4.3 BSD Unix kernel - NSC HYPERchannel support
 *
 * $Header: raw_hy.c,v 3.1 84/02/15 04:27:44 steveg Exp $
 * $Locker:  $
 *
 * Copyright (c) 1984, Tektronix Inc.
 * All Rights Reserved
 *
 */


#include "param.h"
#include "mbuf.h"
#include "socket.h"
#include "protosw.h"
#include "socketvar.h"
#include "errno.h"

#include "../net/if.h"
#include "../net/route.h"
#include "../net/raw_cb.h"

#ifdef	BBNNET
#define	INET
#endif
#include "../netinet/in.h"
#include "../netinet/in_systm.h"
#include "../netinet/in_var.h"
#include "if_hy.h"

/*
 * Raw interface to HYPERchannel.
 */

/*
 * Generate HYPERchannel leader and pass packet to hyoutput.
 * The user must create a skeletal leader in order to
 * communicate message type, message subtype, etc.
 * We don't really check the header supplied by the user.
 */
rhy_output(m, so)
	register struct mbuf *m;
	struct socket *so;
{
	struct mbuf *n;
	int error = 0;
	register struct sockaddr_in *sin;
	register struct rawcb *rp = sotorawcb(so);
	struct in_ifaddr *ia;

	/*
	 * Verify user has supplied necessary space
	 * for the header.
	 */
	if ((m->m_off > MMAXOFF || m->m_len < sizeof(struct hym_hdr)) &&
	    (m = m_pullup(m, sizeof(struct hym_hdr))) == 0) {
		error = EMSGSIZE;	/* XXX */
		goto bad;
	}

	sin = (struct sockaddr_in *)&rp->rcb_faddr;
	/* no routing here */
	ia = in_iaonnetof(in_netof(sin->sin_addr));
	if (ia)
		return (hyoutput(ia->ia_ifp, m, (struct sockaddr *)sin));
	error = ENETUNREACH;
bad:
	m_freem(m);
	return (error);
}
