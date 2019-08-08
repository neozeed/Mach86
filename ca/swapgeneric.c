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
/* $Header: swapgeneric.c,v 4.2 85/08/19 22:39:18 webb Exp $ */
/* $Source: /ibm/acis/usr/sys_ca/ca/RCS/swapgeneric.c,v $ */

/*	swapgeneric.c	6.1	83/08/05	*/


#include "../machine/pte.h"

#include "../h/param.h"
#include "../h/conf.h"
#include "../h/buf.h"
#include "../h/vm.h"
#include "../h/systm.h"
#include "../h/reboot.h"
#include "../caio/ioccvar.h"

/*
 * Generic configuration;  all in one
 */
dev_t rootdev, argdev, dumpdev;
int nswap;
struct swdevt swdevt[] = {
	{ -1, 1, 0 },
	{ 0, 0, 0 },
};
long dumplo;
int dmmin, dmmax, dmtext;


extern struct iocc_driver hdcdriver;
extern struct iocc_driver fdcdriver;
extern struct iocc_driver stcdriver;

struct genericconf {
	caddr_t gc_driver;
	char *gc_name;
	dev_t gc_root;
} genericconf[] = {
	{ (caddr_t) & hdcdriver, "hd", makedev(1, 0), },
	{ (caddr_t) & fdcdriver, "fd", makedev(3, 0), },
	{ (caddr_t) & stcdriver, "st", makedev(5, 0), },
	{ 0 },
};


setconf()
{
	register struct iocc_device *mi;
	register struct genericconf *gc;
	int unit, swaponroot = 0;

	if (boothowto & RB_ASKNAME) {
		char name[128];
retry:
		printf("root device? ");
		gets(name);
		for (gc = genericconf; gc->gc_driver; gc++)
			if (gc->gc_name[0] == name[0] &&
			    gc->gc_name[1] == name[1])
				goto gotit;
		goto bad;
gotit:
		if (name[3] == '*') {
			name[3] = name[4];
			swaponroot++;
		}
		if (name[2] >= '0' && name[2] <= '7' && name[3] == 0) {
			unit = name[2] - '0';
			goto found;
		}
		printf("bad/missing unit number\n");
bad:
		printf("use hd%%d, fd%%d, ud%%d, or st%%d\n");
		goto retry;
	}
	unit = 1; /*BJB 0->1*/
	for (gc = genericconf; gc->gc_driver; gc++) {
		for (mi = ioccdinit; mi->iod_driver; mi++) {
			if (mi->iod_alive == 0)
				continue;
			if (mi->iod_unit == 0 && mi->iod_driver ==
			    (struct iocc_driver *)gc->gc_driver) {
				printf("root on %s1\n", /*BJB 0->1*/
				    mi->iod_driver->idr_dname);
				goto found;
			}
		}
	}
	printf("no suitable root\n");
	asm("wait");
found:
	gc->gc_root = makedev(major(gc->gc_root), unit * 8);
	rootdev = gc->gc_root;
	swdevt[0].sw_dev = argdev = dumpdev =
	    makedev(major(rootdev), minor(rootdev) + 1);
	/* swap size and dumplo set during autoconfigure */
	if (swaponroot)
		rootdev = dumpdev;
}


gets(cp)
	char *cp;
{
	register char *lp;
	register c;

	lp = cp;
	for (;;) {
		c = getchar() & 0177;
		switch (c) {
		case '\n':
		case '\r':
			*lp++ = '\0';
			return;
		case '\b':
		case '#':
			lp--;
			if (lp < cp)
				lp = cp;
			continue;
		case '@':
		case 'u' & 037:
			lp = cp;
			cnputc('\n');
			continue;
		default:
			*lp++ = c;
		}
	}
}


#define KBD_DATA	0x01		  /* value for data present */
#define NONE_FLAG	0x80		  /* flag for no code defined */

getchar()
{
	register int c, ch;
	int iid;

	for (;;) {
		ch = get_kbd(&iid, 0);
		if (iid != KBD_DATA)
			continue;
		if (((c = key_scan(ch)) & NONE_FLAG) == 0)
			break;
	}
	if (c == '\r')
		c = '\n';
	cnputc(c);
	return (c);
}
