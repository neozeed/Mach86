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
static	char sccsid[] = "@(#)print.c 4.7 8/14/83";
/*
 *
 *	UNIX debugger
 *
 */
#include "defs.h"

#ifdef	CMU
/*
 **************************************************************************
 * HISTORY
 * 16-May-86  David Golub (dbg) at Carnegie-Mellon University
 *	Support for multiple-process debugging
 *
 * 11-Oct-85  Robert V Baron (rvb) at Carnegie-Mellon University
 *	Flushed the stupid #
 **************************************************************************
 */
#endif	CMU

#ifdef	KDB
#include <sys/proc.h>
#endif	KDB

MSG		LONGFIL;
MSG		NOTOPEN;
MSG		A68BAD;
MSG		A68LNK;
MSG		BADMOD;

MAP		txtmap;
MAP		datmap;

ADDR		lastframe;
ADDR		callpc;

INT		infile;
INT		outfile;
CHAR		*lp;
L_INT		maxoff;
L_INT		maxpos;
INT		radix;

/* symbol management */
L_INT		localval;

/* breakpoints */
BKPTR		bkpthead;

REGLIST reglist [] = {
	"p1lr",	P1LR,	&pcb.pcb_p1lr,
#ifdef	CMU
	"p1br",	P1BR,	(int *)&pcb.pcb_p1br,
#else	CMU
	"p1br",	P1BR,	&pcb.pcb_p1br,
#endif	CMU
	"p0lr",	P0LR,	&pcb.pcb_p0lr,
#ifdef	CMU
	"p0br",	P0BR,	(int *)&pcb.pcb_p0br,
#else	CMU
	"p0br",	P0BR,	&pcb.pcb_p0br,
#endif	CMU
	"ksp",	KSP,	&pcb.pcb_ksp,
	"esp",	ESP,	&pcb.pcb_esp,
	"ssp",	SSP,	&pcb.pcb_ssp,
	"psl",	PSL,	&pcb.pcb_psl,
	"pc",	PC,	&pcb.pcb_pc,
	"usp",	USP,	&pcb.pcb_usp,
	"fp",	FP,	&pcb.pcb_fp,
	"ap",	AP,	&pcb.pcb_ap,
	"r11",	R11,	&pcb.pcb_r11,
	"r10",	R10,	&pcb.pcb_r10,
	"r9",	R9,	&pcb.pcb_r9,
	"r8",	R8,	&pcb.pcb_r8,
	"r7",	R7,	&pcb.pcb_r7,
	"r6",	R6,	&pcb.pcb_r6,
	"r5",	R5,	&pcb.pcb_r5,
	"r4",	R4,	&pcb.pcb_r4,
	"r3",	R3,	&pcb.pcb_r3,
	"r2",	R2,	&pcb.pcb_r2,
	"r1",	R1,	&pcb.pcb_r1,
	"r0",	R0,	&pcb.pcb_r0,
};

char		lastc;

INT		fcor;
STRING		errflg;
INT		signo;
INT		sigcode;


L_INT		dot;
L_INT		var[];
STRING		symfil;
STRING		corfil;
INT		pid;
L_INT		adrval;
INT		adrflg;
L_INT		cntval;
INT		cntflg;

STRING		signals[] = {
		"",
		"hangup",
		"interrupt",
		"quit",
		"illegal instruction",
		"trace/BPT",
		"IOT",
		"EMT",
		"floating exception",
		"killed",
		"bus error",
		"memory fault",
		"bad system call",
		"broken pipe",
		"alarm call",
		"terminated",
		"signal 16",
		"stop (signal)",
		"stop (tty)",
		"continue (signal)",
		"child termination",
		"stop (tty input)",
		"stop (tty output)",
		"input available (signal)",
		"cpu timelimit",
		"file sizelimit",
		"signal 26",
		"signal 27",
		"signal 28",
		"signal 29",
		"signal 30",
		"signal 31",
};

/* general printing routines ($) */

printtrace(modif)
{
	INT		narg, i, stat, name, limit;
	POS		dynam;
	REG BKPTR	bkptr;
	CHAR		hi, lo;
	ADDR		word;
	STRING		comptr;
	ADDR		argp, frame, link;
	register struct nlist *sp;
	INT		stack;
	INT		ntramp;

	IF cntflg==0 THEN cntval = -1; FI

	switch (modif) {

#ifndef	KDB
	    case '<':
		IF cntval == 0
		THEN	WHILE readchar() != EOR
			DO OD
			lp--;
			break;
		FI
		IF rdc() == '<'
		THEN	stack = 1;
		ELSE	stack = 0; lp--;
		FI
							/* fall through */
	    case '>':
		{CHAR		file[64];
		CHAR		Ifile[128];
		extern CHAR	*Ipath;
		INT		index;

		index=0;
		IF rdc()!=EOR
		THEN	REP file[index++]=lastc;
			    IF index>=63 THEN error(LONGFIL); FI
			PER readchar()!=EOR DONE
			file[index]=0;
			IF modif=='<'
			THEN	IF Ipath THEN
					strcpy(Ifile, Ipath);
					strcat(Ifile, "/");
					strcat(Ifile, file);
				FI
				IF strcmp(file, "-")!=0
				THEN	iclose(stack, 0);
					infile=open(file,0);
					IF infile<0
					THEN	infile=open(Ifile,0);
					FI
				ELSE	lseek(infile, 0L, 0);
				FI
				IF infile<0
				THEN	infile=0; error(NOTOPEN);
				ELSE	IF cntflg
					THEN	var[9] = cntval;
					ELSE	var[9] = 1;
					FI
				FI
			ELSE	oclose();
				outfile=open(file,1);
				IF outfile<0
				THEN	outfile=creat(file,0644);
#ifndef EDDT
				ELSE	lseek(outfile,0L,2);
#endif
				FI
			FI

		ELSE	IF modif == '<'
			THEN	iclose(-1, 0);
			ELSE	oclose();
			FI
		FI
		lp--;
		}
		break;

	    case 'p':
		IF kernel == 0 THEN
			printf("not debugging kernel\n");
		ELSE
			IF adrflg THEN
				int pte = access(RD, dot, DSP, 0);
				masterpcbb = (pte&PG_PFNUM)*512;
			FI
			getpcb();
		FI
		break;
#else	KDB
#if	MACH_VM
	    case 'p':
		/*
		 *	pid$p	sets process to pid
		 *	$p	sets process to the one that invoked
		 *		kdb
		 */
		{
			struct proc 	*p;
			extern struct proc *	pfind();

			extern void	kdbgetprocess();

			if (adrflg) {
				p = pfind(dot);
			}
			else {
				p = pfind(curpid);	/* pid at entry */
			}
			if (p) {
				printf("proc entry at 0x%X\n", p);
				kdbgetprocess(p, &curmap, &curpcb);
				var[varchk('m')] = curmap;
				pid = dot;
				if (pid == curpid) {
					/*
					 * pcb for entry process is saved
					 * register set
					 */
					 curpcb = &pcb;
				}
			}
			else {
				printf("no such process");
			}
		}
		break;

	    case 'l':
		/*
		 *	print list of all active processes (as in PS)
		 */
		{
			struct proc	*p;
			extern struct proc	*allproc;
			int		wait_channel;

			for (p = allproc; p; p = p->p_nxt) {

				wait_channel = (int)p->p_wchan;
				printf("entry 0x%X pid %5d %c",
					p, p->p_pid,
					p->p_stat == SSLEEP ? 'S' :
					p->p_stat == SRUN   ? 'R' :
					p->p_stat == SIDL   ? 'I' :
					p->p_stat == SSTOP  ? 'T' : '?');
				if (wait_channel) {
					prints(" wait ");
					psymoff(wait_channel, ISYM, "");
				}
				printc(EOR);
			}
		}
#endif	MACH_VM
#endif	KDB

	    case 'd':
		if (adrflg) {
			if (adrval<2 || adrval>16) {printf("must have 2 <= radix <= 16"); break;}
			printf("radix=%d base ten",radix=adrval);
		}
		break;
#ifndef	KDB

	    case 'q': case 'Q': case '%':
		done();
#endif	KDB

	    case 'w': case 'W':
		maxpos=(adrflg?adrval:MAXPOS);
		break;

	    case 's': case 'S':
		maxoff=(adrflg?adrval:MAXOFF);
		break;

	    case 'v': case 'V':
		prints("variables\n");
		FOR i=0;i<=35;i++
		DO IF var[i]
		   THEN printc((i<=9 ? '0' : 'a'-10) + i);
			printf(" = %X\n",var[i]);
		   FI
		OD
		break;

	    case 'm': case 'M':
#if	MACH_VM
		vm_map_print(dot);
#else	MACH_VM
		printmap("? map",&txtmap);
		printmap("/ map",&datmap);
#endif	MACH_VM
		break;

	    case 0: case '?':
		IF pid
		THEN printf("pcs id = %d\n",pid);
		ELSE prints("no process\n");
		FI
		sigprint(); flushbuf();

	    case 'r': case 'R':
		printregs();
		return;

	case 'c': 
	case 'C':
		oldtrace(modif) ;
		break;

	case 'n': 
	case 'N':
		newtrace(modif) ;
		break;

	    /*print externals*/
	    case 'e': case 'E':
		for (sp = symtab; sp < esymtab; sp++) {
		   if (sp->n_type==(N_DATA|N_EXT) ORF sp->n_type==(N_BSS|N_EXT))
		   	printf("%s:%12t%R\n", sp->n_un.n_name, get(sp->n_value,DSP));
		}
		break;

	    case 'a': case 'A':
		error("No algol 68 on VAX");
		/*NOTREACHED*/

	    /*print breakpoints*/
	    case 'b': case 'B':
		printf("breakpoints\ncount%8tbkpt%24tcommand\n");
		for (bkptr=bkpthead; bkptr; bkptr=bkptr->nxtbkpt)
			if (bkptr->flag) {
		   		printf("%-8.8d",bkptr->count);
				psymoff(leng(bkptr->loc),ISYM,"%24t");
				comptr=bkptr->comm;
				WHILE *comptr DO printc(*comptr++); OD
			}
		break;

	    default: error(BADMOD);
	}
}

printmap(s,amap)
STRING	s; MAP *amap;
{
	int file;
	file=amap->ufd;
	printf("%s%12t`%s'\n",s,(file<0 ? "-" : (file==fcor ? corfil : symfil)));
	printf("b1 = %-16R",amap->b1);
	printf("e1 = %-16R",amap->e1);
	printf("f1 = %-16R",amap->f1);
	printf("\nb2 = %-16R",amap->b2);
	printf("e2 = %-16R",amap->e2);
	printf("f2 = %-16R",amap->f2);
	printc(EOR);
}

printregs()
{
	REG REGPTR	p;
	L_INT		v;

	FOR p=reglist; p < &reglist[24]; p++
#if	MACH_VM
	DO	v = kcore ? *(int *)((int)p->rkern - (int)&pcb + (int)curpcb)
			  : *(ADDR *)(((ADDR)&u)+p->roffs);
#else	MACH_VM
	DO	v = kcore ? *p->rkern : *(ADDR *)(((ADDR)&u)+p->roffs);
#endif	MACH_VM
		printf("%s%6t%R %16t", p->rname, v);
		valpr(v,(p->roffs==PC?ISYM:DSYM));
		printc(EOR);
	OD
	printpc();
}

getreg(regnam) {
	REG REGPTR	p;
	REG STRING	regptr;
	CHAR	*olp;
	CHAR		regnxt;

	olp=lp;
	FOR p=reglist; p < &reglist[24]; p++
	DO	regptr=p->rname;
		IF (regnam == *regptr++)
		THEN
			WHILE *regptr
			DO IF (regnxt=readchar()) != *regptr++
				THEN --regptr; break;
				FI
			OD
			IF *regptr
			THEN lp=olp;
			ELSE
#if	MACH_VM
				int i = kcore ? (int)p->rkern - (int)&pcb + (int)curpcb
					      : p->roffs;
#else	MACH_VM
				int i = kcore ? (int)p->rkern : p->roffs;
#endif	MACH_VM
				return (i);
			FI
		FI
	OD
	lp=olp;
	return(0);
}

printpc()
{
#ifndef	KDB
	dot= *(ADDR *)(((ADDR)&u)+PC);
#endif	KDB
	psymoff(dot,ISYM,":%16t"); printins(0,ISP,chkget(dot,ISP));
	printc(EOR);
}

char	*illinames[] = {
	"reserved addressing fault",
	"privileged instruction fault",
	"reserved operand fault"
};
char	*fpenames[] = {
	0,
	"integer overflow trap",
	"integer divide by zero trap",
	"floating overflow trap",
	"floating/decimal divide by zero trap",
	"floating underflow trap",
	"decimal overflow trap",
	"subscript out of range trap",
	"floating overflow fault",
	"floating divide by zero fault",
	"floating undeflow fault"
};

sigprint()
{
	IF (signo>=0) ANDF (signo<sizeof signals/sizeof signals[0])
	THEN prints(signals[signo]); FI
	switch (signo) {

	case SIGFPE:
		IF (sigcode > 0 &&
		    sigcode < sizeof fpenames / sizeof fpenames[0]) THEN
			prints(" ("); prints(fpenames[sigcode]); prints(")");
		FI
		break;

	case SIGILL:
		IF (sigcode >= 0 &&
		    sigcode < sizeof illinames / sizeof illinames[0]) THEN
			prints(" ("); prints(illinames[sigcode]); prints(")");
		FI
		break;
	}
}
