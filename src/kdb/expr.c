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
static	char sccsid[] = "@(#)expr.c	4.8 8/11/83";
#endif
/*
 *
 *	UNIX debugger
 *
 */

#include "defs.h"

MSG		BADSYM;
MSG		BADVAR;
MSG		BADKET;
MSG		BADSYN;
MSG		NOCFN;
MSG		NOADR;
MSG		BADLOC;

ADDR		lastframe;
ADDR		savlastf;
ADDR		savframe;
ADDR		savpc;
ADDR		callpc;



CHAR		*lp;
INT		radix;
STRING		errflg;
L_INT		localval;
CHAR		isymbol[1024];

CHAR		lastc,peekc;

L_INT		dot;
L_INT		ditto;
INT		dotinc;
L_INT		var[];
L_INT		expv;




expr(a)
{	/* term | term dyadic expr |  */
	INT		rc;
	L_INT		lhs;

	rdc(); lp--; rc=term(a);

	WHILE rc
	DO  lhs = expv;

	    switch ((int)readchar()) {

		    case '+':
			term(a|1); expv += lhs; break;

		    case '-':
			term(a|1); expv = lhs - expv; break;

		    case '#':
			term(a|1); expv = round(lhs,expv); break;

		    case '*':
			term(a|1); expv *= lhs; break;

		    case '%':
			term(a|1); expv = lhs/expv; break;

		    case '&':
			term(a|1); expv &= lhs; break;

		    case '|':
			term(a|1); expv |= lhs; break;

		    case ')':
			IF (a&2)==0 THEN error(BADKET); FI

		    default:
			lp--;
			return(rc);
	    }
	OD
	return(rc);
}

term(a)
{	/* item | monadic item | (expr) | */

	switch ((int)readchar()) {

		    case '*':
			term(a|1); expv=chkget(expv,DSP); return(1);

		    case '@':
			term(a|1); expv=chkget(expv,ISP); return(1);

		    case '-':
			term(a|1); expv = -expv; return(1);

		    case '~':
			term(a|1); expv = ~expv; return(1);

		    case '#':
			term(a|1); expv = !expv; return(1);

		    case '(':
			expr(2);
			IF *lp!=')'
			THEN	error(BADSYN);
			ELSE	lp++; return(1);
			FI

		    default:
			lp--;
			return(item(a));
	}
}

item(a)
{	/* name [ . local ] | number | . | ^ | <var | <register | 'x | | */
	INT		base, d;
	CHAR		savc;
	BOOL		hex;
	L_INT		frame;
	register struct nlist *symp;
	int regptr;

	hex=FALSE;

	readchar();
	IF symchar(0)
	THEN	readsym();
		IF lastc=='.'
		THEN	frame= *(ADDR *)(((ADDR)&u)+FP); lastframe=0;
			callpc= *(ADDR *)(((ADDR)&u)+PC);
			WHILE errflg==0
			DO  savpc=callpc;
				findsym(callpc,ISYM);
			    IF  eqsym(cursym->n_un.n_name,isymbol,'~')
			    THEN break;
			    FI
				callpc=get(frame+16, DSP);
			    lastframe=frame;
			    frame=get(frame+12,DSP)&EVEN;
			    IF frame==0
			    THEN error(NOCFN);
			    FI
			OD
			savlastf=lastframe; savframe=frame;
			readchar();
			IF symchar(0)
			THEN	chkloc(expv=frame);
			FI
		ELIF (symp=lookup(isymbol))==0 THEN error(BADSYM);
		ELSE expv = symp->n_value;
		FI
		lp--;

	ELIF getnum(readchar)
	THEN ;
	ELIF lastc=='.'
	THEN	readchar();
		IF symchar(0)
		THEN	lastframe=savlastf; callpc=savpc;
			chkloc(savframe);
		ELSE	expv=dot;
		FI
		lp--;

	ELIF lastc=='"'
	THEN	expv=ditto;

	ELIF lastc=='+'
	THEN	expv=inkdot(dotinc);

	ELIF lastc=='^'
	THEN	expv=inkdot(-dotinc);

	ELIF lastc=='<'
	THEN	savc=rdc();
		IF regptr=getreg(savc)
		THEN	IF kcore THEN expv = *(int *)regptr;
			ELSE expv= * (ADDR *)(((ADDR)&u)+regptr); FI
		ELIF (base=varchk(savc)) != -1
		THEN	expv=var[base];
		ELSE	error(BADVAR);
		FI

	ELIF lastc=='\''
	THEN	d=4; expv=0;
		WHILE quotchar()
		DO  IF d--
		    THEN expv = (expv << 8) | lastc;
		    ELSE error(BADSYN);
		    FI
		OD

	ELIF a
	THEN	error(NOADR);
	ELSE	lp--; return(0);
	FI
	return(1);
}

/* service routines for expression reading */
getnum(rdf) int (*rdf)();
{
	INT base,d,frpt;
	BOOL hex;
	UNION{REAL r; L_INT i;} real;
	IF isdigit(lastc) ORF (hex=TRUE, lastc=='#' ANDF isxdigit((*rdf)()))
	THEN	expv = 0;
		base = (hex ? 16 : radix);
		WHILE (base>10 ? isxdigit(lastc) : isdigit(lastc))
		DO  expv = (base==16 ? expv<<4 : expv*base);
		    IF (d=convdig(lastc))>=base THEN error(BADSYN); FI
		    expv += d; (*rdf)();
		    IF expv==0
		    THEN IF (lastc=='x' ORF lastc=='X')
				 THEN hex=TRUE; base=16; (*rdf)();
				 ELIF (lastc=='t' ORF lastc=='T')
			     THEN hex=FALSE; base=10; (*rdf)();
		    	 ELIF (lastc=='o' ORF lastc=='O')
		    	 THEN hex=FALSE; base=8; (*rdf)();
				 FI
		    FI
		OD
		IF lastc=='.' ANDF (base==10 ORF expv==0) ANDF !hex
		THEN	real.r=expv; frpt=0; base=10;
			WHILE isdigit((*rdf)())
			DO	real.r *= base; frpt++;
				real.r += lastc-'0';
			OD
			WHILE frpt--
			DO	real.r /= base; OD
			expv = real.i;
		FI
		peekc=lastc;
/*		lp--; */
		return(1);
	ELSE return(0);
	FI
}

readsym()
{
	REG char	*p;

	p = isymbol;
	REP IF p < &isymbol[sizeof(isymbol)-1]
	    THEN *p++ = lastc;
	    FI
	    readchar();
	PER symchar(1) DONE
	*p++ = 0;
}

convdig(c)
CHAR c;
{
	IF isdigit(c)
	THEN	return(c-'0');
	ELIF isxdigit(c)
	THEN	return(c-'a'+10);
	ELSE	return(17);
	FI
}

symchar(dig)
{
	IF lastc=='\\' THEN readchar(); return(TRUE); FI
	return( isalpha(lastc) ORF lastc=='_' ORF dig ANDF isdigit(lastc) );
}

varchk(name)
{
	IF isdigit(name) THEN return(name-'0'); FI
	IF isalpha(name) THEN return((name&037)-1+10); FI
	return(-1);
}

chkloc(frame)
L_INT		frame;
{
	readsym();
	REP IF localsym(frame)==0 THEN error(BADLOC); FI
	    expv=localval;
	PER !eqsym(cursym->n_un.n_name,isymbol,'~') DONE
}

eqsym(s1, s2, c)
	register char *s1, *s2;
{

	if (!strcmp(s1,s2))
		return (1);
	if (*s1 == c && !strcmp(s1+1, s2))
		return (1);
	return (0);
}
