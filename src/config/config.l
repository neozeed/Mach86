%{

/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)config.l	5.1 (Berkeley) 5/8/85
 */

#include <ctype.h>
#include "y.tab.h"
#include "config.h"

#define tprintf if (do_trace) printf

/*
 * Key word table
 */

struct kt {
	char *kt_name;
	int kt_val;
} key_words[] = {
	{ "and",	AND },
	{ "args",	ARGS },
	{ "at",		AT },
	{ "config",	CONFIG },
	{ "controller",	CONTROLLER },
	{ "cpu",	CPU },
	{ "csr",	CSR },
	{ "device",	DEVICE },
	{ "disk",	DISK },
	{ "drive",	DRIVE },
	{ "dst",	DST },
	{ "dumps",	DUMPS },
	{ "flags",	FLAGS },
	{ "hz",		HZ },
	{ "ident",	IDENT },
	{ "machine",	MACHINE },
	{ "major",	MAJOR },
	{ "master",	MASTER },
	{ "maxusers",	MAXUSERS },
	{ "mba",	MBA },
	{ "minor",	MINOR },
	{ "nexus",	NEXUS },
	{ "on",		ON },
	{ "options",	OPTIONS },
	{ "priority",	PRIORITY },
	{ "pseudo-device",PSEUDO_DEVICE },
	{ "root",	ROOT },
	{ "size",	SIZE },
	{ "slave",	SLAVE },
	{ "swap",	SWAP },
	{ "tape",	DEVICE },
	{ "timezone",	TIMEZONE },
	{ "trace",	TRACE },
	{ "uba",	UBA },
	{ "vector",	VECTOR },
	{ 0, 0 },
};
%}
WORD	[A-Za-z_][-A-Za-z_]*
%%
{WORD}		{
			int i;

			if ((i = kw_lookup(yytext)) == -1)
			{
				yylval.str = yytext;
				tprintf("id(%s) ", yytext);
				return ID;
			}
			tprintf("(%s) ", yytext);
			return i;
		}
\"[^"]+\"	{
			yytext[strlen(yytext)-1] = '\0';
			yylval.str = yytext + 1;
			return ID;
		}
0[0-7]*		{
			yylval.val = octal(yytext);
			tprintf("#O:%o ", yylval.val);
			return NUMBER;
		}
0x[0-9a-fA-F]+	{
			yylval.val = hex(yytext);
			tprintf("#X:%x ", yylval.val);
			return NUMBER;
		}
[1-9][0-9]*	{
			yylval.val = atoi(yytext);
			tprintf("#D:%d ", yylval.val);
			return NUMBER;
		}
[0-9]"."[0-9]*	{
			double atof();
			yylval.val = (int) (60 * atof(yytext) + 0.5);
			return FPNUMBER;
		}
"-"		{
			return MINUS;
		}
"?"		{
			yylval.val = -1;
			tprintf("? ");
			return NUMBER;
		}
\n/[ \t]	{
			yyline++;
			tprintf("\n... ");
		}
\n		{
			yyline++;
			tprintf("\n");
			return SEMICOLON;
		}
#.*		{	/* Ignored (comment) */;	}
[ \t]*		{	/* Ignored (white space) */;	}
";"		{	return SEMICOLON;		}
","		{	return COMMA;			}
"="		{	return EQUALS;			}
"@"		{	return AT;			}
.		{	return yytext[0];		}

%%
/*
 * kw_lookup
 *	Look up a string in the keyword table.  Returns a -1 if the
 *	string is not a keyword otherwise it returns the keyword number
 */

kw_lookup(word)
register char *word;
{
	register struct kt *kp;

	for (kp = key_words; kp->kt_name != 0; kp++)
		if (eq(word, kp->kt_name))
			return kp->kt_val;
	return -1;
}

/*
 * Number conversion routines
 */

octal(str)
char *str;
{
	int num;

	(void) sscanf(str, "%o", &num);
	return num;
}

hex(str)
char *str;
{
	int num;

	(void) sscanf(str+2, "%x", &num);
	return num;
}
