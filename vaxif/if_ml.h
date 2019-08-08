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
/*CMU:	if_ml.h	7.1	5/29/84	*/

/*
 *	File:	if_ml.h
 */

/*
 **************************************************************************
 * HISTORY
 * 11-Oct-85  Robert V Baron (rvb) at Carnegie-Mellon University
 *	move the extern for mlbuf here
 *
 **************************************************************************
 */

#define WIRESIZE 1500

struct ml_header {
	u_char	ml_shost;	/* source host */
	u_char	ml_dhost;	/* destination host */
	u_short	ml_type;	/* packet type */
};

struct mlbuf {
	int	ml_upmask;
	int	ml_packetsize;
	int	ml_semaphore;
	int	ml_wait;
	char	ml_wire[WIRESIZE];
	int	fill;		/* to help with quad-word alignment */
};

#define MLPTSIZE btoc(sizeof(struct mlbuf));

#ifdef KERNEL
extern struct mlbuf *mlbuf;
#endif

#define MLTYPE_IP	0x0201		/* IP protocol */
