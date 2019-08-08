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
/* $Header: in_cksum.c,v 4.0 85/07/15 00:42:40 ibmacis GAMMA $ */
/* $Source: /ibm/acis/usr/sys_ca/ca/RCS/in_cksum.c,v $ */

#ifndef lint
static char *rcsid = "$Header: in_cksum.c,v 4.0 85/07/15 00:42:40 ibmacis GAMMA $";
#endif

/*	in_cksum.c	6.1	83/07/29	*/

#include "../h/types.h"
#include "../h/mbuf.h"
#include "../netinet/in.h"
#include "../netinet/in_systm.h"

/*
 * Checksum routine for Internet Protocol family headers (NON-VAX Version).
 *
 * This routine is very heavily used in the network
 * code and should be modified for each CPU to be as fast as possible.
 * This particular version is a quick hack which needs to be rewritten.
 */

in_cksum(m, len)
	register struct mbuf *m;
	register int len;
{
	register u_short * w;		  /* on vax, known to be r9 */
	register int sum = 0;		  /* on vax, known to be r8 */
	register int mlen = 0;

	for (;;) {
		/*
		 * Each trip around loop adds in
		 * word from one mbuf segment.
		 */
		w = mtod(m, u_short *);
		if (mlen == -1) {
			sum += *(u_char *)w;
			if (sum >= 0xFFFF)
				sum = (sum + 1) & 0xFFFF;
			w = (u_short *)((char *)w + 1);
			mlen = m->m_len - 1;
			len--;
		} else
			mlen = m->m_len;
		m = m->m_next;
		if (len < mlen)
			mlen = len;
		len -= mlen;
		if ((int)w & 01)
			while ((mlen -= 2) >= 0) {
				sum += (((*(char *)w) & 0xFF) << 8) +
				    ((*(((char *)w) + 1)) & 0xFF);
				if (sum >= 0xFFFF)
					sum = (sum + 1) & 0xFFFF;
				++w;
	} else while ((mlen -= 2) >= 0) {
			sum += *w++;
			if (sum >= 0xFFFF)
				sum = (sum + 1) & 0xFFFF;
	}
		if (mlen == -1) {
			sum += *(u_char *)w << 8;
			if (sum >= 0xFFFF)
				sum = (sum + 1) & 0xFFFF;
		}
		if (len == 0)
			break;
		/*
		 * Locate the next block with some data.
		 * If there is a word split across a boundary we
		 * will wrap to the top with mlen == -1 and
		 * then add it in shifted appropriately.
		 */
		for (;;) {
			if (m == 0) {
				printf("cksum: out of data\n");
				goto done;
			}
			if (m->m_len)
				break;
			m = m->m_next;
		}
	}
done:
	if (sum)
		sum ^= 0xFFFF;
	return sum;
}
