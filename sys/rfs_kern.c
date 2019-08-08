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
/*	rfs_kern.c	CMU	82/01/20	*/

/*
 *  Remote file system - basic protocol services module
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
#include "cs_socket.h"

#if	CS_RFS
#include "../h/param.h"
#include "../h/systm.h"
#include "../h/dir.h"
#include "../h/user.h"
#include "../h/buf.h"
#include "../h/mbuf.h"
#include "../h/protosw.h"
#include "../h/ioctl.h"
#include "../h/tty.h"
#include "../h/file.h"
#include "../h/stat.h"
#include "../h/inode.h"
#include "../h/map.h"
#include "../h/proc.h"
#include "../h/uio.h"
#include "../h/rfs.h"



/*
 *  rfsAttach - attach to a remote connection
 *
 *  ip = host name inode pointer (locked)
 *
 *  Attach to the remote connection for the current process and the
 *  specified host.  If a connection does not already exist, it will be
 *  established and entered into the process connection table.  The
 *  inode is unlocked.
 *
 *  Return: a pointer to the remote control block or NULL on error
 *  (with u.u_error set appropriately).     
 */

struct rfsConnectionBlock *
rfsAttach(ip)
    struct inode *ip;
{
    register struct rfsConnectionBlock *rcbp = NULL;
    register struct rfsProcessEntry *rpep;
    int error;

    rfs_printf2(RFSD_C_TRACEIN, "<Att  ip=%X\n", ip);
    rcbp = rfsProcessConnection(ip);
    if (rcbp == NULL)
    {
	rcbp = rfsGet(ip);
	if (rcbp == NULL)
	{
	    rfs_printf2(RFSD_C_TRACEOUT, ">Att  %d\n", u.u_error);
	    return(NULL);
	}
	rpep = rfsCurrentProcessEntry;
	rfsEnQueueConnection(&rpep->rpe_rcbq, rcbp);
    }
    if (rcbp->rcb_so == NULL)
    {
	error = rfsConnect(rcbp);
	if (error)
	{
	    rfs_printf2(RFSD_C_TRACEOUT, ">Att  %d(Conn)\n", error);
	    u.u_error = error;
	    return(NULL);
	}
    }
    rfsIncrCheck(rcbp);
    rfs_printf3(RFSD_C_TRACEOUT, ">Att  rcb=%X(%d)\n", rcbp, rcbp->rcb_refc);
    return(rcbp);
}



/*
 *  rfsProcessConnection - lookup connection between host and current process
 *
 *  ip = remote host link inode (locked)
 *
 *  Return: NULL if no connection currently exists, otherwise releases the
 *  inode and returns a pointer to the approriate connection block.
 */

struct rfsConnectionBlock *
rfsProcessConnection(ip)
    register struct inode *ip;
{
    register struct rfsProcessEntry *rpep = rfsCurrentProcessEntry;
    register struct rfsConnectionQueue *rcbqp = &(rpep->rpe_rcbq);
    register struct rfsConnectionBlock *rcbp;

    rfs_printf2(RFSD_C_TRACEIN, "<PrCn i=%X\n", ip);
    for (rcbp = rfsConnectionFirst(rcbqp);
	 !rfsConnectionLast(rcbqp, rcbp);
	 rcbp = rfsConnectionNext(rcbp))
    {
	if (rcbp->rcb_ip == ip)
	{
	    iput(ip);
	    rfs_printf2(RFSD_C_TRACEOUT, ">PrCn rcb=%X\n", rcbp);
	    return(rcbp);
	}
    }
    rfs_printf1(RFSD_C_TRACEOUT, ">PrCn NULL\n");
    return(NULL);
}



/*
 *  rfsGet - allocate a remote control block for host
 *
 *  ip = remote host link pointer (locked)
 *
 *  Allocate a new remoteo connection block for the process.  Read the supplied
 *  remote link inode to determine its contact information and record it in the
 *  new remote connection block.  The inode pointer is unlocked.
 *
 *  Return: a pointer to the newly created remote control block or NULL
 *  if none are available.
 */

struct rfsConnectionBlock *
rfsGet(ip)
    struct inode *ip;
{
    register struct rfsConnectionBlock *rcbp;
    int error;
    int resid;

    rfs_printf2(RFSD_C_TRACEIN, "<Get  ip=%X\n", ip);
    rcbp = rfsConnectionAllocate();
    if (rcbp == NULL)
    {
	iput(ip);
	rfs_printf1(RFSD_C_TRACEOUT, ">Get  FULL\n");
	u.u_error = ENOBUFS;
	return(NULL);
    }

    rcbp->rcb_refc = 1;
    rcbp->rcb_so = NULL;
    rcbp->rcb_lport = 0;
    bzero(rcbp->rcb_pswd, sizeof(rcbp->rcb_pswd));
    error = rdwri(UIO_READ, ip, (caddr_t)&rcbp->rcb_rl, sizeof(rcbp->rcb_rl), 0, 1, &resid);
    rcbp->rcb_flags = 0;
    if (error)
    {
 	rfs_printf2(RFSD_LOG|RFSD_C_TRACEOUT, "*Get  %d(name read)\n", error);
    free:
	rfsConnectionFree(rcbp);
	rcbp = NULL;
	iput(ip);
	goto out;
    }
    if (resid > sizeof(rcbp->rcb_rl)-sizeof(rcbp->rcb_addr))
    {
	u.u_error = EIO;
	rfs_printf2(RFSD_LOG|RFSD_C_TRACEOUT, "*Get  %d(link size)\n", u.u_error);
	goto free;
    }
    if (resid > sizeof(rcbp->rcb_rl)-sizeof(rcbp->rcb_addr)-sizeof(rcbp->rcb_fport) || rcbp->rcb_fport == 0)
	rcbp->rcb_fport = RFSPORT;
    if (resid > sizeof(rcbp->rcb_rl)-sizeof(rcbp->rcb_addr)-sizeof(rcbp->rcb_fport)-sizeof(rcbp->rcb_pvers))
	rcbp->rcb_pvers = 0;
    rcbp->rcb_ip = ip;
    /*
     *  Pre-calculate a value for adjustment of remote block device numbers
     *  so that they can never match local block device numbers and are
     *  unlikely to match remote ones.  This calulation yields an offset in
     *  the range [1,255-nblkdev] which is added to any remote block device
     *  number before passing it back to the application.
     */
    rcbp->rcb_majx = 
      (
	((rcbp->rcb_rl.rl_addr.s_addr&0xff0000)<<8)
	+
	(rcbp->rcb_rl.rl_addr.s_addr&0xff000000)
      )
      %
	(255-nblkdev)
      +
	1
      ;
    iunlock(ip);
out:
    rfs_printf2(RFSD_C_TRACEOUT, ">Get  rcb=%X\n", rcbp);
    return(rcbp);
}



/*
 *  rfsConnect - establish connection and perform initial connection protocol
 *
 *  rcbp = remote control block of connection to initialize
 *
 *  Open a connection to the remote host, send the initial connection message
 *  and handle the reply to initialize the remote connection block for this
 *  process and host.
 *
 *  Return: 0 or an error number as appropriate.
 */

rfsConnect(rcbp)
    register struct rfsConnectionBlock *rcbp;
{
    struct
    {
	struct rfsConnectMsg rcm;
	char rcmp[RFSMAXPSWD];
    } rcms;
    int error;
    register int l;

    rfs_printf2(RFSD_C_TRACEIN, "<Conn rcb=%X\n", rcbp);
    if (error = rfsEstablish(rcbp, rcbp->rcb_addr, rcbp->rcb_fport, 0, 1, RFSMAXRETRY))
    {
	rfs_printf2(RFSD_C_TRACEOUT, ">Conn %d(Estab)\n", error);
	return(error);
    }
    for (l=0; l<RFSMAXPSWD; l++)
	if ((rcms.rcmp[l]=rcbp->rcb_pswd[l]) == 0)
	    break;
    rcms.rcm.rcm_type = RFST_CONNECT | (l<<8);
    rcms.rcm.rcm_port = 0;
    rcms.rcm.rcm_uid  = u.u_uid;
    rcms.rcm.rcm_gid  = u.u_gid;
    rcms.rcm.rcm_ruid = u.u_ruid;
    rcms.rcm.rcm_rgid = u.u_rgid;
    rfsLock(rcbp);
    if (error = rfsSend(rcbp, (caddr_t)&rcms, sizeof (rcms.rcm)+l))
    {
	rfs_printf2(RFSD_C_TRACEOUT, ">Conn %d(Send)\n", error);
	rfsError(rcbp);
	goto out;
    }
    if (error = rfsReceive(rcbp, (caddr_t)&rcms, sizeof(rcms.rcm), RFST_CONNECT|RFST_REPLY))
    {
	rfs_printf2(RFSD_C_TRACEOUT, ">Conn %d(Recv)\n", error);
	rfsError(rcbp);
	goto out;
    }
    rfsUnLock(rcbp);
    if ((error=rcms.rcm.rcm_errno) == 0)
    {
	if (u.u_uid != rcms.rcm.rcm_uid || u.u_ruid != rcms.rcm.rcm_ruid ||
	    u.u_gid != rcms.rcm.rcm_gid || u.u_rgid != rcms.rcm.rcm_rgid)
	{
	    error = EPERM;
	}
	else
	    goto done;
    }

out:
    /*
     *  Release the remote connection so that another attempt will be made
     *  on the next operation.
     */
    rfsUnEstablish(rcbp);
done:
    rfs_printf2(RFSD_C_TRACEOUT, ">Conn %d\n", error);
    return(error);
}



/*
 *  rfsDetach - detach from remote connection block
 *
 *  rcbp = remote connection block
 *
 *  Decrement the reference count on the connection block.  On the last
 *  reference, zero the password (for security), release theinode, and release
 *  any associated remote connection.
 */

void
rfsDetach(rcbp)
    register struct rfsConnectionBlock *rcbp;
{
    rfs_printf3(RFSD_C_TRACEIN, "<Det  rcb=%X(%d)\n", rcbp, rcbp->rcb_refc);
    if (rcbp->rcb_refc == 1)
    {
	/*
	 *  Zero the password at the first opportunity.
	 */
	bzero(rcbp->rcb_pswd, sizeof(rcbp->rcb_pswd));
	if (rcbp->rcb_ip)
 	{
	    ilock(rcbp->rcb_ip);
	    iput(rcbp->rcb_ip);
	    rcbp->rcb_ip = NULL;
	}
	rfsUnEstablish(rcbp);
	rfsConnectionFree(rcbp);
	/* the connection block is now gone */
    }
    else
	rfsDecrCheck(rcbp);
    rfs_printf1(RFSD_C_TRACEOUT, ">Det\n");
}



/*
 *  rfsNameLength - calculate length of pathname 
 *
 *  Iterate through each character of the pathname specified in u.u_dirp to
 *  determine its length.  The name is always stored in the buffer allocated
 *  by namei() in system space.
 *
 *  Return: the length of the pathname (excluding the trailing byte) or -1 on
 *  error with u.u_error set as appropriate (although errors should longer be
 *  possible with the name in system space).
 */

rfsNameLength()
{
    register int len;
    char *dirp;

	return(strlen(u.u_nd.ni_dirp));
/*    dirp = u.u_dirp;
    for (len=0; schar() && u.u_error == 0; len++);
    u.u_dirp = dirp;

    if (u.u_error)
	return(-1);
    return(len);*/
}



/*
 *  rfsCopy - copy a remote connection block
 *
 *  rcbp = remote connection block to copy
 *
 *  Allocate a new remote connection block, replicating the password, remote
 *  address, local port, device number adjustment and inode fields of the old
 *  remote connection block.  The new connection block has a reference count of
 *  1 and no active remote connection.
 *
 *  Return: the new connection block or NULL if none can be allocated.
 */

struct rfsConnectionBlock *
rfsCopy(rcbp)
struct rfsConnectionBlock *rcbp;
{
    register struct rfsConnectionBlock *crcbp;

    rfs_printf2(RFSD_C_TRACEIN, "<Copy rcb=%X\n", rcbp);
    crcbp = rfsConnectionAllocate();
    if (crcbp == NULL)
    {
	rfs_printf1(RFSD_C_TRACEOUT, ">Copy FULL\n");
	return(NULL);
    }

    crcbp->rcb_refc = 1;
    crcbp->rcb_so = NULL;
    crcbp->rcb_majx = rcbp->rcb_majx;
    crcbp->rcb_lport = rcbp->rcb_lport;
    crcbp->rcb_rl = rcbp->rcb_rl;
    crcbp->rcb_flags = 0;	/* overlays rad */
    ilock(rcbp->rcb_ip);
    crcbp->rcb_ip = rcbp->rcb_ip;
#if	CS_ICHK
    iincr_chk(crcbp->rcb_ip);
#else	CS_ICHK
    crcbp->rcb_ip->i_count++;
#endif	CS_ICHK
    iunlock(rcbp->rcb_ip);
    bcopy(rcbp->rcb_pswd, crcbp->rcb_pswd, sizeof(rcbp->rcb_pswd));
    rfs_printf2(RFSD_C_TRACEOUT, ">Copy rcb=%X\n", crcbp);
    return(crcbp);
}



/*
 *  rfsLock - lock remote control block
 *
 *  rcbp = remote control block
 */

void
rfsLock(rcbp)
register struct rfsConnectionBlock *rcbp;
{
    while (rcbp->rcb_flags&RFSF_LOCKED)
    {
	rcbp->rcb_flags |= RFSF_WANTED;
	sleep((caddr_t)&(rcbp->rcb_flags), PZERO-1);
    }
    rcbp->rcb_flags |= RFSF_LOCKED;
}



/*
 *  rfsUnLock - unlock remote control block
 *
 *  rcbp = remote control block
 */

void
rfsUnLock(rcbp)
register struct rfsConnectionBlock *rcbp;
{
    if ((rcbp->rcb_flags&RFSF_LOCKED) == 0)
	panic("rfsUnLock");
    if (rcbp->rcb_flags&RFSF_WANTED)
	wakeup((caddr_t)&(rcbp->rcb_flags));
    rcbp->rcb_flags &= ~(RFSF_LOCKED|RFSF_WANTED);
}



/*
 *  rfsError - set error status for connection
 *
 *  rcbp = remote connection block
 *
 *  Prevent any further sends on the connection, unlock it if necessary, and
 *  remember that we encountered a protocol error.
 */

void
rfsError(rcbp)
register struct rfsConnectionBlock *rcbp;
{
    rfsShutdown(rcbp);
    if (rcbp->rcb_flags&RFSF_LOCKED)
	rfsUnLock(rcbp);
    rcbp->rcb_flags |= RFSF_ERROR;
}
#endif	CS_RFS
