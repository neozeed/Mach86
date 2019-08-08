/*
 * Copyright (c) 1982, 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)rl.c	7.1 (Berkeley) 6/5/86
 */

/*
 * Standalone RL02 disk driver
 */
#include "../machine/pte.h"

#include "../h/param.h"
#include "../h/inode.h"
#include "../h/fs.h"

#include "../vaxuba/rlreg.h"
#include "../vaxuba/ubareg.h"

#include "saio.h"
#include "savax.h"

u_short	rlstd[] = { 0774400 };
short	rl_off[] = { 0, 361, 0, -1, -1, -1, -1, -1 };

/* struct to keep state info about the controller */
struct	rl_stat {
	short	rl_dn;		/* drive number */
	short	rl_cylnhd;	/* cylinder and head */
	u_short	rl_bleft;	/* bytes left to transfer */
	u_short	rl_bpart;	/* bytes transferred */
} rl_stat[] = { -1, 0, 0, 0};

rlopen(io)
	register struct iob *io;
{
	register struct rldevice *rladdr =
		(struct rldevice *)ubamem(io->i_unit, rlstd[0]);
	register struct rl_stat *st = &rl_stat[0];
	register int ctr = 0;

	if (rl_off[io->i_boff] == -1 ||
	    io->i_boff < 0 || io->i_boff > 7)
		_stop("rl bad unit");

	/*
	 * DEC reports that:
	 * For some unknown reason the RL02 (seems to be only drive 1)
	 * does not return a valid drive status the first time that a
	 * GET STATUS request is issued for the drive, in fact it can
	 * take up to three or more GET STATUS requests to obtain the
	 * correct status.
	 * In order to overcome this, the driver has been modified to
	 * issue a GET STATUS request and validate the drive status
	 * returned.  If a valid status is not returned after eight
	 * attempts, then an error message is printed.
	 */
	do {
		rladdr->rlda.getstat = RL_RESET;
		rladdr->rlcs = (io->i_unit <<8) | RL_GETSTAT; /* Get status*/
		rlwait(rladdr);
	} while( (rladdr->rlmp.getstat&RLMP_STATUS) != RLMP_STATOK && ++ctr<8 );

	if ((rladdr->rlcs & RL_DE) || (ctr >= 8))
		_stop("rl unit does not respond");

	if ((rladdr->rlmp.getstat & RLMP_DT) == 0 )	/* NO RL01'S */
		_stop("rl01 unit not supported");

	/* Determine disk posistion */
	rladdr->rlcs = (io->i_unit << 8) | RL_RHDR;
	rlwait(rladdr);

	/* save disk drive posistion */
	st->rl_cylnhd = (rladdr->rlmp.readhdr & 0177700) >> 6;
	st->rl_dn = io->i_unit;

	/* byte offset for cylinder desired */
	io->i_boff = rl_off[io->i_boff] * NRLBPSC * NRLTRKS * NRLSECT;
}

rlstrategy(io, func)
	register struct iob *io;
{
	register struct rldevice *rladdr =
		(struct rldevice *)ubamem(io->i_unit, rlstd[0]);
	register struct rl_stat *st = &rl_stat[0];
	int com;
	daddr_t bn;
	short cn, sn, head;
	int diff, ubinfo, ubaddr, errcnt = 0;

retry:
	ubinfo = ubasetup(io, 1);
	bn = io->i_bn;		/* block number */
	cn = bn / 40;		/* 40 512 byte blocks per cylinder */
	sn = (bn % 20) << 1;
	head = (bn / 20) & 1;
	st->rl_bleft = io->i_cc;	/* total number of bytes to trans */
	ubaddr = ubinfo;

stupid_rl:
	/* find out how many cylinders to seek */
	diff = (st->rl_cylnhd >> 1) - cn;
	if ( diff == 0 && (st->rl_cylnhd & 1) == head )
		goto noseek;

	/* first time or we switched drives */
	st->rl_dn = io->i_unit;	/* drive number */

	if ( diff < 0 )
		rladdr->rlda.seek = -diff<<7 | RLDA_HGH | head << 4;
	else
		rladdr->rlda.seek = diff<<7 | RLDA_LOW | head << 4;
	rladdr->rlcs = (st->rl_dn << 8) | RL_SEEK;

	/* reset position of drive */
	st->rl_cylnhd = (cn << 1) | head;

noseek:
	/* wait for controller and drive */
	while( (rladdr->rlcs & RL_DCRDY) != RL_DCRDY)
		continue;

	/* calculate the max number of bytes we can trans */
	st->rl_bpart = NRLSECT * NRLBPSC - (sn * NRLBPSC);
	if ( st->rl_bleft < st->rl_bpart )
		st->rl_bpart = st->rl_bleft;

	rladdr->rlda.rw = (st->rl_cylnhd << 6) | sn;
	rladdr->rlmp.rw = -(st->rl_bpart >> 1);
	rladdr->rlba = ubaddr;

	com = (st->rl_dn << 8) | ((ubaddr>>12)&RL_BAE);

	if (func == READ)
		com |= RL_READ;
	else
		com |= RL_WRITE;
	rladdr->rlcs = com;

	/* wait for controller and drive */
	while( (rladdr->rlcs & RL_DCRDY) != RL_DCRDY)
		continue;

	if (rladdr->rlcs & RL_ERR) {
		int status;

		if ( rladdr->rlcs & RL_DE ) {
			rladdr->rlda.getstat = RL_GSTAT;
			rladdr->rlcs = (st->rl_dn << 8) | RL_GETSTAT;
			rlwait(rladdr);
			status = rladdr->rlmp.getstat;
			rladdr->rlda.getstat = RL_RESET;
			rladdr->rlcs = (st->rl_dn <<8) | RL_GETSTAT;
			rlwait(rladdr);
		}
		printf("rl error: (cyl,head,sec)=(%d,%d,%d) cs=%b mp=%b\n",
		    cn, head, sn, rladdr->rlcs & 0xffff, RLCS_BITS,
		    status, RLER_BITS);

		/* Determine disk posistion */
		rladdr->rlcs = (st->rl_dn << 8) | RL_RHDR;
		rlwait(rladdr);

		/* save disk drive posistion */
		st->rl_cylnhd = (rladdr->rlmp.readhdr & 0177700) >> 6;

		if (errcnt == 10) {
			printf("rl: unrecovered error\n");
			return (-1);
		}
		errcnt++;
		goto retry;
	}

	/* do we have to finish off the rest of the transfer? */
	if ( (st->rl_bleft -= st->rl_bpart) > 0 ) {
		/* increment head and/or cylinder */
		if ( ++head > 1 ) {
			cn++;		/* want next cyl, head 0 sector 0 */
			head = 0;
		}

		/* we always want sector to be zero */
		sn = 0;

		/*
		 * standalone code for ubafree does what regular
		 *   ubapurge does and we want to purge last transfer
		 */
		ubafree(io, ubinfo);

		ubaddr = ubinfo + io->i_cc - st->rl_bleft;

		goto stupid_rl;
	}

	ubafree(io, ubinfo);

	if (errcnt)
		printf("rl: recovered by retry\n");
	return (io->i_cc);
}

rlwait(rladdr)
	register struct rldevice *rladdr;
{

	while ((rladdr->rlcs & RL_CRDY) == 0)
		continue;
}

rlioctl(io, cmd, arg)
	struct iob *io;
	int cmd;
	caddr_t arg;
{

	return (ECMD);
}
