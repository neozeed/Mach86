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
/*	rfs_descrip.c	CMU	82/01/20	*/

/*
 *  Remote file system - file descriptor based operations module
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
#include "../h/inode.h"
#include "../h/user.h"
#include "../h/map.h"
#include "../h/proc.h"
#include "../h/uio.h"
#include "../h/stat.h"
#include "../h/rfs.h"



/*
 *  rfs_rw - common read/write call handling
 *
 *  fp   = file pointer
 *  rw   = read/write flag
 *  uiop = user I/O descriptor
 *
 *  Return: 0 on success or an error number as appropriate.
 */

rfs_rw(fp, rw, uiop)
    struct file *fp;
    enum uio_rw rw;
    struct uio *uiop;
{
    int error;

    rfs_printf3(RFSD_S_TRACEIN, "<RW   %s(f=%X)\n", syscallnames[u.u_rfscode], fp);
    error = rfsRdWr((struct rfsConnectionBlock *)fp->f_data, uiop, (rw == UIO_READ)?RFST_READ:RFST_WRITE);
    rfs_printf2(RFSD_S_TRACEOUT, ">RW   %d\n", error);
    return(error);
}



/*
 *  rfsRdWr - perform remote read/write operation
 *
 *  rcbp = remote connection block of file
 *  uiop = user I/O descriptor
 *  type = read/write message type
 *
 *  Send the appropriate read or write message and process the reply.
 *
 *  Return: 0 on success with the user I/O descriptor updated or an error
 *  number as appropriate.
 */

rfsRdWr(rcbp, uiop, type)
    register struct rfsConnectionBlock *rcbp;
    struct uio *uiop;
    int type;
{
    label_t lqsave;
    struct rfsRWMsg rrwm;
    unsigned count;
    int error;

    rrwm.rrwm_type = type;
    rrwm.rrwm_count = count = uiop->uio_resid;
    rfsLock(rcbp);
    if (error = rfsSend(rcbp, (caddr_t)&rrwm, sizeof (rrwm)))
    {
	rfsError(rcbp);
	return(error);
    }
    if (type == RFST_READ)
    {
	if (error = rfsReceive(rcbp, (caddr_t)&rrwm, sizeof (rrwm), RFST_REPLY|RFST_READ))
	{
	    rfsError(rcbp);
	    return(error);
	}
	if (rrwm.rrwm_errno)
	{
	    rfsUnLock(rcbp);
	    return(error);
	}
	bcopy((caddr_t)&u.u_qsave, (caddr_t)&lqsave, sizeof(label_t));
	while (rrwm.rrwm_count)
	{
	    unsigned ucount;

	    ucount = uiop->uio_resid;
	    if (setjmp(&u.u_qsave))
		;
	    error = soreceive(rcbp->rcb_so, (struct mbuf **)0, uiop, 0, (struct mbuf **)0);
	    if (error)
		break;
	    rrwm.rrwm_count -= ucount-uiop->uio_resid;
	}
	bcopy((caddr_t)&lqsave, (caddr_t)&u.u_qsave, sizeof(label_t));
    }
    else
    {
	bcopy((caddr_t)&u.u_qsave, (caddr_t)&lqsave, sizeof(label_t));
	if (setjmp(&u.u_qsave))
	    ;
	error = sosend(rcbp->rcb_so, (struct mbuf *)0, uiop, 0, (struct mbuf *)0);
	bcopy((caddr_t)&lqsave, (caddr_t)&u.u_qsave, sizeof(label_t));
	if (error)
	{
	    rfsError(rcbp);
	    rfs_printf2(RFSD_PROTOCOL, "*RdWr %d\n", error);
	    return(error);
	}
	if (error = rfsReceive(rcbp, (caddr_t)&rrwm, sizeof(rrwm), RFST_REPLY|RFST_WRITE))
	{
	    rfsError(rcbp);
	    rfs_printf2(RFSD_PROTOCOL, "*RdWr %d\n", error);
	    return(error);
	}
	error = rrwm.rrwm_errno;
	uiop->uio_resid = count - rrwm.rrwm_count;
    }
    rfsUnLock(rcbp);
    return(error);
}



/*
 *  rfs_ioctl - remote ioctl() call
 *
 *  fp   = file pointer
 *  cmd  = ioctl command to execute
 *  data = pointer to argument block
 *
 *  Return: 0 on success or an error number as appropriate.
 */

rfs_ioctl(fp, cmd, data)
    struct file *fp;
    int cmd;
    caddr_t data;
{
    struct rfsConnectionBlock *rcbp = (struct rfsConnectionBlock *)fp->f_data;
    struct rfsIoctlMsg rim;
    int error;

    rfs_printf3(RFSD_S_TRACEIN, "<Ioct %s(f=%X)\n", syscallnames[u.u_rfscode], fp);
    rim.rim_type = RFST_IOCTL;
    rim.rim_cmd  = cmd;
    rim.rim_argp = data;
    bcopy(data, (caddr_t)&rim.rim_arg[0], sizeof(rim.rim_arg));
    rfsLock(rcbp);
    if (error = rfsSend(rcbp, (caddr_t)&rim, sizeof (rim)))
    {
	rfsError(rcbp);
	goto out;
    }
    if (error = rfsReceive(rcbp, (caddr_t)&rim, sizeof (rim), RFST_REPLY|RFST_IOCTL))
    {
	rfsError(rcbp);
	goto out;
    }
    rfsUnLock(rcbp);
    if ((error = rim.rim_errno) == 0)
    {
	u.u_r.r_val1 = rim.rim_rval;
	bcopy((caddr_t)&rim.rim_arg[0], (caddr_t)data, sizeof (rim.rim_arg));
    }
out:
    rfs_printf2(RFSD_S_TRACEOUT, ">Ioct %d", error);
    return(error);
}



/*
 *  rfs_select - remote select() call
 *
 *  fp   = file pointer
 *  flag = read/write flag
 *
 *  Return: TRUE always (for now).
 *
 *  TODO:  finish this (somehow)
 */

/* ARGSUSED */
rfs_select(fp, flag)
    struct file *fp;
    int flag;
{
    return(1);
}



/*
 *  rfs_close - remote close() call
 *
 *  fp = file pointer
 */

rfs_close(fp)
    struct file *fp;
{
    rfs_printf3(RFSD_S_TRACEIN, "<Clse %s(f=%X)\n", syscallnames[u.u_rfscode], fp);
    rfsDetach((struct rfsConnectionBlock *)fp->f_data);
    rfs_printf1(RFSD_S_TRACEOUT, ">Clse");
}



/*
 *  Remote file descriptor operations dispatch table.
 *
 *  This is the table used by most standard file descriptor based system calls
 *  to dispatch to the proper processing routines (above) when the descriptor
 *  type is DTYPE_RFSINO.
 */

struct 	fileops rfsops =
	{ rfs_rw, rfs_ioctl, rfs_select, rfs_close };




/*
 *  rfs_open - remote open() call
 *
 *  rcbp = remote connection block
 *
 *  Return: NULL always with u.u_error set as appropriate.
 */

struct inode *
rfs_open(rcbp)
    register struct rfsConnectionBlock *rcbp;
{
    struct a {
	char	*fname;
	int	mode;
	int	crtmode;
    } *uap = (struct a *) u.u_ap;

    return(rfs_copen(rcbp, RFST_OPEN, uap->mode, uap->mode-FOPEN));
}



/*
 *  rfs_creat - remote creat() call
 *
 *  rcbp = remote connection block
 *
 *  Return: NULL always with u.u_error set as appropriate.
 */

struct inode *
rfs_creat(rcbp)
    register struct rfsConnectionBlock *rcbp;
{
    struct a {
	char	*fname;
	int	fmode;
    } *uap = (struct a *)u.u_ap;

    return(rfs_copen(rcbp, RFST_CREAT, uap->fmode, FWRITE));
}



/*
 *  rfs_copen - common code for open and creat
 *
 *  rcbp  = remote connection block
 *  type = open/creat type
 *  mode = open/creat mode
 *  rw   = file mode to assign to allocated file descriptor
 *
 *  Perform the remote open or create operation and if successfull allocate a
 *  file descriptor to record the remote connection for the now open file.
 *
 *  Return: NULL always with u.u_error set as appropriate.
 */

struct inode *
rfs_copen(rcbp, type, mode, rw)
    register struct rfsConnectionBlock *rcbp;
    short type;
    short mode;
    int rw;
{
    register struct file *fp;

    rcbp = rfsOpCr(rcbp, type, mode);
    if (rcbp != NULL)
    {
	fp = u.u_ofile[u.u_r.r_val1];
	fp->f_flag = rw&FMASK;
	fp->f_type = DTYPE_RFSINO;
	fp->f_ops  = &rfsops;
	fp->f_offset = 0;
	fp->f_data = (caddr_t)rcbp;
    }
    return(NULL);
}



/*
 *  rfsOpCr - perfrom remote open or creatre operation
 *
 *  rcbp  = remote connection block
 *  type = message type to send
 *  mode = mode to send in open message
 *
 *  Send the appropriate open or create message on the remote connection and
 *  create a new connection to use thereafter for the newly open file.
 *  The password field is zeroed in the new connection block since it won't
 *  be needed on that connection and it would be cumbersome to track the
 *  connection done later if we needed to zero it (such as before a reboot).
 *
 *  Return: the new remote connection block or NULL on error with u.u_error set
 *  as appropriate.
 */

struct rfsConnectionBlock *
rfsOpCr(rcbp, type, mode)
    register struct rfsConnectionBlock *rcbp;
    short type;
    short mode;
{
    register struct rfsConnectionBlock *frcbp = NULL;
    register int len;
    struct rfsOpenMsg rom;
    int error;

    rfs_printf3(RFSD_P_TRACEIN, "<OpCr t=%d,m=%o\n", type, mode);
    rom.rom_type = type;
    rom.rom_mode = mode;
    len = rfsNameLength();
    if (len < 0)
    {
	rfs_printf2(RFSD_P_TRACEOUT, ">OpCr %d\n", u.u_error);
	return(NULL);
    }
    if ((frcbp = rfsCopy(rcbp)) == NULL)
    {
	rfs_printf1(RFSD_P_TRACEOUT, "*OpCr (Copy)\n");
	error = ENOBUFS;
	goto out;
    }
    bzero(frcbp->rcb_pswd, sizeof(frcbp->rcb_pswd));
    rom.rom_count = len;
    rfsLock(rcbp);
    error = rfsSend(rcbp, (caddr_t)&rom, sizeof(rom));
    if (error)
	goto fail;
    error = rfsSendName(rcbp, len);
    if (error)
	goto fail;
    rfsUnLock(rcbp);
    rcbp = frcbp;
    error = rfsEstablish(rcbp, rcbp->rcb_addr, 0, rcbp->rcb_lport, 0, RFSMAXRETRY);
    if (error)
	goto out;
    error = rfsReceive(rcbp, (caddr_t)&rom, sizeof (rom), RFST_REPLY|rom.rom_type);
    if (error)
	goto out;
    if (error = rom.rom_errno)
	goto out;
    rfs_printf2(RFSD_P_TRACEOUT, ">OpCr rcb=%X\n", rcbp);
    return(rcbp);

fail:
    rfsError(rcbp);
out:
    if (frcbp)
	rfsDetach(frcbp);
    u.u_error = error;
    rfs_printf2(RFSD_P_TRACEOUT, ">OpCr %d\n", error);
    return(NULL);
}



/*
 *  rfs_finode - non fileops file descriptor call processing
 *
 *  fp = file descriptor
 *
 *  Dispatch through namei dispatch table to appropriate processing routine.
 *
 *  Return: NULL (always) with u.u_error set as appropriate.
 */

struct file *
rfs_finode(fp)
    struct file *fp;
{
    rfs_printf3(RFSD_S_TRACEIN, "<Fino %s(f=%X)\n", syscallnames[u.u_rfscode], fp);
    (void)(*(rfs_sysent[u.u_rfscode]))((struct rfsConnectionBlock *)fp->f_data);
    if (u.u_error == 0)
	u.u_error = EREMOTE;
    rfs_printf2(RFSD_S_TRACEOUT, ">Fino %d\n", u.u_error);
    return(NULL);
}



/*
 *  rfs_lseek - remote lseek() call
 *
 *  rcbp = remote control block
 *
 *  Return: NULL (always) with u.u_error set as appropriate.
 */

struct inode *
rfs_lseek(rcbp)
register struct rfsConnectionBlock *rcbp;
{
    register struct a {
	int	fd;
	off_t	off;
	int	sbase;
    } *uap = (struct a *)u.u_ap;

    u.u_error = rfsLseek(rcbp, uap->off, uap->sbase, &u.u_r.r_off);
    return(NULL);
}



/*
 *  rfsLseek - perform remote lseek operation
 *
 *  rcbp  = remot connectionblcok
 *  off   = offset to seek to
 *  sbase = base for seek
 *  offp  = pointer for returned offset value
 *
 *  Perform the remote lseek operation and return its result in the supplied
 *  pointer.
 *
 *  Return: 0 or an error number as appropriate.
 */

rfsLseek(rcbp, off, sbase, offp)
struct rfsConnectionBlock *rcbp;
off_t off;
int sbase;
off_t *offp;
{
    struct rfsSeekMsg rsm;
    int error;

    rsm.rsm_type = RFST_SEEK;
    rsm.rsm_off  = off;
    rsm.rsm_sbase = sbase;
    rfsLock(rcbp);
    if (error = rfsSend(rcbp, (caddr_t)&rsm, sizeof (rsm)))
    {
	rfsError(rcbp);
	goto fail;
    }
    if (error = rfsReceive(rcbp, (caddr_t)&rsm, sizeof (rsm), RFST_REPLY|RFST_SEEK))
    {
	rfsError(rcbp);
	goto fail;
    }
    rfsUnLock(rcbp);
    if ((error = rsm.rsm_errno) == 0)
	*offp = rsm.rsm_off;
fail:
    return(error);
}



/*
 *  rfs_fstat - remote fstat() call
 *
 *  fp  = file pointer
 *
 *  N.B.  This call is an explicit hook from fstat().
 *
 *  Return: 0 or an error number as appropriate.
 */

rfs_fstat(fp)
struct file *fp;
{
    register struct a {
	    int	fdes;
	    struct	stat *sb;
    } *uap = (struct a *)u.u_ap;

    rfs_printf3(RFSD_S_TRACEIN, "<SysC %s(f=%X)\n", syscallnames[u.u_rfscode], fp);
    (void) rfs_stat1((struct rfsConnectionBlock *)fp->f_data, uap->sb, RFST_NFSTAT, 0);
    rfs_printf2(RFSD_S_TRACEOUT, ">SysC %d", u.u_error);
    return(u.u_error);
}



/*
 *  rfs_ofstat - remote ofstat() call
 *
 *  rcbp = remote connection blcok
 *
 *  Return: NULL (always) with u.u_error set as appropriate.  
 */

struct inode *
rfs_ofstat(rcbp)
struct rfsConnectionBlock *rcbp;
{
    register struct a {
	    int	fdes;
	    struct	stat *sb;
    } *uap = (struct a *)u.u_ap;

    return(rfs_stat1(rcbp, uap->sb, RFST_FSTAT, sizeof(struct stat)-32));
}



/*
 *  rfsC_fstat - remote control fstat() call
 *
 *  fp  = file pointer
 *
 *  N.B.  This call is an explicit hook from fstat().
 *
 *  Return: EINVAL (always) for now.
 */

rfsC_fstat(fp)
struct file *fp;
{
    register struct a {
	    int	fdes;
	    struct	stat *sb;
    } *uap = (struct a *)u.u_ap;

    return(EINVAL);
}
#endif	CS_RFS
