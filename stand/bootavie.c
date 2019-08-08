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
 * Copyright (c) 1982 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)bootra.c	6.2 (Berkeley) 6/8/85
 */

#include "../h/param.h"
#include "../h/inode.h"
#include "../h/fs.h"
#include "../h/vm.h"
#include <a.out.h>
#include "saio.h"
#include "../h/reboot.h"

char bootprog[] = "ra(0,0)boot";

/*
 * Boot program... arguments passed in r10 and r11
 * are passed through to the full boot program.
 */

main()
{
	register howto, devtype;	/* howto=r11, devtype=r10 */
	int io;

#ifdef lint
	howto = 0; devtype = 0;
#endif
	printf("copying to ra(0,0)\n");
	io = open("ra(0,0)", 1);
	if (io >= 0)
		copyroot(io);
	printf("boot failed");
	_exit();
}

copyroot(io)
{
	register int i;
	int count;
	char *addr;

	i = 0;
	addr = (char *) 0x100000;
	while (i < 400*1024) {
		printf(".");
		count = write(io, addr, 512);
		if (count != 512) {
			printf("copy failed\n");
			_exit();
		}
		addr += 512;
		i += 512;
	}
	printf("copied!\n");
	_exit();
}
