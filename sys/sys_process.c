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
 * Copyright (c) 1982 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)sys_process.c	6.4 (Berkeley) 6/8/85
 */
#if CMU

/*
 ************************************************************************
 * HISTORY
 * 16-Feb-86  Bill Bolosky (bolosky) at Carnegie-Mellon University
 *	Switched in procxmt from IBM version, with changes under switch
 *	ROMP.  There are an ungodly number of them: this function should
 *	be closely looked at, because it is ridiculous as is...
 *
 ************************************************************************
 */
#endif CMU

#include "../machine/reg.h"
#ifdef	romp
#include "../machine/scr.h"
#else	romp
#include "../machine/psl.h"
#endif	romp
#include "../machine/pte.h"

#include "param.h"
#include "systm.h"
#include "dir.h"
#include "user.h"
#include "proc.h"
#include "inode.h"
#include "text.h"
#include "seg.h"
#include "vm.h"
#include "buf.h"
#include "acct.h"

/*
 * Priority for tracing
 */
#define	IPCPRI	PZERO

/*
 * Tracing variables.
 * Used to pass trace command from
 * parent to child being traced.
 * This data base cannot be
 * shared and is locked
 * per user.
 */
struct {
	int	ip_lock;
	int	ip_req;
	int	*ip_addr;
	int	ip_data;
} ipc;

/*
 * sys-trace system call.
 */
ptrace()
{
	register struct proc *p;
	register struct a {
		int	req;
		int	pid;
		int	*addr;
		int	data;
	} *uap;

	uap = (struct a *)u.u_ap;
	if (uap->req <= 0) {
		u.u_procp->p_flag |= STRC;
		return;
	}
	p = pfind(uap->pid);
	if (p == 0 || p->p_stat != SSTOP || p->p_ppid != u.u_procp->p_pid ||
	    !(p->p_flag & STRC)) {
		u.u_error = ESRCH;
		return;
	}
	while (ipc.ip_lock)
		sleep((caddr_t)&ipc, IPCPRI);
	ipc.ip_lock = p->p_pid;
	ipc.ip_data = uap->data;
	ipc.ip_addr = uap->addr;
	ipc.ip_req = uap->req;
	p->p_flag &= ~SWTED;
	while (ipc.ip_req > 0) {
		if (p->p_stat==SSTOP)
			setrun(p);
		sleep((caddr_t)&ipc, IPCPRI);
	}
	u.u_r.r_val1 = ipc.ip_data;
	if (ipc.ip_req < 0)
		u.u_error = EIO;
	ipc.ip_lock = 0;
	wakeup((caddr_t)&ipc);
}

#ifdef vax
#define	NIPCREG 16
int ipcreg[NIPCREG] =
	{R0,R1,R2,R3,R4,R5,R6,R7,R8,R9,R10,R11,AP,FP,SP,PC};
#endif
#ifdef	romp
#define NIPCREG 18	 /* allow modification of only these u area variables */
int ipcreg[NIPCREG] =
	{R0,R1,R2,R3,R4,R5,R6,R7,R8,R9,R10,R11,R12,R13,R14,R15,IAR,MQ};
#endif	romp

#ifndef	romp
#define	PHYSOFF(p, o) \
	((physadr)(p)+((o)/sizeof(((physadr)0)->r[0])))
#else	romp
#define UAREA_ADDR(o) ((int *)(0x20000000 - UPAGES*NBPG + (int)(o)))
#endif	romp

#ifdef	romp
extern char fubyte(), fuibyte();
extern int fuword(), fuiword();
extern int subyte(), suibyte(), suword(), suiword();
/* Presumably these are here due to differences in locore. */
#endif	romp
/*
 * Code that the child process
 * executes to implement the command
 * of the parent process in tracing.
 */
procxmt()
{
	register int i;
	register *p;
	register struct text *xp;

	if (ipc.ip_lock != u.u_procp->p_pid)
		return (0);
	u.u_procp->p_slptime = 0;
	i = ipc.ip_req;
	ipc.ip_req = 0;
	switch (i) {

	/* read user I */
	case 1:
		if (!useracc((caddr_t)ipc.ip_addr, 4, B_READ))
			goto error;
#ifndef	romp
		ipc.ip_data = fuiword((caddr_t)ipc.ip_addr);
#else	romp
		ipc.ip_data = getubits((caddr_t)ipc.ip_addr, fuiword, fuibyte);
#endif	romp
		break;

	/* read user D */
	case 2:
		if (!useracc((caddr_t)ipc.ip_addr, 4, B_READ))
			goto error;
#ifndef	romp
		ipc.ip_data = fuword((caddr_t)ipc.ip_addr);
#else	romp
		ipc.ip_data = getubits((caddr_t)ipc.ip_addr, fuword, fubyte);
#endif	romp
		break;

	/* read u */
	case 3:
		i = (int)ipc.ip_addr;
#ifdef	romp
		if (i<0 || i >= ctob(UPAGES) || i&(sizeof(int)-1))
#else	romp
		if (i<0 || i >= ctob(UPAGES))
#endif	romp
			goto error;
#ifndef	romp
		ipc.ip_data = *(int *)PHYSOFF(&u, i);
#else	romp
		ipc.ip_data = *UAREA_ADDR(i);
#endif	romp
		break;

	/* write user I */
	/* Must set up to allow writing */
	case 4:
		/*
		 * If text, must assure exclusive use
		 */
		if (xp = u.u_procp->p_textp) {
			if (xp->x_count!=1 || xp->x_iptr->i_mode&ISVTX)
				goto error;
			xp->x_iptr->i_flag |= IXMOD;	/* XXX */
		}
#ifndef	romp
		i = -1;
		if ((i = suiword((caddr_t)ipc.ip_addr, ipc.ip_data)) < 0) {
			if (chgprot((caddr_t)ipc.ip_addr, RW) &&
			    chgprot((caddr_t)ipc.ip_addr+(sizeof(int)-1), RW))
				i = suiword((caddr_t)ipc.ip_addr, ipc.ip_data);
			(void) chgprot((caddr_t)ipc.ip_addr, RO);
			(void) chgprot((caddr_t)ipc.ip_addr+(sizeof(int)-1), RO);
#else	romp
		/* Must set up to allow	writing */
		if ((i = setubits((caddr_t)ipc.ip_addr, ipc.ip_data, suiword, suibyte ))) {
			if( chgprot( (caddr_t)ipc.ip_addr, RW ) && chgprot( (caddr_t)ipc.ip_addr+3, RW ))
				i = setubits((caddr_t)ipc.ip_addr, ipc.ip_data, suiword, suibyte);
			(void) chgprot((caddr_t)ipc.ip_addr, RO);
			(void) chgprot((caddr_t)ipc.ip_addr+3, RO);
#endif	romp
		}
		if (i < 0)
			goto error;
		if (xp)
			xp->x_flag |= XWRIT;
		break;

#ifndef	romp
	/* write user D */
	case 5:
		if (suword((caddr_t)ipc.ip_addr, 0) < 0)
#else	romp
	case 5: /* write user D */
		if (setubits((caddr_t)ipc.ip_addr, 0, suword, subyte))
#endif	romp
			goto error;
#ifndef	romp
		(void) suword((caddr_t)ipc.ip_addr, ipc.ip_data);
#else	romp
		(void) setubits((caddr_t)ipc.ip_addr, ipc.ip_data, suword, subyte);
#endif	romp
		break;

#ifndef	romp
	/* write u */
	case 6:
		i = (int)ipc.ip_addr;
		p = (int *)PHYSOFF(&u, i);
#else	romp
	case 6: /* write u */
		i = (int)(caddr_t)ipc.ip_addr;
		p = UAREA_ADDR(i);
#endif	romp
		for (i=0; i<NIPCREG; i++)
			if (p == &u.u_ar0[ipcreg[i]])
				goto ok;
#ifndef	romp
		if (p == &u.u_ar0[PS]) {
			ipc.ip_data |= PSL_USERSET;
			ipc.ip_data &=  ~PSL_USERCLR;
#else	romp
		if (p == &u.u_ar0[ICSCS]) {
			ipc.ip_data |= ICSCS_USERSET;
			ipc.ip_data &= ~ICSCS_USERCLR;
#endif	romp
			goto ok;
		}
		goto error;

	ok:
		*p = ipc.ip_data;
		break;

#ifndef	romp
	/* set signal and continue */
	/* one version causes a trace-trap */
	case 9:
	case 7:
#else	romp
	case 7: /* set signal and run */
	case 9: /* set signal and single step */
#endif	romp
		if ((int)ipc.ip_addr != 1)
#ifndef	romp
			u.u_ar0[PC] = (int)ipc.ip_addr;
#else	romp
			u.u_ar0[IAR] = (int)ipc.ip_addr;
#endif	romp
		if ((unsigned)ipc.ip_data > NSIG)
			goto error;
		u.u_procp->p_cursig = ipc.ip_data;	/* see issig */
		if (i == 9)
#ifndef	romp
			u.u_ar0[PS] |= PSL_T;
#else	romp
			u.u_ar0[ICSCS] |= ICSCS_INSTSTEP;  /* see locore */
#endif	romp
		wakeup((caddr_t)&ipc);
		return (1);

	/* force exit */
	case 8:
		wakeup((caddr_t)&ipc);
		exit(u.u_procp->p_cursig);

	default:
	error:
		ipc.ip_req = -1;
	}
	wakeup((caddr_t)&ipc);
	return (0);
}
#ifdef	romp

getubits(cp, wsubr, bsubr )
	register char *cp;
	register int (*wsubr)();
	register char (*bsubr)();
{
	register i, fourbytes;

	if ((int)cp & 3 ) {
		for (i=0; i<4; ++i) {
			fourbytes <<= 8;
			fourbytes |= (*bsubr)(cp+i);
		}
		return (fourbytes);
	}
	return ((*wsubr)(cp));
}

setubits(cp, bits, wsubr, bsubr)
	register char *cp;
	register int bits;
	register int (*wsubr)();
	register int (*bsubr)();
{
	register i, r;

	if ((int)cp & 3) {
		for (i=3; i>=0; --i) {
			if (r = (*bsubr)(cp+i, bits & 0xFF))
				return (r);
			bits >>= 8;
		}
		return (r);
	}
	return((*wsubr)(cp, bits));
}
#endif	romp
