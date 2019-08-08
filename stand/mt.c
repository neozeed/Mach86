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
 *	@(#)mt.c	6.2 (Berkeley) 6/8/85
 */

/*
 * TM78/TU78 tape driver
 * Made to work reliably by by Jeffrey R. Schwab (Purdue)
 */
#include "../machine/pte.h"

#include "../h/param.h"
#include "../h/inode.h"
#include "../h/fs.h"

#include "../vaxmba/mtreg.h"
#include "../vaxmba/mbareg.h"

#include "saio.h"
#include "savax.h"

short	mttypes[] =
	{ MBDT_TU78, 0 };

#define	MASKREG(reg)	((reg)&0xffff)

mtopen(io)
	register struct iob *io;
{
	register int skip;
	register struct mtdevice *mtaddr =
		(struct mtdevice *)mbadrv(io->i_unit);
	int i;

	for (i = 0; mttypes[i]; i++)
		if (mttypes[i] == (mtaddr->mtdt&MBDT_TYPE))
			goto found;
	_stop("not a tape\n");
found:
	mbainit(UNITTOMBA(io->i_unit));
	mtaddr->mtid = MTID_CLR;
	DELAY(250);
	while ((mtaddr->mtid & MTID_RDY) == 0)
		;

	/* clear any attention bits present on open */
	i = mtaddr->mtner;
	mtaddr->mtas = mtaddr->mtas;

	mtstrategy(io, MT_REW);
	skip = io->i_boff;
	while (skip--) {
		io->i_cc = -1;
		mtstrategy(io, MT_SFORWF);
	}
}

mtclose(io)
	register struct iob *io;
{

	mtstrategy(io, MT_REW);
}

mtstrategy(io, func)
	register struct iob *io;
	int func;
{
	register int errcnt, s, ic;
	register struct mtdevice *mtaddr =
	    (struct mtdevice *)mbadrv(io->i_unit);
	struct mba_regs *mba = mbamba(io->i_unit);

	errcnt = 0;
retry:
	/* code to trap for attention up prior to start of command */
	if ((mtaddr->mtas & 0xffff) != 0) {
		printf("mt unexpected attention er=%x - continuing\n",
			MASKREG(mtaddr->mtner));
		mtaddr->mtas = mtaddr->mtas;
	}

	if (func == READ || func == WRITE) {
		mtaddr->mtca = 1<<2;	/* 1 record */
		mtaddr->mtbc = io->i_cc;
		mbastart(io, func);
		/* wait for mba to go idle and read result status */
		while((mba->mba_sr & MBSR_DTBUSY) != 0)
			;
		ic = mtaddr->mter & MTER_INTCODE;
	} else {
		mtaddr->mtncs[0] = (-io->i_cc << 8)|func|MT_GO;
	rwait:
		do
			s = mtaddr->mtas&0xffff;
		while (s == 0);
		ic = mtaddr->mtner & MTER_INTCODE;
		mtaddr->mtas = mtaddr->mtas;	/* clear attention */
	}
	switch (ic) {
	case MTER_TM:
	case MTER_EOT:
	case MTER_LEOT:
		return (0);

	case MTER_DONE:
		/* make sure a record was read */
		if ((mtaddr->mtca & (1 << 2)) != 0) {
			printf("mt record count not decremented - retrying\n");
			goto retry;
		}
		break;

	case MTER_RWDING:
		goto rwait;
	default:
		printf("mt hard error: er=%x\n",
		    MASKREG(mtaddr->mter));
		mtaddr->mtid = MTID_CLR;
		DELAY(250);
		while ((mtaddr->mtid & MTID_RDY) == 0)
			;
		return (-1);

	case MTER_RETRY:
		printf("mt error: er=%x\n", MASKREG(mtaddr->mter));
		if (errcnt == 10) {
			printf("mt: unrecovered error\n");
			return (-1);
		}
		errcnt++;
		goto retry;
	}
	if (errcnt)
		printf("mt: recovered by retry\n");
	return (io->i_cc);	/* NO PARTIAL RECORD READS!!! */
}
