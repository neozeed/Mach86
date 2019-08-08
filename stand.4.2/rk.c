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
/*	rk.c	6.1	83/07/29	*/

/*
 * RK611/RK07
 */
#include "../machine/pte.h"

#include "../h/param.h"
#include "../h/inode.h"
#include "../h/fs.h"

#include "../vaxuba/ubareg.h"
#include "../vaxuba/rkreg.h"

#include "saio.h"
#include "savax.h"

u_short	rkstd[] = { 0777440 };
short	rk_off[] = { 0, 241, 0, -1, -1, -1, 393, -1 };

rkopen(io)
	register struct iob *io;
{
	register struct rkdevice *rkaddr = (struct rkdevice *)ubamem(io->i_unit, rkstd[0]);

	if (rk_off[io->i_boff] == -1 ||
	    io->i_boff < 0 || io->i_boff > 7)
		_stop("rk bad unit");
	io->i_boff = rk_off[io->i_boff] * NRKSECT*NRKTRK;
	rkaddr->rkcs2 = RKCS2_SCLR;
	rkwait(rkaddr);
}

rkstrategy(io, func)
	register struct iob *io;
{
	register struct rkdevice *rkaddr = (struct rkdevice *)ubamem(io->i_unit, rkstd[0]);
	int com;
	daddr_t bn;
	short dn, cn, sn, tn;
	int ubinfo, errcnt = 0;

retry:
	ubinfo = ubasetup(io, 1);
	bn = io->i_bn;
	dn = io->i_unit;
	cn = bn/(NRKSECT*NRKTRK);
	sn = bn%NRKSECT;
	tn = (bn / NRKSECT) % NRKTRK;
	rkaddr->rkcs2 = dn;
	rkaddr->rkcs1 = RK_CDT|RK_PACK|RK_GO;
	rkwait(rkaddr);
	rkaddr->rkcs1 = RK_CDT|RK_DCLR|RK_GO;
	rkwait(rkaddr);
	rkaddr->rkda = sn | (tn << 8);
	rkaddr->rkcyl = cn;
	rkaddr->rkba = ubinfo;
	rkaddr->rkwc = -(io->i_cc >> 1);
	com = RK_CDT|((ubinfo>>8)&0x300)|RK_GO;
	if (func == READ)
		com |= RK_READ;
	else
		com |= RK_WRITE;
	rkaddr->rkcs1 = com;
	rkwait(rkaddr);
	while ((rkaddr->rkds & RKDS_SVAL) == 0)
		;
	ubafree(io, ubinfo);
	if (rkaddr->rkcs1 & RK_CERR) {
		printf("rk error: (cyl,trk,sec)=(%d,%d,%d) cs2=%b er=%b\n",
		    cn, tn, sn, rkaddr->rkcs2, RKCS2_BITS,
		    rkaddr->rker, RKER_BITS);
		rkaddr->rkcs1 = RK_CDT|RK_DCLR|RK_GO;
		rkwait(rkaddr);
		if (errcnt == 10) {
			printf("rk: unrecovered error\n");
			return (-1);
		}
		errcnt++;
		goto retry;
	}
	if (errcnt)
		printf("rk: recovered by retry\n");
	return (io->i_cc);
}

rkwait(rkaddr)
	register struct rkdevice *rkaddr;
{

	while ((rkaddr->rkcs1 & RK_CRDY) == 0)
		;
}

/*ARGSUSED*/
rkioctl(io, cmd, arg)
	struct iob *io;
	int cmd;
	caddr_t arg;
{

	return (ECMD);
}
