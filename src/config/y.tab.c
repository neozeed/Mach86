
# line 1 "config.y"
typedef union  {
	char	*str;
	int	val;
	struct	file_list *file;
	struct	idlst *lst;
} YYSTYPE;
# define AND 257
# define ANY 258
# define ARGS 259
# define AT 260
# define COMMA 261
# define CONFIG 262
# define CONTROLLER 263
# define CPU 264
# define CSR 265
# define DEVICE 266
# define DISK 267
# define DRIVE 268
# define DST 269
# define DUMPS 270
# define EQUALS 271
# define FLAGS 272
# define HZ 273
# define IDENT 274
# define MACHINE 275
# define MAJOR 276
# define MASTER 277
# define MAXUSERS 278
# define MBA 279
# define MINOR 280
# define MINUS 281
# define NEXUS 282
# define ON 283
# define OPTIONS 284
# define PRIORITY 285
# define PSEUDO_DEVICE 286
# define ROOT 287
# define SEMICOLON 288
# define SIZE 289
# define SLAVE 290
# define SWAP 291
# define TIMEZONE 292
# define TRACE 293
# define UBA 294
# define VECTOR 295
# define ID 296
# define NUMBER 297
# define FPNUMBER 298

# line 65 "config.y"

/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)config.y	5.1 (Berkeley) 5/8/85
 */

#include "config.h"
#include <ctype.h>
#include <stdio.h>

struct	device cur;
struct	device *curp = 0;
char	*temp_id;
char	*val_id;
char	*malloc();

#define yyclearin yychar = -1
#define yyerrok yyerrflag = 0
extern int yychar;
extern short yyerrflag;
#ifndef YYMAXDEPTH
#define YYMAXDEPTH 150
#endif
YYSTYPE yylval, yyval;
# define YYERRCODE 256

# line 466 "config.y"


yyerror(s)
	char *s;
{

	fprintf(stderr, "config: line %d: %s\n", yyline, s);
}

/*
 * return the passed string in a new space
 */
char *
ns(str)
	register char *str;
{
	register char *cp;

	cp = malloc((unsigned)(strlen(str)+1));
	(void) strcpy(cp, str);
	return (cp);
}

/*
 * add a device to the list of devices
 */
newdev(dp)
	register struct device *dp;
{
	register struct device *np;

	np = (struct device *) malloc(sizeof *np);
	*np = *dp;
	if (curp == 0)
		dtab = np;
	else
		curp->d_next = np;
	curp = np;
}

/*
 * note that a configuration should be made
 */
mkconf(sysname)
	char *sysname;
{
	register struct file_list *fl, **flp;

	fl = (struct file_list *) malloc(sizeof *fl);
	fl->f_type = SYSTEMSPEC;
	fl->f_needs = sysname;
	fl->f_rootdev = NODEV;
	fl->f_argdev = NODEV;
	fl->f_dumpdev = NODEV;
	fl->f_fn = 0;
	fl->f_next = 0;
	for (flp = confp; *flp; flp = &(*flp)->f_next)
		;
	*flp = fl;
	confp = flp;
}

struct file_list *
newswap()
{
	struct file_list *fl = (struct file_list *)malloc(sizeof (*fl));

	fl->f_type = SWAPSPEC;
	fl->f_next = 0;
	fl->f_swapdev = NODEV;
	fl->f_swapsize = 0;
	fl->f_needs = 0;
	fl->f_fn = 0;
	return (fl);
}

/*
 * Add a swap device to the system's configuration
 */
mkswap(system, fl, size)
	struct file_list *system, *fl;
	int size;
{
	register struct file_list **flp;
	char *cp, name[80];

	if (system == 0 || system->f_type != SYSTEMSPEC) {
		yyerror("\"swap\" spec precedes \"config\" specification");
		return;
	}
	if (size < 0) {
		yyerror("illegal swap partition size");
		return;
	}
	/*
	 * Append swap description to the end of the list.
	 */
	flp = &system->f_next;
	for (; *flp && (*flp)->f_type == SWAPSPEC; flp = &(*flp)->f_next)
		;
	fl->f_next = *flp;
	*flp = fl;
	fl->f_swapsize = size;
	/*
	 * If first swap device for this system,
	 * set up f_fn field to insure swap
	 * files are created with unique names.
	 */
	if (system->f_fn)
		return;
	if (eq(fl->f_fn, "generic"))
		system->f_fn = ns(fl->f_fn);
	else
		system->f_fn = ns(system->f_needs);
}

/*
 * find the pointer to connect to the given device and number.
 * returns 0 if no such device and prints an error message
 */
struct device *
connect(dev, num)
	register char *dev;
	register int num;
{
	register struct device *dp;
	struct device *huhcon();

	if (num == QUES)
		return (huhcon(dev));
	for (dp = dtab; dp != 0; dp = dp->d_next) {
		if ((num != dp->d_unit) || !eq(dev, dp->d_name))
			continue;
		if (dp->d_type != CONTROLLER && dp->d_type != MASTER) {
			yyerror(sprintf(errbuf,
			    "%s connected to non-controller", dev));
			return (0);
		}
		return (dp);
	}
	yyerror(sprintf(errbuf, "%s %d not defined", dev, num));
	return (0);
}

/*
 * connect to an unspecific thing
 */
struct device *
huhcon(dev)
	register char *dev;
{
	register struct device *dp, *dcp;
	struct device rdev;
	int oldtype;

	/*
	 * First make certain that there are some of these to wildcard on
	 */
	for (dp = dtab; dp != 0; dp = dp->d_next)
		if (eq(dp->d_name, dev))
			break;
	if (dp == 0) {
		yyerror(sprintf(errbuf, "no %s's to wildcard", dev));
		return (0);
	}
	oldtype = dp->d_type;
	dcp = dp->d_conn;
	/*
	 * Now see if there is already a wildcard entry for this device
	 * (e.g. Search for a "uba ?")
	 */
	for (; dp != 0; dp = dp->d_next)
		if (eq(dev, dp->d_name) && dp->d_unit == -1)
			break;
	/*
	 * If there isn't, make one because everything needs to be connected
	 * to something.
	 */
	if (dp == 0) {
		dp = &rdev;
		init_dev(dp);
		dp->d_unit = QUES;
		dp->d_name = ns(dev);
		dp->d_type = oldtype;
		newdev(dp);
		dp = curp;
		/*
		 * Connect it to the same thing that other similar things are
		 * connected to, but make sure it is a wildcard unit
		 * (e.g. up connected to sc ?, here we make connect sc? to a
		 * uba?).  If other things like this are on the NEXUS or
		 * if they aren't connected to anything, then make the same
		 * connection, else call ourself to connect to another
		 * unspecific device.
		 */
		if (dcp == TO_NEXUS || dcp == 0)
			dp->d_conn = dcp;
		else
			dp->d_conn = connect(dcp->d_name, QUES);
	}
	return (dp);
}

init_dev(dp)
	register struct device *dp;
{

	dp->d_name = "OHNO!!!";
	dp->d_type = DEVICE;
	dp->d_conn = 0;
	dp->d_vec = 0;
	dp->d_addr = dp->d_pri = dp->d_flags = dp->d_dk = 0;
	dp->d_slave = dp->d_drive = dp->d_unit = UNKNOWN;
}

/*
 * make certain that this is a reasonable type of thing to connect to a nexus
 */
check_nexus(dev, num)
	register struct device *dev;
	int num;
{

	switch (machine) {

	case MACHINE_VAX:
		if (!eq(dev->d_name, "uba") && !eq(dev->d_name, "mba"))
			yyerror("only uba's and mba's should be connected to the nexus");
		if (num != QUES)
			yyerror("can't give specific nexus numbers");
		break;

	case MACHINE_SUN:
		if (!eq(dev->d_name, "mb"))
			yyerror("only mb's should be connected to the nexus");
		break;
#ifdef	CMU

	case MACHINE_ROMP:
		if (!eq(dev->d_name, "iocc"))
			yyerror("only iocc's should be connected to the nexus");
		break;
#endif	CMU
	}
}

/*
 * Check the timezone to make certain it is sensible
 */

check_tz()
{
	if (abs(timezone) > 12 * 60)
		yyerror("timezone is unreasonable");
	else
		hadtz = 1;
}

/*
 * Check system specification and apply defaulting
 * rules on root, argument, dump, and swap devices.
 */
checksystemspec(fl)
	register struct file_list *fl;
{
	char buf[BUFSIZ];
	register struct file_list *swap;
	int generic;

	if (fl == 0 || fl->f_type != SYSTEMSPEC) {
		yyerror("internal error, bad system specification");
		exit(1);
	}
	swap = fl->f_next;
	generic = swap && swap->f_type == SWAPSPEC && eq(swap->f_fn, "generic");
	if (fl->f_rootdev == NODEV && !generic) {
		yyerror("no root device specified");
		exit(1);
	}
	/*
	 * Default swap area to be in 'b' partition of root's
	 * device.  If root specified to be other than on 'a'
	 * partition, give warning, something probably amiss.
	 */
	if (swap == 0 || swap->f_type != SWAPSPEC) {
		dev_t dev;

		swap = newswap();
		dev = fl->f_rootdev;
		if (minor(dev) & 07) {
			sprintf(buf,
"Warning, swap defaulted to 'b' partition with root on '%c' partition",
				(minor(dev) & 07) + 'a');
			yyerror(buf);
		}
		swap->f_swapdev =
		   makedev(major(dev), (minor(dev) &~ 07) | ('b' - 'a'));
		swap->f_fn = devtoname(swap->f_swapdev);
		mkswap(fl, swap, 0);
	}
	/*
	 * Make sure a generic swap isn't specified, along with
	 * other stuff (user must really be confused).
	 */
	if (generic) {
		if (fl->f_rootdev != NODEV)
			yyerror("root device specified with generic swap");
		if (fl->f_argdev != NODEV)
			yyerror("arg device specified with generic swap");
		if (fl->f_dumpdev != NODEV)
			yyerror("dump device specified with generic swap");
		return;
	}
	/*
	 * Default argument device and check for oddball arrangements.
	 */
	if (fl->f_argdev == NODEV)
		fl->f_argdev = swap->f_swapdev;
	if (fl->f_argdev != swap->f_swapdev)
		yyerror("Warning, arg device different than primary swap");
	/*
	 * Default dump device and warn if place is not a
	 * swap area or the argument device partition.
	 */
	if (fl->f_dumpdev == NODEV)
		fl->f_dumpdev = swap->f_swapdev;
	if (fl->f_dumpdev != swap->f_swapdev && fl->f_dumpdev != fl->f_argdev) {
		struct file_list *p = swap->f_next;

		for (; p && p->f_type == SWAPSPEC; p = p->f_next)
			if (fl->f_dumpdev == p->f_swapdev)
				return;
		sprintf(buf, "Warning, orphaned dump device, %s",
			"do you know what you're doing");
		yyerror(buf);
	}
}

/*
 * Verify all devices specified in the system specification
 * are present in the device specifications.
 */
verifysystemspecs()
{
	register struct file_list *fl;
	dev_t checked[50], *verifyswap();
	register dev_t *pchecked = checked;

	for (fl = conf_list; fl; fl = fl->f_next) {
		if (fl->f_type != SYSTEMSPEC)
			continue;
		if (!finddev(fl->f_rootdev))
			deverror(fl->f_needs, "root");
		*pchecked++ = fl->f_rootdev;
		pchecked = verifyswap(fl->f_next, checked, pchecked);
#define	samedev(dev1, dev2) \
	((minor(dev1) &~ 07) != (minor(dev2) &~ 07))
		if (!alreadychecked(fl->f_dumpdev, checked, pchecked)) {
			if (!finddev(fl->f_dumpdev))
				deverror(fl->f_needs, "dump");
			*pchecked++ = fl->f_dumpdev;
		}
		if (!alreadychecked(fl->f_argdev, checked, pchecked)) {
			if (!finddev(fl->f_argdev))
				deverror(fl->f_needs, "arg");
			*pchecked++ = fl->f_argdev;
		}
	}
}

/*
 * Do as above, but for swap devices.
 */
dev_t *
verifyswap(fl, checked, pchecked)
	register struct file_list *fl;
	dev_t checked[];
	register dev_t *pchecked;
{

	for (;fl && fl->f_type == SWAPSPEC; fl = fl->f_next) {
		if (eq(fl->f_fn, "generic"))
			continue;
		if (alreadychecked(fl->f_swapdev, checked, pchecked))
			continue;
		if (!finddev(fl->f_swapdev))
			fprintf(stderr,
			   "config: swap device %s not configured", fl->f_fn);
		*pchecked++ = fl->f_swapdev;
	}
	return (pchecked);
}

/*
 * Has a device already been checked
 * for it's existence in the configuration?
 */
alreadychecked(dev, list, last)
	dev_t dev, list[];
	register dev_t *last;
{
	register dev_t *p;

	for (p = list; p < last; p++)
		if (samedev(*p, dev))
			return (1);
	return (0);
}

deverror(systemname, devtype)
	char *systemname, *devtype;
{

	fprintf(stderr, "config: %s: %s device not configured\n",
		systemname, devtype);
}

/*
 * Look for the device in the list of
 * configured hardware devices.  Must
 * take into account stuff wildcarded.
 */
finddev(dev)
	dev_t dev;
{

	/* punt on this right now */
	return (1);
}
short yyexca[] ={
-1, 1,
	0, -1,
	-2, 0,
	};
# define YYNPROD 92
# define YYLAST 205
short yyact[]={

   8,  72,  73,  93,  94, 141,  23,  12,  15,  44,
   9,  11,  62, 138, 105,  85,  62,  19,  17,  14,
 136,  10,  21, 135, 134,  42,  43,  61,  16,  63,
  13,  61,   7,  63,  35, 139,  20,   6, 133, 131,
 130,  76, 126, 125, 124, 123, 117,  96,  95,  90,
  86,  45,  41,  35,  82,  40,  55, 129, 119,  27,
  26, 120,  25,  24,  81, 122, 140,  54,  98,  69,
  97,  71,  70,  68,  59, 127, 100, 104, 115,  38,
  60,  75, 103, 121,  53, 102,  80,  47,  52,  57,
 118,  28,  34,  36,  39,  29,  83,  58,  99,  51,
  50,  56,  30,  31,  32,  49,  48,  46,  22,  33,
  18,  37,   5,   4,  67,   3,   2,   1, 101, 109,
  64,  65,  66, 106, 112, 128,  92,   0,   0,   0,
   0,   0,   0,   0,  74,  77,  78,  79,   0,   0,
  84,   0,   0,   0,   0,   0,  39,   0,  91,   0,
   0,  87,  88,  89,   0,   0,   0,   0,   0, 116,
 108, 111, 114, 107, 110, 113,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0, 116, 132,   0,   0,   0,   0,
   0,   0,   0,   0, 137 };
short yypact[]={

-1000,-1000,-256,-1000,-225,-226,-228,-1000,-229,-1000,
-1000,-1000,-1000,-1000,-243,-243,-243,-241,-1000,-245,
-272,-246,-203,-243,-1000,-1000,-1000,-1000,-186,-263,
-186,-186,-186,-263,-1000,-1000,-1000,-188,-1000,-202,
-1000,-1000,-197,-198,-296,-1000,-203,-1000,-1000,-1000,
-1000,-1000,-242,-242,-242,-242,-1000,-231,-1000,-267,
-247,-1000,-1000,-1000,-231,-231,-231,-248,-243,-293,
-249,-250,-199,-201,-1000,-262,-1000,-262,-262,-262,
-1000,-243,-251,-207,-252,-253,-1000,-1000,-1000,-1000,
-1000,-1000,-1000,-1000,-1000,-1000,-1000,-254,-255,-182,
-1000,-232,-1000,-1000,-257,-258,-1000,-1000,-1000,-1000,
-1000,-1000,-1000,-1000,-1000,-1000,-243,-1000,-1000,-259,
-273,-274,-277,-1000,-1000,-1000,-1000,-262,-1000,-284,
-261,-214,-1000,-1000,-1000,-1000,-1000,-1000,-1000,-1000,
-292,-1000 };
short yypgo[]={

   0,  77, 126,  80,  78, 125,  85,  82, 124, 123,
 119, 118, 117, 116, 115, 113, 112, 111, 110, 108,
 107,  87, 106, 105, 100,  99,  81,  98,  76,  79,
  91,  89,  86,  95,  97,  96,  90 };
short yyr1[]={

   0,  12,  13,  13,  14,  14,  14,  14,  14,  16,
  16,  16,  16,  16,  16,  16,  16,  16,  16,  16,
  16,  16,  16,  16,  16,  16,  16,  16,  18,  19,
  20,  20,  21,  21,  21,  21,  22,  27,  27,  28,
  11,  11,  23,   9,   9,  24,  10,  10,  25,   8,
   8,   7,  26,  26,   5,   5,   6,   6,   6,  17,
  17,  29,  29,   2,   2,   1,   3,   3,   3,  15,
  15,  15,  15,  15,  15,  30,  33,  31,  31,  34,
  34,  35,  35,  36,  36,  36,  36,  32,  32,  32,
   4,   4 };
short yyr2[]={

   0,   1,   2,   0,   2,   2,   2,   1,   2,   2,
   2,   2,   2,   1,   2,   2,   4,   3,   2,   4,
   3,   3,   5,   4,   3,   5,   4,   2,   2,   2,
   2,   1,   1,   1,   1,   1,   3,   3,   1,   2,
   1,   1,   3,   1,   1,   3,   1,   1,   3,   1,
   1,   4,   1,   0,   2,   0,   1,   2,   3,   3,
   1,   1,   3,   1,   1,   1,   1,   1,   1,   4,
   4,   4,   4,   3,   4,   3,   0,   2,   0,   3,
   3,   2,   0,   2,   2,   2,   2,   2,   2,   0,
   1,   2 };
short yychk[]={

-1000, -12, -13, -14, -15, -16, 293, 288, 256, 266,
 277, 267, 263, 286, 275, 264, 284, 274, -18, 273,
 292, 278, -19, 262, 288, 288, 288, 288, -30, -33,
 -30, -30, -30, -33,  -1, 296,  -1, -17, -29,  -1,
 296, 297, 297, 298, 281, 297, -20, -21, -22, -23,
 -24, -25, 291, 287, 270, 259,  -1, -31, -34, 260,
  -3, 294, 279, 296, -31, -31, -31,  -3, 261, 271,
 269, 269, 297, 298, -21, -26, 283, -26, -26, -26,
 -32, 295, 285, -35,  -3, 282, 297, -32, -32, -32,
 297, -29,  -2, 296, 297, 297, 297, 269, 269, -27,
 -28, -11,  -6,  -7,  -1, 276,  -9,  -6,  -7, -10,
  -6,  -7,  -8,  -6,  -7,  -4,  -1, 297, -36, 265,
 268, 290, 272, 297, 297, 297, 297, 257,  -5, 289,
 297, 297,  -4, 297, 297, 297, 297, -28, 297, 296,
 280, 297 };
short yydef[]={

   3,  -2,   1,   2,   0,   0,   0,   7,   0,  76,
  76,  76,  76,  76,   0,   0,   0,   0,  13,   0,
   0,   0,   0,   0,   4,   5,   6,   8,  78,   0,
  78,  78,  78,   0,   9,  65,  10,  11,  60,  61,
  12,  14,  15,  18,   0,  27,  28,  31,  32,  33,
  34,  35,  53,  53,  53,  53,  29,  89,  82,   0,
   0,  66,  67,  68,  89,  89,  89,  73,   0,   0,
  17,  20,  21,  24,  30,   0,  52,   0,   0,   0,
  69,   0,   0,  77,   0,   0,  75,  70,  71,  72,
  74,  59,  62,  63,  64,  16,  19,  23,  26,  36,
  38,  55,  40,  41,  56,   0,  42,  43,  44,  45,
  46,  47,  48,  49,  50,  87,  90,  88,  81,   0,
   0,   0,   0,  79,  80,  22,  25,   0,  39,   0,
  57,   0,  91,  83,  84,  85,  86,  37,  54,  58,
   0,  51 };
#ifndef lint
static char yaccpar_sccsid[] = "@(#)yaccpar	4.1	(Berkeley)	2/11/83";
#endif not lint

#
# define YYFLAG -1000
# define YYERROR goto yyerrlab
# define YYACCEPT return(0)
# define YYABORT return(1)

/*	parser for yacc output	*/

#ifdef YYDEBUG
int yydebug = 0; /* 1 for debugging */
#endif
YYSTYPE yyv[YYMAXDEPTH]; /* where the values are stored */
int yychar = -1; /* current input token number */
int yynerrs = 0;  /* number of errors */
short yyerrflag = 0;  /* error recovery flag */

yyparse() {

	short yys[YYMAXDEPTH];
	short yyj, yym;
	register YYSTYPE *yypvt;
	register short yystate, *yyps, yyn;
	register YYSTYPE *yypv;
	register short *yyxi;

	yystate = 0;
	yychar = -1;
	yynerrs = 0;
	yyerrflag = 0;
	yyps= &yys[-1];
	yypv= &yyv[-1];

 yystack:    /* put a state and value onto the stack */

#ifdef YYDEBUG
	if( yydebug  ) printf( "state %d, char 0%o\n", yystate, yychar );
#endif
		if( ++yyps> &yys[YYMAXDEPTH] ) { yyerror( "yacc stack overflow" ); return(1); }
		*yyps = yystate;
		++yypv;
		*yypv = yyval;

 yynewstate:

	yyn = yypact[yystate];

	if( yyn<= YYFLAG ) goto yydefault; /* simple state */

	if( yychar<0 ) if( (yychar=yylex())<0 ) yychar=0;
	if( (yyn += yychar)<0 || yyn >= YYLAST ) goto yydefault;

	if( yychk[ yyn=yyact[ yyn ] ] == yychar ){ /* valid shift */
		yychar = -1;
		yyval = yylval;
		yystate = yyn;
		if( yyerrflag > 0 ) --yyerrflag;
		goto yystack;
		}

 yydefault:
	/* default state action */

	if( (yyn=yydef[yystate]) == -2 ) {
		if( yychar<0 ) if( (yychar=yylex())<0 ) yychar = 0;
		/* look through exception table */

		for( yyxi=yyexca; (*yyxi!= (-1)) || (yyxi[1]!=yystate) ; yyxi += 2 ) ; /* VOID */

		while( *(yyxi+=2) >= 0 ){
			if( *yyxi == yychar ) break;
			}
		if( (yyn = yyxi[1]) < 0 ) return(0);   /* accept */
		}

	if( yyn == 0 ){ /* error */
		/* error ... attempt to resume parsing */

		switch( yyerrflag ){

		case 0:   /* brand new error */

			yyerror( "syntax error" );
		yyerrlab:
			++yynerrs;

		case 1:
		case 2: /* incompletely recovered error ... try again */

			yyerrflag = 3;

			/* find a state where "error" is a legal shift action */

			while ( yyps >= yys ) {
			   yyn = yypact[*yyps] + YYERRCODE;
			   if( yyn>= 0 && yyn < YYLAST && yychk[yyact[yyn]] == YYERRCODE ){
			      yystate = yyact[yyn];  /* simulate a shift of "error" */
			      goto yystack;
			      }
			   yyn = yypact[*yyps];

			   /* the current yyps has no shift onn "error", pop stack */

#ifdef YYDEBUG
			   if( yydebug ) printf( "error recovery pops state %d, uncovers %d\n", *yyps, yyps[-1] );
#endif
			   --yyps;
			   --yypv;
			   }

			/* there is no state on the stack with an error shift ... abort */

	yyabort:
			return(1);


		case 3:  /* no shift yet; clobber input char */

#ifdef YYDEBUG
			if( yydebug ) printf( "error recovery discards char %d\n", yychar );
#endif

			if( yychar == 0 ) goto yyabort; /* don't discard EOF, quit */
			yychar = -1;
			goto yynewstate;   /* try again in the same state */

			}

		}

	/* reduction by production yyn */

#ifdef YYDEBUG
		if( yydebug ) printf("reduce %d\n",yyn);
#endif
		yyps -= yyr2[yyn];
		yypvt = yypv;
		yypv -= yyr2[yyn];
		yyval = yypv[1];
		yym=yyn;
			/* consult goto table to find next state */
		yyn = yyr1[yyn];
		yyj = yypgo[yyn] + *yyps + 1;
		if( yyj>=YYLAST || yychk[ yystate = yyact[yyj] ] != -yyn ) yystate = yyact[yypgo[yyn]];
		switch(yym){
			
case 1:
# line 88 "config.y"
 { verifysystemspecs(); } break;
case 4:
# line 99 "config.y"
 { newdev(&cur); } break;
case 6:
# line 103 "config.y"
 { do_trace = !do_trace; } break;
case 9:
# line 111 "config.y"
 {
		if (!strcmp(yypvt[-0].str, "vax")) {
			machine = MACHINE_VAX;
			machinename = "vax";
		} else if (!strcmp(yypvt[-0].str, "sun")) {
			machine = MACHINE_SUN;
			machinename = "sun";
#ifdef	CMU
		} else if (!strcmp(yypvt[-0].str, "romp")) {
			machine = MACHINE_ROMP;
			machinename = "romp";
		} else if (!strcmp(yypvt[-0].str, "ca")) {
			machine = MACHINE_ROMP;
			machinename = "ca";
#endif	CMU
		} else
			yyerror("Unknown machine type");
	      } break;
case 10:
# line 130 "config.y"
 {
		struct cputype *cp =
		    (struct cputype *)malloc(sizeof (struct cputype));
		cp->cpu_name = ns(yypvt[-0].str);
		cp->cpu_next = cputype;
		cputype = cp;
		free(temp_id);
	      } break;
case 12:
# line 141 "config.y"
 { ident = ns(yypvt[-0].str); } break;
case 14:
# line 145 "config.y"
 { yyerror("HZ specification obsolete; delete"); } break;
case 15:
# line 147 "config.y"
 { timezone = 60 * yypvt[-0].val; check_tz(); } break;
case 16:
# line 149 "config.y"
 { timezone = 60 * yypvt[-2].val; dst = yypvt[-0].val; check_tz(); } break;
case 17:
# line 151 "config.y"
 { timezone = 60 * yypvt[-1].val; dst = 1; check_tz(); } break;
case 18:
# line 153 "config.y"
 { timezone = yypvt[-0].val; check_tz(); } break;
case 19:
# line 155 "config.y"
 { timezone = yypvt[-2].val; dst = yypvt[-0].val; check_tz(); } break;
case 20:
# line 157 "config.y"
 { timezone = yypvt[-1].val; dst = 1; check_tz(); } break;
case 21:
# line 159 "config.y"
 { timezone = -60 * yypvt[-0].val; check_tz(); } break;
case 22:
# line 161 "config.y"
 { timezone = -60 * yypvt[-2].val; dst = yypvt[-0].val; check_tz(); } break;
case 23:
# line 163 "config.y"
 { timezone = -60 * yypvt[-1].val; dst = 1; check_tz(); } break;
case 24:
# line 165 "config.y"
 { timezone = -yypvt[-0].val; check_tz(); } break;
case 25:
# line 167 "config.y"
 { timezone = -yypvt[-2].val; dst = yypvt[-0].val; check_tz(); } break;
case 26:
# line 169 "config.y"
 { timezone = -yypvt[-1].val; dst = 1; check_tz(); } break;
case 27:
# line 171 "config.y"
 { maxusers = yypvt[-0].val; } break;
case 28:
# line 175 "config.y"
 { checksystemspec(*confp); } break;
case 29:
# line 180 "config.y"
 { mkconf(yypvt[-0].str); } break;
case 39:
# line 206 "config.y"
 { mkswap(*confp, yypvt[-1].file, yypvt[-0].val); } break;
case 40:
# line 211 "config.y"
 {
			struct file_list *fl = newswap();

			if (eq(yypvt[-0].str, "generic"))
				fl->f_fn = yypvt[-0].str;
			else {
				fl->f_swapdev = nametodev(yypvt[-0].str, 0, 'b');
				fl->f_fn = devtoname(fl->f_swapdev);
			}
			yyval.file = fl;
		} break;
case 41:
# line 223 "config.y"
 {
			struct file_list *fl = newswap();

			fl->f_swapdev = yypvt[-0].val;
			fl->f_fn = devtoname(yypvt[-0].val);
			yyval.file = fl;
		} break;
case 42:
# line 234 "config.y"
 {
			struct file_list *fl = *confp;

			if (fl && fl->f_rootdev != NODEV)
				yyerror("extraneous root device specification");
			else
				fl->f_rootdev = yypvt[-0].val;
		} break;
case 43:
# line 246 "config.y"
 { yyval.val = nametodev(yypvt[-0].str, 0, 'a'); } break;
case 45:
# line 252 "config.y"
 {
			struct file_list *fl = *confp;

			if (fl && fl->f_dumpdev != NODEV)
				yyerror("extraneous dump device specification");
			else
				fl->f_dumpdev = yypvt[-0].val;
		} break;
case 46:
# line 265 "config.y"
 { yyval.val = nametodev(yypvt[-0].str, 0, 'b'); } break;
case 48:
# line 271 "config.y"
 {
			struct file_list *fl = *confp;

			if (fl && fl->f_argdev != NODEV)
				yyerror("extraneous arg device specification");
			else
				fl->f_argdev = yypvt[-0].val;
		} break;
case 49:
# line 283 "config.y"
 { yyval.val = nametodev(yypvt[-0].str, 0, 'b'); } break;
case 51:
# line 289 "config.y"
 { yyval.val = makedev(yypvt[-2].val, yypvt[-0].val); } break;
case 54:
# line 299 "config.y"
 { yyval.val = yypvt[-0].val; } break;
case 55:
# line 301 "config.y"
 { yyval.val = 0; } break;
case 56:
# line 306 "config.y"
 { yyval.str = yypvt[-0].str; } break;
case 57:
# line 308 "config.y"
 {
			char buf[80];

			(void) sprintf(buf, "%s%d", yypvt[-1].str, yypvt[-0].val);
			yyval.str = ns(buf); free(yypvt[-1].str);
		} break;
case 58:
# line 315 "config.y"
 {
			char buf[80];

			(void) sprintf(buf, "%s%d%s", yypvt[-2].str, yypvt[-1].val, yypvt[-0].str);
			yyval.str = ns(buf); free(yypvt[-2].str);
		} break;
case 61:
# line 331 "config.y"
 {
		struct opt *op = (struct opt *)malloc(sizeof (struct opt));
		op->op_name = ns(yypvt[-0].str);
		op->op_next = opt;
		op->op_value = 0;
		opt = op;
		free(temp_id);
	      } break;
case 62:
# line 340 "config.y"
 {
		struct opt *op = (struct opt *)malloc(sizeof (struct opt));
		op->op_name = ns(yypvt[-2].str);
		op->op_next = opt;
		op->op_value = ns(yypvt[-0].str);
		opt = op;
		free(temp_id);
		free(val_id);
	      } break;
case 63:
# line 352 "config.y"
 { yyval.str = val_id = ns(yypvt[-0].str); } break;
case 64:
# line 354 "config.y"
 { char nb[16]; yyval.str = val_id = ns(sprintf(nb, "%d", yypvt[-0].val)); } break;
case 65:
# line 359 "config.y"
 { yyval.str = temp_id = ns(yypvt[-0].str); } break;
case 66:
# line 364 "config.y"
 { yyval.str = ns("uba"); } break;
case 67:
# line 366 "config.y"
 { yyval.str = ns("mba"); } break;
case 68:
# line 368 "config.y"
 { yyval.str = ns(yypvt[-0].str); } break;
case 69:
# line 373 "config.y"
 { cur.d_type = DEVICE; } break;
case 70:
# line 375 "config.y"
 { cur.d_type = MASTER; } break;
case 71:
# line 377 "config.y"
 { cur.d_dk = 1; cur.d_type = DEVICE; } break;
case 72:
# line 379 "config.y"
 { cur.d_type = CONTROLLER; } break;
case 73:
# line 381 "config.y"
 {
		cur.d_name = yypvt[-0].str;
		cur.d_type = PSEUDO_DEVICE;
		} break;
case 74:
# line 386 "config.y"
 {
		cur.d_name = yypvt[-1].str;
		cur.d_type = PSEUDO_DEVICE;
		cur.d_slave = yypvt[-0].val;
		} break;
case 75:
# line 394 "config.y"
 {
		cur.d_name = yypvt[-1].str;
		if (eq(yypvt[-1].str, "mba"))
			seen_mba = 1;
		else if (eq(yypvt[-1].str, "uba"))
			seen_uba = 1;
		cur.d_unit = yypvt[-0].val;
		} break;
case 76:
# line 405 "config.y"
 { init_dev(&cur); } break;
case 79:
# line 415 "config.y"
 {
		if (eq(cur.d_name, "mba") || eq(cur.d_name, "uba"))
			yyerror(sprintf(errbuf,
			    "%s must be connected to a nexus", cur.d_name));
		cur.d_conn = connect(yypvt[-1].str, yypvt[-0].val);
		} break;
case 80:
# line 422 "config.y"
 { check_nexus(&cur, yypvt[-0].val); cur.d_conn = TO_NEXUS; } break;
case 83:
# line 432 "config.y"
 { cur.d_addr = yypvt[-0].val; } break;
case 84:
# line 434 "config.y"
 { cur.d_drive = yypvt[-0].val; } break;
case 85:
# line 436 "config.y"
 {
		if (cur.d_conn != 0 && cur.d_conn != TO_NEXUS &&
		    cur.d_conn->d_type == MASTER)
			cur.d_slave = yypvt[-0].val;
		else
			yyerror("can't specify slave--not to master");
		} break;
case 86:
# line 444 "config.y"
 { cur.d_flags = yypvt[-0].val; } break;
case 87:
# line 448 "config.y"
 { cur.d_vec = yypvt[-0].lst; } break;
case 88:
# line 450 "config.y"
 { cur.d_pri = yypvt[-0].val; } break;
case 90:
# line 456 "config.y"
 {
		struct idlst *a = (struct idlst *)malloc(sizeof(struct idlst));
		a->id = yypvt[-0].str; a->id_next = 0; yyval.lst = a;
		} break;
case 91:
# line 460 "config.y"

		{
		struct idlst *a = (struct idlst *)malloc(sizeof(struct idlst));
	        a->id = yypvt[-1].str; a->id_next = yypvt[-0].lst; yyval.lst = a;
		} break;
		}
		goto yystack;  /* stack new state and value */

	}
