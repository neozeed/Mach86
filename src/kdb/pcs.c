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
#ifndef lint
static	char sccsid[] = "@(#)pcs.c	4.2 8/11/83";
#endif

#define DBG	1
#ifdef  DBG
/*****************************************************************
 * 6-jan-86	David Golub (dbg)
 *  new commands:
 *	:j single-steps until CALL[S,G] or RET encountered,
 *         printing PC at each stop
 *      :p like :s, but prints PC if count not done yet
 *
 ****************************************************************/
#endif	DBG

/*
 *
 *	UNIX debugger
 *
 */

#include "defs.h"


MSG		NOBKPT;
MSG		SZBKPT;
MSG		EXBKPT;
MSG		NOPCS;
MSG		BADMOD;

/* breakpoints */
BKPTR		bkpthead;

CHAR		*lp;
CHAR		lastc;

INT		signo;
L_INT		dot;
INT		pid;
L_INT		cntval;
L_INT		loopcnt;

L_INT		entrypt;
INT		adrflg;

#ifdef	DBG
#include	"pcs.h"
#endif	DBG

/* sub process control */

subpcs(modif)
{
	REG INT		check;
	INT		execsig,runmode;
	REG BKPTR	bkptr;
	STRING		comptr;
	execsig=0; loopcnt=cntval;

	switch (modif) {

	    /* delete breakpoint */
	    case 'd': case 'D':
		IF (bkptr=scanbkpt(dot))
		THEN bkptr->flag=0; return;
		ELSE error(NOBKPT);
		FI

	    /* set breakpoint */
	    case 'b': case 'B':
		IF (bkptr=scanbkpt(dot))
		THEN bkptr->flag=0;
		FI
		FOR bkptr=bkpthead; bkptr; bkptr=bkptr->nxtbkpt
		DO IF bkptr->flag == 0
		   THEN break;
		   FI
		OD
		IF bkptr==0
#ifdef	CMU
		THEN IF (bkptr=(BKPTR)sbrk(sizeof *bkptr)) == (BKPTR)-1
#else	CMU
		THEN IF (bkptr=sbrk(sizeof *bkptr)) == -1
#endif	CMU
		     THEN error(SZBKPT);
		     ELSE bkptr->nxtbkpt=bkpthead;
			  bkpthead=bkptr;
		     FI
		FI
		bkptr->loc = dot;
		bkptr->initcnt = bkptr->count = cntval;
		bkptr->flag = BKPTSET;
		check=MAXCOM-1; comptr=bkptr->comm; rdc(); lp--;
		REP *comptr++ = readchar();
		PER check-- ANDF lastc!=EOR DONE
		*comptr=0; lp--;
		IF check
		THEN return;
		ELSE error(EXBKPT);
		FI

#ifndef	KDB
	    /* exit */
	    case 'k' :case 'K':
		IF pid
		THEN printf("%d: killed", pid); endpcs(); return;
		FI
		error(NOPCS);

	    /* run program */
	    case 'r': case 'R':
		endpcs();
		setup(); runmode=CONTIN;
		IF adrflg 
		THEN IF !scanbkpt(dot) THEN loopcnt++; FI
		ELSE IF !scanbkpt(entrypt+2) THEN loopcnt++; FI
		FI
		break;
#endif	KDB

	    /* single step */
	    case 's': case 'S':
#ifndef	KDB
		IF pid
		THEN
#endif	KDB
			runmode=SINGLE; execsig=getsig(signo);
#ifdef	DBG
			sstepmode = STEP_NORM;
#endif	DBG
#ifndef	KDB
		ELSE setup(); loopcnt--;
		FI
#endif	KDB
		break;

#ifdef	DBG
	    case 'p': case 'P':

		runmode = SINGLE;
		execsig = getsig(signo);
		sstepmode = STEP_PRINT;
		break;

	    case 'j': case 'J':

		runmode = SINGLE;
		execsig = getsig(signo);
		sstepmode = STEP_CALLT;
		icount = 0;
		break;
#endif	DBG

	    /* continue with optional signal */
	    case 'c': case 'C': case 0:
		IF pid==0 THEN error(NOPCS); FI
		runmode=CONTIN; execsig=getsig(signo);
#ifdef	DBG
		sstepmode = STEP_NONE;
#endif	DBG
		break;

	    default: error(BADMOD);
	}

#ifndef	KDB
	IF loopcnt>0 ANDF runpcs(runmode,execsig)
	THEN printf("breakpoint%16t");
	ELSE printf("stopped at%16t");
	FI
	delbp();
	printpc();
#else	KDB
	IF loopcnt>0 THEN
		runpcs(runmode,0);
	FI
#endif	KDB
}
