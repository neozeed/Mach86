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
 * 5799-CGZ (C) COPYRIGHT IBM CORPORATION  1986
 * LICENSED MATERIALS - PROPERTY OF IBM
 * REFER TO COPYRIGHT INSTRUCTIONS FORM NUMBER G120-2083
 */
/* $Header: st.c,v 5.1 86/02/13 11:10:41 relyea Exp $ */
/* $Source: /ibm/acis/usr/sys_ca/caio/RCS/st.c,v $ */

/*	st.c	6.3	83/09/25	*/

#include "st.h"
#if NST > 0
/*
 * ST11/TE10 tape driver
 *
 * TODO:
 *	test driver with more than one slave
 *	test driver with more than one controller
 *	test reset code
 *	what happens if you offline tape during rewind?
 *	test using file system on tape
 */
#include "../ca/pte.h"

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/buf.h"
#include "../h/dir.h"
#include "../h/conf.h"
#include "../h/user.h"
#include "../h/file.h"
#include "../h/map.h"
#include "../h/vm.h"
#include "../h/ioctl.h"
#include "../h/mtio.h"
#include "../h/cmap.h"
#include "../h/uio.h"
#include "../h/kernel.h"
#include "../ca/debug.h"

#include "../caio/ioccvar.h"
#include "../caio/streg.h"

/* Defines to set and get the MMU TCR */
#include        "../ca/mmu.h"
caddr_t real_buf_addr();

#define spl_tape()	_spl4()		/* cpu level 4 */
#define spl_high()	_spl2()		/* at cpu level 2 */

#define ST_CTLR_TIMEOUT	16		/* 16 seconds should be enough */
#define BDBTOFSB( X )	(X)		/* tape block = 512 bytes */
#define STBLOCK_SIZE	512
#define STTIMER_INC	5		/* number of seconds per timeout */
#define STTIMER_REW	(5 * 60)	/* for tape to rewind (2 is typical) */
#define STTIMER_REOF	(10 * 60)	/* for tape to forward space file */
#define STTIMER_SEEK	(1 * 60)	/* for tape to position */
#define STTIMER_NORMAL	60		/* for normal tape operation */

#define	SHOW_ENQ	0x01
#define SHOW_DEQ	0x02
#define SHOW_INTR	0x04
#define SHOW_INIT	0x08
#define SHOW_XFER	0x10
#define SHOW_WAIT	0x20		  /* show wait */
#define SHOW_REGS	0x40
#define SHOW_ORDWR	0x80
#define SHOW_LED	0x100		  /* show unit in LEDs */
#define SHOW_COUNT	0x200		  /* show the current count information */
#define SHOW_GO		0x400		  /* show each command data */
#define SHOW_TIMER	0x800
#define SHOW_SLEEP	0x1000
#define SHOW_DATA	0x10000
#define OLD_STATUS	0x100000

/*
 * There is a cstbuf per tape controller.
 * It is used as the token to pass to the internal routines
 * to execute tape ioctls, and also acts as a lock on the slaves
 * on the controller, since there is only one per controller.
 * In particular, when the tape is rewinding on close we release
 * the user process but any further attempts to use the tape drive
 * before the rewind completes will hang waiting for cstbuf.
 */
struct	buf	cstbuf[NST];

/*
 * Raw tape operations use rstbuf.
 */
struct	buf	rstbuf[NST];

/*
 * Driver interface routines and variables.
 */
int stdebug = 0; /* streaming tape debug flag */
int	stprobe(), stslave(), stattach(), stint();
char	*stcmdprint();
struct	iocc_ctlr *stminfo[NST];
struct	iocc_device *stdinfo[NST];
struct	buf stutab[NST];
int	printf();
int	uprintf();
int	strestart();

#define STADDR	0xf00001e8
/*struct stdevice *st = (struct stdevice *) STADDR;*/
struct stdevice *staddr = (struct stdevice *) STADDR;
/* write to 0xf00006f4 to reset interrupt */
char *int_reset = (char *) 0xf00006f4;

caddr_t ststd[] = { (caddr_t) STADDR, 0 };

struct iocc_driver stcdriver = { stprobe, stslave, stattach, 0, (caddr_t *) ststd, "st", stdinfo, "stc", stminfo, stint, 2 };

/* bits in minor device */
#define	STUNIT(dev)	0
#define	T_NOREWIND	04
#define	T_1600BPI	08

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
struct	st_softc {
	short	sc_flags;	/* lock against multiple opens */
#define ST_OPEN		0x01	/* device is open */
#define ST_HARDERR	0x02	/* hard error - must close first */
#define ST_TIMERR	0x04	/* error detected via timeout */
	short	sc_lastiow;	/* last op was a write */
	daddr_t	sc_blkno;	/* block number, for block device tape in 512 byte block units */
	daddr_t	sc_nxrec;	/* position of end of tape, if known */
	u_short	sc_erreg;	/* copy of last erreg */
	u_short	sc_dsreg;	/* copy of last dsreg */
	short	sc_resid;	/* copy of last bc */
	char	sc_lastcmd;	/* last command to handle direction changes */
	u_short	sc_dens;	/* prototype command with density info */
	short	sc_timo;	/* time until timeout expires */
	short	sc_lasttimo;	/* length of time of last timeout */
	short	sc_tact;	/* timeout is active */
	caddr_t st_physaddr; /* the real memory address */
/* XXX the next line should be removed */
/*	short	*sc_baddr;	/* address to copy to */
	int	sc_blkcnt;	/* # of blocks to transfer */
	int	sc_blkbase;	/* base block # used when calculating relative 
				 * offset into file (eg. open the tape for an 
				 * operation, when the last operation didn't 
				 * rewind the tape drive 
				 */ 
	short	sc_init;	/* are we initializing the drive??? */
	short	sc_intr;	/* when we expect an reset caused interrupt */
	char	sc_data[8];	/* data bytes returned from read status */
	char	sc_fill[12];	/* fill to power of two */
} st_softc[NST];

#ifdef DEBUG
#define ST_TRACE
#define ST_MAX_TRACE (128 * 4)
char	st_tracebuff[ST_MAX_TRACE];	/* a trace buffer for debugging */
int	stsoftlen = sizeof (struct st_softc);
#endif
int	st_trace;			/* subscript into trace buffer */

/*
 * States for ic->ic_tab.b_active, the per controller state flag.
 * This is used to sequence control in the driver.
 */
#define	SSEEK	1		/* seeking */
#define	SIO	2		/* doing seq i/o */
#define	SCOM	3		/* sending control command */
#define	SREW	4		/* sending a drive rewind */
#define SWAIT	5		/* waiting for drive ready */

/*
 * Determine if there is a controller for
 * a st at address reg.  Our goal is to make the
 * device interrupt.
 */
stprobe(reg)
	caddr_t reg;
{
	DEBUGF( (stdebug&SHOW_INIT),
		printf("stprobe: entered\n"));

	return(PROBE_NOINT);
}

/*
 * Due to a design flaw, we cannot ascertain if the tape
 * exists or not unless it is on line - ie: unless a tape is
 * mounted. This is too servere a restriction to bear,
 * so all units are assumed to exist.
 */
/*ARGSUSED*/
stslave(iod, reg)
	struct iocc_device *iod;
	caddr_t reg;
{
	DEBUGF( (stdebug&SHOW_INIT),
		printf("stslave: entered\n"));

	return (1);
}

/*
 * Record attachment of the unit to the controller.
 */
/*ARGSUSED*/
stattach(iod)
	struct iocc_device *iod;
{
	DEBUGF( (stdebug&SHOW_INIT),
		printf("stattach: entered\n"));
	return (1);
}

static stdrive_init=0; /* cheap fix for multiple reset, should be put in attach */
int	sttimer();

/*
 * Open the device.  Tapes are unique open
 * devices, so we refuse if it is already open.
 * We also check that a tape is available, and
 * don't block waiting here; if you want to wait
 * for a tape you should timeout in user code.
 */
stopen(dev, flag)
	dev_t dev;
	int flag;
{
	register int stunit;
	register struct iocc_device *iod;
	register struct st_softc *sc;
	int olddens, dens;
	int s;
	register struct stdevice *addr;

	stunit = STUNIT(dev);
	DEBUGF( (stdebug&SHOW_ORDWR),
		printf("stopen: unit=%d\n",stunit));

	if (stunit>=NST || ((sc = &st_softc[stunit])->sc_flags&ST_OPEN) ||
	    (iod = stdinfo[stunit]) == 0 || iod->iod_alive == 0)
		return (ENXIO);

	addr = (struct stdevice *) iod->iod_addr;
	DEBUGF( (stdebug&SHOW_ORDWR),
		printf("stopen: after ENXIO, sc=%x  iod=%x \n",sc ,iod));


	st_trace = 0;
/* if first time thru or if exception indicated then init the adapter */
	if (!stdrive_init) {
		register int s;
		s = spl_tape();
		stinit_adapter();
		stdrive_init++;
		sc->sc_blkno = (daddr_t)0;
		splx(s);
	}

	st_rew_wait(dev);		/* wait for rewind if in progress */
if ((stdebug & OLD_STATUS) == 0) {
	s = spl_tape();
	*int_reset = 0xff;		/* reset interrupt */
	sc->sc_intr=1;			/* expect interrupt from this */
	if (stcheck_status(1) > 0) {		/* get current status */
		stinit_adapter();	/* try re-initing just in case */
		if (stcheck_status(1)>0) {
			splx(s);
			if (addr->stcs == 0x00)
				uprintf("st%d: drive turned off?\n",stunit);
			else
				uprintf("st%d: not ready\n",stunit);
			return(EIO);
		}
	}
	*int_reset = 0xff;		/* reset interrupt */
	splx(s);
} else


	{
		register int x;
		s = spl_tape();
		if (sc->sc_tact == 0) {
			sc->sc_lasttimo = sc->sc_timo = 0;	/* no timeout in progress */
			sc->sc_tact = 1;
			timeout(sttimer, (caddr_t)dev, STTIMER_INC*hz);
		}
		splx(s);
		sc->sc_flags = ST_OPEN;			/* "open" so we can issue cmds */
		x = stcommand(dev,ST_SENSE,1);  /* issue sense command */
		sc->sc_flags = 0;			/* back to non-open */
		if (x) {		/* if bad get status */
			stinit_adapter();	/* try re-initing just in case */
			if (stcheck_status(1) > 0) {
				splx(s);
				uprintf("st%d: not ready\n",stunit);
				return(EIO);
			}
		}
	}

	if ((addr->stcs&STC_EXC) || (sc->sc_data[0]&ST0_BAD)) {
		uprintf("st%d: not online\n", stunit);
		return (EIO);
	}

	if ((flag&FWRITE) && (sc->sc_data[0]&ST0_WRP)) {
		uprintf("st%d: write protected\n", stunit);
		return (EIO);
	}

	sc->sc_flags = ST_OPEN;
	/* base = absolute blkno, used for rel. offset when NOREWIND is on */
	sc->sc_blkbase = sc->sc_blkno; 
#define ST_TAPEMAX	999999
	sc->sc_nxrec = ST_TAPEMAX;
	sc->sc_lastiow = 0;
	s = spl_tape();
	if (sc->sc_tact == 0) {
		sc->sc_lasttimo = sc->sc_timo = 0;	/* no timeout in progress */
		sc->sc_tact = 1;
		timeout(sttimer, (caddr_t)dev, STTIMER_INC*hz);
	}
	splx(s);
	DEBUGF((stdebug&SHOW_ORDWR), printf("stopen done.\n"));
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
stclose(dev, flag)
	register dev_t dev;
	register flag;
{
	register struct st_softc *sc = &st_softc[STUNIT(dev)];
	DEBUGF( (stdebug&SHOW_ORDWR),
		printf("stclose: entered\n"));

	if (flag == FWRITE || (flag&FWRITE) && sc->sc_lastiow) {
		DEBUGF( (stdebug&SHOW_ORDWR),
			printf("stclose: writing 2 EOF\n"));
		
		stcommand(dev, ST_WEOF, 2);
	}
	if ((minor(dev)&T_NOREWIND) == 0) {
		/*
		 * 0 count means don't hang waiting for rewind complete
		 * rather cstbuf stays busy until the operation completes
		 * preventing further opens from completing by
		 * preventing a ST_SENSE from completing.
		 */
		DEBUGF( (stdebug&SHOW_ORDWR),
			printf("stclose: rewinding before close\n"));
		stcommand(dev, ST_TERMINATE, 0);
	}
	sc->sc_flags = 0;
}

/*
 * Execute a command on the tape drive
 * a specified number of times.
 * result  = 0	all ok
 * result != 0	error flag set
 */
stcommand(dev, com, count)
	dev_t dev;
	int com, count;
{
	register struct buf *bp;
	register int s;
	register struct st_softc *sc = &st_softc[STUNIT(dev)];
	register int result = 0;

	DEBUGF( (stdebug&SHOW_ENQ),
		printf("***stcommand: cmd=%x (%s) count=%x\n",com,
			stcmdprint(com), count));

	if (sc->sc_flags & ST_HARDERR)
		return(1);			/* hard error - just return */
	bp = &cstbuf[STUNIT(dev)];
	s = spl_tape();
	while (bp->b_flags&B_BUSY) {
		/*
		 * This special check is because B_BUSY never
		 * gets cleared in the non-waiting rewind case.
		 */
		if (bp->b_repcnt == 0 && (bp->b_flags&B_DONE))
			break;
		bp->b_flags |= B_WANTED;
		DEBUGF( (stdebug&SHOW_SLEEP),
			printf("***stcommand: sleeping\n"));
		sleep((caddr_t)bp, PRIBIO);
	}
	if (sc->sc_flags & ST_HARDERR) {
		splx(s);
		return(1);			/* hard error - just return */
	}
	bp->b_flags = B_BUSY|B_READ;
	splx(s);
	bp->b_dev = dev;
	bp->b_repcnt = -count;
	bp->b_bcount = count;
	bp->b_command = com;
	bp->b_blkno = 0;
	ststrategy(bp);
	/*
	 * In case of rewind from close, don't wait.
	 * This is the only case where count can be 0.
	 */
	if (count == 0)
		return(0);			/* pretend we are ok */
	iowait(bp);
	if (bp->b_flags&B_WANTED)
		wakeup((caddr_t)bp);
	result = bp->b_flags & B_ERROR;
	bp->b_flags &= B_ERROR;
/*	if (result)
		stextendstatus(uprintf);	/* print extended status */
	return(result);
}

/*
 * Queue a tape operation.
 */
ststrategy(bp)
	register struct buf *bp;
{
	register int stunit = STUNIT(bp->b_dev);
	register struct iocc_ctlr *ic;
	register struct buf *dp;
	register int s;

	DEBUGF( (stdebug&SHOW_ENQ),
		printf("ststrategy: entered  bp=0x%x\n",bp));

	/*
	 * Put transfer at end of unit queue
	 */
	s = spl_tape();
	dp = &stutab[STUNIT(bp->b_dev)];
	bp->av_forw = NULL;
	ic = stdinfo[STUNIT(bp->b_dev)]->iod_mi;
	if (dp->b_actf == NULL) {
		dp->b_actf = bp;
		/*
		 * Transport not already active...
		 * put at end of controller queue.
		 */
		dp->b_forw = NULL;
		if (ic->ic_tab.b_actf == NULL)
			ic->ic_tab.b_actf = dp;
		else
			ic->ic_tab.b_actl->b_forw = dp;
		ic->ic_tab.b_actl = dp;
	} else {
		dp->b_actl->av_forw = bp;

		DEBUGF( (stdebug&SHOW_ENQ),
		printf("ststrategy: request queued --- device busy\n"));
	}
	dp->b_actl = bp;
	/*
	 * If the controller is not busy, get
	 * it going.
	 */

	if (ic->ic_tab.b_active == 0)
		ststart(ic);

	DEBUGF( (stdebug&SHOW_ENQ),
		printf("ststrategy: exiting\n"));
	splx(s);
}

/*
 * Start activity on a st controller.
 */
ststart(ic)
	register struct iocc_ctlr *ic;
{
	register struct buf *bp, *dp;
/*	register struct stdevice *addr = (struct stdevice *)ic->ic_addr;*/
	register struct stdevice *addr = (struct stdevice *) staddr;
	register struct st_softc *sc;
	register struct iocc_device *iod;
	int stunit, cmd;
	int s;
	int x;
	daddr_t blkno;
	DEBUGF( (stdebug&SHOW_DEQ),
		printf("ststart: staddr=0x%x  ic_addr=0x%x\n",staddr,ic->ic_addr));

	/*
	 * Look for an idle transport on the controller.
	 */
loop:
	if ((dp = ic->ic_tab.b_actf) == NULL)
		return;
	if ((bp = dp->b_actf) == NULL) {
		ic->ic_tab.b_actf = dp->b_forw;
		goto loop;
	}
	DEBUGF( (stdebug&SHOW_DEQ),
		printf(" with bp->cmd=0x%x (%s)  B_READ=%d  bp=0x%x\n",bp->b_command, stcmdprint(bp->b_command), (bp->b_flags&B_READ),bp));

	stunit = STUNIT(bp->b_dev);

	/* initialization needed for STUNLOAD */
#define NO_ADDR 0			  /* flag for no real address yet */
	st_softc[0].st_physaddr = NO_ADDR; /* mark as not translated */
	dp->b_un.b_addr = bp->b_un.b_addr;	/* set current address */



	iod = stdinfo[STUNIT(bp->b_dev)];
	/*
	 * Record pre-transfer status (e.g. for ST_SENSE)
	 */
	sc = &st_softc[STUNIT(bp->b_dev)];
	addr = (struct stdevice *)ic->ic_addr;

	/*
	 * Default is that last command was NOT a write command;
	 * if we do a write command we will notice this in stint().
	 */
	sc->sc_lastiow = 0;
	if (sc->sc_flags & ST_HARDERR) {
		/*
		 * Have had a hard error on a non-raw tape
		 * or the tape unit is now unavailable
		 * (e.g. taken off line).
		 */
		bp->b_flags |= B_ERROR;
		DEBUGF( (stdebug&SHOW_DEQ),
			printf("ststart: hard error - dequeuing\n"));
		goto next;
	}

	if (stbusy()) /* is device busy? */
		goto notready;
	if (addr->stcs == STC_ONL && stready(1))
		goto notready;		/* try again a bit later if only not ready */
	if ((x = stcheck_status(0))) {
		if (bp->b_blkno+sc->sc_blkbase <= 1) {
			printf("st%d: stcheck_status failed: 0x%b (prob. blank tape)\n",stunit,addr->stcs,STCFMT);
			if (x < 0)
				stextendstatus(printf);	/* print extended status */
		}
		else if ((sc->sc_data[0] & ST0_FIL) == 0) {
			printf("st%d: stcheck_status failed: 0x%b\n",stunit,addr->stcs,STCFMT);
			if (x < 0)
				stextendstatus(printf);	/* print extended status */
		}
		else if (bp->b_flags&B_READ) {
			/*
			 * Reading at end of file returns 0 bytes.
			 * XXX - should consume tape mark with read tape mark 
			 */
			bp->b_resid = bp->b_bcount;
			clrbuf(bp);
			DEBUGF( (stdebug&SHOW_DEQ),
				printf("ststart: read TM resid=%d\n",bp->b_resid));
			goto next;
		}
		sc->sc_flags |= ST_HARDERR;
		bp->b_flags |= B_ERROR;
		stbrk();
		goto next;
	}

	if (stready(1))
		goto notready;


	/* control commands */
	if (bp == &cstbuf[STUNIT(bp->b_dev)]) {
		DEBUGF( (stdebug&SHOW_DEQ),
			printf("ststart: reached start1\n"));
		/*
		 * Execute control operation with the specified count.
		 */
		ic->ic_tab.b_active = SCOM;	/* default value */
		sc->sc_lasttimo = sc->sc_timo = STTIMER_NORMAL;
		/*
		 * Set next state; give time to complete
		 * rewind, or 10 seconds per iteration (minimum 60
		 * seconds and max 8 minutes) to complete other ops.
		 */
		switch(bp->b_command) {
		case ST_SENSE:
			DEBUGF( (stdebug&SHOW_DEQ),
				printf("ststart: sense\n"));
			break;
		case ST_REW:
		case ST_RETENSION:
		case ST_ERASE:
			ic->ic_tab.b_active = SREW;
			sc->sc_lasttimo = sc->sc_timo = STTIMER_REW;
			DEBUGF( (stdebug&SHOW_ENQ),
				printf("ststart: ST_REW\n"));
			break;
		case ST_SFORW:
			sc->sc_blkcnt = bp->b_bcount;
			sc->sc_lasttimo = sc->sc_timo = STTIMER_SEEK;
			break;
		case ST_REOF:
			sc->sc_blkcnt = bp->b_bcount;
			sc->sc_lasttimo = sc->sc_timo = STTIMER_REOF;
			break;
 		/* if TERMINATING then reset counters */
		case ST_TERMINATE:
 			sc->sc_blkcnt = bp->b_bcount;
 			sc->sc_blkcnt = 0;
 			sc->sc_blkno = 0;
			break;
		/* if Writing EOF's, then load the number of EOF's to write */
		case ST_WEOF:
			sc->sc_blkcnt = bp->b_bcount;
			break;
		}
		goto dobpcmd;
	}


 
	/*
	 * The following checks handle boundary cases for operation
	 * on non-raw tapes.  On raw tapes the initialization of
	 * sc->sc_nxrec by stphys causes them to be skipped normally
	 * (except in the case of retries).
	 */
	if (BDBTOFSB(bp->b_blkno+sc->sc_blkbase) > sc->sc_nxrec) {
		/*
		 * Can't read past known end-of-file.
		 */
		bp->b_flags |= B_ERROR;
		bp->b_error = ENXIO;
		DEBUGF( (stdebug&SHOW_DEQ),
			printf("ststart: can't read past EOF"));
		goto next;
	}
	if (BDBTOFSB(bp->b_blkno+sc->sc_blkbase) == sc->sc_nxrec &&
	    bp->b_flags&B_READ) {
		/*
		 * Reading at end of file returns 0 bytes.
		 */
		bp->b_resid = bp->b_bcount;
		clrbuf(bp);
		DEBUGF( (stdebug&SHOW_DEQ),
			printf("ststart: reading past EOF returns 0 bytes"));
		goto next;
	}
	if ((bp->b_flags&B_READ) == 0)
		/*
		 * Writing sets EOF
		 */
		sc->sc_nxrec = BDBTOFSB(bp->b_blkno+sc->sc_blkbase) + bp->b_bcount / STBLOCK_SIZE;
	/*
	 * If the data transfer command is in the correct place,
	 * set up all the adapter register and start the operation.
	 */
		DEBUGF( (stdebug&SHOW_ENQ), 
			printf("ststart: reached start2.9  blkno=0x%x  BDBTOFSB()=%x\n",sc->sc_blkno, BDBTOFSB(bp->b_blkno+sc->sc_blkbase)));

	if (BDBTOFSB(blkno = sc->sc_blkno) == BDBTOFSB(bp->b_blkno+sc->sc_blkbase)) {
	/*	addr->stbc = -bp->b_bcount;*/
		if ((bp->b_flags&B_READ) == 0) {
			cmd = ST_WCOM;
			DEBUGF( (stdebug&SHOW_ENQ),
				printf("ststart: ST_WCOM\n"));
		} else {
			cmd = ST_RCOM;
			DEBUGF( (stdebug&SHOW_ENQ),
				printf("ststart: ST_RCOM\n"));
		}
		ic->ic_tab.b_active = SIO;
		sc->sc_lasttimo = sc->sc_timo = STTIMER_SEEK;	/* premature, but should serve */

#ifdef UNDEF
		/* 
		 * if not the first command (ie blkno != 0)
		 * if last command is not the same as this command you must 
		 * terminate 
		 */
		if ((((sc->sc_lastcmd == ST_WCOM) && (cmd == ST_RCOM)) ||
			((sc->sc_lastcmd == ST_RCOM) && (cmd == ST_WCOM))) &&
			(sc->sc_blkno > 0)) {

			DEBUGF( (stdebug>0), {
				printf("ststart: Previous command isn't the same as this command, so rewind\n");
				printf("  lastcmd=0x%x  curcmd=0x%x  at blkno=0x%x\n",sc->sc_lastcmd,cmd,sc->sc_blkno);
			});
			
			sc->sc_lasttimo = sc->sc_timo = STTIMER_REW;
			sc->sc_lastcmd = ST_TERMINATE;
			sc->sc_blkcnt = 0;

			goto docmd;
		}
#endif

		/* I could put the blkcnt in the LD_COUNT part of STINT */
		sc->sc_blkcnt = bp->b_bcount / STBLOCK_SIZE;
		sc->sc_lastcmd = cmd;

		DEBUGF( (stdebug&SHOW_DEQ),
			printf("ststart: start3 blkcnt=0x%x lastcmd=0x%x blkno=0x%x  BDBTOFSB()=%x\n",sc->sc_blkcnt,sc->sc_lastcmd, blkno, BDBTOFSB(bp->b_blkno+sc->sc_blkbase)));

		sc->sc_lasttimo = sc->sc_timo = STTIMER_NORMAL;

		goto docmd;	/* issue the command */
	}
	/*
	 * Tape positioned incorrectly;
	 * set to seek forwards or backwards (rewind + forw) to the correct spot.
	 * This happens for raw tapes only on error retries.
	 */
	ic->ic_tab.b_active = SSEEK;
	/*sc->sc_timo = imin(imax(10 * -addr->stbc, 60), 5 * 60);*/

	if (BDBTOFSB(blkno) < BDBTOFSB(bp->b_blkno+sc->sc_blkbase)) {
		bp->b_command = ST_SFORW;
		sc->sc_blkcnt = bp->b_blkno+sc->sc_blkbase - sc->sc_blkno;
		sc->sc_lasttimo = sc->sc_timo = STTIMER_SEEK;

		DEBUGF( (stdebug&SHOW_DEQ),
			printf("ststart: ST_SFORW for %d (dec) blocks, blkno=%d  BDBTOFSB=%d\n", blkno - BDBTOFSB(bp->b_blkno+sc->sc_blkbase), blkno, bp->b_blkno+sc->sc_blkbase));
	} else {
		bp->b_command = ST_TERMINATE;
		sc->sc_lasttimo = sc->sc_timo = STTIMER_REW;
		sc->sc_blkcnt = 0;
		sc->sc_blkno = 0;

		DEBUGF( (stdebug&SHOW_DEQ),
			printf("ststart: ST_REW for %d (dec) blocks, blkno=%d  BDBTOFSB=%d\n", blkno - BDBTOFSB(bp->b_blkno+sc->sc_blkbase), blkno, BDBTOFSB(bp->b_blkno+sc->sc_blkbase)));
	}

dobpcmd:
	/*
	 * Do the command in bp->b_command.
	 * then fall thru to normal case.
	 */
	sc->sc_lastcmd = bp->b_command;
	/*
	 * do the command in sc->sc_lastcmd.
	 */
docmd:
	/* we can only skip 256 sectors at one time */
	if (sc->sc_blkcnt > 255)
		sc->sc_blkcnt = 256; /* 0x0 actually skips 256 sectors*/

	DEBUGF( (stdebug&SHOW_ENQ),
		printf("ststart: cmd=0x%x (%s) b_count=0x%x\n",sc->sc_lastcmd,stcmdprint(sc->sc_lastcmd),bp->b_bcount));

	if (stready(1)) /* device ready? */
		goto notready;

	addr->stcmd = sc->sc_lastcmd;

doint:
	return;

next:
	DEBUGF( (stdebug&SHOW_DEQ),
		printf("ststart: at next label\n"));
	/*
	 * Done with this operation due to error or
	 * the fact that it doesn't do anything.
	 * Dequeue the transfer and continue processing this slave.
	 */
	ic->ic_tab.b_errcnt = 0;
	dp->b_actf = bp->av_forw;
	iodone(bp);
	sc->sc_timo = 0;		/* kill timeout */
	ic->ic_tab.b_active = 0;		/* not active */
	goto loop;

/*
 * come here if the tape controller is not ready. 
 * bump an error counter and if it is not ready after several retries then
 * we will give up and indicate an error
 * this mechanism is used because although the tape drive has completed
 * the previous operation it is not ready to accept the next operation
 * (as indicated by READY), in many cases, until over a second later!
 */
notready:
	if (++ic->ic_tab.b_errcnt > ST_CTLR_TIMEOUT * 16) {
		printf("st0: controller not ready\n");
		sc->sc_flags |= ST_HARDERR;
		bp->b_flags |= B_ERROR;
		ic->ic_tab.b_active = 0;		/* not active */
		goto next;
	}
	DEBUGF(stdebug&SHOW_TIMER, printf("st0: not ready - timeout started\n"));
	timeout(strestart, ic, hz / 16);	/* check again in 1/16 second */
	ic->ic_tab.b_active = SWAIT;		/* waiting for ready */
	return;
}

strestart(ic)
	register struct iocc_ctlr *ic;
{
	register int s = spl_tape();

	DEBUGF(stdebug & SHOW_TIMER,
		printf("strestart: calling ststart\n"));
	ststart(ic);
	splx(s);
}

/*
 * ST interrupt routine.
 */
stint(stnum)
	int stnum;
{
	struct buf *dp;
	int stunit = 0;
	register struct buf *bp;
/*	register struct iocc_ctlr *ic = stminfo[stnum];*/
/*XXX hardcoded to zero (only one tape adapter per machine */
	register struct iocc_ctlr *ic = stminfo[0];
	register struct stdevice *addr = (struct stdevice *)stdinfo[0]->iod_addr;
	register struct st_softc *sc = &st_softc[0];
	register state;
	char rc,status;
	int i, s;

	DEBUGF( (stdebug&SHOW_INTR),
		printf("stint: entered status=0x%b\n",addr->stcs,STCFMT));

	/* following check on sc_intr may result in occasional stealing of 
	 * interrupts from other devices - but leaving it out results in
	 * stray interrupt messages 
	 */
	if ((addr->stcs & STC_INT) == 0 && stnum == 0) {
		if (sc->sc_intr) {
			sc->sc_intr = 0;	/* reset interrupt expected */
			*int_reset = 0xff;
			return(0);
		}
		return(1);		/* not our's */
	}
	sc->sc_intr = 0;

	if ((dp = ic->ic_tab.b_actf) == NULL) {
		DEBUGF( (stdebug&SHOW_INTR),
			printf("stint: nothing active\n"));
		return(0);
	}
	bp = dp->b_actf;
	DEBUGF( (stdebug&SHOW_INTR),
		printf("stint:bp=0x%x\n",bp));


	/* if unrecoverable error has occurred then dequeue */
	if (sc->sc_flags & ST_HARDERR) {
		bp->b_flags |= B_ERROR;
		goto opdone;
	}

	/*
	 * If last command was a rewind, and tape is still
	 * rewinding, wait for the rewind complete interrupt.
	 */
	if (ic->ic_tab.b_active == SREW) {
#ifdef UNDEF
		ic->ic_tab.b_active = SCOM;
#endif
		if (addr->stcs & STC_BSY) {
			DEBUGF((stdebug & SHOW_INTR),
				printf("busy rewinding\n"));
			sc->sc_lasttimo = sc->sc_timo = STTIMER_REW;		/* 5 minutes */
			return(0);
		}
	}

process_int:
	DEBUGF( (stdebug&SHOW_INTR),
		printf("stint: process_int entered\n "));

	/* rc of 0x31 means end of read, 0x00 end of write */
	status=addr->stcs&0xff;
	rc=addr->stdata&0xff;
#ifdef ST_TRACE
	{ register int t = st_trace;
	st_tracebuff[t] = sc->sc_lastcmd;
	st_tracebuff[t+1] = status;
	st_tracebuff[t+2] = rc;
	st_tracebuff[t+3] = time.tv_sec;
	st_trace = (t + 4) & (ST_MAX_TRACE - 1);
	}
#endif
	DEBUGF( (stdebug&SHOW_INTR),
		printf("stint: CASE stmt. rc=0x%x status=0x%b state=%d\n",rc,status,STCFMT,ic->ic_tab.b_active));

	switch ( rc ) {
		case CC_DONE:
			goto opdoop;	/* done this operation - see what is next */
			break;
		case CC_LD_COUNT:
			/* now pass the count (in blocks) of data to transfer */
			addr->stdata = sc->sc_blkcnt;
			DEBUGF( (stdebug&SHOW_COUNT),
				printf("stint: CC_LD_COUNT =0x%x\n",sc->sc_blkcnt));

			if (sc->sc_lastcmd == ST_WCOM) {
				stunload(bp, dp, 0, staddr);
				sc->sc_blkno++;
			}
			break;
		case CC_POR_AA:
			addr->stcmd = ST_CMD_AA; /* hand shake */
			break;
		case CC_POR:
			stbusy(); 			/* is device busy? */
			addr->stcmd = ST_SENSE;		/* read status */
			break;
		case CC_RD_STATUS:
			stget_status();
			goto opdoop;
		case CC_READ: 
			stunload(bp, dp, 0, staddr);
			sc->sc_blkno++;
			sc->sc_timo = sc->sc_lasttimo;	/* reset timeout */

			break;
		case CC_READ_LAST:
			stunload(bp, dp, 0, staddr);
			sc->sc_blkno++;

			goto opdoop;
		case CC_WRITE:
			stunload(bp, dp, 0, staddr);
			sc->sc_blkno++;
			sc->sc_timo = sc->sc_lasttimo;	/* reset timeout */

			break;
		/* for now any error does us in - later will do retries */
		case CC_EXCEPTION:
		case CC_CMD_REJ_EXCEPT:
			
			stprintstatus("exception ",status);
			stcheck_status(1);
			stextendstatus(printf); /* extended status */
			sc->sc_flags |= ST_HARDERR;
			bp->b_flags |= B_ERROR;
			goto opdone;
		default:
			DEBUGF( (stdebug&SHOW_INTR),
				printf("stint: DEFAULT2 read unknown condition code  rc=0x%x\n",rc));
			/* initialize tape adapter - do handshake */
			streset(addr);
			/*stinit_adapter();*/
			break;
		}
		goto opreset; /* reset interrupt and continue */
opdoop:

	if (sc->sc_init == 1) {
		printf("stint:<*****> received interrupt during initialization\n");
		goto opreset; /* reset interrupt and continue */
	}


	/*
	 * An operation completed... record status
	 */
	sc->sc_timo = 0;
	if ((bp->b_flags & B_READ) == 0)
		sc->sc_lastiow = 1;
	state = ic->ic_tab.b_active;
	ic->ic_tab.b_active = 0;
	/*
	 * Check for errors.
	 */
#ifdef TOBEDONE
	if (addr->stcs&ST_ERR) {
		while (addr->ster & STER_SDWN)
			;			/* await settle down */
		/*
		 * If we hit the end of the tape file, update our position.
		 */
		if (addr->ster&STER_EOF) {
			stseteof(bp);		/* set blkno and nxrec */
			state = SCOM;		/* force completion */
			/*
			 * Stuff bc so it will be unstuffed correctly
			 * later to get resid.
			 */
			addr->stbc = -bp->b_bcount;
			goto opdone;
		}
		/*
		 * If we were reading raw tape and the only error was that the
		 * record was too long, then we don't consider this an error.
		 */
		if (bp == &rstbuf[STUNIT(bp->b_dev)] && (bp->b_flags&B_READ) &&
		    (addr->ster&(STER_HARD|STER_SOFT)) == STER_RLE)
			goto ignoreerr;
		/*
		 * If error is not hard, and this was an i/o operation
		 * retry up to 8 times.
		 */
		if ((addr->ster&STER_HARD)==0 && state==SIO) {
			if (++ic->ic_tab.b_errcnt < 7) {
				sc->sc_blkno++;
			/*	ubadone(ic);*/
				goto opcont;
			}
		} else
			/*
			 * Hard or non-i/o errors on non-raw tape
			 * cause it to close.
			 */
			if ((sc->sc_flags&ST_HARDERR) == 0 && bp != &rstbuf[STUNIT(bp->b_dev)])
				sc->sc_flags |= ST_HARDERR;
		/*
		 * Couldn't recover error
		 */
		printf("te%d: hard error bn%d er=%b\n", minor(bp->b_dev)&03,
		    bp->b_blkno+sc->sc_blkbase, sc->sc_erreg, STER_BITS);
		bp->b_flags |= B_ERROR;
		goto opdone;
	}
#endif
	/*
	 * Advance tape control FSM.
	 */
ignoreerr:
	switch (state) {

	case SIO:
		/*
		 * Read/write increments tape block number
		 */
		/*sc->sc_blkno++;*/ /* done at each read/write of 512 bytes */
		DEBUGF( (stdebug&SHOW_INTR),
			printf("stint: SIO (read/write) case stmt. sc_blkno=0x%x\n",sc->sc_blkno));

		goto opdone;

	case SCOM:
		/*
		 * For forward/backward space record update current position.
		 */
		DEBUGF( (stdebug&SHOW_INTR),
			printf("stint: SCOM case stmt. "));
		if (bp == &cstbuf[STUNIT(bp->b_dev)])
		switch (bp->b_command) {

		case ST_SFORW:
			sc->sc_blkno += sc->sc_blkcnt;
			if ((bp->b_bcount -= sc->sc_blkcnt) > 0)
				goto opcont;		/* more to do */
			break;
		}
		goto opdone;

	case SSEEK:
		/* this will assign sc_blkno the 512 byte block number */
		sc->sc_blkno += sc->sc_blkcnt;
		DEBUGF( (stdebug&SHOW_INTR),
			printf("stint: SSEEK case stmt.  sc_blkno=0x%x\n",sc->sc_blkno));
		goto opcont;
		break;

	default:
		DEBUGF( (stdebug&SHOW_INTR),
			printf("stint: DEFAULT1 case stmt.=0x%x\n ",state));
		/*panic("stint");*/
	}
			
opdone:
	/*
	 * Reset error count and remove
	 * from device queue.
	 */
	 sc->sc_timo = 0;	/* cancel timer */
	ic->ic_tab.b_errcnt = 0;
	ic->ic_tab.b_active = 0;	/* no longer active */
	dp->b_actf = bp->av_forw;
	/*
	 * Check resid; watch out for resid >32767 (stbc not negative).
	 */
/*XXX	bp->b_resid = ((int) -addr->stbc) & 0xffff;*/
	bp->b_resid = 0;/* transfer complete */
/*	ubadone(ic);*/
	iodone(bp);
	/*
	 * Circulate slave to end of controller
	 * queue to give other slaves a chance.
	 */
	ic->ic_tab.b_actf = dp->b_forw;
	if (dp->b_actf) {
		dp->b_forw = NULL;
		if (ic->ic_tab.b_actf == NULL)
			ic->ic_tab.b_actf = dp;
		else
			ic->ic_tab.b_actl->b_forw = dp;
		ic->ic_tab.b_actl = dp;
	}
	if (ic->ic_tab.b_actf == 0) {
		/* reset interrupt??? */
		*int_reset = 0xff;
		DEBUGF( (stdebug&SHOW_INTR),
			printf("stint: exiting at opdone\n",rc));
		return(0);
	}
opcont:
	sc->sc_timo = 0;		/* kill timeout */
	ststart(ic);
opreset:

	/* reset interrupt??? */
	*int_reset = 0xff;
	DEBUGF( (stdebug&SHOW_INTR),
		printf("stint: exiting at opcont\n",rc));
	return(0);
}

/* is the device busy?  Wait until it's not. */
int stbusydelay = 1999;
stbusy()
{
	int timeout;
	
	/* wait for busy bit to go away */
	timeout = stbusydelay;
	while (((staddr->stcs&STC_BSY)) && (--timeout)) 
		delay(1);
	if (!timeout)
		printf("stbusy: timeout occurred DEVICE still BUSY!!\n");

	return(!timeout);
}

/* is the device ready?  Wait until it is ready.
 * return 0 if ready (not timed out)
 */
int streadydelay = 99;		/* will delay for about 1ms in flag=1 case */
stready(flag)
{
	register int timeout;
	/* wait for ready bit to come on */
	timeout = streadydelay;

	if (!flag)
		timeout <<= 7;		/* use longer timeout - must wait */
	if ((staddr->stcs & STC_EXC|STC_RDY) == STC_EXC) {
		DEBUGF((stdebug&SHOW_REGS), 
			printf("stready: status=0x%b\n",staddr->stcs,STCFMT));
		return(1);	/* cannot be ready if exception */
	}
	/* wait for ready */
	while (((staddr->stcs&STC_RDY) == 0) && (--timeout))
		stdelay();	/* check every 10 usec */

	DEBUGF((stdebug&SHOW_REGS), 
		printf("stready: status=0x%b\n",staddr->stcs,STCFMT));

	if (!timeout && !flag)
		printf("st0: timeout occurred DEVICE still NOT READY!!\n");

	return(!timeout);
}

stdelay()
{
	DELAY(10);	/* check every 10 usec */
}


sttransblk(direction, buf_addr)
char direction;
/*short *buf_addr;	/* address to copy from/to */
int *buf_addr;	/* address to copy from/to */
{
	int i, s, timeout;
	char rc;
	register struct stdevice *addr;
	register struct st_softc *sc;
	register int *sector_window = (int *) STADDR;

	addr = (struct stdevice *) stdinfo[0]->iod_addr;

	/*stbusy(); /* is device busy? */
	delay(1);

	DEBUGF( (stdebug&SHOW_COUNT),
		printf("sttransblk:  dir=0x%x  addr=0x%x  secaddr=0x%x\n",direction,buf_addr,sector_window));

	if (direction == ST_RCOM) {

		DEBUGF( (stdebug&SHOW_COUNT),
			printf("sttransblk:ST_RCOM  dir=0x%x \n",direction));

		s = spl_tape();
		SET_VR0;
		/* do one block transfer */
		/* replace with fast loop */
		for (i = STBLOCK_SIZE/4 ; i ;  i--) {
			/* *buf_addr++ = addr->stsec;*/
			*buf_addr++ = *sector_window;
		}
		CLR_VR0;
		splx(s);
		/*if (rc == CC_READ_LAST)
			goto opdone;*/

	} else if (direction == ST_WCOM) {

		DEBUGF( (stdebug&SHOW_COUNT),
			printf("sttransblk:ST_WCOM  dir=0x%x \n",direction));
		s = spl_tape();
		SET_VR0;
		/* do one block transfer */
		/* replace with fast loop */
		for (i = STBLOCK_SIZE/4 ; i ;  i--) {
			/* addr->stsec = *buf_addr++;*/
			*sector_window = *buf_addr++;
		}
		CLR_VR0;
		splx(s);
	} else
		DEBUGF( (stdebug&SHOW_COUNT),
			printf("sttransblk:ERROR no direction chosen! dir=0x%x \n",direction));

}




sttimer(dev)
	int dev;
{
	register struct st_softc *sc = &st_softc[STUNIT(dev)];
	register short x;
	register struct iocc_ctlr *ic;

	if (sc->sc_timo > 0 && (sc->sc_timo -= STTIMER_INC) <= 0) {
		printf("st%d: lost interrupt - last cmd was 0x%x(%s)\n", STUNIT(dev),sc->sc_lastcmd,stcmdprint(sc->sc_lastcmd));
		sc->sc_timo = sc->sc_lasttimo;		/* reset the timer */
		x = spl_tape();
		ic = stdinfo[STUNIT(dev)]->iod_mi;
		/* set error flag */
	/*XXX*/	sc->sc_flags |= ST_HARDERR | ST_TIMERR;
		stint(-1);				/* flag as timer interrupt */
		(void) splx(x);
	}

	if ((sc->sc_flags&ST_OPEN) == 0 && (stutab[0].b_active == 0)) 
		sc->sc_tact = 0; 	/* timer not active */
	else
		timeout(sttimer, (caddr_t)dev, STTIMER_INC*hz);
}

#ifdef TOBEDONE
stseteof(bp)
	register struct buf *bp;
{
	register int stunit = STUNIT(bp->b_dev);
	register struct stdevice *addr =
	    (struct stdevice *)stdinfo[stunit]->iod_addr;
	register struct st_softc *sc = &st_softc[stunit];
	DEBUGF( (stdebug>8),
		printf("stseteof: entered\n"));

	if (bp == &cstbuf[STUNIT(bp->b_dev)]) {
		if (sc->sc_blkno > BDBTOFSB(bp->b_blkno+sc->sc_blkbase)) {
			/* reversing */
			sc->sc_nxrec = BDBTOFSB(bp->b_blkno+sc->sc_blkbase) - addr->stbc;
			sc->sc_blkno = sc->sc_nxrec;
		} else {
			/* spacing forward */
			sc->sc_blkno = BDBTOFSB(bp->b_blkno+sc->sc_blkbase) + addr->stbc;
			sc->sc_nxrec = sc->sc_blkno - 1;
		}
		return;
	}
	/* eof on read */
	sc->sc_nxrec = BDBTOFSB(bp->b_blkno+sc->sc_blkbase);
}
#endif

stread(dev, uio)
	dev_t dev;
	struct uio *uio;
{
	int errno;
	DEBUGF( (stdebug&SHOW_ORDWR),
		printf("stread: entered\n"));

	errno = stphys(dev, uio);
	if (errno)
		return (errno);
	return (physio(ststrategy, &rstbuf[STUNIT(dev)], dev, B_READ, minphys, uio));
}

stwrite(dev, uio)
	dev_t dev;
	struct uio *uio;
{
	int errno;
	DEBUGF( (stdebug&SHOW_ORDWR),
		printf("stwrite: entered\n"));

	errno = stphys(dev, uio);
	if (errno)
		return (errno);
	return (physio(ststrategy, &rstbuf[STUNIT(dev)], dev, B_WRITE, minphys, uio));
}

/*
 * Check that a raw device exists.
 * If it does, set up sc_blkno and sc_nxrec
 * so that the tape will appear positioned correctly.
 */
stphys(dev, uio)
	dev_t dev;
	struct uio *uio;
{
	register int stunit = STUNIT(dev);
	register daddr_t a;
	register struct st_softc *sc;
	register struct iocc_device *iod;
	DEBUGF( (stdebug&SHOW_ORDWR),
		printf("stphys: entered\n"));

	if (stunit >= NST || (iod=stdinfo[stunit]) == 0 || iod->iod_alive == 0)
		return (ENXIO);
	sc = &st_softc[stunit];
	a = BDBTOFSB(uio->uio_offset >> 9);
	sc->sc_blkno = a;
	sc->sc_nxrec = a + 1;
	return (0);
}

/*ARGSUSED*/
stioctl(dev, cmd, data, flag)
	caddr_t data;
	dev_t dev;
{
	int stunit = STUNIT(dev);
	register struct st_softc *sc = &st_softc[stunit];
	register struct buf *bp = &cstbuf[STUNIT(dev)];
	register callcount;
	int fcount;
	struct mtop *mtop;
	struct mtget *mtget;
	/* we depend on the values and order of the MT codes here */
/*XXX*/	static stops[] =
	   {ST_WEOF,ST_REOF,ST_SREV,ST_SFORW,ST_SREV,ST_REW,ST_OFFL,ST_SENSE,ST_ERASE,ST_RETENSION};
	DEBUGF( (stdebug&SHOW_ORDWR),
		printf("stioctl: entered\n"));

	switch (cmd) {

	case MTIOCTOP:	/* tape operation */
		mtop = (struct mtop *)data;
		switch (mtop->mt_op) {

		case MTFSR:
		case MTWEOF:
			fcount = mtop->mt_count;
			callcount = 1;
			break;

		case MTFSF:
			callcount = mtop->mt_count;
			fcount = 1;
			break;

		case MTBSF:
		case MTBSR:
			uprintf("st0: can't back up\n");
			return(ENXIO);		/* can't back up */

		case MTERASE:
		case MTREW: case MTOFFL: case MTNOP: case MTRETENSION:
			callcount = 1;
			fcount = 1;
			break;

		default:
			return (ENXIO);
		}
		if (callcount <= 0 || fcount <= 0)
			return (EINVAL);
		DEBUGF(stdebug & SHOW_ENQ, 
			printf("cmd=%x fcount=%d callcount=%d\n",stops[mtop->mt_op], fcount, callcount));
		while (--callcount >= 0) {
			if (stcommand(dev, stops[mtop->mt_op], fcount))
				return(EIO);
			if ((mtop->mt_op == MTFSR) &&
			    bp->b_resid)
				return (EIO);
			if ((bp->b_flags&B_ERROR))
				break;
		}
		return (geterror(bp));

	case MTIOCGET:
		mtget = (struct mtget *)data;
		mtget->mt_dsreg = sc->sc_dsreg;
		mtget->mt_erreg = sc->sc_data[0] + (sc->sc_data[1] << 8);
		mtget->mt_resid = sc->sc_resid;
		mtget->mt_type = MT_ISST;
		break;

	default:
		return (ENXIO);
	}
	return (0);
}

stwait(addr)
	register struct stdevice *addr;
{
	register s;
	DEBUGF( (stdebug&SHOW_WAIT),
		printf("stwait: entered\n"));

	do
		{
		s = addr->stcs;
		DELAY(1);
		}
	while ((s & STC_BSY));
}



/* stcmd - issue "cmd" and wait for completion code of "waitfor" */
stcmd( cmd, waitfor )
register int cmd, waitfor;
{
	int timeout=299;
	register struct stdevice *addr = (struct stdevice *)stdinfo[0]->iod_addr;

	if (stbusy()) /* is device busy? */
		return(1);	/* can't issue command - busy */

	DEBUGF((stdebug&SHOW_REGS),
		printf("stcmd: cmd=%x (%s) ",cmd,stcmdprint(cmd)));
	addr->stcmd = cmd;

	if (cmd & ST_DATA)
		{
		while (((addr->stdata&0xff) != CC_LD_COUNT) && (--timeout))
			delay(1);
		if (!timeout)
			printf("st: timeout6' cmd=%x waitfor=%x got %x\n",cmd,CC_LD_COUNT,addr->stdata);
		else
			DEBUGF(" data=%x ",(cmd>>8) & 0xffff);
		addr->stdata = cmd >> 8;
		timeout=99;
		}
	while (((addr->stdata&0xff) != waitfor) && (--timeout))
		delay(1);
	if (!timeout)
		printf("st: timeout6 cmd=%x waitfor=%x got %x\n",cmd,waitfor,addr->stdata);
	else
		DEBUGF((stdebug&SHOW_REGS),
			printf("status=0x%b data=%x ",addr->stcs,STCFMT,waitfor));

	return(!timeout);	/* 0 = ok, != 0 timeout */
}


/* stread_status() - read the eight status bytes returned from a read */
/*	status command the first six bytes returned are directly from */
/*	the tape drive */
stread_status()
{
	register struct stdevice *addr = (struct stdevice *)stdinfo[0]->iod_addr;
	register int i;
	register int data;

	DEBUGF( (stdebug&SHOW_GO),
		printf("read_status entered \n"));

	/* flush out any other data */
	for (i=0; (addr->stcs & STC_OBF); ++i) {
		data = addr->stdata;
		DELAY(1);
	}
	stcmd(ST_SENSE, CC_RD_STATUS);		/* read status */
}

stget_status()
{
	register struct st_softc *sc = &st_softc[0];
	int i, x;
	register int data;
	int timeout;
	register struct stdevice *addr;
	addr = (struct stdevice *)stdinfo[0]->iod_addr;

	DEBUGF( (stdebug&SHOW_GO),
		printf("stget_status entered \n"));

	/* read 8 data bytes from port */
	for (i=0; i<8; i++) {
		/* spin until data ready to read*/
		timeout = 10000;
		while ( (((x=addr->stcs) & STC_OBF) == 0) && (--timeout)) 
			DELAY(1);
		if (!timeout) {
			printf("st: timeout7 occurred\n");
			break;
		}

		sc->sc_data[i] = addr->stdata;
		data = sc->sc_data[i];

		if (timeout) {
			DEBUGF( (stdebug&SHOW_DATA),
				printf("data%d read from drive = 0x%x\n",i,data));
		} else
			printf("TIMOUT! data%d read from drive = 0x%x\n",i,data);
	}
	DEBUGF((stdebug & SHOW_REGS), stextendstatus(printf));

}

/*
 * print extended status information
 */
stextendstatus(fn)
int (*fn)();
{
	register struct st_softc *sc = &st_softc[0];
		
	(*fn)("st0: extended status: status0=%b status1=%b\n",
		sc->sc_data[1],ST1FMT,sc->sc_data[0],ST0FMT);
}


/*
 * check status. 
 * if flag=0 only do so if exception indicated
 * otherwise do it always.
 * return 0 if everything looks all right 
 * return >0 if we have any sort of problem (timeout)
 * return < 0 if exception was cleared by read status
 */
stcheck_status(flag)
	register int flag;
{
	register struct st_softc *sc = &st_softc[0];
	register struct stdevice *addr;
	register int status;
	int s;
	int returnflag=0;

	s = spl_tape();
	addr = (struct stdevice *)stdinfo[0]->iod_addr;

	returnflag |= stbusy(); /* is device busy? */

	status = addr->stcs;
	sc->sc_dsreg = status;		/* remember status */

	DEBUGF( (stdebug&SHOW_REGS),
		stprintstatus("stcheck_status ",status));
	DEBUGF((stdebug&SHOW_REGS),
		printf("stcheck_status flag=%x\n",flag));

	/* do we have an exception or status request?? */
	if (returnflag || (status & STC_EXC) || flag) {

#ifdef ST_TRACE
		{ register int t = st_trace;
		st_tracebuff[t] = sc->sc_lastcmd;
		st_tracebuff[t+1] = status;
		st_tracebuff[t+2] = addr->stdata;
		st_tracebuff[t+3] = time.tv_sec;
		st_trace = (t + 4) & (ST_MAX_TRACE - 1);
		}
#endif
		sc->sc_intr = 1;		/* expect interrupt */
		returnflag = 0;
		stread_status();
		returnflag |= stbusy();		/* wait for busy to go away */
		stget_status();
		if (addr->stcs & STC_EXC) {
			stinit_adapter();	/* still problem - init adapter */
			delay(50);
		}
		else if (status & STC_EXC)
			returnflag = -1;	/* read status cleared exception */
			

#ifdef LATER
		if ((sc->sc_data[1] & 0x81) == 0x81) {
			returnflag = sc->sc_data[1];
			DEBUGF( (stdebug>1),
				printf("stcheck_status READ a FILE MARK!"));
			stinit_adapter();
			delay(50);
		}
#endif

	} 

	returnflag |= stready(0); /* device ready? */

	splx(s);

	return(returnflag);
}


stinit_adapter()
{
	register struct stdevice *addr;
	register struct st_softc *sc = &st_softc[0];
	int x;
	int s;

	addr = (struct stdevice *)stdinfo[0]->iod_addr;

	sc->sc_init = 1;	/* initializing the drive */
	sc->sc_intr = 1;	/* we expect an interrupt from this */

	s = spl_tape();

	/* initialize tape adapter - do handshake */
	streset(addr);

	stcmd(ST_CMD_AA, CC_POR);

	DEBUGF( (stdebug&SHOW_INIT),
		printf("stinit1: starting reset initialization  rc=0x%x  status=0x%b\n",addr->stdata,addr->stcs,STCFMT));
	stcmd(ST_RESET, CC_POR_AA);
	DEBUGF( (stdebug&SHOW_INIT),
		printf("stinit2: rc=0x%x  status=0x%b\n",addr->stdata,addr->stcs,STCFMT));
	/*addr->stcmd = 00;*/
	stcmd(ST_CMD_AA, CC_POR);

	DEBUGF( (stdebug&SHOW_INIT),
		printf("stinit3: rc=0x%x  status=0x%b\n",addr->stdata,addr->stcs,STCFMT));

	stread_status();
	stget_status();

	x = addr->stdata;
	DEBUGF( (stdebug&SHOW_INIT),
		printf("stinit4: end reset initialization  rc=0x%x  status=0x%b\n",addr->stdata,addr->stcs,STCFMT));

	splx(s);

	*int_reset = 0xff;
	x = addr->stdata;
	/*x = addr->stcs;*/

	sc->sc_init = 0;
}

stprintstatus( string, status )
char *string;
char status;
{
	printf("st0: %s - status=0x%b\n",string,status,STCFMT);
}


/*
 * load/unload the adapter buffer
 * called for write/read when a data transfer is required.
 * adunload is called to transfer as much data as exists in
 * the current page, and then the next page address is obtained
 * and the process continues.
 */
stunload(bp, dp, unit, staddr)
	register struct buf *bp, *dp;
	register int unit;
	struct stdevice *staddr;
{
/* count remaining = end-address - current address */
	register unsigned int stcnt = (bp->b_un.b_addr + bp->b_bcount) - dp->b_un.b_addr;
					  /* note bp != dp */
	register caddr_t addr = st_softc[STUNIT(bp->b_dev)].st_physaddr; /* the real memory address */
/*
 * following calculation yields the number of bytes left on this page
 * it will give zero when at the start of a page which skips the first
 * adunload.
 */
	register int nleft = PGOFSET - (((int)dp->b_un.b_addr - 1) & PGOFSET);
	register int flag = bp->b_flags & B_READ; /* 0=write, non-zero = read */

	/*printf("stunload: bp=%x  dp=0x%x  unit=0x%x  staddr=0x%x\n", bp, dp, unit, staddr);*/
	/*printf("stunload: bpaddr=0x%x  bpcount=0x%x  dpaddr=0x%x  \n",bp->b_un.b_addr, bp->b_bcount, dp->b_un.b_addr);*/


	if (stcnt > STBLOCK_SIZE)
		stcnt = STBLOCK_SIZE; /* actual amount to transfer */
	/*printf("stcnt=%d ", stcnt);*/
	if (nleft > stcnt)
		nleft = stcnt;
	/*printf("nleft=%d ", nleft);*/
	if (nleft) {
		if (addr == NO_ADDR) {
			addr = real_buf_addr(bp, dp->b_un.b_addr); /* first time */
			if (addr == (caddr_t) -1) {
				printf("stunload: real_buf_addr1 failed when passed bp=0x%x  dp->addr=0x%x\n", bp, dp->b_un.b_addr);
				bp->b_flags |= B_ERROR;
				bp->b_error = ENXIO;
				iodone(bp);
				return(1);
			}
		}

		/*printf("stunload:adunload1 addr=0x%x staddr=0x%x nleft=0x%x flag=0x%x\n", addr, staddr, nleft, flag);*/
		xadunload(addr, staddr, nleft, flag);
		addr += nleft;
		dp->b_un.b_addr += nleft;
	}
	/*printf("nleft=%d ", stcnt - nleft);*/
	if (nleft = stcnt - nleft) {	  /* if more to transfer then we are now at a
					     page boundary */
		addr = real_buf_addr(bp, dp->b_un.b_addr);
		if (addr == (caddr_t) -1) {
			printf("stunload: real_buf_addr2 failed when passed bp=0x%x  dp->addr=0x%x\n", bp, dp->b_un.b_addr);
			bp->b_flags |= B_ERROR;
			bp->b_error = ENXIO;
			iodone(bp);
			return(1);
		}
		/*printf("stunload:adunload2 addr=0x%x staddr=0x%x nleft=0x%x flag=0x%x\n", addr, staddr, nleft, flag);*/
		xadunload(addr, staddr, nleft, flag);
		dp->b_un.b_addr += nleft;
		addr += nleft;
	}
	/*printf("nleft=%d ", STBLOCK_SIZE - stcnt);*/
	if ((nleft = STBLOCK_SIZE - stcnt)) {
		/*printf("stunload:adunload3 addr=0x%x staddr=0x%x nleft=0x%x flag=0x%x\n", addr, staddr, nleft, flag);*/
		xadunload((caddr_t)0, staddr, nleft, flag); /* partial read */
	}
	st_softc[STUNIT(bp->b_dev)].st_physaddr = addr; /* updated real address */
	/*printf("addr=%x\n", addr);*/
}


char *stcmdprint(cmd)
register int cmd;
{
	static char *cmds[] = {
	"RESET", "SELECT", "REW", "ERASE", "RETENSION", "SENSE", "WRITE", "WEOF",
	"READ", "REOF", "badA", "badB", "FORW", "TERMINATE" };
	register char *str;

	if (cmd >= 0 && cmd <= ST_TERMINATE)
		str = cmds[cmd];
	else if (cmd == ST_CMD_AA)
		str = "CMD_AA";
	else
		str = "unknown";
	return(str);
}

/*
 * during open - wait for a rewind to finish if one is going on 
 * this sleep is interruptable.
 * we will give up if we exceed the maximum rewind time 
 * (in case pending rewind is hung)
 */
st_rew_wait(dev)
{
	register int timer = 0;
	register struct iocc_ctlr *ic = stdinfo[STUNIT(dev)]->iod_mi;

	while (ic->ic_tab.b_active != 0) {
		if (++timer > 120)
			break;
		DEBUGF( stdebug & SHOW_SLEEP, printf("sleeping on lbolt\n"));
		sleep(&lbolt, PZERO+1);
	}
	DEBUGF( stdebug & SHOW_SLEEP, printf("sleep timer=%d\n",timer));
}
	
stbrk()
{
}

streset(addr) 
	register struct stdevice *addr;
{

	addr->stcs = STC_ENABLE;
	DELAY(1);
	addr->stcs = STC_ENABLE+STC_NORESET;

	delay(1);
}
#endif
