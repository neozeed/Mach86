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
 *	@(#)quota_subr.c	6.4 (Berkeley) 6/8/85
 */

#ifdef QUOTA
/*
 * MELBOURNE QUOTAS
 *
 * Miscellaneous subroutines.
 */
#include "param.h"
#include "systm.h"
#include "dir.h"
#include "user.h"
#include "proc.h"
#include "inode.h"
#include "quota.h"
#include "mount.h"
#include "fs.h"
#include "uio.h"

struct dquot *
dqp(q, dev)
	struct quota *q;
	dev_t dev;
{
	register struct dquot **dqq;
	register i;

	if (q == NOQUOTA || q->q_flags & Q_NDQ)
		return (NODQUOT);
	i = getfsx(dev);
	if (i < 0 || i >= NMOUNT)
		return (NODQUOT);
	dqq = &q->q_dq[i];
	if (*dqq == LOSTDQUOT) {
		*dqq = discquota(q->q_uid, mount[i].m_qinod);
		if (*dqq != NODQUOT)
			(*dqq)->dq_own = q;
	}
	if (*dqq != NODQUOT)
		(*dqq)->dq_cnt++;
	return (*dqq);
}

/*
 * Quota cleanup at process exit, or when
 * switching to another user.
 */
qclean()
{
	register struct proc *p = u.u_procp;
	register struct quota *q = p->p_quota;

	if (q == NOQUOTA)
		return;
	/*
	 * Before we rid ourselves of this quota, we must be sure that
	 * we no longer reference it (otherwise clock might do nasties).
	 * But we have to have some quota (or clock will get upset).
	 * (Who is this clock anyway ??). So we will give ourselves
	 * root's quota for a short while, without counting this as
	 * a reference in the ref count (as either this proc is just
	 * about to die, in which case it refers to nothing, or it is
	 * about to be given a new quota, which will just overwrite this
	 * one).
	 */
	p->p_quota = quota;
	u.u_quota = quota;
	delquota(q);
}

qstart(q)
	register struct quota *q;
{

	u.u_quota = q;
	u.u_procp->p_quota = q;
}

qwarn(dq)
	register struct dquot *dq;
{
	register struct fs *fs = NULL;

	if (dq->dq_isoftlimit && dq->dq_curinodes >= dq->dq_isoftlimit) {
		dq->dq_flags |= DQ_MOD;
		fs = getfs(dq->dq_dev);
		if (dq->dq_iwarn && --dq->dq_iwarn)
			uprintf(
			    "Warning: too many files on %s, %d warning%s left\n"
			    , fs->fs_fsmnt
			    , dq->dq_iwarn
			    , dq->dq_iwarn > 1 ? "s" : ""
			);
		else
			uprintf(
			    "WARNING: too many files on %s, NO MORE!!\n"
			    , fs->fs_fsmnt
			);
	} else
		dq->dq_iwarn = MAX_IQ_WARN;

	if (dq->dq_bsoftlimit && dq->dq_curblocks >= dq->dq_bsoftlimit) {
		dq->dq_flags |= DQ_MOD;
		if (fs == NULL)
			fs = getfs(dq->dq_dev);
		if (dq->dq_bwarn && --dq->dq_bwarn)
			uprintf(
		    "Warning: too much disc space on %s, %d warning%s left\n"
			    , fs->fs_fsmnt
			    , dq->dq_bwarn
			    , dq->dq_bwarn > 1 ? "s" : ""
			);
		else
			uprintf(
		    "WARNING: too much disc space on %s, NO MORE!!\n"
			    , fs->fs_fsmnt
			);
	} else
		dq->dq_bwarn = MAX_DQ_WARN;
}
#endif
