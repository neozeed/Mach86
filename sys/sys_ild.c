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
 **********************************************************************
 * HISTORY
 * 06-Aug-85  Mike Accetta (mja) at Carnegie-Mellon University
 *	Upgraded from 4.1BSD.
 *	[V1(1)]
 *
 **********************************************************************
 */

/*
 *	/dev/lock header
 */
/*	
 *	ilwrite() : write driver
 *		1. copy Lock request info to lockbuf
 *		2. follow action in l_act
 *		3. Error return conditions
 *			-1: lockrequest fails(only on act=1)
 *			-2: attempt to release a lock not set
 *			    by calling program
 *			-3: illegal action requested
 */

#include "ild.h"
#if NILD > 0

#include "../h/param.h"
#include "../h/dir.h"
#include "../h/user.h"
#include "../h/proc.h"
#include "../h/buf.h"
#include "../h/ild.h"
#include "../h/uio.h"

#define	ilwrite	ildwrite
#define	ilrma	ildrma
#define	ilioctl	ildioctl

#ifndef	TRUE
#define	TRUE	1
#define	FALSE	0
#endif	TRUE

static int ildebug = FALSE;

/*ARGSUSED*/
ilwrite(dev, uio)
	dev_t dev;
	struct uio *uio;
{
	struct Lockreq	lockbuf;
	register int i;
	register int blockflag;
	register int pid;
	int error = 0;

	if (uio->uio_resid != sizeof(lockbuf)) {
		if (ildebug)
			uprintf("ildr: count = %d, expect %d\n", uio->uio_resid, sizeof(lockbuf));
		return(EINVAL);
	}
/*
 *		copy lockrequest info to lockbuf
 */
	error = uiomove((caddr_t)&lockbuf, sizeof(lockbuf), UIO_WRITE, uio);
	if (error)
		return(error);
	pid = u.u_procp->p_pid;
	if (( lockbuf.lr_act < A_RLS1)
	&& ((lockbuf.lr_type < T_CS) || (lockbuf.lr_type > T_DB )
	   || (lockbuf.lr_mod < M_EXCL) || (lockbuf.lr_mod > M_SHARE )))
	{
		return(-5);
	}
/*
 *		follow action from lock request
 */
	if (ildebug)
		uprintf("ildr: act %d, type %d, mode %d, pid %d\n",
			lockbuf.lr_act, lockbuf.lr_type, lockbuf.lr_mod, pid);
	switch(lockbuf.lr_act)
	{
	  case A_RTN:
					/*
					 * attempt to set lock.
					 * error return if failure.
					 */
		for ( i = 0; i <= lockbuf.lr_type; i++) {
			if (Lockset[i] == 0) {
				if (ildebug)
					uprintf("ildr: lock %d not available\n", i);
				return(-1);
			}
		}
		if (ilunique(&lockbuf) >= 0) {
			return(-1);
		}
		(void)ilenter(&lockbuf,pid);
		break;

	  case A_SLP:
				/* attempt to set lock.
				 * sleep on blocking address if failure.
				 */
		do
		{
			do
			{
				blockflag = TRUE;
				for ( i = 0; i <= lockbuf.lr_type; i++)
					if (Lockset[i] == 0)
					{
						if (ildebug)
							uprintf("ildr: lock %d not available\n", i);
						sleep((caddr_t)&Lockset[i],LOCKPRI);
						blockflag = FALSE;
					}
			}
			while (!blockflag);
			if (( i = ilunique(&lockbuf)) >= 0 )
			{
				blockflag = FALSE;
				Locktab[i].l_wflag = W_ON;
				sleep((caddr_t)&Locktab[i],LOCKPRI);
			}
		}
		while (!blockflag);
		(void)ilenter(&lockbuf,pid);
		break;

	  case A_RLS1:
				/* remove 1 lock */
		if ((i = ilfind(&lockbuf,pid)) >= 0)
		{
			ilrm(i,pid);
		}
		else
			error = -2;
		break;

	  case A_RLSA:
				/* remove all locks for this process id*/
		ilrma(pid);
		break;

	  case A_ABT:		/* remove all locks */
		ilclose();
		break;

	  default :
		error = -3;
	}
	return(error);
}
/*
 *	ilunique- check for match on key
 *	
 *	return index of Locktab if match found
 *	else return -1
 */
static
ilunique(ll)
register struct Lockreq *ll;
{
	register int	k;
	register struct Lockform	*p;

	for (k = 0; k < NLOCKS; k++)
	{
		p = &Locktab[k];
		if ((p->l_mod != M_EMTY)
		&& (ilcomp(p->l_key,ll->lr_key) == 0)
		&& (p->l_type == ll->lr_type)
		&& ( (p->l_mod == M_EXCL) || (ll->lr_mod == M_EXCL)) ) {
			if (ildebug) {
				register int i;

				uprintf("ildr: lock ");
				for (i = 0; i < KEYSIZE; i++)
					uprintf("%c", ll->lr_key[i]);
				uprintf(" busy\n");
			}
			return(k);
		}
	}
	return(-1);
}

static
ilfind(ll,pid)
register struct Lockreq *ll;
{
	register int	k;
	register struct Lockform	*p;

	for (k = 0; k < NLOCKS; k++)
	{
		p = &Locktab[k];
		if ((p->l_mod != M_EMTY)
		&& (ilcomp(p->l_key,ll->lr_key) == 0)
		&& (p->l_type == ll->lr_type)
		&& (p->l_pid == pid))
			return(k);
	}
	return(-1);
}

/*
 *	remove the lth Lock
 *		if the correct user is requesting the move.
 */
static
ilrm(l,llpid)
int l;
int	llpid;
{
	register struct Lockform *a;
	register	k;


	a = &Locktab[l];
	if (a->l_pid == llpid && a->l_mod != M_EMTY)
	{
		a->l_mod = M_EMTY;
		a->l_pid = 0;
		if (a->l_wflag == W_ON)
		{
			a->l_wflag = W_OFF;
			wakeup((caddr_t)&Locktab[l]);
		}
		for (k = 0; k <= a->l_type; k++)
		{
			Lockset[k]++;
			if (Lockset[k] == 1)
				wakeup((caddr_t)&Lockset[k]);
		}
	}
}

/*
 *	ilrma releases all locks for a given process id(pd)
 *	-- called from sys1.c$exit() code.
 */
ilrma(pd)
int pd;
{
	register int	i;

	for ( i = 0; i < NLOCKS; i++ )
		ilrm(i,pd);
}

/*
 *	enter Lockbuf in locktable
 *	return position in Locktable
 *	error return of -1
 */
static
ilenter(ll,pid)
register struct Lockreq *ll;
int pid;
{
	int	k,l;
	register char	*f,*t;
	register struct Lockform	*p;

	for (k = 0; k < NLOCKS; k++)
	{
		p = &Locktab[k];
		if (p->l_mod == M_EMTY)
		{
			p->l_pid = pid;
			p->l_type = ll->lr_type;
			p->l_mod = ll->lr_mod;
			f = ll->lr_key;
			t = p->l_key;
			for (l = 0; l < KEYSIZE; l++)
				*t++ = *f++;
			for (l = 0; l <= ll->lr_type; l++)
				Lockset[l]--;
			return(k);
		}
	}
	return (-1);
}

/*
 *	ilcomp- string compare
 *	  	returns 0 if strings match
 *		returns -1 otherwise
 */
static
ilcomp(s1,s2)
register char *s1,*s2;
{
	register int	k;

	for (k = 0; k < KEYSIZE; k++)
		if ( *s1++ != *s2++)
			return (-1);
	return (0);
}

/*
 *	ilclose- releases all locks
 */
static
ilclose()
{
	register int	k;
	register caddr_t c;

	for (k = 0; k < NLOCKS; k++)
		wakeup( (caddr_t)&Locktab[k] );
	for (k = 0; k < 4; k++)
		wakeup( (caddr_t)&Lockset[k]);
	for (c = (caddr_t)&Locktab[0].l_pid; c < (caddr_t)&Locktab[NLOCKS];)
		*c++ = 0;
	Lockset[0] = NLOCKS;
	Lockset[1] = PLOCKS;
	Lockset[2] = RLOCKS;
	Lockset[3] = DLOCKS;
}

/*
 * I/O control for locking device
 * -- turns debugging on and off
 */
/*ARGSUSED*/
ilioctl(dev, cmd, addr, flag)
dev_t dev;
int cmd;
caddr_t addr;
int flag;
{
	ildebug = cmd;
}
#endif
