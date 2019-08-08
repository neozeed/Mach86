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
/*	boot.c	6.1	83/07/29	*/

/*
 **************************************************************************
 * HISTORY
 * 01-Jul-85  Mike Accetta (mja) at Carnegie-Mellon University
 *	CS_BOOT:  picked up the ULTRIX-32 boot options to pass in
 *	the boot unit in the second byte of R10 and the partition in
 *	the high 4 bits of R11 so so that these may be passed through
 *	to UNIX for use in determining the root device;
 *	CS_KDB:  changed copyunix() to save the symbol table after _end
 *	if bit 2 (04) was supplied in the boot flags and pass its
 *	top through to UNIX in R9.
 *
 * 20-Aug-85  Robert V Baron (rvb) at Carnegie-Mellon University
 *	Allow code to be loaded directly into shared memory, by letting
 *	the high 16 bits of "howto" (r11) specify a 64k boundary foR
 *	loading to.
 *	Pass the correct major/minor as r10 to the program that was
 *	started.
 **************************************************************************
 */

#include "../h/features.h"

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

char line[100] = "xx(0,0)vmunix";

int	retry = 0;

main()
{
	register howto, devtype;	/* howto=r11, devtype=r10 */
	int io;
#if	CS_BOOT
	int bootdev;

	bootdev = devtype;
	devtype &= 0377;
#endif	CS_BOOT

#ifdef lint
	howto = 0; devtype = 0;
#endif
	printf("\nBoot\n");
#ifdef JUSTASK
	howto = RB_ASKNAME|RB_SINGLE;
#else
	if ((howto&RB_ASKNAME)==0) {
		if (devtype>=0 && devtype<sizeof(devname)/2
		    && devname[devtype][0]) {
			line[0] = devname[devtype][0];
			line[1] = devname[devtype][1];
#if	CS_BOOT
			line[3] = ((bootdev>> 8)&0x7) + '0';
			line[5] = ((howto  >>12)&0xf) + '0';
#endif	CS_BOOT
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
		if (io >= 0)
#if	CS_BOOT
		{
		    for (devtype=0;
			 devtype<(sizeof(devname)/sizeof(devname[0]));
			 devtype++)
		    {
			if (line[0] == devname[devtype][0] &&
			    line[1] == devname[devtype][1])
			{
			    /*
			     *  If boot file has owner execute permission,
			     *  always read in symbol table.
			     */
			    if (iob[io-3].i_ino.i_mode&IEXEC)
				howto |= RB_KDB;
			    copyunix(howto,
				     (bootdev&0x80000000)
				     +
				     (devtype<<8)
				     +
				     (atoi(index(line, '(')+1)<<3)
				     +
				     atoi(index(line, ',')+1),
				     io);
			}
		    }
		    close(io);
		}
#else	CS_BOOT
			copyunix(howto, io);
#endif	CS_BOOT
		if (++retry > 2)
			howto = RB_SINGLE|RB_ASKNAME;
	}
}

/*ARGSUSED*/
#if	CS_BOOT
copyunix(howto, devtype, io)
	register howto, devtype;	/* howto=r11, devtype=r10 */
	int io;
#else	CS_BOOT
copyunix(howto, io)
	register howto, io;
#endif	CS_BOOT
{
	struct exec x;
#if	CS_KDB
	register int esym;		/* esym=r9 */
#endif	CS_KDB
	register int i;
	char *addr;
#if	CS_BOOT
	char *base = (char *) (howto & 0xffff0000); /* high 16 bits */

	addr = base;
#endif	CS_BOOT
	i = read(io, (char *)&x, sizeof x);
	if (i != sizeof x ||
	    (x.a_magic != 0407 && x.a_magic != 0413 && x.a_magic != 0410))
		_stop("Bad format\n");

#if	CS_BOOT
	if (base)
		printf("%x // ", base);
#endif	CS_BOOT
	printf("%d", x.a_text);
	if (x.a_magic == 0413 && lseek(io, 0x400, 0) == -1)
		goto shread;
#if	CS_BOOT
	if (read(io, addr, x.a_text) != x.a_text)
#else	CS_BOOT
	if (read(io, (char *)0, x.a_text) != x.a_text)
#endif	CS_BOOT
		goto shread;
#if	CS_BOOT
	addr += x.a_text;
#else	CS_BOOT
	addr = (char *)x.a_text;
#endif	CS_BOOT
	if (x.a_magic == 0413 || x.a_magic == 0410)
		while ((int)addr & CLOFSET)
			*addr++ = 0;
	printf("+%d", x.a_data);
	if (read(io, addr, x.a_data) != x.a_data)
		goto shread;
	addr += x.a_data;
	printf("+%d", x.a_bss);
#if	CS_KDB
	if ((howto&RB_KDB) && x.a_syms)
	{
		for (i = 0; i < x.a_bss; i++)
			*addr++ = 0;
		*((int *)addr) = x.a_syms;
		addr += sizeof(x.a_syms);
		printf("[+%d", x.a_syms);
		if (read(io, addr, x.a_syms) != x.a_syms)
			goto shread;
		addr += x.a_syms;
		if (read(io, addr, sizeof(int)) != sizeof(int))
			goto shread;
		i = *((int *)addr) - sizeof(int);
		addr += sizeof(int);
		printf("+%d]", i);
		if (read(io, addr, i) != i)
			goto shread;
		addr += i;
		esym = ((int)(addr+sizeof(int)-1))&~(sizeof(int)-1);
		x.a_bss = 0;
	}
	else
		howto &= ~RB_KDB;
#endif	CS_KDB
	x.a_bss += 128*512;	/* slop */
	for (i = 0; i < x.a_bss; i++)
		*addr++ = 0;
	x.a_entry &= 0x7fffffff;
#if	CS_BOOT
	printf(" start 0x%x\n", base + x.a_entry);
	(*((int (*)()) (base + x.a_entry)))();
#else	CS_BOOT
	printf(" start 0x%x\n", x.a_entry);
	(*((int (*)()) x.a_entry))();
#endif	CS_BOOT
	_exit();
shread:
	_stop("Short read\n");
}
