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
 *	@(#)conf.c	6.3 (Berkeley) 8/2/85
 */

#include "../machine/pte.h"

#include "../h/param.h"
#include "../h/inode.h"
#include "../h/fs.h"

#include "../vaxmba/mbareg.h"

#include "saio.h"

devread(io)
	register struct iob *io;
{
	int cc;

	io->i_flgs |= F_RDDATA;
	io->i_error = 0;
	cc = (*devsw[io->i_ino.i_dev].dv_strategy)(io, READ);
	io->i_flgs &= ~F_TYPEMASK;
	return (cc);
}

devwrite(io)
	register struct iob *io;
{
	int cc;

	io->i_flgs |= F_WRDATA;
	io->i_error = 0;
	cc = (*devsw[io->i_ino.i_dev].dv_strategy)(io, WRITE);
	io->i_flgs &= ~F_TYPEMASK;
	return (cc);
}

devopen(io)
	register struct iob *io;
{

	(*devsw[io->i_ino.i_dev].dv_open)(io);
}

devclose(io)
	register struct iob *io;
{

	(*devsw[io->i_ino.i_dev].dv_close)(io);
}

devioctl(io, cmd, arg)
	register struct iob *io;
	int cmd;
	caddr_t arg;
{

	return ((*devsw[io->i_ino.i_dev].dv_ioctl)(io, cmd, arg));
}

/*ARGSUSED*/
nullsys(io)
	struct iob *io;
{

	;
}

/*ARGSUSED*/
nullioctl(io, cmd, arg)
	struct iob *io;
	int cmd;
	caddr_t arg;
{

	return (ECMD);
}

int	nullsys(), nullioctl();
#if defined(VAX780) || defined(VAX750) || defined(VAX8600)
int	hpstrategy(), hpopen(), hpioctl();
#endif
#if defined(VAX780) || defined(VAX750) || defined(VAX8600)
int	upstrategy(), upopen(), upioctl();
#endif
int	rkstrategy(), rkopen(), rkioctl();
int	rastrategy(), raopen(), raioctl();
#if defined(VAX730)
int	idcstrategy(), idcopen(), idcioctl();
#endif
int	rlstrategy(), rlopen(), rlioctl();
#ifndef BOOT
int	tmstrategy(), tmopen(), tmclose();
int	tsstrategy(), tsopen(), tsclose();
#if defined(VAX780) || defined(VAX750) || defined(VAX8600)
int	htstrategy(), htopen(), htclose();
int	mtstrategy(), mtopen(), mtclose();
#endif
int	utstrategy(), utopen(), utclose();
#endif

struct devsw devsw[] = {
#if defined(VAX780) || defined(VAX750) || defined(VAX8600)
	{ "hp",	hpstrategy,	hpopen,		nullsys,	hpioctl },
#endif
#if defined(VAX780) || defined(VAX750) || defined(VAX8600)
	{ "up",	upstrategy,	upopen,		nullsys,	upioctl },
#endif
	{ "hk",	rkstrategy,	rkopen,		nullsys,	rkioctl },
	{ "ra",	rastrategy,	raopen,		nullsys,	raioctl },
#if defined(VAX730)
	{ "rb",	idcstrategy,	idcopen,	nullsys,	idcioctl },
#endif
	{ "rl",	rlstrategy,	rlopen,		nullsys,	rlioctl },
#ifndef BOOT
	{ "ts",	tsstrategy,	tsopen,		tsclose,	nullioctl },
#if defined(VAX780) || defined(VAX750) || defined(VAX8600)
	{ "ht",	htstrategy,	htopen,		htclose,	nullioctl },
	{ "mt",	mtstrategy,	mtopen,		mtclose,	nullioctl },
#endif
	{ "tm",	tmstrategy,	tmopen,		tmclose,	nullioctl },
	{ "ut",	utstrategy,	utopen,		utclose,	nullioctl },
#endif
	{ 0, 0, 0, 0, 0 },
};
