/*
 * Copyright (c) 1982, 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)boot.c	7.1 (Berkeley) 6/5/86
 */

#include "../h/param.h"
#include "../h/inode.h"
#include "../h/fs.h"
#include "../h/vm.h"
#include <a.out.h>
#include "saio.h"
#include "../h/reboot.h"

/*
 * Boot program... arguments passed in r10 and r11 determine
 * whether boot stops to ask for system name and which device
 * boot comes from.
 */

/* Types in r10 specifying major device */
char	devname[][2] = {
	'h','p',	/* 0 = hp */
	0,0,		/* 1 = ht */
	'u','p',	/* 2 = up */
	'h','k',	/* 3 = hk */
	0,0,		/* 4 = sw */
	0,0,		/* 5 = tm */
	0,0,		/* 6 = ts */
	0,0,		/* 7 = mt */
	0,0,		/* 8 = tu */
	'r','a',	/* 9 = ra */
	'u','t',	/* 10 = ut */
	'r','b',	/* 11 = rb */
	0,0,		/* 12 = uu */
	0,0,		/* 13 = rx */
	'r','l',	/* 14 = rl */
};
#define	MAXTYPE	(sizeof(devname) / sizeof(devname[0]))

#define	UNIX	"vmunix"
char line[100];

int	retry = 0;

main()
{
	register unsigned howto, devtype;	/* howto=r11, devtype=r10 */
	int io, i;
	register type, part, unit;
	register char *cp;
	long atol();

#ifdef lint
	howto = 0; devtype = 0;
#endif
	printf("\nBoot\n");
#ifdef JUSTASK
	howto = RB_ASKNAME|RB_SINGLE;
#else
	type = (devtype >> B_TYPESHIFT) & B_TYPEMASK;
	unit = (devtype >> B_UNITSHIFT) & B_UNITMASK;
	unit += 8 * ((devtype >> B_ADAPTORSHIFT) & B_ADAPTORMASK);
	part = (devtype >> B_PARTITIONSHIFT) & B_PARTITIONMASK;
	if ((howto & RB_ASKNAME) == 0) {
		if (type >= 0 && type <= MAXTYPE && devname[type][0]) {
			cp = line;
			*cp++ = devname[type][0];
			*cp++ = devname[type][1];
			*cp++ = '(';
			if (unit >= 10)
				*cp++ = unit / 10 + '0';
			*cp++ = unit % 10 + '0';
			*cp++ = ',';
			*cp++ = part + '0';
			*cp++ = ')';
			strcpy(cp, UNIX);
		} else
			howto = RB_SINGLE|RB_ASKNAME;
	}
#endif
	for (;;) {
		if (howto & RB_ASKNAME) {
			printf(": ");
			gets(line);
		} else
			printf(": %s\n", line);
		io = open(line, 0);
		if (io >= 0) {
			if (howto & RB_ASKNAME) {
				/*
				 * Build up devtype register to pass on to
				 * booted program.
				 */ 
				cp = line;
				for (i = 0; i <= MAXTYPE; i++)
					if ((devname[i][0] == cp[0]) && 
					    (devname[i][1] == cp[1]))
					    	break;
				if (i <= MAXTYPE) {
					devtype = i << B_TYPESHIFT;
					cp += 3;
					i = *cp++ - '0';
					if (*cp >= '0' && *cp <= '9')
						i = i * 10 + *cp++ - '0';
					cp++;
					devtype |= ((i % 8) << B_UNITSHIFT);
					devtype |= ((i / 8) << B_ADAPTORSHIFT);
					devtype |= atol(cp) << B_PARTITIONSHIFT;
				}
			}
			devtype |= B_DEVMAGIC;
			loadpcs();
			copyunix(howto, devtype, io);
			close(io);
			howto = RB_SINGLE|RB_ASKNAME;
		}
		if (++retry > 2)
			howto = RB_SINGLE|RB_ASKNAME;
	}
}

/*ARGSUSED*/
copyunix(howto, devtype, io)
	register howto, devtype, io;	/* howto=r11, devtype=r10 */
{
	struct exec x;
	register int i;
	char *addr;

	i = read(io, (char *)&x, sizeof x);
	if (i != sizeof x ||
	    (x.a_magic != 0407 && x.a_magic != 0413 && x.a_magic != 0410))
		_stop("Bad format\n");
	printf("%d", x.a_text);
	if (x.a_magic == 0413 && lseek(io, 0x400, 0) == -1)
		goto shread;
	if (read(io, (char *)0, x.a_text) != x.a_text)
		goto shread;
	addr = (char *)x.a_text;
	if (x.a_magic == 0413 || x.a_magic == 0410)
		while ((int)addr & CLOFSET)
			*addr++ = 0;
	printf("+%d", x.a_data);
	if (read(io, addr, x.a_data) != x.a_data)
		goto shread;
	addr += x.a_data;
	printf("+%d", x.a_bss);
	x.a_bss += 128*512;	/* slop */
	for (i = 0; i < x.a_bss; i++)
		*addr++ = 0;
	x.a_entry &= 0x7fffffff;
	printf(" start 0x%x\n", x.a_entry);
	(*((int (*)()) x.a_entry))();
	return;
shread:
	_stop("Short read\n");
}

/* 750 Patchable Control Store magic */

#include "../vax/mtpr.h"
#include "../vax/cpu.h"
#define	PCS_BITCNT	0x2000		/* number of patchbits */
#define	PCS_MICRONUM	0x400		/* number of ucode locs */
#define	PCS_PATCHADDR	0xf00000	/* start addr of patchbits */
#define	PCS_PCSADDR	(PCS_PATCHADDR+0x8000)	/* start addr of pcs */
#define	PCS_PATCHBIT	(PCS_PATCHADDR+0xc000)	/* patchbits enable reg */
#define	PCS_ENABLE	0xfff00000	/* enable bits for pcs */

loadpcs()
{
	register int *ip;	/* known to be r11 below */
	register int i;		/* known to be r10 below */
	register int *jp;	/* known to be r9 below */
	register int j;
	static int pcsdone = 0;
	union cpusid sid;
	char pcs[100];
	char *closeparen;
	char *index();

	sid.cpusid = mfpr(SID);
	if (sid.cpuany.cp_type!=VAX_750 || sid.cpu750.cp_urev<95 || pcsdone)
		return;
	printf("Updating 11/750 microcode: ");
	strncpy(pcs, line, 99);
	pcs[99] = 0;
	closeparen = index(pcs, ')');
	if (closeparen)
		*(++closeparen) = 0;
	else
		return;
	strcat(pcs, "pcs750.bin");
	i = open(pcs, 0);
	if (i < 0)
		return;
	/*
	 * We ask for more than we need to be sure we get only what we expect.
	 * After read:
	 *	locs 0 - 1023	packed patchbits
	 *	 1024 - 11264	packed microcode
	 */
	if (read(i, (char *)0, 23*512) != 22*512) {
		printf("Error reading %s\n", pcs);
		close(i);
		return;
	}
	close(i);

	/*
	 * Enable patchbit loading and load the bits one at a time.
	 */
	*((int *)PCS_PATCHBIT) = 1;
	ip = (int *)PCS_PATCHADDR;
	jp = (int *)0;
	for (i=0; i < PCS_BITCNT; i++) {
		asm("	extzv	r10,$1,(r9),(r11)+");
	}
	*((int *)PCS_PATCHBIT) = 0;

	/*
	 * Load PCS microcode 20 bits at a time.
	 */
	ip = (int *)PCS_PCSADDR;
	jp = (int *)1024;
	for (i=j=0; j < PCS_MICRONUM * 4; i+=20, j++) {
		asm("	extzv	r10,$20,(r9),(r11)+");
	}

	/*
	 * Enable PCS.
	 */
	i = *jp;		/* get 1st 20 bits of microcode again */
	i &= 0xfffff;
	i |= PCS_ENABLE;	/* reload these bits with PCS enable set */
	*((int *)PCS_PCSADDR) = i;

	sid.cpusid = mfpr(SID);
	printf("new rev level=%d\n", sid.cpu750.cp_urev);
	pcsdone = 1;
}
