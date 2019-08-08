/*
 * Copyright (c) 1982, 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)sys_process.c	7.1 (Berkeley) 6/5/86
 */

#include "../machine/reg.h"
#include "../machine/psl.h"
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
#include "ptrace.h"

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

#if defined(vax)
#define	NIPCREG 16
int ipcreg[NIPCREG] =
	{R0,R1,R2,R3,R4,R5,R6,R7,R8,R9,R10,R11,AP,FP,SP,PC};
#endif

#define	PHYSOFF(p, o) \
	((physadr)(p)+((o)/sizeof(((physadr)0)->r[0])))

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

	case PT_READ_I:			/* read the child's text space */
		if (!useracc((caddr_t)ipc.ip_addr, 4, B_READ))
			goto error;
		ipc.ip_data = fuiword((caddr_t)ipc.ip_addr);
		break;

	case PT_READ_D:			/* read the child's data space */
		if (!useracc((caddr_t)ipc.ip_addr, 4, B_READ))
			goto error;
		ipc.ip_data = fuword((caddr_t)ipc.ip_addr);
		break;

	case PT_READ_U:			/* read the child's u. */
		i = (int)ipc.ip_addr;
		if (i<0 || i >= ctob(UPAGES))
			goto error;
		ipc.ip_data = *(int *)PHYSOFF(&u, i);
		break;

	case PT_WRITE_I:		/* write the child's text space */
		/*
		 * If text, must assure exclusive use
		 */
		if (xp = u.u_procp->p_textp) {
			if (xp->x_count!=1 || xp->x_iptr->i_mode&ISVTX)
				goto error;
			xp->x_flag |= XTRC;
		}
		i = -1;
		if ((i = suiword((caddr_t)ipc.ip_addr, ipc.ip_data)) < 0) {
			if (chgprot((caddr_t)ipc.ip_addr, RW) &&
			    chgprot((caddr_t)ipc.ip_addr+(sizeof(int)-1), RW))
				i = suiword((caddr_t)ipc.ip_addr, ipc.ip_data);
			(void) chgprot((caddr_t)ipc.ip_addr, RO);
			(void) chgprot((caddr_t)ipc.ip_addr+(sizeof(int)-1), RO);
		}
		if (i < 0)
			goto error;
		if (xp)
			xp->x_flag |= XWRIT;
		break;

	case PT_WRITE_D:		/* write the child's data space */
		if (suword((caddr_t)ipc.ip_addr, 0) < 0)
			goto error;
		(void) suword((caddr_t)ipc.ip_addr, ipc.ip_data);
		break;

	case PT_WRITE_U:		/* write the child's u. */
		i = (int)ipc.ip_addr;
		p = (int *)PHYSOFF(&u, i);
		for (i=0; i<NIPCREG; i++)
			if (p == &u.u_ar0[ipcreg[i]])
				goto ok;
		if (p == &u.u_ar0[PS]) {
			ipc.ip_data |= PSL_USERSET;
			ipc.ip_data &=  ~PSL_USERCLR;
			goto ok;
		}
		goto error;

	ok:
		*p = ipc.ip_data;
		break;

	case PT_STEP:			/* single step the child */
	case PT_CONTINUE:		/* continue the child */
		if ((int)ipc.ip_addr != 1)
			u.u_ar0[PC] = (int)ipc.ip_addr;
		if ((unsigned)ipc.ip_data > NSIG)
			goto error;
		u.u_procp->p_cursig = ipc.ip_data;	/* see issig */
		if (i == PT_STEP) 
			u.u_ar0[PS] |= PSL_T;
		wakeup((caddr_t)&ipc);
		return (1);

	case PT_KILL:			/* kill the child process */
		wakeup((caddr_t)&ipc);
		exit(u.u_procp->p_cursig);

	default:
	error:
		ipc.ip_req = -1;
	}
	wakeup((caddr_t)&ipc);
	return (0);
}
