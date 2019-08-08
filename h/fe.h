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
/*	fe.h	CMU	1/18/80	*/

/*
 *  Front End external definitions (in separate file for systat)
 *
 **********************************************************************
 * HISTORY
 * 19-Mar-86  Mike Accetta (mja) at Carnegie-Mellon University
 *	Fixed to define FEIOCSETS/FEIOCGETS in terms of new static
 *	MAXNFE symbol so that they may be used in applications where
 *	NFE will not be defined.
 *	
 * 06-Aug-85  Mike Accetta (mja) at Carnegie-Mellon University
 *	Upgraded from 4.1BSD.
 *	[V1(1)]
 *
 * 13-Dec-84  Mike Accetta (mja) at Carnegie-Mellon University
 *	Added new FEBRKINH flag, new FECSETPOS and FECSETMASK
 *	definitions and changed FEIOCCDET call to new FEIOCCSET call
 *	which can handle multiple special status flags (V3.07l).
 *
 * 17-May-83  Mike Accetta (mja) at Carnegie-Mellon University
 *	Added FEDETHUP and FEMODES definitions and FEIOCCDET call to
 *	change detach processing (V3.06j).
 *
 * 07-Jul-82  Mike Accetta (mja) at Carnegie-Mellon University
 *	Added FEIOCLINEN definition (V3.05c).
 *
 * 23-Jan-82  Mike Accetta (mja) at Carnegie-Mellon University
 *	Added FEDETACH, FEACTIVE and felettertty() definitions;  added
 *	inactive field to fe11 structure for recording time since last
 *	SYNC request was received;  added FEIOCCHECK and FEIOCATTACH
 *	ioctl definitions (V3.03e).
 *
 * 21-Oct-81  Mike Accetta (mja) at Carnegie-Mellon University
 *	Added definition for BREAK line message (V3.02e).
 *
 * 20-Aug-81  Mike Accetta (mja) at Carnegie-Mellon University
 *	Removed obsolete SNDCHR bit definition and added LOGGED
 *	bit definition;  added new "oldintro" field to fet structure
 *	for preserving intro message context during inserts;  added
 *	FEIOCCLOG and FEIOCNFE definitions (V3.00).
 *
 * 23-Jul-81  Mike Accetta (mja) at Carnegie-Mellon University
 *	Added fettyletter() macro (V2.00r).
 *
 * 13-May-81  Mike Accetta (mja) at Carnegie-Mellon University
 *	Added FEIOCDISC definition (V2.00j).
 *
 * 12-Nov-80  Mike Accetta (mja) at Carnegie-Mellon University
 *	Added FEIOCSETS and FEIOCGETS definitions (V2.00).
 *
 * 27-Jun-80  Mike Accetta (mja) at Carnegie-Mellon University
 *	Changed to support multiple Front End connections (V1.08a).
 *
 * 12-Feb-80  Mike Accetta (mja) at Carnegie-Mellon University
 *	Changed the fet11 structure so that ttyb field is only a
 *	pointer into the fe_tty[] array rather than being the actual
 *	structure itself (V1.03b).
 *
 * 24-Jan-80  Mike Accetta (mja) at Carnegie-Mellon University
 *	Changed MAXLINE from 128 to 192, FETTYPE from 0100 to 040,
 *	SYNC from 0200 to 0317 and added LINEMSG definition all
 *	for new FE protocol (V1.01a).
 *
 * 18-Jan-80  Mike Accetta (mja) at Carnegie-Mellon University
 *	Created (V1.00).
 *
 **********************************************************************
 */

#define	MAXNFE	3		/* maximum umber of FE's */
#ifdef	KERNEL
#if	NFE > MAXNFE
OOPS - Increase MAXNFE (and relink clients of FEIOC[SG]ETS)
#endif	NFE > MAXNFE
#endif	KERNEL
#define	NLINE	26		/* number of UNIX lines on FE */
#define	IDLESECS 5		/* Frequency of idle timeout check */
#define MAXIDLE	(120/IDLESECS)	/* logged out inactivity disconnect time */
#define	INACTIVE (120/IDLESECS)	/* FE inactivity disconnect time */
#define FEQUOTA	32		/* quota increment */
#define	FEQLOW	128		/* quota low */
#define MAXLINE	192		/* maximum number of Front End terminals */
#define	LINEOFS	0400		/* offset of line numbers for FE #2 */
#define	FEIBUF	128		/* size of input buffer (must be power of 2) */
#define	FEIMASK	(FEIBUF-1)	/* input buffer mask for wrap around */

#define	fettyletter(c)	((((c) < 26)? 'a' : 'A'-26) + (c))
#define	felettertty(c)	(((c) >= 'a' && (c) <= 'z')?((c)-'a'):\
			(((c) >= 'A' && (c) <= 'Z')?((c)-'A'+26):(-1)))

/* Front End messages */
#define SYSMES	00
#define BREAK	00
#define QINC	01
#define CONNECT	02
#define DISCON	03
#define CACK	04
#define CONREF	05
#define HALT	06
#define RESUME	07
#define CLEAR	010
#define REFUSE	012
#define ACCEPT	013
#define FORCE	014
#define SYNCREQ	020
#define	FETTYPE	040	/* terminal type base */
#define	LINEMSG	0300	/* line message base */
#define SYNC	0317

/* bits in flags */
#define SNDSYNC	 01		/* Send SYNC */
#define SNDISC	 02		/* send Disconnect */
#define SNDCACK	 04		/* send Connect-Acknowledgement */
#define SNDINC	 010		/* send quota increment */
#define SNDFORC  020		/* send forced command */
#define SNDCON	 040		/* send Connect */
#define SNDCONR	 0100		/* Send Connection-Refused */
#define SNDCLR	 0200		/* send clear output */
#define SNDHALT	 0400		/* send halt output */
#define SNDRSM	 01000		/* send resume output */
#define LOGGED	 02000		/* line is logged in */
#define FERBUSY	 04000		/* receive routine (scanner) busy */
#define FEXBUSY	 010000		/* transmit routine busy */
#define	FEXNEST	 020000		/* transmit routine nested call */
#define FETHERE	 040000		/* Front End linkage established */
#define	REFUSEC	 0100000	/* Refuse further line connections */
#define ALLOWC	 0200000	/* flag to allow system connects */
#define GOTSYNC	 0400000	/* set when SYNC character received */
#define	FEINIT	 01000000	/* performed initialization sequence */
#define	FEACTIVE 02000000	/* have seen SYNC request recently */
/*
 *  The following flags set by the FEIOCCSET call must be defined consecutively
 *  and correspond to the  bit supplied in ioctl() argument but shifted left by
 *  the appropriate offset.
 */
#define FEDETHUP 04000000	/* send hangup signal on detach */
#define FEBRKINH 010000000	/* inhibit disconnect on BREAK message */

#define	FECSETPOS	20	/* bit shift to first bit in mask */
#define	FECSETMASK	(FEDETHUP|FEBRKINH)

#define	FEMODES	(LOGGED|FEDETHUP|FEBRKINH)	/* permanent mode bits */

#define	SNDSYS	(SNDCONR|SNDCON|SNDFORC|SNDINC|SNDCACK|SNDISC|SNDSYNC)
#define	SNDLINE	(SNDRSM|SNDHALT|SNDCLR|SNDCONR|SNDINC|SNDCACK|SNDISC)

struct fes11 {
    int	    flags;		/* miscellaneous flags */
    short   iptr;		/* index of place in buffer for next char */
    short   optr;		/* index of place in buffer to take char */
    short   iquo;		/* quota for characters from front end */
    short   oquo;		/* quota for characters to front end */
    u_short inactive;   	/* time since last SYNC request */
    char    ochar;		/* next character to be sent */
    char    linpos;		/* front end polling position for output */
    char    force1;		/* message to be forced through Front End */
    char    force2;
    char    chrbuf[FEIBUF];	/* input buffer */
    char    lcbtab[MAXLINE];	/* the UNIX line numbers for each fe line */
};
#define	 FENOLINE (-1)	/* unconnected line in lcbtab */

struct fetab
{
    struct tty *fetp;		/* tty structure of this FE dz line */
    int (*xstart)();		/* start routine */
    int fedzl;			/* the dz line number */
    struct fes11 fe11;		/* flag bits for this FE */
};

struct fet {
    struct tty *ttyb;	  /* tty buffer */
    struct fetab *fetabp; /* front end table pointer */
    int linf;		  /* line flags */
    int line;		  /* front end line number */
    int ttype;		  /* terminal type from connect */
    short linqo;	  /* the output line quota */
    short linqi;	  /* the input line quota */
    char *intro;	  /* position in welcome message */
    char *oldintro;	  /* position in interrupted welcome message */
    unsigned idle;	  /* inactivity time / IDLESECS */
};
#define	FEFREE	 (-1)	  /* free UNIX line */
#define	FEDETACH (0)	  /* detached UNIX line */

/*  Layout of special read/write buffer  */
struct fespbuf
{
    int splinf;			/* line flags */
    int spline;			/* FE line number */
};
#define	SPWMASK	(SNDISC)

#define	FEIOCSETS	_IOW (F, 255, struct feiocbuf[MAXNFE]) /* set FE status */
#define	FEIOCGETS	_IOR (F, 254, struct feiocbuf[MAXNFE]) /* get FE status */
#define	FEIOCGETT	_IOR (F, 253, int)	/* get FE terminal type */
#define	FEIOCDISC	_IO  (F, 251)		/* disconnect line */
#define	FEIOCCLOG	_IOW (F, 250, int)	/* change logged-in status */
#define	FEIOCNFE	_IOR (F, 249, int)	/* return NFE */
#define	FEIOCCHECK	_IOW (F, 248, char[3])	/* check detached status of line */
#define	FEIOCATTACH	_IOW (F, 247, char[3])	/* attach to line */
#define	FEIOCLINEN	_IOR (F, 246, int)	/* return line number */
#define	FEIOCCSET	_IOW (F, 245, int)	/* change special status flags */

/*  Layout of special I/O control buffer  */
struct feiocbuf
{
    int spflags;
    char spfrc1;
    char spfrc2;
    unsigned short spnline;
};
#define	IOCMASK	(SNDISC|REFUSEC|SNDFORC|ALLOWC)
