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
 *	@(#)drtest.c	6.2 (Berkeley) 6/8/85
 */

/*
 * Standalone program to test a disk and driver
 * by reading the disk a track at a time.
 */
#include "../h/param.h"
#include "../h/inode.h"
#include "../h/fs.h"
#include "saio.h"

#define SECTSIZ	512

extern	int end;
char	*malloc();
char	*prompt();

main()
{
	char *cp, *bp;
	int fd, tracksize, debug;
	register int sector, lastsector;
	struct st st;

	printf("Testprogram for stand-alone driver\n\n");
again:
	cp = prompt("Enable debugging (1=bse, 2=ecc, 3=bse+ecc)? ");
	debug = atoi(cp);
	if (debug < 0)
		debug = 0;
	fd = getdevice();
	ioctl(fd, SAIODEVDATA, (char *)&st);
	printf("Device data: #cylinders=%d, #tracks=%d, #sectors=%d\n",
		st.ncyl, st.ntrak, st.nsect);
	ioctl(fd, SAIODEBUG, (char *)debug);
	tracksize = st.nsect * SECTSIZ;
	bp = malloc(tracksize);
	printf("Reading in %d byte records\n", tracksize);
	printf("Start ...make sure drive is on-line\n");
	lseek(fd, 0, 0);
	lastsector = st.ncyl * st.nspc;
	for (sector = 0; sector < lastsector; sector += st.nsect) {
		if (sector && (sector % (st.nspc * 10)) == 0)
			printf("cylinder %d\n", sector/st.nspc);
		read(fd, bp, tracksize);
	}
	goto again;
}

/*
 * Prompt and verify a device name from the user.
 */
getdevice()
{
	register char *cp;
	register struct devsw *dp;
	int fd;

top:
	cp = prompt("Device to read? ");
	if ((fd = open(cp, 2)) < 0) {
		printf("Known devices are: ");
		for (dp = devsw; dp->dv_name; dp++)
			printf("%s ",dp->dv_name);
		printf("\n");
		goto top;
	}
	return (fd);
}

char *
prompt(msg)
	char *msg;
{
	static char buf[132];

	printf("%s", msg);
	gets(buf);
	return (buf);
}

/*
 * Allocate memory on a page-aligned address.
 * Round allocated chunk to a page multiple to
 * ease next request.
 */
char *
malloc(size)
	int size;
{
	char *result;
	static caddr_t last = 0;

	if (last == 0)
		last = (caddr_t)(((int)&end + 511) & ~0x1ff);
	size = (size + 511) & ~0x1ff;
	result = (char *)last;
	last += size;
	return (result);
}
