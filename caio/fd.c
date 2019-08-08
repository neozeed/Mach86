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
/* $Header: fd.c,v 5.0 86/01/31 18:09:04 ibmacis ibm42a $ */
/* $Source: /ibm/acis/usr/sys_ca/caio/RCS/fd.c,v $ */

#include "fd.h"
#ifdef NFD > 0

#include "../h/param.h"
#include "../h/vm.h"
#include "../h/buf.h"
#include "../h/time.h"
#include "../h/proc.h"
#include "../h/errno.h"
#include "../ca/pte.h"
#include "../ca/io.h"
#include "../caio/ioccvar.h"
#include "../ca/debug.h"

#include "../ca/mmu.h"
caddr_t real_buf_addr();

#include "../h/kernel.h"
#include "../h/uio.h"
#include "../h/file.h"
#include "../caio/dmareg.h"
#include "../caio/fdreg.h"
#include "../caio/fdvar.h"
#include "../caio/fdio.h"

/* per-controller data */
struct fd_ctlr fd_ctlr[NFDC];

/* per-drive buffers */
struct buf rfdbuf[NFD];			  /* buffers for raw I/O */
struct buf fdutab[NFD];			  /* per drive buffers */

/* per-drive data */
struct fd_softc fd_softc[NFD];
/* End of per-drive data */

struct iocc_device *fddinfo[NFD];
struct iocc_ctlr *fdminfo[NFDC];

int fdprobe(), fdslave(), fdattach(), fddgo(), fdint(), fdwatch(), fdcallint() ;
unsigned dmaminphys();

caddr_t fdstd[] = {
	(caddr_t)0xf00003f2, (caddr_t)0xf0000372, 0
};

/* because the floppy disk first register is write-only it appears to
 * not exist to autoconf.
 * we set idr_csr to 2 to find a read/write register
 */
struct iocc_driver fdcdriver = {
	fdprobe, fdslave, fdattach, fddgo, fdstd, "fd", fddinfo, "fdc", fdminfo,
	fdint, 2
};

int fdwstart;

char fdresult();

/*
 * Drive description table.
 */
struct fdst fdst[] = {
      /*sec/track heads sec/cyl #cyl byte/disk   gpl  fgpl  step  xfer drive */
	/* 360K diskette in 1.2M drive */
	{    9,     2,   2 * 9,  40,2*9*40*FDBPS,0x23,0x50, 0xdf,0x01,FD1200K },
	/* 1.2M diskette */
	{    15,    2,   2 * 15, 80,2*15*80*FDBPS,0x1b,0x54,0xdf,0x00,FD1200K },
	/* 360K diskette in 360k drive */
	{    9,     2,   2 * 9,  40,2*9*40*FDBPS,0x2a,0x50, 0xdf,0x02,FD360K }
};

int fddebug = 0;			  /* default no debugging */

fdprobe(reg)
	register struct fddevice *reg;
{
	DEBUGF(fddebug > 8,printf("fdprobe entered\n"););
	reg->fd_digout = FDMOTORON(0) | FDINTENABLE;
	DELAY(FDMOTORSTART);
	reg->fd_digout = 0;
	DELAY(FDMOTORSTART);
	return (PROBE_OK);
}

fdslave(iod, reg)
	register struct iocc_device *iod;
	register struct fddevice *reg;
{ 
	register int unit = iod->iod_unit;
	int timeout = FDWAITTIME;
	u_char status;

	/* motor on */
	reg->fd_digout = FDMOTORON(unit) | iod->iod_slave;
	DELAY(FDMOTORSTART);

	/* recalibrate */
	reg->fd_data =FDREST;
	while ((((status = reg->fd_status) & (FDSTATRQM | FDSTATDIO)) != FDSTATRQM) && (--timeout))
		DELAY( (int) 2);
	reg->fd_data = iod->iod_slave;
	timeout = FDWAITTIME;
	while ((((status = reg->fd_status) & (FDSTATRQM | FDSTATDIO)) != FDSTATRQM) && (--timeout))
		DELAY( (int) 2);
	DELAY(FDMOTORSTART);

	/* seek to track 0 */
	reg->fd_data = FDSEEK;
	timeout = FDWAITTIME;
	while ((((status = reg->fd_status) & (FDSTATRQM | FDSTATDIO)) != FDSTATRQM) && (--timeout))
		DELAY( (int) 2);
	reg->fd_data = iod->iod_slave;
	timeout = FDWAITTIME;
	while ((((status = reg->fd_status) & (FDSTATRQM | FDSTATDIO)) != FDSTATRQM) && (--timeout))
		DELAY( (int) 2);
	reg->fd_data = 0;
	DELAY(FDMOTORSTART);
	timeout = FDWAITTIME;
	while ((((status = reg->fd_status) & (FDSTATRQM | FDSTATDIO)) != FDSTATRQM) && (--timeout))
		DELAY( (int) 2);


	/* read status */
	reg->fd_data = FDREADSTAT;
	timeout = FDWAITTIME;
	while ((((status = reg->fd_status) & (FDSTATRQM | FDSTATDIO)) != FDSTATRQM) && (--timeout))
		DELAY( (int) 2);
	reg->fd_data = iod->iod_slave;
	timeout = FDWAITTIME;
	 while ((((status = reg->fd_status) & (FDSTATRQM | FDSTATDIO)) != (FDSTATDIO | FDSTATRQM)) && (--timeout))
		DELAY( (int) 2);
	status = reg->fd_data;
	reg->fd_digout = 0;
	DELAY(FDMOTORSTART);
	DEBUGF(fddebug,printf("fd%d, status=0x%b\n",unit,status,FDSR3););
	return(status & FDTRAK0);
}

fdattach(iod)
	register struct iocc_device *iod;
{
	register struct buf *dp = &fdutab[iod->iod_unit];
	register struct fd_softc *sc = &fd_softc[iod->iod_unit];
	register struct fd_ctlr *fdc = &fd_ctlr[iod->iod_ctlr];
	register struct iocc_ctlr *um = fdminfo[iod->iod_ctlr];

	dp->b_actf = NULL;
	dp->b_actl = NULL;
	dp->b_active = 0;
	um->ic_channel = DM_CHAN2;
	um->ic_party = DM_THIRD;
	um->ic_transfer = DM_SINGLE;
	sc->sc_dens=0;
	sc->sc_motor=0;
	sc->sc_open=0;
	sc->sc_curcyl=0;
	sc->sc_flags=0;
	sc->sc_drive=FDUNKNOWN;
	fdc->fdc_lstdens=0;
	fdc->fdc_drives=0;
	fdc->fdc_state=FDS_IDLE;
	return (1);
}

fdopen(dev, flags)
	dev_t dev;
	int   flags;
{
	register int unit = FDUNIT(dev);
	register struct fd_softc *sc;
	register struct iocc_device *ui;
	register int error,s;
	struct fd_ctlr *fdc;

	DEBUGF(fddebug > 8,printf("fdopen entered\n"););

	if (unit >= NFD || (ui = fddinfo[unit]) == 0 || ui->iod_alive == 0)
		return (ENXIO);

	sc = &fd_softc[unit];
	fdc = &fd_ctlr[ui->iod_ctlr];

loop:
	s=FD_SPL();
	if (sc->sc_open == 0) {

		/* don't try to autodensity while someone else is */
		if (sc->sc_flags & FDF_LOCK) {
			splx(s);
			sleep((caddr_t) sc,FDPRI);
			goto loop;
		} else {
			sc->sc_flags |= FDF_LOCK;
		}
			
		/*
		 * read device status to determine if
		 * a floppy is present in the drive and
		 * what density it is
		 */
		if (sc->sc_motor == 0) {
			error=fdautodensity(dev,flags);
			if (error) {
				sc->sc_flags &= ~FDF_LOCK;
				splx(s);
				wakeup((caddr_t) sc);
				return(error);
			}
		}
		if (fdwstart++ == 0) {
			fdc->fdc_tocnt = 0;
			timeout(fdwatch, (caddr_t)0, hz); /* start watchdog */
		}
		DEBUGF(fddebug > 8,printf("fdopen: \n"););
		sc->sc_flags &= ~FDF_LOCK;
		fdc->fdc_drives++;		/* number of open drives on ctlr*/
	} else {
		if (sc->sc_flags & FDF_LOCK) {
			DEBUGF(fddebug > 8, printf("fdopen: EBUSY  sc->sc_open=%d  \n", sc->sc_open););
			splx(s);
			return (EBUSY);
		}
		if ((sc->sc_sr3 & FDNOWRITE) && (flags & FWRITE)) {
			uprintf("fd%d: write protected.\n",unit);
			splx(s);
			return (EIO);
		}
	}
	sc->sc_open++;
	splx(s);
	wakeup((caddr_t) sc);

	DEBUGF(fddebug > 8,printf("fdopen exit\n"););
	return (0);
}


/* do the hardware initialization */
fdautodensity(dev,flags)
	dev_t dev;
	int   flags;
{
	register int unit = FDUNIT(dev);
	register struct fd_softc *sc = &fd_softc[unit];
	register struct fd_ctlr *fdc = &fd_ctlr[fddinfo[unit]->iod_ctlr];

	DEBUGF(fddebug > 8,printf("fdautodensity entered\n"););

	sc->sc_errcnt=0;
      for (sc->sc_retries=0;sc->sc_retries < FDRETRIES ;sc->sc_retries++) {
		struct buf *bp = geteblk(FDBPS);

		sc->sc_flags = (unit & FDF_DEVTYPE) | FDF_LOCK;
		bp->b_dev = dev;
		bp->b_flags = B_SETUP;
		bp->b_blkno = 0;
		bp->b_error = 0;
		bp->av_forw = 0;
		fdstrategy(bp);
		iowait(bp);

		if ((sc->sc_sr3 & FDNOWRITE) && (flags & FWRITE)) {
			fdmotoroff(unit);
			uprintf("fd%d: write protected.\n",unit);
			brelse(bp);
			return (EIO);
		}

	

		/* this last piece of code tries to read sector 1 
		 * track 0 * if it can then we are at the correct 
		 * density, otherwise * try different density
		 */
		DEBUGF(fddebug,printf("density = %d, retries= %d",sc->sc_dens,sc->sc_retries););
		DEBUGF(fddebug,printf(" drive =%d\n",sc->sc_drive););

		sc->sc_flags = (unit & FDF_DEVTYPE) | FDF_LOCK;
		bp->b_dev = dev;
		bp->b_flags = B_READ;
		bp->b_error = 0;
		bp->b_blkno = fdst[sc->sc_dens].nspc;	/* seek to cylinder 1 */
		bp->av_forw = 0;
		bp->b_bcount = FDBPS;
		sc->sc_curcyl = 0;		/* force a seek each round */
		sc->sc_resid = 0;
		/*
		 * read device status to determine if
		 * a floppy is present in the drive and
		 * what density it is
		 */
		fdstrategy(bp);
		DEBUGF(fddebug,printf("strategy complete\n"););
		if (fdwstart++ == 0) {
			fdc = &fd_ctlr[fddinfo[unit]->iod_mi->ic_ctlr];
			fdc->fdc_tocnt = 0;
			timeout(fdwatch, (caddr_t)0, hz); /* start watchdog */
		}
		iowait(bp);
		DEBUGF(fddebug,printf("Floppy in drive\n"););

		/* check command status register after operation */
		if (((sc->sc_sr0 & FDSR0MASK) != 0) || (sc->sc_curcyl != 1) || (sc->sc_hrdhd != 0) || (bp->b_flags & B_ERROR)) {
			if (++sc->sc_errcnt > 1) { /* force two errors before changing den */
				sc->sc_errcnt = 0;
				switch (sc->sc_drive) {

				default:
				case FDUNKNOWN:
					if (++sc->sc_dens >FDMAXTYPE)
						sc->sc_dens=0;
					break;

				case FD360K:
					sc->sc_dens=FD360K360K;
					break;

				case FD1200K:
					sc->sc_dens = (sc->sc_dens == 0);
					break;
				}
			}
		} else {
			brelse(bp);
			sc->sc_retries = -1;
			sc->sc_drive = fdst[sc->sc_dens].drive;
			break;
		}
		brelse(bp);
		if (sc->sc_flags & FDF_TIMEOUT) {
			fdmotoroff(unit);
			uprintf("fd%d: door open or hardware fault.\n",unit);
			return(ENXIO);		 /* return error if no disk */
		}
	
	} /* end for */

	DEBUGF(fddebug,printf("Flags = %x ",flags););
	
	if (sc->sc_retries > 0) {
	 	if (flags == FDFORMREQ) 
			sc->sc_flags |= FDF_FORMAT;
		else {
			fdmotoroff(unit);
			uprintf("fd%d: bad or unformatted floppy.\n",unit);
			return (ENXIO);
		}
	}

	DEBUGF(fddebug > 0, {
		switch (sc->sc_dens) {
		case 0:
			printf("fd: opening floppy drive for 360k in 1.2M\n");
			break;
		case 1:
			printf("fd: opening floppy drive for 1.2M in 1.2M\n");
			break;
		case 2:
			printf("fd: opening floppy drive for 360K in 360K\n");
		}
	});

	return (0);
}

fdread(dev, uio)
	dev_t dev;
	struct uio *uio;
{
	int unit = FDUNIT(dev);
	struct fd_softc *sc = &fd_softc[unit];
	DEBUGF(fddebug > 0x70,printf("fdread entered\n"););

	if (uio->uio_offset < 0)
		return (ENXIO);
	if (sc->sc_flags & FDF_FORMAT)
		return (ENXIO);
	return (physio(fdstrategy, &rfdbuf[unit], dev, B_READ, dmaminphys, uio));
}


fdwrite(dev, uio)
	dev_t dev;
	struct uio *uio;
{
	int unit = FDUNIT(dev);
	struct fd_softc *sc = &fd_softc[unit];
	DEBUGF(fddebug > 0x70,printf("fdwrite entered\n"););

	if (uio->uio_offset < 0)
		return (ENXIO);
	if (sc->sc_flags & FDF_FORMAT)
		return (ENXIO);
	return (physio(fdstrategy, &rfdbuf[unit], dev, B_WRITE,dmaminphys, uio));
}


/*
 * Control routine:
 * processes four kinds of requests:
 *
 *	(1) Set density (i.e., format the diskette) according to
 *		  that specified data parameter
 *	(2) Arrange for the next sector to be written with a deleted-
 *		  data mark.
 *	(3) Report whether the last sector read had a deleted-data mark
 *	(4) Report the density of the diskette in the indicated drive
 *	    (since the density it automatically determined by the driver,
 *	     this is the only way to let an application program know the
 *	     density)
 *
 * Requests relating to deleted-data marks can be handled right here.
 * A "set density" (format) request, however, must additionally be
 * processed through "fdstart", just like a read or write request.
 */

fdioctl(dev, cmd, data, flag)
	dev_t dev;
	caddr_t data;
{
	int unit = FDUNIT(dev);
	struct fd_softc *sc = &fd_softc[unit];
	int s;
	DEBUGF(fddebug > 8,printf("fdioctl entered\n"););

	switch (cmd) {

	case FDIOC_FORMAT:
		if ((flag & FWRITE) == 0)
			return(EBADF);
		if (sc->sc_open > 1)
			return (EBUSY);
		switch (*(int *)data) {

		case 0:
		case 1:
			sc->sc_dens=(*(int *)data);
			if (sc->sc_drive == FD360K)
				sc->sc_dens += FD360K360K;
			if (sc->sc_dens > FDMAXTYPE)
				return(EIO);
			else
				return (fdformat(dev));
		default:
			return(EIO);
		}

		/* get density */
	case FDIOC_GDENS:
		*(int *)data = sc->sc_dens;
		return (0);

		/* reset adapter */
	case FDIOC_RESET:
		s=FD_SPL();
		fdrecal(unit);
		fdreset(fddinfo[unit]->iod_ctlr);
		splx(s);
		return (0);
	}
	return (ENXIO);
}

/*ARGSUSED*/
fdclose(dev, flag)
	dev_t dev;
{
	register struct fd_softc *sc = &fd_softc[FDUNIT(dev)];
	register struct fd_ctlr *fdc = &fd_ctlr[fddinfo[FDUNIT(dev)]->iod_ctlr];

	sc->sc_open = 0;
	fdc->fdc_drives--;
	DEBUGF(fddebug > 8,printf("fdclose: dev=0x%x, sc_open=%d\n", dev, sc->sc_open););
}


/*
*	Strategy Routine:
*	Arguments:
*	  Pointer to  I/O buffer structure
*	  R/W function flag
*	Function:
*	  Start up the device
*/


fdstrategy(bp)
	register struct buf *bp;
{
	struct iocc_device *ui;
	register struct buf *dp;
	int s, unit = FDUNIT(bp->b_dev);
	struct fd_softc *sc = &fd_softc[unit];
	register struct fdst *st = &fdst[sc->sc_dens];
	DEBUGF(fddebug > 8,printf("fdstrategy entered\n"););

	if (unit >= NFD)
		goto bad;
	ui = fddinfo[unit];
	if (ui == 0 || ui->iod_alive == 0)
		goto bad;

	if (bp->b_blkno < 0)  {
		DEBUGF(fddebug > 0,printf("fdstrategy: requested a sector out of range\n"););
		goto bad;
	}
	/* if total transfer execeeds disk capacity, truncate b_bcount */
	sc->sc_trunc=0;
	if (((bp->b_blkno * BSIZE) + bp->b_bcount) > st->nbpd) {
		sc->sc_trunc=bp->b_bcount;
		bp->b_bcount =st->nbpd - (bp->b_blkno * BSIZE); 
		if (bp->b_bcount < 0)
			bp->b_bcount = 0;
		sc->sc_trunc-=bp->b_bcount;
		DEBUGF(fddebug > 0,printf("fdstrategy: transfer count to large, reduced to = %d (dec) \n", bp->b_bcount););
	}
	if (bp->b_bcount <= 0) {
		bp->b_resid = sc->sc_trunc;
		goto done;
	}

	s = FD_SPL();
	DEBUGF(fddebug > 1,printf("fdstrat: bp=0x%x, fl=0x%x, un=%d, bl=%d, cnt=%d\n", bp, bp->b_flags, unit, bp->b_blkno, bp->b_bcount););
	bp->b_cylin = bp->b_blkno;	  /* don't care to calculate trackno */
	dp = &fdutab[unit];
	disksort(dp, bp);		  /* */
	if (dp->b_active == 0) {
		fdustart(ui);
		bp = &ui->iod_mi->ic_tab;
		DEBUGF(fddebug > 8,printf("fdstrategy after fdustart  bp=0x%x\n", bp););
		if (bp->b_actf && bp->b_active == 0)
			fdstart(ui->iod_mi);
	}
	splx(s);

	DEBUGF(fddebug > 8,printf("fdstrategy exit\n"););
	return;

bad:
	bp->b_error = ENXIO;
	bp->b_flags |= B_ERROR;
done:
	iodone(bp);
	return;
}


/*
 * Unit start routine.
 * Put this unit on the ready queue for the controller
 */
fdustart(ui)
	register struct iocc_device *ui;
{
	struct buf *dp = &fdutab[ui->iod_unit];
	struct iocc_ctlr *um = ui->iod_mi;

	DEBUGF(fddebug > 8,printf("fdustart entered um=0x%x\n", um););

	dp->b_forw = NULL;
	if (um->ic_tab.b_actf == NULL)
		um->ic_tab.b_actf = dp;
	else
		um->ic_tab.b_actl->b_forw = dp;
	um->ic_tab.b_actl = dp;
	dp->b_active++;
}


/*
 * Controller start routine.
 * Start a new transfer or continue a multisector
 * transfer. If this is a new transfer (dp->b_active == 1)
 * save the start address of the data buffer and the total
 * byte count in the soft control structure. These are
 * restored into the buffer structure when the transfer has
 * been completed, before calling 'iodone'.
 */
fdstart(um)
	register struct iocc_ctlr *um;
{
	register struct fdst *st;
	register struct fd_ctlr *fdc;
	register struct fd_softc *sc;
	struct buf *dp, *bp;
	int unit;
	register int sector;
	register char cylinder, head;

	DEBUGF(fddebug > 8,printf("fdstart entered\n"););

	if (um->ic_tab.b_active)
		return;
loop:
	if ((dp = um->ic_tab.b_actf) == NULL)
		return;
	if ((bp = dp->b_actf) == NULL) {
		um->ic_tab.b_actf = dp->b_forw;
		goto loop;
	}
	um->ic_tab.b_active++;
	unit = FDUNIT(bp->b_dev);
	sc = &fd_softc[unit];
	fdc = &fd_ctlr[um->ic_ctlr];
	if (fdc->fdc_lstdens == sc->sc_dens)
		fdctrl(unit,fdc->fdc_ctrl);	/* select drive */
	else
		fdmotoron(unit);		/* set proper xfer rate */
	st = &fdst[sc->sc_dens];

	/* save multi-sector info */
	if (dp->b_active == 1) {
		sc->sc_resid = bp->b_bcount; /* */
		sc->sc_bcnt = bp->b_bcount;
		sc->sc_uaddr = bp->b_un.b_addr;
		dp->b_active++;
	}
	bp->b_bcount = sc->sc_resid;

	if (bp->b_flags & B_SETUP) {
		fdc->fdc_state= FDS_SETUP;
		(void) fdint(um->ic_ctlr);
		return;
	}
	DEBUGF(fddebug > 8,printf("fdstart: "););
	/* for either read or write, we must SEEK */
	fdc->fdc_tocnt = 0;
	/* enable interupts */
	fdctrl(unit,fdc->fdc_ctrl | FDINTENABLE);

	sector = ((bp->b_blkno * BSIZE) + (sc->sc_bcnt - sc->sc_resid)) /FDBPS;
	cylinder = sector / (unsigned)(st->nspc);
	sector %= st->nspc;
	head = (sector / st->nsect) & 1;
	sc->sc_maxbyt = (st->nspc - sector) * FDBPS;
	sector %= (st->nsect);

	sc->sc_cyl = cylinder;
	sc->sc_sec = (char) sector;
	sc->sc_head = head;
	if (bp->b_bcount > sc->sc_maxbyt)
		bp->b_bcount = sc->sc_maxbyt;

	if ((sc->sc_curcyl != cylinder) || (sc->sc_hrdhd != head)) {
		fdc->fdc_state = FDS_SEEK;
		/* send seek commands to adapter */
		fdseek(unit,cylinder, head);
	} else {
		DEBUGF(fddebug > 8,printf("fdstart: no seek needed...sc->sc_curcyl=0x%x  req. cyl=0x%x\n", sc->sc_curcyl, cylinder););
		dma_setup(um);
		DEBUGF(fddebug > 8,printf("fdstart: after dma_setup\n"););
	}
	DEBUGF(fddebug > 8,printf("fdstart: exiting"););
}

fddgo(um)
	struct iocc_ctlr *um;
{
	struct buf *bp = um->ic_tab.b_actf->b_actf;
	register int unit = FDUNIT(bp->b_dev);
	struct fd_softc *sc = &fd_softc[unit];
	struct fd_ctlr *fdc = &fd_ctlr[um->ic_ctlr];

	register struct fdst *st = &fdst[sc->sc_dens];
	register char sector = sc->sc_sec;
	register char cylinder = sc->sc_cyl;
	register char head = sc->sc_head;

	DEBUGF(fddebug > 8,printf("fddgo: entered\n"););


	sc->sc_error = 0;
	DEBUGF(fddebug > 8,printf("\rfd: cyl %x  sec %x  count %x  ", cylinder, sector, bp->b_bcount););
					  /* */

	/* determine if READ or WRITE or FORMAT */
	if (bp->b_flags & B_CTRL) {
		/* FORMAT */
		fdc->fdc_state = FDS_FORMAT;
		fdcmd(unit,FDFORMAT); 		/* start format (1 track) */
		fdcmd(unit,(head << FDBIT2)+(char) fddinfo[unit]->iod_slave);
		fdcmd(unit,FDSECSIZE);		/* byte per sector (2= 512)*/
		fdcmd(unit,st->nsect);		/* sectors per cylinder */
		fdcmd(unit,st->fgpl);		/* Gap length */
		fdcmd(unit,FDPATTERN);		/* pattern */
		goto start_dma;
	}
	if (bp->b_flags & B_READ) {
		/* READ */
		fdc->fdc_state = FDS_READ;
		fdcmd(unit,FDREAD);
	} else {
		/* WRITE */
		fdc->fdc_state = FDS_WRITE; 
		fdcmd(unit,FDWRITE);
	}
	/* end read/write if */


	/* common disk setup code for read and write */
	fdcmd(unit,(head << FDBIT2) + (char) fddinfo[unit]->iod_slave);
	fdcmd(unit,cylinder);			/* cylinder */
	fdcmd(unit,head);	  		/* put head number at bit 1 */
	fdcmd(unit,(sector + 1));		/* sector number (1-n) */
	fdcmd(unit,FDSECSIZE);			/* sector size */
	fdcmd(unit, (char) st->nsect);		/* last sector on track */
	fdcmd(unit,st->gpl);			/* gap length */
	fdcmd(unit,FDSECBASE);			/* sector size */

start_dma:
	dma_go(um->ic_channel);

	DEBUGF(fddebug > 8,printf("fddgo: exiting\n"););

}


fdint(ctlr)
	int ctlr;
{
	int unit;
	struct iocc_ctlr *um = fdminfo[ctlr];
	register struct buf *bp, *dp;
	register struct fd_softc *sc;
	register struct fdst *st; 
	struct iocc_device *ui;
	struct fd_ctlr *fdc;

	DEBUGF(fddebug,printf("fdint: entered; ctlr=%d  ",ctlr););

	if (!um->ic_tab.b_active) {
		DEBUGF(fddebug > 1,printf("fdint: um not active\n"););
		return (1);
	}
	dp = um->ic_tab.b_actf;
	if (!dp->b_active) {
		DEBUGF(fddebug > 1,printf("fdint: dp->b_active not active\n"););
		return(1);
	}
	bp = dp->b_actf;
	unit = FDUNIT(bp->b_dev);
	sc = &fd_softc[unit];
	st = &fdst[sc->sc_dens];
	ui = fddinfo[unit];
	fdc = &fd_ctlr[ctlr];
	fdc->fdc_tocnt = 0;
	DEBUGF(fddebug>0,printf(" fdintstate= %s ",statestring[fdc->fdc_state].value););
	fdctrl(unit,fdc->fdc_ctrl & ~FDINTENABLE);
	if (sc->sc_flags & (FDF_TIMEOUT)) {
		if ((fdc->fdc_state == FDS_READ) || (fdc->fdc_state == FDS_FORMAT) || (fdc->fdc_state == FDS_WRITE)) {
			dma_done(um->ic_channel);
			fdunload_results(unit);
		}
		goto giveup;
	}

	switch (fdc->fdc_state) {

		/*
		 * Incomplete commands.  Perform next step
		 * and return.  Note that b_active is set on
		 * entrance and, therefore, also on exit.
		 */
	case FDS_SEEK:
		fdc->fdc_state = FDS_SKDON;
		timeout(fdcallint, ctlr, FDHDSETTLE*st->drive);
		return(0);

	case FDS_SKDON:				/* done with seek */
		fdcmd(unit,FDSENI);

		/* get result */
		sc->sc_sr0 = fdresult(unit,sc->sc_sr0);
		sc->sc_curcyl = fdresult(unit,sc->sc_curcyl);
		sc->sc_hrdcyl=sc->sc_curcyl;
		sc->sc_curcyl = (sc->sc_dens == 0) ? (sc->sc_curcyl / 2) : sc->sc_curcyl;
		(void) fdwait(unit,FDSTATDAB | FDSTATDBB, 0); /* wait for READY */
		DEBUGF(fddebug,printf("Seek ret = 0x%b", (int) sc->sc_sr0,FDSR0););

		/* if error */
		if ((sc->sc_sr0 & FDSR0MASK) != 0)
			goto error;
		fdctrl(unit,fdc->fdc_ctrl | FDINTENABLE);
		dma_setup(um);

		return (0);

	case FDS_READ:
	case FDS_WRITE:
	case FDS_FORMAT:
		dma_done(um->ic_channel);

		DEBUGF(fddebug > 8, {
			if (bp->b_flags & B_READ) 
				printf("fdint: B_READ; ");
			else if (bp->b_flags & B_WRITE)
				printf("fdint: B_WRITE; ");
			else
				printf("fdint: B_CTRL: ");
		});
		fdunload_results(unit);

		/* if error */
		if ((sc->sc_sr0 & FDSR0MASK) != 0)
			goto error;

		fdc->fdc_state = FDS_IDLE;	  /* end read/write/format */
		goto done;

	case FDS_SETUP:
		/* don't reset the controller if the other unit is open */
		if (fdc->fdc_drives == 0)
			fdctrl(unit,fdc->fdc_ctrl & FDRST);
		sc->sc_error = 0;
		fdmotoron(unit);
		fdcmd(unit,FDSPEC);
		fdcmd(unit,st->step);
		fdcmd(unit,FDLOAD);
		fdrecal(unit);
		DEBUGF(fddebug,printf ("Fdrecall completed\n"););
		if (sc->sc_error) {
			bp->b_error=EIO;
			bp->b_flags=B_ERROR;
		} 
		fdc->fdc_state = FDS_DONE;
		timeout(fdcallint,ctlr,FDSETDELAY*st->drive); 
		return(0);

	case FDS_RECAL:
		fdc->fdc_state = FDS_IDLE;
		um->ic_tab.b_active = 0;
		fdstart(um);
		return(0);

	case FDS_DONE: 
		fdc->fdc_state = FDS_IDLE;
		um->ic_tab.b_errcnt = 0;
		um->ic_tab.b_active = 0;
		goto cleanup;

	default:
		printf("fd%d: state %s (reset)\n", unit, statestring[fdc->fdc_state].value);
		fdreset(ctlr);
		return (0);		  /* */
	}
error:
	/*
	 * In case of an error:
	 *  (a) In case of TIMEOUT giveup.
	 *  (b) Otherwise try operation three more times
	 */

	if (++um->ic_tab.b_errcnt >= 3) {
		goto giveup;
	}
	fdrecal(unit);
	fdc->fdc_state=FDS_RECAL;
	timeout(fdcallint, ctlr, FDSETDELAY*st->drive);
	return (0);

giveup:
	/*
	 * Hard I/O error --
	 *	Current request is aborted, next request is started.
	 */
	if (sc->sc_open == 0)
		goto done;

	sc->sc_trunc += sc->sc_resid;
	sc->sc_resid = 0;		  /* make sure the transfer is terminated */

	um->ic_tab.b_errcnt = 0;
	um->ic_tab.b_active = 0;
	fdread_status(unit);
	printf("fd%d: hard error trk = %d  sec =%d ",unit,(int) sc->sc_cyl, (int)sc->sc_sec);
	printf("Sr0 = 0x%b ",(int)sc->sc_sr0,FDSR0);
	printf("Sr1 = 0x%b ",(int)sc->sc_sr1,FDSR1);
	printf("Sr2 = 0x%b ",(int)sc->sc_sr2,FDSR2);
	printf("Sr3 = 0x%b ",(int)sc->sc_sr3,FDSR3);
	printf("Cyl= %d Sec=%d Head=%d St=%d ",sc->sc_hrdcyl,sc->sc_hrdsc,sc->sc_hrdhd,sc->sc_size);
	printf("state = %s.\n",statestring[fdc->fdc_state].value);

	fdrecal(unit);
	if ((fdc->fdc_state == FDS_WRITE) || (fdc->fdc_state == FDS_READ) || (fdc->fdc_state == FDS_SEEK) || (fdc->fdc_state == FDS_FORMAT)) {
		bp->b_flags |= B_ERROR;
		fdc->fdc_state = FDS_IDLE;
		goto cleanup;
	}
	return(0);

done:
	DEBUGF(fddebug > 8,printf("fdint: at done; "););

	um->ic_tab.b_active = 0;
	um->ic_tab.b_errcnt = 0;

	/* multi-sector tricks */
	if ((sc->sc_resid -= sc->sc_maxbyt) > 0) {
		bp->b_un.b_addr += sc->sc_maxbyt;
		fdstart(um);
		return (0);
	}
cleanup:
	bp->b_resid = sc->sc_trunc;
	bp->b_un.b_addr = sc->sc_uaddr;
	bp->b_bcount = sc->sc_bcnt;

	dp->b_actf = bp->av_forw;
	iodone(bp);
	fdc->fdc_state = FDS_IDLE;
	um->ic_tab.b_actf = dp->b_forw;
	dp->b_active = 0;
	dp->b_errcnt = 0;
	DEBUGF(fddebug > 8,printf(".. bp=%x, new=%x\n", bp, dp->b_actf););
	/*
	 * If this unit has more work to do,
	 * start it up right away
	 */
	if (dp->b_actf)
		fdustart(ui);

	fdstart(um);
	DEBUGF(fddebug > 8,printf("fdint: exit; "););
	return (0);
}

fdcallint(ctlr)
{
	int	s = FD_SPL();

	DEBUGF(fddebug>0,printf("fdcallint: ctlr=%d\n",ctlr););
	(void) fdint(ctlr);
	splx(s);
	return;
}


fdwatch()
{

	register struct iocc_device *ui;
	register struct iocc_ctlr *um;
	register struct fd_softc *sc;
	struct fd_ctlr *fdc;
	int i, dopen = 0;
	int s=FD_SPL();

	for (i = 0; i < NFD; i++) {
		ui = fddinfo[i];
		if (ui == 0 || ui->iod_alive == 0)
			continue;
		sc = &fd_softc[i];
		if ((sc->sc_open == 0) && (fdutab[i].b_active == 0)) {
			if (sc->sc_motor >= 1) {
				if (--sc->sc_motor < 1) {
					/* turn off motor */
					fdmotoroff(i);
				} else {
					dopen++;
				}
			}
			continue;
		}
		dopen++;
		um = ui->iod_mi;
		fdc = &fd_ctlr[um->ic_ctlr];
		if (++fdc->fdc_tocnt >= FD_MAXTIMEOUT) {
			fdc->fdc_tocnt = 0;
			if (um->ic_tab.b_active) {
				printf("fd%d: timeout\n", i);/* for debugging */
				sc->sc_flags|=FDF_TIMEOUT;
				(void) fdint(um->ic_ctlr);
			}
		}
	}
	if (dopen)
		timeout(fdwatch, (caddr_t)0, hz);
	else
		fdwstart = 0;
	splx(s);
}


fdreset(ctlr)
	int ctlr;
{
	register struct iocc_ctlr *dm;

	DEBUGF(fddebug > 8,printf("fdreset entered\n"););

	if ((dm = fdminfo[ctlr]) != 0 && dm->ic_alive != 0) {
		fd_ctlr[ctlr].fdc_state = FDS_IDLE;
		fdstart(dm);
	}
}



/*
 * Initiate a format command.
 */
fdformat(dev)
	dev_t dev;
{
	register int unit = FDUNIT(dev);
	register struct fd_softc *sc = &fd_softc[unit];
	register struct iocc_device *ui = fddinfo[unit];
	struct buf *bp = geteblk(FDBPS);
	register struct fdst *st = &fdst[sc->sc_dens];
	register int error=0,cyl,head,sect;
	register struct fdformdata *fdformdata = (struct fdformdata *) bp->b_un.b_addr;

	/* Set up */
	bp->b_dev = dev;
	bp->b_error = 0;
	bp->b_flags = B_SETUP;
	bp->b_blkno = 0;
	bp->av_forw = 0;
	bp->b_bcount = FDBPS;
	fdstrategy(bp);
	iowait(bp);
	if (bp->b_flags & B_ERROR) {
		error = bp->b_error;
		goto out;
	}

	sc->sc_flags = FDF_FORMAT | FDF_LOCK;
	for (cyl = 0; cyl < st->ncyl ; cyl++) {
		for (head = 0; head < st->ntrak; head++) {
			for (sect = 0; sect < st->nsect; sect++) {
				fdformdata[sect].fdtrack = (char) cyl;
				fdformdata[sect].fdhead = (char) head;
				fdformdata[sect].fdsect = (char) sect + 1;
				fdformdata[sect].fdsize = FDSECSIZE;
			}
			DEBUGF(fddebug,{
			register caddr_t tmpaddr = bp->b_un.b_addr;
			register int count;
			printf("\n");
			for (count=0; count < st->nsect; count++)
			 printf("size=%d sect=%d hd=%d cyl=%d\n",(char) *tmpaddr++,(char) *tmpaddr++,(char) *tmpaddr++,(char) *tmpaddr++);
			});

			/* Format */
			bp->b_dev = dev;
			bp->b_flags = B_CTRL;
			bp->b_error = 0;
			bp->b_blkno = cyl * st->nspc + head * st->nsect;
			bp->b_bcount = st->nsect * FDBPS;
			bp->av_forw = 0;
			fdstrategy(bp);
			iowait(bp);
			if (bp->b_flags & B_ERROR) {
				error = EIO;
				break;
			}
		}
		if(error)
			break;
	}
out:
	sc->sc_flags &= ~FDF_LOCK & ~FDF_FORMAT;
	brelse(bp);	
	return (error);
}

/* read extended status */
fdread_status(unit)
	int unit;
{
	register struct fd_softc *sc = &fd_softc[unit];

	fdcmd(unit,FDREADSTAT);			  /* read status */
	fdcmd(unit,(char) fddinfo[unit]->iod_slave);	  /* drive head */
	sc->sc_sr3 = fdresult(unit,FDWAITREAD);
	DEBUGF(fddebug,printf("fdread_status: 0x%b\n", (int) sc->sc_sr3, FDSR3););
}


/* recalibrate floppy drive (reset head to track 0)*/
fdrecal(unit)
	int unit;
{
	register struct fd_softc *sc = &fd_softc[unit];

	fdcmd(unit,FDREST);
	fdcmd(unit, (char) fddinfo[unit]->iod_slave);
	fdread_status(unit);
/*	if ((sc->sc_sr3 & FDTRAK0))
		sc->sc_error++; 
	else */
		sc->sc_curcyl = 0;
}

/* start support routines for fd.c (these aren't "classic" driver routines)   */

fdmotoron(unit)
{
	register struct iocc_device *iod = fddinfo[unit];
	register struct fddevice *fdadapter = (struct fddevice *) iod->iod_addr;
	register struct fd_softc *sc = &fd_softc[unit];
	register struct fd_ctlr *fdc = &fd_ctlr[iod->iod_ctlr];

	fdctrl(unit,fdc->fdc_ctrl | FDMOTORON(iod->iod_slave));
	/* set drive transfer rate here too */
	fdadapter->fd_digin = fdst[sc->sc_dens].xfer;
	fdctrl(unit,fdc->fdc_ctrl | FDMOTORON(iod->iod_slave));
	sc->sc_motor = FDMOTORWAIT;
	fdc->fdc_lstdens = sc->sc_dens;
}


/* turn off motor if other unit is open turn it on */
fdmotoroff(unit)
{
	register struct iocc_device *iod = fddinfo[unit]; 
	register struct iocc_device *iod2;
	register struct fd_softc *sc = &fd_softc[unit];
	register struct fd_ctlr *fdc = &fd_ctlr[iod->iod_ctlr];
	register int unit2;

	if (((unit2 = FDOTHERUNIT(unit)) < NFD) && (iod2 = fddinfo[unit2]) && (iod2->iod_alive)) {
		fdctrl(unit2,fdc->fdc_ctrl & FDMOTOROFF(iod->iod_slave));
	} else {
		fdctrl(unit,fdc->fdc_ctrl & FDMOTOROFF(iod->iod_slave));
	}
	sc->sc_motor = 0;
}

fdctrl(unit,cmd)
	unsigned char cmd;
{
	register struct iocc_device *iod = fddinfo[unit];
	register struct fddevice *fdadapter = (struct fddevice *) iod->iod_addr;
	register struct fd_ctlr *fdc = &fd_ctlr[iod->iod_ctlr];

	fdc->fdc_ctrl = cmd & FDCTRLMASK;
	fdadapter->fd_digout = cmd | iod->iod_slave; 
}

/* send command to floppy disk controller */
fdcmd(unit,cmd)
	char cmd;
{
	register struct fddevice *fdadapter = (struct fddevice *) fddinfo[unit]->iod_addr;
	register struct fd_softc *sc = &fd_softc[unit];

	if (fdwait(unit,FDSTATRQM | FDSTATDIO, FDSTATRQM) == 0) /* wait for READY */
		sc->sc_error++;
	fdadapter->fd_data = cmd;	  /* */
}


/* Wait for disk I/O to complete */
fdwait(unit,mask, compare)
	unsigned char mask;
	unsigned char compare;
{
	register struct fddevice *fdadapter = (struct fddevice *) fddinfo[unit]->iod_addr;
	int timeout = FDWAITTIME;
	u_char status;

	while ((((status = fdadapter->fd_status) & mask) != compare) && (--timeout))
		DELAY( (int) 2);

	if (timeout)
		return(1);
	else {
		DEBUGF(fddebug,{
			printf("fdwait: mask 0x%b,",mask,FDSTAT);
			printf(" compare 0x%b, ",compare,FDSTAT);
			printf("status 0x%b,\n",status,FDSTAT);
		});
		return(0);
	}
}


/* read result byte from the NEC diskette controller */
char fdresult(unit,value)
	char value;
{
	register struct fddevice *fdadapter = (struct fddevice *) fddinfo[unit]->iod_addr;
	register struct fd_softc *sc = &fd_softc[unit];
	char result,temp;

	if (fdwait(unit,FDSTATRQM | FDSTATDIO, FDSTATRQM | FDSTATDIO))  /* wait for READY */
		result = fdadapter->fd_data;	  /* */
	else {
	 	temp = fdadapter->fd_data;
		if (value =  FDWAITREAD)
			result = temp;
		else
			result = value;
		sc->sc_error++;
	}

	return (result);
}


fdseek(unit,cylinder, head)
	unsigned char cylinder, head;
{
	register struct fd_softc *sc = &fd_softc[unit];

	DEBUGF(fddebug > 8,printf("fdseek: entered  cyl=%d  head=%d;  sc->sc_dens=%d", cylinder, head, sc->sc_dens););

	fdcmd(unit,FDSEEK);
	fdcmd(unit,(head << FDBIT2) + (char) fddinfo[unit]->iod_slave);
	/* double step on a 360k diskette in 1.2M drive */
	if (sc->sc_dens == 0)
		fdcmd(unit,cylinder * FDDBLSTEP); /* cylinder */
	else
		fdcmd(unit,cylinder);	  /* cylinder */

	DEBUGF(fddebug > 8,printf("fdseek: exiting; "););
}


/* unload command results */
fdunload_results(unit)
{
	register struct fd_softc *sc = &fd_softc[unit];

	sc->sc_sr0 = fdresult(unit,FDWAITREAD);
	sc->sc_sr1 = fdresult(unit,FDWAITREAD);
	sc->sc_sr2 = fdresult(unit,FDWAITREAD);
	sc->sc_hrdcyl = fdresult(unit,sc->sc_hrdcyl); /* next sector is at this cylinder */
	sc->sc_hrdhd = fdresult(unit,sc->sc_hrdhd);
	sc->sc_hrdsc = fdresult(unit,sc->sc_hrdsc);
	sc->sc_size = fdresult(unit,sc->sc_size);
	DEBUGF(fddebug > 0, {
		printf("FD: unload Sr0 = 0x%b ",(int) sc->sc_sr0,FDSR0); /* */
		printf(" Dsr1 = 0x%b ",(int) sc->sc_sr1,FDSR1); /* */ 
		printf(" DSr2 = 0x%b ",(int) sc->sc_sr2,FDSR2);	/* */
		printf(" cyl = %x ",sc->sc_hrdcyl);/* */
		printf(" hd = %x ", sc->sc_hrdhd); /* */
		printf(" sec = %x ", sc->sc_hrdsc); /* */
		printf(" n = %x\n ", sc->sc_size); /* */
	});
}

#endif					  /* end if for NFD */
