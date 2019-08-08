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
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#ifndef lint
static char sccsid[] = "@(#)mkswapconf.c	5.1 (Berkeley) 5/8/85";
#endif not lint

/*
 * Build a swap configuration file.
 */
#include "config.h"

#include <stdio.h>
#include <ctype.h>

swapconf()
{
	register struct file_list *fl;
	struct file_list *do_swap();

	fl = conf_list;
	while (fl) {
		if (fl->f_type != SYSTEMSPEC) {
			fl = fl->f_next;
			continue;
		}
		fl = do_swap(fl);
	}
}

struct file_list *
do_swap(fl)
	register struct file_list *fl;
{
	FILE *fp;
	char  swapname[80], *cp;
	register struct file_list *swap;
	dev_t dev;

	if (eq(fl->f_fn, "generic")) {
		fl = fl->f_next;
		return (fl->f_next);
	}
	(void) sprintf(swapname, "swap%s.c", fl->f_fn);
	fp = fopen(path(swapname), "w");
	if (fp == 0) {
		perror(path(swapname));
		exit(1);
	}
	fprintf(fp, "#include \"../h/param.h\"\n");
	fprintf(fp, "#include \"../h/conf.h\"\n");
	fprintf(fp, "\n");
	/*
	 * If there aren't any swap devices
	 * specified, just return, the error
	 * has already been noted.
	 */
	swap = fl->f_next;
	if (swap == 0 || swap->f_type != SWAPSPEC) {
		(void) unlink(path(swapname));
		fclose(fp);
		return (swap);
	}
	fprintf(fp, "dev_t\trootdev = makedev(%d, %d);\n",
		major(fl->f_rootdev), minor(fl->f_rootdev));
	fprintf(fp, "dev_t\targdev  = makedev(%d, %d);\n",
		major(fl->f_argdev), minor(fl->f_argdev));
	fprintf(fp, "dev_t\tdumpdev = makedev(%d, %d);\n",
		major(fl->f_dumpdev), minor(fl->f_dumpdev));
	fprintf(fp, "\n");
	fprintf(fp, "struct\tswdevt swdevt[] = {\n");
	do {
		dev = swap->f_swapdev;
		fprintf(fp, "\t{ makedev(%d, %d),\t0,\t%d },\t/* %s */\n",
		    major(dev), minor(dev), swap->f_swapsize, swap->f_fn);
		swap = swap->f_next;
	} while (swap && swap->f_type == SWAPSPEC);
	fprintf(fp, "\t{ 0, 0, 0 }\n");
	fprintf(fp, "};\n");
	fclose(fp);
	return (swap);
}

static	int devtablenotread = 1;
static	struct devdescription {
	char	*dev_name;
	int	dev_major;
	struct	devdescription *dev_next;
} *devtable;

/*
 * Given a device name specification figure out:
 *	major device number
 *	partition
 *	device name
 *	unit number
 * This is a hack, but the system still thinks in
 * terms of major/minor instead of string names.
 */
dev_t
nametodev(name, defunit, defpartition)
	char *name;
	int defunit;
	char defpartition;
{
	char *cp, partition;
	int unit;
	register struct devdescription *dp;

	cp = name;
	if (cp == 0) {
		fprintf(stderr, "config: internal error, nametodev\n");
		exit(1);
	}
	while (*cp && !isdigit(*cp))
		cp++;
	unit = *cp ? atoi(cp) : defunit;
	if (unit < 0 || unit > 31) {
		fprintf(stderr,
"config: %s: invalid device specification, unit out of range\n", name);
		unit = defunit;			/* carry on more checking */
	}
	if (*cp) {
		*cp++ = '\0';
		while (*cp && isdigit(*cp))
			cp++;
	}
	partition = *cp ? *cp : defpartition;
	if (partition < 'a' || partition > 'h') {
		fprintf(stderr,
"config: %c: invalid device specification, bad partition\n", *cp);
		partition = defpartition;	/* carry on */
	}
	if (devtablenotread)
		initdevtable();
	for (dp = devtable; dp->dev_next; dp = dp->dev_next)
		if (eq(name, dp->dev_name))
			break;
	if (dp == 0) {
		fprintf(stderr, "config: %s: unknown device\n", name);
		return (NODEV);
	}
	return (makedev(dp->dev_major, (unit << 3) + (partition - 'a')));
}

char *
devtoname(dev)
	dev_t dev;
{
	char buf[80]; 
	register struct devdescription *dp;

	if (devtablenotread)
		initdevtable();
	for (dp = devtable; dp->dev_next; dp = dp->dev_next)
		if (major(dev) == dp->dev_major)
			break;
	if (dp == 0)
		dp = devtable;
	sprintf(buf, "%s%d%c", dp->dev_name,
		minor(dev) >> 3, (minor(dev) & 07) + 'a');
	return (ns(buf));
}

initdevtable()
{
	char buf[BUFSIZ];
	int maj;
	register struct devdescription **dp = &devtable;
	FILE *fp;

	sprintf(buf, "../conf/devices.%s", machinename);
	fp = fopen(buf, "r");
	if (fp == NULL) {
		fprintf(stderr, "config: can't open %s\n", buf);
		exit(1);
	}
	while (fscanf(fp, "%s\t%d\n", buf, &maj) == 2) {
		*dp = (struct devdescription *)malloc(sizeof (**dp));
		(*dp)->dev_name = ns(buf);
		(*dp)->dev_major = maj;
		dp = &(*dp)->dev_next;
	}
	*dp = 0;
	fclose(fp);
	devtablenotread = 0;
}
