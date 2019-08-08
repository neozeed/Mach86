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
#define	access	kdbaccess
#define	bchkget	kdbbchkget
#define	bignumprint	kdbbignumprint
#define	bpwait	kdbbpwait
#define	bsbrk	kdbbsbrk
#define	casebody	kdbcasebody
#define	charpos	kdbcharpos
#define	chkerr	kdbchkerr
#define	chkget	kdbchkget
#define	chkloc	kdbchkloc
#define	chkmap	kdbchkmap
#define	command	kdbcommand
#define	convdig	kdbconvdig
#define	convert	kdbconvert
#define	delbp	kdbdelbp
#define	digit	kdbdigit
#define	dispaddress	kdbdispaddress
#define	done	kdbdone
#define	editchar	kdbeditchar
#define	endline	kdbendline
#define	endpcs	kdbendpcs
#define	eol	kdbeol
#define	eqstr	kdbeqstr
#define	eqsym	kdbeqsym
#define	error	kdberror
#define	execbkpt	kdbexecbkpt
#define	execbkptf	kdbexecbkptf
#define	exform	kdbexform
#define	expr	kdbexpr
#define	findsym	kdbfindsym
#define	flushbuf	kdbflushbuf
#define	get	kdbget
#define	getformat	kdbgetformat
#define	getnum	kdbgetnum
#define	getreg	kdbgetreg
#define	getsig	kdbgetsig
#define	iclose	kdbiclose
#define	inkdot	kdbinkdot
#define	insregname	kdbinsregname
#define	item	kdbitem
#define	length	kdblength
#define	localsym	kdblocalsym
#define	lookup	kdblookup
#define	mapescbyte	kdbmapescbyte
#define	mkioptab	kdbmkioptab
#define	newline	kdbnewline
#define	nextchar	kdbnextchar
#define	nextpcs	kdbnextpcs
#define	oclose	kdboclose
#define	operandout	kdboperandout
#define	pcimmediate	kdbpcimmediate
#define	physrw	kdbphysrw
#define	printc	kdbprintc
#define	printdate	kdbprintdate
#define	printdbl	kdbprintdbl
#define	printesc	kdbprintesc
#define	printf	kdbprintf
#define	printins	kdbprintins
#define	printmap	kdbprintmap
#define	printnum	kdbprintnum
#define	printoct	kdbprintoct
#define	printpc	kdbprintpc
#define	printregs	kdbprintregs
#define	prints	kdbprints
#define	printtrace	kdbprinttrace
#define	psymoff	kdbpsymoff
#define	put	kdbput
#define	putchar	cnputc
#define	quotchar	kdbquotchar
#define	rdc	kdbrdc
#define	rdfp	kdbrdfp
#define	read	kdbread
#define	readchar	kdbreadchar
#define	readregs	kdbreadregs
#define	readsym	kdbreadsym
#define	rintr	kdbrintr
#define	round	kdbround
#define	runpcs	kdbrunpcs
#define	rwerr	kdbrwerr
#define	sbrk	kdbsbrk
#define	scanbkpt	kdbscanbkpt
#define	scanform	kdbscanform
#define	setbp	kdbsetbp
#define	setcor	kdbsetcor
#define	setsym	kdbsetsym
#define	setup	kdbsetup
#define	setvar	kdbsetvar
#define	shell	kdbshell
#define	shortliteral	kdbshortliteral
#define	sigprint	kdbsigprint
#define	snarf	kdbsnarf
#define	snarfreloc	kdbsnarfreloc
#define	snarfuchar	kdbsnarfuchar
#define	subpcs	kdbsubpcs
#define	symchar	kdbsymchar
#define	term	kdbterm
#define	valpr	kdbvalpr
#define	varchk	kdbvarchk
#define	within	kdbwithin
#define	write	kdbwrite

#define	adrflg	kdbadrflg
#define	adrval	kdbadrval
#define	bkpthead	kdbbkpthead
#define	bpstate	kdbbpstate
#define	callpc	kdbcallpc
#define	cntflg	kdbcntflg
#define	cntval	kdbcntval
#define	corfil	kdbcorfil
#define	cursym	kdbcursym
#define	datbas	kdbdatbas
#define	datmap	kdbdatmap
#define	digitptr	kdbdigitptr
#define	ditto	kdbditto
#define	dot	kdbdot
#define	dotinc	kdbdotinc
#define	entrypt	kdbentrypt
#define	eof	kdbeof
#define	eqformat	kdbeqformat
#define	erasec	kdberasec
#define	errflg	kdberrflg
#define	errno	kdberrno
#define	esymtab	kdbesymtab
#define	executing	kdbexecuting
#define	exitflg	kdbexitflg
#define	expv	kdbexpv
#define	fcor	kdbfcor
#define	filhdr	kdbfilhdr
#define	fltimm	kdbfltimm
#define	fpenames	kdbfpenames
#define	fphack	kdbfphack
#define	fsym	kdbfsym
#define	ifiledepth	kdbifiledepth
#define	illinames	kdbillinames
#define	incp	kdbincp
#define	infile	kdbinfile
#define	insoutvar	kdbinsoutvar
#define	insttab	kdbinsttab
#define	ioptab	kdbioptab
#define	istack	kdbistack
#define	isymbol	kdbisymbol
#define	itolws	kdbitolws
#define	kcore	kdbkcore
#define	kernel	kdbkernel
#define	lastc	kdblastc
#define	lastcom	kdblastcom
#define	lastframe	kdblastframe
#define	line	kdbline
#define	localval	kdblocalval
#define	locmsk	kdblocmsk
#define	locval	kdblocval
#define	loopcnt	kdbloopcnt
#define	lp	kdblp
#define	masterpcbb	kdbmasterpcbb
#define	maxfile	kdbmaxfile
#define	maxoff	kdbmaxoff
#define	maxpos	kdbmaxpos
#define	maxstor	kdbmaxstor
#define	mkfault	kdbmkfault
#define	outfile	kdboutfile
#define	pcb	kdbpcb
#define	peekc	kdbpeekc
#define	pid	kdbpid
#define	printbuf	kdbprintbuf
#define	printptr	kdbprintptr
#define	radix	kdbradix
#define	reglist	kdbreglist
#define	regname	kdbregname
#define	savframe	kdbsavframe
#define	savlastf	kdbsavlastf
#define	savpc	kdbsavpc
#define	sbr	kdbsbr
#define	sigcode	kdbsigcode
#define	sigint	kdbsigint
#define	signals	kdbsignals
#define	signo	kdbsigno
#define	sigqit	kdbsigqit
#define	slr	kdbslr
#define	space	kdbspace
#define	stformat	kdbstformat
#define	stksiz	kdbstksiz
#define	symfil	kdbsymfil
#define	symtab	kdbsymtab
#define	systab	kdbsystab
#define	txtmap	kdbtxtmap
#define	ty_NORELOC	kdbty_NORELOC
#define	ty_nbyte	kdbty_nbyte
#define	type	kdbtype
#define	udot	kdbudot
#define	userpc	kdbuserpc
#define	var	kdbvar
#define	wtflag	kdbwtflag
#define	wtflg	kdbwtflg
#define	xargc	kdbxargc

#define curmap	kdbcurmap
#define	curpcb	kdbcurpcb
#define curpid	kdbcurpid
