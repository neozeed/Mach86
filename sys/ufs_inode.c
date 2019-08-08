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
 *	@(#)ufs_inode.c	6.17 (Berkeley) 9/4/85
 */
#if	CMU
/*
 **********************************************************************
 * HISTORY
 * 31-May-86  Avadis Tevanian (avie) at Carnegie-Mellon University
 *	Uncache inodes when the grow.  Initialize pager_id field in
 *	iget.
 *
 * 01-Mar-86  Mike Accetta (mja) at Carnegie-Mellon University
 *	CS_GENERIC: suppress inode time updates if "iupdnot"
 *	variable is non-zero (TEMP).
 *
 * 24-Feb-86  Bill Bolosky (bolosky) at Carnegie-Mellon University
 *	Added #if CS_COMPAT around check for IFMPC in iupdat.
 *
 * 11-Feb-86  Mike Accetta (mja) at Carnegie-Mellon University
 *	CS_OLDFS: set i_blocks to zero for old file system
 *	inodes on read (this is now estimated at stat() time).
 *
 * 26-Jan-86  Avadis Tevanian (avie) at Carnegie-Mellon University
 *	Upgraded to 4.3.
 *
 * 26-Aug-85  Mike Accetta (mja) at Carnegie-Mellon University
 *	CS_ICHK:  Added inode count consistency check macros.
 *	[V1(1)].
 *
 * 13-Jun-85  Mike Accetta (mja) at Carnegie-Mellon University
 *	CS_OLDFS:  Modified iget() and iupdat() to translate to/from
 *	old/new style inode format and itrunc() to properly handle old
 *	file system block pointers;  re-enabled ifind() routine which
 *	is still needed by the old ialloc() code.
 *	[V1(1)].
 *
 **********************************************************************
 */
 
#include "cs_ichk.h"
#include "cs_generic.h"
#include "cs_oldfs.h"
#include "mach_vm.h"
#endif	CMU

#if	MACH_VM
#include "../vm/vm_pager.h"
#endif	MACH_VM
#include "param.h"
#include "systm.h"
#include "mount.h"
#include "dir.h"
#include "user.h"
#include "inode.h"
#include "fs.h"
#include "buf.h"
#include "cmap.h"
#ifdef QUOTA
#include "quota.h"
#endif
#include "kernel.h"

#define	INOHSZ	512
#if	((INOHSZ&(INOHSZ-1)) == 0)
#define	INOHASH(dev,ino)	(((dev)+(ino))&(INOHSZ-1))
#else
#define	INOHASH(dev,ino)	(((unsigned)((dev)+(ino)))%INOHSZ)
#endif

union ihead {				/* inode LRU cache, Chris Maltby */
	union  ihead *ih_head[2];
	struct inode *ih_chain[2];
} ihead[INOHSZ];

struct inode *ifreeh, **ifreet;

#if	CS_ICHK
/* 
 *  Define these in one place to avoid string constant replication.
 */
char *PANICMSG_IINCR = "iincr";
char *PANICMSG_IDECR = "idecr";

#endif	CS_ICHK
/*
 * Initialize hash links for inodes
 * and build inode free list.
 */
ihinit()
{
	register int i;
	register struct inode *ip = inode;
	register union  ihead *ih = ihead;

	for (i = INOHSZ; --i >= 0; ih++) {
		ih->ih_head[0] = ih;
		ih->ih_head[1] = ih;
	}
	ifreeh = ip;
	ifreet = &ip->i_freef;
	ip->i_freeb = &ifreeh;
	ip->i_forw = ip;
	ip->i_back = ip;
	for (i = ninode; --i > 0; ) {
		++ip;
		ip->i_forw = ip;
		ip->i_back = ip;
		*ifreet = ip;
		ip->i_freeb = ifreet;
		ifreet = &ip->i_freef;
	}
	ip->i_freef = NULL;
}

#if	CS_OLDFS
#define	notdef
#endif	CS_OLDFS
#ifdef notdef
/*
 * Find an inode if it is incore.
 * This is the equivalent, for inodes,
 * of ``incore'' in bio.c or ``pfind'' in subr.c.
 */
struct inode *
ifind(dev, ino)
	dev_t dev;
	ino_t ino;
{
	register struct inode *ip;
	register union  ihead *ih;

	ih = &ihead[INOHASH(dev, ino)];
	for (ip = ih->ih_chain[0]; ip != (struct inode *)ih; ip = ip->i_forw)
		if (ino==ip->i_number && dev==ip->i_dev)
			return (ip);
	return ((struct inode *)0);
}
#endif notdef
#if	CS_OLDFS
#undef	notdef
#endif	CS_OLDFS

/*
 * Look up an inode by device,inumber.
 * If it is in core (in the inode structure),
 * honor the locking protocol.
 * If it is not in core, read it in from the
 * specified device.
 * If the inode is mounted on, perform
 * the indicated indirection.
 * In all cases, a pointer to a locked
 * inode structure is returned.
 *
 * panic: no imt -- if the mounted file
 *	system is not in the mount table.
 *	"cannot happen"
 */
struct inode *
iget(dev, fs, ino)
	dev_t dev;
	register struct fs *fs;
	ino_t ino;
{
	register struct inode *ip;
	register union  ihead *ih;
	register struct mount *mp;
	register struct buf *bp;
	register struct dinode *dp;
	register struct inode *iq;


loop:
	ih = &ihead[INOHASH(dev, ino)];
	for (ip = ih->ih_chain[0]; ip != (struct inode *)ih; ip = ip->i_forw)
		if (ino == ip->i_number && dev == ip->i_dev) {
			/*
			 * Following is essentially an inline expanded
			 * copy of igrab(), expanded inline for speed,
			 * and so that the test for a mounted on inode
			 * can be deferred until after we are sure that
			 * the inode isn't busy.
			 */
			if ((ip->i_flag&ILOCKED) != 0) {
				ip->i_flag |= IWANT;
				sleep((caddr_t)ip, PINOD);
				goto loop;
			}
			if ((ip->i_flag&IMOUNT) != 0) {
				for (mp = &mount[0]; mp < &mount[NMOUNT]; mp++)
					if(mp->m_inodp == ip) {
						dev = mp->m_dev;
						fs = mp->m_bufp->b_un.b_fs;
						ino = ROOTINO;
						goto loop;
					}
				panic("no imt");
			}
			if (ip->i_count == 0) {		/* ino on free list */
				if (iq = ip->i_freef)
					iq->i_freeb = ip->i_freeb;
				else
					ifreet = ip->i_freeb;
				*ip->i_freeb = iq;
				ip->i_freef = NULL;
				ip->i_freeb = NULL;
			}
#if	CS_ICHK
			iincr_chk(ip);
#else	CS_ICHK
			ip->i_count++;
#endif	CS_ICHK
			ip->i_flag |= ILOCKED;
			return(ip);
		}

	if ((ip = ifreeh) == NULL) {
		tablefull("inode");
		u.u_error = ENFILE;
		return(NULL);
	}
	if (ip->i_count)
		panic("free inode isn't");
	if (iq = ip->i_freef)
		iq->i_freeb = &ifreeh;
	ifreeh = iq;
	ip->i_freef = NULL;
	ip->i_freeb = NULL;
	/*
	 * Now to take inode off the hash chain it was on
	 * (initially, or after an iflush, it is on a "hash chain"
	 * consisting entirely of itself, and pointed to by no-one,
	 * but that doesn't matter), and put it on the chain for
	 * its new (ino, dev) pair
	 */
	remque(ip);
	insque(ip, ih);
#if	MACH_VM
	ip->pager_id = VM_PAGER_ID_NULL;
#endif	MACH_VM
	ip->i_dev = dev;
	ip->i_fs = fs;
	ip->i_number = ino;
	cacheinval(ip);
	ip->i_flag = ILOCKED;
#if	CS_ICHK
	iincr_chk(ip);
#else	CS_ICHK
	ip->i_count++;
#endif	CS_ICHK
	ip->i_lastr = 0;
#ifdef QUOTA
	dqrele(ip->i_dquot);
#endif
	bp = bread(dev, fsbtodb(fs, itod(fs, ino)), (int)fs->fs_bsize);
	/*
	 * Check I/O errors
	 */
	if ((bp->b_flags&B_ERROR) != 0) {
		brelse(bp);
		/*
		 * the inode doesn't contain anything useful, so it would
		 * be misleading to leave it on its hash chain.
		 * 'iput' will take care of putting it back on the free list.
		 */
		remque(ip);
		ip->i_forw = ip;
		ip->i_back = ip;
		/*
		 * we also loose its inumber, just in case (as iput
		 * doesn't do that any more) - but as it isn't on its
		 * hash chain, I doubt if this is really necessary .. kre
		 * (probably the two methods are interchangable)
		 */
		ip->i_number = 0;
#ifdef QUOTA
		ip->i_dquot = NODQUOT;
#endif
		iput(ip);
		return(NULL);
	}
	dp = bp->b_un.b_dino;
#if	CS_OLDFS
	/*
	 *  The mode, nlink, uid and gid fields are identical in both versions.
	 *  The size field used to be a longword and is now a quadword.  The
	 *  disk addresses are no longer compressed on disk but there is in-
	 *  core space for 15 pointers (12 direct and 3 indirect) so the old 13
	 *  (10 direct and 3 indirect) can share the same space with the direct
	 *  pointer overlapping into the indirect area.  The file times are
	 *  now stored in memory with an expansion word which is currently
	 *  unused.  The blocks used field is new and set to zero for old file
	 *  systems for now (until something else turns out be needed).
	 */
	if (isoldfs(fs)) {
		register char *p1, *p2;	/* wish these were really registers */
		int i,j;
#define		oi	((struct oinode *)dp)

		oi = oi + itooo(fs, ino);
		ip->i_mode = oi->oi_mode;
		ip->i_nlink = oi->oi_nlink;
		ip->i_uid = oi->oi_uid;
		ip->i_gid = oi->oi_gid;
		ip->i_size = oi->oi_size;
		p2 = (char *)oi->oi_addr;
		for (j=2; --j >= 0; ) {
		    if (j) {
			i = NOADDR-NIADDR;
			p1 = (char *)ip->i_db;
		    }
		    else {
			i = NIADDR;
			p1 = (char *)ip->i_ib;
		    }
		    for (; --i >= 0; ) {
			*p1++ = *p2++;
			*p1++ = *p2++;
			*p1++ = *p2++;
			*p1++ = 0;
		    }
		}
		ip->i_db[NDADDR-2] = 0;
		ip->i_db[NDADDR-1] = 0;
		ip->i_atime = oi->oi_atime;
		ip->i_mtime = oi->oi_mtime;
		ip->i_ctime = oi->oi_ctime;
		ip->i_blocks = 0;
#undef		oi
	}
	else
	{
		dp += itoo(fs, ino);
		ip->i_ic = dp->di_ic;
	}
#else	CS_OLDFS
	dp += itoo(fs, ino);
	ip->i_ic = dp->di_ic;
#endif	CS_OLDFS
	brelse(bp);
#ifdef QUOTA
	if (ip->i_mode == 0)
		ip->i_dquot = NODQUOT;
	else
		ip->i_dquot = inoquota(ip);
#endif
	return (ip);
}

/*
 * Convert a pointer to an inode into a reference to an inode.
 *
 * This is basically the internal piece of iget (after the
 * inode pointer is located) but without the test for mounted
 * filesystems.  It is caller's responsibility to check that
 * the inode pointer is valid.
 */
igrab(ip)
	register struct inode *ip;
{
	while ((ip->i_flag&ILOCKED) != 0) {
		ip->i_flag |= IWANT;
		sleep((caddr_t)ip, PINOD);
	}
	if (ip->i_count == 0) {		/* ino on free list */
		register struct inode *iq;

		if (iq = ip->i_freef)
			iq->i_freeb = ip->i_freeb;
		else
			ifreet = ip->i_freeb;
		*ip->i_freeb = iq;
		ip->i_freef = NULL;
		ip->i_freeb = NULL;
	}
	ip->i_count++;
	ip->i_flag |= ILOCKED;
}

/*
 * Decrement reference count of
 * an inode structure.
 * On the last reference,
 * write the inode out and if necessary,
 * truncate and deallocate the file.
 */
iput(ip)
	register struct inode *ip;
{

	if ((ip->i_flag & ILOCKED) == 0)
		panic("iput");
	IUNLOCK(ip);
	irele(ip);
}

irele(ip)
	register struct inode *ip;
{
	int mode;

	if (ip->i_count == 1) {
		ip->i_flag |= ILOCKED;
		if (ip->i_nlink <= 0 && ip->i_fs->fs_ronly == 0) {
			itrunc(ip, (u_long)0);
			mode = ip->i_mode;
			ip->i_mode = 0;
			ip->i_rdev = 0;
			ip->i_flag |= IUPD|ICHG;
			ifree(ip, ip->i_number, mode);
#ifdef QUOTA
			(void) chkiq(ip->i_dev, ip, ip->i_uid, 0);
			dqrele(ip->i_dquot);
			ip->i_dquot = NODQUOT;
#endif
		}
		IUPDAT(ip, &time, &time, 0);
		IUNLOCK(ip);
		ip->i_flag = 0;
		/*
		 * Put the inode on the end of the free list.
		 * Possibly in some cases it would be better to
		 * put the inode at the head of the free list,
		 * (eg: where i_mode == 0 || i_number == 0)
		 * but I will think about that later .. kre
		 * (i_number is rarely 0 - only after an i/o error in iget,
		 * where i_mode == 0, the inode will probably be wanted
		 * again soon for an ialloc, so possibly we should keep it)
		 */
		if (ifreeh) {
			*ifreet = ip;
			ip->i_freeb = ifreet;
		} else {
			ifreeh = ip;
			ip->i_freeb = &ifreeh;
		}
		ip->i_freef = NULL;
		ifreet = &ip->i_freef;
	} else if (!(ip->i_flag & ILOCKED))
		ITIMES(ip, &time, &time);
#if	CS_ICHK
	idecr_chk(ip);
#else	CS_ICHK
	ip->i_count--;
#endif	CS_ICHK
}

#if	CS_GENERIC
int iupdnot = 0;	/* TEMP: suppress time updates if true */
#endif	CS_GENERIC
/*
 * Check accessed and update flags on
 * an inode structure.
 * If any is on, update the inode
 * with the current time.
 * If waitfor is given, then must insure
 * i/o order so wait for write to complete.
 */
iupdat(ip, ta, tm, waitfor)
	register struct inode *ip;
	struct timeval *ta, *tm;
	int waitfor;
{
	register struct buf *bp;
	struct dinode *dp;
	register struct fs *fp;

	fp = ip->i_fs;
	if ((ip->i_flag & (IUPD|IACC|ICHG|IMOD)) != 0) {
		if (fp->fs_ronly)
			return;
		bp = bread(ip->i_dev, fsbtodb(fp, itod(fp, ip->i_number)),
			(int)fp->fs_bsize);
		if (bp->b_flags & B_ERROR) {
			brelse(bp);
			return;
		}
#if	CS_GENERIC
		if (iupdnot)
			ip->i_flag &= ~(ICHG);
#endif	CS_GENERIC
		if (ip->i_flag&IACC)
			ip->i_atime = ta->tv_sec;
		if (ip->i_flag&IUPD)
			ip->i_mtime = tm->tv_sec;
		if (ip->i_flag&ICHG)
			ip->i_ctime = time.tv_sec;
		ip->i_flag &= ~(IUPD|IACC|ICHG|IMOD);
#if	CS_OLDFS
		if (isoldfs(fp)) {
			register char *p1, *p2;
			int i,j;
#define			oi	((struct oinode *)dp)

			oi = (struct oinode *)bp->b_un.b_dino + itooo(fp, ip->i_number);
			oi->oi_mode = ip->i_mode;
			oi->oi_nlink = ip->i_nlink;
			oi->oi_uid = ip->i_uid;
			oi->oi_gid = ip->i_gid;
			oi->oi_size = ip->i_size;
			p1 = (char *)oi->oi_addr;
			for (j=2; --j >= 0; ) {
				if (j) {
					i = NOADDR-NIADDR;
					p2 = (char *)ip->i_db;
				}
				else {
					i = NIADDR;
					p2 = (char *)ip->i_ib;
				}
				for(; --i >= 0;) {
					*p1++ = *p2++;
					*p1++ = *p2++;
					*p1++ = *p2++;
#if CS_COMPAT
					if(*p2++ != 0 && (ip->i_mode&IFMT)!=IFMPC
					   && (ip->i_mode&IFMT)!=IFMPB)
#else CS_COMPAT
					if (*p2++ != 0)
#endif CS_COMPAT
						printf("iaddress > 2^24\n");
				}
			}
			oi->oi_atime = ip->i_atime;
			oi->oi_mtime = ip->i_mtime;
			oi->oi_ctime = ip->i_ctime;
#undef		oi
		}
		else {
			dp = bp->b_un.b_dino + itoo(fp, ip->i_number);
			dp->di_ic = ip->i_ic;
		}
#else	CS_OLDFS
		dp = bp->b_un.b_dino + itoo(fp, ip->i_number);
		dp->di_ic = ip->i_ic;
#endif	CS_OLDFS
		if (waitfor)
			bwrite(bp);
		else
			bdwrite(bp);
	}
}

#define	SINGLE	0	/* index of single indirect block */
#define	DOUBLE	1	/* index of double indirect block */
#define	TRIPLE	2	/* index of triple indirect block */
/*
 * Truncate the inode ip to at most
 * length size.  Free affected disk
 * blocks -- the blocks of the file
 * are removed in reverse order.
 *
 * NB: triple indirect blocks are untested.
 */
itrunc(oip, length)
	register struct inode *oip;
	u_long length;
{
	register daddr_t lastblock;
	daddr_t bn, lastiblock[NIADDR];
	register struct fs *fs;
	register struct inode *ip;
	struct buf *bp;
	int offset, lbn, osize, size, count, level, s;
	long nblocks, blocksreleased = 0;
	register int i;
	dev_t dev;
	struct inode tip;
	extern long indirtrunc();
	extern struct cmap *mfind();

	if (oip->i_size <= length) {
		oip->i_flag |= ICHG|IUPD;
		iupdat(oip, &time, &time, 1);
		return;
	}
	/*
	 * Calculate index into inode's block list of
	 * last direct and indirect blocks (if any)
	 * which we want to keep.  Lastblock is -1 when
	 * the file is truncated to 0.
	 */
	fs = oip->i_fs;
	lastblock = lblkno(fs, length + fs->fs_bsize - 1) - 1;
	lastiblock[SINGLE] = lastblock - NDADDR;
#if	CS_OLDFS
	if (isoldfs(fs))
	    lastiblock[SINGLE] += NDADDR-(NOADDR-NIADDR);
#endif	CS_OLDFS
	lastiblock[DOUBLE] = lastiblock[SINGLE] - NINDIR(fs);
	lastiblock[TRIPLE] = lastiblock[DOUBLE] - NINDIR(fs) * NINDIR(fs);
	nblocks = btodb(fs->fs_bsize);
	/*
	 * Update the size of the file. If the file is not being
	 * truncated to a block boundry, the contents of the
	 * partial block following the end of the file must be
	 * zero'ed in case it ever become accessable again because
	 * of subsequent file growth.
	 */
	osize = oip->i_size;
	offset = blkoff(fs, length);
	if (offset == 0) {
		oip->i_size = length;
	} else {
		lbn = lblkno(fs, length);
		bn = fsbtodb(fs, bmap(oip, lbn, B_WRITE, offset));
		if (u.u_error || (long)bn < 0)
			return;
		oip->i_size = length;
		size = blksize(fs, oip, lbn);
		count = howmany(size, DEV_BSIZE);
		dev = oip->i_dev;
#if	MACH_VM
		if (oip->pager_id != VM_PAGER_ID_NULL)
			vm_object_uncache(vm_pager_inode, oip->pager_id);
#else	MACH_VM
		s = splimp();
		for (i = 0; i < count; i += CLSIZE)
			if (mfind(dev, bn + i))
				munhash(dev, bn + i);
		splx(s);
#endif	MACH_VM
		bp = bread(dev, bn, size);
		if (bp->b_flags & B_ERROR) {
			u.u_error = EIO;
			oip->i_size = osize;
			brelse(bp);
			return;
		}
		bzero(bp->b_un.b_addr + offset, size - offset);
		bdwrite(bp);
	}
	/*
	 * Update file and block pointers
	 * on disk before we start freeing blocks.
	 * If we crash before free'ing blocks below,
	 * the blocks will be returned to the free list.
	 * lastiblock values are also normalized to -1
	 * for calls to indirtrunc below.
	 */
	tip = *oip;
	tip.i_size = osize;
	for (level = TRIPLE; level >= SINGLE; level--)
		if (lastiblock[level] < 0) {
			oip->i_ib[level] = 0;
			lastiblock[level] = -1;
		}
	for (i = NDADDR - 1; i > lastblock; i--)
		oip->i_db[i] = 0;
	oip->i_flag |= ICHG|IUPD;
	syncip(oip);

	/*
	 * Indirect blocks first.
	 */
	ip = &tip;
	for (level = TRIPLE; level >= SINGLE; level--) {
		bn = ip->i_ib[level];
		if (bn != 0) {
			blocksreleased +=
			    indirtrunc(ip, bn, lastiblock[level], level);
			if (lastiblock[level] < 0) {
				ip->i_ib[level] = 0;
				free(ip, bn, (off_t)fs->fs_bsize);
				blocksreleased += nblocks;
			}
		}
		if (lastiblock[level] >= 0)
			goto done;
	}

	/*
	 * All whole direct blocks or frags.
	 */
	for (i = NDADDR - 1; i > lastblock; i--) {
		register int bsize;

		bn = ip->i_db[i];
		if (bn == 0)
			continue;
		ip->i_db[i] = 0;
		bsize = (off_t)blksize(fs, ip, i);
		free(ip, bn, bsize);
		blocksreleased += btodb(bsize);
	}
	if (lastblock < 0)
		goto done;

	/*
	 * Finally, look for a change in size of the
	 * last direct block; release any frags.
	 */
	bn = ip->i_db[lastblock];
#if	CS_OLDFS
	if (bn != 0 && !isoldfs(fs)) {
#else	CS_OLDFS
	if (bn != 0) {
#endif	CS_OLDFS
		int oldspace, newspace;

		/*
		 * Calculate amount of space we're giving
		 * back as old block size minus new block size.
		 */
		oldspace = blksize(fs, ip, lastblock);
		ip->i_size = length;
		newspace = blksize(fs, ip, lastblock);
		if (newspace == 0)
			panic("itrunc: newspace");
		if (oldspace - newspace > 0) {
			/*
			 * Block number of space to be free'd is
			 * the old block # plus the number of frags
			 * required for the storage we're keeping.
			 */
			bn += numfrags(fs, newspace);
			free(ip, bn, oldspace - newspace);
			blocksreleased += btodb(oldspace - newspace);
		}
	}
done:
/* BEGIN PARANOIA */
	for (level = SINGLE; level <= TRIPLE; level++)
		if (ip->i_ib[level] != oip->i_ib[level])
			panic("itrunc1");
	for (i = 0; i < NDADDR; i++)
		if (ip->i_db[i] != oip->i_db[i])
			panic("itrunc2");
/* END PARANOIA */
	oip->i_blocks -= blocksreleased;
	if (oip->i_blocks < 0)			/* sanity */
		oip->i_blocks = 0;
	oip->i_flag |= ICHG;
#ifdef QUOTA
	(void) chkdq(oip, -blocksreleased, 0);
#endif
}

/*
 * Release blocks associated with the inode ip and
 * stored in the indirect block bn.  Blocks are free'd
 * in LIFO order up to (but not including) lastbn.  If
 * level is greater than SINGLE, the block is an indirect
 * block and recursive calls to indirtrunc must be used to
 * cleanse other indirect blocks.
 *
 * NB: triple indirect blocks are untested.
 */
long
indirtrunc(ip, bn, lastbn, level)
	register struct inode *ip;
	daddr_t bn, lastbn;
	int level;
{
	register int i;
	struct buf *bp, *copy;
	register daddr_t *bap;
	register struct fs *fs = ip->i_fs;
	daddr_t nb, last;
	long factor;
	int blocksreleased = 0, nblocks;

	/*
	 * Calculate index in current block of last
	 * block to be kept.  -1 indicates the entire
	 * block so we need not calculate the index.
	 */
	factor = 1;
	for (i = SINGLE; i < level; i++)
		factor *= NINDIR(fs);
	last = lastbn;
	if (lastbn > 0)
		last /= factor;
	nblocks = btodb(fs->fs_bsize);
	/*
	 * Get buffer of block pointers, zero those 
	 * entries corresponding to blocks to be free'd,
	 * and update on disk copy first.
	 */
	copy = geteblk((int)fs->fs_bsize);
	bp = bread(ip->i_dev, fsbtodb(fs, bn), (int)fs->fs_bsize);
	if (bp->b_flags&B_ERROR) {
		brelse(copy);
		brelse(bp);
		return (0);
	}
	bap = bp->b_un.b_daddr;
	bcopy((caddr_t)bap, (caddr_t)copy->b_un.b_daddr, (u_int)fs->fs_bsize);
	bzero((caddr_t)&bap[last + 1],
	  (u_int)(NINDIR(fs) - (last + 1)) * sizeof (daddr_t));
	bwrite(bp);
	bp = copy, bap = bp->b_un.b_daddr;

	/*
	 * Recursively free totally unused blocks.
	 */
	for (i = NINDIR(fs) - 1; i > last; i--) {
		nb = bap[i];
		if (nb == 0)
			continue;
		if (level > SINGLE)
			blocksreleased +=
			    indirtrunc(ip, nb, (daddr_t)-1, level - 1);
		free(ip, nb, (int)fs->fs_bsize);
		blocksreleased += nblocks;
	}

	/*
	 * Recursively free last partial block.
	 */
	if (level > SINGLE && lastbn >= 0) {
		last = lastbn % factor;
		nb = bap[i];
		if (nb != 0)
			blocksreleased += indirtrunc(ip, nb, last, level - 1);
	}
	brelse(bp);
	return (blocksreleased);
}

/*
 * remove any inodes in the inode cache belonging to dev
 *
 * There should not be any active ones, return error if any are found
 * (nb: this is a user error, not a system err)
 *
 * Also, count the references to dev by block devices - this really
 * has nothing to do with the object of the procedure, but as we have
 * to scan the inode table here anyway, we might as well get the
 * extra benefit.
 *
 * this is called from sumount()/sys3.c when dev is being unmounted
 */
#ifdef QUOTA
iflush(dev, iq)
	dev_t dev;
	struct inode *iq;
#else
iflush(dev)
	dev_t dev;
#endif
{
	register struct inode *ip;
	register open = 0;

	for (ip = inode; ip < inodeNINODE; ip++) {
#ifdef QUOTA
		if (ip != iq && ip->i_dev == dev)
#else
		if (ip->i_dev == dev)
#endif
			if (ip->i_count)
				return(-1);
			else {
				remque(ip);
				ip->i_forw = ip;
				ip->i_back = ip;
				/*
				 * as i_count == 0, the inode was on the free
				 * list already, just leave it there, it will
				 * fall off the bottom eventually. We could
				 * perhaps move it to the head of the free
				 * list, but as umounts are done so
				 * infrequently, we would gain very little,
				 * while making the code bigger.
				 */
#ifdef QUOTA
				dqrele(ip->i_dquot);
				ip->i_dquot = NODQUOT;
#endif
			}
		else if (ip->i_count && (ip->i_mode&IFMT)==IFBLK &&
		    ip->i_rdev == dev)
			open++;
	}
	return (open);
}

/*
 * Lock an inode. If its already locked, set the WANT bit and sleep.
 */
ilock(ip)
	register struct inode *ip;
{

	ILOCK(ip);
}

/*
 * Unlock an inode.  If WANT bit is on, wakeup.
 */
iunlock(ip)
	register struct inode *ip;
{

	IUNLOCK(ip);
}
