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
/* $Header: aed.c,v 5.0 86/01/31 18:05:55 ibmacis ibm42a $ */
/* $Source: /ibm/acis/usr/sys_ca/caio/RCS/aed.c,v $ */

#include "aed.h"
#if NAED > 0
#define AEDDEBUG
#include "../h/param.h"
#include "../h/dir.h"
#include "../h/user.h"
#include "../h/proc.h"
#include "../h/conf.h"
#include "../h/errno.h"
#include "../h/buf.h"
#include "../h/ioctl.h"
#include "../h/signal.h"
#include "../h/uio.h"
#include "../ca/debug.h"
#include "../cacons/aedioctl.h"
#include "../ca/param.h"
#include "../caio/ioccvar.h"
#include "../h/tty.h"
#include "../cacons/screen_conf.h"
#include "../caio/aeddefs.h"

extern char aeddebug;
char aedstep = 0;		/* I guess */

#define ISPEED			B9600

#ifdef AEDDEBUG
#define aedrd(X,Y,Z)	{ if (aeddebug > 10)\
			    printf("aedrd %x -> %x (%d)\n",AED_MMAP+Z,X,Y);\
			    bcopy(AED_MMAP+Z, X, Y);}

#define aedwr(X,Y,Z)	{ if (aeddebug > 10)\
			    printf("aedwr %x -> %x (%d)\n",X,AED_MMAP+Z,Y);\
			    bcopy(X, AED_MMAP+Z, Y);}
#else
#define aedrd(X,Y,Z)	bcopy(AED_MMAP+Z, X, Y)
#define aedwr(X,Y,Z)	bcopy(X, AED_MMAP+Z, Y)
#endif

AED aed_info;
struct buf aedbuf;
struct tty aedsotty;
extern 

int aedstrategy();
int aedatch();
int aedprobe();

struct	iocc_device *aedinfo[NAED];
caddr_t aedstd[] = { (caddr_t)  0xf40a0000, 0 };
int aedintr();

struct iocc_driver aeddriver =
/*	probe  slave attach   dgo std name   info ... */
	{ aedprobe, 0, aedatch, 0, aedstd, "aed", aedinfo, 0, 0, aedintr, 0x4010 };


aedprobe(reg)
	caddr_t reg;
{
	register struct undevice *addr = (struct undevice *)reg;

#ifdef AEDDEBUG
	if (aeddebug)
		aed_put_mono("AEPROBE\n");
#endif AEDDEBUG
	return PROBE_NOINT;
}

aedatch(dev)
register dev_t dev;
{
	register AED *aed = &aed_info;
#ifdef AEDDEBUG
	if (aeddebug)
		aed_put_mono("AEDATCH\n");
#endif AEDDEBUG
	aed->state = 0;
	aed->pgrp = 0;
	aed->ctrl_offset = 0;
	aed->sram_loc = AED_MMAP + 0x4000;
}


aedopen()
{
	register AED *aed = &aed_info;	/* get AED state struct */
	register struct proc *p = u.u_procp;
	int dummy;

#ifdef AEDDEBUG
	if (aeddebug)
		aed_put_mono("AEDOPEN\n");
#endif AEDDEBUG
	/* Should always reset aed structure */
	/* review whether the structure is even needed! */
	AED_DELAY;
	aed->state = 0;
	aed->pgrp = 0;
	aed->ctrl_offset = 0;
	aed->sram_loc = AED_MMAP + 0x4000;
	AED_DELAY;
	aedrd(&dummy,2,AED_RESET);	/* reset AED processor */
	AED_DELAY;
	return(0);
}


aedclose()
{
	register AED *aed = &aed_info;
#ifdef AEDDEBUG
	if (aeddebug)
		aed_put_mono("AEDCLOSE\n");
#endif AEDDEBUG
	return(0);
}

	/* the following needs to be simplified */
aedread(dev,uio)
register dev_t dev;
register struct uio *uio;
{
#ifdef AEDDEBUG
	if (aeddebug)
		aed_put_mono("AEDREAD\n");
#endif AEDDEBUG
	return(physio(aedstrategy,&aedbuf,dev,B_READ,minphys,uio));
}

	/* the following needs to be simplified. */
aedwrite(dev,uio)
dev_t dev;
register struct uio *uio;
{
#ifdef AEDDEBUG
if (aeddebug)
	aed_put_mono("AEDWRITE\n");
#endif AEDDEBUG
	return(physio(aedstrategy,&aedbuf,dev,B_WRITE,minphys,uio));
}

int breakout = 0;
static int timeout = 250;

	/* the following needs to be simplified */
aedioctl(dev,cmd,data,flag)
dev_t dev;
register caddr_t data;
{
	register AED *aed = &aed_info;
	int dummy;
	int counter;
	short poll;

#ifdef AEDDEBUG
	if (aeddebug)
		aed_put_mono("AEDIOCTL\n");
#endif AEDDEBUG
	switch (cmd) {
	case AED_BEEP:
		beep();		/* kernel beep routine. in cons.c ? */
		break;
	case AEDGET_CTRL_LOC:
		/* Report current offset of AED control store pointer */
		*(long *)data = (long)aed->ctrl_offset;
		break;
	case AEDSET_CTRL_LOC:
		/* set offset of AED control store pointer */
		if (*(long *)data < 0 || *(long *)data >= CTRL_STORE_SIZE)
			return(EINVAL);
		(long)aed->ctrl_offset = *(long *)data;
		break;
	case AEDGET_SRAM_LOC:
		/* report address of AED shared RAM */
		*(long *)data = (long)aed->sram_loc;
		break;
	case AEDRESET:
		/* reset AED processor */
		AED_DELAY;
		aedrd(&dummy,2,AED_RESET);
		AED_DELAY;
		break;
	case AEDSTART:
		/* start AED processor */
		AED_DELAY;
		aedrd(&dummy,2,AED_START);
		AED_DELAY;
		break;
	case AEDSTOP:
		/* stop AED processor */
		AED_DELAY;
		aedrd(&dummy,2,AED_STOP);
		AED_DELAY;
		break;
	case AEDSTATE:
		/* report AED state variable */
		*(unsigned long *)data = aed->state;
		break;
	case AEDSEM_WHILE:
		/*  AED semaphore and read until it changes */
		AED_DELAY;
		counter = 0;
		do {
			AED_DELAY;
			aedrd(&poll,2,AED_SEMAPHORE);	/* read semaphore */
			AED_DELAY;
			if (breakout) break;
		} while ((poll == *(short *)data) && (counter++ < timeout));
		breakout = 0;
		AED_DELAY;
		*(short *)data = poll;
		if (counter >= timeout) return(EINVAL);  /* time-out */
		break;
	case AEDSEM_READ:
		/* read AED semaphore */
		AED_DELAY;
		aedrd((short *)data,2,AED_SEMAPHORE);	/* read semaphore */
		break;
	case AEDSEM_SET:
		/* set AED semaphore */
		AED_DELAY;
		aedwr((short *)data,2,AED_SEMAPHORE);	/* set semaphore */
		break;
	case AEDSEM_TIMEOUT:
		timeout = *(int *)data;
		break;
	case AEDSEM_UNTIL:
		/*  AED semaphore and read until it matches data */
		AED_DELAY;
		counter = 0;
		do {
			AED_DELAY;
			aedrd(&poll,2,AED_SEMAPHORE);	/* read semaphore */
			AED_DELAY;
			if (breakout) break;
		} while ((poll != *(short *)data) && (counter++ < timeout));
		breakout = 0;
		AED_DELAY;
		*(short *)data = poll;
		if (counter >= timeout) return(EINVAL);  /* time-out */
		break;
	case AEDSEM_SET_WAIT:
		/* set AED semaphore and read until it has changed */
		if (*(short *)data == 0)
			return(EINVAL);
		AED_DELAY;
		aedwr((short *)data,2,AED_SEMAPHORE);	/* set semaphore */
		counter = 0;
		do {
			AED_DELAY;
			aedrd(&poll,2,AED_SEMAPHORE);	/* read semaphore */
			AED_DELAY;
			if (breakout)
				break;
		} while ((poll == *(short *)data) && (counter++ < timeout));
		breakout = 0;
		AED_DELAY;
		*(short *)data = poll;
		if (counter >= timeout) return(EINVAL);  /* time-out */
		break;
	case AED_TOG_DEBUG:
		/* toggle debugging */
		aeddebug = !aeddebug;
		break;
	case AEDDELAY:
		AED_DELAY;
		break;
	case AED_LEDS:
		AED_DELAY;
		display(*(int *)data);
		AED_DELAY;
		break;
	default:
		uprintf("aedioctl: Unknown cmd (%x)\n",cmd);
		return(EINVAL);
	}
	return(0);
}

aedstrategy(aedbp)
register struct buf *aedbp;	/* pointer to user buffer */
{
	register AED *aed = &aed_info;	/* get address of AED info struct */
	register long loc;

	loc = aed->ctrl_offset;	/* get pointer to AED control store */

	if (aedbp->b_flags & B_READ)
	{
#ifdef AEDDEBUG
if (aeddebug>10)
	printf("AEDSTRATEGY: read (%d) at loc (%x)\n",aedbp->b_bcount, loc);
#endif AEDDEBUG
		aedrd(aedbp->b_un.b_addr,aedbp->b_bcount, loc);
	} else {
#ifdef AEDDEBUG
if (aeddebug>10)
	printf("AEDSTRATEGY: write (%d) at loc (%x)\n",aedbp->b_bcount, loc);
#endif AEDDEBUG
		aedwr(aedbp->b_un.b_addr, aedbp->b_bcount, loc);
	}

	aed->ctrl_offset += aedbp->b_bcount; /* adjust control store pointer */
	iodone(aedbp);
	return(0);
}


#define CONSTTYHOG 4095

aed_ttyinput(c,tp,fromuser)
register c;
register struct tty *tp;
{
	register AED *aed = &aed_info;
	register struct tty *aedtp = &aedsotty;
#ifdef AEDDEBUG
	if (aeddebug)
		aed_put_mono("AED_TTYINPUT\r\n");
#endif AEDDEBUG
	if (fromuser)
	{
		aed_put_mono("AED_TTYINPUT_mfsr\r\n");
		while (aedtp->t_rawq.c_cc > TTYHOG)
			sleep((caddr_t)&aedtp->t_rawq,TTOPRI);
		while (putc(c,&aedtp->t_rawq) < 0)
			sleep((caddr_t)&aedtp->t_rawq,TTOPRI);
		wakeup((caddr_t)&aedtp->t_rawq);
	} else {
		if ((putc(c,&aedtp->t_rawq) < 0) || 
			(aedtp->t_rawq.c_cc > CONSTTYHOG))
			{
			aed->state |= AED_LOADING_CODE;
			aed_screen_init();
			screen_sw[CONS_AED].flags |= CONSDEV_INIT;	/* now initialized */
			aed->state |= AED_RUN_CODE;
			aed->state &= ~(AED_LOADING_CODE + AED_WM_MODE);
			rawq_to_aed(aedtp);	/* flush queue to AED screen */
			gsignal(aed->pgrp,SIGINT); /* signal process group */
		} else
			wakeup((caddr_t)&aedtp->t_rawq);
			
	}
	return;
}


aed_ttread(tp, uio)
register struct tty *tp;
struct uio *uio;
{
	register struct tty *aedtp = &aedsotty;
	int error = 0;
#ifdef AEDDEBUG
	if (aeddebug)
		aed_put_mono("AED_TTREAD\r\n");
#endif AEDDEBUG
	if (aedtp->t_rawq.c_cc <= 0)
		sleep((caddr_t)&aedtp->t_rawq, TTIPRI);

	while (!error && aedtp->t_rawq.c_cc && uio->uio_resid)
		error = ureadc(getc(&aedtp->t_rawq), uio);

	/* wakeup queue sleepers for more input */
	wakeup((caddr_t)&aedtp->t_rawq);   

	return(error);
}


aedsoatch(dev)
register dev_t dev;
{
register struct tty *tp = &aedsotty;

#ifdef AEDDEBUG
	if (aeddebug)
		aed_put_mono("AEDSOATCH\n");
#endif AEDDEBUG
	tp->t_state = 0;
	ttychars(tp);
	tp->t_ospeed = tp->t_ispeed = ISPEED;
	tp->t_flags = 0;
	tp->t_line = AEDSO_DISC;	/* use AED line discipline */
}


aedsoopen(dev,flag)
register dev_t dev;
register int flag;
{
register struct tty *tp = &aedsotty;

#ifdef AEDDEBUG
	if (aeddebug)
		aed_put_mono("AEDSOOPEN\r\n");
#endif AEDDEBUG
	/* Always should reset variables */
	aedsoatch(0);

	tp->t_oproc = 0;
	tp->t_ospeed = tp->t_ispeed = ISPEED;
	tp->t_state = TS_ISOPEN | TS_CARR_ON;
	tp->t_flags = 0;
	return((*linesw[tp->t_line].l_open)(dev,tp));
}



aedsoclose(dev)
register dev_t dev;
{
	register struct tty *tp = &aedsotty;
#ifdef AEDDEBUG
	if (aeddebug)
		aed_put_mono("AEDSOCLOSE\r\n");
#endif AEDDEBUG
	tp->t_state = 0;
	return(0);
}


aedsoread(dev,uio)
register dev_t dev;
register struct uio *uio;
{
register struct tty *tp = &aedsotty;

#ifdef AEDDEBUG
	if (aeddebug)
		aed_put_mono("AEDOSREAD\r\n");
#endif AEDDEBUG
	return(aed_ttread(tp,uio));
}


rawq_to_aed(tp)
register struct tty *tp;
{
	register int c;
#ifdef AEDDEBUG
	char *string = "RAWQ_TO_AED\r\n";
	if (aeddebug)
		aed_put_mono("RAWQ_TO_AED\r\n");
#endif AEDDEBUG
	while ((c = getc(&tp->t_rawq)) != -1)
		aed_screen_putc(c);
}


/*
 * put characters on mono screen if it exists; otherwise forget about
 * it.
 */
aed_put_mono(s)
register char *s;
{
#include "mono.h"
#if NMONO > 0
	while (*s)
		mono_screen_putc(*s++);
#endif NMONO
if (aeddebug && aedstep)
	aedstop();
}

aedstop()
{}

aedintr()
{
return(1);		/* not us */
}
#endif NAED
