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
 *	@(#)kern_xxx.c	6.4 (Berkeley) 6/8/85
 */
#if	CMU
/*
 **********************************************************************
 * HISTORY
 *  5-Apr-86  Avadis Tevanian (avie) at Carnegie-Mellon University
 *	Added a call to shutdown the vm_object system at reboot time.
 *	This will cause the paging areas to be properly deallocated.
 *
 * 22-Mar-86  Avadis Tevanian (avie) at Carnegie-Mellon University
 *	Merged VM and Romp versions.  Made splhigh changes conditional
 *	on CS_GENERIC.
 *
 *  6-Mar-86  Bill Bolosky (bolosky) at Carnegie-Mellon University
 *	Changed spl7() to splhigh().
 *
 * 14-Feb-86  Bill Bolosky (bolosky) at Carnegie-Mellon University
 *	Switched off imports of psl.h and reg.h when compiling on
 *	Sailboat under switch ROMP.
 *
 * 25-Jan-86  Avadis Tevanian (avie) at Carnegie-Mellon University
 *	Upgraded to 4.3.
 *
 * 16-Nov-85  Avadis Tevanian (avie) at Carnegie-Mellon University
 *	CS_BUGFIX:  Fixed off by one errors in checking for validity of
 *	signal numbers.
 *
 * 04-Oct-85  Mike Accetta (mja) at Carnegie-Mellon University
 *	CS_COMPAT:  supplied missing compatibility code for ovlimit().
 *	[V1(1)]
 *
 * 25-May-85  Mike Accetta (mja) at Carnegie-Mellon University
 *	CS_COMPAT:  changed ossig() to include job control mechanism
 *	compatibility as well.
 *	[V1(1)]
 *
 **********************************************************************
 */
 
#include "cs_bugfix.h"
#include "cs_compat.h"
#include "cs_generic.h"
#include "mach_vm.h"
#endif	CMU

#include "param.h"
#include "systm.h"
#include "dir.h"
#include "user.h"
#include "kernel.h"
#include "proc.h"
#include "reboot.h"
#if	CS_COMPAT
#include "vm.h"
#endif	CS_COMPAT

gethostid()
{

	u.u_r.r_val1 = hostid;
}

sethostid()
{
	struct a {
		int	hostid;
	} *uap = (struct a *)u.u_ap;

	if (suser())
		hostid = uap->hostid;
}

gethostname()
{
	register struct a {
		char	*hostname;
		int	len;
	} *uap = (struct a *)u.u_ap;
	register u_int len;

	len = uap->len;
	if (len > hostnamelen + 1)
		len = hostnamelen + 1;
	u.u_error = copyout((caddr_t)hostname, (caddr_t)uap->hostname, len);
}

sethostname()
{
	register struct a {
		char	*hostname;
		u_int	len;
	} *uap = (struct a *)u.u_ap;

	if (!suser())
		return;
	if (uap->len > sizeof (hostname) - 1) {
		u.u_error = EINVAL;
		return;
	}
	hostnamelen = uap->len;
	u.u_error = copyin((caddr_t)uap->hostname, hostname, uap->len);
	hostname[hostnamelen] = 0;
}

reboot()
{
	register struct a {
		int	opt;
	};

#if	MACH_VM
	if (suser()) {
		vm_object_shutdown();
		boot(RB_BOOT, ((struct a *)u.u_ap)->opt);
	}
#else	MACH_VM
	if (suser())
		boot(RB_BOOT, ((struct a *)u.u_ap)->opt);
#endif	MACH_VM
}

#ifdef COMPAT
#include "../h/quota.h"

osetuid()
{
	register uid;
	register struct a {
		int	uid;
	} *uap;

	uap = (struct a *)u.u_ap;
	uid = uap->uid;
	if (u.u_ruid == uid || u.u_uid == uid || suser()) {
#ifdef QUOTA
		if (u.u_quota->q_uid != uid) {
			qclean();
			qstart(getquota(uid, 0, 0));
		}
#endif
		u.u_uid = uid;
		u.u_procp->p_uid = uid;
		u.u_ruid = uid;
	}
}

osetgid()
{
	register gid;
	register struct a {
		int	gid;
	} *uap;

	uap = (struct a *)u.u_ap;
	gid = uap->gid;
	if (u.u_rgid == gid || u.u_gid == gid || suser()) {
		leavegroup(u.u_rgid);
		(void) entergroup(gid);
		u.u_gid = gid;
		u.u_rgid = gid;
	}
}

/*
 * Pid of zero implies current process.
 * Pgrp -1 is getpgrp system call returning
 * current process group.
 */
osetpgrp()
{
	register struct proc *p;
	register struct a {
		int	pid;
		int	pgrp;
	} *uap;

	uap = (struct a *)u.u_ap;
	if (uap->pid == 0)
		p = u.u_procp;
	else {
		p = pfind(uap->pid);
		if (p == 0) {
			u.u_error = ESRCH;
			return;
		}
	}
	if (uap->pgrp <= 0) {
		u.u_r.r_val1 = p->p_pgrp;
		return;
	}
	if (p->p_uid != u.u_uid && u.u_uid && !inferior(p)) {
		u.u_error = EPERM;
		return;
	}
	p->p_pgrp = uap->pgrp;
}

otime()
{

	u.u_r.r_time = time.tv_sec;
}

ostime()
{
	register struct a {
		int	time;
	} *uap = (struct a *)u.u_ap;
	struct timeval tv;

	tv.tv_sec = uap->time;
	tv.tv_usec = 0;
	setthetime(&tv);
}

/* from old timeb.h */
struct timeb {
	time_t	time;
	u_short	millitm;
	short	timezone;
	short	dstflag;
};

oftime()
{
	register struct a {
		struct	timeb	*tp;
	} *uap;
	struct timeb tb;

	uap = (struct a *)u.u_ap;
#if	CS_GENERIC
	(void) splhigh();
#else	CS_GENERIC
	(void) spl7();
#endif	CS_GENERIC
	tb.time = time.tv_sec;
	tb.millitm = time.tv_usec / 1000;
	(void) spl0();
	tb.timezone = tz.tz_minuteswest;
	tb.dstflag = tz.tz_dsttime;
	u.u_error = copyout((caddr_t)&tb, (caddr_t)uap->tp, sizeof (tb));
}

oalarm()
{
	register struct a {
		int	deltat;
	} *uap = (struct a *)u.u_ap;
	register struct proc *p = u.u_procp;
#if	CS_GENERIC
	int s = splhigh();
#else	CS_GENERIC
	int s = spl7();
#endif	CS_GENERIC

	untimeout(realitexpire, (caddr_t)p);
	timerclear(&p->p_realtimer.it_interval);
	u.u_r.r_val1 = 0;
	if (timerisset(&p->p_realtimer.it_value) &&
	    timercmp(&p->p_realtimer.it_value, &time, >))
		u.u_r.r_val1 = p->p_realtimer.it_value.tv_sec - time.tv_sec;
	if (uap->deltat == 0) {
		timerclear(&p->p_realtimer.it_value);
		splx(s);
		return;
	}
	p->p_realtimer.it_value = time;
	p->p_realtimer.it_value.tv_sec += uap->deltat;
	timeout(realitexpire, (caddr_t)p, hzto(&p->p_realtimer.it_value));
	splx(s);
}

onice()
{
	register struct a {
		int	niceness;
	} *uap = (struct a *)u.u_ap;
	register struct proc *p = u.u_procp;

	donice(p, (p->p_nice-NZERO)+uap->niceness);
}

#include "../h/times.h"

otimes()
{
	register struct a {
		struct	tms *tmsb;
	} *uap = (struct a *)u.u_ap;
	struct tms atms;

	atms.tms_utime = scale60(&u.u_ru.ru_utime);
	atms.tms_stime = scale60(&u.u_ru.ru_stime);
	atms.tms_cutime = scale60(&u.u_cru.ru_utime);
	atms.tms_cstime = scale60(&u.u_cru.ru_stime);
	u.u_error = copyout((caddr_t)&atms, (caddr_t)uap->tmsb, sizeof (atms));
}

scale60(tvp)
	register struct timeval *tvp;
{

	return (tvp->tv_sec * 60 + tvp->tv_usec / 16667);
}

#include "../h/vtimes.h"

ovtimes()
{
	register struct a {
		struct	vtimes *par;
		struct	vtimes *chi;
	} *uap = (struct a *)u.u_ap;
	struct vtimes avt;

	if (uap->par) {
		getvtimes(&u.u_ru, &avt);
		u.u_error = copyout((caddr_t)&avt, (caddr_t)uap->par,
			sizeof (avt));
		if (u.u_error)
			return;
	}
	if (uap->chi) {
		getvtimes(&u.u_cru, &avt);
		u.u_error = copyout((caddr_t)&avt, (caddr_t)uap->chi,
			sizeof (avt));
		if (u.u_error)
			return;
	}
}

#ifndef	romp
#include "../machine/psl.h"
#include "../machine/reg.h"
#endif	romp

owait()
{
#ifdef	romp
	return;
#else	romp
	struct rusage ru;
	struct vtimes *vtp, avt;

	if ((u.u_ar0[PS] & PSL_ALLCC) != PSL_ALLCC) {
		u.u_error = wait1(0, (struct rusage *)0);
		return;
	}
	vtp = (struct vtimes *)u.u_ar0[R1];
	u.u_error = wait1(u.u_ar0[R0], &ru);
	if (u.u_error)
		return;
	getvtimes(&ru, &avt);
	(void) copyout((caddr_t)&avt, (caddr_t)vtp, sizeof (struct vtimes));
#endif	romp
}

getvtimes(aru, avt)
	register struct rusage *aru;
	register struct vtimes *avt;
{

	avt->vm_utime = scale60(&aru->ru_utime);
	avt->vm_stime = scale60(&aru->ru_stime);
	avt->vm_idsrss = ((aru->ru_idrss+aru->ru_isrss) / hz) * 60;
	avt->vm_ixrss = aru->ru_ixrss / hz * 60;
	avt->vm_maxrss = aru->ru_maxrss;
	avt->vm_majflt = aru->ru_majflt;
	avt->vm_minflt = aru->ru_minflt;
	avt->vm_nswap = aru->ru_nswap;
	avt->vm_inblk = aru->ru_inblock;
	avt->vm_oublk = aru->ru_oublock;
}

ovlimit()
{
#if	CS_COMPAT
	register struct a {
		u_int	which;
		int	limit;
	} *uap = (struct a *)u.u_ap;
	register struct rlimit *alimp;
	struct rlimit noraise;
 
	if (uap->which > RLIM_NLIMITS) {
		u.u_error = EINVAL;
		return;
	}
	if (uap->which-- == 0)
	{
		/*
		 *  Use CPU time limit to remember old NORAISE state.  If the
		 *  limit is not infinite (RLIM_INIFINITY set at startup and
		 *  below), then restrictions are in effect, otherwise they are
		 *  not.
		 */
		if (u.u_rlimit[RLIMIT_CPU].rlim_max == RLIM_INFINITY)
			noraise.rlim_cur = 0;
		else
			noraise.rlim_cur = -1;
		alimp = &noraise;
	}
	else
		alimp = &u.u_rlimit[uap->which];
	u.u_r.r_val1 = alimp->rlim_cur;
	if (uap->limit < 0)
		return;
	if (uap->limit > alimp->rlim_cur && (u.u_rlimit[RLIMIT_CPU].rlim_max != RLIM_INFINITY))
		if (!suser())
			return;
	switch (uap->which)
	{
	case RLIMIT_DATA:
		if (uap->limit > ctob(MAXDSIZ))
			uap->limit = ctob(MAXDSIZ);
		break;
	case RLIMIT_STACK:
		if (uap->limit > ctob(MAXSSIZ))
			uap->limit = ctob(MAXSSIZ);
		break;
	}
	alimp->rlim_cur = uap->limit;
	if (uap->which == RLIMIT_RSS)
		u.u_procp->p_maxrss = uap->limit/NBPG;
	if (alimp == &noraise)
	{
		if (alimp->rlim_cur)
			u.u_rlimit[RLIMIT_CPU].rlim_max	= RLIM_INFINITY-1;
		else
			u.u_rlimit[RLIMIT_CPU].rlim_max	= RLIM_INFINITY;
		
	}
#else	CS_COMPAT

	u.u_error = EACCES;
#endif	CS_COMPAT
}

ossig()
{
	struct a {
		int	signo;
		int	(*fun)();
	} *uap = (struct a *)u.u_ap;
	register int a;
	struct sigvec vec;
	register struct sigvec *sv = &vec;
	struct proc *p = u.u_procp;

#if	CS_COMPAT
	long signalmask;
	extern int osigcode[3];
 
	a = uap->signo & 0377;
#if	CS_BUGFIX
	if(a<=0 || a>NSIG || a==SIGKILL || a==SIGSTOP) {
#else	CS_BUGFIX
	if(a<=0 || a>=NSIG || a==SIGKILL || a==SIGSTOP) {
#endif	CS_BUGFIX
		u.u_error = EINVAL;
		return;
	}
	sv->sv_handler = uap->fun;
	if (u.u_pcb.pcb_sigc[0] != osigcode[0])
		bcopy((caddr_t)osigcode, (caddr_t)u.u_pcb.pcb_sigc, sizeof(osigcode));
	if ((uap->signo &~ 0377) || (sv->sv_handler != SIG_DFL && sv->sv_handler != SIG_IGN &&
	    (((int)(sv->sv_handler))&1)))
	{
		p->p_flag |= SJCSIG;
	}
	else
		p->p_flag |= SOUSIG;	/* mark as simulating old stuff */
	signalmask = 1L << (a-1);
	/* 
	 * Don't clobber registers if we are to simulate
	 * a ret+rti.
	 */
	if ((uap->signo&01000) == 0)
		u.u_r.r_val1 = (int)u.u_signal[a];
	(void) splhigh();
	switch ((int)sv->sv_handler)
        {
	    case SIG_HOLD:
		p->p_sigmask |= signalmask;
		break;
 
	    case SIG_IGN:
		p->p_sigmask &= ~signalmask;
		break;
	
	    default:
		p->p_sigmask &= ~signalmask;
		if (((int)(sv->sv_handler))&1)
		{
			sv->sv_mask = signalmask;
			*((int *)(&sv->sv_handler)) &= ~1;
		}
		else
			sv->sv_mask = 0;
		break;
	}
	sv->sv_flags = SV_INTERRUPT;
	setsigvec(a, sv);
	/*
	 * Now handle options.
	 */
	if (uap->signo & 0400) {
		/*
		 * Simulate a PDP11 style wait instrution which
		 * atomically lowers priority, enables interrupts
		 * and hangs.
		 */
		opause();
		/*NOTREACHED*/
	}
	if (uap->signo & 01000)
		u.u_eosys = SIMULATERTI;
#else	CS_COMPAT
	a = uap->signo;
	sv->sv_handler = uap->fun;
	/*
	 * Kill processes trying to use job control facilities
	 * (this'll help us find any vestiges of the old stuff).
	 */
	if ((a &~ 0377) ||
	    (sv->sv_handler != SIG_DFL && sv->sv_handler != SIG_IGN &&
	     ((int)sv->sv_handler) & 1)) {
		psignal(p, SIGSYS);
		return;
	}
#if	CS_BUGFIX
	if (a <= 0 || a > NSIG || a == SIGKILL || a == SIGSTOP ||
#else	CS_BUGFIX
	if (a <= 0 || a >= NSIG || a == SIGKILL || a == SIGSTOP ||
#endif	CS_BUGFIX
	    a == SIGCONT && sv->sv_handler == SIG_IGN) {
		u.u_error = EINVAL;
		return;
	}
	sv->sv_mask = 0;
	sv->sv_flags = SV_INTERRUPT;
	u.u_r.r_val1 = (int)u.u_signal[a];
	setsigvec(a, sv);
	p->p_flag |= SOUSIG;		/* mark as simulating old stuff */
#endif	CS_COMPAT
}
#endif
