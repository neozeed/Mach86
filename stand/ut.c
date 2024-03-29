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
 *	@(#)ut.c	6.2 (Berkeley) 6/8/85
 */

/*
 * SI Model 9700 -- emulates TU45 on the UNIBUS
 */
#include "../machine/pte.h"

#include "../h/param.h"
#include "../h/inode.h"
#include "../h/fs.h"

#include "../vaxuba/ubareg.h"
#include "../vaxuba/utreg.h"

#include "saio.h"
#include "savax.h"

#define	MASKREG(reg)	((reg)&0xffff)

u_short	utstd[] = { 0172440 };		/* non-standard */

utopen(io)
	register struct iob *io;
{
	int skip;

	utstrategy(io, UT_REW);
	skip = io->i_boff;
	while (skip-- > 0)
		utstrategy(io, UT_SFORWF);
}

utclose(io)
	register struct iob *io;
{

	utstrategy(io, UT_REW);
}

#define utwait(addr)	{do word=addr->utcs1; while((word&UT_RDY)==0);}

utstrategy(io, func)
	register struct iob *io;
{
	register u_short word;
	register int errcnt;
	register struct utdevice *addr =
	    (struct utdevice *)ubamem(io->i_unit, utstd[0]);
	int info, resid;
	u_short dens;

	dens = (io->i_unit&07) | PDP11FMT | UT_PE;
	errcnt = 0;
retry:
	utquiet(addr);
	addr->uttc = dens;
	info = ubasetup(io, 1);
	addr->utwc = -((io->i_cc+1) >> 1);
	addr->utfc = -io->i_cc;
	if (func == READ) {
		addr->utba = info;
		addr->utcs1 = UT_RCOM | ((info>>8) & 0x30) | UT_GO;
	} else if (func == WRITE) {
		addr->utba = info;
		addr->utcs1 = UT_WCOM | ((info>>8) & 0x30) | UT_GO;
	} else if (func == UT_SREV) {
		addr->utcs1 = UT_SREV | UT_GO;
		return (0);
	} else
		addr->utcs1 = func | UT_GO;
	utwait(addr);
	ubafree(io, info);
	word = addr->utds;
	if (word&(UTDS_EOT|UTDS_TM)) {
		addr->utcs1 = UT_CLEAR | UT_GO;
		goto done;
	}
	if ((word&UTDS_ERR) || (addr->utcs1&UT_TRE)) {
		if (errcnt == 0)
			printf("tj error: cs1=%b er=%b cs2=%b ds=%b",
			  addr->utcs1, UT_BITS, addr->uter, UTER_BITS,
			  addr->utcs2, UTCS2_BITS, word, UTDS_BITS);
		if (errcnt == 10) {
			printf("\n");
			return (-1);
		}
		errcnt++;
		if (addr->utcs1&UT_TRE)
			addr->utcs2 |= UTCS2_CLR;
		addr->utcs1 = UT_CLEAR | UT_GO;
		utstrategy(io, UT_SREV);
		utquiet(addr);
		if (func == WRITE) {
			addr->utcs1 = UT_ERASE | UT_GO;
			utwait(addr);
		}
		goto retry;
	}
	if (errcnt)
		printf(" recovered by retry\n");
done:
	if (func == READ) {
		resid = 0;
		if (io->i_cc > MASKREG(addr->utfc))
			resid = io->i_cc - MASKREG(addr->utfc);
	} else
		resid = MASKREG(-addr->utfc);
	return (io->i_cc - resid);
}

utquiet(addr)
	register struct utdevice *addr;
{
	register u_short word;

	utwait(addr);
	do
		word = addr->utds;
	while ((word&UTDS_DRY) == 0 && (word&UTDS_PIP));
}
