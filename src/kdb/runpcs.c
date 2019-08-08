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
static	char sccsid[] = "@(#)runpcs.c	1.2	(decvax!reilly)	2/18/84";
#endif

#undef	DEBUG

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

extern	MAP	txtmap;

MSG		NOFORK;
MSG		ENDPCS;
MSG		BADWAIT;

CHAR		*lp;
ADDR		sigint;
ADDR		sigqit;

/* breakpoints */
BKPTR		bkpthead;

REGLIST		reglist[];

CHAR		lastc;

INT		fcor;
INT		fsym;
STRING		errflg;
INT		errno;
INT		signo;
INT		sigcode;

L_INT		dot;
STRING		symfil;
INT		wtflag;
L_INT		pid;
L_INT		expv;
INT		adrflg;
L_INT		loopcnt;

#ifdef	DBG
#include	"pcs.h"
#endif	DBG



/* service routines for sub process control */

getsig(sig)
{	return(expr(0) ? expv : sig);
}

ADDR userpc = 1;

runpcs(runmode,execsig)
{
	INT		rc;
	REG BKPTR	bkpt;
	IF adrflg THEN userpc=dot; FI
#ifndef	KDB
	printf("%s: running\n", symfil);

	WHILE --loopcnt>=0
	DO
#endif	KDB
		if (execsig == 0)
			printf("kernel: running\n");
#ifdef DEBUG
		printf("\ncontinue %x %d\n",userpc,execsig);
#endif
		IF runmode==SINGLE
		THEN delbp(); /* hardware handles single-stepping */
		ELSE /* continuing from a breakpoint is hard */
			IF bkpt=scanbkpt(userpc)
			THEN execbkpt(bkpt,execsig); execsig=0;
			FI
			setbp();
		FI
#ifndef	KDB
		ptrace(runmode,pid,userpc,execsig);
		bpwait(); chkerr(); execsig=0; delbp(); readregs();
#else	KDB
#ifdef	DEBUG
		printf("reset(%d) from runpcs()\n", runmode);
#endif	DEBUG
		reset(runmode);
}

INT execbkptf = 0;

/*
 * determines whether to stop, and what to print if so
 * flag:	1 if entered by trace trap
 * execsig:	(seems not to be used by kernel debugger)
 *
 * exits:
 *	skipping breakpoint (execbkptf != 0):
 *		runpcs(CONTIN)
 *      next iteration of breakpoint:
 *		runpcs(CONTIN)
 *	next iteration of single-step:
 *		runpcs(SINGLE)
 *
 *	stopped by breakpoint:
 *		returns 1
 *	stopped by single-step, or
 *		by CALL/RET:
 *		returns 0
 *
 *	normal return MUST reset sstepmode!
 */

nextpcs(flag, execsig)
{
	INT		rc;
	REG BKPTR	bkpt;

	pcb.pcb_psl &= ~PSL_T;
	signo = flag?SIGTRAP:0;
	delbp();
	if (execbkptf)
	{
	    execbkptf = 0;
	    runpcs(CONTIN, 1);
	}
#endif	KDB

		IF (signo==0) ANDF (bkpt=scanbkpt(userpc))
		THEN /* stopped by BPT instruction */
#ifdef DEBUG
			printf("\n BPT code; '%s'%o'%o'%d",
				bkpt->comm,bkpt->comm[0],EOR,bkpt->flag);
#endif
			dot=bkpt->loc;
			IF bkpt->flag==BKPTEXEC
			ORF ((bkpt->flag=BKPTEXEC)
				ANDF bkpt->comm[0]!=EOR
				ANDF command(bkpt->comm,':')
				ANDF --bkpt->count)
#ifndef	KDB
			THEN execbkpt(bkpt,execsig); execsig=0; loopcnt++;
#else	KDB
			THEN loopcnt++; execbkpt(bkpt,execsig); execsig=0;
#endif	KDB
			ELSE bkpt->count=bkpt->initcnt; rc=1;
			FI
		ELSE execsig=signo; rc=0;
		FI
#ifndef	KDB
	OD
#else	KDB
	if (--loopcnt > 0) {
	    if (sstepmode == STEP_PRINT){
		printf("%16t");
	    	printpc();
	    }
	    runpcs(rc?CONTIN:SINGLE, 1);
	}
#ifdef	DBG
	if (sstepmode == STEP_CALLT){
		/* keep going until CALL or RETURN */
		int	ins;

		ins = chkget(dot,ISP) & 0xff;
		if (ins == I_CALLS || ins == I_CALLG ||
		    ins == I_RET) {
			printf("%d instructions executed\n", icount);
		}
		else {
			loopcnt++;
			icount++;
			runpcs(SINGLE, 1);
		}
	}
	sstepmode = STEP_NONE;	/* don't wait for CALL/RET */
#endif	DBG
#endif	KDB
	return(rc);
}

#define BPOUT 0
#define BPIN 1
INT bpstate = BPOUT;

#ifndef	KDB
endpcs()
{
	REG BKPTR	bkptr;
	IF pid
	THEN ptrace(EXIT,pid,0,0); pid=0; userpc=1;
	     FOR bkptr=bkpthead; bkptr; bkptr=bkptr->nxtbkpt
	     DO IF bkptr->flag
		THEN bkptr->flag=BKPTSET;
		FI
	     OD
	FI
	bpstate=BPOUT;
}
#endif	KDB

#ifdef VFORK
nullsig()
{

}
#endif

#ifndef	KDB
setup()
{
	close(fsym); fsym = -1;
#ifndef VFORK
	IF (pid = fork()) == 0
#else
	IF (pid = vfork()) == 0
#endif
	THEN ptrace(SETTRC,0,0,0);
#ifdef VFORK
	     signal(SIGTRAP,nullsig);
#endif
	     signal(SIGINT,sigint); signal(SIGQUIT,sigqit);
	     doexec(); exit(0);
	ELIF pid == -1
	THEN error(NOFORK);
	ELSE bpwait(); readregs(); lp[0]=EOR; lp[1]=0;
	     fsym=open(symfil,wtflag);
	     IF errflg
	     THEN printf("%s: cannot execute\n",symfil);
		  endpcs(); error(0);
	     FI
	FI
	bpstate=BPOUT;
}
#endif	KDB

execbkpt(bkptr,execsig)
BKPTR	bkptr;
{
#ifdef DEBUG
	printf("exbkpt: %d\n",bkptr->count);
#endif
	delbp();
#ifndef	KDB
	ptrace(SINGLE,pid,bkptr->loc,execsig);
	bkptr->flag=BKPTSET;
	bpwait(); chkerr(); readregs();
#else	KDB
	bkptr->flag=BKPTSET;
	execbkptf++;
#ifdef	DEBUG
	printf("reset(%d) from execbkpt()\n", SINGLE);
#endif	DEBUG
	reset(SINGLE);
#endif	KDB
}


#ifndef	KDB
doexec()
{
	STRING		argl[MAXARG];
	CHAR		args[LINSIZ];
	STRING		p, *ap, filnam;
	extern STRING environ;
	ap=argl; p=args;
	*ap++=symfil;
	REP	IF rdc()==EOR THEN break; FI
		*ap = p;
	
		/* 
		 * Store the arg if not either a '>' or '<'
		 */
		WHILE lastc!=EOR ANDF lastc!=SP ANDF lastc!=TB ANDF lastc!='>' ANDF lastc!='<'							/* slr001 */
			DO *p++=lastc; readchar(); OD		/* slr001 */
		*p++ = '\0';					/* slr001 */
	 	ap++;						/* slr001 */

		/*
		 * First thing is to look for direction characters
		 * and get filename.  Do not use up the args for filenames.
		 */
		IF lastc=='<'
		THEN	REP readchar(); PER lastc==SP ORF lastc==TB DONE
			filnam = p;
			WHILE lastc!=EOR ANDF lastc!=SP ANDF lastc!=TB ANDF lastc!='>'
				DO *p++=lastc; readchar(); OD
			*p = 0;
			close(0);
			IF open(filnam,0)<0
			THEN	printf("%s: cannot open\n",filnam); _exit(0);
			FI
			p = *--ap; /* slr001 were on arg ahead of ourselves */
		ELIF lastc=='>'
		THEN	REP readchar(); PER lastc==SP ORF lastc==TB DONE
			filnam = p;
			WHILE lastc!=EOR ANDF lastc!=SP ANDF lastc!=TB ANDF lastc!='<'
				DO *p++=lastc; readchar(); OD
			*p = '\0';
			close(1);
			IF creat(filnam,0666)<0
			THEN	printf("%s: cannot create\n",filnam); _exit(0);
			FI
			p = *--ap; /* slr001 were on arg ahead of ourselves */
		FI

	PER lastc!=EOR DONE
	*ap++=0;
	exect(symfil, argl, environ);
	perror(symfil);
}
#endif	KDB

BKPTR	scanbkpt(adr)
ADDR adr;
{
	REG BKPTR	bkptr;
	FOR bkptr=bkpthead; bkptr; bkptr=bkptr->nxtbkpt
	DO IF bkptr->flag ANDF bkptr->loc==adr
	   THEN break;
	   FI
	OD
	return(bkptr);
}

delbp()
{
	REG ADDR	a;
	REG BKPTR	bkptr;
	IF bpstate!=BPOUT
	THEN
		FOR bkptr=bkpthead; bkptr; bkptr=bkptr->nxtbkpt
		DO	IF bkptr->flag
			THEN a=bkptr->loc;
#ifndef	KDB
				IF a < txtmap.e1 THEN
				ptrace(WIUSER,pid,a,
					(bkptr->ins&0xFF)|(ptrace(RIUSER,pid,a,0)&~0xFF));
				ELSE
				ptrace(WDUSER,pid,a,
					(bkptr->ins&0xFF)|(ptrace(RDUSER,pid,a,0)&~0xFF));
				FI
#else	KDB
				put(a, ISP, (bkptr->ins&0xff)|(get(a, ISP)&~0xff));
#endif	KDB
			FI
		OD
		bpstate=BPOUT;
	FI
}

setbp()
{
	REG ADDR		a;
	REG BKPTR	bkptr;

	IF bpstate!=BPIN
	THEN
		FOR bkptr=bkpthead; bkptr; bkptr=bkptr->nxtbkpt
		DO IF bkptr->flag
		   THEN a = bkptr->loc;
#ifndef	KDB
			IF a < txtmap.e1 THEN
				bkptr->ins = ptrace(RIUSER, pid, a, 0);
				ptrace(WIUSER, pid, a, BPT | (bkptr->ins&~0xFF));
			ELSE
				bkptr->ins = ptrace(RDUSER, pid, a, 0);
				ptrace(WDUSER, pid, a, BPT | (bkptr->ins&~0xFF));
			FI
#else	KDB
			bkptr->ins = get(a, ISP);
			put(a, ISP, BPT | (bkptr->ins)&~0xff);
#endif	KDB
			IF errno
			THEN prints("cannot set breakpoint: ");
			     psymoff(bkptr->loc,ISYM,"\n");
			FI
		   FI
		OD
		bpstate=BPIN;
	FI
}

#ifndef	KDB
bpwait()
{
	REG ADDR w;
	ADDR stat;

	signal(SIGINT, 1);
	WHILE (w = wait(&stat))!=pid ANDF w != -1 DONE
	signal(SIGINT,sigint);
	IF w == -1
	THEN pid=0;
	     errflg=BADWAIT;
	ELIF (stat & 0177) != 0177
	THEN sigcode = 0;
	     IF signo = stat&0177
	     THEN sigprint();
	     FI
	     IF stat&0200
	     THEN prints(" - core dumped");
		  close(fcor);
		  setcor();
	     FI
	     pid=0;
	     errflg=ENDPCS;
	ELSE signo = stat>>8;
	     sigcode = ptrace(RUREGS, pid, &((struct user *)0)->u_code, 0);
	     IF signo!=SIGTRAP
	     THEN sigprint();
	     ELSE signo=0;
	     FI
	     flushbuf();
	FI
}

readregs()
{
	/*get REG values from pcs*/
	REG i;
	FOR i=24; --i>=0; 
	DO *(ADDR *)(((ADDR)&u)+reglist[i].roffs) =
		    ptrace(RUREGS, pid, reglist[i].roffs, 0);
	OD
 	userpc= *(ADDR *)(((ADDR)&u)+PC);
}
#endif	KDB
