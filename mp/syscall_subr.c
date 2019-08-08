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
 **************************************************************************
 * HISTORY
 * 28-Apr-86  Avadis Tevanian (avie) at Carnegie-Mellon University
 *	Cleaned up: conditionals for MACH_SHM, removed timekeeper garbage.
 *
 * 11-Oct-85  Robert V Baron (rvb) at Carnegie-Mellon University
 *	Worked on NM.  Flushed MPS and added IPC
 *
 * 15-Aug-85  Avadis Tevanian (avie) at Carnegie-Mellon University
 *	Add TPswtch and TPsystem_name.
 *
 * 21-Nov-84  Robert V Baron (rvb) at Carnegie-Mellon University
 *	add MPS call to return MPS address -- rather than using nlist in
 *	user level code.
 *
 * 16-Nov-84  Robert V Baron (rvb) at Carnegie-Mellon University
 *	Add clock access routines  for fg to allow user level time
 *	synchronization.
 *	Add glocal time_keeper clock.
 **************************************************************************
 */

#include "mach_mp.h"
#include "mach_acc.h"
#include "mach_shm.h"

#include "../machine/reg.h"

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/dir.h"
#include "../h/user.h"
#include "../h/proc.h"
#include "../h/map.h"
#include "../h/inode.h"
#include "../h/ioctl.h"		/* */
#include "../h/file.h"		/* */
#include "../h/uio.h"		/* */
#include "../h/timeb.h"
#include "../h/times.h"
#include "../h/reboot.h"
#include "../h/fs.h"
#include "../h/conf.h"
#include "../h/buf.h"
#include "../h/mount.h"
#include "../h/quota.h"
#include "../h/kernel.h"

#include "../sync/mp_queue.h"
#if	MACH_SHM
#include "../mp/shmem.h"
#endif	MACH_SHM

#include "../accent/accent.h"
#include "../mp/syscall_nm.h"

#ifdef vax
#include "../vax/mtpr.h"
#endif

TPDummy()
{
	return Dummy;
}

TPSuccess()
{
	return Success;
}

TPFailure()
{
	return Dummy;
}

TPgetpid(pid)
char *pid;
{
	return (! suword(pid, u.u_procp->p_pid) ? Success : NotAUserAddress);
}

#define SYSTEM_NAME ""

TPsystem_name(name, len)
	char	*name;
	int	len;
{
	register int	syslen;

	syslen = strlen(SYSTEM_NAME);
	if (len > syslen + 1)
		len = syslen + 1;
	return(copyout((caddr_t) SYSTEM_NAME, (caddr_t) name, len));
}

TPswtch()
{
	u.u_ru.ru_nivcsw++;
	unix_swtch(u.u_procp, 0);
/*	runrun++;
	aston();		/* cause context-switch */
}

/*
 * Read system call.
 */
TPread(fdes, cbuf, count)
int fdes;
char *cbuf;
unsigned count;
{
	struct uio auio;
	struct iovec aiov;

	aiov.iov_base = (caddr_t)cbuf;
	aiov.iov_len = count;
	auio.uio_iov = &aiov;
	auio.uio_iovcnt = 1;
	arwuio(fdes, &auio, UIO_READ);
}

arwuio(fdes, uio, rw)
	register struct uio *uio;
	enum uio_rw rw;
{
	register struct file *fp;
	register struct iovec *iov;
	int i, count;

	GETF(fp, fdes);
	if ((fp->f_flag&(rw==UIO_READ ? FREAD : FWRITE)) == 0) {
		u.u_error = EBADF;
		return;
	}
	uio->uio_resid = 0;
	uio->uio_segflg = 0;
	iov = uio->uio_iov;
	for (i = 0; i < uio->uio_iovcnt; i++) {
		if (iov->iov_len < 0) {
			u.u_error = EINVAL;
			return;
		}
		uio->uio_resid += iov->iov_len;
		if (uio->uio_resid < 0) {
			u.u_error = EINVAL;
			return;
		}
		iov++;
	}
	count = uio->uio_resid;
	uio->uio_offset = fp->f_offset;
	if ((u.u_procp->p_flag&SOUSIG) == 0 && setjmp(&u.u_qsave)) {
		if (uio->uio_resid == count)
			u.u_eosys = RESTARTSYS;
	} else
		u.u_error = (*fp->f_ops->fo_rw)(fp, rw, uio);
	u.u_r.r_val1 = count - uio->uio_resid;
	fp->f_offset += u.u_r.r_val1;
}

Nm()
{
	register struct a {
		long indicator;
		char **addr;
	} *uap = (struct a *) u.u_ap;

	u.u_r.r_val1 = TPnm(uap->indicator, uap->addr);
}

TPnm(indicator, addr)
char **addr;
{
#if	MACH_ACC
	register procp_ ipcp = CurrentProcess();
#endif	MACH_ACC
	register int ret;

	switch (indicator) {
#if	MACH_MP
	case NM_CPU_NO:
		ret = suword(addr, cpu_number()); break;
#endif	MACH_MP
#if	MACH_ACC
	case NM_PIDX:
		ret = suword(addr, pidx(ipcp)); break;
	case NM_PPTR:
		ret = suword(addr, ipcp); break;
	case NM_KPORT:
		ret = suword(addr, ipcp->KPorts[KernelPort]); break;
	case NM_IPC:
		ret = suword(addr, IPC); break;
#endif	MACH_ACC
#if	MACH_SHM
	case NM_SHMRMAP:
		ret = suword(addr, shrmap); break;
	case NM_SHM:
		ret = suword(addr, SHM); break;
	case NM_SHM_BYTES:
		ret = suword(addr, SHM->bytes); break;
	case NM_AAREA:
		ret = suword(addr, aarea); break;
#endif	MACH_SHM
	default:
		return(EINVAL);
	}
	return(ret);
}

TPMoveWords()
{
}
