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
static	char sccsid[] = "@(#)command.c	4.5 8/11/83";
#endif
/*
 *
 *	UNIX debugger
 *
 */

#include "defs.h"

MSG		BADEQ;
MSG		NOMATCH;
MSG		BADVAR;
MSG		BADCOM;

MAP		txtmap;
MAP		datmap;
INT		executing;
CHAR		*lp;
INT		fcor;
INT		fsym;
INT		mkfault;
STRING		errflg;

CHAR		lastc;
CHAR		eqformat[512] = "z";
CHAR		stformat[512] = "X\"= \"^i";

L_INT		dot;
L_INT		ditto;
INT		dotinc;
INT		lastcom = '=';
L_INT		var[];
L_INT		locval;
L_INT		locmsk;
INT		pid;
L_INT		expv;
L_INT		adrval;
INT		adrflg;
L_INT		cntval;
INT		cntflg;




/* command decoding */

command(buf,defcom)
STRING		buf;
CHAR		defcom;
{
#ifdef	CMU
	INT		itype, ptype, modifier;
	L_INT		regptr;
#else	CMU
	INT		itype, ptype, modifier, regptr;
#endif	CMU
	BOOL		longpr, eqcom;
	CHAR		wformat[1];
	CHAR		savc;
	L_INT		w, savdot;
	STRING		savlp=lp;
	IF buf
	THEN IF *buf==EOR
	     THEN return(FALSE);
	     ELSE lp=buf;
	     FI
	FI

	REP
	IF adrflg=expr(0)
	THEN dot=expv; ditto=dot;
	FI
	adrval=dot;
	IF rdc()==',' ANDF expr(0)
	THEN cntflg=TRUE; cntval=expv;
	ELSE cntflg=FALSE; cntval=1; lp--;
	FI

	IF !eol(rdc())
	THEN lastcom=lastc;
	ELSE IF adrflg==0 THEN dot=inkdot(dotinc); FI
	     lp--; lastcom=defcom;
	FI

	switch(lastcom&STRIP) {

	    case '/':
		itype=DSP; ptype=DSYM;
		goto trystar;

	    case '=':
		itype=NSP; ptype=0;
		goto trypr;

	    case '?':
		itype=ISP; ptype=ISYM;
		goto trystar;

	    trystar:
		IF rdc()=='*' THEN lastcom |= QUOTE; ELSE lp--; FI
		IF lastcom&QUOTE
		THEN itype |= STAR; ptype = (DSYM+ISYM)-ptype;
		FI

	    trypr:
		longpr=FALSE; eqcom=lastcom=='=';
		switch (rdc()) {

			case 'm':
			    {/*reset map data*/
			    INT		fcount;
			    MAP		*smap;
			    UNION{MAP *m; L_INT *mp;}amap;

			    IF eqcom THEN error(BADEQ); FI
			    smap=(itype&DSP?&datmap:&txtmap);
			    amap.m=smap; fcount=3;
			    IF itype&STAR
			    THEN amap.mp += 3;
			    FI
			    WHILE fcount-- ANDF expr(0)
			    DO *(amap.mp)++ = expv; OD
			    IF rdc()=='?' THEN smap->ufd=fsym;
			    ELIF lastc == '/' THEN smap->ufd=fcor;
			    ELSE lp--;
			    FI
			    }
			    break;

			case 'L':
			    longpr=TRUE;
			case 'l':
			    /*search for exp*/
			    IF eqcom THEN error(BADEQ); FI
			    dotinc=(longpr?4:2); savdot=dot;
			    expr(1); locval=expv;
			    IF expr(0) THEN locmsk=expv; ELSE locmsk = -1L; FI
				IF !longpr THEN locmsk &= 0xFFFF; locval &= 0xFFFF; FI
			    LOOP w=get(dot,itype);
				 IF errflg ORF mkfault ORF (w&locmsk)==locval THEN break; FI
				 dot=inkdot(dotinc);
			    POOL
			    IF errflg
			    THEN dot=savdot; errflg=NOMATCH;
			    FI
			    psymoff(dot,ptype,"");
			    break;

			case 'W':
			    longpr=TRUE;
			case 'w':
			    IF eqcom THEN error(BADEQ); FI
			    wformat[0]=lastc; expr(1);
			    REP  savdot=dot; psymoff(dot,ptype,":%16t"); exform(1,wformat,itype,ptype);
				 errflg=0; dot=savdot;
				 IF longpr
				 THEN put(dot,itype,expv);
				 ELSE put(dot,itype,itol(get(dot+2,itype),expv));
				 FI
				 savdot=dot;
				 printf("=%8t"); exform(1,wformat,itype,ptype);
				 newline();
			    PER  expr(0) ANDF errflg==0 DONE
			    dot=savdot;
			    chkerr();
			    break;

			default:
			    lp--;
			    getformat(eqcom ? eqformat : stformat);
			    IF !eqcom
			    THEN psymoff(dot,ptype,":%16t");
			    FI
			    scanform(cntval,(eqcom?eqformat:stformat),itype,ptype);
		}
		break;

	    case '>':
		lastcom=0; savc=rdc();
		IF regptr=getreg(savc)
		THEN IF kcore THEN *(int *)regptr = dot; ELSE
		     * (ADDR *) (((ADDR)&u)+regptr)=dot;
#ifndef	KDB
		     ptrace(WUREGS,pid,regptr,* (ADDR *) (((ADDR)&u)+regptr));
#endif	KDB
		     FI
		ELIF (modifier=varchk(savc)) != -1
		THEN	var[modifier]=dot;
		ELSE	error(BADVAR);
		FI
		break;

	    case '!':
		lastcom=0;
		shell(); break;

	    case '$':
		lastcom=0;
		printtrace(nextchar()); break;

	    case ':':
		IF !executing
		THEN executing=TRUE;
		     subpcs(nextchar());
		     executing=FALSE;
		     lastcom=0;
		FI
		break;

	    case 0:
		prints(DBNAME);
		break;

	    default: error(BADCOM);
	}

	flushbuf();
	PER rdc()==';' DONE
	IF buf THEN lp=savlp; ELSE lp--; FI
	return(adrflg ANDF dot!=0);
}

