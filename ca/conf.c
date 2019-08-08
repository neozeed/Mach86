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
/* $Header: conf.c,v 4.7 85/09/05 21:12:05 webb Exp $ */
/* $Source: /ibm/acis/usr/sys_ca/ca/RCS/conf.c,v $ */

#ifndef lint
static char *rcsid = "$Header: conf.c,v 4.7 85/09/05 21:12:05 webb Exp $";
#endif

#if CMU
/***********************************************************************
 * HISTORY
 * 25-Feb-86  Bill Bolosky (bolosky) at Carnegie-Mellon University
 *	Added include of ioctl.h.
 *
 */
#endif CMU

/*     conf.c  6.1     83/07/29        */
#include "../h/param.h"
#include "../h/systm.h"
#include "../h/buf.h"
#include "../h/ioctl.h"
#include "../h/tty.h"
#include "../h/conf.h"

int nulldev();
int nodev();

#include "dk.h"
#if NDK > 0
int dkinit(), dkstrategy();
#else NDK
#define dkstrategy nodev
#define dkinit	nodev
#endif NDK

#include "hd.h"
#if NHD > 0
int hdinit(), hdstrategy(), hdread(), hdwrite(), hdopen(), hddump(), hdsize();
#else NHD
#define hdinit	nodev
#define hdstrategy	nodev
#define hdread	nodev
#define hdwrite	nodev
#define hdopen	nodev
#define hddump	nodev
#define hdsize	nodev
#endif NHD

#include "ud.h"
#if NUD > 0
int udinit(), udstrategy(), udread(), udwrite(), udopen();
#else NUD
#define udinit	nodev
#define udstrategy	nodev
#define udread	nodev
#define udwrite	nodev
#define udopen	nodev
#endif NUD

#include "fd.h"
#if NFD > 0
int fdinit(), fdstrategy(), fdread(), fdwrite(), fdopen(), fdclose(), fdsize(),fdioctl();
#else NFD
#define fdinit	nodev
#define fdstrategy	nodev
#define fdread	nodev
#define fdwrite	nodev
#define fdopen	nodev
#define fdclose	nodev
#define fdioctl	nodev
#define fdsize	nodev
#endif NFD

int swstrategy(), swread(), swwrite();

#include "st.h"
#if NST > 0
int stinit(), ststrategy(), stread(), stwrite(), stopen(), stclose(), stsize(), stioctl();
#else NST
#define stinit	nodev
#define ststrategy	nodev
#define stread	nodev
#define stwrite	nodev
#define stopen	nodev
#define stclose	nodev
#define stioctl	nodev
#define stsize	nodev
#endif NST

struct bdevsw bdevsw[] = {
	/* open		close		strat		dump
	  size		flags */
	{ nulldev,	nulldev,	dkstrategy,	nodev,	/*0*/
	  0,		0 },
	{ hdopen,	nulldev,	hdstrategy,	hddump,	/*1*/
	  hdsize,	0 },
	{ udopen,	nulldev,	udstrategy,	nodev,	/*2*/
	  0,		0 },
	{ fdopen,	fdclose,	fdstrategy,	nodev,	/*3*/
	  0,		0 },
	{ nodev,	nodev,		swstrategy,	nodev,	/*4*/ 
	  0,		0 },
	{ stopen,	stclose,	ststrategy,	nodev,	/*5*/
	  0,		0 },
};
int nblkdev = sizeof(bdevsw) / sizeof(bdevsw[0]);

int cnopen(), cnclose(), cnread(), cnwrite(), cnioctl();
struct tty cons;

#include "asy.h"
#if NASY > 0
int asyopen(), asyclose(), asyread(), asywrite(), asyioctl();
struct tty asy[];
#else
#define		asyopen		nodev
#define		asyclose	nodev
#define		asystrategy	nodev
#define		asyread		nodev
#define		asywrite	nodev
#define		asyioctl	nodev
#define		asy	0
#endif

int syopen(), syread(), sywrite(), syioctl(), syselect();

int mmread(), mmwrite();
#define        mmselect        seltrue

#include "pty.h"
#if NPTY > 0
int ptsopen(), ptsclose(), ptsread(), ptswrite(), ptsstop();
int ptcopen(), ptcclose(), ptcread(), ptcwrite(), ptcselect();
int ptyioctl();
struct tty pt_tty[];
#else
#define ptsopen         nodev
#define ptsclose        nodev
#define ptsread         nodev
#define ptswrite        nodev
#define ptcopen         nodev
#define ptcclose        nodev
#define ptcread         nodev
#define ptcwrite        nodev
#define ptyioctl        nodev
#define pt_tty          0
#define ptcselect       nodev
#define ptsstop         nulldev
#endif

#include "lp.h"
#if NLP > 0
int lpopen(), lpclose(), lpwrite();
#else
#define lpopen nodev
#define lpclose nodev
#define lpwrite nodev
#endif

#include "psp.h"
#if NPSP > 0
int pspopen(), pspclose(), pspread(), pspwrite(), pspioctl();
struct tty psp[];
#else
#define		pspopen		nodev
#define		pspclose	nodev
#define		pspstrategy	nodev
#define		pspread		nodev
#define		pspwrite	nodev
#define		pspioctl	nodev
#define		psp	0
#endif

#include "aed.h"
#if NAED > 0
int aedopen(), aedclose(), aedread(), aedwrite(), aedioctl();
int aedsoopen(), aedsoclose(), aedsoread();
#else
#define		aedopen		nodev
#define		aedclose	nodev
#define		aedread		nodev
#define		aedwrite	nodev
#define		aedioctl	nodev
#define		aedsoopen	nodev
#define		aedsoclose	nodev
#define		aedsoread	nodev
#endif

#include "ms.h"
#if NMS > 0
int msopen(), msclose(), msread(), msioctl(), msdselect();
struct  tty ms[];
#else
#define		msopen		nodev
#define		msclose		nodev
#define		msread		nodev
#define		msioctl		nodev
#define 	ms		0
#define 	msdselect	nodev
#endif

#ifdef	VIRTUE
#include	"rfs.h"
#if NRFS > 0
int	rmtopen(), rmtclose(), rmtread(), rmtwrite(), rmtselect();
#else
#define		rmtopen	nodev
#define		rmtclose	nodev
#define		rmtread	nodev
#define		rmtwrite	nodev
#define		rmtselect	nodev
#endif
#endif	VIRTUE

int ttselect(), seltrue();

struct cdevsw cdevsw[] = {
	/* open		close		read		write
	  ioctl		stop		reset		tty
	  select	mmap	*/
	{ cnopen,	cnclose,	cnread,		cnwrite,	/*0*/
	  cnioctl,	nulldev,	nulldev,	&cons,
	  ttselect,	nodev },
	{ asyopen,	asyclose,	asyread,	asywrite,	/*1*/
	  asyioctl,	nulldev,	nulldev,	asy,
	  ttselect,	nodev },
	{ syopen,	nulldev,	syread,		sywrite,	/*2*/
	  syioctl,	nulldev,	nulldev,	0,
	  syselect,	nodev },
	{ nulldev,	nulldev,	mmread,		mmwrite,	/*3*/
	  nodev,	nulldev,	nulldev,	0,
	  mmselect,	nodev },
	{ hdopen,	nulldev,	hdread,		hdwrite,	/*4*/
	  nulldev,	nulldev,	nulldev,	0,
	  seltrue,	nodev },
	{ udopen,	nulldev,	udread,		udwrite,	/*5*/
	  nulldev,	nulldev,	nulldev,	0,
	  seltrue,	nodev },
	{ ptsopen,	ptsclose,	ptsread,	ptswrite,	/*6*/
	  ptyioctl,	ptsstop,	nodev,		pt_tty,
	  ttselect,	nodev },
	{ ptcopen,	ptcclose,	ptcread,	ptcwrite,	/*7*/
	  ptyioctl,	nulldev,	nodev,		pt_tty,
	  ptcselect,	nodev },
	{ lpopen,	lpclose,	nodev,		lpwrite,	/*8*/
	  nodev,	nulldev,	nodev,		0,
	  seltrue,	nodev },
	{ nulldev,	nulldev,	swread,		swwrite,	/*9*/
	  nodev,	nodev,		nulldev,	0,
	  nodev,	nodev },
	{ fdopen,	fdclose,	fdread,		fdwrite,	/*10*/
	  fdioctl,	nulldev,	nulldev,	0,
	  seltrue,	nodev },
	{ stopen,	stclose,	stread,		stwrite,	/*11*/
	  stioctl,	nulldev,	nulldev,	0,
	  seltrue,	nodev },
	{ pspopen,	pspclose,	pspread,	pspwrite,	/*12*/
	  pspioctl,	nulldev,	nulldev,	psp,
	  ttselect,	nodev },
	{ aedopen,	aedclose,	aedread,	aedwrite,	/*13*/
	  aedioctl,	nulldev,	nulldev,	0,
	  nodev,	nodev },
	{ aedsoopen,	aedsoclose,	aedsoread,	nodev,		/*14*/
	  nodev,	nulldev,	nulldev,	0,
	  nodev,	nodev },
	{ msopen,	msclose,	msread,		nodev,		/*15*/
	  msioctl,	nulldev,	nulldev,	ms,
	  msdselect,	nodev },
#ifdef	VIRTUE
	{ rmtopen,	rmtclose,	rmtread,	rmtwrite,	/*16*/	
	  nodev,	nodev,		nodev,		0,
	  rmtselect,	nodev },
#else
	{ nodev,	nodev,		nodev,		nodev,		/*16*/
	  nodev,	nodev,		nodev,		0,
	  nodev,	nodev },
#endif	VIRTUE
};
int nchrdev = sizeof(cdevsw) / sizeof(cdevsw[0]);

int mem_no = 3;		/* major device number of memory special file */

/*
 * Swapdev is a fake device implemented
 * in sw.c used only internally to get to swstrategy.
 * It cannot be provided to the users, because the
 * swstrategy routine munches the b_dev and b_blkno entries
 * before calling the appropriate driver.  This would horribly
 * confuse, e.g. the hashing routines. Instead, /dev/drum is
 * provided as a character (raw) device.
 */
dev_t swapdev = makedev(4, 0);

/* Romp/XT debugging switches -- may be patched during execution */

char vmdebug = 0x00;		  /* virtual memory */
char trdebug = 0x00;		  /* trap traces */
char svdebug = 0x00;		  /* svc traces */
char ttydebug = 0;		  /* TTY (async) debug */
char pspdebug = 0;		  /* psp ( planar async ) */
char cndebug = 0;		  /* CONSOLE (mono/keybrd) debug */
char iodebug = 0x00;		  /* low level i/o */
char fsdebug = 0x00;		  /* Unix file system */
char swdebug = 0x00;		  /* swapping system */
char padebug = 0x00;		  /* paging system */
char sydebug = 0x00;		  /* syncronization */

char ecdebug = 0x00;
int hddebug = 0x00;
int aeddebug = 0x00;
char autodebug = 0x00;		  /* autoconfig debug */
static int z = 0x01010101;	  /* end of debugging switches */

#ifdef ROROOT
dev_t rorootdev = ROROOT;  /* if root make it read-only */
#endif
