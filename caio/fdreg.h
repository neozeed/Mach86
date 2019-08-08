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
/* $Header: fdreg.h,v 5.0 86/01/31 18:09:37 ibmacis ibm42a $ */
/* $Source: /ibm/acis/usr/sys_ca/caio/RCS/fdreg.h,v $ */

#ifndef lint
static char *rcsidfdreg = "$Header: fdreg.h,v 5.0 86/01/31 18:09:37 ibmacis ibm42a $";
#endif

struct 	fddevice {	
	unsigned char fd_digout;	/* 0:   digital output register */
	unsigned char fd_empty;		/* 1:   no register at 0xf00003f3 */
	unsigned char fd_status;	/* 2:   status register */
#define	FDSTAT "\20\10RQM\7DIO\6NDM\5CB\4DBD\3DBC\2DBB\1DAB"
#define	FDSTATDAB		0x01	/* device 0 seeking */
#define	FDSTATDBB		0x02	/* device 1 seeking */
#define	FDSTATDIO		0x40	/* data direction */
#define	FDSTATRQM		0x80	/* Request data register */
	unsigned char fd_data;		/* 3:   data register */
	unsigned char fd_fixd;		/* 4:   fixed disk register */
	unsigned char fd_digin;		/* 5:   digital input register */
#define	FDNOTREADY		0x80	/* ready bit on fd_digin */
};

/* fd adapter command defines */
#define	FDREAD			0xe6
#define	FDWRITE			0xc5
#define	FDFORMAT 		0x4d
#define	FDSPEC			0x03	/* specify command */
#define	FDREADSTAT 		0x04	/* read status register */
#define	FDSENI			0x08	/* sense interrupt command */
#define	FDREST			0x07	/* reset drive to cylinder 0 */
#define	FDSEEK			0x0f	/* seek to given cyl */

/* fd control register defines */
#define	FDRST			0xfb	/*reset adapter card */
#define	FDMOTORON(slave)	((slave?0x20:0x10)+0x04)
#define	FDMOTOROFF(slave)	(slave?0xdf:0xef)
#define	FDINTENABLE		0x08

/* other hardwar magic numbers */
#define	FDBIT2			2	/* shift to put head # in bit 2 */
#define	FDPATTERN 		0xf6	/* formatting pattern byte */
#define	FDTRACK39		0x2be	/* block # for 39th track of 360K */
#define	FDLOAD			0x02	/* head load time and bit 0=dma data
					 * mode 50ms HLT */
#define	FDMOTORSTART		500000	/* motor start time in usec (this
					 * number is very pessimistic. It is 
					 * used only at auto config time) 
					 */
