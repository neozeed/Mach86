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
 *	@(#)sys_inode.c	6.13 (Berkeley) 8/4/85
 */
#if	CMU
/*
 **********************************************************************
 * HISTORY
 * 31-May-86  Avadis Tevanian (avie) at Carnegie-Mellon University
 *	Use the new pager_id field of the inode to call
 *	vm_object_uncache.
 *
 *  1-May-86  Avadis Tevanian (avie) at Carnegie-Mellon University
 *	Remove inode from the object cache if it is truncated.
 *
 * 03-Mar-86  Mike Accetta (mja) at Carnegie-Mellon University
 *	CS_COMPAT: log directory access on console if no
 *	controlling terminal.
 *
 * 16-Feb-86  Bill Bolosky (bolosky) at Carnegie-Mellon University
 *	Added Sailboat compatibility under switch ROMP.  Changes due to
 *	the fact that the IBM character-special drivers take an inode
 *	pointer as an extra parameter.
 *
 * 11-Feb-86  Mike Accetta (mja) at Carnegie-Mellon University
 *	CS_OLDFS:  changed ino_stat() to estimate block count for
 *	inodes from old-format file systems.
 *
 * 25-Feb-86  Avadis Tevanian (avie) at Carnegie-Mellon University
 *	Installed VM changes.
 *
 * 25-Jan-86  Avadis Tevanian (avie) at Carnegie-Mellon University
 *	Upgraded to 4.3.
 *
 * 29-Jan-86  Mike Accetta (mja) at Carnegie-Mellon University
 *	CS_COMPAT:  Added read directory logging in ino_rw().
 *
 * 12-Aug-85  Mike Accetta (mja) at Carnegie-Mellon University
 *	CS_COMPAT:  Changed ino_ioctl() to allow restarts unless old
 *	signal mechanism is in use without job-control.
 *	[V1(1)]
 *
 * 05-Aug-85  Mike Accetta (mja) at Carnegie-Mellon University
 *	CS_GENERIC:  Fix bug in rdwri() so that it only sets error EIO
 *	on a residual count when no other error was indicated.
 *	[V1(1)]
 *	
 * 29-Jul-85  Mike Accetta (mja) at Carnegie-Mellon University
 *	CS_COMPAT:  Added MPX hooks from 4.1 for now.
 *	[V1(1)]
 *
 * 18-Jul-85  Mike Accetta (mja) at Carnegie-Mellon University
 *	CS_XMOD:  Carried over FIOCXMOD and FIOCCNT calls and
 *	clearing of exclusive read (IXREAD) and write (IXWRITE) bits in
 *	the inode for final close of file table entry from which
 *	exclusive use mode was set.
 *	[V1(1)]
 *
 **********************************************************************
 */
 
#include "cs_compat.h"
#include "cs_generic.h"
#include "cs_oldfs.h"
#include "cs_xmod.h"
#include "mach_vm.h"
#endif	CMU

#include "param.h"
#include "systm.h"
#include "dir.h"
#include "user.h"
#include "inode.h"
#include "proc.h"
#include "fs.h"
#include "conf.h"
#include "buf.h"
#include "mount.h"
#include "file.h"
#include "uio.h"
#include "ioctl.h"
#include "tty.h"
#if	MACH_VM
#include "../vm/vm_pager.h"
#else	MACH_VM
#include "cmap.h"
#endif	MACH_VM
#include "stat.h"
#include "kernel.h"
#if	CS_COMPAT
#include "acct.h"
#endif	CS_COMPAT

int	ino_rw(), ino_ioctl(), ino_select(), ino_close();
struct 	fileops inodeops =
	{ ino_rw, ino_ioctl, ino_select, ino_close };

ino_rw(fp, rw, uio)
	struct file *fp;
	enum uio_rw rw;
	struct uio *uio;
{
	register struct inode *ip = (struct inode *)fp->f_data;
	int error;

	if ((ip->i_mode&IFMT) == IFREG) {
		ILOCK(ip);
		if (fp->f_flag&FAPPEND && rw == UIO_WRITE)
			uio->uio_offset = fp->f_offset = ip->i_size;
		error = rwip(ip, uio, rw);
		IUNLOCK(ip);
	} else
		error = rwip(ip, uio, rw);
#if	CS_COMPAT
	if ((ip->i_mode&IFMT) == IFDIR && (fp->f_flag&FDIROK) == 0 && (u.u_acflag&A42MODE) == 0)
	{
		if ((u.u_acflag&AREADIR) == 0)
		{
			if (u.u_modes&UMODE_DIRTRACE)
			{
				extern int uprintf(), printf();
				int (*print)() = uprintf;

				if (u.u_ttyp)
					print = uprintf;
				else
					print = printf;
				(*print)("[%s(%d) readdir 4.1]\n", u.u_comm,
					 u.u_procp->p_pid);
			}
			u.u_acflag |= AREADIR;
		}
	}
#endif	CS_COMPAT
	return (error);
}

rdwri(rw, ip, base, len, offset, segflg, aresid)
	struct inode *ip;
	caddr_t base;
	int len, offset, segflg;
	int *aresid;
	enum uio_rw rw;
{
	struct uio auio;
	struct iovec aiov;
	int error;

	auio.uio_iov = &aiov;
	auio.uio_iovcnt = 1;
	aiov.iov_base = base;
	aiov.iov_len = len;
	auio.uio_resid = len;
	auio.uio_offset = offset;
	auio.uio_segflg = segflg;
	error = rwip(ip, &auio, rw);
	if (aresid)
		*aresid = auio.uio_resid;
	else
		if (auio.uio_resid)
#if	CS_GENERIC
		    if (error == 0)
#endif	CS_GENERIC
			error = EIO;
	return (error);
}

rwip(ip, uio, rw)
	register struct inode *ip;
	register struct uio *uio;
	enum uio_rw rw;
{
	dev_t dev = (dev_t)ip->i_rdev;
	struct buf *bp;
	struct fs *fs;
	daddr_t lbn, bn;
	register int n, on, type;
	int size;
	long bsize;
	extern int mem_no;
	int error = 0;

	if (rw != UIO_READ && rw != UIO_WRITE)
		panic("rwip");
	if (rw == UIO_READ && uio->uio_resid == 0)
		return (0);
	if (uio->uio_offset < 0 &&
	    ((ip->i_mode&IFMT) != IFCHR || mem_no != major(dev)))
		return (EINVAL);
	if (rw == UIO_READ)
		ip->i_flag |= IACC;
	type = ip->i_mode&IFMT;
#if	CS_COMPAT
	if (type == IFCHR || type == IFMPC) {
#else	CS_COMPAT
	if (type == IFCHR) {
#endif	CS_COMPAT
		if (rw == UIO_READ)
#ifdef	romp
			error = (*cdevsw[major(dev)].d_read)(dev, uio, ip);
#else	romp
			error = (*cdevsw[major(dev)].d_read)(dev, uio);
#endif	romp
		else {
			ip->i_flag |= IUPD|ICHG;
#if	romp
			error = (*cdevsw[major(dev)].d_write)(dev, uio, ip);
#else	romp
			error = (*cdevsw[major(dev)].d_write)(dev, uio);
#endif	romp
		}
		return (error);
	}
	if (uio->uio_resid == 0)
		return (0);
	if (rw == UIO_WRITE && type == IFREG &&
	    uio->uio_offset + uio->uio_resid >
	      u.u_rlimit[RLIMIT_FSIZE].rlim_cur) {
		psignal(u.u_procp, SIGXFSZ);
		return (EFBIG);
	}
#if	CS_COMPAT
	if (type != IFBLK && type != IFMPB) {
#else	CS_COMPAT
	if (type != IFBLK) {
#endif	CS_COMPAT
		dev = ip->i_dev;
		fs = ip->i_fs;
		bsize = fs->fs_bsize;
	} else
		bsize = BLKDEV_IOSIZE;
#if	CS_COMPAT
	if (type == IFDIR)
		u.u_acflag |= AREADIR;
#endif	CS_COMPAT
	do {
		lbn = uio->uio_offset / bsize;
		on = uio->uio_offset % bsize;
		n = MIN((unsigned)(bsize - on), uio->uio_resid);
#if	CS_COMPAT
		if (type != IFBLK && type != IFMPB) {
#else	CS_COMPAT
		if (type != IFBLK) {
#endif	CS_COMPAT
			if (rw == UIO_READ) {
				int diff = ip->i_size - uio->uio_offset;
				if (diff <= 0)
					return (0);
				if (diff < n)
					n = diff;
			}
			bn = fsbtodb(fs,
			    bmap(ip, lbn, rw == UIO_WRITE ? B_WRITE: B_READ, (int)(on+n)));
			if (u.u_error || rw == UIO_WRITE && (long)bn<0)
				return (u.u_error);
			if (rw == UIO_WRITE && uio->uio_offset + n > ip->i_size &&
			   (type == IFDIR || type == IFREG || type == IFLNK))
				ip->i_size = uio->uio_offset + n;
			size = blksize(fs, ip, lbn);
		} else {
			bn = lbn * (BLKDEV_IOSIZE/DEV_BSIZE);
			rablock = bn + (BLKDEV_IOSIZE/DEV_BSIZE);
			rasize = size = bsize;
		}
		if (rw == UIO_READ) {
			if ((long)bn<0) {
				bp = geteblk(size);
				clrbuf(bp);
			} else if (ip->i_lastr + 1 == lbn)
				bp = breada(dev, bn, size, rablock, rasize);
			else
				bp = bread(dev, bn, size);
			ip->i_lastr = lbn;
		} else {
			int i, count, s;
			extern struct cmap *mfind();

#if	MACH_VM
			if (ip->pager_id != VM_PAGER_ID_NULL)
				vm_object_uncache(vm_pager_inode,
							ip->pager_id);
#else	MACH_VM
			count = howmany(size, DEV_BSIZE);
			s = splimp();
			for (i = 0; i < count; i += CLBYTES / DEV_BSIZE)
				if (mfind(dev, bn + i))
					munhash(dev, bn + i);
			splx(s);
#endif	MACH_VM
			if (n == bsize) 
				bp = getblk(dev, bn, size);
			else
				bp = bread(dev, bn, size);
		}
		n = MIN(n, size - bp->b_resid);
		if (bp->b_flags & B_ERROR) {
			error = EIO;
			brelse(bp);
			goto bad;
		}
		u.u_error =
		    uiomove(bp->b_un.b_addr+on, n, rw, uio);
		if (rw == UIO_READ) {
			if (n + on == bsize || uio->uio_offset == ip->i_size)
				bp->b_flags |= B_AGE;
			brelse(bp);
		} else {
			if ((ip->i_mode&IFMT) == IFDIR)
				bwrite(bp);
			else if (n + on == bsize) {
				bp->b_flags |= B_AGE;
				bawrite(bp);
			} else
				bdwrite(bp);
			ip->i_flag |= IUPD|ICHG;
			if (u.u_ruid != 0)
				ip->i_mode &= ~(ISUID|ISGID);
		}
	} while (u.u_error == 0 && uio->uio_resid > 0 && n != 0);
	if (error == 0)				/* XXX */
		error = u.u_error;		/* XXX */
bad:
	return (error);
}

ino_ioctl(fp, com, data)
	struct file *fp;
	register int com;
	caddr_t data;
{
	register struct inode *ip = ((struct inode *)fp->f_data);
	register int fmt = ip->i_mode & IFMT;
	dev_t dev;

#if	CS_COMPAT
	if (com == FIOCDIROK)
	{
		if (*((int *)data))
			fp->f_flag |= FDIROK;
		else
			fp->f_flag &= ~FDIROK;
		return(0);
	}
#endif	CS_COMPAT
#if	CS_XMOD
	if (com==FIOCXMOD)
	{
		int mode = *((int *)data);
		int error = 0;
		int bits;

		ilock(ip);
		switch (mode)
		{
		    case FXMNONE:
			if (fp->f_flag&FXMODE)
			{
				ip->i_flag &= ~(IXREAD|IXWRITE);
				fp->f_flag &= ~FXMODE;
			}
			else
			    error = EBADF;
			break;
		    case FXMWRITE:
			bits = IXWRITE;
			goto common;
		    case FXMUPD:
			bits = (IXREAD|IXWRITE);
		    common:
			if ((fp->f_flag&(FREAD|FWRITE)) < (mode+1))
			{
				error = EBADF;
				break;
			}
			if (fp->f_flag&FXMODE)
				ip->i_flag &= ~(IXREAD|IXWRITE);
			if (ip->i_flag&(IXREAD|IXWRITE))
			{
				error = EBUSY;
				break;
			}
			ip->i_flag |= bits;
			fp->f_flag |= FXMODE;
			break;
		    default:
			error = EINVAL;
		}
		iunlock(ip);
		return(error);
	}
	if (com==FIOCFCNT)
	{
		register struct file *ffp;
		register int *fcnt = (int *)data;
		int bits;

		fcnt[0] = fcnt[1] = fcnt[2] = 0;
		for(ffp=file; ffp < fileNFILE; ffp++)
			if (ffp->f_count && ffp->f_data == (caddr_t)ip)
			{
				if (bits=(ffp->f_flag&(FREAD|FWRITE)))
					fcnt[bits-1] += ffp->f_count;
			}
		return(0);
	}
#endif	CS_XMOD
	switch (fmt) {

	case IFREG:
	case IFDIR:
		if (com == FIONREAD) {
			*(off_t *)data = ip->i_size - fp->f_offset;
			return (0);
		}
		if (com == FIONBIO || com == FIOASYNC)	/* XXX */
			return (0);			/* XXX */
		/* fall into ... */

	default:
		return (ENOTTY);

	case IFCHR:
#if	CS_COMPAT
	case IFMPC:
#endif	CS_COMPAT
		dev = ip->i_rdev;
		u.u_r.r_val1 = 0;
#if	CS_COMPAT
		if (/*(u.u_procp->p_flag&SJCSIG == 0) &&*/ setjmp(&u.u_qsave)) {
#else	CS_COMPAT
		if (setjmp(&u.u_qsave)) {
#endif	CS_COMPAT
			if ((u.u_sigintr & sigmask(u.u_procp->p_cursig)) != 0)
				return(EINTR);
			u.u_eosys = RESTARTSYS;
			return (0);
		}
		return ((*cdevsw[major(dev)].d_ioctl)(dev, com, data,
		    fp->f_flag));
	}
}

ino_select(fp, which)
	struct file *fp;
	int which;
{
	register struct inode *ip = (struct inode *)fp->f_data;
	register dev_t dev;

	switch (ip->i_mode & IFMT) {

	default:
		return (1);		/* XXX */

	case IFCHR:
		dev = ip->i_rdev;
		return (*cdevsw[major(dev)].d_select)(dev, which);
	}
}

#ifdef notdef
ino_clone()
{

	return (EOPNOTSUPP);
}
#endif

ino_stat(ip, sb)
	register struct inode *ip;
	register struct stat *sb;
{

	ITIMES(ip, &time, &time);
	/*
	 * Copy from inode table
	 */
	sb->st_dev = ip->i_dev;
	sb->st_ino = ip->i_number;
#if	CS_COMPAT
	if (isoldlnk(ip))
		sb->st_mode = (ip->i_mode&~IFMT)|IFLNK;
	else
#endif	CS_COMPAT
	sb->st_mode = ip->i_mode;
	sb->st_nlink = ip->i_nlink;
	sb->st_uid = ip->i_uid;
	sb->st_gid = ip->i_gid;
	sb->st_rdev = (dev_t)ip->i_rdev;
	sb->st_size = ip->i_size;
	sb->st_atime = ip->i_atime;
	sb->st_spare1 = 0;
	sb->st_mtime = ip->i_mtime;
	sb->st_spare2 = 0;
	sb->st_ctime = ip->i_ctime;
	sb->st_spare3 = 0;
	/* this doesn't belong here */
	if ((ip->i_mode&IFMT) == IFBLK)
		sb->st_blksize = BLKDEV_IOSIZE;
	else if ((ip->i_mode&IFMT) == IFCHR)
		sb->st_blksize = MAXBSIZE;
	else
		sb->st_blksize = ip->i_fs->fs_bsize;
#if	CS_OLDFS
	if (isoldfs(ip->i_fs))
		sb->st_blocks = (sb->st_size + DEV_BSIZE - 1)/DEV_BSIZE;
	else
#endif	CS_OLDFS
	sb->st_blocks = ip->i_blocks;
	sb->st_spare4[0] = sb->st_spare4[1] = 0;
	return (0);
}

ino_close(fp)
	register struct file *fp;
{
	register struct inode *ip = (struct inode *)fp->f_data;
#ifdef romp
	register struct inode *okip = (struct inode *)fp->f_data;
#endif romp
	register struct mount *mp;
	int flag, mode;
	dev_t dev;
	register int (*cfunc)();

	if (fp->f_flag & (FSHLOCK|FEXLOCK))
		ino_unlock(fp, FSHLOCK|FEXLOCK);
	flag = fp->f_flag;
	dev = (dev_t)ip->i_rdev;
	mode = ip->i_mode & IFMT;
	ilock(ip);
#if	CS_XMOD
	if (flag & FXMODE)
		ip->i_flag &= ~(IXREAD|IXWRITE);
#endif	CS_XMOD
	iput(ip);
	fp->f_data = (caddr_t) 0;		/* XXX */
	switch (mode) {

	case IFCHR:
#if	CS_COMPAT
	case IFMPC:
#endif	CS_COMPAT
		cfunc = cdevsw[major(dev)].d_close;
		break;

	case IFBLK:
		/*
		 * We don't want to really close the device if it is mounted
		 */
/* MOUNT TABLE SHOULD HOLD INODE */
		for (mp = mount; mp < &mount[NMOUNT]; mp++)
			if (mp->m_bufp != NULL && mp->m_dev == dev)
				return;
#if	CS_COMPAT
	case IFMPB:
#endif	CS_COMPAT
		cfunc = bdevsw[major(dev)].d_close;
		break;

	default:
		return;
	}

	/*
	 * Check that another inode for the same device isn't active.
	 * This is because the same device can be referenced by
	 * two different inodes.
	 */
#if	CS_COMPAT
	if ((flag & FMP) == 0)
#endif	CS_COMPAT
	for (fp = file; fp < fileNFILE; fp++) {
		if (fp->f_type != DTYPE_INODE)		/* XXX */
			continue;
		if (fp->f_count && (ip = (struct inode *)fp->f_data) &&
		    ip->i_rdev == dev && (ip->i_mode&IFMT) == mode)
			return;
	}
	if (mode == IFBLK) {
		/*
		 * On last close of a block device (that isn't mounted)
		 * we must invalidate any in core blocks, so that
		 * we can, for instance, change floppy disks.
		 */
		bflush(dev);
		binval(dev);
	}
	if (setjmp(&u.u_qsave)) {
		/*
		 * If device close routine is interrupted,
		 * must return so closef can clean up.
		 */
		if (u.u_error == 0)
			u.u_error = EINTR;	/* ??? */
		return;
	}
#ifdef 	romp
	(*cfunc)(dev, flag, fp, okip);
#else	romp
#if	CS_COMPAT
	/*
	 *  This is used only by mxclose() so the file pointer is still intact
	 *  here (because of the FMP check above).  I wonder about the
	 *  preceding romp line, though.
	 */
	(*cfunc)(dev, flag, fp);
#else	CS_COMPAT
	(*cfunc)(dev, flag);
#endif	CS_COMPAT
#endif	romp
}

/*
 * Place an advisory lock on an inode.
 */
ino_lock(fp, cmd)
	register struct file *fp;
	int cmd;
{
	register int priority = PLOCK;
	register struct inode *ip = (struct inode *)fp->f_data;

	if ((cmd & LOCK_EX) == 0)
		priority += 4;
	if (setjmp(&u.u_qsave)) {
		if ((u.u_sigintr & sigmask(u.u_procp->p_cursig)) != 0)
			return(EINTR);
		u.u_eosys = RESTARTSYS;
		return (0);
	}
	/*
	 * If there's a exclusive lock currently applied
	 * to the file, then we've gotta wait for the
	 * lock with everyone else.
	 */
again:
	while (ip->i_flag & IEXLOCK) {
		/*
		 * If we're holding an exclusive
		 * lock, then release it.
		 */
		if (fp->f_flag & FEXLOCK) {
			ino_unlock(fp, FEXLOCK);
			continue;
		}
		if (cmd & LOCK_NB)
			return (EWOULDBLOCK);
		ip->i_flag |= ILWAIT;
		sleep((caddr_t)&ip->i_exlockc, priority);
	}
	if ((cmd & LOCK_EX) && (ip->i_flag & ISHLOCK)) {
		/*
		 * Must wait for any shared locks to finish
		 * before we try to apply a exclusive lock.
		 *
		 * If we're holding a shared
		 * lock, then release it.
		 */
		if (fp->f_flag & FSHLOCK) {
			ino_unlock(fp, FSHLOCK);
			goto again;
		}
		if (cmd & LOCK_NB)
			return (EWOULDBLOCK);
		ip->i_flag |= ILWAIT;
		sleep((caddr_t)&ip->i_shlockc, PLOCK);
		goto again;
	}
	if (fp->f_flag & FEXLOCK)
		panic("ino_lock");
	if (cmd & LOCK_EX) {
		cmd &= ~LOCK_SH;
		ip->i_exlockc++;
		ip->i_flag |= IEXLOCK;
		fp->f_flag |= FEXLOCK;
	}
	if ((cmd & LOCK_SH) && (fp->f_flag & FSHLOCK) == 0) {
		ip->i_shlockc++;
		ip->i_flag |= ISHLOCK;
		fp->f_flag |= FSHLOCK;
	}
	return (0);
}

/*
 * Unlock a file.
 */
ino_unlock(fp, kind)
	register struct file *fp;
	int kind;
{
	register struct inode *ip = (struct inode *)fp->f_data;
	int flags;

	kind &= fp->f_flag;
	if (ip == NULL || kind == 0)
		return;
	flags = ip->i_flag;
	if (kind & FSHLOCK) {
		if ((flags & ISHLOCK) == 0)
			panic("ino_unlock: SHLOCK");
		if (--ip->i_shlockc == 0) {
			ip->i_flag &= ~ISHLOCK;
			if (flags & ILWAIT)
				wakeup((caddr_t)&ip->i_shlockc);
		}
		fp->f_flag &= ~FSHLOCK;
	}
	if (kind & FEXLOCK) {
		if ((flags & IEXLOCK) == 0)
			panic("ino_unlock: EXLOCK");
		if (--ip->i_exlockc == 0) {
			ip->i_flag &= ~(IEXLOCK|ILWAIT);
			if (flags & ILWAIT)
				wakeup((caddr_t)&ip->i_exlockc);
		}
		fp->f_flag &= ~FEXLOCK;
	}
}

/*
 * Openi called to allow handler
 * of special files to initialize and
 * validate before actual IO.
 */
openi(ip, mode)
	register struct inode *ip;
{
	dev_t dev = (dev_t)ip->i_rdev;
	register int maj = major(dev);

	switch (ip->i_mode&IFMT) {

	case IFCHR:
#if	CS_COMPAT
	case IFMPC:
#endif	CS_COMPAT
		if ((u_int)maj >= nchrdev)
			return (ENXIO);
#ifdef	romp
		return ((*cdevsw[maj].d_open)(dev, mode, ip));
#else	romp
		return ((*cdevsw[maj].d_open)(dev, mode));
#endif	romp

	case IFBLK:
#if	CS_COMPAT
	case IFMPB:
#endif	CS_COMPAT
		if ((u_int)maj >= nblkdev)
			return (ENXIO);
		return ((*bdevsw[maj].d_open)(dev, mode));
	}
	return (0);
}

/*
 * Revoke access the current tty by all processes.
 * Used only by the super-user in init
 * to give ``clean'' terminals at login.
 */
vhangup()
{

	if (!suser())
		return;
	if (u.u_ttyp == NULL)
		return;
	forceclose(u.u_ttyd);
	if ((u.u_ttyp->t_state) & TS_ISOPEN)
		gsignal(u.u_ttyp->t_pgrp, SIGHUP);
}

forceclose(dev)
	dev_t dev;
{
	register struct file *fp;
	register struct inode *ip;

	for (fp = file; fp < fileNFILE; fp++) {
		if (fp->f_count == 0)
			continue;
		if (fp->f_type != DTYPE_INODE)
			continue;
		ip = (struct inode *)fp->f_data;
		if (ip == 0)
			continue;
		if ((ip->i_mode & IFMT) != IFCHR)
			continue;
		if (ip->i_rdev != dev)
			continue;
		fp->f_flag &= ~(FREAD|FWRITE);
	}
}
