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
/*	vmunix device driver for the /dev/lock -- 
 *	an in core device used as a Lock table
 */

/* Only the parameters NLOCKS, PLOCKS, RLOCKS and DLOCKS
 * may be changed by an INGRES installation.  See 
 * 'HOW TO INSTALL CONCURRENCY DEVICE' for details.
 * The file can be printed by "nroff .../doc/other/lockdev.nr"
 */

# define	DLOCKS	10	/* max number of data base locks (limits # of ingreses */
# define	RLOCKS	(2*DLOCKS + 6)
# define	PLOCKS	(RLOCKS + 3)
# define	NLOCKS	(PLOCKS + 1)
# define	KEYSIZE	12

# define	M_EMTY		0
# define	M_SHARE		2
# define	M_EXCL		1

# define	T_CS		0
# define	T_PAGE		1
# define	T_REL		2
# define	T_DB		3

# define	A_RTN		1
# define	A_SLP		2
# define	A_RLS1		3
# define	A_RLSA		4
# define	A_ABT		5

# define	W_ON		1
# define	W_OFF		0

struct Lockreq
			/* Lock Request */
{
	char	lr_act;		/* requested action:
				 *	=1 request lock, err return
				 *	=2 request lock, sleep
				 *	=3 release lock
				 *	=4 release all locks for pid
				 */
	char	lr_type;	/* same as Locktab l_type */
	char	lr_mod;		/* same as Locktab l_mod */
	char	lr_key[KEYSIZE];/* requested key	*/
};

#ifdef	KERNEL

# define	LOCKPRI	(PZERO+1)	/* sleep priority */

/*
 *	data structure for Lock table
 */
struct	Lockform
{
	int	l_pid;
	char	l_wflag;	/* wait flag: = 1 a process is waiting*/
	char	l_type;		/* type of lock:
					= 0 for critical section
					= 1 for page
					= 2 for logical
					= 3 for data base
				*/
	char	l_mod;		/* mod of Lock or lock action requested 
				 *	= 0 slot empty
				 *	= 1 exclusive lock
				 *	= 2 shared lock
				 */
	char 	l_key[KEYSIZE];
} Locktab[NLOCKS];

int	Lockset[] =
			/* array of number of locks which can be
			 * set for each lock.
			 */
{
	NLOCKS,
	PLOCKS,
	RLOCKS,
	DLOCKS
};

#endif	KERNEL
