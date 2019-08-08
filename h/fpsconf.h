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
/*	
 *	(c) 1984 Copyright	Apunix Computer Services
 *
 *	$Log:	/fps/fps/dev/RCS/conf.bsd42,v $
 * Revision 1.4  85/02/09  11:13:56  phb
 * Added provisions for APSIM to load the correct tmrom file when it starts up.
 * 
 * Revision 1.3  84/10/15  23:52:55  phb
 * Updated for specification of all system dependent paramaters in
 * the fpsconf.h file.
 * 
 * Revision 1.2  84/05/29  00:08:14  phb
 * Added TM4K & TM2K defines.
 * 
 * Revision 1.1  83/12/15  21:03:00  phb
 * Initial revision
 * 
 */

/*
 *	Configuration file for Berkeley 4.2 UNIX
 */

/*
 *	All we do here is turn on all the 4.[12] stuff
 */
#define	BERK4
#define	BSD42
#define	FPSRESET

/*
 * We need types.h included when a user program includes fpsreg.h
 */
#define TYPESDOTH

/*====================== Local Changes Begin Here ============================*/

/*
 * Maximum size transfer allowed over the bus.  In the case of a VAX this
 * number is 60K due to limitations of the UBA.  For a PDP-11 or other
 * 16 bit host computers it can probably be 64K.
 */
#define	MINPHYS		(60*1024L)	/* from /sys/bio.c: minphys() */

/*
 *	Possible options here are lines:
 *
 *		#define	FPSPARITY
 *			For sites that have the memory parity option.
 *		#define	NEWFORMAT
 *			For vax sites with the vax interface board installed
 *			in the AP (this allows the support of 32 bit integer
 *			transfers with the proper conversion between VAX
 *			and AP format)
 */
#define NEWFORMAT

/*
 *	Most AP's come preset at unibus interrupt level br4, if yours
 *	is different, you'll need to change the line below.
 */
#define	SPLHI	spl4()

/*
 *	Here one of the following MUST be defined:
 *
 *		#define TM4K
 *			For sites with 4K of TM ROM (all FPS-5000 series and
 *			most late model AP-120B's and FPS-100's).
 *		#define TM2K
 *			For sites with 2K of TM ROM
 *
 *	The failure to define the correct option will cause various math
 *	library routines to behave strangely (e.g., vsmul).
 */
#define TM4K

/*
 *	The following determines the maximum program source size
 *	that may be installed in your machine.  For the FPS-5000
 *	series this number should be either 16K (FPS 5105, 5110, 5410,
 *	5420, and 5430) or 32K (FPS 5205, 5210, 5310, and 5320), however
 *	older AP-120B's and FPS-100's may have to set this number
 *	as low as 4K or 2K due to wrap around of the program source
 *	memory address logic.
 */
#define MAXPS	(32*1024)

/*
 *	Any path dependencies are here
 */

#define	APAL	"/usr/local/bin/apal"
#define	APLINK	"/usr/local/bin/aplink"
#define	CAPVFC	"/usr/local/bin/capvfc"
#define	APVFC	"/usr/local/bin/apvfc"
#define	GPAL	"/usr/local/bin/gpal"
#define	GPLOAD	"/usr/local/bin/gpload"
#define	APLIB	"/usr/local/lib/aplib"
#ifdef TM4K
#define	TMROM	"/usr/local/lib/fpstmrom.4k"
#endif
#ifdef TM2K
#define	TMROM	"/usr/local/lib/fpstmrom.2k"
#endif

/*
 * Where the fps.h file is that has the definition of NFPS (the number
 * of AP's) in it.  FILL IN THE NAME OF YOUR SYSTEM BELOW!!!
 */
#define FPSH	"/sys/SYSTEMNAME/fps.h"

/*
 *	Number of lines actually printed on a line printer page
 *			(for listings)
 */
#ifndef	NLINES
#define	NLINES		56
#endif

/*
 *	APSIM Configuration dependent pramaters
 *
 *		Change according to your system and your AP-120B memory sizes
 *		and options.  These only effect the simulator, and since
 *		all memory is simulated by a temporary file it may not be
 *		advisable to increase MDSIZE above 8K even if you actually
 *		have more memory in your AP, or else the temporary files
 *		will be inordinately large.
 */
#define	FASTMEM		/* REMOVE IF YOU HAVE STANDARD OR SLOW DATA MEMORY */
#define	MDSIZE		8192L	/* default size of main data memory */
#define	PSSIZE		1536L	/* default size of program source memory */
#define	ROMLOW		04000L	/* low table memory ROM address */
#define	ROMHIGH		05000L	/* high table memory ROM address */
#define	RAMLOW		010000L	/* low table memory RAM address */
#define	RAMHIGH		020000L	/* default high table memory RAM address */

/*
 *  We'd really hate to define these in the standard places just for this
 *  special device so they'll go here instead.
 */
#define	SIGFPS	16
#define	EFPS	100
