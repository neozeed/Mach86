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
 *	@(#)ufs_mount.c	6.12 (Berkeley) 9/4/85
 */
#if	CMU
/*
 **********************************************************************
 * HISTORY
 * 21-May-86  David Golub (dbg) at Carnegie-Mellon University
 *	Call vm_object_cache_clear to remove any cached objects that
 *	might be on device being dismounted.
 *
 * 21-Apr-86  Mike Accetta (mja) at Carnegie-Mellon University
 *	CS_OLDFS:  fix mountfs() to also set fs_fsize to BSIZE.
 *
 * 22-Mar-86  Bill Bolosky (bolosky) at Carnegie-Mellon University
 *	Added a CS_OLDFS #if around a piece of code which called isoldfs
 *	and it's then clause.
 *
 * 25-Feb-86  Avadis Tevanian (avie) at Carnegie-Mellon University
 *	Installed VM changes.
 *
 * 26-Jan-86  Avadis Tevanian (avie) at Carnegie-Mellon University
 *	Upgraded to 4.3.
 *
 * 03-Aug-85  Mike Accetta (mja) at Carnegie-Mellon University
 *	CS_RPAUSE:  Changed to initialize file system low water mark
 *	fields on mount.
 *
 * 18-Jun-85  Mike Accetta (mja) at Carnegie-Mellon University
 *	CS_OLDFS:  changed fsmount(), unmount1() and sbupdate() to
 *	correctly process old-style file systems and smount() to also
 *	maintain the mounted-on name in the old-style super-block
 *	buffer.
 *	[V1(1)]
 *
 **********************************************************************
 */
 
#include "cs_oldfs.h"
#include "cs_rpause.h"
#include "mach_vm.h"
#endif	CMU

#include "param.h"
#include "systm.h"
#include "dir.h"
#include "user.h"
#include "inode.h"
#include "proc.h"
#include "fs.h"
#include "buf.h"
#include "mount.h"
#include "file.h"
#include "conf.h"
#if	CS_OLDFS
#include "filsys.h"
#endif	CS_OLDFS

#if	MACH_VM
#include "../vm/vm_kern.h"
#include "../vm/vm_object.h"
#endif	MACH_VM

smount()
{
	register struct a {
		char	*fspec;
		char	*freg;
		int	ronly;
	} *uap = (struct a *)u.u_ap;
	dev_t dev;
	register struct inode *ip;
	register struct fs *fs;
#if	CS_OLDFS
	register char *ocp, *cp;
#endif	CS_OLDFS
	register struct nameidata *ndp = &u.u_nd;
	u_int len;

	u.u_error = getmdev(&dev, uap->fspec);
	if (u.u_error)
		return;
	ndp->ni_nameiop = LOOKUP | NOCACHE | FOLLOW;
	ndp->ni_segflg = UIO_USERSPACE;
	ndp->ni_dirp = (caddr_t)uap->freg;
	ip = namei(ndp);
	if (ip == NULL)
		return;
	if (ip->i_count != 1) {
		iput(ip);
		u.u_error = EBUSY;
		return;
	}
	if ((ip->i_mode&IFMT) != IFDIR) {
		iput(ip);
		u.u_error = ENOTDIR;
		return;
	}
	fs = mountfs(dev, uap->ronly, ip);
	if (fs == 0)
		return;
	(void) copyinstr(uap->freg, fs->fs_fsmnt, sizeof(fs->fs_fsmnt)-1, &len);
	bzero(fs->fs_fsmnt + len, sizeof (fs->fs_fsmnt) - len);
#if	CS_OLDFS
	if (isoldfs(fs))
	/*
	 * Copy into the old style name.
	 */
		(void) bcopy(fs->fs_oldfs->s_fsmnt, fs->fs_fsmnt, len+1);
#endif	CS_OLDFS
}

/* this routine has races if running twice */
struct fs *
mountfs(dev, ronly, ip)
	dev_t dev;
	int ronly;
	struct inode *ip;
{
	register struct mount *mp = 0;
	struct buf *tp = 0;
	register struct buf *bp = 0;
	register struct fs *fs;
	int blks;
	caddr_t space;
	int i, size;
	register error;
	int needclose = 0;

	error =
	    (*bdevsw[major(dev)].d_open)(dev, ronly ? FREAD : FREAD|FWRITE);
	if (error)
		goto out;
	needclose = 1;
	tp = bread(dev, SBLOCK, SBSIZE);
	if (tp->b_flags & B_ERROR)
		goto out;
	for (mp = &mount[0]; mp < &mount[NMOUNT]; mp++)
		if (mp->m_bufp != 0 && dev == mp->m_dev) {
			mp = 0;
			error = EBUSY;
			goto out;
		}
	for (mp = &mount[0]; mp < &mount[NMOUNT]; mp++)
		if (mp->m_bufp == 0)
			goto found;
	mp = 0;
	error = EMFILE;		/* needs translation */
	goto out;
found:
	mp->m_bufp = tp;	/* just to reserve this slot */
	mp->m_dev = NODEV;
	fs = tp->b_un.b_fs;
	if (fs->fs_magic != FS_MAGIC || fs->fs_bsize > MAXBSIZE
	    || fs->fs_bsize < sizeof(struct fs)) {
#if	CS_OLDFS
	/*
	 *  This code is a bit too trusting.  We may actually be mounting an
	 *  old-style file system (or even a garbage one) and the super-block
	 *  size field is not to be immediately believed.  If the test below
	 *  (which we mimic here) is going to consider this an old-style file
	 *  system or the super-block size is otherwise ridiculous, give it a
	 *  maximal size so that the buffer allocation code between here and
	 *  there doesn't blow up.  Since we do an extra consistency check here
	 *  that won't be repeated below, assume that this is at best an old-
	 *  style file system if any of these consistency checks trigger.
	 */
		fs->fs_magic = 0;
		fs->fs_sbsize = SBSIZE;
		/*
		 *  We just changed a few fields in the file system structure
		 *  buffer whose storage actually overlaps the i-list in an
		 *  old-style file system.  Invalidate this buffer so that this
		 *  change disappears when the buffer is released.
		 */
		tp->b_flags |= B_INVAL;
#else	CS_OLDFS
		error = EINVAL;		/* also needs translation */
		goto out;
#endif	CS_OLDFS
	}
	bp = geteblk((int)fs->fs_sbsize);
	mp->m_bufp = bp;
	bcopy((caddr_t)tp->b_un.b_addr, (caddr_t)bp->b_un.b_addr,
	   (u_int)fs->fs_sbsize);
	brelse(tp);
	tp = 0;
	fs = bp->b_un.b_fs;
#if	CS_OLDFS
	if (fs->fs_magic != FS_MAGIC || fs->fs_bsize > MAXBSIZE)
	/*
	 *  If either of these checks fail (the first possibly because the
	 *  magic value was cleared above), we may have an old-style file
	 *  system on our hands.  To handle these, we maintain a minimal file
	 *  system structure as usual with most fields initialized to zero to
	 *  catch unsuspected uses and a few initialized to appropriate values
	 *  for the old-style file system such that all the necessary standard
	 *  file system macros do the right thing.  We use the very end of the
	 *  allocated file system structure buffer (which is much larger than
	 *  it need be for old-style file systems) for the old-style super-
	 *  block and record a pointer to it in a spare field of the file
	 *  system structure for easy future reference.
	 */
	{
		register struct filsys *fp;

		/*
		 *  Clear the entire file system structure since whatever we
		 *  just read from the device is random garbage.
		 */
		bzero((caddr_t)fs, SBSIZE);
		/*
		 *  Initialize the various file system contants so that we can
		 *  use the standard file system macros (in particular fstodb()
		 *  below).
		 */
		fs->fs_sbsize = SBSIZE;	/* as above */
		fs->fs_bsize = BSIZE;
		fs->fs_bmask = ~(BSIZE-1);
		fs->fs_bshift = 10;	/* LOG2(BSIZE) */
		fs->fs_fsbtodb = 1;	/* LOG2(BSIZE/DEV_BSIZE) */
		fs->fs_nindir = BSIZE/sizeof(daddr_t);
		fs->fs_inopb = BSIZE/sizeof(struct oinode);
		/*
		 *  Read the old-style super-block from the correct device
		 *  address and verify that it makes minimal sense.  Since the
		 *  buffer cache now consists of variable length blocks, it is
		 *  no longer possible to permanently lock the super-block
		 *  directly into the cache.  If we did this, subsequent I/O
		 *  operations with different block sizes (e.g. any non-BSIZE
		 *  operations on the block device) would attempt to reallocate
		 *  the buffer size and destroy the LOCKED status.  It might be
		 *  possible to fix this in the buffer manipulation routines
		 *  but since the new-style file system already has this
		 *  problem, we can live with it in the old-style file system
		 *  as well.
		 */
		tp = bread(dev, fsbtodb(fs, SUPERB), BSIZE);
		if(tp->b_flags & B_ERROR)
			goto out;
#ifndef	lint
		if (sizeof(*fs) > (SBSIZE-BSIZE))	/* sanity */
			panic("fs<<fp");
#endif	lint
		fp = (struct filsys *)&(bp->b_un.b_addr)[SBSIZE-BSIZE];
		bcopy((caddr_t)tp->b_un.b_addr, (caddr_t)fp, BSIZE);
		if (fp->s_nfree > NICFREE || fp->s_ninode > NICINOD)
			goto out;
		brelse(tp);
		tp = 0;
		/*
		 *  Finish file system structure initialization.  The read-
		 *  only, and modification flags are maintained only in
		 *  the file system structure since this is where the rest of
		 *  the system expects them.  The file system size, and
		 *  modification time fields are maintained in parallel since
		 *  some standard file system routines may expect to find them
		 *  in the file system structure.  The others are only used by
		 *  the old-style file system routines.  Hereafter, the
		 *  isoldfs() macro will distinguish between old and new style
		 *  file systems according to the whether or not the old-style
		 *  super-block pointer is non-null.
		 */
		fs->fs_oldfs = fp;
		fs->fs_magic = FS_MAGIC;	/* keep getfs() happy */
		fs->fs_fsize = BSIZE;		/* for direnter() and ??? */
		fs->fs_ronly = (ronly != 0);	/* for read-only checks */
		fs->fs_fmod = (ronly == 0);	/* for update() */
		fs->fs_size = fp->s_fsize;	/* for badblock() and ??? */
		fs->fs_dsize = fp->s_fsize-
			       fp->s_isize;	/* for freespace() and ??? */
		fs->fs_minfree = 0;		/* for freespace() and ??? */
		fs->fs_time = fp->s_time;	/* for time setup */
		fs->fs_ncg = 1;			/* for inode water marks */
		fs->fs_ipg = (fp->s_isize-SUPERB-1)*INOPB(fs);	/* for inode water marks */
		fp->s_ilock = 0;
		fp->s_flock = 0;
		fp->s_nbehind = 0;
		fp->s_lasti = 1;
	}
	else
	{
	/*
	 *  The consistency checks determined this to be a new-style file
	 *  system, so make sure to clear the old-style super-block pointer
	 *  since it may have a random value in the file system (it overlaps
	 *  the spare field array).  We also begin the else clause which
	 *  encompasses the remainder of the new-style file system processing
	 *  here (without indentation).
	 */
	fs->fs_oldfs = 0;
#endif	CS_OLDFS
	fs->fs_ronly = (ronly != 0);
	if (ronly == 0)
		fs->fs_fmod = 1;
	blks = howmany(fs->fs_cssize, fs->fs_fsize);
#if	MACH_VM
	space = (caddr_t)kmem_alloc(user_pt_map, (vm_size_t)fs->fs_cssize, TRUE);
#else	MACH_VM
	space = wmemall(vmemall, (int)fs->fs_cssize);
#endif	MACH_VM
	if (space == 0) {
		error = ENOMEM;
		goto out;
	}
	for (i = 0; i < blks; i += fs->fs_frag) {
		size = fs->fs_bsize;
		if (i + fs->fs_frag > blks)
			size = (blks - i) * fs->fs_fsize;
		tp = bread(dev, fsbtodb(fs, fs->fs_csaddr + i), size);
		if (tp->b_flags&B_ERROR) {
#if	MACH_VM
			kmem_free(user_pt_map, space, (vm_size_t)fs->fs_cssize);
#else	MACH_VM
			wmemfree(space, (int)fs->fs_cssize);
#endif	MACH_VM
			goto out;
		}
		bcopy((caddr_t)tp->b_un.b_addr, space, (u_int)size);
		fs->fs_csp[fragstoblks(fs, i)] = (struct csum *)space;
		space += size;
		brelse(tp);
		tp = 0;
	}
#if	CS_OLDFS
	/*
	 *  Close the new-style file system clause.
	 */
	}
#endif	CS_OLDFS
#if	CS_RPAUSE
	/*
	 *  Enable first file system/inodes full console messages and calculate
	 *  low water pause/high water resume marks for fragments and inodes.
 	 *
	 *  Fragment water marks:
	 *  lo - 1% of total (but <= 100)	(old file system)
	 *  hi - 2% of total (but <= 200)	(old file system)
	 *  lo - minfree% of total		(new file system)
	 *  hi - lo + minfree*100 		(new file system)
	 *
	 *  Inode water marks:
	 *  lo/hi - 1% of total (but <= 50)	(old file system)
	 *  lo/hi - 1% of total (but <= 50) 	(new file system)
	 */
	fs->fs_flags  &= ~(FS_FNOSPC|FS_INOSPC);
#if	CS_OLDFS
	if (isoldfs(fs))
	{
	    FS_FLOWAT(fs) = (fs->fs_dsize/100);
	    if (FS_FLOWAT(fs) > 100)
		FS_FLOWAT(fs) = 100;
	    FS_FHIWAT(fs) = 2*FS_FLOWAT(fs);
	}
	else
#endif	CS_OLDFS
	{
	    FS_FLOWAT(fs) = ((fs->fs_dsize * fs->fs_minfree)/100);
	    FS_FHIWAT(fs) = FS_FLOWAT(fs)+(fs->fs_minfree*100);
	}
	FS_ILOWAT(fs) = ((fs->fs_ncg * fs->fs_ipg)/100);
	if (FS_ILOWAT(fs) > 50)
	    FS_ILOWAT(fs) = 50;
	FS_IHIWAT(fs) = FS_ILOWAT(fs);
#ifdef	DEBUG
	printf("flowat=%d, fhiwat=%d, ilowat=%d, ihiwat=%d\n",
	       FS_FLOWAT(fs), FS_FHIWAT(fs), FS_ILOWAT(fs), FS_IHIWAT(fs));
#endif	DEBUG
#endif	CS_RPAUSE
	mp->m_inodp = ip;
	mp->m_dev = dev;
	if (ip) {
		ip->i_flag |= IMOUNT;
		iunlock(ip);
	}
	return (fs);
out:
	if (error == 0)
		error = EIO;
	if (ip)
		iput(ip);
	if (mp)
		mp->m_bufp = 0;
	if (bp)
		brelse(bp);
	if (tp)
		brelse(tp);
	if (needclose)
		(*bdevsw[major(dev)].d_close)(dev, ronly ? FREAD : FREAD|FWRITE);
	binval(dev);
	u.u_error = error;
	return (0);
}

umount()
{
	struct a {
		char	*fspec;
	} *uap = (struct a *)u.u_ap;

	u.u_error = unmount1(uap->fspec, 0);
}

unmount1(fname, forcibly)
	caddr_t fname;
	int forcibly;
{
	dev_t dev;
	register struct mount *mp;
	int stillopen, flag, error;
	register struct inode *ip;
	register struct fs *fs;

	error = getmdev(&dev, fname);
	if (error)
		return (error);
	for (mp = &mount[0]; mp < &mount[NMOUNT]; mp++)
		if (mp->m_bufp != NULL && dev == mp->m_dev)
			goto found;
	return (EINVAL);
found:
#if	MACH_VM
	vm_object_cache_clear();	/* remove cached files */
#else	MACH_VM
	xumount(dev);	/* remove unused sticky files from text table */
#endif	MACH_VM
	nchinval(dev);	/* flush the name cache */
	update();
#ifdef QUOTA
	if ((stillopen = iflush(dev, mp->m_qinod)) < 0 && !forcibly)
#else
	if ((stillopen = iflush(dev)) < 0 && !forcibly)
#endif
		return (EBUSY);
	if (stillopen < 0)
		return (EBUSY);			/* XXX */
#ifdef QUOTA
	closedq(mp);
	/*
	 * Here we have to iflush again to get rid of the quota inode.
	 * A drag, but it would be ugly to cheat, & this doesn't happen often
	 */
	(void)iflush(dev, (struct inode *)NULL);
#endif
	ip = mp->m_inodp;
	ip->i_flag &= ~IMOUNT;
	irele(ip);
	fs = mp->m_bufp->b_un.b_fs;
#if	CS_OLDFS
	/*
	 *  For an old-style file system, it's not safe to free the cylinder
	 *  group summary information which was never allocated.
	 */
	if (!isoldfs(fs))
#endif	CS_OLDFS
#if	MACH_VM
	kmem_free(user_pt_map, (caddr_t)fs->fs_csp[0], (vm_size_t)fs->fs_cssize);
#else	MACH_VM
	wmemfree((caddr_t)fs->fs_csp[0], (int)fs->fs_cssize);
#endif	MACH_VM
	flag = !fs->fs_ronly;
	brelse(mp->m_bufp);
	mp->m_bufp = 0;
	mp->m_dev = 0;
#if	MACH_VM
#else	MACH_VM
	mpurge(mp - &mount[0]);
#endif	MACH_VM
	if (!stillopen) {
		(*bdevsw[major(dev)].d_close)(dev, flag);
		binval(dev);
	}
	return (0);
}

sbupdate(mp)
	struct mount *mp;
{
	register struct fs *fs = mp->m_bufp->b_un.b_fs;
	register struct buf *bp;
	int blks;
	caddr_t space;
	int i, size;

#if	CS_OLDFS
	if (isoldfs(fs))
	{
		register struct filsys *fp = fs->fs_oldfs;

		/*
		 *  If we aren't really ready to do the flush back now,
		 *  remember to restore the modification flag so that it
		 *  will be tried again the next time.
		 */
		if ((fs->fs_fmod=(fp->s_ilock!=0 || fp->s_flock!=0)) == 0)
		{
			fp->s_time = fs->fs_time;
			bp = getblk(mp->m_dev, fsbtodb(fs, SUPERB), BSIZE);
			bcopy((caddr_t)fp, bp->b_un.b_addr, BSIZE);
			bwrite(bp);
		}
		return;
	}
#endif	CS_OLDFS
	bp = getblk(mp->m_dev, SBLOCK, (int)fs->fs_sbsize);
	bcopy((caddr_t)fs, bp->b_un.b_addr, (u_int)fs->fs_sbsize);
#if	CS_RPAUSE
	/*
	 *  These fields are supposed to be zero in 4.2 super-blocks and are
	 *  currently maintained only internally so we can just zero them here
	 *  (in the outgoing copy) for now.  Perhaps someday it will make sense
	 *  to record some of them on disk...
	 */
	bp->b_un.b_fs->fs_flowat = 0;
	bp->b_un.b_fs->fs_fhiwat = 0;
	bp->b_un.b_fs->fs_ilowat = 0;
	bp->b_un.b_fs->fs_ihiwat = 0;
	bp->b_un.b_fs->fs_flags = 0;
#endif	CS_RPAUSE
	bwrite(bp);
	blks = howmany(fs->fs_cssize, fs->fs_fsize);
	space = (caddr_t)fs->fs_csp[0];
	for (i = 0; i < blks; i += fs->fs_frag) {
		size = fs->fs_bsize;
		if (i + fs->fs_frag > blks)
			size = (blks - i) * fs->fs_fsize;
		bp = getblk(mp->m_dev, fsbtodb(fs, fs->fs_csaddr + i), size);
		bcopy(space, bp->b_un.b_addr, (u_int)size);
		space += size;
		bwrite(bp);
	}
}

/*
 * Common code for mount and umount.
 * Check that the user's argument is a reasonable
 * thing on which to mount, and return the device number if so.
 */
getmdev(pdev, fname)
	caddr_t fname;
	dev_t *pdev;
{
	dev_t dev;
	register struct inode *ip;
	register struct nameidata *ndp = &u.u_nd;

	if (!suser())
		return (u.u_error);
	ndp->ni_nameiop = LOOKUP | FOLLOW;
	ndp->ni_segflg = UIO_USERSPACE;
	ndp->ni_dirp = fname;
	ip = namei(ndp);
	if (ip == NULL) {
		if (u.u_error == ENOENT)
			return (ENODEV); /* needs translation */
		return (u.u_error);
	}
	if ((ip->i_mode&IFMT) != IFBLK) {
		iput(ip);
		return (ENOTBLK);
	}
	dev = (dev_t)ip->i_rdev;
	iput(ip);
	if (major(dev) >= nblkdev)
		return (ENXIO);
	*pdev = dev;
	return (0);
}
