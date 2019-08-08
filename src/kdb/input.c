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
static	char sccsid[] = "@(#)input.c	4.2 8/11/83";
#endif
/*
 *
 *	UNIX debugger
 *
 */

#include "defs.h"

INT		mkfault;
CHAR		line[LINSIZ];
INT		infile;
CHAR		*lp;
CHAR		peekc,lastc = EOR;
INT		eof;

/* input routines */

eol(c)
CHAR	c;
{
	return(c==EOR ORF c==';');
}

rdc()
{	REP	readchar();
	PER	lastc==SP ORF lastc==TB
	DONE
	return(lastc);
}

#ifdef	KDB
char erasec[] = {'\b', ' ', '\b'};

char *
editchar(line, lp)
char *line;
register char *lp;
{
    switch (*lp)
    {
	case 0177:
	case 'H'&077:
	    if (lp > line)
	    {
		write(1, erasec, sizeof(erasec));
		lp--;
	    }
	    break;
	case 'U'&077:
	    while (lp > line)
	    {
		write(1, erasec, sizeof(erasec));
		lp--;
	    }
	    break;
	case 'R'&077:
	    write(1, "^R\n", 3);
	    if (lp > line)
		write(1, line, lp-line);
	    break;
	default:
	    return(++lp);
    }
    return(lp);
}

#endif	KDB
readchar()
{
	IF eof
	THEN	lastc=0;
	ELSE	IF lp==0
		THEN	lp=line;
			REP eof = read(infile,lp,1)==0;
			    IF mkfault THEN error(0); FI
#ifdef	KDB
			    lp = editchar(line, lp);
			PER eof==0 ANDF (lp == line ORF lp[-1]!=EOR) DONE
#else	KDB
			PER eof==0 ANDF *lp++!=EOR DONE
#endif	KDB
			*lp=0; lp=line;
		FI
		IF lastc = peekc THEN peekc=0;
		ELIF lastc = *lp THEN lp++;
		FI
	FI
	return(lastc);
}

nextchar()
{
	IF eol(rdc())
	THEN lp--; return(0);
	ELSE return(lastc);
	FI
}

quotchar()
{
	IF readchar()=='\\'
	THEN	return(readchar());
	ELIF lastc=='\''
	THEN	return(0);
	ELSE	return(lastc);
	FI
}

getformat(deformat)
STRING		deformat;
{
	REG STRING	fptr;
	REG BOOL	quote;
	fptr=deformat; quote=FALSE;
	WHILE (quote ? readchar()!=EOR : !eol(readchar()))
	DO  IF (*fptr++ = lastc)=='"'
	    THEN quote = ~quote;
	    FI
	OD
	lp--;
	IF fptr!=deformat THEN *fptr++ = '\0'; FI
}
