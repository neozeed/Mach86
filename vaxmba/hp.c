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
 *	@(#)hp.c	6.14 (Berkeley) 9/17/85
 */
#if	CMU
 
/*
 **********************************************************************
 * HISTORY
 * 26-Jan-86  Avadis Tevanian (avie) at Carnegie-Mellon University
 *	Upgraded to 4.3.
 *
 * 20-Aug-85  Robert V Baron (rvb) at Carnegie-Mellon University
 *	Fix up hpdump to be able to dump from shared memory
 *
 * 10-May-85  Mike Accetta (mja) at Carnegie-Mellon University
 *	CS_GENERIC: Upgraded from 4.1BSD.  Retained local partition
 *	sizes for RM03, SI9730 and SI9751.
 *	[V1(1)]
 *
 **********************************************************************
 */
 
#include "cs_generic.h"
#include "mach_load.h"
#include "mach_hp.h"
#endif	CMU

#ifdef HPDEBUG
int	hpdebug;
#endif
#ifdef HPBDEBUG
int	hpbdebug;
#endif

#include "hp.h"
#if NHP > 0
/*
 * HP disk driver for RP0x+RMxx+ML11
 *
 * TODO:
 *	check RM80 skip sector handling when ECC's occur later
 *	check offset recovery handling
 *	see if DCLR and/or RELEASE set attention status
 *	print bits of mr && mr2 symbolically
 */
#include "../machine/pte.h"

#include "param.h"
#include "systm.h"
#include "dk.h"
#include "buf.h"
#include "conf.h"
#include "dir.h"
#include "user.h"
#include "map.h"
#include "../vax/mtpr.h"
#include "vm.h"
#include "cmap.h"
#include "dkbad.h"
#include "ioctl.h"
#include "uio.h"
#include "syslog.h"

#include "../vax/dkio.h"
#include "mbareg.h"
#include "mbavar.h"
#include "hpreg.h"

/* THIS SHOULD BE READ OFF THE PACK, PER DRIVE */
struct	size {
	daddr_t	nblocks;
	int	cyloff;
} rp06_sizes[8] = {
	15884,	0,		/* A=cyl 0 thru 37 */
	33440,	38,		/* B=cyl 38 thru 117 */
	340670,	0,		/* C=cyl 0 thru 814 */
	15884,	118,		/* D=cyl 118 thru 155 */
	55936,	156,		/* E=cyl 156 thru 289 */
	219384,	290,		/* F=cyl 290 thru 814 */
	291280,	118,		/* G=cyl 118 thru 814 */
	0,	0,
}, rp05_sizes[8] = {
	15884,	0,		/* A=cyl 0 thru 37 */
	33440,	38,		/* B=cyl 38 thru 117 */
	171798,	0,		/* C=cyl 0 thru 410 */
	15884,	118,		/* D=cyl 118 thru 155 */
	55936,	156,		/* E=cyl 156 thru 289 */
	50512,	290,		/* F=cyl 290 thru 410 */
	122408,	118,		/* G=cyl 118 thru 410 */
	0,	0,
}, rm03_sizes[8] = {
	15884,	0,		/* A=cyl 0 thru 99 */
	33440,	100,		/* B=cyl 100 thru 308 */
	131680,	0,		/* C=cyl 0 thru 822 */
	15884,	309,		/* D=cyl 309 thru 408 */
	55936,	409,		/* E=cyl 409 thru 758 */
	10144,	759,		/* F=cyl 759 thru 822 */
#if	CS_GENERIC && ! MACH_HP
	81984,	310,		/* G=cyl 310 thru 822 */
#else	CS_GENERIC && ! MACH_HP
	82144,	309,		/* G=cyl 309 thru 822 */
#endif	CS_GENERIC && ! MACH_HP
	0,	0,
}, rm05_sizes[8] = {
	15884,	0,		/* A=cyl 0 thru 26 */
	33440,	27,		/* B=cyl 27 thru 81 */
	500384,	0,		/* C=cyl 0 thru 822 */
	15884,	562,		/* D=cyl 562 thru 588 */
	55936,	589,		/* E=cyl 589 thru 680 */
	86240,	681,		/* F=cyl 681 thru 822 */
	158592,	562,		/* G=cyl 562 thru 822 */
	291346,	82,		/* H=cyl 82 thru 561 */
}, rm80_sizes[8] = {
	15884,	0,		/* A=cyl 0 thru 36 */
	33440,	37,		/* B=cyl 37 thru 114 */
	242606,	0,		/* C=cyl 0 thru 558 */
	15884,	115,		/* D=cyl 115 thru 151 */
	55936,	152,		/* E=cyl 152 thru 280 */
	120559,	281,		/* F=cyl 281 thru 558 */
	192603,	115,		/* G=cyl 115 thru 558 */
	0,	0,
}, rp07_sizes[8] = {
	15884,	0,		/* A=cyl 0 thru 9 */
	66880,	10,		/* B=cyl 10 thru 51 */
	1008000, 0,		/* C=cyl 0 thru 629 */
	15884,	235,		/* D=cyl 235 thru 244 */
	307200,	245,		/* E=cyl 245 thru 436 */
	308650,	437,		/* F=cyl 437 thru 629 */
	631850,	235,		/* G=cyl 235 thru 629 */
	291346,	52,		/* H=cyl 52 thru 234 */
}, cdc9775_sizes[8] = {
	15884,	0,		/* A=cyl 0 thru 12 */
	66880,	13,		/* B=cyl 13 thru 65 */
	1077760, 0,		/* C=cyl 0 thru 841 */
	15884,	294,		/* D=cyl 294 thru 306 */
	307200,	307,		/* E=cyl 307 thru 546 */
	377440,	547,		/* F=cyl 547 thru 841 */
	701280,	294,		/* G=cyl 294 thru 841 */
	291346,	66,		/* H=cyl 66 thru 293 */
}, cdc9730_sizes[8] = {
#if	CS_GENERIC
	( 50-  0)*(32*10),   0,	/* A=cyl   0 thru  49 */
	(155- 50)*(32*10),  50,	/* B=cyl  50 thru 154 */
	(823-  0)*(32*10),   0,	/* C=cyl   0 thru 822 (entire disk) */
	(456-155)*(32*10), 155,	/* D=cyl 155 thru 455 */
	(823-456)*(32*10)
		  -32-126, 456,	/* E=cyl 456 thru 822 */
	0,	0,		/* F unused */
	0,	0,		/* G unused */
	(823-155)*(32*10)
		  -32-126, 155,	/* H=cyl 155 thru 822 */
#else	CS_GENERIC
	15884,	0,		/* A=cyl 0 thru 49 */
	33440,	50,		/* B=cyl 50 thru 154 */
	263360,	0,		/* C=cyl 0 thru 822 */
	15884,	155,		/* D=cyl 155 thru 204 */
	55936,	205,		/* E=cyl 205 thru 379 */
	141664,	380,		/* F=cyl 380 thru 822 */
	213664,	155,		/* G=cyl 155 thru 822 */
	0,	0,
#endif	CS_GENERIC
}, capricorn_sizes[8] = {
	15884,	0,		/* A=cyl 0 thru 31 */
	33440,	32,		/* B=cyl 32 thru 97 */
	524288,	0,		/* C=cyl 0 thru 1023 */
	15884,	668,		/* D=cyl 668 thru 699 */
	55936,	700,		/* E=cyl 700 thru 809 */
	109472,	810,		/* F=cyl 810 thru 1023 */
	182176,	668,		/* G=cyl 668 thru 1023 */
	291346,	98,		/* H=cyl 98 thru 667 */
}, eagle_sizes[8] = {
#if	CS_GENERIC && ! MACH_HP
	( 17-  0)*(48*20),   0,	/* A=cyl   0 thru  16 */
	( 52- 17)*(48*20),  17,	/* A=cyl  17 thru  51 */
	(842-  0)*(48*20),   0,	/* C=cyl   0 thru 841 */
	(153- 52)*(48*20),  52,	/* D=cyl  52 thru 152 */
	(842-153)*(48*20)
		  -48-126, 153,	/* E=cyl 153 thru 841 */
	0,	0,		/* F unused */
	0,	0,		/* G unused */
	(842- 52)*(48*20)
		  -48-126,  52,	/* H=cyl  52 thru 841 */
#else	CS_GENERIC && ! MACH_HP
	15884,	0,		/* A=cyl 0 thru 16 */
	66880,	17,		/* B=cyl 17 thru 86 */
	808320,	0,		/* C=cyl 0 thru 841 */
	15884,	391,		/* D=cyl 391 thru 407 */
	307200,	408,		/* E=cyl 408 thru 727 */
	109296,	728,		/* F=cyl 728 thru 841 */
	432816,	391,		/* G=cyl 391 thru 841 */
	291346,	87,		/* H=cyl 87 thru 390 */
#endif	CS_GENERIC && ! MACH_HP
}, ampex_sizes[8] = {
	15884,	0,		/* A=cyl 0 thru 26 */
	33440,	27,		/* B=cyl 27 thru 81 */
	495520,	0,		/* C=cyl 0 thru 814 */
	15884,	562,		/* D=cyl 562 thru 588 */
	55936,	589,		/* E=cyl 589 thru 680 */
	81312,	681,		/* F=cyl 681 thru 814 */
	153664,	562,		/* G=cyl 562 thru 814 */
	291346,	82,		/* H=cyl 82 thru 561 */
};
/* END OF STUFF WHICH SHOULD BE READ IN PER DISK */

/*
 * Table for converting Massbus drive types into
 * indices into the partition tables.  Slots are
 * left for those drives devined from other means
 * (e.g. SI, AMPEX, etc.).
 */
short	hptypes[] = {
#define	HPDT_RM03	0
	MBDT_RM03,
#define	HPDT_RM05	1
	MBDT_RM05,
#define	HPDT_RP06	2
	MBDT_RP06,
#define	HPDT_RM80	3
	MBDT_RM80,
#define	HPDT_RP04	4
	MBDT_RP04,
#define	HPDT_RP05	5
	MBDT_RP05,
#define	HPDT_RP07	6
	MBDT_RP07,
#define	HPDT_ML11A	7
	MBDT_ML11A,
#define	HPDT_ML11B	8
	MBDT_ML11B,
#define	HPDT_9775	9
	-1,
#define	HPDT_9730	10
	-1,
#define	HPDT_CAPRICORN	11
	-1,
#define HPDT_EAGLE	12
	-1,
#define	HPDT_9300	13
	-1,
#define HPDT_RM02	14
	MBDT_RM02,		/* beware, actually capricorn or eagle */
	0
};
struct	mba_device *hpinfo[NHP];
int	hpattach(),hpustart(),hpstart(),hpdtint();
struct	mba_driver hpdriver =
	{ hpattach, 0, hpustart, hpstart, hpdtint, 0,
	  hptypes, "hp", 0, hpinfo };

/*
 * These variable are all measured in sectors.  
 * Sdist is how much to "lead" in the search for a desired sector
 * (i.e. if want N, search for N-sdist.)
 * Maxdist and mindist define the region right before our desired sector within
 * which we don't bother searching.  We don't search when we are already less
 * then maxdist and more than mindist sectors "before" our desired sector.
 * Maxdist should be >= sdist.
 * 
 * Beware, sdist, mindist and maxdist are not well tuned
 * for many of the drives listed in this table.
 * Try patching things with something i/o intensive
 * running and watch iostat.
 */
struct hpst {
	short	nsect;		/* # sectors/track */
	short	ntrak;		/* # tracks/cylinder */
	short	nspc;		/* # sector/cylinders */
	short	ncyl;		/* # cylinders */
	struct	size *sizes;	/* partition tables */
	short	sdist;		/* seek distance metric */
	short	maxdist;	/* boundaries of non-searched area */
	short	mindist;	/* preceding the target sector */
} hpst[] = {
    { 32, 5,	32*5,	823,	rm03_sizes,	7, 4, 1 },	/* RM03 */
    { 32, 19,	32*19,	823,	rm05_sizes,	7, 4, 1 },	/* RM05 */
    { 22,19,	22*19,	815,	rp06_sizes,	7, 4, 1 },	/* RP06 */
    { 31, 14, 	31*14,	559,	rm80_sizes,	7, 4, 1 },	/* RM80 */
    { 22, 19,	22*19,	411,	rp05_sizes,	7, 4, 1 },	/* RP04 */
    { 22, 19,	22*19,	411,	rp05_sizes,	7, 4, 1 },	/* RP05 */
    { 50, 32,	50*32,	630,	rp07_sizes,    15, 8, 3 },	/* RP07 */
    { 1, 1,	1,	1,	0,		0, 0, 0 },	/* ML11A */
    { 1, 1,	1,	1,	0,		0, 0, 0 },	/* ML11B */
    { 32, 40,	32*40,	843,	cdc9775_sizes,	7, 4, 1 },	/* 9775 */
    { 32, 10,	32*10,	823,	cdc9730_sizes,	7, 4, 1 },	/* 9730 */
    { 32, 16,	32*16,	1024,	capricorn_sizes,10,4, 3 },	/* Capricorn */
    { 48, 20,	48*20,	842,	eagle_sizes,   15, 8, 3 },	/* EAGLE */
    { 32, 19,	32*19,	815,	ampex_sizes,	7, 4, 1 },	/* 9300 */
};

u_char	hp_offset[16] = {
    HPOF_P400, HPOF_M400, HPOF_P400, HPOF_M400,
    HPOF_P800, HPOF_M800, HPOF_P800, HPOF_M800,
    HPOF_P1200, HPOF_M1200, HPOF_P1200, HPOF_M1200,
    0, 0, 0, 0,
};

struct	buf	rhpbuf[NHP];
struct	buf	bhpbuf[NHP];
struct	dkbad	hpbad[NHP];

struct	hpsoftc {
	u_char	sc_hpinit;	/* drive initialized */
	u_char	sc_recal;	/* recalibrate state */
	u_char	sc_hdr;		/* next i/o includes header */
	u_char	sc_doseeks;	/* perform explicit seeks */
	daddr_t	sc_mlsize;	/* ML11 size */
	int	sc_pgdone;	/* amount sucessfully transfered */
	daddr_t	sc_badbn;	/* replacement block number */
} hpsoftc[NHP];

#define	b_cylin b_resid
 
/* #define ML11 0  to remove ML11 support */
#define	ML11	(hptypes[mi->mi_type] == MBDT_ML11A)
#define	RP06	(hptypes[mi->mi_type] <= MBDT_RP06)
#define	RM80	(hptypes[mi->mi_type] == MBDT_RM80)

#define hpunit(dev)	(minor(dev) >> 3)
#define	MASKREG(reg)	((reg)&0xffff)
#define HPWAIT(mi, addr) (((addr)->hpds & HPDS_DRY) || hpwait(mi))

/*ARGSUSED*/
hpattach(mi, slave)
	register struct mba_device *mi;
{

	mi->mi_type = hpmaptype(mi);
	if (!ML11 && mi->mi_dk >= 0) {
		struct hpst *st = &hpst[mi->mi_type];

		dk_mspw[mi->mi_dk] = 1.0 / 60 / (st->nsect * 256);
	}
}

/*
 * Map apparent MASSBUS drive type into manufacturer
 * specific configuration.  For SI controllers this is done
 * based on codes in the serial number register.  For
 * EMULEX controllers, the track and sector attributes are
 * used when the drive type is an RM02 (not supported by DEC).
 */
hpmaptype(mi)
	register struct mba_device *mi;
{
	register struct hpdevice *hpaddr = (struct hpdevice *)mi->mi_drv;
	register int type = mi->mi_type;

	/*
	 * Model-byte processing for SI controllers.
	 * NB:  Only deals with RM03 and RM05 emulations.
	 */
	if (type == HPDT_RM03 || type == HPDT_RM05) {
		int hpsn = hpaddr->hpsn;

		if ((hpsn & SIMB_LU) != mi->mi_drive)
			return (type);
		switch ((hpsn & SIMB_MB) & ~(SIMB_S6|SIRM03|SIRM05)) {

		case SI9775D:
			printf("hp%d: 9775 (direct)\n", mi->mi_unit);
			type = HPDT_9775;
			break;

		case SI9730D:
			printf("hp%d: 9730 (direct)\n", mi->mi_unit);
			type = HPDT_9730;
			break;

		/*
		 * Beware, since the only SI controller we
		 * have has a 9300 instead of a 9766, we map the
		 * drive type into the 9300.  This means that
		 * on a 9766 you lose the last 8 cylinders (argh).
		 */
		case SI9766:
			printf("hp%d: 9300\n", mi->mi_unit);
			type = HPDT_9300;
			break;

		case SI9762:
			printf("hp%d: 9762\n", mi->mi_unit);
			type = HPDT_RM03;
			break;

		case SICAPD:
			printf("hp%d: capricorn\n", mi->mi_unit);
			type = HPDT_CAPRICORN;
			break;

		case SI9751D:
			printf("hp%d: eagle\n", mi->mi_unit);
			type = HPDT_EAGLE;
			break;
		}
		return (type);
	}

	/*
	 * EMULEX SC750 or SC780.  Poke the holding register.
	 */
	if (type == HPDT_RM02) {
		int ntracks, nsectors;

		hpaddr->hpof = HPOF_FMT22;
		mbclrattn(mi);
		hpaddr->hpcs1 = HP_NOP;
		hpaddr->hphr = HPHR_MAXTRAK;
		ntracks = MASKREG(hpaddr->hphr) + 1;
		if (ntracks == 16) {
			printf("hp%d: capricorn\n", mi->mi_unit);
			type = HPDT_CAPRICORN;
			goto done;
		}
		if (ntracks == 19) {
			printf("hp%d: 9300\n", mi->mi_unit);
			type = HPDT_9300;
			goto done;
		}
		hpaddr->hpcs1 = HP_NOP;
		hpaddr->hphr = HPHR_MAXSECT;
		nsectors = MASKREG(hpaddr->hphr) + 1;
		if (ntracks == 20 && nsectors == 48) {
			type = HPDT_EAGLE;
			printf("hp%d: eagle\n", mi->mi_unit);
			goto done;
		}
		printf("hp%d: ntracks %d, nsectors %d: unknown device\n",
			mi->mi_unit, ntracks, nsectors);
done:
		hpaddr->hpcs1 = HP_DCLR|HP_GO;
		mbclrattn(mi);		/* conservative */
		return (type);
	} 

	/*
	 * Map all ML11's to the same type.  Also calculate
	 * transfer rates based on device characteristics.
	 */
	if (type == HPDT_ML11A || type == HPDT_ML11B) {
		register struct hpsoftc *sc = &hpsoftc[mi->mi_unit];
		register int trt;

		sc->sc_mlsize = hpaddr->hpmr & HPMR_SZ;
		if ((hpaddr->hpmr & HPMR_ARRTYP) == 0)
			sc->sc_mlsize >>= 2;
		if (mi->mi_dk >= 0) {
			trt = (hpaddr->hpmr & HPMR_TRT) >> 8;
			dk_mspw[mi->mi_dk] = 1.0 / (1<<(20-trt));
		}
		type = HPDT_ML11A;
	}
	return (type);
}

hpopen(dev)
	dev_t dev;
{
	register int unit = hpunit(dev);
	register struct mba_device *mi;

	if (unit >= NHP || (mi = hpinfo[unit]) == 0 || mi->mi_alive == 0)
		return (ENXIO);
	return (0);
}

hpstrategy(bp)
	register struct buf *bp;
{
	register struct mba_device *mi;
	register struct hpst *st;
	register int unit;
	long sz;
	int xunit = minor(bp->b_dev) & 07;
	int s;

	sz = bp->b_bcount;
	sz = (sz+511) >> 9;
	unit = hpunit(bp->b_dev);
	if (unit >= NHP) {
		bp->b_error = ENXIO;
		goto bad;
	}
	mi = hpinfo[unit];
	if (mi == 0 || mi->mi_alive == 0) {
		bp->b_error = ENXIO;
		goto bad;
	}
	st = &hpst[mi->mi_type];
	if (ML11) {
		struct hpsoftc *sc = &hpsoftc[unit];

		if (bp->b_blkno < 0 ||
		    bp->b_blkno+sz > sc->sc_mlsize) {
			if (bp->b_blkno == sc->sc_mlsize) {
			    bp->b_resid = bp->b_bcount;
			    goto done;
			}
			bp->b_error = EINVAL;
			goto bad;
		}
		bp->b_cylin = 0;
	} else {
		if (bp->b_blkno < 0 ||
		    bp->b_blkno+sz > st->sizes[xunit].nblocks) {
			if (bp->b_blkno == st->sizes[xunit].nblocks) {
			    bp->b_resid = bp->b_bcount;
			    goto done;
			}
			bp->b_error = EINVAL;
			goto bad;
		}
		bp->b_cylin = bp->b_blkno/st->nspc + st->sizes[xunit].cyloff;
	}
	s = spl5();
	disksort(&mi->mi_tab, bp);
	if (mi->mi_tab.b_active == 0)
		mbustart(mi);
	splx(s);
	return;

bad:
	bp->b_flags |= B_ERROR;
done:
	iodone(bp);
	return;
}

hpustart(mi)
	register struct mba_device *mi;
{
	register struct hpdevice *hpaddr = (struct hpdevice *)mi->mi_drv;
	register struct buf *bp = mi->mi_tab.b_actf;
	register struct hpst *st;
	struct hpsoftc *sc = &hpsoftc[mi->mi_unit];
	daddr_t bn;
	int sn, tn, dist;

	st = &hpst[mi->mi_type];
	hpaddr->hpcs1 = 0;
	if ((hpaddr->hpcs1&HP_DVA) == 0)
		return (MBU_BUSY);
	if ((hpaddr->hpds & HPDS_VV) == 0 || !sc->sc_hpinit) {
		struct buf *bbp = &bhpbuf[mi->mi_unit];

		sc->sc_hpinit = 1;
		hpaddr->hpcs1 = HP_DCLR|HP_GO;
		if (mi->mi_mba->mba_drv[0].mbd_as & (1<<mi->mi_drive))
			printf("DCLR attn\n");
		hpaddr->hpcs1 = HP_PRESET|HP_GO;
		if (!ML11)
			hpaddr->hpof = HPOF_FMT22;
		mbclrattn(mi);
		if (!ML11) {
			bbp->b_flags = B_READ|B_BUSY;
			bbp->b_dev = bp->b_dev;
			bbp->b_bcount = 512;
			bbp->b_un.b_addr = (caddr_t)&hpbad[mi->mi_unit];
			bbp->b_blkno = st->ncyl*st->nspc - st->nsect;
			bbp->b_cylin = st->ncyl - 1;
			mi->mi_tab.b_actf = bbp;
			bbp->av_forw = bp;
			bp = bbp;
		}
	}
	if (mi->mi_tab.b_active || mi->mi_hd->mh_ndrive == 1)
		return (MBU_DODATA);
	if (ML11)
		return (MBU_DODATA);
	if ((hpaddr->hpds & HPDS_DREADY) != HPDS_DREADY)
		return (MBU_DODATA);
	bn = bp->b_blkno;
	sn = bn % st->nspc;
	tn = sn / st->nsect;
	sn = sn % st->nsect;
	if (bp->b_cylin == MASKREG(hpaddr->hpdc)) {
		if (sc->sc_doseeks)
			return (MBU_DODATA);
		dist = sn - (MASKREG(hpaddr->hpla) >> 6) - 1;
		if (dist < 0)
			dist += st->nsect;
		if (dist > st->maxdist || dist < st->mindist)
			return (MBU_DODATA);
	} else
		hpaddr->hpdc = bp->b_cylin;
	if (sc->sc_doseeks)
		hpaddr->hpcs1 = HP_SEEK|HP_GO;
	else {
		sn = (sn + st->nsect - st->sdist) % st->nsect;
		hpaddr->hpda = (tn << 8) + sn;
		hpaddr->hpcs1 = HP_SEARCH|HP_GO;
	}
	return (MBU_STARTED);
}

hpstart(mi)
	register struct mba_device *mi;
{
	register struct hpdevice *hpaddr = (struct hpdevice *)mi->mi_drv;
	register struct buf *bp = mi->mi_tab.b_actf;
	register struct hpst *st = &hpst[mi->mi_type];
	struct hpsoftc *sc = &hpsoftc[mi->mi_unit];
	daddr_t bn;
	int sn, tn;

	if (bp->b_flags & B_BAD)
		bn = sc->sc_badbn;
	else
		bn = bp->b_blkno + sc->sc_pgdone;
	if (ML11)
		hpaddr->hpda = bn;
	else {
		sn = bn%st->nspc;
		tn = sn/st->nsect;
		sn %= st->nsect;
		hpaddr->hpdc = bp->b_cylin;
		hpaddr->hpda = (tn << 8) + sn;
	}
	if (sc->sc_hdr) {
		if (bp->b_flags & B_READ)
			return (HP_RHDR|HP_GO);
		else
			return (HP_WHDR|HP_GO);
	}
	return (0);
}

hpdtint(mi, mbsr)
	register struct mba_device *mi;
	int mbsr;
{
	register struct hpdevice *hpaddr = (struct hpdevice *)mi->mi_drv;
	register struct buf *bp = mi->mi_tab.b_actf;
	register struct hpst *st;
	register int er1, er2;
	struct hpsoftc *sc = &hpsoftc[mi->mi_unit];
	int retry = 0, i;

	st = &hpst[mi->mi_type];
	if (hpaddr->hpds&HPDS_ERR || mbsr&MBSR_EBITS) {
		er1 = hpaddr->hper1;
		er2 = hpaddr->hper2;
		if (HPWAIT(mi, hpaddr) == 0)
			goto hard;
#ifdef HPDEBUG
		if (hpdebug) {
			int dc = hpaddr->hpdc, da = hpaddr->hpda;

			printf("hperr: bp %x cyl %d blk %d pgdone %d as %o ",
				bp, bp->b_cylin, bp->b_blkno, sc->sc_pgdone,
				hpaddr->hpas&0xff);
			printf("dc %x da %x\n",MASKREG(dc), MASKREG(da));
			printf("errcnt %d ", mi->mi_tab.b_errcnt);
			printf("mbsr=%b ", mbsr, mbsr_bits);
			printf("er1=%b er2=%b\n", MASKREG(er1), HPER1_BITS,
			    MASKREG(er2), HPER2_BITS);
			DELAY(1000000);
		}
#endif
		if (er1 & HPER1_HCRC) {
			er1 &= ~(HPER1_HCE|HPER1_FER);
			er2 &= ~HPER2_BSE;
		}
		if (er1 & HPER1_WLE) {
			log(LOG_WARNING, "hp%d: write locked\n",
			    hpunit(bp->b_dev));
			bp->b_flags |= B_ERROR;
		} else if (sc->sc_hdr) {
			goto hard;
		} else if (RM80 && er2&HPER2_SSE) {
			(void) hpecc(mi, SSE);
			return (MBD_RESTARTED);
		} else if ((er2 & HPER2_BSE) && !ML11) {
			if (hpecc(mi, BSE))
				return (MBD_RESTARTED);
			goto hard;
		} else if (MASKREG(er1) == HPER1_FER && RP06) {
			if (hpecc(mi, BSE))
				return (MBD_RESTARTED);
			goto hard;
		} else if ((er1 & (HPER1_DCK | HPER1_ECH)) == HPER1_DCK &&
		    mi->mi_tab.b_errcnt > 7) {
			if (hpecc(mi, ECC))
				return (MBD_RESTARTED);
			/* else done */
		} else if ((er1 & HPER1_HCRC) && !ML11 && hpecc(mi, BSE)) {
 			/*
 			 * HCRC means the header is screwed up and the sector
 			 * might well exist in the bad sector table, 
			 * better check....
 			 */
			return (MBD_RESTARTED);
		} else if (++mi->mi_tab.b_errcnt > 27 ||
		    (ML11 && mi->mi_tab.b_errcnt > 15) ||
		    mbsr & MBSR_HARD ||
		    er1 & HPER1_HARD ||
		    (!ML11 && (er2 & HPER2_HARD))) {
hard:
			if (bp->b_flags & B_BAD)
				bp->b_blkno = sc->sc_badbn;
			else {
				bp->b_blkno = bp->b_blkno + btop(bp->b_bcount -
				    MASKREG(-mi->mi_mba->mba_bcr));
				if (er1 & (HPER1_DCK | HPER1_ECH))
					bp->b_blkno--;
			}
			harderr(bp, "hp");
			if (mbsr & (MBSR_EBITS &~ (MBSR_DTABT|MBSR_MBEXC)))
				printf("mbsr=%b ", mbsr, mbsr_bits);
			printf("er1=%b er2=%b",
			    MASKREG(hpaddr->hper1), HPER1_BITS,
			    MASKREG(hpaddr->hper2), HPER2_BITS);
			if (hpaddr->hpmr)
				printf(" mr=%o", MASKREG(hpaddr->hpmr));
			if (hpaddr->hpmr2)
				printf(" mr2=%o", MASKREG(hpaddr->hpmr2));
			if (sc->sc_hdr)
				printf(" (hdr i/o)");
			printf("\n");
			bp->b_flags |= B_ERROR;
			bp->b_flags &= ~B_BAD;
		} else
			retry = 1;
		hpaddr->hpcs1 = HP_DCLR|HP_GO;
		if ((mi->mi_tab.b_errcnt & 07) == 4) {
			hpaddr->hpcs1 = HP_RECAL|HP_GO;
			sc->sc_recal = 1;
			return (MBD_RESTARTED);
		}
	}
#ifdef HPDEBUG
	else
		if (hpdebug && sc->sc_recal) {
			printf("recal %d ", sc->sc_recal);
			printf("errcnt %d\n", mi->mi_tab.b_errcnt);
			printf("mbsr=%b ", mbsr, mbsr_bits);
			printf("er1=%b er2=%b\n",
			    hpaddr->hper1, HPER1_BITS,
			    hpaddr->hper2, HPER2_BITS);
		}
#endif
	HPWAIT(mi, hpaddr);
	switch (sc->sc_recal) {

	case 1:
		hpaddr->hpdc = bp->b_cylin;
		hpaddr->hpcs1 = HP_SEEK|HP_GO;
		sc->sc_recal++;
		return (MBD_RESTARTED);
	case 2:
		retry = 1;
		break;
	}
	sc->sc_recal = 0;
	if (retry) {
		if (mi->mi_tab.b_errcnt >= 16 && (bp->b_flags & B_READ)) {
			hpaddr->hpof =
			    hp_offset[mi->mi_tab.b_errcnt & 017]|HPOF_FMT22;
			hpaddr->hpcs1 = HP_OFFSET|HP_GO;
			HPWAIT(mi, hpaddr);
			mbclrattn(mi);
		}
		return (MBD_RETRY);
	}
	if (mi->mi_tab.b_errcnt >= 16) {
		/*
		 * This is fast and occurs rarely; we don't
		 * bother with interrupts.
		 */
		hpaddr->hpcs1 = HP_RTC|HP_GO;
		HPWAIT(mi, hpaddr);
		mbclrattn(mi);
	}
	if ((bp->b_flags & B_BAD) && hpecc(mi, CONT))
		return (MBD_RESTARTED);
	sc->sc_hdr = 0;
	sc->sc_pgdone = 0;
	bp->b_resid = MASKREG(-mi->mi_mba->mba_bcr);
	if (!ML11) {
		hpaddr->hpof = HPOF_FMT22;
		hpaddr->hpcs1 = HP_RELEASE|HP_GO;
	}
	return (MBD_DONE);
}

/*
 * Wait (for a bit) for a drive to come ready;
 * returns nonzero on success.
 */
hpwait(mi)
	register struct mba_device *mi;
{
	register struct hpdevice *hpaddr = (struct hpdevice *)mi->mi_drv;
	register i = 100000;

	while ((hpaddr->hpds & HPDS_DRY) == 0 && --i)
		DELAY(10);
	if (i == 0)
		printf("hp%d: intr, not ready\n", mi->mi_unit);
	return (i);
}

hpread(dev, uio)
	dev_t dev;
	struct uio *uio;
{
	register int unit = hpunit(dev);

	if (unit >= NHP)
		return (ENXIO);
	return (physio(hpstrategy, &rhpbuf[unit], dev, B_READ, minphys, uio));
}

hpwrite(dev, uio)
	dev_t dev;
	struct uio *uio;
{
	register int unit = hpunit(dev);

	if (unit >= NHP)
		return (ENXIO);
	return (physio(hpstrategy, &rhpbuf[unit], dev, B_WRITE, minphys, uio));
}

/*ARGSUSED*/
hpioctl(dev, cmd, data, flag)
	dev_t dev;
	int cmd;
	caddr_t data;
	int flag;
{

	switch (cmd) {

	case DKIOCHDR:	/* do header read/write */
		hpsoftc[hpunit(dev)].sc_hdr = 1;
		return (0);

	default:
		return (ENXIO);
	}
}

hpecc(mi, flag)
	register struct mba_device *mi;
	int flag;
{
	register struct mba_regs *mbp = mi->mi_mba;
	register struct hpdevice *rp = (struct hpdevice *)mi->mi_drv;
	register struct buf *bp = mi->mi_tab.b_actf;
	register struct hpst *st = &hpst[mi->mi_type];
	struct hpsoftc *sc = &hpsoftc[mi->mi_unit];
	int npf, o;
	int bn, cn, tn, sn;
	int bcr;

	bcr = MASKREG(-mbp->mba_bcr);
	if (bp->b_flags & B_BAD)
		npf = bp->b_error;
	else
		npf = btop(bp->b_bcount - bcr);
	o = (int)bp->b_un.b_addr & PGOFSET;
	bn = bp->b_blkno;
	cn = bp->b_cylin;
	sn = bn%(st->nspc) + npf;
	tn = sn/st->nsect;
	sn %= st->nsect;
	cn += tn/st->ntrak;
	tn %= st->ntrak;
	bn += npf;
	switch (flag) {
	case ECC: {
		register int i;
		caddr_t addr;
		struct pte mpte;
		int bit, byte, mask;

		npf--;		/* because block in error is previous block */
		bn--;
		if (bp->b_flags & B_BAD)
			bn = sc->sc_badbn;
		log(LOG_WARNING, "hp%d%c: soft ecc sn%d\n", hpunit(bp->b_dev),
		    'a'+(minor(bp->b_dev)&07), bn);
		mask = MASKREG(rp->hpec2);
		i = MASKREG(rp->hpec1) - 1;		/* -1 makes 0 origin */
		bit = i&07;
		i = (i&~07)>>3;
		byte = i + o;
		while (i < 512 && (int)ptob(npf)+i < bp->b_bcount && bit > -11) {
			mpte = mbp->mba_map[npf+btop(byte)];
			addr = ptob(mpte.pg_pfnum) + (byte & PGOFSET);
			putmemc(addr, getmemc(addr)^(mask<<bit));
			byte++;
			i++;
			bit -= 8;
		}
		if (bcr == 0)
			return (0);
		npf++;
		break;
		}

	case SSE:
		rp->hpof |= HPOF_SSEI;
		if (bp->b_flags & B_BAD) {
			bn = sc->sc_badbn;
			goto fixregs;
		}
		mbp->mba_bcr = -(bp->b_bcount - (int)ptob(npf));
		break;

	case BSE:
 		if (rp->hpof & HPOF_SSEI)
 			sn++;
#ifdef HPBDEBUG
		if (hpbdebug)
		printf("hpecc, BSE: bn %d cn %d tn %d sn %d\n", bn, cn, tn, sn);
#endif
		if (bp->b_flags & B_BAD)
			return (0);
		if ((bn = isbad(&hpbad[mi->mi_unit], cn, tn, sn)) < 0)
			return (0);
		bp->b_flags |= B_BAD;
		bp->b_error = npf + 1;
 		rp->hpof &= ~HPOF_SSEI;
		bn = st->ncyl*st->nspc - st->nsect - 1 - bn;
		sc->sc_badbn = bn;
	fixregs:
		cn = bn/st->nspc;
		sn = bn%st->nspc;
		tn = sn/st->nsect;
		sn %= st->nsect;
		mbp->mba_bcr = -(min(512, bp->b_bcount - (int)ptob(npf)));
#ifdef HPBDEBUG
		if (hpbdebug)
		printf("revector to cn %d tn %d sn %d\n", cn, tn, sn);
#endif
		break;

	case CONT:
#ifdef HPBDEBUG
		if (hpbdebug)
		printf("hpecc, CONT: bn %d cn %d tn %d sn %d\n", bn,cn,tn,sn);
#endif
		bp->b_flags &= ~B_BAD;
		if ((int)ptob(npf) >= bp->b_bcount)
			return (0);
		mbp->mba_bcr = -(bp->b_bcount - (int)ptob(npf));
		break;
	}
	rp->hpcs1 = HP_DCLR|HP_GO;
	if (rp->hpof & HPOF_SSEI)
		sn++;
	rp->hpdc = cn;
	rp->hpda = (tn<<8) + sn;
	mbp->mba_sr = -1;
	mbp->mba_var = (int)ptob(npf) + o;
	rp->hpcs1 = bp->b_flags&B_READ ? HP_RCOM|HP_GO : HP_WCOM|HP_GO;
	mi->mi_tab.b_errcnt = 0;	/* error has been corrected */
	sc->sc_pgdone = npf;
	return (1);
}

#define	DBSIZE	20

hpdump(dev)
	dev_t dev;
{
	register struct mba_device *mi;
	register struct mba_regs *mba;
	struct hpdevice *hpaddr;
	char *start;
	int num, unit;
	register struct hpst *st;
#if	MACH_LOAD
	struct size *sizes;
#endif	MACH_LOAD

	num = maxfree;
#if	MACH_LOAD
	start = (char *) loadpt;
#else	MACH_LOAD
	start = 0;
#endif	MACH_LOAD
	unit = hpunit(dev);
	if (unit >= NHP)
		return (ENXIO);
#if	MACH_LOAD
#define	phys(a,b)	((b) (( ((int)(a)) & 0x7fffffff) + loadpt))
#else	MACH_LOAD
#define	phys(a,b)	((b)((int)(a)&0x7fffffff))
#endif	MACH_LOAD
	mi = phys(hpinfo[unit],struct mba_device *);
	if (mi == 0 || mi->mi_alive == 0)
		return (ENXIO);
	mba = phys(mi->mi_hd, struct mba_hd *)->mh_physmba;
	mba->mba_cr = MBCR_INIT;
	hpaddr = (struct hpdevice *)&mba->mba_drv[mi->mi_drive];
	if ((hpaddr->hpds & HPDS_VV) == 0) {
		hpaddr->hpcs1 = HP_DCLR|HP_GO;
		hpaddr->hpcs1 = HP_PRESET|HP_GO;
		hpaddr->hpof = HPOF_FMT22;
	}
	st = &hpst[mi->mi_type];
	if (dumplo < 0)
		return (EINVAL);
#if	MACH_LOAD
	sizes = phys(st->sizes, struct size *);
	if (dumplo + num >= sizes[minor(dev)&07].nblocks)
#else	MACH_LOAD
	if (dumplo + num >= st->sizes[minor(dev)&07].nblocks)
#endif	MACH_LOAD
		num = st->sizes[minor(dev)&07].nblocks - dumplo;
	while (num > 0) {
		register struct pte *hpte = mba->mba_map;
		register int i;
		int blk, cn, sn, tn;
		daddr_t bn;

		blk = num > DBSIZE ? DBSIZE : num;
#if	MACH_LOAD
		bn = dumplo + btop(start) - btop(loadpt);
		cn = bn/st->nspc + sizes[minor(dev)&07].cyloff;
#else	MACH_LOAD
		bn = dumplo + btop(start);
		cn = bn/st->nspc + st->sizes[minor(dev)&07].cyloff;
#endif	MACH_LOAD
		sn = bn%st->nspc;
		tn = sn/st->nsect;
		sn = sn%st->nsect;
		hpaddr->hpdc = cn;
		hpaddr->hpda = (tn << 8) + sn;
		for (i = 0; i < blk; i++)
			*(int *)hpte++ = (btop(start)+i) | PG_V;
		mba->mba_sr = -1;
		mba->mba_bcr = -(blk*NBPG);
		mba->mba_var = 0;
		hpaddr->hpcs1 = HP_WCOM | HP_GO;
		while ((hpaddr->hpds & HPDS_DRY) == 0)
			DELAY(10);
		if (hpaddr->hpds&HPDS_ERR)
			return (EIO);
		start += blk*NBPG;
		num -= blk;
	}
	return (0);
}

hpsize(dev)
	dev_t dev;
{
	int unit = hpunit(dev);
	struct mba_device *mi;
	struct hpst *st;

	if (unit >= NHP || (mi = hpinfo[unit]) == 0 || mi->mi_alive == 0)
		return (-1);
	st = &hpst[mi->mi_type];
	return ((int)st->sizes[minor(dev) & 07].nblocks);
}
#endif
