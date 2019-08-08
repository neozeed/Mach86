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
#ifndef	romp
/* ************************************************************************ *\
 *									    *
 * Package:	mp_queues						    *
 *									    *
 *									    *
 * File:	mp_queuedb.c						    *
 *									    *
 * Abstract:								    *
 *									    *
 * Author:	Robert V. Baron						    *
 *		Copyright (c) 1985 by Robert V. Baron			    *
 *									    *
 * History:
 * 11-Oct-85  Robert V Baron (rvb) at Carnegie-Mellon University
 *	header changes
 *
 * 19-Mar-85  Robert V Baron (rvb) at Carnegie-Mellon University
 *	Created
 *									    *
\* ************************************************************************ */

/*CMU:	%M%	%I%	%G%	*/


#include "wb_ml.h"
#include "mach_mp.h"

#include "../h/map.h"
#include "../h/param.h"
#include "../h/vmmac.h"
#ifdef	vax
#include "../vax/mtpr.h"
#endif	vax
#include "../machine/pte.h"

#include "../sync/mp_queue.h"

queueprotect(str, kmsgptr, limit, ro_rw, reset)
char *str;
register mpqhead_t *kmsgptr;
{
register int len = 0;
register mpqhead_t *lastp;
register int *ptep;

	if (!kmsgptr) {
		printf("queueprotect: %s uninitialized\n", str) ;
		return ;
	}

	lastp = kmsgptr;
	kmsgptr = mpqnext(kmsgptr, mpqhead_t *);
	for ( ; lastp != kmsgptr ; kmsgptr = mpqnext(kmsgptr, mpqhead_t *)) {
		ptep = (int *)&Sysmap[btop((int) kmsgptr - 0x80000000)];
		*ptep &= ~PG_PROT;
		if (ro_rw)
			*ptep |= PG_URKW;
		else
			*ptep |= PG_URKR;
		mtpr(TBIS, kmsgptr);
		len++ ;
		if (limit && len > limit) {
			printf("queueprotect: %s @%x MUNGED\n", str, lastp);
			printf("next = %x prev = %x\n",
				lastp->mpq_next, lastp->mpq_prev);
			printf("nextnext = %x nextprev = %x\n",
				mpqnext(lastp, mpqhead_t *)->mpq_next,
				mpqnext(lastp, mpqhead_t *)->mpq_prev);
			if (reset)
				initmpqueue(lastp);
			return 0;
		}
	}
	return 1;

}
#endif	romp
