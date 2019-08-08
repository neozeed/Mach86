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
 *
 */

#ifndef lint
static char sccsid[] = "@(#)mkheaders.c	5.1 (Berkeley) 5/8/85";
#endif not lint

#if	CMU

/*
 **********************************************************************
 * HISTORY
 * 25-Jan-86  Avadis Tevanian (avie) at Carnegie-Mellon University
 *	Upgraded to 4.3.
 *
 * 25-Jul-85  Mike Accetta (mja) at Carnegie-Mellon University
 *	Fixed bug in tomacro() which neglected to verify a lower case
 *	character before converting to upper case.
 *
 **********************************************************************
 */
#endif	CMU

/*
 * Make all the .h files for the optional entries
 */

#include <stdio.h>
#include <ctype.h>
#include "config.h"
#include "y.tab.h"

headers()
{
	register struct file_list *fl;

	for (fl = ftab; fl != 0; fl = fl->f_next)
		if (fl->f_needs != 0)
			do_count(fl->f_needs, fl->f_needs, 1);
}

/*
 * count all the devices of a certain type and recurse to count
 * whatever the device is connected to
 */
do_count(dev, hname, search)
	register char *dev, *hname;
	int search;
{
	register struct device *dp, *mp;
	register int count;

	for (count = 0,dp = dtab; dp != 0; dp = dp->d_next)
		if (dp->d_unit != -1 && eq(dp->d_name, dev)) {
			if (dp->d_type == PSEUDO_DEVICE) {
				count =
				    dp->d_slave != UNKNOWN ? dp->d_slave : 1;
#if	CMU
				if (dp->d_flags)
					dev = NULL;
#endif	CMCUS
				break;
			}
			count++;
			/*
			 * Allow holes in unit numbering,
			 * assumption is unit numbering starts
			 * at zero.
			 */
			if (dp->d_unit + 1 > count)
				count = dp->d_unit + 1;
			if (search) {
				mp = dp->d_conn;
				if (mp != 0 && mp != (struct device *)-1 &&
				    mp->d_conn != (struct device *)-1) {
					do_count(mp->d_name, hname, 0);
					search = 0;
				}
			}
		}
	do_header(dev, hname, count);
}

do_header(dev, hname, count)
	char *dev, *hname;
	int count;
{
	char *file, *name, *inw, *toheader(), *tomacro();
	struct file_list *fl, *fl_head;
	FILE *inf, *outf;
	int inc, oldcount;

	file = toheader(hname);
#if	CMU
	name = tomacro(dev?dev:hname) + (dev == NULL);
#else	CMU
	name = tomacro(dev);
#endif	CMU
	inf = fopen(file, "r");
	oldcount = -1;
	if (inf == 0) {
		outf = fopen(file, "w");
		if (outf == 0) {
			perror(file);
			exit(1);
		}
		fprintf(outf, "#define %s %d\n", name, count);
		(void) fclose(outf);
		return;
	}
	fl_head = 0;
	for (;;) {
		char *cp;
		if ((inw = get_word(inf)) == 0 || inw == (char *)EOF)
			break;
		if ((inw = get_word(inf)) == 0 || inw == (char *)EOF)
			break;
		inw = ns(inw);
		cp = get_word(inf);
		if (cp == 0 || cp == (char *)EOF)
			break;
		inc = atoi(cp);
		if (eq(inw, name)) {
			oldcount = inc;
			inc = count;
		}
		cp = get_word(inf);
		if (cp == (char *)EOF)
			break;
		fl = (struct file_list *) malloc(sizeof *fl);
		fl->f_fn = inw;
		fl->f_type = inc;
		fl->f_next = fl_head;
		fl_head = fl;
	}
	(void) fclose(inf);
	if (count == oldcount) {
		for (fl = fl_head; fl != 0; fl = fl->f_next)
			free((char *)fl);
		return;
	}
	if (oldcount == -1) {
		fl = (struct file_list *) malloc(sizeof *fl);
		fl->f_fn = name;
		fl->f_type = count;
		fl->f_next = fl_head;
		fl_head = fl;
	}
	outf = fopen(file, "w");
	if (outf == 0) {
		perror(file);
		exit(1);
	}
	for (fl = fl_head; fl != 0; fl = fl->f_next) {
		fprintf(outf, "#define %s %d\n",
		    fl->f_fn, count ? fl->f_type : 0);
		free((char *)fl);
	}
	(void) fclose(outf);
}

/*
 * convert a dev name to a .h file name
 */
char *
toheader(dev)
	char *dev;
{
	static char hbuf[80];

	(void) strcpy(hbuf, path(dev));
	(void) strcat(hbuf, ".h");
	return (hbuf);
}

/*
 * convert a dev name to a macro name
 */
char *tomacro(dev)
	register char *dev;
{
	static char mbuf[20];
	register char *cp;

	cp = mbuf;
	*cp++ = 'N';
	while (*dev)
#if	CMU
		if (!islower(*dev))
			*cp++ = *dev++;
		else
#endif	CMU
		*cp++ = toupper(*dev++);
	*cp++ = 0;
	return (mbuf);
}
