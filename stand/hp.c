/*
 * Copyright (c) 1982, 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)hp.c	7.1 (Berkeley) 6/5/86
 */

/*
 * RP??/RM?? disk driver
 * with ECC handling and bad block forwarding.
 * Also supports header io operations and
 * commands to write check header and data.
 */
#include "../h/param.h"
#include "../h/inode.h"
#include "../h/fs.h"
#include "../h/dkbad.h"

#include "../vax/pte.h"
#include "../vaxmba/hpreg.h"
#include "../vaxmba/mbareg.h"

#include "saio.h"
#include "savax.h"

#define	RETRIES		27

#define	MASKREG(reg)	((reg)&0xffff)

#define	MAXBADDESC	126
#define	SECTSIZ 	512	/* sector size in bytes */
#define	HDRSIZ		4	/* number of bytes in sector header */

extern	struct st hpst[];
extern	short hptypes[];

#define	RP06 (hptypes[sc->type] == MBDT_RP06 || hptypes[sc->type] == MBDT_RP05 \
	|| hptypes[sc->type] == MBDT_RP04)
#define	ML11 (hptypes[sc->type] == MBDT_ML11A)
#define	RM80 (hptypes[sc->type] == MBDT_RM80)

u_char	hp_offset[16] = {
    HPOF_P400, HPOF_M400, HPOF_P400, HPOF_M400,
    HPOF_P800, HPOF_M800, HPOF_P800, HPOF_M800,
    HPOF_P1200, HPOF_M1200, HPOF_P1200, HPOF_M1200,
    0, 0, 0, 0,
};

struct	dkbad hpbad[MAXNMBA*8];

struct	hp_softc {
	char	type;
	char	gottype;
	char	ssect;		/* 1 when on track w/skip sector */
	char	debug;
#	define	HPF_BSEDEBUG	01	/* debugging bad sector forwarding */
#	define	HPF_ECCDEBUG	02	/* debugging ecc correction */
	int	ecclim;
	int	retries;
} hp_softc[MAXNMBA * 8];

int	sectsiz;

/*
 * When awaiting command completion, don't
 * hang on to the status register since
 * this ties up some controllers.
 */
#define	HPWAIT(addr) \
	while ((((addr)->hpds)&HPDS_DRY)==0) DELAY(500);

hpopen(io)
	register struct iob *io;
{
	register unit = io->i_unit;
	struct hpdevice *hpaddr = (struct hpdevice *)mbadrv(unit);
	register struct st *st;
	register struct hp_softc *sc = &hp_softc[unit];

	mbainit(UNITTOMBA(unit));
	if (sc->gottype == 0) {
		register i, type = hpaddr->hpdt & MBDT_TYPE;
		struct iob tio;

		for (i = 0; hptypes[i]; i++)
			if (hptypes[i] == type)
				goto found;
		_stop("unknown drive type");
found:
		sc->retries = RETRIES;
		sc->ecclim = 11;
		sc->debug = 0;
		hpaddr->hpcs1 = HP_DCLR|HP_GO;		/* init drive */
		hpaddr->hpcs1 = HP_PRESET|HP_GO;
		if (!ML11)
			hpaddr->hpof = HPOF_FMT22;
		sc->type = hpmaptype(hpaddr, i, UNITTODRIVE(unit));
		/*
		 * Read in the bad sector table.
		 */
		st = &hpst[sc->type];
		tio = *io;
		tio.i_bn = st->nspc * st->ncyl - st->nsect;
		tio.i_ma = (char *)&hpbad[unit];
		tio.i_cc = sizeof (struct dkbad);
		tio.i_flgs |= F_RDDATA;
		for (i = 0; i < 5; i++) {
			if (hpstrategy(&tio, READ) == sizeof (struct dkbad))
				break;
			tio.i_bn += 2;
		}
		if (i == 5) {
			printf("Unable to read bad sector table\n");
			for (i = 0; i < MAXBADDESC; i++) {
				hpbad[unit].bt_bad[i].bt_cyl = -1;
				hpbad[unit].bt_bad[i].bt_trksec = -1;
			}
		}
		sc->gottype = 1;
	}
	st = &hpst[sc->type];
	if (io->i_boff < 0 || io->i_boff > 7 ||
	    st->off[io->i_boff]== -1)
		_stop("hp bad minor");
	io->i_boff = st->off[io->i_boff] * st->nspc;
}

hpstrategy(io, func)
	register struct iob *io;
{
	register unit = io->i_unit;
	struct mba_regs *mba = mbamba(unit);
	daddr_t bn, startblock;
	struct hpdevice *hpaddr = (struct hpdevice *)mbadrv(unit);
	register struct hp_softc *sc = &hp_softc[unit];
	struct st *st = &hpst[sc->type];
	int cn, tn, sn, bytecnt, bytesleft, rv; 
	char *membase;
	int er1, er2, hprecal;

	sectsiz = SECTSIZ;
	if ((io->i_flgs & (F_HDR|F_HCHECK)) != 0)
		sectsiz += HDRSIZ;
	if ((hpaddr->hpds & HPDS_VV) == 0) {
		hpaddr->hpcs1 = HP_DCLR|HP_GO;
		hpaddr->hpcs1 = HP_PRESET|HP_GO;
		if (!ML11)
			hpaddr->hpof = HPOF_FMT22;
	}
	io->i_errcnt = 0;
	sc->ssect = 0;
	rv = bytecnt = io->i_cc;
	membase = io->i_ma;
	startblock = io->i_bn;
	hprecal = 0;

restart:
	bn = io->i_bn;
	cn = bn/st->nspc;
	sn = bn%st->nspc;
	tn = sn/st->nsect;
	sn = sn%st->nsect + sc->ssect;

	HPWAIT(hpaddr);
	mba->mba_sr = -1;
	if (ML11)
		hpaddr->hpda = bn;
	else {
		hpaddr->hpdc = cn;
		hpaddr->hpda = (tn << 8) + sn;
	}
	if (mbastart(io, func) != 0) {		/* start transfer */
		rv = -1;
		goto done;
	}
	HPWAIT(hpaddr);
	/*
	 * Successful data transfer, return.
	 */
	if ((hpaddr->hpds&HPDS_ERR) == 0 && (mba->mba_sr&MBSR_EBITS) == 0)
		goto done;

	/*
	 * Error handling.  Calculate location of error.
	 */
	bytesleft = MASKREG(mba->mba_bcr);
	if (bytesleft) 
		bytesleft |= 0xffff0000;	/* sxt */
	bn = io->i_bn + (io->i_cc + bytesleft) / sectsiz;
	er1 = MASKREG(hpaddr->hper1);
	er2 = MASKREG(hpaddr->hper2);
	if (er1 & (HPER1_DCK|HPER1_ECH))
		bn--;	/* Error is in Prev block */
	cn = bn/st->nspc;
	sn = bn%st->nspc;
	tn = sn/st->nsect;
	sn = sn%st->nsect;
	if (sc->debug & (HPF_ECCDEBUG|HPF_BSEDEBUG)) {
		printf("hp error: sn%d (cyl,trk,sec)=(%d,%d,%d) ds=%b\n",
			bn, cn, tn, sn, MASKREG(hpaddr->hpds), HPDS_BITS);
		printf("er1=%b er2=%b\n", er1, HPER1_BITS, er2, HPER2_BITS);
		printf("bytes left: %d, of 0x%x, da 0x%x\n",-bytesleft,
			hpaddr->hpof, hpaddr->hpda);
	}
	if (er1 & HPER1_HCRC) {
		er1 &= ~(HPER1_HCE|HPER1_FER);
		er2 &= ~HPER2_BSE;
		if ((io->i_flgs&F_NBSF) == 0 && hpecc(io, BSE) == 0)
			goto success;
	}
	/*
	 * Give up early if drive write locked.
	 */
	if (er1&HPER1_WLE) {
		printf("hp%d: write locked\n", unit);
		rv = -1;
		goto done;
	}
	/*
	 * Skip sector handling.
	 */
	if (RM80 && (er2 & HPER2_SSE)) {
		(void) hpecc(io, SSE);
		sc->ssect = 1;
		goto restart;
	}
	/*
	 * Attempt to forward bad sectors on anything but an ML11.
	 * Interpret format error bit as a bad block on RP06's.
	 */
	if (((er2 & HPER2_BSE) && !ML11) ||
	    (MASKREG(er1) == HPER1_FER && RP06)) {
		if (io->i_flgs & F_NBSF) {
			io->i_error = EBSE;	
			goto hard;
		}
		if (hpecc(io, BSE) == 0)
			goto success;
		io->i_error = EBSE;
		goto hard;
	}
	/*
	 * ECC correction?
	 */
	if ((er1 & (HPER1_DCK|HPER1_ECH)) == HPER1_DCK) {
		if (hpecc(io, ECC) == 0)
			goto success;
		io->i_error = EECC;
		goto hard;
	} 

	/*
	 * If a hard error, or maximum retry count
	 * exceeded, clear controller state and
	 * pass back error to caller.
	 */
	if (++io->i_errcnt > sc->retries || (er1 & HPER1_HARD) ||
	    (!ML11 && (er2 & HPER2_HARD)) || (ML11 && (io->i_errcnt >= 16))) {
		io->i_error = EHER;
		if (mba->mba_sr & (MBSR_WCKUP|MBSR_WCKLWR))
			io->i_error = EWCK;
hard:
		io->i_errblk = bn + sc->ssect;
		if (sc->debug & (HPF_BSEDEBUG|HPF_ECCDEBUG))
		    printf(" dc=%d, da=0x%x",MASKREG(hpaddr->hpdc),
			  MASKREG(hpaddr->hpda));
		else {
		    printf("hp error: sn%d (cyl,trk,sec)=(%d,%d,%d) ds=%b \n",
			   bn, cn, tn, sn, MASKREG(hpaddr->hpds), HPDS_BITS);
		    printf("er1=%b er2=%b", er1, HPER1_BITS, er2, HPER2_BITS);
		}
		hpaddr->hpcs1 = HP_DCLR|HP_GO;
		printf("\n");
		rv = -1;
		goto done;

	}
	/* fall thru to retry */
	hpaddr->hpcs1 = HP_DCLR|HP_GO;
	HPWAIT(hpaddr);

	/* 
	 * Every fourth retry recalibrate.
	 */
	if (((io->i_errcnt & 07) == 4) ) {
		hpaddr->hpcs1 = HP_RECAL|HP_GO;
		HPWAIT(hpaddr);
		hpaddr->hpdc = cn;
		hpaddr->hpcs1 = HP_SEEK|HP_GO;
		HPWAIT(hpaddr);
	}

	if (io->i_errcnt >= 16 && (io->i_flgs & F_READ)) {
		hpaddr->hpof = hp_offset[io->i_errcnt & 017]|HPOF_FMT22;
		hpaddr->hpcs1 = HP_OFFSET|HP_GO;
		HPWAIT(hpaddr);
	}
	if (sc->debug & (HPF_ECCDEBUG|HPF_BSEDEBUG))
		printf("restart: bn=%d, cc=%d, ma=0x%x hprecal=%d\n",
		  io->i_bn, io->i_cc, io->i_ma, hprecal);
	goto restart;

success:
	/*
	 * On successful error recovery, bump
	 * block number to advance to next portion
	 * of i/o transfer.
	 */
	bn++;
	if ((bn-startblock) * sectsiz < bytecnt) {
		io->i_bn = bn;
		io->i_ma = membase + (io->i_bn - startblock)*sectsiz;
		io->i_cc = bytecnt - (io->i_bn - startblock)*sectsiz;
		if (sc->debug & (HPF_ECCDEBUG|HPF_BSEDEBUG))
			printf("restart: bn=%d, cc=%d, ma=0x%x hprecal=%d\n",
			  io->i_bn, io->i_cc, io->i_ma, hprecal);
		goto restart;
	}
done:
	if (io->i_errcnt >= 16) {
		hpaddr->hpcs1 = HP_RTC|HP_GO;
		while (hpaddr->hpds & HPDS_PIP)
			;
	}
	io->i_bn = startblock;		/*reset i_bn to original */
	io->i_cc = bytecnt;		/*reset i_cc to total count xfered*/
	io->i_ma = membase;		/*reset i_ma to original */
	return (rv);
}

hpecc(io, flag)
	register struct iob *io;
	int flag;
{
	register unit = io->i_unit;
	register struct mba_regs *mbp = mbamba(unit);
	register struct hpdevice *rp = (struct hpdevice *)mbadrv(unit);
	register struct hp_softc *sc = &hp_softc[unit];
	register struct st *st = &hpst[sc->type];
	int npf, bn, cn, tn, sn, bcr;

	bcr = MASKREG(mbp->mba_bcr);
	if (bcr)
		bcr |= 0xffff0000;		/* sxt */
	npf = (bcr + io->i_cc) / sectsiz;	/* # sectors read */
	if (flag == ECC)
		npf--;		/* Error is in prev block --ghg */
	bn = io->i_bn + npf + sc->ssect;	/* physical block #*/
	if (sc->debug & HPF_ECCDEBUG)
		printf("bcr=%d npf=%d ssect=%d sectsiz=%d i_cc=%d\n",
			bcr, npf, sc->ssect, sectsiz, io->i_cc);
	/*
	 * ECC correction logic.
	 */
	if (flag == ECC) {
		register int i;
		caddr_t addr;
		int bit, o, mask;

		printf("hp%d: soft ecc sn%d\n", unit, bn);
		mask = MASKREG(rp->hpec2);
		for (i = mask, bit = 0; i; i >>= 1)
			if (i & 1)
				bit++;
		if (bit > sc->ecclim) {
			printf("%d-bit error\n", bit);
			return (1);
		}
		i = MASKREG(rp->hpec1) - 1;	/* -1 makes 0 origin */
		bit = i&07;
		o = (i & ~07) >> 3;
		rp->hpcs1 = HP_DCLR | HP_GO;
		while (o <sectsiz && npf*sectsiz + o < io->i_cc && bit > -11) {
			addr = io->i_ma + (npf*sectsiz) + o;
			/*
			 * No data transfer occurs with a write check,
			 * so don't correct the resident copy of data.
			 */
			if ((io->i_flgs & (F_CHECK|F_HCHECK)) == 0) {
				if (sc->debug & HPF_ECCDEBUG)
					printf("addr=%x old=%x ", addr,
						(*addr & 0xff));
				*addr ^= (mask << bit);
				if (sc->debug & HPF_ECCDEBUG)
					printf("new=%x\n",(*addr & 0xff));
			}
			o++, bit -= 8;
		}
		return (0);
	}

	/*
	 * Skip sector error.
	 * Set skip-sector-inhibit and
	 * read next sector
	 */
	if (flag == SSE) {
		rp->hpcs1 = HP_DCLR | HP_GO;
		HPWAIT(rp);
		rp->hpof |= HPOF_SSEI;
		return (0);	
	}

	/*
	 * Bad block forwarding.
	 */
	 if (flag == BSE) {
		int bbn;

		rp->hpcs1 = HP_DCLR | HP_GO;
		if (sc->debug & HPF_BSEDEBUG)
			printf("hpecc: BSE @ bn %d\n", bn);
		cn = bn/st->nspc;
		sn = bn%st->nspc;
		tn = sn/st->nsect;
		sn = sn%st->nsect;
		bcr += sectsiz;
		if ((bbn = isbad(&hpbad[unit], cn, tn, sn)) < 0)
			return (1);
		bbn = st->ncyl*st->nspc - st->nsect - 1 - bbn;
		cn = bbn/st->nspc;
		sn = bbn%st->nspc;
		tn = sn/st->nsect;
		sn = sn%st->nsect;
		io->i_cc = sectsiz;
		io->i_ma += npf*sectsiz;
		if (sc->debug & HPF_BSEDEBUG)
			printf("revector to cn %d tn %d sn %d\n", cn, tn, sn);
		rp->hpof &= ~HPOF_SSEI;
		mbp->mba_sr = -1;
		rp->hpdc = cn;
		rp->hpda = (tn<<8) + sn;
		mbastart(io,io->i_flgs);
		io->i_errcnt = 0;
		HPWAIT(rp);
		return (rp->hpds&HPDS_ERR);
	}
	printf("hpecc: flag=%d\n", flag);
	return (1);
}

/*ARGSUSED*/
hpioctl(io, cmd, arg)
	struct iob *io;
	int cmd;
	caddr_t arg;
{
	register unit = io->i_unit;
	register struct hp_softc *sc = &hp_softc[unit];
	struct st *st = &hpst[sc->type];
	struct mba_drv *drv = mbadrv(unit);

	switch(cmd) {

	case SAIODEBUG:
		sc->debug = (int)arg;
		break;

	case SAIODEVDATA:
		if (drv->mbd_dt&MBDT_TAP)
			return (ECMD);
		*(struct st *)arg = *st;
		break;

	case SAIOGBADINFO:
		if (drv->mbd_dt&MBDT_TAP)
			return (ECMD);
		*(struct dkbad *)arg = hpbad[unit];
		break;

	case SAIOECCLIM:
		sc->ecclim = (int)arg;
		break;

	case SAIORETRIES:
		sc->retries = (int)arg;
		break;

	case SAIOSSI:			/* skip-sector-inhibit */
		if (drv->mbd_dt&MBDT_TAP)
			return (ECMD);
		if ((io->i_flgs&F_SSI) == 0) {
			/* make sure this is done once only */
			io->i_flgs |= F_SSI;
			st->nsect++;
			st->nspc += st->ntrak;
		}
		break;

	case SAIONOSSI:			/* remove skip-sector-inhibit */
		if (io->i_flgs & F_SSI) {
			io->i_flgs &= ~F_SSI;
			drv->mbd_of &= ~HPOF_SSEI;
			st->nsect--;
			st->nspc -= st->ntrak;
		}
		break;

	case SAIOSSDEV:			/* drive have skip sector? */
		return (RM80 ? 0 : ECMD);

	default:
		return (ECMD);
	}
	return (0);
}
