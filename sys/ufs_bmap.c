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
 *	@(#)ufs_bmap.c	6.4 (Berkeley) 6/8/85
 */
#if	CMU
/*
 **********************************************************************
 * HISTORY
 * 25-Jan-86  Avadis Tevanian (avie) at Carnegie-Mellon University
 *	Upgraded to 4.3.
 *
 * 13-Jun-85  Mike Accetta (mja) at Carnegie-Mellon University
 *	Modified bmap() to also interpret old format file systems.
 *	[V1(1)].
 **********************************************************************
 */
 
#include "cs_oldfs.h"
#endif	CMU

#include "param.h"
#include "systm.h"
#include "inode.h"
#include "dir.h"
#include "user.h"
#include "buf.h"
#include "proc.h"
#include "fs.h"
#if	CS_OLDFS
#include "filsys.h"
#endif	CS_OLDFS

/*
 * Bmap defines the structure of file system storage
 * by returning the physical block number on a device given the
 * inode and the logical block number in a file.
 * When convenient, it also leaves the physical
 * block number of the next block of the file in rablock
 * for use in read-ahead.
 */
/*VARARGS3*/
daddr_t
bmap(ip, bn, rwflg, size)
	register struct inode *ip;
	daddr_t bn;
	int rwflg;
	int size;	/* supplied only when rwflg == B_WRITE */
{
	register int i;
	int osize, nsize;
	struct buf *bp, *nbp;
	struct fs *fs;
	int j, sh;
	daddr_t nb, lbn, *bap, pref, blkpref();

	if (bn < 0) {
		u.u_error = EFBIG;
		return ((daddr_t)0);
	}
	fs = ip->i_fs;
	rablock = 0;
	rasize = 0;		/* conservative */

#if	CS_OLDFS
	if (isoldfs(fs))
	{
	dev_t dev;

	dev = ip->i_dev;

	/*
	 * blocks 0..NOADDR-4 are direct blocks
	 */
	if(bn < NOADDR-3) {
		i = bn;
		nb = ip->i_db[i];
		if(nb == 0) {
			if(rwflg==B_READ || (bp = alloc(ip, (daddr_t)0, 0))==NULL)
				return((daddr_t)-1);
			nb = dbtofsb(fs, bp->b_blkno);
			if ((ip->i_mode&IFMT) == IFDIR)
				/*
				 * Write directory blocks synchronously
				 * so they never appear with garbage in
				 * them on the disk.
				 */
				bwrite(bp);
			else
				bdwrite(bp);
			ip->i_db[i] = nb;
			ip->i_flag |= IUPD|ICHG;
		}
		if(i < NOADDR-4)
		{
			rablock = fsbtodb(fs, ip->i_db[i+1]);
			rasize = fs->fs_bsize;
		}
		return(nb);
	}

	/*
	 * addresses NOADDR-3, NOADDR-2, and NOADDR-1
	 * have single, double, triple indirect blocks.
	 * the first step is to determine
	 * how many levels of indirection.
	 */
	sh = 1;
	bn -= NOADDR-3;
	for(j=3; j>0; j--) {
		sh *= NINDIR(fs);
		if(bn < sh)
			break;
		bn -= sh;
	}
	if(j == 0) {
		u.u_error = EFBIG;
		return((daddr_t)0);
	}

	/*
	 * fetch the first indirect block
	 */
	nb = ip->i_ib[NIADDR-j];
	if(nb == 0) {
		if(rwflg==B_READ || (bp = alloc(ip, (daddr_t)0, 0))==NULL)
			return((daddr_t)-1);
		nb = dbtofsb(fs, bp->b_blkno);
		/*
		 * Write synchronously so that indirect blocks
		 * never point at garbage.
		 */
		bwrite(bp);
		ip->i_ib[NIADDR-j] = nb;
		ip->i_flag |= IUPD|ICHG;
	}

	/*
	 * fetch through the indirect blocks
	 */
	for(; j<=3; j++) {
		bp = bread(dev, fsbtodb(fs, nb), (int)fs->fs_bsize);
		if(bp->b_flags & B_ERROR) {
			brelse(bp);
			return((daddr_t)0);
		}
		bap = bp->b_un.b_daddr;
		sh /= NINDIR(fs);
		i = (bn/sh) % NINDIR(fs);
		nb = bap[i];
		if(nb == 0) {
			if(rwflg==B_READ || (nbp = alloc(ip, (daddr_t)0, 0))==NULL) {
				brelse(bp);
				return((daddr_t)-1);
			}
			nb = dbtofsb(fs, nbp->b_blkno);
			if (j < 3 || (ip->i_mode&IFMT) == IFDIR)
				/*
				 * Write synchronously so indirect blocks
				 * never point at garbage and blocks
				 * in directories never contain garbage.
				 */
				bwrite(nbp);
			else
				bdwrite(nbp);
			bap[i] = nb;
			bdwrite(bp);
		} else
			brelse(bp);
	}

	/*
	 * calculate read-ahead.
	 */
	if(i < NINDIR(fs)-1)
	{
		rablock = fsbtodb(fs, bap[i+1]);
		rasize = fs->fs_bsize;
	}
	return(nb);
	}
#endif	CS_OLDFS
	/*
	 * If the next write will extend the file into a new block,
	 * and the file is currently composed of a fragment
	 * this fragment has to be extended to be a full block.
	 */
	nb = lblkno(fs, ip->i_size);
	if (rwflg == B_WRITE && nb < NDADDR && nb < bn) {
		osize = blksize(fs, ip, nb);
		if (osize < fs->fs_bsize && osize > 0) {
			bp = realloccg(ip, ip->i_db[nb],
				blkpref(ip, nb, (int)nb, &ip->i_db[0]),
				osize, (int)fs->fs_bsize);
			if (bp == NULL)
				return ((daddr_t)-1);
			ip->i_size = (nb + 1) * fs->fs_bsize;
			ip->i_db[nb] = dbtofsb(fs, bp->b_blkno);
			ip->i_flag |= IUPD|ICHG;
			bdwrite(bp);
		}
	}
	/*
	 * The first NDADDR blocks are direct blocks
	 */
	if (bn < NDADDR) {
		nb = ip->i_db[bn];
		if (rwflg == B_READ) {
			if (nb == 0)
				return ((daddr_t)-1);
			goto gotit;
		}
		if (nb == 0 || ip->i_size < (bn + 1) * fs->fs_bsize) {
			if (nb != 0) {
				/* consider need to reallocate a frag */
				osize = fragroundup(fs, blkoff(fs, ip->i_size));
				nsize = fragroundup(fs, size);
				if (nsize <= osize)
					goto gotit;
				bp = realloccg(ip, nb,
					blkpref(ip, bn, (int)bn, &ip->i_db[0]),
					osize, nsize);
			} else {
				if (ip->i_size < (bn + 1) * fs->fs_bsize)
					nsize = fragroundup(fs, size);
				else
					nsize = fs->fs_bsize;
				bp = alloc(ip,
					blkpref(ip, bn, (int)bn, &ip->i_db[0]),
					nsize);
			}
			if (bp == NULL)
				return ((daddr_t)-1);
			nb = dbtofsb(fs, bp->b_blkno);
			if ((ip->i_mode&IFMT) == IFDIR)
				/*
				 * Write directory blocks synchronously
				 * so they never appear with garbage in
				 * them on the disk.
				 */
				bwrite(bp);
			else
				bdwrite(bp);
			ip->i_db[bn] = nb;
			ip->i_flag |= IUPD|ICHG;
		}
gotit:
		if (bn < NDADDR - 1) {
			rablock = fsbtodb(fs, ip->i_db[bn + 1]);
			rasize = blksize(fs, ip, bn + 1);
		}
		return (nb);
	}

	/*
	 * Determine how many levels of indirection.
	 */
	pref = 0;
	sh = 1;
	lbn = bn;
	bn -= NDADDR;
	for (j = NIADDR; j>0; j--) {
		sh *= NINDIR(fs);
		if (bn < sh)
			break;
		bn -= sh;
	}
	if (j == 0) {
		u.u_error = EFBIG;
		return ((daddr_t)0);
	}

	/*
	 * fetch the first indirect block
	 */
	nb = ip->i_ib[NIADDR - j];
	if (nb == 0) {
		if (rwflg == B_READ)
			return ((daddr_t)-1);
		pref = blkpref(ip, lbn, 0, (daddr_t *)0);
	        bp = alloc(ip, pref, (int)fs->fs_bsize);
		if (bp == NULL)
			return ((daddr_t)-1);
		nb = dbtofsb(fs, bp->b_blkno);
		/*
		 * Write synchronously so that indirect blocks
		 * never point at garbage.
		 */
		bwrite(bp);
		ip->i_ib[NIADDR - j] = nb;
		ip->i_flag |= IUPD|ICHG;
	}

	/*
	 * fetch through the indirect blocks
	 */
	for (; j <= NIADDR; j++) {
		bp = bread(ip->i_dev, fsbtodb(fs, nb), (int)fs->fs_bsize);
		if (bp->b_flags & B_ERROR) {
			brelse(bp);
			return ((daddr_t)0);
		}
		bap = bp->b_un.b_daddr;
		sh /= NINDIR(fs);
		i = (bn / sh) % NINDIR(fs);
		nb = bap[i];
		if (nb == 0) {
			if (rwflg==B_READ) {
				brelse(bp);
				return ((daddr_t)-1);
			}
			if (pref == 0)
				if (j < NIADDR)
					pref = blkpref(ip, lbn, 0,
						(daddr_t *)0);
				else
					pref = blkpref(ip, lbn, i, &bap[0]);
		        nbp = alloc(ip, pref, (int)fs->fs_bsize);
			if (nbp == NULL) {
				brelse(bp);
				return ((daddr_t)-1);
			}
			nb = dbtofsb(fs, nbp->b_blkno);
			if (j < NIADDR || (ip->i_mode&IFMT) == IFDIR)
				/*
				 * Write synchronously so indirect blocks
				 * never point at garbage and blocks
				 * in directories never contain garbage.
				 */
				bwrite(nbp);
			else
				bdwrite(nbp);
			bap[i] = nb;
			bdwrite(bp);
		} else
			brelse(bp);
	}

	/*
	 * calculate read-ahead.
	 */
	if (i < NINDIR(fs) - 1) {
		rablock = fsbtodb(fs, bap[i+1]);
		rasize = fs->fs_bsize;
	}
	return (nb);
}
