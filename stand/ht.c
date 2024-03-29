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
 * Copyright (c) 1982 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)ht.c	6.2 (Berkeley) 6/8/85
 */

/*
 * TM03/TU?? tape driver
 */
#include "../machine/pte.h"

#include "../h/param.h"
#include "../h/inode.h"
#include "../h/fs.h"

#include "../vaxmba/htreg.h"
#include "../vaxmba/mbareg.h"

#include "saio.h"
#include "savax.h"

short	httypes[] =
	{ MBDT_TM03, MBDT_TE16, MBDT_TU45, MBDT_TU77, 0 };

#define	MASKREG(reg)	((reg)&0xffff)

htopen(io)
	register struct iob *io;
{
	register int skip;
	register struct htdevice *htaddr =
	   (struct htdevice *)mbadrv(io->i_unit);
	int i;

	for (i = 0; httypes[i]; i++)
		if (httypes[i] == (htaddr->htdt&MBDT_TYPE))
			goto found;
	_stop("not a tape\n");
found:
	mbainit(UNITTOMBA(io->i_unit));
	htaddr->htcs1 = HT_DCLR|HT_GO;
	htstrategy(io, HT_REW);
	skip = io->i_boff;
	while (skip--) {
		io->i_cc = -1;
		while (htstrategy(io, HT_SFORW))
			;
		DELAY(65536);
		htstrategy(io, HT_SENSE);
	}
}

htclose(io)
	register struct iob *io;
{

	htstrategy(io, HT_REW);
}

htstrategy(io, func)
	register struct iob *io;
	int func;
{
	register int den, errcnt, ds;
	int er;
	short fc;
	register struct htdevice *htaddr =
	    (struct htdevice *)mbadrv(io->i_unit);

	errcnt = 0;
retry:
	den = HTTC_1600BPI|HTTC_PDP11;
	htquiet(htaddr);
	htaddr->htcs1 = HT_DCLR|HT_GO;
	htaddr->httc = den;
	htaddr->htfc = -io->i_cc;
	if (func == HT_SREV) {
		htaddr->htfc = -1;
		htaddr->htcs1 = HT_SREV|HT_GO;
		return (0);
	}
	if (func == READ || func == WRITE)
		mbastart(io, func);
	else
		htaddr->htcs1 = func|HT_GO;
	htquiet(htaddr);
	ds = htaddr->htds;
	er = htaddr->hter;
	if (ds & HTDS_TM) {
		htaddr->htcs1 = HT_DCLR|HT_GO;
		return (0);
	}
	if (ds & HTDS_ERR) {
		htaddr->htcs1 = HT_DCLR|HT_GO;
		if ((er & HTER_CORCRC) == 0) {
			printf("ht error: ds=%b, er=%b\n",
			    MASKREG(ds), HTDS_BITS,
			    MASKREG(er), HTER_BITS);
			if (errcnt == 10) {
				printf("ht: unrecovered error\n");
				return (-1);
			}
			errcnt++;
			htstrategy(io, HT_SREV);
			goto retry;
		}
	}
	if (errcnt)
		printf("ht: recovered by retry\n");
	fc = htaddr->htfc;
	return (io->i_cc+fc);
}

htquiet(htaddr)
	register struct htdevice *htaddr;
{
	register int s;

	do
		s = htaddr->htds;
	while ((s & HTDS_DRY) == 0);
}
