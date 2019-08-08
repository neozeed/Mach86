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
/* $Header: ioccvar.h,v 5.0 86/01/31 18:11:15 ibmacis ibm42a $ */
/* $Source: /ibm/acis/usr/sys_ca/caio/RCS/ioccvar.h,v $ */

#if !defined(lint) && !defined(LOCORE) && defined(RCS_HDRS)
static char *rcsidioccvar = "$Header: ioccvar.h,v 5.0 86/01/31 18:11:15 ibmacis ibm42a $";
#endif

/*	ioccvar.h	6.1	83/08/13	*/

/*
 * This file contains definitions related to the kernel structures
 * for dealing with the iocc subsystem.
 *
 * There is one iocc_hd structure per dma channel.
 * Each adapter on the I/O bus which can control more than one device
 * (such as the hard disk and floppy disk adapters) has a iocc_ctlr structure.
 * Each device on an adapter has a iocc_device structure.
 */

#ifndef LOCORE

/*
 * Per dma channel structure.
 *
 * This structure hold pointers to dma registers, the state of the channel,
 * and TCW addresses.
 */
struct	iocc_hd {
	caddr_t	hd_addr;		/* dummy to keep config happy */
	struct dma_8bit_device  *dh_8237_reg; /* address of channel registers */
	struct tcw_reg *dh_pagetcw;	/* tcw page mode */
	struct tcw_reg *dh_regiontcw;	/* tcw region mode */
	short	dh_channel;		/* channel # */
	short	dh_subchan;		/* subchannel on the 8237 chip */
	short	dh_width;		/* 8 or 16 bit channel */
	short	dh_state;		/* state of this channel */
	struct	iocc_ctlr *dh_actf;	/* head of queue to transfer */
	struct  iocc_ctlr *dh_actl;	/* tail of queue to transfer */
};

/*
 * Per-controller structure.
 * (E.g. one for each hard disk adapter, and other things
 * that control more than one device.)
 *
 * Unix calls controllers what IBM calls adapters.  The words are
 * (almost) interchangeable.
 *
 * If an adapter has devices attached, then there are
 * cross-referenced iocc_device structures.
 * This structure is the one which is queued in iocc resource wait,
 * and saves the information about iocc resources which are used.
 * The queue of devices waiting to transfer is also attached here.
 */
struct iocc_ctlr {
	struct	iocc_driver *ic_driver;
	short	ic_ctlr;	/* controller index in driver */
	short	ic_alive;	/* controller exists */
	caddr_t	ic_addr;	/* address of device in i/o space */
	int	ic_irq;		/* the IRQ level of the device */
	struct	iocc_hd *ic_hd; /* link back to the header */
	int	ic_ioinfo;	/* save iocc registers, etc */
	struct	buf ic_tab;	/* queue of devices for this controller */
	short	ic_channel;		/* dma channel number */
	char	ic_party;		/* dma 1st or 3rd */
	char	ic_transfer;		/* dma transfer type */
	int	ic_cmd;			/*  communication to dgo ??? */
/* This is the forwrd link in a list of controllers on a dma channel */
	struct iocc_ctlr *ic_forw;	/* new for dma */
};

/*
 * Per ``device'' structure.
 * (A controller has devices or uses dma).
 * (Everything else is a ``device''.)
 *
 * If a controller has many drives attached, then there will
 * be several iocc_device structures associated with a single iocc_ctlr
 * structure.
 *
 * This structure contains all the information necessary to run
 * a non-dma device such as an asy card.  It also contains information
 * for slaves of iocc controllers as to which device on the slave
 * this is.  A flags field here can also be given in the system specification
 * and is used to tell which asy lines are hard wired or other device
 * specific parameters.
 */
struct iocc_device {
	struct	iocc_driver *iod_driver; /* corresponding driver struct */
	short	iod_unit;	/* unit number on the system */
	short	iod_ctlr;	/* mass ctlr number; -1 if none */
	short	iod_slave;	/* slave on controller */
	caddr_t	iod_addr;	/* address of device in i/o space */
	short	iod_irq;	/* interrupt request level */
	short	iod_dk;		/* if init 1 set to number for iostat */
	int	iod_flags;	/* parameter from system specification */
	short	iod_alive;	/* device exists */
	short	iod_type;	/* driver specific type information */
	caddr_t	iod_physaddr;	/* phys addr, for standalone (dump) code */
/* this is the forward link in a list of devices on a controller */
	struct	iocc_device *iod_forw;
/* if the device is connected to a controller, this is the controller */
	struct	iocc_ctlr *iod_mi;
	struct	iocc_hd *iod_hd;
};

/*
 * Per-driver structure.
 *
 * Each iocc driver defines entries for a set of routines
 * as well as an array of types which are acceptable to it.
 * These are used at boot time by the configuration program.
 */
struct iocc_driver {
	int	(*idr_probe)();		/* see if a driver is really there */
	int	(*idr_slave)();		/* see if a slave is there */
	int	(*idr_attach)();	/* setup driver for a slave */
	int	(*idr_dgo)();		/* fill csr/ba to start transfer */
	caddr_t	*idr_addr;		/* device csr addresses */
	char	*idr_dname;		/* name of a device */
	struct	iocc_device **idr_dinfo;/* backpointers to iodinit structs */
	char	*idr_mname;		/* name of a controller */
	struct	iocc_ctlr **idr_minfo;	/* backpointers to iominit structs */
	int	(*idr_intr)();		/* interrupt service routine */
	int	idr_csr;		/* offset to read/write csr */
};

#ifdef KERNEL
/*
 * IOCC related kernel variables
 */
struct	iocc_hd iocc_hd[];

/*
 * Iominit and iodinit initialize the mass storage controller and
 * device tables specifying possible devices.
 */
extern	struct	iocc_ctlr iominit[];
extern	struct	iocc_device ioccdinit[];

/*
 * IOCC device address space is mapped by IOMEMmap
 * into virtual address iomem[][].
 */
extern	struct pte IOMEMmap[][512];	/* iocc device addr pte's */
extern	char iomem[][512*NBPG];		/* iocc device addr space */

#endif KERNEL
#endif !LOCORE

#define PROBE_BAD	0		/* if the probe fails */
#define PROBE_NOINT	1		/* if probe ok but no interrupt */
#define PROBE_OK	2		/* if the probe was ok (interrupt caused) */
#define PROBE_BAD_INT	-1		/* we lost or didn't get interrupt */

/*
 * delay routine for probe routines:
 * quit when the interrupt is received or if we exceed the
 * loop count
 */
#define PROBE_DELAY(n) 		\
	{ extern int int_level; register int N = (n); \
			while (--N >= 0 && int_level < 0) \
				* (char *) DELAY_ADDR = 0xff;	}
