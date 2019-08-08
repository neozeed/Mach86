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
/* $Header: fdvar.h,v 5.0 86/01/31 18:09:55 ibmacis ibm42a $ */
/* $Source: /ibm/acis/usr/sys_ca/caio/RCS/fdvar.h,v $ */

#ifndef lint
static char *rcsidfdvar = "$Header: fdvar.h,v 5.0 86/01/31 18:09:55 ibmacis ibm42a $";
#endif

#define	b_cylin	b_resid
#define	FD_SPL()	_spl4()			/* CPU level 4 */
#define	FDUNIT(dev)	(minor(dev))
#define	FDBPS		512			/* # bytes per sector */
#define	FDSECSIZE	FDBPS / 256
#define	FDWAITTIME 	300			/* Time until FDWAIT gives up */
#define	B_CTRL		0x80000000		/* control (format) request */
#define	B_SETUP		0x40000000		/* read drive status (open) */
#define	FDDBLSTEP	2			/* Double step cylinder */
#define	FDSECBASE	0xff			/* magic base for sector size*/
#define	FDFORMREQ	0x0b			/* magic flgs for fdformat */
#define	FDWAITREAD	0xff			/* read results even if err */
#define	FDSETDELAY	hz/8			/* delay after setup */
#define	FDHDSETTLE	hz/1000*18		/* 18 ms (head settle time) */
#define	FDPRI		PZERO+5
#define	FDMAXTYPE	2			/* max floppy type (index) */
#define	FD360K360K	2			/* index for 360K in 360K drive */
#define	FDCTRLMASK	0xfc			/* Mask out slave number */
#define	FDOTHERUNIT(unit) (unit ^ 0x01)		/* get the unit of the other */
						/* slave on the current ctlr */
#define	BSIZE		DEV_BSIZE

/* per-controller data */
struct	fd_ctlr {
	int	fdc_state;	/* controller state */
#define	FDS_READ	1	/* read started */
#define	FDS_WRITE	2	/* write started */
#define	FDS_FORMAT	3	/* format started */
#define	FDS_IDLE	4	/* device is idle */
#define	FDS_SEEK	5	/* device is seeking */
#define	FDS_SETUP	6	/* set up for density */
#define	FDS_DONE	7	/* done with setup */
#define	FDS_SKDON	8	/* done with seek */
#define	FDS_RECAL	9	/* done with recalibrate */
	int	fdc_tocnt;	/* for watchdog routine */
#define	FD_MAXTIMEOUT	10	/* # seconds to wait before giving up */
	char	fdc_ctrl;	/* last control command sent to the ctlr */
	int	fdc_lstdens;	/* last density set by motor on */
	int	fdc_drives;	/* the number of open drives on ctlr */
};

/* per-drive data */
struct 	fd_softc {
	int	sc_flags;	/* drive status flags */
#define	FDF_DEVTYPE	0x07	/* mapping flags */
#define	FDF_LOCK	0x10	/* exclusive use */
#define	FDF_FORMAT	0x80	/* format in progress */
#define	FDF_TIMEOUT	0x200	/* Device has timed out waiting for intr */
	int	sc_open;	/* count number of opens */
	int	sc_dens;	/* density of floppy (0=360K) (1=1.2M) */
	int	sc_motor;	/* Motor on, time until motor is off */
#define	FDMOTORWAIT	3	/* delay before motor off */
	int	sc_error;	/* Low level routine error flage */
	int	sc_errcnt;	/* Autodensity error count flag XXX */
	int	sc_retries;	/* Retry count in Autodensity XXX */
#define	FDRETRIES	12	/* number of disk retries for autodensity */
	int	sc_drive;	/* floppy drive type */
#define	FDUNKNOWN	0
#define	FD1200K		1
#define	FD360K		2
	int	sc_sr0;		/* Adapter status register 0 */
#define	FDSR0	"\20\10Invalid\7ComNotDon\6SeekEnd\5NoTrack0\4NotReady\3HeadAddr\2Unit2\1Unit1"
#define	FDSR0MASK	0xc0
	int	sc_sr1;		/* Adapter status register 1 */
#define	FDSR1 "\20\10EndOfCyl\7NU\6DataCRCError\5Overrun\4NU\3IdNotFound\2NoWrite\1MissAddrMark"
	int	sc_sr2;		/* Adapter status register 2 */
#define	FDSR2 "\20\10NU\7CtrlMark\6DataField\5WrongCyl\4ScanEqualHit\3ScanNotSat\2BadCyl\1MissAddr"
	int	sc_sr3;		/* Adapter status register 3 */
#define	FDSR3 "\20\10Fault\7WriteProt\6Ready\5Track0\4TwoSided\3Head1\2Unit2\1Unit1"
#define	FDTRAK0		0x10
#define	FDNOWRITE	0x40
#define	FDREADY		0x20
	int	sc_curcyl;	/* Current cylinder */
	int	sc_hrdcyl;	/* Cylinder position reported by adapter */
	int	sc_hrdhd;	/* Head position reported by adapter */
	int	sc_hrdsc;	/* Sector position reported by adapter */
	int	sc_size;	/* Sector size reported by adapter */
	/*
	 * The rest of this structure is used to 
	 * store temporaries while simulating multi 
	 * sector transfers
	 */
	caddr_t	sc_uaddr;	/* Save starting xfer address */
	long	sc_bcnt;	/* total transfer count */
	long	sc_resid;	/* no. of bytes left to transfer */
	long	sc_trunc;	/* number of bytes that have been truncated */
	char	sc_cyl;		/* requested cylinder # */
	char	sc_sec;		/* requested sector # */
	char	sc_head;	/* requested head # */
	int	sc_maxbyt;	/* number of bytes left on track */
};

/*
 * Drive description table.
 */
struct 	fdst {
	short	nsect;		/* # sectors/track */
	short	ntrak;		/* # surfaces or heads */
	short	nspc;		/* # sectors/cylinder */
	short	ncyl;		/* # cylinders */
	long	nbpd;		/* # bytes per disk */
	char	gpl;		/* Gap length */
	char	fgpl;		/* Format Gap length */
	char	step;		/* Stepping rate and head unload time */
	char	xfer;		/* Data transfer rate */
	int	drive;		/* Drive type */
};
struct 	fdformdata {
	char	fdtrack;	/* track to format */
	char	fdhead;		/* head to format */
	char	fdsect;		/* sector to format */
	char	fdsize;		/* sector size */
};

struct 	statestring {
	char	value[7];
	} statestring[] = {
	{"Zero"},
	{"Read"},
	{"Write"},
	{"Format"},
	{"Idle"},
	{"Seek"},
	{"Setup"},
	{"Done"},
	{"SkDone"},
	{"Recal"}
	};
