/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#ifndef lint
static char sccsid[] = "@(#)mkmakefile.c	5.2 (Berkeley) 9/17/85";
#endif not lint

#ifdef	CMU
/*
 **********************************************************************
 * HISTORY
 *  9-Jun-86  Bill Bolosky (bolosky) at Carnegie-Mellon University
 *	Added code to cause blt.o to be loaded into the romp kernel.
 *
 * 18-May-86  Avadis Tevanian (avie) at Carnegie-Mellon University
 *	Make sure that swapXXX.o depends on the inline program if
 *	building a VAX system.
 *
 * 18-Apr-86  Charlie & (root) at Carnegie-Mellon University
 *	New dependency generation code.  Have vmunix do a md -d *.n
 *	Modified .s files to use CC -E vs PP (== /lib/cpp).  This way it
 *	uses search rules to find cpp.
 *
 * 25-Jan-86  Avadis Tevanian (avie) at Carnegie-Mellon University
 *	Upgraded to 4.3.
 *
 * 17-Oct-85  Mike Accetta (mja) at Carnegie-Mellon University
 *	Changed to turn OPTIONS symbols internally into pseudo-device
 *	entries for file lines beginning with "OPTIONS/" so that
 *	include files are generated for these symbols.
 *
 * 12-Aug-85  Avadis Tevanian (avie) at Carnegie-Mellon University
 *	Modified to process files ending in ".s" with the preprocessor
 *	and assembler.
 *
 **********************************************************************
 */
#endif	CMU

/*
 * Build the makefile for the system, from
 * the information in the files files and the
 * additional files for the machine being compiled to.
 */

#include <stdio.h>
#include <ctype.h>
#include "y.tab.h"
#include "config.h"

#define next_word(fp, wd) \
	{ register char *word = get_word(fp); \
	  if (word == (char *)EOF) \
		return; \
	  else \
		wd = word; \
	}

static	struct file_list *fcur;
char *tail();

/*
 * Lookup a file, by make.
 */
struct file_list *
fl_lookup(file)
	register char *file;
{
	register struct file_list *fp;

	for (fp = ftab ; fp != 0; fp = fp->f_next) {
		if (eq(fp->f_fn, file))
			return (fp);
	}
	return (0);
}

/*
 * Lookup a file, by final component name.
 */
struct file_list *
fltail_lookup(file)
	register char *file;
{
	register struct file_list *fp;

	for (fp = ftab ; fp != 0; fp = fp->f_next) {
		if (eq(tail(fp->f_fn), tail(file)))
			return (fp);
	}
	return (0);
}

/*
 * Make a new file list entry
 */
struct file_list *
new_fent()
{
	register struct file_list *fp;

	fp = (struct file_list *) malloc(sizeof *fp);
	fp->f_needs = 0;
	fp->f_next = 0;
	fp->f_flags = 0;
	fp->f_type = 0;
	if (fcur == 0)
		fcur = ftab = fp;
	else
		fcur->f_next = fp;
	fcur = fp;
	return (fp);
}

char	*COPTS;
#ifdef	CMU
int	haverdb;		/* for ROMP */
#endif	CMU

/*
 * Build the makefile from the skeleton
 */
makefile()
{
	FILE *ifp, *ofp;
	char line[BUFSIZ];
	struct opt *op;

	read_files();
	strcpy(line, "../conf/Makefile.");
	(void) strcat(line, machinename);
	ifp = fopen(line, "r");
	if (ifp == 0) {
		perror(line);
		exit(1);
	}
	ofp = fopen(path("Makefile"), "w");
	if (ofp == 0) {
		perror(path("Makefile"));
		exit(1);
	}
	fprintf(ofp, "IDENT=-D%s", raise(ident));
	if (profiling)
		fprintf(ofp, " -DGPROF");
	if (cputype == 0) {
		printf("cpu type must be specified\n");
		exit(1);
	}
	{ struct cputype *cp;
	  for (cp = cputype; cp; cp = cp->cpu_next)
		fprintf(ofp, " -D%s", cp->cpu_name);
	}
	for (op = opt; op; op = op->op_next)
#ifdef	CMU
		{
#endif	CMU
		if (op->op_value)
			fprintf(ofp, " -D%s=\"%s\"", op->op_name, op->op_value);
		else
			fprintf(ofp, " -D%s", op->op_name);
#ifdef	CMU
		if (strcmp(op->op_name,"RDB") == 0)
			++haverdb;
		}
#endif	CMU
	fprintf(ofp, "\n");
	if (hadtz == 0)
		printf("timezone not specified; gmt assumed\n");
#ifdef	CMU
	switch(machine) {

	case MACHINE_VAX:
		if (maxusers == 0) {
			printf("maxusers not specified; 24 assumed\n");
			maxusers = 24;
		} else if (maxusers < 8) {
			printf("minimum of 8 maxusers assumed\n");
			maxusers = 8;
		} else if (maxusers > 1024) {
			printf("maxusers truncated to 1024\n");
			maxusers = 1024;
		}
		break;

	case MACHINE_SUN:
		if (maxusers == 0) {
			printf("maxusers not specified; 8 assumed\n");
			maxusers = 8;
		} else if (maxusers < 2) {
			printf("minimum of 2 maxusers assumed\n");
			maxusers = 2;
		} else if (maxusers > 32) {
			printf("maxusers truncated to 32\n");
			maxusers = 32;
		}
		break;

	case MACHINE_ROMP:
		if (maxusers == 0) {
			maxusers = 16;
			printf("maxusers not specified; %d assumed\n",maxusers);
		} else if (maxusers < 4) {
			maxusers = 4;
			printf("minimum of %d maxusers assumed\n",maxusers);
		} else if (maxusers > 32) {
			maxusers = 32;
			printf("maxusers truncated to %d\n",maxusers);
		}
		break;

	}
#else	CMU
#ifdef vax
	if (maxusers == 0) {
		printf("maxusers not specified; 24 assumed\n");
		maxusers = 24;
	} else if (maxusers < 8) {
		printf("minimum of 8 maxusers assumed\n");
		maxusers = 8;
	} else if (maxusers > 1024)
		printf("warning: maxusers > 1024 (%d)\n", maxusers);
#endif
#endif	CMU
	fprintf(ofp, "PARAM=-DTIMEZONE=%d -DDST=%d -DMAXUSERS=%d\n",
	    timezone, dst, maxusers);
	while (fgets(line, BUFSIZ, ifp) != 0) {
		if (*line == '%')
			goto percent;
		if (profiling && strncmp(line, "COPTS=", 6) == 0) {
			register char *cp;

			fprintf(ofp,
			    "GPROF.EX=/usr/src/lib/libc/%s/csu/gmon.ex\n",
			    machinename);
			cp = index(line, '\n');
			if (cp)
				*cp = 0;
			cp = line + 6;
			while (*cp && (*cp == ' ' || *cp == '\t'))
				cp++;
			COPTS = malloc((unsigned)(strlen(cp) + 1));
			if (COPTS == 0) {
				printf("config: out of memory\n");
				exit(1);
			}
			strcpy(COPTS, cp);
			fprintf(ofp, "%s -pg\n", line);
			continue;
		}
		fprintf(ofp, "%s", line);
		continue;
	percent:
		if (eq(line, "%OBJS\n"))
			do_objs(ofp);
		else if (eq(line, "%CFILES\n"))
			do_cfiles(ofp);
#if	CMU
		else if (eq(line, "%SFILES\n"))
			do_sfiles(ofp);
#endif	CMU
		else if (eq(line, "%RULES\n"))
			do_rules(ofp);
		else if (eq(line, "%LOAD\n"))
			do_load(ofp);
		else
			fprintf(stderr,
			    "Unknown %% construct in generic makefile: %s",
			    line);
	}
	(void) fclose(ifp);
	(void) fclose(ofp);
}

/*
 * Read in the information about files used in making the system.
 * Store it in the ftab linked list.
 */
read_files()
{
	FILE *fp;
	register struct file_list *tp;
	register struct device *dp;
	register struct opt *op;
	char *wd, *this, *needs, *devorprof;
	char fname[32];
#if	CMU
	int options;
	int not_option;
#endif	CMU
	int nreqs, first = 1, configdep, isdup;

	ftab = 0;
	(void) strcpy(fname, "files");
openit:
	fp = fopen(fname, "r");
	if (fp == 0) {
		perror(fname);
		exit(1);
	}
next:
#if	CMU
	options = 0;
#endif	CMU
	/*
	 * filename	[ standard | optional ] [ config-dependent ]
	 *	[ dev* | profiling-routine ] [ device-driver]
	 */
	wd = get_word(fp);
	if (wd == (char *)EOF) {
		(void) fclose(fp);
		if (first == 1) {
			(void) sprintf(fname, "files.%s", machinename);
			first++;
			goto openit;
		}
		if (first == 2) {
			(void) sprintf(fname, "files.%s", raise(ident));
			first++;
			fp = fopen(fname, "r");
			if (fp != 0)
				goto next;
		}
		return;
	}
	if (wd == 0)
		goto next;
	this = ns(wd);
	next_word(fp, wd);
	if (wd == 0) {
		printf("%s: No type for %s.\n",
		    fname, this);
		exit(1);
	}
	if ((tp = fl_lookup(this)) && (tp->f_type != INVISIBLE || tp->f_flags))
		isdup = 1;
	else
		isdup = 0;
	tp = 0;
	if (first == 3 && (tp = fltail_lookup(this)) != 0)
		printf("%s: Local file %s overrides %s.\n",
		    fname, this, tp->f_fn);
	nreqs = 0;
	devorprof = "";
	configdep = 0;
	needs = 0;
	if (eq(wd, "standard"))
		goto checkdev;
	if (!eq(wd, "optional")) {
		printf("%s: %s must be optional or standard\n", fname, this);
		exit(1);
	}
#if	CMU
	if (strncmp(this, "OPTIONS/", 8) == 0)
		options++;
	not_option = 0;
#endif	CMU
nextopt:
	next_word(fp, wd);
	if (wd == 0)
		goto doneopt;
	if (eq(wd, "config-dependent")) {
		configdep++;
		goto nextopt;
	}
#if	CMU
	if (eq(wd, "not")) {
		not_option = !not_option;
		goto nextopt;
	}
#endif	CMU
	devorprof = wd;
	if (eq(wd, "device-driver") || eq(wd, "profiling-routine")) {
		next_word(fp, wd);
		goto save;
	}
	nreqs++;
	if (needs == 0 && nreqs == 1)
		needs = ns(wd);
	if (isdup)
		goto invis;
#if	CMU
	if (options)
	{
		register struct opt *op;
		struct opt *lop = 0;
		struct device tdev;

		/*
		 *  Allocate a pseudo-device entry which we will insert into
		 *  the device list below.  The flags field is set non-zero to
		 *  indicate an internal entry rather than one generated from
		 *  the configuration file.  The slave field is set to define
		 *  the corresponding symbol as 0 should we fail to find the
		 *  option in the option list.
		 */
		init_dev(&tdev);
		tdev.d_name = ns(wd);
		tdev.d_type = PSEUDO_DEVICE;
		tdev.d_flags++;
		tdev.d_slave = 0;

		for (op=opt; op; lop=op, op=op->op_next)
		{
			char *od = raise(ns(wd));

			/*
			 *  Found an option which matches the current device
			 *  dependency identifier.  Set the slave field to
			 *  define the option in the header file.
			 */
			if (strcmp(op->op_name, od) == 0)
			{
				tdev.d_slave = 1;
				if (lop == 0)
					opt = op->op_next;
				else
					lop->op_next = op->op_next;
				free(op);
				op = 0;
			 }
			free(od);
			if (op == 0)
				break;
		}
		newdev(&tdev);
	}
 	for (dp = dtab; dp != 0; dp = dp->d_next) {
		if (eq(dp->d_name, wd) && (dp->d_type != PSEUDO_DEVICE || dp->d_slave)) {
			if (not_option)
				goto invis;	/* dont want file if option present */
			else
				goto nextopt;
		}
	}
	if (not_option)
		goto nextopt;		/* want file if option missing */
#else	CMU
	for (dp = dtab; dp != 0; dp = dp->d_next)
		if (eq(dp->d_name, wd))
			goto nextopt;
#endif	CMU
	for (op = opt; op != 0; op = op->op_next)
		if (op->op_value == 0 && opteq(op->op_name, wd)) {
			if (nreqs == 1) {
				free(needs);
				needs = 0;
			}
			goto nextopt;
		}
invis:
	while ((wd = get_word(fp)) != 0)
		;
	tp = new_fent();
	tp->f_fn = this;
	tp->f_type = INVISIBLE;
	tp->f_needs = needs;
	tp->f_flags = isdup;
	goto next;

doneopt:
	if (nreqs == 0) {
		printf("%s: what is %s optional on?\n",
		    fname, this);
		exit(1);
	}

checkdev:
	if (wd) {
		next_word(fp, wd);
		if (wd) {
			if (eq(wd, "config-dependent")) {
				configdep++;
				goto checkdev;
			}
			devorprof = wd;
			next_word(fp, wd);
		}
	}

save:
	if (wd) {
		printf("%s: syntax error describing %s\n",
		    fname, this);
		exit(1);
	}
	if (eq(devorprof, "profiling-routine") && profiling == 0)
		goto next;
	tp = new_fent();
	tp->f_fn = this;
#if	CMU
	if (options)
		tp->f_type = INVISIBLE;
	else
#endif	CMU
	if (eq(devorprof, "device-driver"))
		tp->f_type = DRIVER;
	else if (eq(devorprof, "profiling-routine"))
		tp->f_type = PROFILING;
	else
		tp->f_type = NORMAL;
	tp->f_flags = 0;
	if (configdep)
		tp->f_flags |= CONFIGDEP;
	tp->f_needs = needs;
	goto next;
}

opteq(cp, dp)
	char *cp, *dp;
{
	char c, d;

	for (; ; cp++, dp++) {
		if (*cp != *dp) {
			c = isupper(*cp) ? tolower(*cp) : *cp;
			d = isupper(*dp) ? tolower(*dp) : *dp;
			if (c != d)
				return (0);
		}
		if (*cp == 0)
			return (1);
	}
}

do_objs(fp)
	FILE *fp;
{
	register struct file_list *tp, *fl;
	register int lpos, len;
	register char *cp, och, *sp;
	char swapname[32];

	fprintf(fp, "OBJS=");
	lpos = 6;
	for (tp = ftab; tp != 0; tp = tp->f_next) {
		if (tp->f_type == INVISIBLE)
			continue;
		sp = tail(tp->f_fn);
		for (fl = conf_list; fl; fl = fl->f_next) {
			if (fl->f_type != SWAPSPEC)
				continue;
			sprintf(swapname, "swap%s.c", fl->f_fn);
			if (eq(sp, swapname))
				goto cont;
		}
		cp = sp + (len = strlen(sp)) - 1;
		och = *cp;
		*cp = 'o';
		if (len + lpos > 72) {
			lpos = 8;
			fprintf(fp, "\\\n\t");
		}
		fprintf(fp, "%s ", sp);
		lpos += len + 1;
		*cp = och;
cont:
		;
	}
	if (lpos != 8)
		putc('\n', fp);
}

do_cfiles(fp)
	FILE *fp;
{
	register struct file_list *tp;
	register int lpos, len;

	fprintf(fp, "CFILES=");
	lpos = 8;
	for (tp = ftab; tp != 0; tp = tp->f_next) {
		if (tp->f_type == INVISIBLE)
			continue;
		if (tp->f_fn[strlen(tp->f_fn)-1] != 'c')
			continue;
		if ((len = 3 + strlen(tp->f_fn)) + lpos > 72) {
			lpos = 8;
			fprintf(fp, "\\\n\t");
		}
		fprintf(fp, "../%s ", tp->f_fn);
		lpos += len + 1;
	}
	if (lpos != 8)
		putc('\n', fp);
}

#ifdef	CMU
do_sfiles(fp)
	FILE *fp;
{
	register struct file_list *tp;
	register int lpos, len;

	fprintf(fp, "SFILES=");
	lpos = 8;
	for (tp = ftab; tp != 0; tp = tp->f_next) {
		if (tp->f_type == INVISIBLE)
			continue;
		if (tp->f_fn[strlen(tp->f_fn)-1] != 's')
			continue;
		if ((len = 3 + strlen(tp->f_fn)) + lpos > 72) {
			lpos = 8;
			fprintf(fp, "\\\n\t");
		}
		fprintf(fp, "../%s ", tp->f_fn);
		lpos += len + 1;
	}
	if (lpos != 8)
		putc('\n', fp);
}

#endif CMU
char *
tail(fn)
	char *fn;
{
	register char *cp;

	cp = rindex(fn, '/');
	if (cp == 0)
		return (fn);
	return (cp+1);
}

/*
 * Create the makerules for each file
 * which is part of the system.
 * Devices are processed with the special c2 option -i
 * which avoids any problem areas with i/o addressing
 * (e.g. for the VAX); assembler files are processed by as.
 */
do_rules(f)
	FILE *f;
{
	register char *cp, *np, och, *tp;
	register struct file_list *ftp;
	char *extras;

for (ftp = ftab; ftp != 0; ftp = ftp->f_next) {
	if (ftp->f_type == INVISIBLE)
		continue;
	cp = (np = ftp->f_fn) + strlen(ftp->f_fn) - 1;
	och = *cp;
	*cp = '\0';
	fprintf(f, "%so: ../%s%c\n", tail(np), np, och);
	tp = tail(np);
	if (och == 's') {
#ifdef	CMU
		fprintf(f, "\t${CC} -E ${SFLAGS} ../%ss | ${AS} -o %so\n\n",
			np, tp);
#else	CMU
		fprintf(f, "\t-ln -s ../%ss %sc\n", np, tp);
		fprintf(f, "\t${CC} -E ${COPTS} %sc | ${AS} -o %so\n",
			tp, tp);
		fprintf(f, "\trm -f %sc\n\n", tp);
#endif	CMU
		continue;
	}
	if (ftp->f_flags & CONFIGDEP)
		extras = "${PARAM} ";
	else
		extras = "";
	switch (ftp->f_type) {

	case NORMAL:
		switch (machine) {

		case MACHINE_VAX:
			fprintf(f, "\t${CC} -c -S ${COPTS} %s../%sc\n",
				extras, np);
			fprintf(f, "\t${C2} %ss | inline |",
			    tp);
			fprintf(f, " ${AS} -o %so\n", tp);
			fprintf(f, "\trm -f %ss\n\n", tp);
			break;

#ifdef	CMU
		case MACHINE_SUN:
		case MACHINE_ROMP:
			fprintf(f, "\t${CC} -c -O ${COPTS} %s../%sc\n\n",
				extras, np);
			break;
#endif	CMU
		}
		break;

	case DRIVER:
		switch (machine) {

		case MACHINE_VAX:
			fprintf(f, "\t${CC} -c -S ${COPTS} %s../%sc\n",
				extras, np);
			fprintf(f,"\t${C2} -i %ss | inline |",
			    tp);
			fprintf(f, " ${AS} -o %so\n", tp);
			fprintf(f, "\trm -f %ss\n\n", tp);
			break;

#ifdef	CMU
		case MACHINE_SUN:
		case MACHINE_ROMP:
			fprintf(f, "\t${CC} -c -O ${COPTS} %s../%sc\n\n",
				extras, np);
#endif	CMU
		}
		break;

	case PROFILING:
		if (!profiling)
			continue;
		if (COPTS == 0) {
			fprintf(stderr,
			    "config: COPTS undefined in generic makefile");
			COPTS = "";
		}
		switch (machine) {

		case MACHINE_VAX:
			fprintf(f, "\t${CC} -c -S %s %s../%sc\n",
				COPTS, extras, np);
			fprintf(f, "\tex - %ss < ${GPROF.EX}\n", tp);
			fprintf(f, "\tinline %ss | ${AS} -o %so\n",
			    tp, tp);
			fprintf(f, "\trm -f %ss\n\n", tp);
			break;

#ifdef	CMU
		case MACHINE_SUN:
			fprintf(stderr,
			    "config: don't know how to profile kernel on sun\n");
			break;

		case MACHINE_ROMP:
			fprintf(stderr,
			    "config: don't know how to profile kernel on romp\n");
			break;
#endif	CMU
		}
		break;

	default:
		printf("Don't know rules for %s\n", np);
		break;
	}
	*cp = och;
}
}

/*
 * Create the load strings
 */
do_load(f)
	register FILE *f;
{
	register struct file_list *fl;
	int first = 1;
	struct file_list *do_systemspec();

	fl = conf_list;
	while (fl) {
		if (fl->f_type != SYSTEMSPEC) {
			fl = fl->f_next;
			continue;
		}
		fl = do_systemspec(f, fl, first);
		if (first)
			first = 0;
	}
	fprintf(f, "all:");
	for (fl = conf_list; fl != 0; fl = fl->f_next)
		if (fl->f_type == SYSTEMSPEC)
			fprintf(f, " %s", fl->f_needs);
	fprintf(f, "\n");
}

struct file_list *
do_systemspec(f, fl, first)
	FILE *f;
	register struct file_list *fl;
	int first;
{

#ifdef	CMU
/*
 * following is required because if a depends upon b and a depends upon c
 * and you have a c, and ask for "a" make isn't smart enought to figure it
 * out!
 */
	if (machine == MACHINE_ROMP && haverdb)
		{ register char *p = fl->f_needs;
		fprintf(f,"# rule needed to merge in debugger\n");
		fprintf(f,"%s.ws: %s $(VMRDB) \n",p,p);
		fprintf(f,"\tsh ../conf/make.ws %s $(VMRDB) %s.ws\n\n",p, p);
		}
#endif	CMU
	fprintf(f, "%s: Makefile", fl->f_needs);
	if (machine == MACHINE_VAX)
		fprintf(f, " inline", machinename);
	fprintf(f, " locore.o ${OBJS} param.o ioconf.o swap%s.o\n", fl->f_fn);
	fprintf(f, "\t@echo loading %s\n\t@rm -f %s\n",
	    fl->f_needs, fl->f_needs);
	if (first) {
		fprintf(f, "\t@sh ../conf/newvers.sh\n");
		fprintf(f, "\t@${CC} $(CFLAGS) -c vers.c\n");
	}
	switch (machine) {

	case MACHINE_VAX:
		fprintf(f, "\t@${LD} -n -o %s -e start -x -T 80000000 ",
			fl->f_needs);
		break;
#ifdef	CMU

	case MACHINE_SUN:
		fprintf(f, "\t@${LD} -o %s -e start -x -T 4000 ",
			fl->f_needs);
		break;

	case MACHINE_ROMP:
		fprintf(f, "\t@${LD} -o %s ${LDFLAGS} -T E0000000 ",
			fl->f_needs);
		break;
#endif	CMU
	}
	fprintf(f, "locore.o ${OBJS} vers.o ioconf.o param.o ");
#if	CMU
	if (machine == MACHINE_ROMP) fprintf(f,"blt.o ");
	fprintf(f, "swap%s.o ${LIBS}\n", fl->f_fn);
	fprintf(f, "\tmd -d *.n\n");
#else	CMU
	fprintf(f, "swap%s.o\n", fl->f_fn);
#endif	CMU
	fprintf(f, "\t@echo rearranging symbols\n");
	fprintf(f, "\t@-symorder ../%s/symbols.sort %s\n",
	    machinename, fl->f_needs);
	fprintf(f, "\t@size %s\n", fl->f_needs);
	fprintf(f, "\t@chmod 755 %s\n\n", fl->f_needs);
	do_swapspec(f, fl->f_fn);
#ifdef	CMU
	for (fl = fl->f_next; fl != NULL && fl->f_type == SWAPSPEC; fl = fl->f_next)
#else	CMU
	for (fl = fl->f_next; fl->f_type == SWAPSPEC; fl = fl->f_next)
#endif	CMU
		;
	return (fl);
}

do_swapspec(f, name)
	FILE *f;
	register char *name;
{

	if (!eq(name, "generic")) {
#ifdef	CMU
		if (machine == MACHINE_VAX)
			fprintf(f, "swap%s.o: swap%s.c inline\n", name, name);
		else
			fprintf(f, "swap%s.o: swap%s.c\n", name, name);
#else	CMU
		fprintf(f, "swap%s.o: swap%s.c\n", name, name);
#endif	CMU
		fprintf(f, "\t${CC} -I. -c -O ${COPTS} swap%s.c\n\n", name);
		return;
	}
#ifdef	CMU
	if (machine == MACHINE_VAX)
		fprintf(f, "swapgeneric.o: ../%s/swapgeneric.c inline\n", machinename);
	else
		fprintf(f, "swapgeneric.o: ../%s/swapgeneric.c\n", machinename);
#else	CMU
	fprintf(f, "swapgeneric.o: ../%s/swapgeneric.c\n", machinename);
#endif	CMU
	switch (machine) {

	case MACHINE_VAX:
		fprintf(f, "\t${CC} -c -S ${COPTS} ");
		fprintf(f, "../%s/swapgeneric.c\n", machinename);
		fprintf(f, "\t${C2} swapgeneric.s | ");
		fprintf(f, "inline");
		fprintf(f, " | ${AS} -o swapgeneric.o\n");
		fprintf(f, "\trm -f swapgeneric.s\n\n");
		break;

#ifdef	CMU
	case MACHINE_SUN:
	case MACHINE_ROMP:
		fprintf(f, "\t${CC} -c -O ${COPTS} ");
		fprintf(f, "../%s/swapgeneric.c\n\n", machinename);
		break;
#endif	CMU
	}
}

char *
raise(str)
	register char *str;
{
	register char *cp = str;

	while (*str) {
		if (islower(*str))
			*str = toupper(*str);
		str++;
	}
	return (cp);
}
