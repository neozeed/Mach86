/*
 * Copyright (c) 1982, 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)tm.c	7.1 (Berkeley) 6/5/86
 */

#include "te.h"
#include "ts.h"
#if NTE > 0
/*
 * TM11/TE10 tape driver
 *
 * TODO:
 *	test driver with more than one slave
 *	test driver with more than one controller
 *	test reset code
 *	what happens if you offline tape during rewind?
 *	test using file system on tape
 */
#include "../machine/pte.h"

#include "param.h"
#include "systm.h"
#include "buf.h"
#include "dir.h"
#include "conf.h"
#include "user.h"
#include "file.h"
#include "map.h"
#include "vm.h"
#include "ioctl.h"
#include "mtio.h"
#include "cmap.h"
#include "uio.h"
#include "kernel.h"
#include "tty.h"

#include "../vax/cpu.h"
#include "ubareg.h"
#include "ubavar.h"
#include "tmreg.h"

/*
 * There is a ctmbuf per tape controller.
 * It is used as the token to pass to the internal routines
 * to execute tape ioctls, and also acts as a lock on the slaves
 * on the controller, since there is only one per controller.
 * In particular, when the tape is rewinding on close we release
 * the user process but any further attempts to use the tape drive
 * before the rewind completes will hang waiting for ctmbuf.
 */
struct	buf	ctmbuf[NTM];

/*
 * Raw tape operations use rtmbuf.  The driver
 * notices when rtmbuf is being used and allows the user
 * program to continue after errors and read records
 * not of the standard length (BSIZE).
 */
struct	buf	rtmbuf[NTM];

/*
 * Driver unibus interface routines and variables.
 */
int	tmprobe(), tmslave(), tmattach(), tmdgo(), tmintr();
struct	uba_ctlr *tmminfo[NTM];
struct	uba_device *tedinfo[NTE];
struct	buf teutab[NTE];
short	tetotm[NTE];
u_short	tmstd[] = { 0772520, 0 };
struct	uba_driver tmdriver =
 { tmprobe, tmslave, tmattach, tmdgo, tmstd, "te", tedinfo, "tm", tmminfo, 0 };

/* bits in minor device */
#define	TEUNIT(dev)	(minor(dev)&03)
#define	TMUNIT(dev)	(tetotm[TEUNIT(dev)])
#define	T_NOREWIND	04
#define	T_1600BPI	0x8

#define	INF	(daddr_t)1000000L

/*
 * Software state per tape transport.
 *
 * 1. A tape drive is a unique-open device; we refuse opens when it is already.
 * 2. We keep track of the current position on a block tape and seek
 *    before operations by forward/back spacing if necessary.
 * 3. We remember if the last operation was a write on a tape, so if a tape
 *    is open read write and the last thing done is a write we can
 *    write a standard end of tape mark (two eofs).
 * 4. We remember the status registers after the last command, using
 *    then internally and returning them to the SENSE ioctl.
 * 5. We remember the last density the tape was used at.  If it is
 *    not a BOT when we start using it and we are writing, we don't
 *    let the density be changed.
 */
struct	te_softc {
	char	sc_openf;	/* lock against multiple opens */
	char	sc_lastiow;	/* last op was a write */
	daddr_t	sc_blkno;	/* block number, for block device tape */
	daddr_t	sc_nxrec;	/* position of end of tape, if known */
	u_short	sc_erreg;	/* copy of last erreg */
	u_short	sc_dsreg;	/* copy of last dsreg */
	short	sc_resid;	/* copy of last bc */
#ifdef unneeded
	short	sc_lastcmd;	/* last command to handle direction changes */
#endif
	u_short	sc_dens;	/* prototype command with density info */
	short	sc_tact;	/* timeout is active */
	daddr_t	sc_timo;	/* time until timeout expires */
	struct	tty *sc_ttyp;	/* record user's tty for errors */
} te_softc[NTE];
#ifdef unneeded
int	tmgapsdcnt;		/* DEBUG */
#endif

/*
 * States for um->um_tab.b_active, the per controller state flag.
 * This is used to sequence control in the driver.
 */
#define	SSEEK	1		/* seeking */
#define	SIO	2		/* doing seq i/o */
#define	SCOM	3		/* sending control command */
#define	SREW	4		/* sending a drive rewind */

/*
 * Determine if there is a controller for
 * a tm at address reg.  Our goal is to make the
 * device interrupt.
 */
tmprobe(reg)
	caddr_t reg;
{
	register int br, cvec;		/* must be r11,r10; value-result */

#ifdef lint
	br = 0; cvec = br; br = cvec;
	tmintr(0);
#endif
	((struct tmdevice *)reg)->tmcs = TM_IE;
	/*
	 * If this is a tm11, it ought to have interrupted
	 * by now, if it isn't (ie: it is a ts04) then we just
	 * hope that it didn't interrupt, so autoconf will ignore it.
	 * Just in case, we will reference one
	 * of the more distant registers, and hope for a machine
	 * check, or similar disaster if this is a ts.
	 *
	 * Note: on an 11/780, badaddr will just generate
	 * a uba error for a ts; but our caller will notice that
	 * so we won't check for it.
	 */
	if (badaddr((caddr_t)&((struct tmdevice *)reg)->tmrd, 2))
		return (0);
	return (sizeof (struct tmdevice));
}

/*
 * Due to a design flaw, we cannot ascertain if the tape
 * exists or not unless it is on line - ie: unless a tape is
 * mounted. This is too servere a restriction to bear,
 * so all units are assumed to exist.
 */
/*ARGSUSED*/
tmslave(ui, reg)
	struct uba_device *ui;
	caddr_t reg;
{

	return (1);
}

/*
 * Record attachment of the unit to the controller.
 */
/*ARGSUSED*/
tmattach(ui)
	struct uba_device *ui;
{
	/*
	 * Tetotm is used in TMUNIT to index the ctmbuf and rtmbuf
	 * arrays given a te unit number.
	 */
	tetotm[ui->ui_unit] = ui->ui_mi->um_ctlr;
}

int	tmtimer();
/*
 * Open the device.  Tapes are unique open
 * devices, so we refuse if it is already open.
 * We also check that a tape is available, and
 * don't block waiting here; if you want to wait
 * for a tape you should timeout in user code.
 */

#ifdef AVIV
int tmdens[4] = { 0x6000, 0x0000, 0x2000, 0 };
int tmdiag;
#endif AVIV

tmopen(dev, flag)
	dev_t dev;
	int flag;
{
	register int teunit;
	register struct uba_device *ui;
	register struct te_softc *sc;
	int olddens, dens;
	int s;

	teunit = TEUNIT(dev);
	if (teunit>=NTE || (ui = tedinfo[teunit]) == 0 || ui->ui_alive == 0)
		return (ENXIO);
	if ((sc = &te_softc[teunit])->sc_openf)
		return (EBUSY);
	olddens = sc->sc_dens;
	dens = TM_IE | TM_GO | (ui->ui_slave << 8);
#ifndef AVIV
	if ((minor(dev) & T_1600BPI) == 0)
		dens |= TM_D800;
#else AVIV
	dens |= tmdens[(minor(dev)>>3)&03];
#endif AVIV
	sc->sc_dens = dens;
get:
	tmcommand(dev, TM_SENSE, 1);
	if (sc->sc_erreg&TMER_SDWN) {
		sleep((caddr_t)&lbolt, PZERO+1);
		goto get;
	}
	sc->sc_dens = olddens;
	if ((sc->sc_erreg&(TMER_SELR|TMER_TUR)) != (TMER_SELR|TMER_TUR)) {
		uprintf("te%d: not online\n", teunit);
		return (EIO);
	}
	if ((flag&FWRITE) && (sc->sc_erreg&TMER_WRL)) {
		uprintf("te%d: no write ring\n", teunit);
		return (EIO);
	}
	if ((sc->sc_erreg&TMER_BOT) == 0 && (flag&FWRITE) &&
	    dens != sc->sc_dens) {
		uprintf("te%d: can't change density in mid-tape\n", teunit);
		return (EIO);
	}
	sc->sc_openf = 1;
	sc->sc_blkno = (daddr_t)0;
	sc->sc_nxrec = INF;
	sc->sc_lastiow = 0;
	sc->sc_dens = dens;
	sc->sc_ttyp = u.u_ttyp;
	s = splclock();
	if (sc->sc_tact == 0) {
		sc->sc_timo = INF;
		sc->sc_tact = 1;
		timeout(tmtimer, (caddr_t)dev, 5*hz);
	}
	splx(s);
	return (0);
}

/*
 * Close tape device.
 *
 * If tape was open for writing or last operation was
 * a write, then write two EOF's and backspace over the last one.
 * Unless this is a non-rewinding special file, rewind the tape.
 * Make the tape available to others.
 */
tmclose(dev, flag)
	register dev_t dev;
	register flag;
{
	register struct te_softc *sc = &te_softc[TEUNIT(dev)];

	if (flag == FWRITE || (flag&FWRITE) && sc->sc_lastiow) {
		tmcommand(dev, TM_WEOF, 1);
		tmcommand(dev, TM_WEOF, 1);
		tmcommand(dev, TM_SREV, 1);
	}
	if ((minor(dev)&T_NOREWIND) == 0)
		/*
		 * 0 count means don't hang waiting for rewind complete
		 * rather ctmbuf stays busy until the operation completes
		 * preventing further opens from completing by
		 * preventing a TM_SENSE from completing.
		 */
		tmcommand(dev, TM_REW, 0);
	sc->sc_openf = 0;
}

/*
 * Execute a command on the tape drive
 * a specified number of times.
 */
tmcommand(dev, com, count)
	dev_t dev;
	int com, count;
{
	register struct buf *bp;
	register int s;

	bp = &ctmbuf[TMUNIT(dev)];
	s = spl5();
	while (bp->b_flags&B_BUSY) {
		/*
		 * This special check is because B_BUSY never
		 * gets cleared in the non-waiting rewind case.
		 */
		if (bp->b_repcnt == 0 && (bp->b_flags&B_DONE))
			break;
		bp->b_flags |= B_WANTED;
		sleep((caddr_t)bp, PRIBIO);
	}
	bp->b_flags = B_BUSY|B_READ;
	splx(s);
	bp->b_dev = dev;
	bp->b_repcnt = -count;
	bp->b_command = com;
	bp->b_blkno = 0;
	tmstrategy(bp);
	/*
	 * In case of rewind from close, don't wait.
	 * This is the only case where count can be 0.
	 */
	if (count == 0)
		return;
	iowait(bp);
	if (bp->b_flags&B_WANTED)
		wakeup((caddr_t)bp);
	bp->b_flags &= B_ERROR;
}

/*
 * Queue a tape operation.
 */
tmstrategy(bp)
	register struct buf *bp;
{
	int teunit = TEUNIT(bp->b_dev);
	register struct uba_ctlr *um;
	register struct buf *dp;
	int s;

	/*
	 * Put transfer at end of unit queue
	 */
	dp = &teutab[teunit];
	bp->av_forw = NULL;
	s = spl5();
	um = tedinfo[teunit]->ui_mi;
	if (dp->b_actf == NULL) {
		dp->b_actf = bp;
		/*
		 * Transport not already active...
		 * put at end of controller queue.
		 */
		dp->b_forw = NULL;
		if (um->um_tab.b_actf == NULL)
			um->um_tab.b_actf = dp;
		else
			um->um_tab.b_actl->b_forw = dp;
		um->um_tab.b_actl = dp;
	} else
		dp->b_actl->av_forw = bp;
	dp->b_actl = bp;
	/*
	 * If the controller is not busy, get
	 * it going.
	 */
	if (um->um_tab.b_active == 0)
		tmstart(um);
	splx(s);
}

/*
 * Start activity on a tm controller.
 */
tmstart(um)
	register struct uba_ctlr *um;
{
	register struct buf *bp, *dp;
	register struct tmdevice *addr = (struct tmdevice *)um->um_addr;
	register struct te_softc *sc;
	register struct uba_device *ui;
	int teunit, cmd;
	daddr_t blkno;

	/*
	 * Look for an idle transport on the controller.
	 */
loop:
	if ((dp = um->um_tab.b_actf) == NULL)
		return;
	if ((bp = dp->b_actf) == NULL) {
		um->um_tab.b_actf = dp->b_forw;
		goto loop;
	}
	teunit = TEUNIT(bp->b_dev);
	ui = tedinfo[teunit];
	/*
	 * Record pre-transfer status (e.g. for TM_SENSE)
	 */
	sc = &te_softc[teunit];
	addr = (struct tmdevice *)um->um_addr;
	addr->tmcs = (ui->ui_slave << 8);
	sc->sc_dsreg = addr->tmcs;
	sc->sc_erreg = addr->tmer;
	sc->sc_resid = addr->tmbc;
	/*
	 * Default is that last command was NOT a write command;
	 * if we do a write command we will notice this in tmintr().
	 */
	sc->sc_lastiow = 0;
	if (sc->sc_openf < 0 || (addr->tmcs&TM_CUR) == 0) {
		/*
		 * Have had a hard error on a non-raw tape
		 * or the tape unit is now unavailable
		 * (e.g. taken off line).
		 */
		bp->b_flags |= B_ERROR;
		goto next;
	}
	if (bp == &ctmbuf[TMUNIT(bp->b_dev)]) {
		/*
		 * Execute control operation with the specified count.
		 */
		if (bp->b_command == TM_SENSE)
			goto next;
		/*
		 * Set next state; give 5 minutes to complete
		 * rewind, or 10 seconds per iteration (minimum 60
		 * seconds and max 5 minutes) to complete other ops.
		 */
		if (bp->b_command == TM_REW) {
			um->um_tab.b_active = SREW;
			sc->sc_timo = 5 * 60;
		} else {
			um->um_tab.b_active = SCOM;
			sc->sc_timo =
			    imin(imax(10*(int)-bp->b_repcnt,60),5*60);
		}
		if (bp->b_command == TM_SFORW || bp->b_command == TM_SREV)
			addr->tmbc = bp->b_repcnt;
		goto dobpcmd;
	}
	/*
	 * The following checks handle boundary cases for operation
	 * on non-raw tapes.  On raw tapes the initialization of
	 * sc->sc_nxrec by tmphys causes them to be skipped normally
	 * (except in the case of retries).
	 */
	if (bdbtofsb(bp->b_blkno) > sc->sc_nxrec) {
		/*
		 * Can't read past known end-of-file.
		 */
		bp->b_flags |= B_ERROR;
		bp->b_error = ENXIO;
		goto next;
	}
	if (bdbtofsb(bp->b_blkno) == sc->sc_nxrec &&
	    bp->b_flags&B_READ) {
		/*
		 * Reading at end of file returns 0 bytes.
		 */
		bp->b_resid = bp->b_bcount;
		clrbuf(bp);
		goto next;
	}
	if ((bp->b_flags&B_READ) == 0)
		/*
		 * Writing sets EOF
		 */
		sc->sc_nxrec = bdbtofsb(bp->b_blkno) + 1;
	/*
	 * If the data transfer command is in the correct place,
	 * set up all the registers except the csr, and give
	 * control over to the UNIBUS adapter routines, to
	 * wait for resources to start the i/o.
	 */
	if ((blkno = sc->sc_blkno) == bdbtofsb(bp->b_blkno)) {
		addr->tmbc = -bp->b_bcount;
		if ((bp->b_flags&B_READ) == 0) {
			if (um->um_tab.b_errcnt)
				cmd = TM_WIRG;
			else
				cmd = TM_WCOM;
		} else
			cmd = TM_RCOM;
		um->um_tab.b_active = SIO;
		um->um_cmd = sc->sc_dens|cmd;
#ifdef notdef
		if (tmreverseop(sc->sc_lastcmd))
			while (addr->tmer & TMER_SDWN)
				DELAY(10),tmgapsdcnt++;
		sc->sc_lastcmd = TM_RCOM;		/* will serve */
#endif
		sc->sc_timo = 60;	/* premature, but should serve */
		(void) ubago(ui);
		return;
	}
	/*
	 * Tape positioned incorrectly;
	 * set to seek forwards or backwards to the correct spot.
	 * This happens for raw tapes only on error retries.
	 */
	um->um_tab.b_active = SSEEK;
	if (blkno < bdbtofsb(bp->b_blkno)) {
		bp->b_command = TM_SFORW;
		addr->tmbc = blkno - bdbtofsb(bp->b_blkno);
	} else {
		bp->b_command = TM_SREV;
		addr->tmbc = bdbtofsb(bp->b_blkno) - blkno;
	}
	sc->sc_timo = imin(imax(10 * -addr->tmbc, 60), 5 * 60);
dobpcmd:
#ifdef notdef
	/*
	 * It is strictly necessary to wait for the tape
	 * to stop before changing directions, but the TC11
	 * handles this for us.
	 */
	if (tmreverseop(sc->sc_lastcmd) != tmreverseop(bp->b_command))
		while (addr->tmer & TM_SDWN)
			DELAY(10),tmgapsdcnt++;
	sc->sc_lastcmd = bp->b_command;
#endif
	/*
	 * Do the command in bp.
	 */
	addr->tmcs = (sc->sc_dens | bp->b_command);
	return;

next:
	/*
	 * Done with this operation due to error or
	 * the fact that it doesn't do anything.
	 * Release UBA resources (if any), dequeue
	 * the transfer and continue processing this slave.
	 */
	if (um->um_ubinfo)
		ubadone(um);
	um->um_tab.b_errcnt = 0;
	dp->b_actf = bp->av_forw;
	iodone(bp);
	goto loop;
}

/*
 * The UNIBUS resources we needed have been
 * allocated to us; start the device.
 */
tmdgo(um)
	register struct uba_ctlr *um;
{
	register struct tmdevice *addr = (struct tmdevice *)um->um_addr;

	addr->tmba = um->um_ubinfo;
	addr->tmcs = um->um_cmd | ((um->um_ubinfo >> 12) & 0x30);
}

/*
 * Tm interrupt routine.
 */
/*ARGSUSED*/
tmintr(tm11)
	int tm11;
{
	struct buf *dp;
	register struct buf *bp;
	register struct uba_ctlr *um = tmminfo[tm11];
	register struct tmdevice *addr;
	register struct te_softc *sc;
	int teunit;
	register state;

	if ((dp = um->um_tab.b_actf) == NULL)
		return;
	bp = dp->b_actf;
	teunit = TEUNIT(bp->b_dev);
	addr = (struct tmdevice *)tedinfo[teunit]->ui_addr;
	sc = &te_softc[teunit];
	/*
	 * If last command was a rewind, and tape is still
	 * rewinding, wait for the rewind complete interrupt.
	 */
	if (um->um_tab.b_active == SREW) {
		um->um_tab.b_active = SCOM;
		if (addr->tmer&TMER_RWS) {
			sc->sc_timo = 5*60;		/* 5 minutes */
			return;
		}
	}
	/*
	 * An operation completed... record status
	 */
	sc->sc_timo = INF;
	sc->sc_dsreg = addr->tmcs;
	sc->sc_erreg = addr->tmer;
	sc->sc_resid = addr->tmbc;
	if ((bp->b_flags & B_READ) == 0)
		sc->sc_lastiow = 1;
	state = um->um_tab.b_active;
	um->um_tab.b_active = 0;
	/*
	 * Check for errors.
	 */
	if (addr->tmcs&TM_ERR) {
		while (addr->tmer & TMER_SDWN)
			DELAY(10);			/* await settle down */
		/*
		 * If we hit the end of the tape file, update our position.
		 */
		if (addr->tmer&TMER_EOF) {
			tmseteof(bp);		/* set blkno and nxrec */
			state = SCOM;		/* force completion */
			/*
			 * Stuff bc so it will be unstuffed correctly
			 * later to get resid.
			 */
			addr->tmbc = -bp->b_bcount;
			goto opdone;
		}
		/*
		 * If we were reading raw tape and the only error was that the
		 * record was too long, then we don't consider this an error.
		 */
		if (bp == &rtmbuf[TMUNIT(bp->b_dev)] && (bp->b_flags&B_READ) &&
		    (addr->tmer&(TMER_HARD|TMER_SOFT)) == TMER_RLE)
			goto ignoreerr;
		/*
		 * If error is not hard, and this was an i/o operation
		 * retry up to 8 times.
		 */
		if ((addr->tmer&TMER_HARD)==0 && state==SIO) {
			if (++um->um_tab.b_errcnt < 7) {
				sc->sc_blkno++;
				ubadone(um);
				goto opcont;
			}
		} else
			/*
			 * Hard or non-i/o errors on non-raw tape
			 * cause it to close.
			 */
			if (sc->sc_openf>0 && bp != &rtmbuf[TMUNIT(bp->b_dev)])
				sc->sc_openf = -1;
		/*
		 * Couldn't recover error
		 */
		tprintf(sc->sc_ttyp,
		    "te%d: hard error bn%d er=%b\n", minor(bp->b_dev)&03,
		    bp->b_blkno, sc->sc_erreg, TMER_BITS);
#ifdef	AVIV
		if (tmdiag) {
			addr->tmmr = DAB;
			printf("reject code 0%o", addr->tmmr & DAB_MASK);
			addr->tmmr = DTS;
			if (addr->tmmr & DTS_MASK)
			    printf(", dead track 0%o", addr->tmmr & DTS_MASK);
			addr->tmmr = RWERR;
			printf(", read/write errors %b\n",
			    addr->tmmr & RWERR_MASK,
			    RWERR_BITS);
			addr->tmmr = DRSENSE;
			printf("drive sense %b, ", addr->tmmr & DRSENSE_MASK,
			    DRSENSE_BITS);
			printf("fsr %b\n", addr->tmfsr, FSR_BITS);
		}
#endif AVIV
		bp->b_flags |= B_ERROR;
		goto opdone;
	}
	/*
	 * Advance tape control FSM.
	 */
ignoreerr:
	switch (state) {

	case SIO:
		/*
		 * Read/write increments tape block number
		 */
		sc->sc_blkno++;
		goto opdone;

	case SCOM:
		/*
		 * For forward/backward space record update current position.
		 */
		if (bp == &ctmbuf[TMUNIT(bp->b_dev)])
		switch ((int)bp->b_command) {

		case TM_SFORW:
			sc->sc_blkno -= bp->b_repcnt;
			break;

		case TM_SREV:
			sc->sc_blkno += bp->b_repcnt;
			break;
		}
		goto opdone;

	case SSEEK:
		sc->sc_blkno = bdbtofsb(bp->b_blkno);
		goto opcont;

	default:
		panic("tmintr");
	}
opdone:
	/*
	 * Reset error count and remove
	 * from device queue.
	 */
	um->um_tab.b_errcnt = 0;
	dp->b_actf = bp->av_forw;
	/*
	 * Check resid; watch out for resid >32767 (tmbc not negative).
	 */
	bp->b_resid = ((int) -addr->tmbc) & 0xffff;
	ubadone(um);
	iodone(bp);
	/*
	 * Circulate slave to end of controller
	 * queue to give other slaves a chance.
	 */
	um->um_tab.b_actf = dp->b_forw;
	if (dp->b_actf) {
		dp->b_forw = NULL;
		if (um->um_tab.b_actf == NULL)
			um->um_tab.b_actf = dp;
		else
			um->um_tab.b_actl->b_forw = dp;
		um->um_tab.b_actl = dp;
	}
	if (um->um_tab.b_actf == 0)
		return;
opcont:
	tmstart(um);
}

tmtimer(dev)
	int dev;
{
	register struct te_softc *sc = &te_softc[TEUNIT(dev)];
	register short x;

	if (sc->sc_timo != INF && (sc->sc_timo -= 5) < 0) {
		printf("te%d: lost interrupt\n", TEUNIT(dev));
		sc->sc_timo = INF;
		x = spl5();
		tmintr(TMUNIT(dev));
		(void) splx(x);
	}
	timeout(tmtimer, (caddr_t)dev, 5*hz);
}

tmseteof(bp)
	register struct buf *bp;
{
	register int teunit = TEUNIT(bp->b_dev);
	register struct tmdevice *addr = 
	    (struct tmdevice *)tedinfo[teunit]->ui_addr;
	register struct te_softc *sc = &te_softc[teunit];

	if (bp == &ctmbuf[TMUNIT(bp->b_dev)]) {
		if (sc->sc_blkno > bdbtofsb(bp->b_blkno)) {
			/* reversing */
			sc->sc_nxrec = bdbtofsb(bp->b_blkno) - addr->tmbc;
			sc->sc_blkno = sc->sc_nxrec;
		} else {
			/* spacing forward */
			sc->sc_blkno = bdbtofsb(bp->b_blkno) + addr->tmbc;
			sc->sc_nxrec = sc->sc_blkno - 1;
		}
		return;
	} 
	/* eof on read */
	sc->sc_nxrec = bdbtofsb(bp->b_blkno);
}

tmread(dev, uio)
	dev_t dev;
	struct uio *uio;
{
	int errno;

	errno = tmphys(dev, uio);
	if (errno)
		return (errno);
	return (physio(tmstrategy, &rtmbuf[TMUNIT(dev)], dev, B_READ, minphys, uio));
}

tmwrite(dev, uio)
	dev_t dev;
	struct uio *uio;
{
	int errno;

	errno = tmphys(dev, uio);
	if (errno)
		return (errno);
	return (physio(tmstrategy, &rtmbuf[TMUNIT(dev)], dev, B_WRITE, minphys, uio));
}

/*
 * Check that a raw device exists.
 * If it does, set up sc_blkno and sc_nxrec
 * so that the tape will appear positioned correctly.
 */
tmphys(dev, uio)
	dev_t dev;
	struct uio *uio;
{
	register int teunit = TEUNIT(dev);
	register daddr_t a;
	register struct te_softc *sc;
	register struct uba_device *ui;

	if (teunit >= NTE || (ui=tedinfo[teunit]) == 0 || ui->ui_alive == 0)
		return (ENXIO);
	sc = &te_softc[teunit];
	a = bdbtofsb(uio->uio_offset >> 9);
	sc->sc_blkno = a;
	sc->sc_nxrec = a + 1;
	return (0);
}

tmreset(uban)
	int uban;
{
	register struct uba_ctlr *um;
	register tm11, teunit;
	register struct uba_device *ui;
	register struct buf *dp;

	for (tm11 = 0; tm11 < NTM; tm11++) {
		if ((um = tmminfo[tm11]) == 0 || um->um_alive == 0 ||
		   um->um_ubanum != uban)
			continue;
		printf(" tm%d", tm11);
		um->um_tab.b_active = 0;
		um->um_tab.b_actf = um->um_tab.b_actl = 0;
		if (um->um_ubinfo) {
			printf("<%d>", (um->um_ubinfo>>28)&0xf);
			um->um_ubinfo = 0;
		}
		((struct tmdevice *)(um->um_addr))->tmcs = TM_DCLR;
		for (teunit = 0; teunit < NTE; teunit++) {
			if ((ui = tedinfo[teunit]) == 0 || ui->ui_mi != um ||
			    ui->ui_alive == 0)
				continue;
			dp = &teutab[teunit];
			dp->b_active = 0;
			dp->b_forw = 0;
			if (um->um_tab.b_actf == NULL)
				um->um_tab.b_actf = dp;
			else
				um->um_tab.b_actl->b_forw = dp;
			um->um_tab.b_actl = dp;
			if (te_softc[teunit].sc_openf > 0)
				te_softc[teunit].sc_openf = -1;
		}
		tmstart(um);
	}
}

/*ARGSUSED*/
tmioctl(dev, cmd, data, flag)
	caddr_t data;
	dev_t dev;
{
	int teunit = TEUNIT(dev);
	register struct te_softc *sc = &te_softc[teunit];
	register struct buf *bp = &ctmbuf[TMUNIT(dev)];
	register callcount;
	int fcount;
	struct mtop *mtop;
	struct mtget *mtget;
	/* we depend of the values and order of the MT codes here */
	static tmops[] =
	   {TM_WEOF,TM_SFORW,TM_SREV,TM_SFORW,TM_SREV,TM_REW,TM_OFFL,TM_SENSE};

	switch (cmd) {

	case MTIOCTOP:	/* tape operation */
		mtop = (struct mtop *)data;
		switch (mtop->mt_op) {

		case MTWEOF:
			callcount = mtop->mt_count;
			fcount = 1;
			break;

		case MTFSF: case MTBSF:
			callcount = mtop->mt_count;
			fcount = INF;
			break;

		case MTFSR: case MTBSR:
			callcount = 1;
			fcount = mtop->mt_count;
			break;

		case MTREW: case MTOFFL: case MTNOP:
			callcount = 1;
			fcount = 1;
			break;

		default:
			return (ENXIO);
		}
		if (callcount <= 0 || fcount <= 0)
			return (EINVAL);
		while (--callcount >= 0) {
			tmcommand(dev, tmops[mtop->mt_op], fcount);
			if ((mtop->mt_op == MTFSR || mtop->mt_op == MTBSR) &&
			    bp->b_resid)
				return (EIO);
			if ((bp->b_flags&B_ERROR) || sc->sc_erreg&TMER_BOT)
				break;
		}
		return (geterror(bp));

	case MTIOCGET:
		mtget = (struct mtget *)data;
		mtget->mt_dsreg = sc->sc_dsreg;
		mtget->mt_erreg = sc->sc_erreg;
		mtget->mt_resid = sc->sc_resid;
		mtget->mt_type = MT_ISTM;
		break;

	default:
		return (ENXIO);
	}
	return (0);
}

#define	DBSIZE	20

tmdump()
{
	register struct uba_device *ui;
	register struct uba_regs *up;
	register struct tmdevice *addr;
	int blk, num;
	int start;

	start = 0;
	num = maxfree;
#define	phys(a,b)	((b)((int)(a)&0x7fffffff))
	if (tedinfo[0] == 0)
		return (ENXIO);
	ui = phys(tedinfo[0], struct uba_device *);
	up = phys(ui->ui_hd, struct uba_hd *)->uh_physuba;
	ubainit(up);
	DELAY(1000000);
	addr = (struct tmdevice *)ui->ui_physaddr;
	tmwait(addr);
	addr->tmcs = TM_DCLR | TM_GO;
	while (num > 0) {
		blk = num > DBSIZE ? DBSIZE : num;
		tmdwrite(start, blk, addr, up);
		start += blk;
		num -= blk;
	}
	tmeof(addr);
	tmeof(addr);
	tmwait(addr);
	if (addr->tmcs&TM_ERR)
		return (EIO);
	addr->tmcs = TM_REW | TM_GO;
	tmwait(addr);
	return (0);
}

tmdwrite(dbuf, num, addr, up)
	register dbuf, num;
	register struct tmdevice *addr;
	struct uba_regs *up;
{
	register struct pte *io;
	register int npf;

	tmwait(addr);
	io = up->uba_map;
	npf = num+1;
	while (--npf != 0)
		 *(int *)io++ = (dbuf++ | (1<<UBAMR_DPSHIFT) | UBAMR_MRV);
	*(int *)io = 0;
	addr->tmbc = -(num*NBPG);
	addr->tmba = 0;
	addr->tmcs = TM_WCOM | TM_GO;
}

tmwait(addr)
	register struct tmdevice *addr;
{
	register s;

	do
		s = addr->tmcs;
	while ((s & TM_CUR) == 0);
}

tmeof(addr)
	struct tmdevice *addr;
{

	tmwait(addr);
	addr->tmcs = TM_WEOF | TM_GO;
}
#endif
