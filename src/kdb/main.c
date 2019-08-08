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
static	char sccsid[] = "@(#)main.c 4.3 4/1/82";
/*
 * adb - main command loop and error/interrupt handling
 */
#include "defs.h"

/*
 **************************************************************************
 * HISTORY
 * 16-May-86  David Golub (dbg) at Carnegie-Mellon University
 *	Support for debugging other processes than the one interrupted.
 *
 * 11-Oct-85  Robert V Baron (rvb) at Carnegie-Mellon University
 *	defined "<t"
 *
 **************************************************************************
 */

#include <sys/proc.h>

MSG		NOEOR;

INT		mkfault;
INT		executing;
INT		infile;
CHAR		*lp;
L_INT		maxoff;
L_INT		maxpos;
ADDR		sigint;
ADDR		sigqit;
INT		wtflag;
L_INT		maxfile;
STRING		errflg;
L_INT		exitflg;

CHAR		lastc;
INT		eof;

INT		lastcom;

long	maxoff = MAXOFF;
long	maxpos = MAXPOS;
#ifndef	KDB
char	*Ipath = "/usr/lib/adb";
#else	KDB
L_INT	dot;
#endif	KDB

#ifndef	KDB
main(argc, argv)
	register char **argv;
	int argc;
#else	KDB
kdb(type, trapsp, curproc)
	int		type;
	int		*trapsp;
	struct proc	*curproc;
#endif	KDB
{

	mkioptab();
#ifndef	KDB
another:
	if (argc>1) {
		if (eqstr("-w", argv[1])) {
			wtflag = 2;		/* suitable for open() */
			argc--, argv++;
			goto another;
		}
		if (eqstr("-k", argv[1])) {
			kernel = 1;
			argc--, argv++;
			goto another;
		}
		if (argv[1][0] == '-' && argv[1][1] == 'I') {
			Ipath = argv[1]+2;
			argc--, argv++;
		}
	}
	if (argc > 1)
		symfil = argv[1];
	if (argc > 2)
		corfil = argv[2];
	xargc = argc;
	setsym(); setcor(); setvar();

	if ((sigint=signal(SIGINT,SIG_IGN)) != SIG_IGN) {
		sigint = fault;
		signal(SIGINT, fault);
	}
	sigqit = signal(SIGQUIT, SIG_IGN);
	setexit();
#else	KDB
	{
	    extern ADDR userpc;
	    extern INT pid;

	    userpc = dot = pcb.pcb_pc;
#if	MACH_VM
	    if (curproc) {
		
		/*
	     	 *	Get the map for the current process
		 */
		kdbgetprocess(curproc, &curmap, &curpcb);
		curpid = curproc->p_pid;
		var[varchk('m')] = curmap;
	    }
	    else {
		/*
		 *	if there's no process...
		 */
		curmap = NULL;	/* take our chances */
		curpid = 1;	/* fake */
	    }
	    /*
	     *	But the pcb is the saved set of registers
	     */
	    curpcb = &pcb;
	    pid    = curpid;
#else	MACH_VM
	    pid = 1;
#endif	MACH_VM
	    var[varchk('t')] = (int)trapsp;
	}
	maxoff = 0x100000;
	wtflag = 1;
	kcore = 1;
	flushbuf();
#ifdef	DEBUG
	{
		extern L_INT loopcnt;
		printf("loop=%d\n", loopcnt);
	}
#endif	DEBUG
	switch (setexit())
	{
	    case SINGLE:
#ifdef	DEBUG
		printf("S ");
#endif	DEBUG
		pcb.pcb_psl |= PSL_T;
		/* fall through */
	    case CONTIN:
#ifdef	DEBUG
		printf("CONT=>\n");
#endif	DEBUG
		return(1);
	    case 0:
#ifdef	DEBUG
		printf("nextpcs(%d)\n", type);
#endif	DEBUG
		IF nextpcs(type, 0)
		THEN printf("breakpoint%16t");
		ELSE printf("stopped at%16t");
		FI
		printpc();
	}
#endif	KDB
	if (executing)
		delbp();
	executing = 0;
	for (;;) {
		flushbuf();
		if (errflg) {
			printf("%s\n", errflg);
#ifdef	CMU
			exitflg = (int)errflg;
#else	CMU
			exitflg = errflg;
#endif	CMU
			errflg = 0;
		}
		if (mkfault) {
			mkfault=0;
			printc('\n');
			prints(DBNAME);
		}
		lp=0; rdc(); lp--;
		if (eof) {
#ifndef	KDB
			if (infile) {
				iclose(-1, 0); eof=0; reset();
			} else
				done();
#else	KDB
				return(1);
#endif	KDB
		} else
			exitflg = 0;
		command(0, lastcom);
		if (lp && lastc!='\n')
			error(NOEOR);
	}
}

#ifndef	KDB
done()
{
	endpcs();
	exit(exitflg);
}
#endif	KDB

L_INT
round(a,b)
REG L_INT a, b;
{
	REG L_INT w;
	w = (a/b)*b;
	IF a!=w THEN w += b; FI
	return(w);
}

/*
 * If there has been an error or a fault, take the error.
 */
chkerr()
{
	if (errflg || mkfault)
		error(errflg);
}

/*
 * An error occurred; save the message for later printing,
 * close open files, and reset to main command loop.
 */
error(n)
	char *n;
{
	errflg = n;
	iclose(0, 1); oclose();
#ifndef	KDB
	reset();
#else	KDB
	reset(1);
#endif	KDB
}

#ifndef	KDB
/*
 * An interrupt occurred; reset the interrupt
 * catch, seek to the end of the current file
 * and remember that there was a fault.
 */
fault(a)
{
	signal(a, fault);
	lseek(infile, 0L, 2);
	mkfault++;
}
#endif	KDB
