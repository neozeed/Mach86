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

typedef union  {
	char	*str;
	int	val;
	struct	file_list *file;
	struct	idlst *lst;
} YYSTYPE;
extern YYSTYPE yylval;
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
