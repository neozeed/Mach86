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
/* $Header: hd.c,v 5.1 86/02/25 21:29:40 webb Exp $ */
/* $Source: /ibm/acis/usr/sys_ca/caio/RCS/hd.c,v $ */

#ifdef	CMU
/*
 ************************************************************************
 * HISTORY
 *  8-May-86  Bill Bolosky (bolosky) at Carnegie-Mellon University
 *	MACH_VM: Increased delay times to account for faster interrupts.
 *
 ***********************************************************************
 */
#include "mach_vm.h"
#endif	CMU

/* $Header: hd.c,v 5.1 86/02/25 21:29:40 webb Exp $ */
/*
 ** hd.c for IBM Workstation
 **
 ** Restrictions:
 **
 **
 ** Note: Currently hd.c is modeled on up.c in terms of managing the device
 ** and controller queue.  We do not yet overlap seeks with transfers.  To
 ** do so will require a softc structure, timeouts to check the Seek
 ** Complete bit, code in hdstart to do seeks (in up.c, they are done in
 ** the ustart routine), and code in hdint to handle (dequeue?) a drive
 ** when the seek has been completed by the controller, but not yet by the
 ** drive.  The timeout routine will check for the Seek Complete bit (if
 ** controller not active) and requeue requests when it is on.  Hdint
 ** should also poll drives for Seek Complete, setting attention status in
 ** the softc structure as necessary.
 **
 ** As in up.c, the dp->b_active variable should use the following values:
 **	0 = Drive not busy
 **	1 = Seek in progress
 **	2 = Data transfer in progress
 **
 */

#include "hd.h"
#if NHD > 0
#include        "../h/param.h"
#include        "../h/vm.h"
#include        "../h/buf.h"
#include        "../h/time.h"
#include        "../h/proc.h"
#include	"../h/errno.h"
#include        "../ca/pte.h"
#include        "../ca/io.h"
#include 	"../ca/hddisk.h"
#include	"../caio/ioccvar.h"
#include	"../h/kernel.h"
#include	"../h/systm.h"
#include	"../h/cmap.h"
#include	"../h/dk.h"
#include	"../caio/hdconfig.h"


/* Defines to set and get the MMU TCR */
#include        "../ca/mmu.h"
caddr_t real_buf_addr();

#define HDTIMEOUT	10		  /* number of seconds til interrupt known lost */
#define MAX_WAIT_COUNT	1500000	  	  /* about 20 seconds worth */
#define NO_ADDR 0			  /* flag for no real address yet */

/* Disk block definitions */

#define	BSIZE	DEV_BSIZE
#define	BSHIFT	DEV_BSHIFT

/* minor device uses following bits
 * bits 0-2	partition number
 * bit  3	physical unit (0 or 1)
 * bit	4	controller (0 or 1)
 * bit  5	not used (only 4 drives supported)
 * bit  6	suppress retries 
 */
#define	drive(d) ((minor(d) >> 3) & 0x07)
#define	part(p)	 (minor(p) & 0x07)
#define NORETRY(d)	((minor(d) >> 6) & 0x01)

#define NBPS(st)	512	/* or st->nbps	*/

#define	b_cylin	b_resid

struct hddevice {			  /*      must cast data when used         */
	u_char data;			  /* 0:   data register (actually u_short) */
	u_char err;			  /* 1:   error/pcmp register */
	u_char sct;			  /* 2:   sector count */
	u_char sno;			  /* 3:   sector number */
	u_char cyll;			  /* 4:   cylinder number lo */
	u_char cylh;			  /* 5:   cylinder number hi */
	u_char sdh;			  /* 6:   size/drive/head */
	u_char stat;			  /* 7:   status/cmd register */
	u_char pad[510];		  /* 8:   pad characters */
	u_char intr;			  /* 518: interrupt control register */
};

#define	DATA	data
#define ERR	err
#define PCMP	err
#define SCT	sct
#define	SNO	sno
#define	CYLL	cyll
#define CYLH	cylh
#define	SDH	sdh
#define	STAT	stat
#define	CMD	stat
#define INTR	intr



caddr_t hdstd[] = {
	(caddr_t)0xf00001f0, (caddr_t)0xf0000170, 0
};

int hdprobe(), hdslave(), hdattach(), hdint();
int hdwstart, hdwatch();		  /* watch routine */

/* hddinfo contains pointers to the slaves (drives) */
struct iocc_device *hddinfo[NHD];

struct iocc_ctlr *hdminfo[NHDC];


struct iocc_driver hdcdriver = {
	hdprobe, hdslave, hdattach,
/*    dgo	addr	dname	dinfo	mname	minfo	intr	csr	 */
	0, hdstd, "hd", hddinfo, "hdc", hdminfo, hdint, 2
};

#define spl_disk()	_spl4()		/* at CPU level 4 */
#define spl_high()	_spl1()		/* at CPU level 1 */

/* Partition table */

#define NHDST 6			  /* support 6 (1 dummy) drives (for now) */

struct partab {

	daddr_t len;			  /* # of blocks in partition */
	int start;			  /* starting cylinder of partition */

} hdinit_sizes[8] = {	/* dummy disk for non-recognized types */
	0, 0,			/* spare */
	0, 0,			/* spare */
	34, 0,			/* c - first two tracks */
	0, 0,			/* spare */
	0, 0,			/* spare */
	0, 0,			/* spare */
	0, 0,			/* spare */
	0, 0,			/* spare */
}, hd20m_sizes[8] = {
	15861, 0,		/* root area  cyl   0 - 310 */
	10200, 311,		/* swap area  cyl 311 - 510 */
	16269, 511,		/* user area  cyl 511 - 829 */
	42330, 0,		/* whole disk cyl   0 - 829 */
	0, 0,			/* spare */
	0, 0,			/* spare */
	0, 0,			/* spare */
	0, 0,			/* spare */
}, hd40m_sizes[8] = {		/* leaving room for reserved areas (AUSTIN) */
	15884,	1,		/* A=cyl 1 thru 187 */
	10032,	188,		/* B=cyl 188 thru 306 */
	87040,	0,		/* C=cyl 0 thru 1023 */
	15884,	307,		/* D=cyl 307 thru 493 */
	0,	0,
	43945,	494,		/* F=cyl 494 thru 1010 */
	59840,	307,		/* G=cyl 307 thru 1010 */
	0,	0,
}, hd70m_sizes[8] = {		/* leaving room for reserved areas (AUSTIN) */
	15884, 1,		/* A=cyl 1 thru 117 */
	33440, 118,		/* B=cyl 118 thru 363 */
	139264, 0,		/* C=cyl 0 thru 1023 */
	15884, 364,		/* D=cyl 364 thru 480 */
	55936, 481,		/* E=cyl 481 thru 892 */
	16592, 893,		/* F=cyl 893 thru 1014 */
	88536, 364,		/* G=cyl 364 thru 1014 */
	0, 0,
}, hd70r_sizes[8] = {
	15884,	1,		/* A=cyl 1 thru 104 */
	33440,	105,		/* B=cyl 105 thru 323 */
	141525,	0,		/* C=cyl 0 thru 924 */
	15884,	324,		/* D=cyl 324 thru 427 */
	55936,	428,		/* E=cyl 428 thru 793 */
	18819,	794,		/* F=cyl 794 thru 916 */
	90729,	324,		/* G=cyl 324 thru 916 */
	0,	0,
}, hd40r_sizes[8] = {
	15884,	1,		/* A=cyl 1 thru 134 */
	10032,	135,		/* B=cyl 135 thru 219 */
	87227,	0,		/* C=cyl 0 thru 732 */
	15884,	220,		/* D=cyl 220 thru 353 */
	0,	0,
	43911,	354,		/* F=cyl 354 thru 722 */
	59857,	220,		/* G=cyl 220 thru 722 */
	0,	0,
};


struct hdst {

	struct partab *off;		  /* 0: patition table */
	u_short ncpd;			  /* 4: number of cylinders / drive */
	u_short pcmp;			  /* 6: write precompensation cylinder */
	u_short nbps;			  /* 8: number of bytes / sector */
	u_char nspt;			  /* 10: number of sectors / track */
	u_char ntpc;			  /* 11: number of tracks / cylinder */
	u_short nspc;			  /* 12: number of sectors / cylinder */
	u_short step;			  /* 14: drive stepping rate */
	/* 16 total */

} hdst[NHDST] = {
/*
 * hdinit_sizes is a dummy record that will work with all disks enough
 * to read the configuration record. It must be first in the following table.
 *
 *	     off	ncpd	pcmp	nbps	nspt	ntpc	nspc	step 
 */

	{ hdinit_sizes,	1,	1024,	512,	17,	2,	17 * 2,	0 },	/* 70 MB */
	{ hd70m_sizes,	1024,	1024,	512,	17,	8,	17 * 8,	0 },	/* 70 MB */
	{ hd40m_sizes,	1024,	1024,	512,	17,	5,	17 * 5,	0 },	/* 40 MB */
	{ hd20m_sizes,	830,	400,	512,	17,	3,	17 * 3,	0 },	/* 20 MB */
	{ hd70r_sizes,	925,	925,	512,	17,	9,	17 * 9,	0 },	/* 70 MB */
	{ hd40r_sizes,	733,	300,	512,	17,	7,	17 * 7,	0 },	 /* 40MB IBM */

};

char *hdtypes[NHDST] = {	/* names of the various disk types */
	"hdinit", "hd70m", "hd40m", "hd20m", "hd70r", "hd40r"
};

struct buf hdutab[NHD];			  /* start of request queue */
struct buf *hdutabp[NHD];		  /* pointers to hdutab */
struct buf rhdbuf[NHD];			  /* header for raw i/o */
struct buf bhdbuf[NHD];			  /* buffer header for reading bad block table */
struct buf *rhdbufp[NHD];
struct iocc_ctlr *hdactive;		  /* the currently active controller */
struct iocc_ctlr *hdwaiting;		  /* the currently waiting controller */

/* software structure per adapter */
struct hdc_softc {
	int hd_timer;			  /* timer for watchdog */
} hdc_softc[NHDC];

/*
 * software structure per drive
 * contains information particular to a given drive
 */
struct hd_softc {
	int hd_flag;			  /* flag bits */
#	define HD_BAD_READ	0x01	  /* we have read the bad block table */
#	define HD_FORWARD	0x02	  /* we doing bad block forward */
	caddr_t hd_physaddr;		  /* the current real address */
	struct hdst *hd_st;		  /* the hdst for this drive */

	int hd_fill[1];			  /* fill up to power of 2 size */
} hd_softc[NHD];


#ifdef DEBUG
#define HDDEBUG(how,stmt) if (hddebug & (how)) stmt	/* print it etc. */

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
#define SHOW_SWITCH	0x400		  /* show switching of controllers */
#define SHOW_BAD	0x800		  /* show bad block forwarding */
#define SHOW_GO		0x1000		  /* show each command data */
#define SHOW_CONFIG	0x2000		  /* show configuration */
#define SHOW_DUMP	0x4000		  /* show dumping */
#define SHOW_DUMPIO	0x8000		  /* show dump I/O */
#define SHOW_RETRY	0x10000		  /* show retries */

int hddebug;				  /* XXX -- change to 0 when done debugging */

#else !DEBUG
#define HDDEBUG(how,stmt) 	/* do nothing */

#endif

struct hdbad *hdbad[NHD];		  /* the drives bad sector maps */

/*
 *	Strategy Routine:
 *	Arguments:
 *	  Pointer to buffer structure
 *	Function:
 *	  Check validity of request
 *	  Queue the request
 *	  Start up the device if idle
 *
 *	Note:
 *	  Block numbers (in b_blkno) are RELATIVE to the start of
 *	  the partition.
 *	  Cylinder numbers (in b_cylin) are ABSOLUTE.
 */
hdstrategy(bp)
	register struct buf *bp;
{
	register int dr, pa;
	register struct hdst *st;
	register struct buf *dp;
	register struct iocc_device *iod;
	long sz, bn;
	int s;

	sz = (bp->b_bcount + (BSIZE - 1)) >> BSHIFT;
	dr = drive(bp->b_dev);
	pa = part(bp->b_dev);
	bn = bp->b_blkno;
	iod = hddinfo[dr];
	st = &hdst[iod->iod_type];

	if (dr < NHD && pa < 8 && pa >= 0 && bn >= 0 && bn < st->off[pa].len
	    &&(((bn + sz) <= st->off[pa].len) || (bp->b_flags & B_READ))) {
		if (bn + sz > st->off[pa].len)
			bp->b_bcount = (st->off[pa].len - bn) * BSIZE;
	} else {

		HDDEBUG(SHOW_ENQ, {
			printf("HD: HDSTRATEGY -- BAD %s REQUEST",
			    (bp->b_flags & B_READ) ? "READ" : "WRITE");
			printf("\nblkno=%d bcount=%d baddr=0x%x",
			    bp->b_blkno, bp->b_bcount, bp->b_un.b_addr);
			printf(" bdev=0x%x\n", bp->b_dev);
			printf("drive=%d part'n=%d ", dr, pa);
			printf(" psize (blocks)=%d",
			    st->off[pa].len);
			printf(" pcyl=%d\n",
			    st->off[pa].start);
		});

		bp->b_flags |= B_ERROR;
		iodone(bp);
		return;
	}
	bp->b_cylin = ((bn * (BSIZE / NBPS(st))) / (st->nspc))
		+st->off[pa].start;

	s = spl_disk();			  /* CPU level 3 interrupt */
	dp = hdutabp[dr];
	disksort(dp, bp);

	HDDEBUG(SHOW_ENQ, {
		printf("HD: HDSTRATEGY: flags=%b",
			bp->b_flags,B_FLAGS_FMT);
		printf(" blkno=%d bcount=%d baddr= 0x%x\n",
		    bp->b_blkno, bp->b_bcount, bp->b_un.b_addr);
		printf(" b_cylin=%d", bp->b_cylin);
		printf(" bdev=0x%x", bp->b_dev);
		printf(" drive=%d part=%d", dr, pa);
		printf(" psize=%d",
		    st->off[pa].len);
		printf(" pcyl=%d\n", st->off[pa].start);
	});

	/*
	 * See if the other controller is active, if so then we can do
	 * nothing until it finishes.
	 * Otherwise see if the drive is active.
	 * If it isn't, call ustart routine to queue request to
	 * controller (and maybe start a seek?)
	 * If (now) the controller has work to do, but is not active,
	 * call start routine to start the controller.
	 */

	if (hdactive != 0 && hdactive != iod->iod_mi) {
		(void) hdustart(iod);	  /* ??? start unit */
		hdwaiting = iod->iod_mi;  /* remember we're waiting */
		bp = &hdactive->ic_tab;
		/* test if other actually doing anything if not, then switch */
		if (bp->b_active == 0)
			(void) hdswitch(hdwaiting);
		else
			HDDEBUG(SHOW_ENQ, printf("hdstrategy: other controller (%x) active\n", hdactive));
	} else if (dp->b_active == 0) {
		(void) hdustart(iod);
		bp = &iod->iod_mi->ic_tab;
		if (bp->b_actf && bp->b_active == 0)
			(void) hdstart(iod->iod_mi);
	} else
		HDDEBUG(SHOW_ENQ, printf("adapter active\n"));
	splx(s);
}


/*
 *	Device Startup Routine:
 *	Arguments:
 *	  Pointer to device structure
 *	Function:
 *	  Eventually, position the device (ie, start a seek)
 *	  (See vaxuba/up.c for how to do this)
 *	  Queue the device to the controller
 */
hdustart(iod)
	register struct iocc_device *iod;
{
	register struct buf *bp, *dp;
	register struct iocc_ctlr *ic;

	HDDEBUG(SHOW_DEQ, printf("hdustart(%x)\n", iod));
	if (iod == 0)
		return (0);		  /* Why this check? */
	ic = iod->iod_mi;
	dp = hdutabp[iod->iod_unit];
	if ((bp = dp->b_actf) == NULL) {
		HDDEBUG(SHOW_DEQ, printf("hdustart: actf == NULL\n"));
		goto out;
	}
	/* If controller is active, set soft attention and return */

	/*
	 * If drive is already positioned, just queue the request
	 */
	if (dp->b_active) {
		HDDEBUG(SHOW_DEQ, printf("hdustart: active=%d\n", dp->b_active));
		goto done;
	}
	dp->b_active = 1;		  /* Seek in progress */
	/* Here we should do a seek, or maybe teach hdstart how to do the seek */

done:
	/*
	 * Device is ready to go.
	 * Put it on the ready queue for the controller
	 * (unless it is already there)
	 */
	if (dp->b_active != 2) {
		dp->b_forw = NULL;
		if (ic->ic_tab.b_actf == NULL)
			ic->ic_tab.b_actf = dp;
		else
			ic->ic_tab.b_actl->b_forw = dp;
		ic->ic_tab.b_actl = dp;
		dp->b_active = 2;
	}
out:
	return (0);
}


/*
 *	Controller Startup Routine:
 *	Arguments:
 *	  Pointer to controller structure
 *	Function:
 *	  Compute device-dependent parameters
 *	  Start up controller
 *	  Indicate request to i/o monitor routines
 */
hdstart(ic)
	register struct iocc_ctlr *ic;
{
	register struct buf *bp, *dp;
	register struct hdst *st;
	register struct hddevice *hdaddr = (struct hddevice *)ic->ic_addr;
	register int unit;
	register struct iocc_device *iod;

	HDDEBUG(SHOW_DEQ, printf("hdstart(%x)\n", ic));
loop:
	/*
	 * Pull a device off the controller queue.
	 * If that device has work to do, do it;
	 * otherwise check the next device in the queue.
	 * Note that hdint takes care of rotating the controller between
	 * devices that need servicing.
	 */
	if ((dp = ic->ic_tab.b_actf) == NULL)
		return (0);
	if ((bp = dp->b_actf) == NULL) {
		ic->ic_tab.b_actf = dp->b_forw;
		goto loop;
	}
	unit = drive(bp->b_dev);
	HDDEBUG(SHOW_LED, DISPLAY(minor(bp->b_dev))); /* put into LEDs */
	st = &hdst[(iod=hddinfo[unit])->iod_type]; /* get type */
	HDDEBUG(SHOW_DEQ, {
		printf("HD: HDSTART -- DEQ %s REQUEST",
		    (bp->b_flags & B_READ) ? "READ" : "WRITE");
		printf("\nblkno=%d bcount=%d baddr=0x%x",
		    bp->b_blkno, bp->b_bcount, bp->b_un.b_addr);
		printf(" bdev=0x%x", bp->b_dev);
		printf(" b_cylin=%d\n", bp->b_cylin);
	}
	);
	ic->ic_tab.b_active = 1;	  /* Mark controller active */
	if (hdactive == 0) {
		hdactive = ic;		  /* remember which one is active */
		hdaddr->INTR = DISKHD3;	  /* allow interrupts */
#if	MACH_VM
		DELAY(14);		  /* needed?? */
#else	MACH_VM
		DELAY(10);
#endif	MACH_VM
	}
	dp->b_un.b_addr = bp->b_un.b_addr;
	dp->b_dev = bp->b_dev;		  /* for error messages */
	dp->b_cylin = bp->b_cylin;	  /* current absolute cylinder */
	dp->b_blkno = bp->b_blkno;	  /* current relative block number */
	dp->b_bcount = bp->b_bcount;	  /* current byte count */
	hd_softc[unit].hd_physaddr = NO_ADDR; /* mark as not translated */
	HDDEBUG(SHOW_XFER, printf("HDSTART: starting %s\n",
	    (bp->b_flags & B_READ) ? "read" : "write"));

	if (iod->iod_dk >= 0) {
		dk_busy |= 1<<iod->iod_dk;
		dk_seek[iod->iod_dk]++;
		dk_xfer[iod->iod_dk]++;
		dk_wds[iod->iod_dk] += bp->b_bcount >>6;
	}
	hdgo(hdaddr, st, bp->b_cylin, bp->b_blkno, bp, bp->b_bcount, dp);
	return (0);
}


/*
 * routine to actually start a disk operation
 */

hdgo(hdaddr, st, cylin, blkno, bp, bcount, dp)
	register struct hddevice *hdaddr;
	register struct hdst *st;
	register struct buf *bp;
	long bcount;
	struct buf *dp;
{
	register unsigned sec;
	register int unit = drive(bp->b_dev);

	hdaddr->PCMP = (st->pcmp >> 2);
	hdaddr->CYLL = (cylin & 0xff);
	hdaddr->CYLH = ((cylin >> 8) & 0xff);

	sec = (unsigned)blkno * (unsigned)(BSIZE / NBPS(st));
	sec %= (st->nspc);

	hdaddr->INTR = DISKHD3;	  /* enables heads 8-15 */
	hdaddr->SDH = 0xA0 | (1 & unit) << 4 | (sec / st->nspt);
	hdaddr->SNO = ((sec % st->nspt) + 1);
	hdaddr->SCT = (bcount + (NBPS(st) - 1)) / NBPS(st);

	HDDEBUG(SHOW_GO, printf("HD%d (GO): cyl=%d track=%d sec=%d sct=%d\n",
	    unit, cylin, sec / st->nspt, (sec % st->nspt) + 1, hdaddr->SCT));

	hdwait(DISKBUSY | DISKRDY, DISKRDY, hdaddr);

	if (bp->b_flags & B_READ) {
		HDDEBUG(SHOW_ORDWR | SHOW_REGS,
		    printf("HDSTART: STARTING READ CMD...\n"));
		HDDEBUG(SHOW_REGS, hdstatus(bp, 0, hdaddr));
		hdaddr->CMD = DISKREAD + NORETRY(bp->b_dev);
	} else {
		HDDEBUG(SHOW_ORDWR | SHOW_REGS,
		    printf("HDSTART: STARTING WRITE CMD...\n"));
		HDDEBUG(SHOW_REGS, hdstatus(bp, 0, hdaddr));
		hdaddr->CMD = DISKWRIT + NORETRY(bp->b_dev);
		hdwait(DISKBUSY | DISKDRQ, DISKDRQ, hdaddr);
/*
 * following is for special case of hdwr which does it's
 * own adunload
 */
		if (dp)
			hdunload(bp, dp, unit, hdaddr);
	}
}


/*
 *	Interrupt Routine:
 *	  Check completion status
 *	  Indicate completion to i/o monitor routines
 *	  log errors
 *	  start next request
 * Note that the argument is not used because only one adapter can be
 * turned on at once.
 */

/*ARGSUSED*/
hdint(ctlr)
{
	register struct buf *bp, *dp;
	register struct iocc_ctlr *ic = hdactive; /* get proper controller */
	struct iocc_device *iod;
	register struct hddevice *hdaddr;
/*	register struct hdst *st= &hdst[0];	/* only used for nbps */
	register int unit;
	register int status;
	register int error;

	if (ic == 0) {
		HDDEBUG(SHOW_INTR, printf("hdint: hdactive=0 - int. ignored\n"));
		return (1);		  /* probably during hdprobe */
	}
	hdaddr = (struct hddevice *)ic->ic_addr;
	status = hdaddr->STAT;
	if (ic->ic_tab.b_active == 0) {
		HDDEBUG(SHOW_INTR, printf("HD: UNKNOWN INTERRUPT\nHD: NO I/O IN PROGRESS!!\n\n"));
		return (1);
	}
	dp = ic->ic_tab.b_actf;
	bp = dp->b_actf;
	unit = drive(bp->b_dev);

	switch (status & DISKIMASK) {

	case (DISKDRQ | DISKRDY | DISKDONE):

		if (bp->b_flags & B_READ) {
			HDDEBUG(SHOW_INTR, {
				printf("HD: DRQ INTERRUPT --> (READ)");
				printf(" b_addr=0x%x\n", dp->b_un.b_addr);
				hdstatus(dp, 0, hdaddr);
			});
			hdunload(bp, dp, unit, hdaddr);
			dp->b_blkno++;			/* current block number */
			dp->b_bcount -= NBPS(st);	/* amount left to xfer */
			if ((hd_softc[unit].hd_flag & HD_FORWARD) ||
			    dp->b_un.b_addr >= (bp->b_un.b_addr + bp->b_bcount)) {
				HDDEBUG(SHOW_ORDWR, printf("HDINT: READ REQUEST COMPLETE !!\n"));
				break;
			}
		} else {
			HDDEBUG(SHOW_INTR, {
				printf("HD: DRQ INTERRUPT --> (WRITE)");
				printf(" b_addr=0x%x\n", dp->b_un.b_addr);
				hdstatus(dp, 0, hdaddr);
			});
			hdunload(bp, dp, unit, hdaddr);
			dp->b_blkno++;			/* current block number */
			dp->b_bcount -= NBPS(st);	/* amount left to xfer */
		}
		goto done;

	case (DISKRDY | DISKDONE):
		HDDEBUG(SHOW_ORDWR, printf("HDINT: WRITE REQUEST COMPLETE !!\n"));
		dp->b_blkno++;		/* for err msgs & write forward case */
		dp->b_bcount -= NBPS(st);
		break;

	case DISKWFLT:

		if (hdst[hddinfo[unit]->iod_type].off != hd40r_sizes) {
			printf("HD: Write Fault\n");
			hdstatus(dp, (int) hdaddr->ERR, hdaddr);
			bp->b_flags |= B_ERROR;
			break;	/* this drive isn't usable but others might be */
		}
		/* fall thru for hd40r as it treats recoverable errors as write-faults */

	default:

		if (status & DISKERR) {

			error = hdaddr->ERR;
			if ((error & DISKBBLK) && (hd_softc[unit].hd_flag & HD_FORWARD) == 0 && hdbadblk(bp, dp, hdaddr))
				goto done;

			if (++ic->ic_tab.b_errcnt <= 16) {
				HDDEBUG(SHOW_RETRY,{
					printf("HD: Disk I/O Error ... retrying\n");
					hdstatus(dp, error, hdaddr); });
				if ((ic->ic_tab.b_errcnt&07) == 0) {
					HDDEBUG(SHOW_RETRY, printf("resetting drive\n"));
					hdreset(hddinfo[unit], hdaddr);
				}
				hdstart(ic);	/* retry it */
				goto done;
			}
			if ((error & DISKIDNF) && (hd_softc[unit].hd_flag & HD_FORWARD) == 0 && hdbadblk(bp, dp, hdaddr))
				goto done;	/* ID-NOT-FOUND that is a bad block */
			printf("HD: Hard Disk I/O Error\n");
			hdstatus(dp, error, hdaddr);

			bp->b_flags |= B_ERROR;
			break;
		}
		HDDEBUG(SHOW_INTR, {
			printf("HD: UNKNOWN INTERRUPT\n");
			hdstatus(dp, (int) hdaddr->ERR, hdaddr); });
		return (1);

	}
	/*
	 * operation appears complete - check to see if we are in
	 * the middle of a bad block forward operation
	 * if so then restart the rest of the original operation.
	 */
	if (hd_softc[unit].hd_flag & HD_FORWARD) {
		register struct hdst *st = &hdst[hddinfo[unit]->iod_type];

		hd_softc[unit].hd_flag &= ~HD_FORWARD; /* no longer fowarding */
		HDDEBUG(SHOW_BAD, printf("hdgo(%d,%d,%d)\n", dp->b_cylin,
		    dp->b_blkno, dp->b_bcount));
		hdgo(hdaddr, st, dp->b_cylin, dp->b_blkno, bp, dp->b_bcount, dp);
		goto done;
	}
	/*
	 *	Flag current request complete
	 */
	if (ic->ic_tab.b_active) {	  /* For when we do error recovery */

		ic->ic_tab.b_active = 0;
		ic->ic_tab.b_errcnt = 0;
		ic->ic_tab.b_actf = dp->b_forw;	/* rotate to next drive */
		dp->b_active = 0;
		dp->b_errcnt = 0;
		dp->b_actf = bp->av_forw;
		bp->b_resid = 0;	  /* XXX *//* Error recovery! */
		iodone(bp);

		/*
		 * If more work to do on this drive, restart it.
		 */
		iod = hddinfo[unit];
		if (iod->iod_dk >= 0)
			dk_busy &= ~(1<<iod->iod_dk);
		if (dp->b_actf) {
			(void) hdustart(iod);
		}

	}
	/* Soft attention for this drive gets turned off here */

	/* Check soft attention bits (real seek complete?) for other drives on this
	   controller, and start them */

	/*
	 * If the controller is not transferring, but there are devices
	 * ready to transfer, start the controller.
	 * otherwise, if the other controller is waiting for a chance
	 * then start it. (this is not very fair as the other controller
	 * can be "locked out" if the queue never is emptied).
	 */
	if (ic->ic_tab.b_actf && ic->ic_tab.b_active == 0)
		(void) hdstart(ic);
	else if (ic->ic_tab.b_actf == 0 && hdwaiting != 0)
		(void) hdswitch(hdwaiting);

done:
	hdc_softc[ic->ic_ctlr].hd_timer = 0; /* did not timeout */

	return (0);
}


/*
 *	Raw read routine:
 *	  This routine calls physio which computes and validates
 *	  a physical address from the current logical address.
 *
 *	    Arguments:
 *	      Full device number
 *	    Functions:
 *	      Call physio which does the actual raw (physical) I/O
 *	      The arguments to physio are:
 *		pointer to the strategy routine
 *		buffer for raw I/O
 *		device
 *		read/write flag
 */
hdread(dev, uio)
	register dev_t dev;
	register struct uio *uio;
{
	register int unit = drive(dev);

	HDDEBUG(SHOW_ORDWR, printf("HD: JUST ENTERED HDREAD()!!\n"));
	if (unit >= NHD)
		return (ENXIO);
	return (physio(hdstrategy, rhdbufp[unit], dev, B_READ, minphys, uio));
}


/*
 *	Raw write routine:
 *	arguments (to hdwrite):
 *	  Full device number
 *	Functions:
 *	  Call physio which does the actual raw (physical) I/O
 */
hdwrite(dev, uio)
	dev_t dev;
	register struct uio *uio;
{
	register int unit = drive(dev);

	HDDEBUG(SHOW_ORDWR, printf("HD: JUST ENTERED HDWRITE()!!\n"));
	if (unit >= NHD)
		return (ENXIO);
	return (physio(hdstrategy, rhdbufp[unit], dev, B_WRITE, minphys, uio));
}


hdopen(dev)
	dev_t dev;
{
	register int dr = drive(dev);
	register int pa = part(dev);
	register struct iocc_device *iod;

	HDDEBUG(SHOW_ORDWR, printf("HD: JUST ENTERED HDOPEN(%x)!!\n", dev));

	if (dr >= NHD || pa < 0 || pa > 7)
		return (ENXIO);
	if ((iod = hddinfo[dr]) == 0 || iod->iod_alive == 0 ||
	    hdst[iod->iod_type].off[pa].len == 0)
		return (ENXIO);

	return (0);
}


hdsetup()
{
	register int i;

	if (hdutabp[0] == 0)
		for (i = 0; i < NHD; ++i) {
			hdutabp[i] = &hdutab[i];
			rhdbufp[i] = &rhdbuf[i];
		}
}


/****************************************************************************
*                                                                           *
*       Disk error message routine					    *
*                                                                           *
*****************************************************************************/

check_error(cmd, hdaddr)
	unsigned char cmd;
	register struct hddevice *hdaddr;
{
	unsigned char status;
	unsigned char error;

	status = hdaddr->STAT;
#if	MACH_VM
	DELAY(14);
#else	MACH_VM
	DELAY(10);
#endif	MACH_VM
	if ((DISKERR & status) == 0) {
		HDDEBUG(SHOW_INIT, {
			printf("HDINIT: Disk INIT ");
			printf("CMD=0x%x ", cmd);
			hdstatus((struct buf *)0, 0, hdaddr);
		}
		);
		return (0);		  /* normal return */
	}
	error = hdaddr->ERR;
	printf("HDINIT: Disk I/O Error ");
	printf("CMD=0x%x ", cmd);
#if	MACH_VM
	DELAY(14);
#else	MACH_VM
	DELAY(10);
#endif	MACH_VM
	hdstatus((struct buf *)0, error, hdaddr);
	return (1);			  /* error return */
}


/*****************************************************************************
*                                                                            *
*       Wait for disk I/O to complete					     *
*									     *
******************************************************************************/
hdwait(mask, compare, hdaddr)
	unsigned char mask;
	unsigned char compare;
	register struct hddevice *hdaddr;
{
	register status;
	register unsigned int count = MAX_WAIT_COUNT;

	HDDEBUG(SHOW_WAIT, printf("hdwait(%x,%x,%x) ", mask, compare, hdaddr));
	for (;--count != 0;) {
		status = hdaddr->STAT;
		if ((status & mask) == compare)
			break;
#if	MACH_VM
		DELAY(6);
#else	MACH_VM
		DELAY(2);
#endif	MACH_VM
	}
	if (count == 0)
		printf("hd: adapter @ %x TIMED OUT (%x & %x != %x)\n",
			hdaddr, status, mask, compare);
	HDDEBUG(SHOW_WAIT, printf(" complete (%x)\n", status));
}


hdstatus(bp, error, hdaddr)
	register struct buf *bp;
	register int error;
	register struct hddevice *hdaddr;
{
	register int status = hdaddr->STAT;

	printf("hd: ");
	if (bp)
		printf("hd%d%c bn=%d ",
		    drive(bp->b_dev), part(bp->b_dev) + 'a', bp->b_blkno);
	if (error)
		printf("Err=0x%b ", error,
		    "\20\1MISSING-MARK\2TR000-ERROR\3CMD-ABORTED\5ID-NOT-FOUND\7DATA-CRC\10BAD-BLOCK");
	printf("Status=0x%b ", status,
	    "\20\1ERROR\2INDEX-MARK\3ECC-CORRECTION\4DATA-REQ\5SEEK-DONE\6WRITE-FAULT\7DRIVE-READY\10BUSY");
	printf("Cyl=%d ", ((hdaddr->CYLH << 8) | hdaddr->CYLL));
	printf("Hd=%d ", hdaddr->SDH & 0xF);
	printf("Sect=%d ", hdaddr->SNO);
	printf("SCT=%d ", hdaddr->SCT);
	printf("\n");
}


hdsize(dev)
	dev_t dev;
{
	int unit = drive(dev);
	register struct iocc_device *iod;
	register struct hdst *st;

	if (unit >= NHD || (iod = hddinfo[unit]) == 0 || iod->iod_alive == 0)
		return (-1);
	st = &hdst[iod->iod_type];
	return ((int)st->off[part(dev)].len);
}


hdprobe(reg)
	register caddr_t reg;
{
	register struct hddevice *hdaddr = (struct hddevice *)reg;
	register int i;
	register int err;
	static int first;
	register int result;
	hdsetup();
	HDDEBUG(SHOW_INIT, printf("hdprobe(%x) called\n", reg));
/*
 * following KLUDGE is to get around a problem where it appears
 * that only one adapter may be enabled for interrupts at a time.
 * the DELAY(n) is probably not needed but this device does not
 * act rationally.
 */
	if (first++ == 0)
		for (i = 0; hdaddr || (hdaddr = (struct hddevice *)hdstd[i++]); hdaddr = 0) {
			hdaddr->INTR = DISKDISI;
#if	MACH_VM
			DELAY(14);
#else	MACH_VM
			DELAY(10);
#endif	MACH_VM
		}
	hdaddr = (struct hddevice *)reg;
#ifdef HD_RESET
	printf("hd: attempting reset\n");
	hdaddr->INTR = DISKRSET;	  /* do the reset */
#if	MACH_VM
	DELAY(14);
#else	MACH_VM
	DELAY(10);			  /* delay at least 4.6 us */
#endif	MACH_VM
#endif
	err = hdaddr->ERR;		  /* get diagnose status */
	hdaddr->INTR = 0x00;		  /* interrupts enabled again */
	DELAY(1000);

	hdaddr->CMD = DISKDIAG;
	PROBE_DELAY(1000000);		  /* wait a bit for interrupt */
	err = hdaddr->ERR;		  /* get diagnose status */
	if (err != 01) {
		HDDEBUG(SHOW_INIT, printf("hdc: addr %x diagnose=%x ERROR\n", hdaddr, err));
	}
#if	MACH_VM
	DELAY(14);
#else	MACH_VM
	DELAY(10);			  /* allow to settle */
#endif	MACH_VM
	if (check_error(DISKDIAG, hdaddr))
		result = PROBE_BAD;
	else
		result = PROBE_OK;
#if	MACH_VM
	DELAY(14);
#else	MACH_VM
	DELAY(10);			  /* allow to settle */
#endif	MACH_VM
	hdaddr->INTR = DISKDISI;	  /* disable interrupts again */
	DELAY(500000);			  /* this delay required or ID-NOT-FOUND results 
					  */
	return (result);
}


/*
 * determine if a slave exists and if so what type of drive it is.
 * the adapter interrupt is disabled on entry, so we must disable
 * it again on exit.
 */
hdslave(iod, hdaddr)
	register struct hddevice *hdaddr;
	register struct iocc_device *iod;
{
	register int unit = iod->iod_unit;
	register struct hdst *st = &hdst[iod->iod_type]; /* XXX fix later */
	register struct buf *dp;
	register int s = spl_disk();	/* inhibit interrupts */
	register int i;
	register int result = 0;
	struct hdconfig hdconfig;
	int ncyl, ntrack, nsector;

	HDDEBUG(SHOW_INIT, printf("hdslave called iod=%x hdaddr=%x unit=%x\n", iod, hdaddr, unit));

	dp = &hdutab[unit];
	dp->b_actf = NULL;
	dp->b_actl = NULL;
	dp->b_active = 0;

	if (!hdreset(iod, hdaddr))
		goto out;		/* could not init it */
	result = 1;		  	/* we managed to restore it */

	hdrd(hdaddr, unit, CONFIG_BLOCK, sizeof(struct hdconfig), (caddr_t) & hdconfig, &hdst[0]);
	ncyl = hdconfig.conf_maxcyl + 1;
	ntrack = hdconfig.conf_lasttrack + 1;
	nsector = hdconfig.conf_lastsect;
	HDDEBUG(SHOW_CONFIG, printf("hd%d: %d cyl, %d tracks, %d sectors\n",
	    unit, ncyl, ntrack, nsector));
	for (i = 0, st = &hdst[0]; i < NHDST; ++i, ++st) {
		;
		if (ncyl == st->ncpd && ntrack == st->ntpc && nsector == st->nspt)
			break;
	}
	if (i == NHDST) {
		printf("hd%d: unknown type (%d cyl, %d tracks, %d sectors)\n",
		    unit, ncyl, ntrack, nsector);
		i = 0;		  /* assume hdinit for now */
		st = &hdst[i];	  /* point to proper place */
	} else
		printf("hd%d: %s\n", unit, hdtypes[i]);
	if (iod->iod_type != i) {	/* changed types - load adapter */
		iod->iod_type = i;
		hdreset(iod, hdaddr);
		
	}
/*
 * read in the bad block table. If it is valid then allocate space for
 * it and read in the whole thing.
 */
	{
		struct hdbadtmp hdbadtemp;	  /* temp place to read it in */
		struct hdbad *hdb = (struct hdbad *) &hdbadtemp;

		hdrd(hdaddr, unit, BAD_BLOCK_START, sizeof(struct hdbadtmp), (caddr_t)hdb, st);
		if (hdb->hddefect[0] != 'D' || hdb->hddefect[1] != 'E' ||
		    hdb->hddefect[2] != 'F' || hdb->hddefect[3] != 'E' ||
		    hdb->hddefect[4] != 'C' || hdb->hddefect[5] != 'T' ||
		    hdb->hdcount < 0 || hdb->hdcount >= MAXBADBLKS) {
			printf("hd%d: no bad block table\n", unit);
		} else {
			register int len = (hdbadtemp.hdcount + 1) << 3;

			hdb = (struct hdbad *)calloc(len);
			HDDEBUG(SHOW_BAD, printf("hd%d: %d bytes of bad block table at %x\n", unit, len, hdb));
			hdrd(hdaddr, unit, BAD_BLOCK_START, len, (caddr_t)hdb, st);
			hdbad[unit] = hdb; /* remember it */
			if (hdb->hdcount > 0 && hdb->hdmap[0].hdgood == 0)
				hdmakebad(hdb,unit,st);

		}
	}
out:
	hdaddr->INTR = DISKDISI;
	DELAY(1000);
	splx(s);			  /* restore original priority */
	return (result);
}

hdreset(iod, hdaddr)
	register struct hddevice *hdaddr;
	register struct iocc_device *iod;
{
	register struct hdst *st = &hdst[iod->iod_type]; 
	register u_char hd_sdh;
	register int i;

	DELAY(100);			  /* insurance */
	hdaddr->SCT = st->nspt;
	hdaddr->INTR = DISKHD3;	  /* enables heads 8-15 */
#if	MACH_VM
	DELAY(14);
#else	MACH_VM
	DELAY(10);			/* insurance */
#endif	MACH_VM
	hd_sdh = 0xA0 | (1 & iod->iod_slave) << 4 | (st->ntpc - 1);
	hdaddr->SDH = hd_sdh;
	hdaddr->CMD = DISKSETP;

	hdwait(DISKBUSY, 0, hdaddr);
	if ((hdaddr->STAT & DISKRDY) == 0)
		return(0);		  /* drive not ready - assume not there */
	if (check_error(DISKSETP, hdaddr))
		return(0);
	for (i = 2; i > 0; --i) {
		DELAY(1000);
		hdaddr->INTR = DISKHD3;	  /* enables heads 8-15 */
		hdaddr->SDH = hd_sdh;
		hdaddr->PCMP = (st->pcmp >> 2);
		hdaddr->CYLL = 0;
		hdaddr->CYLH = 0;
		hdaddr->SCT = 1;
		hdaddr->SNO = 0;
		hdaddr->CMD = DISKRSTR | st->step;

		hdwait(DISKBUSY, 0, hdaddr);
		DELAY(1000);
		if (!check_error(DISKRSTR, hdaddr))
			return(1);		  /* did it! */
	}
	return(0);		/* oops, could not init it */
}

hdattach(iod)
	register struct iocc_device *iod;
{
	static float mspw = .0000016097;
	HDDEBUG(SHOW_INIT, printf("hdattach called\n"));
	if (hdwstart == 0) {
		timeout(hdwatch, (caddr_t)0, hz);
		++hdwstart;
	}
	if (iod->iod_dk >= 0)
		dk_mspw[iod->iod_dk] = mspw;
	return (1);
}


/*
 * watchdog timer - checks to see if we've lost an interrupt
 * by having the timer click 10 times without being cleared
 * (by interrupt routine)
 */
hdwatch(reg)
	caddr_t reg;
{
	register struct hdc_softc *hd;
	register struct iocc_ctlr *ic;
	register int s = spl_disk();
	register int i;
	register struct buf *dp, *bp;

	for (i = 0; i < NHDC; ++i) {
		hd = &hdc_softc[i];
		if ((ic = hdminfo[i]) == 0 || ic->ic_alive == 0 ||
		    ic->ic_tab.b_active == 0) {
			hd->hd_timer = 0;
			continue;	  /* not doing anything */
		}
		if (++hd->hd_timer > HDTIMEOUT) {
			if ((dp = ic->ic_tab.b_actf) == NULL ||
					(bp = dp->b_actf) == NULL)
				continue;
			hd->hd_timer = 0;
			printf("hd%d: lost interrupt\n",drive(bp->b_dev));
			hdstatus(dp, 0, (struct hddevice *)ic->ic_addr);
			hdstart(ic);	  /* retry it */
		}
	}
	timeout(hdwatch, (caddr_t)0, hz);
	splx(s);
}

#if	MACH_VM
#define REAL_BUF_ADDR(bp,bufaddr) real_buf_addr(bp,bufaddr)
#else	MACH_VM
#define REAL_BUF_ADDR(bp,bufaddr) (((bp->b_flags & B_PHYS) == 0) ? \
	((caddr_t)vtop(bufaddr)) : real_buf_addr(bp,bufaddr))
#endif	MACH_VM

int hdmicro = 0;		/* for testing available intersector time */
/*
 * load/unload the adapter buffer
 * called for write/read when a data transfer is required.
 * adunload is called to transfer as much data as exists in
 * the current page, and then the next page address is obtained
 * and the process continues.
 */
hdunload(bp, dp, unit, hdaddr)
	register struct buf *bp, *dp;
	int unit;
	struct hddevice *hdaddr;
{
/* count remaining = end-address - current address */
	register int hdcnt = (bp->b_un.b_addr + bp->b_bcount) - dp->b_un.b_addr;
					  /* note bp != dp */
	register caddr_t addr;
	register int nleft ;
	register int flag = bp->b_flags & B_READ;
	register int s;

	DELAY(hdmicro);	
	HDDEBUG(SHOW_COUNT, printf("hdcnt=%d ", hdcnt));
	if (hdcnt > NBPS(st))
		hdcnt = NBPS(st);	  /* actual amount to transfer */
/*
* following calculation yields the number of bytes left on this page
* it will give zero when at the start of a page which skips the first
* adunload.
*/
	nleft = PGOFSET - (((int)dp->b_un.b_addr - 1) & PGOFSET);
	addr = hd_softc[unit].hd_physaddr; /* the real memory address */
	if (nleft > hdcnt)
		nleft = hdcnt;
	HDDEBUG(SHOW_COUNT, printf("nleft=%d ", nleft));
	if (nleft) {
		if (addr == NO_ADDR)
			addr = REAL_BUF_ADDR(bp, dp->b_un.b_addr); /* first time */
		xadunload(addr, hdaddr, nleft, flag);
		addr += nleft;
		dp->b_un.b_addr += nleft;
	}
	HDDEBUG(SHOW_COUNT, printf("nleft=%d ", hdcnt - nleft));
	if (nleft = hdcnt - nleft) {	  /* if more to transfer then we are now at a
					     page boundary */
		addr = REAL_BUF_ADDR(bp, dp->b_un.b_addr);
		xadunload(addr, hdaddr, nleft, flag);
		dp->b_un.b_addr += nleft;
		addr += nleft;
	}
	HDDEBUG(SHOW_COUNT, printf("nleft=%d ", NBPS(st) - hdcnt));
	if ((nleft = NBPS(st) - hdcnt))
		xadunload((caddr_t)0, hdaddr, nleft, flag); /* partial read */
	hd_softc[unit].hd_physaddr = addr; /* updated real address */
	HDDEBUG(SHOW_COUNT, printf("addr=%x\n", addr));
}


/*
 * turn off the current controller (since there are no requests for it)
 * and turn on one with requests pending (ic1).
 */
hdswitch(ic1)
	register struct iocc_ctlr *ic1;
{
#if NHDC > 1
	register struct iocc_ctlr *ic2 = hdactive; /* currently active device */
	register struct hddevice *hdaddr2 = (struct hddevice *)ic2->ic_addr;
	register struct hddevice *hdaddr1 = (struct hddevice *)ic1->ic_addr;
	register struct buf *bp;
	register int i;

	HDDEBUG(SHOW_SWITCH, printf("hdswitch: off=%x on=%x\n", hdaddr2, hdaddr1));
	hdaddr2->INTR = DISKDISI;	  /* turn off old controller */
#if	MACH_VM
	DELAY(14);
#else	MACH_VM
	DELAY(10);			  /* needed? */
#endif	MACH_VM
	i = hdaddr2->STAT;		  /* pick up status (just in case) */
#if	MACH_VM
	DELAY(14);
#else	MACH_VM
	DELAY(10);			  /* needed? */
#endif	MACH_VM
	i = hdaddr1->STAT;		  /* pick up status (just in case) */
	hdactive = 0;			  /* nothing is active yet */
	if (ic2->ic_tab.b_actf == 0)
		hdwaiting = 0;		  /* nobody can be waiting */
	else
		hdwaiting = ic2;	  /* remember that it is waiting */
	bp = &ic1->ic_tab;
	if (bp->b_actf && bp->b_active == 0)
		(void) hdstart(ic1);
	else
		printf("hdswitch: nothing to start??\n");
#endif NHDC
}


/*
 * bad block forwarding routine
 * 1. calculate actual block number from cylin and blkno
 * 2. look this up in the bad block forwarding table
 * 3. start a 1 block transfer on the replacement block
 * 4. calculate the cyl and block info for the next block after
 *	this one
 * result values:
 *	0	bad block is not in bad block table
 *	!= 0	worked (operation started on forwarded block)
 */

hdbadblk(bp, dp, hdaddr)
	register struct buf *bp, *dp;
	register struct hddevice *hdaddr;
{
	register int unit = drive(bp->b_dev);
	register struct hdbad *hdb = hdbad[unit];
	register struct hdst *st = &hdst[hddinfo[unit]->iod_type];
/*
 * bp->b_cylin contains the start cylinder (absolute)
 * bp->b_blkno contains the start block (relative)
 * 	we will calculate the partition offset by undoing the original
 *	b_cylin calculation.
 */
	register int blkno = (bp->b_cylin - bp->b_blkno / st->nspc) * st->nspc + dp->b_blkno;
	register int i;
	register int newblkno;		  /* replacement block number */

	HDDEBUG(SHOW_BAD, printf("hd%d: bad block %d\n", unit, blkno));
	newblkno = hdmapbad(hdb, blkno);
	if (newblkno == 0)
		return (0);		  /* did not find it */
	while (i = hdmapbad(hdb, newblkno))
		newblkno = i;		/* handle double mapped case */
	HDDEBUG(SHOW_BAD, printf("hd%d: bad block %d forwarded to %d\n",
	    unit, blkno, newblkno));

/*
 * If there is more data to transfer after this sector,
 * set a flag to remember that we are doing a bad-block-forward
 * and adjust the starting cylinder that hdint will use.
 * The starting sector (in dp->b_blkno) still points to the bad sector
 * and will be bumped on entry to hdint, so we don't touch it here.
 */
	if (dp->b_bcount > NBPS(st)) {
		hd_softc[unit].hd_flag |= HD_FORWARD; /* mark as doing forward */
		dp->b_cylin = (blkno + 1) / st->nspc;
	}
/*
 * in case of write we have to back off the transfer that loaded the
 * adapter ram so that we can load it again from the proper place.
 * we will mark the real address as invalid (since that is easier
 * than calculating if we really need to or not).
 * the performance penalty of this is small compared to the time taken
 * for the two seeks to pick up the replacment bad block.
 */
	if ((bp->b_flags & B_READ) == 0) {
		dp->b_un.b_addr -= NBPS(st); /* wasn't actually xfered */
		hd_softc[unit].hd_physaddr = NO_ADDR;
					  /* kill the real memory address (just in case) 
					  */
	}
	HDDEBUG(SHOW_BAD, {
		printf(" start next xfr at absolute cyl %d; relative sector %d\n",
		    dp->b_cylin, dp->b_blkno);
		hdstatus(dp, 0, hdaddr);
	}
	);

/*
 * break new block number into cyl and block number and start the
 * transfer
 */
	hdgo(hdaddr, st, newblkno / st->nspc, newblkno % st->nspc, bp, NBPS(st), dp);
	return (1);
}

/*
 * look up "blkno" in the bad block table and return the block
 * that it is mapped to, or 0 if it cannot be found or the table
 * is not valid.
 */
hdmapbad(hdb, blkno)
	register struct hdbad *hdb;
	register int blkno;
{
	register int i;
	register int newblkno = 0;

	if (hdb == 0 || hdb->hddefect[0] != 'D' || hdb->hddefect[1] != 'E' ||
	    hdb->hddefect[2] != 'F' || hdb->hddefect[3] != 'E' ||
	    hdb->hddefect[4] != 'C' || hdb->hddefect[5] != 'T' ||
	    hdb->hdcount < 0 || hdb->hdcount >= MAXBADBLKS)
		return (0);		  /* bad defect table - don't use it */
	for (i = 0; i < hdb->hdcount; ++i)
		if (hdb->hdmap[i].hdbad == blkno) {
			newblkno = hdb->hdmap[i].hdgood;
			break;
		}
	return (newblkno);
}

/*
 * read routine called to read in the configuration record
 * and the bad block records
 * note that it is ok to pass 0 for 'dp' to hdgo as it only uses
 * it for the B_WRITE case.
 */
hdrd(hdaddr, unit, block, count, addr, st)
	register int block, count;
	register caddr_t addr;
	register struct hdst *st;	  /* get type (may be unreal) */
{
	struct buf buf;			  /* only used internally in hd.c */
	register struct buf *bp = &buf;

	HDDEBUG(SHOW_CONFIG, printf("hd%d: hdrd %d (%d bytes)\n", unit, block, count));
	bp->b_dev = makedev(0, unit << 3); /* make a dev */
	bp->b_flags = B_READ;
	bp->b_un.b_addr = addr;
	bp->b_bcount = count;
	bp->b_blkno = block;
	hdgo(hdaddr, st, block / st->nspc, block, bp, count, (struct buf *)0);
	do {
		hdwait(DISKBUSY, 0, hdaddr); /* wait for busy to go away */
		if (check_error(DISKREAD, hdaddr))
			return (-1);	  /* oops, we got an error */
		hdwait(DISKBUSY | DISKDRQ, DISKDRQ, hdaddr);
		adunload(addr, hdaddr, (count > NBPS(st) ? NBPS(st) : count), B_READ);
		count -= NBPS(st);
		addr += NBPS(st);
	}
	while (count > 0);
/*
 * if not a complete block transfer unload the rest of the adapter
 * buffer
 */
	if (count < 0)
		adunload((caddr_t)0, hdaddr, -count, B_READ); /* unload the rest */
	return(0);		/* normal return */
}



#define	DBSIZE	17

/*
 * do a dump.
 * 1. go to high priority and V=R mode (just in case we aren't there
 *    already)
 * 2. reset the disk and adapter (just in case in middle of a transfer)
 * 3. write out the dump in 'DBSIZE' chunks.
 *
 * One complication is that we can't write location 0, so we write the
 * virtual address (0xe0000000) instead. The rest of the time we write
 * the physical address.
 * Another complication is the case where there is a hole in the address
 * space. We write zeros (thru adunload) in this case. 'savecore' will
 * put a hole into the file so it doesn't really hurt to do this.
 */

hddump(dev)
	dev_t dev;
{
	extern int dumpsize;	/* the size to dump */
	struct hddevice *hdaddr;
	register struct iocc_ctlr *ic; /* controller */
	char *start;		/* current core address to write */
	int num,		/* number of blocks to write */
		blk,		/* number to write this chunk */
		unit;		/* the physical unit number */
	struct partab *sizes;	/* partition table pointer */
	register struct iocc_device *iod;
	register struct hdst *st;
	int s = _spl0();	/* make sure interrupts inhibited */
	int base;		/* base address of the partition */

	SET_VR0;		/* go into V=R mode */

	unit = drive(dev);
	if (unit >= NHD)
		return (ENXIO);
	iod = hddinfo[unit];
	if (iod == 0 || iod->iod_alive == 0)
		return (ENXIO);
	ic = iod->iod_mi;
	hdaddr = (struct hddevice *)ic->ic_addr;
	if (!hdreset(iod,hdaddr))		/* reset it */
		return (EFAULT);
	num = dumpsize * NBPG / NBPS(st);	/* number of disk blocks */
	start = 0;
	st = &hdst[iod->iod_type];
	sizes = st->off;
	if (dumplo < 0 || dumplo + num >= sizes[part(dev)].len)
		return (EINVAL);
	base = sizes[part(dev)].start * st->nspc;
	HDDEBUG(SHOW_DUMP, printf("dumping from %x to %x at offset %x of dev %x\n", start, num * NBPS(st), dumplo, dev));
	while (num > 0) {
		daddr_t bn;	/* absolute block number to write at */

		blk = num > DBSIZE ? DBSIZE : num;
		bn = dumplo + base + ((int) start >> BSHIFT);
		HDDEBUG(SHOW_DUMP, printf("%x        \r",(int) start >> BSHIFT));
		if (hdwr(hdaddr, dev, bn, blk * NBPS(st), start, st) < 0)
			{
			register int i;
		/*
		 * write error:
		 * try writing one block at a time (doing bad-block
		 * forwarding as we go)
		 */
			for (i=0; i<blk; ++i)
				{
				int newblk = hdmapbad(hdbad[drive(dev)],bn);
				if (newblk == 0)
					newblk = bn;	/* not mapped */
				if (hdwr(hdaddr, dev, newblk,  NBPS(st), start, st) < 0)
					return(EIO);	/* couldn't fix it */
				bn++;
				start += NBPS(st);
				num--;
				}
			}
		else
			{
			start += blk*NBPS(st);
			num -= blk;
			}
	}
	return (0);
}

/*
 * write routine called to write the core dump.
 * note that it is ok to pass 0 for 'dp' to hdgo as it has a special
 * code for this case (and does not do the hdunload).
 * the addr == 0 is to get around the fact that adunload treats 0
 * as a special case
 */
hdwr(hdaddr, dev, block, count, addr, st)
	register int block, count;
	register caddr_t addr;
	register struct hdst *st;	  /* get type (may be unreal) */
	dev_t dev;
{
	struct buf buf;			  /* only used internally in hd.c */
	register struct buf *bp = &buf;
	register int v;			/* page number */

	HDDEBUG(SHOW_DUMPIO, printf("hd%d: hdwr %d 0x%x (%d bytes)\n", drive(dev), block, addr, count));
	if (addr == 0)
		addr = (char *) SYSBASE;
	bp->b_dev = dev;
	bp->b_flags = B_WRITE;
	bp->b_un.b_addr = addr;
	bp->b_bcount = count;
	bp->b_blkno = block;
	hdgo(hdaddr, st, block / st->nspc, block, bp, count, (struct buf *)0);
	do {
		hdwait(DISKDRQ, DISKDRQ, hdaddr);
		v = btop(addr);	/* page number */
		adunload(ishole(v) ? (caddr_t) 0 : addr,
			hdaddr, (count > NBPS(st) ? NBPS(st) : count), B_WRITE);
		hdwait(DISKBUSY, 0, hdaddr); /* wait for busy to go away */
		if (check_error(DISKWRIT, hdaddr))
			return (-1);	  /* oops, we got an error */
		count -= NBPS(st);
		addr += NBPS(st);
	}
	while (count > 0);
/*
 * if not a complete block transfer unload the rest of the adapter
 * buffer
 */
	if (count < 0)
		adunload((caddr_t)0, hdaddr, -count, B_WRITE); /* unload the rest */
	return(0);		/* normal completion */
}

/*
 * make a bad block table in simulation of what the format utility
 * this is only a stopgap measure so the resulting table is not written
 * back to the disk.
 */
hdmakebad(hdb,unit,st)
register struct hdbad *hdb;
register int unit;
register struct hdst *st;
{
	register int i;
	register int replace_block = (st->ncpd - 1) * st->ntpc * st->nspt - MAXBADBLKS;
	register int cnt = 0;

	for (i = 0; i < hdb->hdcount; ++i)
		if (hdb->hdmap[i].hdbad != 0 && hdb->hdmap[i].hdgood == 0) {
			hdb->hdmap[i].hdgood = replace_block + i;
			++cnt;
		}
	printf("hd%d: %d blocks forwarded to zero - reformat required\n", unit,cnt);
}
#endif	NHD
