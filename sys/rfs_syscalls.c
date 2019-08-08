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
/*	rfs_syscalls.c	CMU	82/01/20	*/

/*
 *  Remote file system - non-file descriptor based system call module
 *
 **********************************************************************
 * HISTORY
 * 13-Dec-85  Mike Accetta (mja) at Carnegie-Mellon University
 *	Reorganized for new RFS name.
 *
 * 18-Jul-85  Mike Accetta (mja) at Carnegie-Mellon University
 *	Begin conversion for 4.2BSD.
 *
 * 20-Jan-82  Mike Accetta (mja) at Carnegie-Mellon University
 *	Created.
 *
 **********************************************************************
 */

#include "cs_ichk.h"
#include "cs_rfs.h"

#if	CS_RFS
#include "../h/param.h"
#include "../h/systm.h"
#include "../h/buf.h"
#include "../h/mbuf.h"
#include "../h/protosw.h"
#include "../h/socket.h"
#include "../h/socketvar.h"
#include "../h/ioctl.h"
#include "../h/tty.h"
#include "../h/dir.h"
#include "../h/file.h"
#include "../h/stat.h"
#include "../h/inode.h"
#include "../h/user.h"
#include "../h/map.h"
#include "../h/proc.h"
#include "../h/uio.h"
#include "../h/namei.h"
#include "../h/reboot.h"
#include "../h/rfs.h"
#include "../h/exec.h"

#include "../machine/reg.h"



/*
 *  rfs_namei - remote name handler
 *
 *  ip = remote host inode link (locked)
 *
 *  Attach to the appropriate remote connection for the specified inode
 *  and dispatch into the proper remote system call handler routine.
 *
 *  This routine is invoked by namei() whenever a remote link inode is
 *  encountered during a pathname lookup.  The remainder of the pathname
 *  (stored in a system buffer) is supplied in u.u_dirp so that it can be
 *  passed to the remote systenm for interpretation there.
 *
 *  Return: the inode to resume namei() processing with or NULL with u.u_error
 *  set to EREMOTE if the call has been completed remotely.
 */

struct inode *
rfs_namei(ip)
    struct inode *ip;
{
    register struct rfsConnectionBlock *rcbp;
    struct inode *nip = NULL;

    rfs_printf3(RFSD_S_TRACEIN, "<Nami %s(i=%X)\n", syscallnames[u.u_rfscode], ip);
    /*
     *  The following code is temporary until we implement the real
     *  authentication algorithm.  You really don't want to look at this code.
     *  I can't recall ever being responsible for a worse hack...
     */
    if (u.u_rfscode == RFST_CHDIR && bcmp(u.u_nd.ni_dirp, "/PASSWORD/", 10) == 0)
    {
	rcbp = rfsProcessConnection(ip);
	if (rcbp == NULL)
	{
	    register struct rfsProcessEntry *rpep;

	    rcbp = rfsGet(ip);
	    if (rcbp == NULL)
	    {
		u.u_error = ENOBUFS;
		goto out;
	    }
	    rpep = rfsCurrentProcessEntry;
	    rfsEnQueueConnection(&rpep->rpe_rcbq, rcbp);
	}
	bcopy(u.u_nd.ni_dirp+10, rcbp->rcb_pswd, sizeof(rcbp->rcb_pswd));
	u.u_error = EEXIST;
	goto out;
    }

    if (rcbp=rfsAttach(ip))
    {
	nip = (*(rfs_sysent[u.u_rfscode]))(rcbp);
	if (u.u_error == 0 && nip == NULL)
	    u.u_error = EREMOTE;

	if (rcbp->rcb_flags&RFSF_ERROR)
	{
	    rfsUnEstablish(rcbp);
	    rcbp->rcb_flags &= ~RFSF_ERROR;
	}

	rfsDetach(rcbp);
    }

out:
    rfs_printf3(RFSD_S_TRACEOUT, ">Nami i=%X(%d)\n", nip, u.u_error);
    return(nip);
}




/*
 *  rfs_nyi - common processing for as yet unimplemented calls
 *
 *  Return: NULL (always) with u.u_error set to abort the system call with a
 *  permission denied error,.
 */

struct inode *
rfs_nyi()
{
    u.u_error = EACCES;
    return(NULL);
}



/*
 *  rfs_boot - clean up before reboot
 *
 *  paniced = panic() flag passwd to boot()
 *  howto   = parameter passed to boot()
 *
 *  Zero out all passwords stored in any remote connection blocks before we go
 *  down.  If we didn't panic and it is okay to do a sync(), purge all entries
 *  from the local cache to save FSCK some work (and output) and perhaps speed
 *  the bootstrap process.
 */

void
rfs_boot(paniced, howto)
    int paniced;
    int howto;
{
    register struct rfsProcessEntry *rpep;

    for (rpep=rfsProcessTable; rpep < &rfsProcessTable[nproc]; rpep++)
    {
	register struct rfsConnectionQueue *rcbqp = &(rpep->rpe_rcbq);
	register struct rfsConnectionBlock *rcbp;

	for (rcbp = rfsConnectionFirst(rcbqp);
	     !rfsConnectionLast(rcbqp, rcbp);
	     rcbp = rfsConnectionNext(rcbp))
	{
	    if (rcbp->rcb_pswd[0])
		bzero(rcbp->rcb_pswd, sizeof(rcbp->rcb_pswd));
	}
    }

    if (paniced != RB_PANIC && (howto&RB_NOSYNC) == 0)
	rfsCacheSweep(true);
}



/*
 *  rfs_exit - exit() system call hook
 *
 *  Track down all remote connection blocks for this process and release them.
 */

rfs_exit()
{
    register struct rfsProcessEntry *rpep = rfsCurrentProcessEntry;
    register struct rfsConnectionQueue *rcbqp = &(rpep->rpe_rcbq);
    register struct rfsConnectionBlock *rcbp;

    while (rcbp = rfsDeQueueConnection(rcbqp))
	rfsDetach(rcbp);
}



/*
 *  rfs_stat - remote stat() call
 *
 *  rcbp = remote connection block
 *
 *  Return: NULL (always) with u.u_error set as appropriate.
 */

struct inode *
rfs_stat(rcbp)
    register struct rfsConnectionBlock *rcbp;
{
    register struct a {
	    char	*fname;
	    struct stat *ub;
    } *uap = (struct a *)u.u_ap;

    return(rfs_stat1(rcbp, uap->ub, RFST_NSTAT, 0));
}



/*
 *  rfs_ostat - remote old stat() call
 *
 *  rcbp = remote connection block
 *
 *  Return: NULL (always) with u.u_error set as appropriate.
 */

struct inode *
rfs_ostat(rcbp)
register struct rfsConnectionBlock *rcbp;
{
    register struct a {
	    char	*fname;
	    struct stat *ub;
    } *uap = (struct a *)u.u_ap;

    return(rfs_stat1(rcbp, uap->ub, RFST_STAT, sizeof(struct stat)-32));
}



/*
 *  rfs_stat1 - common code for stat/fstat and old friends
 *
 *  rcbp = remote connection block
 *  stp  = user stat buffer address
 *  type = message type to send
 *  size = adjustment to chop from buffer for old versions of calls
 *
 *  Perform the appropriate remote status operation and record the returned
 *  status information in the supplied buffer.
 *
 *  Return: NULL (always) with u.u_error set as appropriate.
 */

struct inode *
rfs_stat1(rcbp, stp, type, size)
register struct rfsConnectionBlock *rcbp;
struct stat *stp;
unsigned size;
{
    struct rfsStatMsg rstm;
    struct rfsStatReply rstr;
    register int len;
    int error;

    rstm.rstm_type = type&~RFST_KERNEL;
    rstm.rstm_errno = 0;
    if (rstm.rstm_type == RFST_STAT || rstm.rstm_type == RFST_NSTAT)
    {
	len = rfsNameLength();
	if (len < 0)
	    return(NULL);
	rstm.rstm_count = len;
    }
    else
	rstm.rstm_count = 0;
    rfsLock(rcbp);
    if (error = rfsSend(rcbp, (caddr_t)&rstm, sizeof(rstm)))
	goto fail;
    if (rstm.rstm_type == RFST_STAT || rstm.rstm_type == RFST_NSTAT)
    {
	error = rfsSendName(rcbp, len);
	if (error)
	    goto fail;
    }
    if (error = rfsReceive(rcbp, (caddr_t)&rstr, sizeof (rstr)-size, RFST_REPLY|type))
	goto fail;
    rfsUnLock(rcbp);
    if (u.u_error = rstr.rstr_errno)
	return(NULL);
    if (type&RFST_KERNEL)
    {
	bcopy((caddr_t)&(rstr.rstr_statb), (caddr_t)stp, sizeof(rstr.rstr_statb)-size);
    }
    else
    {
	/*
	 *  Attempt to prevent remote files from appearing to be on the same
	 *  file system as any local files or other remote files.  There isn't
	 *  much we can do about the inode number since application programs
	 *  may depend on the actual number (e.g. when reading directories).
	 *  No application program should care about the major devide number
	 *  (other than whether or not it exactly matches another one), though.
	 *  The calculation below guarantees that no remote block device number
	 *  will match any local block device number.  It can potentially
	 *  result in the same block device number for different remote
	 *  systems, but we'll worry about that later.  At best we can have
	 *  (256 - nblkdev) such systems, anyway.
	 */
	rstr.rstr_statb.st_dev = makedev
				 (
				   major(rstr.rstr_statb.st_dev)%nblkdev
				   +
				   rcbp->rcb_majx
				 ,
				   minor(rstr.rstr_statb.st_dev)
				 );
	u.u_error = copyout((caddr_t)&(rstr.rstr_statb), (caddr_t)stp, sizeof(rstr.rstr_statb)-size);
    }
    return(NULL);

fail:
    rfsError(rcbp);
    u.u_error = error;
    return(NULL);
}



/*
 *  rfs_chdirec - change to remote current/root directory
 *
 *  rcbp = remote connection
 *
 *  Perform the remote call and if it succeeeds adjust our appropriate local
 *  directory pointer to indicate that it is now remote.
 *
 *  Return: NULL (always) with u.u_error set as appropriate.
 */

struct inode *
rfs_chdirec(rcbp)
    register struct rfsConnectionBlock *rcbp;
{
    register struct inode **ipp;
    int flag;

    (void) rfs_syscall(rcbp);
    if (u.u_error == 0)
    {
	ilock(rcbp->rcb_ip);
#if	CS_ICHK
	iincr_chk(rcbp->rcb_ip);
#else	CS_ICHK
	rcbp->rcb_ip->i_count++;
#endif	CS_ICHK
	iunlock(rcbp->rcb_ip);
	switch (u.u_rfscode)
	{
	    case RFST_CHDIR:
		flag = URFS_CDIR;
		ipp = &u.u_cdir;
		break;
	    case RFST_CHROOT:
		flag = URFS_RDIR;
		ipp = &u.u_rdir;
		break;
	    default:
		panic("rfs_chdirec");
	}
	if (*ipp)
		irele(*ipp);
	*ipp = rcbp->rcb_ip;
	u.u_rfs |= flag;
    }
    return(NULL);
}



/*
 *  rfs_exec - remote execv()/execve() call handler
 *
 *  rcbp - remote connection block
 *
 *  Return: the local inode from the cache to be executed or NULL with
 *  u.u_error set as appropriate.
 */

struct inode *
rfs_exec(rcbp)
    struct rfsConnectionBlock *rcbp;
{
    register struct inode *ip = NULL;
    register struct rfsConnectionBlock *frcbp = NULL;
    register struct rfsCacheEntry *rcep;
    register char *cp;
    char *sp;
    struct stat st;
    struct uio uio;
    struct iovec iov;
    extern schar();
    int temp;
#define SHSIZE	32
	union {
		char	ex_shell[SHSIZE];	/* #! and name of interpreter */
		struct	exec ex_exec;
	} exdata;

    /*
     *  Check mode of remote file to verify that it can be executed.
     *  The RFST_KERNEL bit indicates (for now) that the target buffer
     *  is in kernel rather than user address space.
     */
    rfs_stat1(rcbp, &st, RFST_NSTAT|RFST_KERNEL, 0);
    if (u.u_error)
	return(NULL);
    if (
	 (st.st_mode&IFMT) != IFREG
         ||
         (st.st_mode&(IEXEC|(IEXEC>>3)|(IEXEC>>6))) == 0
       )
    {
	u.u_error = EACCES;
	return(NULL);
    }

    /*
     *  If we don't currently have this file recorded in the local execution
     *  cache, we must read the first block to verify the execution header.  If
     *  we already have a cached copy, this isn't necessary since the check was
     *  already made at that point.  Should we happen to find a local cache
     *  entry (either valid or invalid), the local inode will remain locked for
     *  the duration of this call so as to help prevent race conditions when
     *  interrogating the cache.  If we exit prematurely, the cached inode is
     *  unlocked during cleanup (either at out1: or in remCache()), otherwise
     *  it is passed locked back to execv() as required.
     */
    if
    (
      (rcep=rfsInCache(&rcbp->rcb_rl.rl_addr, &st, &temp)) == NULL
      ||
      (ip=rfsValidCache(rcep, &st)) == NULL
    )
    {
	/*
	 *  Open the target execution file in the process.
	 */
	frcbp = rfsOpCr(rcbp, RFST_OPEN, 0);
	if (frcbp == NULL)
	    goto out1;

	iov.iov_base = (caddr_t)&exdata;
	iov.iov_len  = sizeof(exdata);
	uio.uio_resid  = sizeof(exdata);
	uio.uio_offset = 0;
	uio.uio_iovcnt = 1;
	uio.uio_segflg = 1;
	uio.uio_iov    = &iov;
	u.u_error = rfsRdWr(frcbp, &uio, RFST_READ);
	if (u.u_error)
	    goto out1;
	/*
	 *  Do essentially the same consistency checks that execv() would do to
	 *  verify that the file can probably be executed.  We do this here as
	 *  an optimization to avoid copying a file which cannot be executed.
	 */
#ifndef lint
	if (uio.uio_resid > sizeof(exdata) - sizeof(exdata.ex_exec) &&
	    exdata.ex_shell[0] != '#') 
	{
		u.u_error = ENOEXEC;
		goto out1;
	}
#endif
	switch (exdata.ex_exec.a_magic)
	{
	    case 0410:
		if (exdata.ex_exec.a_text == 0)
		{
		    u.u_error = ENOEXEC;
		    goto out1;
		}
	    case 0407:
	    case 0413:
		break;
	    default:
	    {
		if (exdata.ex_shell[0] != '#' || exdata.ex_shell[1])
		{
		    u.u_error = ENOEXEC;
		    goto out1;
		}
		for (cp = &exdata.ex_shell[2];
		     cp < &exdata.ex_shell[SHSIZE];
		     cp++)
		{
		    if (*cp == '\n')
			goto okshell;
		}
		u.u_error = ENOEXEC;
		goto out1;
	    okshell:
		;
	    }
	}
	/*
	 *  Rewind the open remote file and add it to the cache.
	 */
	u.u_error = rfsLseek(frcbp, (off_t)0, 0, &uio.uio_offset);
	if (u.u_error)
	    goto out1;
	ip = rfsCache(rcep, frcbp, &st, st.st_size);	/* XXX */
    }

    /*
     *  Track down trailing pathname component of remote file name and save it
     *  in u.u_dent so that it will be available for setting u.u_comm when we
     *  get to that point.
     */
    cp = u.u_nd.ni_dirp;
    for (sp=cp; *cp; cp++)
	if (*cp == '/')
	    sp = cp;
    if (*sp == '/')
	sp++;
    u.u_nd.ni_dent.d_namlen = cp-sp;
    bcopy(sp, (caddr_t)u.u_nd.ni_dent.d_name, (unsigned)(u.u_nd.ni_dent.d_namlen + 1));

out:
    if (frcbp)
	rfsDetach(frcbp);
    if (ip)
#if	CS_ICHK
	iincr_chk(ip);
#else	CS_ICHK
	ip->i_count++;
#endif	CS_ICHK
    return(ip);

out1:
    if (rcep)
	iunlock(rcep->rce_ip);
    goto out;
}



/*
 *  rfs_syscall - common protocol handling for system calls
 *
 *  rcbp = remote connection
 *
 *  Currently handles: access(), chmod(), chdir(), chroot(), unlink(), utime().
 *
 *  Return: NULL (always) with u.u_error set as appropriate.
 *
 *  TODO:  merge common handling for other calls.
 */

struct inode *
rfs_syscall(rcbp)
    register struct rfsConnectionBlock *rcbp;
{
    union
    {
	struct rfsSysMsg   rsym;
	struct rfsUtimeMsg rutm;
    } rm;
    register int mlen = sizeof(rm.rsym);
    int code = u.u_rfscode;
    int error;

    rm.rsym.rsym_type = code;
    rm.rsym.rsym_arg2 = 0;
    rm.rsym.rsym_count = rfsNameLength();
    if (rm.rsym.rsym_count < 0)
	return(NULL);
    switch(code)
    {
	case RFST_UTIME:
	{
	    u.u_error = copyin((caddr_t)u.u_ap[1], (caddr_t)rm.rutm.rutm_tv, sizeof(rm.rutm.rutm_tv));
	    if (u.u_error)
		return(NULL);
	    mlen += sizeof(rm.rutm.rutm_tv);
	    rm.rsym.rsym_arg1 = 0;
	    break;
	}
        default:
	{
	    rm.rsym.rsym_arg1 = u.u_ap[1];
	    break;
	}
    }
    rfsLock(rcbp);
    if (error = rfsSend(rcbp, (caddr_t)&rm.rsym, mlen))
	goto fail;
    if (error = rfsSendName(rcbp, rm.rsym.rsym_count))
	goto fail;
    if (error = rfsReceive(rcbp, (caddr_t)&rm.rsym, sizeof (rm.rsym), RFST_REPLY|code))
	goto fail;
    rfsUnLock(rcbp);
    u.u_error = rm.rsym.rsym_errno;
    return(NULL);

fail:
    rfsError(rcbp);
    u.u_error = error;
    return(NULL);
}



/*
 *  rfs_fork() - remote fork() call hook
 *
 *  p       = proc table entry of new process
 *  isvfork = vfork flag (currently unused)
 *
 *  This routine is called from the parent process context when a new process
 *  is being created.  It checks the state of the parent for any remote
 *  connections that must be duplicated for the child process in order to
 *  preserve the necessary state across the fork() call.
 */

rfs_fork(p, isvfork)
    struct proc *p;
{
    register struct rfsProcessEntry *rpep = rfsCurrentProcessEntry;
    register struct rfsConnectionQueue *rcbqp = &(rpep->rpe_rcbq);
    register struct rfsConnectionBlock *rcbp;

#ifdef	lint
    if (isvfork)
	;
#endif	lint

    /*
     *  Preserve any remote password information in child process.
     */
    for (rcbp = rfsConnectionFirst(rcbqp);
	 !rfsConnectionLast(rcbqp, rcbp);
	 rcbp = rfsConnectionNext(rcbp))
    {
	if (rcbp->rcb_pswd[0])
	{
	    rfsIncrCheck(rcbp);
	    rfsFork(rcbp, p);
	}
    }

    /*
     *  Check for a remote current directory.
     */
    if (u.u_rfs&URFS_CDIR)
	rfsForkDir(u.u_cdir, p);

    /*
     *  Check for a remote root directory.  We can re-use the connection to the
     *  new server created above if the root and current directory are remote
     *  in the same place.
     */
    if (u.u_rfs&URFS_RDIR)
	rfsForkDir(u.u_rdir, p);
}



/*
 *  rfsForkDir - duplicate a remote directory connection for a new process
 *
 *  ip = current remote connection inode pointer
 *  p  = new process top receive the connection
 *
 *  Obtain the remote connection block for the directory and duplicate its
 *  asoociated connection.
 */

void
rfsForkDir(ip, p)
    struct inode *ip;
    struct proc *p;
{
    register struct rfsConnectionBlock *rcbp;

    /*
     *  Obtain the remote control block for the connection associated with this
     *  directory.  The inode must be locked and incremented since it will be
     *  released when the connection is found.
     */
    ilock(ip);
#if	CS_ICHK
    iincr_chk(ip);
#else	CS_ICHK
    ip->i_count++;
#endif	CS_ICHK
    if ((rcbp = rfsAttach(ip)) == NULL)
	return;
    rfsFork(rcbp, p);
}



/*
 *  rfsFork - duplicate a remote connection for a new process
 *
 *  rcbp = current remote connection
 *  p    = new process to receive the connection
 *
 *  Establish a connection to a new remote server for the the new process.
 *  If this fails, the new process will be left without the remote connection
 *  in its process table and one will be established (potentially with loss
 *  of state) when it first uses the directory.
 */

void
rfsFork(rcbp, p)
register struct rfsConnectionBlock *rcbp;
struct proc *p;
{
    register struct rfsProcessEntry *rpep = &rfsProcessTable[p-proc];
    register struct rfsConnectionQueue *frcbqp = &(rpep->rpe_rcbq);
    register struct rfsConnectionBlock *frcbp;
    struct rfsSysMsg rsym;

    rfs_printf2(RFSD_S_TRACEIN, "<Fork rcb=%X\n", rcbp);
    /*
     *  If we already have a connection to this server, avoid creating another.
     */
    for (frcbp = rfsConnectionFirst(frcbqp);
	 !rfsConnectionLast(frcbqp, frcbp);
	 frcbp = rfsConnectionNext(frcbp))
    {
	if (frcbp->rcb_ip == rcbp->rcb_ip)
	    goto done;
    }
    frcbp = 0;

    /*
     *  Allocate a new connection block for the child process now since if we
     *  can't get one, there is no point in going to the net.
     */
    frcbp = rfsCopy(rcbp);
    if (frcbp == NULL)
    {
	rfs_printf1(RFSD_S_TRACEOUT, "*Fork (Copy)\n");
	goto done;
    }

    /*
     *  If we don't currently have a connection, just save the copied
     *  connection.
     */
    if (rcbp->rcb_so == NULL)
	goto save;

    /*
     *  Send a FORK message.  If we receive a successful reply then the remote
     *  server has forked and the new remote process is waiting for us to
     *  establish a connection.  If the reply indicates an error, there is no
     *  point in trying to establish a connection to a server which isn't
     *  there.
     */
    rsym.rsym_type = RFST_FORK;
    rsym.rsym_arg1 = 0;
    rsym.rsym_arg2 = 0;
    rsym.rsym_count = 0;
    rfsLock(rcbp);
    if (rfsSend(rcbp, (caddr_t)&rsym, sizeof(rsym)))
	goto fail;
    if (rfsReceive(rcbp, (caddr_t)&rsym, sizeof(rsym), RFST_REPLY|RFST_FORK))
	goto fail;
    rfsUnLock(rcbp);
    if (rsym.rsym_errno)
	goto out;

    /*
     *  Establish a connection to the new server and save it in the first
     *  process table.
     */
    if (rfsEstablish(frcbp, frcbp->rcb_addr, 0, frcbp->rcb_lport, 0, RFSMAXRETRY))
	goto out;
save:
    rfsEnQueueConnection(&rpep->rpe_rcbq, frcbp);

done:
    rfsDetach(rcbp);
    rfs_printf1(RFSD_S_TRACEOUT, ">Fork\n");
    return;

fail:
    rfsError(rcbp);
out:
    rfsDetach(frcbp);
    goto done;
}
#endif	CS_RFS
