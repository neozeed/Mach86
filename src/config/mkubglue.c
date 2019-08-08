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
static char sccsid[] = "@(#)mkubglue.c	5.1 (Berkeley) 5/8/85";
#endif not lint

/*
 **********************************************************************
 * HISTORY
 * 25-Jan-86  Avadis Tevanian (avie) at Carnegie-Mellon University
 *	Updated to 4.3.
 *
 * 30-Aug-85 David L. Black (dlb) at CMU.  Make user-time changes
 *	conditionally compiled.
 *
 * 5-Aug-85 David L. Black (dlb) at CMU.  Modified to do user-mode
 *	timing, also fixed interrupt counting problem in ubglue.
 *
 **********************************************************************
 */

/*
 * Make the uba interrupt file ubglue.s
 */
#include <stdio.h>
#include "config.h"
#include "y.tab.h"

ubglue()
{
	register FILE *fp;
	register struct device *dp, *mp;

	fp = fopen(path("ubglue.s"), "w");
	if (fp == 0) {
		perror(path("ubglue.s"));
		exit(1);
	}
	for (dp = dtab; dp != 0; dp = dp->d_next) {
		mp = dp->d_conn;
		if (mp != 0 && mp != (struct device *)-1 &&
		    !eq(mp->d_name, "mba")) {
			struct idlst *id, *id2;

			for (id = dp->d_vec; id; id = id->id_next) {
				for (id2 = dp->d_vec; id2; id2 = id2->id_next) {
					if (id2 == id) {
						dump_vec(fp, id->id, dp->d_unit);
						break;
					}
					if (!strcmp(id->id, id2->id))
						break;
				}
			}
		}
	}
	dump_std(fp);
	for (dp = dtab; dp != 0; dp = dp->d_next) {
		mp = dp->d_conn;
		if (mp != 0 && mp != (struct device *)-1 &&
		    !eq(mp->d_name, "mba")) {
			struct idlst *id, *id2;

			for (id = dp->d_vec; id; id = id->id_next) {
				for (id2 = dp->d_vec; id2; id2 = id2->id_next) {
					if (id2 == id) {
						dump_intname(fp, id->id,
							dp->d_unit);
						break;
					}
					if (!strcmp(id->id, id2->id))
						break;
				}
			}
		}
	}
	dump_ctrs(fp);
	(void) fclose(fp);
}

static int cntcnt = 0;		/* number of interrupt counters allocated */

/*
 * print an interrupt vector
 */
dump_vec(fp, vector, number)
	register FILE *fp;
	char *vector;
	int number;
{
	char nbuf[80];
	register char *v = nbuf;

	(void) sprintf(v, "%s%d", vector, number);
#if	CMU
	fprintf(fp, "\t.globl\t_X%s\n\t.align\t2\n_X%s:\n",
#else	CMU
	fprintf(fp, "\t.globl\t_X%s\n\t.align\t2\n_X%s:\n\tpushr\t$0x3f\n",
#endif	CMU
	    v, v);
#if	CMU
	fprintf(fp,"#if MACH_TIME > 0\n\tTIM_PUSHR(0)\n#else\n\tPUSHR\n#endif\n");
#endif	CMU
	fprintf(fp, "\tincl\t_fltintrcnt+(4*%d)\n", cntcnt++);
	if (strncmp(vector, "dzx", 3) == 0)
		fprintf(fp, "\tmovl\t$%d,r0\n\tjmp\tdzdma\n\n", number);
	else {
		if (strncmp(vector, "uur", 3) == 0) {
			fprintf(fp, "#ifdef UUDMA\n");
			fprintf(fp, "\tmovl\t$%d,r0\n\tjsb\tuudma\n", number);
			fprintf(fp, "#endif\n");
		}
		fprintf(fp, "\tpushl\t$%d\n", number);
#if	CMU
		fprintf(fp, "\tcalls\t$1,_%s\n",vector);
		fprintf(fp, "\tincl\t_cnt+V_INTR\n");
		fprintf(fp, "#if MACH_TIME > 0\n\tTSREI_POPR\n");
		fprintf(fp, "#else\n\tPOPR; rei\n");
		fprintf(fp, "#endif\n\n");
#else	CMU
		fprintf(fp, "\tcalls\t$1,_%s\n\tpopr\t$0x3f\n", vector);
		fprintf(fp, "\tincl\t_cnt+V_INTR\n\trei\n\n");
#endif	CMU
	}
}

/*
 * Start the interrupt name table with the names
 * of the standard vectors not on the unibus.
 * The number and order of these names should correspond
 * with the definitions in scb.s.
 */
dump_std(fp)
	register FILE *fp;
{
	fprintf(fp, "\n\t.globl\t_intrnames\n");
	fprintf(fp, "\n\t.globl\t_eintrnames\n");
	fprintf(fp, "\t.data\n");
	fprintf(fp, "_intrnames:\n");
	fprintf(fp, "\t.asciz\t\"clock\"\n");
	fprintf(fp, "\t.asciz\t\"cnr\"\n");
	fprintf(fp, "\t.asciz\t\"cnx\"\n");
	fprintf(fp, "\t.asciz\t\"tur\"\n");
	fprintf(fp, "\t.asciz\t\"tux\"\n");
	fprintf(fp, "\t.asciz\t\"mba0\"\n");
	fprintf(fp, "\t.asciz\t\"mba1\"\n");
	fprintf(fp, "\t.asciz\t\"mba2\"\n");
	fprintf(fp, "\t.asciz\t\"mba3\"\n");
	fprintf(fp, "\t.asciz\t\"uba0\"\n");
	fprintf(fp, "\t.asciz\t\"uba1\"\n");
	fprintf(fp, "\t.asciz\t\"uba2\"\n");
	fprintf(fp, "\t.asciz\t\"uba3\"\n");
#define	I_FIXED		13			/* number of names above */
}

dump_intname(fp, vector, number)
	register FILE *fp;
	char *vector;
	int number;
{
	register char *cp = vector;

	fprintf(fp, "\t.asciz\t\"");
	/*
	 * Skip any "int" or "intr" in the name.
	 */
	while (*cp)
		if (cp[0] == 'i' && cp[1] == 'n' &&  cp[2] == 't') {
			cp += 3;
			if (*cp == 'r')
				cp++;
		} else {
			putc(*cp, fp);
			cp++;
		}
	fprintf(fp, "%d\"\n", number);
}

dump_ctrs(fp)
	register FILE *fp;
{
	fprintf(fp, "_eintrnames:\n");
	fprintf(fp, "\n\t.globl\t_intrcnt\n");
	fprintf(fp, "\n\t.globl\t_eintrcnt\n");
	fprintf(fp, "_intrcnt:\n", I_FIXED);
	fprintf(fp, "\t.space\t4 * %d\n", I_FIXED);
	fprintf(fp, "_fltintrcnt:\n", cntcnt);
	fprintf(fp, "\t.space\t4 * %d\n", cntcnt);
	fprintf(fp, "_eintrcnt:\n\n");
	fprintf(fp, "\t.text\n");
}
