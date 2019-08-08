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
 *	@(#)ufs_alloc.c	6.16 (Berkeley) 9/17/85
 */
#if	CMU
 
/*
 **********************************************************************
 * HISTORY
 * 25-Jan-86  Avadis Tevanian (avie) at Carnegie-Mellon University
 *	Upgraded to 4.3.
 *
 * 13-Jan-86  Robert V Baron (rvb) at Carnegie-Mellon University
 *	sensor changes for mll
 *
 * 03-Aug-85  Mike Accetta (mja) at Carnegie-Mellon University
 *	CS_RPAUSE:  Added fsfull(), fssleep() and fspause() routines
 *	and changed file system block and inode allocation/free
 *	routines to handle these resource exhaustion and resumption
 *	conditions.
 *	[V1(1)]
 *
 * 13-Jun-85  Mike Accetta (mja) at Carnegie-Mellon University
 *	CS_OLDFS:  Changed to also handle old format file systems.
 *	[V1(1)]
 *
 **********************************************************************
 */
 
#include "cs_oldfs.h"
#include "cs_rpause.h"
#include "wb_sens.h"
#endif	CMU


#include "param.h"
#include "systm.h"
#include "mount.h"
#include "fs.h"
#include "buf.h"
#include "inode.h"
#include "dir.h"
#include "user.h"
#include "quota.h"
#include "kernel.h"
#include "syslog.h"
#if	CS_OLDFS
#include "filsys.h"
#endif	CS_OLDFS
#if	NWB_SENS > 0
#include "proc.h"
#include "../sensor/ufs_sensors.h"
#endif	NWB_SENS > 0

extern u_long		hashalloc();
extern ino_t		ialloccg();
extern daddr_t		alloccg();
extern daddr_t		alloccgblk();
extern daddr_t		fragextend();
extern daddr_t		blkpref();
extern daddr_t		mapsearch();
extern int		inside[], around[];
extern unsigned char	*fragtbl[];

/*
 * Allocate a block in the file system.
 * 
 * The size of the requested block is given, which must be some
 * multiple of fs_fsize and <= fs_bsize.
 * A preference may be optionally specified. If a preference is given
 * the following hierarchy is used to allocate a block:
 *   1) allocate the requested block.
 *   2) allocate a rotationally optimal block in the same cylinder.
 *   3) allocate a block in the same cylinder group.
 *   4) quadradically rehash into other cylinder groups, until an
 *      available block is located.
 * If no block preference is given the following heirarchy is used
 * to allocate a block:
 *   1) allocate a block in the cylinder group that contains the
 *      inode for the file.
 *   2) quadradically rehash into other cylinder groups, until an
 *      available block is located.
 */
struct buf *
alloc(ip, bpref, size)
	register struct inode *ip;
	daddr_t bpref;
	int size;
{
	daddr_t bno;
	register struct fs *fs;
	register struct buf *bp;
	int cg;
	
	fs = ip->i_fs;
#if	CS_OLDFS
	/*
 	 * old format:
	 *
	 * alloc will obtain the next available
	 * free disk block from the free list of
	 * the specified device.
	 * The super block has up to NICFREE remembered
	 * free blocks; the last of these is read to
	 * obtain NICFREE more . . .
	 */
	if (isoldfs(fs))
	{
	register struct filsys *fp;
	
	fp = fs->fs_oldfs;
	while (fp->s_flock)
		sleep((caddr_t)&fp->s_flock, PINOD);
	do {
		if (u.u_uid != 0 && fp->s_tfree <= FS_FLOWAT(fs))
			goto nospace;
		if (fp->s_nfree <= 0)
			goto onospace;
		if (fp->s_nfree > NICFREE) {
			fserr(fs, "bad free count");
			goto onospace;
		}
		bno = fp->s_free[--fp->s_nfree];
		if (bno == 0)
			goto onospace;
	} while (badblock(fs, bno));
	if (fp->s_nfree <= 0) {
		fp->s_flock++;
		bp = bread(ip->i_dev, fsbtodb(fs, bno), (int)fs->fs_bsize);
		if ((bp->b_flags&B_ERROR) == 0) {
			fp->s_nfree = ((FBLKP)(bp->b_un.b_addr))->df_nfree;
			bcopy((caddr_t)((FBLKP)(bp->b_un.b_addr))->df_free,
			    (caddr_t)fp->s_free, sizeof(fp->s_free));
		}
		brelse(bp);
		fp->s_flock = 0;
		wakeup((caddr_t)&fp->s_flock);
		if (fp->s_nfree <= 0)
			goto onospace;
	}
	bp = getblk(ip->i_dev, fsbtodb(fs, bno), (int)fs->fs_bsize);
	clrbuf(bp);
	fs->fs_fmod = 1;
	fp->s_tfree--;
	return (bp);

onospace:
	fp->s_nfree = 0;
	fp->s_tfree = 0;
	goto nospace;
	}
#endif	CS_OLDFS
	if ((unsigned)size > fs->fs_bsize || fragoff(fs, size) != 0) {
		printf("dev = 0x%x, bsize = %d, size = %d, fs = %s\n",
		    ip->i_dev, fs->fs_bsize, size, fs->fs_fsmnt);
		panic("alloc: bad size");
	}
	if (size == fs->fs_bsize && fs->fs_cstotal.cs_nbfree == 0)
		goto nospace;
	if (u.u_uid != 0 && freespace(fs, fs->fs_minfree) <= 0)
		goto nospace;
#ifdef QUOTA
	u.u_error = chkdq(ip, (long)btodb(size), 0);
	if (u.u_error)
		return (NULL);
#endif
	if (bpref >= fs->fs_size)
		bpref = 0;
	if (bpref == 0)
		cg = itog(fs, ip->i_number);
	else
		cg = dtog(fs, bpref);
	bno = (daddr_t)hashalloc(ip, cg, (long)bpref, size,
		(u_long (*)())alloccg);
	if (bno <= 0)
		goto nospace;
	ip->i_blocks += btodb(size);
	ip->i_flag |= IUPD|ICHG;
	bp = getblk(ip->i_dev, fsbtodb(fs, bno), size);
	clrbuf(bp);
	return (bp);
nospace:
#if	CS_RPAUSE
	fsfull(fs, FS_FNOSPC);
#else	CS_RPAUSE
	fserr(fs, "file system full");
	uprintf("\n%s: write failed, file system is full\n", fs->fs_fsmnt);
	u.u_error = ENOSPC;
#endif	CS_RPAUSE
	return (NULL);
}


#if	CS_RPAUSE
/* 
 *  fsfull - handle a full file system condition
 *
 *  fs    = file system with space exhausted
 *  which = FS_FNOSPC iff space exhausted, FS_INOSPC iff inodes exhausted
 *
 *  If the file system is not currently in a space exhausted condition, log a
 *  message on the console and prevent any subsequent message until space rises
 *  above the high water mark or inodes become available.  If resource pauses
 *  are disabled for the ENOSPC error, log an error message to the user
 *  terminal (if they are enabled, the message will be provided by the
 *  fspause() routine when it is invoked).
 *
 *  Return: with u_error set to ENOSPC.
 *
 *  TODO:   Need to handle case where a file system is dismounted while we are
 *  asleep.  If we just continue to use the file system pointer which now
 *  points to a deallocated buffer, we could be in trouble.  This is highly
 *  unlikely, but ...
 */

fsfull(fs, which)
register struct fs *fs;
int which;
{
    char *cmesg;
    char *umesg;

    if (which&FS_FNOSPC)
    {
	cmesg = "file system full";
	umesg = "write failed, file system is full";
    }
    else if (which&FS_INOSPC)
    {
	cmesg = "out of inodes";
	umesg = "create/symlink failed, no inodes free";
    }
    else
	panic("fsfull");
	
    if ((fs->fs_flags&which) == 0)
	fserr(fs, cmesg);
    fs->fs_flags |= which;	/* no more messages while full */
    if ((u.u_rpause&URPS_NOSPC) == 0)
	uprintf("\n%s: %s\n", fs->fs_fsmnt, umesg);

    /*
     *  Record the file system and type of failure for possible restart by our
     *  caller.  In case we have taken a nested call, only record the first
     *  problem (although I'm not sure when this can even happen).
     */
    if (u.u_rpsfs == 0)
    {
	u.u_rpsfs = fs;
	u.u_rpswhich = which;
    }
    u.u_error = ENOSPC;
}



/* 
 *  fssleep - block until space exceeds the high water mark on a file system
 *
 *  fs    = file system with space exhausted
 *  which = FS_FNOSPC iff disk space exhausted, FS_INOSPC iff out of inodes
 *
 *  Sleep (interruptably) waiting for the free fragment or inodes count for the
 *  file system to execeed the high water mark established at mount time.  This
 *  routine is called from fspause() via rpause() during a resource pause on a
 *  full file system.
 */

fssleep(fs, which)
register struct fs *fs;
int which;
{
    if (which&FS_FNOSPC)
    {
        while (freefrags(fs) <= FS_FHIWAT(fs))
	    sleep((caddr_t)&fs->fs_cstotal.cs_nffree, PPIPE);
    }
    else if (which&FS_INOSPC)
    {
        while (freeinodes(fs) <= FS_IHIWAT(fs))
	    sleep((caddr_t)&fs->fs_cstotal.cs_nifree, PPIPE);
    }
    else
	panic("fssleep");
}
 


/* 
 *  fspause - wait for space on file system
 *
 *  This routine is called after the completion of any system call which
 *  resulted in an error.  If the call terminated due to lack of space or
 *  inodes, a resource pause is begun until space becomes available again.  In
 *  this case, the u_rpsfs and u_rpswhich fields of the U area will have been
 *  set by fsfull() above to indicate the file system and operation which
 *  failed and must pause.
 *
 *  This routine is invoked directly by the common system call handler on an
 *  error return from the dispatched system call.  It can also be called from
 *  within a system call routine itself if special handling is required.  To
 *  prevent nested call problems, the resource pause fields are always cleared
 *  by the system call handler before dispatching to the system call and on any
 *  return from this routine.  If they are not set upon entry, the condition is
 *  presumed to have been handled elsewhere (e.g. by a preceding call at a
 *  lower level).
 *
 *  Return: true with no error if the invoking operation should be retried,
 *  otherwise false with the appropriate error set if it should be aborted
 *  (e.g. the error was not a lack of space or the pause is disabled by the
 *  user or was interrupted by a signal).
 */

bool
fspause()
{
    register struct fs *fs = u.u_rpsfs;
    int which = u.u_rpswhich;

    u.u_rpsfs = 0;
    u.u_rpswhich = 0;
    if (fs == 0 || which == 0 || u.u_error != ENOSPC || (u.u_rpause&URPS_NOSPC) == 0)
	return(false);

    u.u_error = 0;
    if (! rpause(fssleep, (int)fs, which, fs->fs_fsmnt,
		(which&1) == 0?" out of inodes":" file system is full"))
    {
	u.u_error = ENOSPC;
	return(false);
    }

    return(true);
}
#endif	CS_RPAUSE
/*
 * Reallocate a fragment to a bigger size
 *
 * The number and size of the old block is given, and a preference
 * and new size is also specified. The allocator attempts to extend
 * the original block. Failing that, the regular block allocator is
 * invoked to get an appropriate block.
 */
struct buf *
realloccg(ip, bprev, bpref, osize, nsize)
	register struct inode *ip;
	daddr_t bprev, bpref;
	int osize, nsize;
{
	daddr_t bno;
	register struct fs *fs;
	register struct buf *bp, *obp;
	int cg, request;
	
	fs = ip->i_fs;
	if ((unsigned)osize > fs->fs_bsize || fragoff(fs, osize) != 0 ||
	    (unsigned)nsize > fs->fs_bsize || fragoff(fs, nsize) != 0) {
		printf("dev = 0x%x, bsize = %d, osize = %d, nsize = %d, fs = %s\n",
		    ip->i_dev, fs->fs_bsize, osize, nsize, fs->fs_fsmnt);
		panic("realloccg: bad size");
	}
	if (u.u_uid != 0 && freespace(fs, fs->fs_minfree) <= 0)
		goto nospace;
	if (bprev == 0) {
		printf("dev = 0x%x, bsize = %d, bprev = %d, fs = %s\n",
		    ip->i_dev, fs->fs_bsize, bprev, fs->fs_fsmnt);
		panic("realloccg: bad bprev");
	}
#ifdef QUOTA
	u.u_error = chkdq(ip, (long)btodb(nsize - osize), 0);
	if (u.u_error)
		return (NULL);
#endif
	cg = dtog(fs, bprev);
	bno = fragextend(ip, cg, (long)bprev, osize, nsize);
	if (bno != 0) {
		do {
			bp = bread(ip->i_dev, fsbtodb(fs, bno), osize);
			if (bp->b_flags & B_ERROR) {
				brelse(bp);
				return (NULL);
			}
		} while (brealloc(bp, nsize) == 0);
		bp->b_flags |= B_DONE;
		bzero(bp->b_un.b_addr + osize, (unsigned)nsize - osize);
		ip->i_blocks += btodb(nsize - osize);
		ip->i_flag |= IUPD|ICHG;
		return (bp);
	}
	if (bpref >= fs->fs_size)
		bpref = 0;
	if (fs->fs_optim == FS_OPTSPACE)
		request = nsize;
	else /* if (fs->fs_optim == FS_OPTTIME) */
		request = fs->fs_bsize;
	bno = (daddr_t)hashalloc(ip, cg, (long)bpref, request,
		(u_long (*)())alloccg);
	if (bno > 0) {
		obp = bread(ip->i_dev, fsbtodb(fs, bprev), osize);
		if (obp->b_flags & B_ERROR) {
			brelse(obp);
			return (NULL);
		}
		bp = getblk(ip->i_dev, fsbtodb(fs, bno), nsize);
		bcopy(obp->b_un.b_addr, bp->b_un.b_addr, (u_int)osize);
		bzero(bp->b_un.b_addr + osize, (unsigned)nsize - osize);
		if (obp->b_flags & B_DELWRI) {
			obp->b_flags &= ~B_DELWRI;
			u.u_ru.ru_oublock--;		/* delete charge */
		}
		brelse(obp);
		free(ip, bprev, (off_t)osize);
		if (nsize < request)
			free(ip, bno + numfrags(fs, nsize),
				(off_t)(request - nsize));
		ip->i_blocks += btodb(nsize - osize);
		ip->i_flag |= IUPD|ICHG;
		return (bp);
	}
nospace:
	/*
	 * no space available
	 */
#if	CS_RPAUSE
	fsfull(fs, "file system full");
#else	CS_RPAUSE
	fserr(fs, "file system full");
	uprintf("\n%s: write failed, file system is full\n", fs->fs_fsmnt);
	u.u_error = ENOSPC;
#endif	CS_RPAUSE
	return (NULL);
}

/*
 * Allocate an inode in the file system.
 * 
 * A preference may be optionally specified. If a preference is given
 * the following hierarchy is used to allocate an inode:
 *   1) allocate the requested inode.
 *   2) allocate an inode in the same cylinder group.
 *   3) quadradically rehash into other cylinder groups, until an
 *      available inode is located.
 * If no inode preference is given the following heirarchy is used
 * to allocate an inode:
 *   1) allocate an inode in cylinder group 0.
 *   2) quadradically rehash into other cylinder groups, until an
 *      available inode is located.
 */
struct inode *
ialloc(pip, ipref, mode)
	register struct inode *pip;
	ino_t ipref;
	int mode;
{
	ino_t ino;
	register struct fs *fs;
	register struct inode *ip;
	int cg;
	
	fs = pip->i_fs;
#if	CS_OLDFS
	/*
	 * Old format:
	 *
	 * Allocate an unused inode on the specified device.
	 * Used with file creation.  The algorithm keeps up to
	 * NICINOD spare inodes in the super block.  When this runs out,
	 * the inodes are searched to pick up more.  We keep searching
	 * foreward on the device, remembering the number of inodes
	 * which are freed behind our search point for which there is no
	 * room in the in-core table.  When this number passes a threshold
	 * (or if we search to the end of the ilist without finding any inodes)
	 * we restart the search from the beginning.
	 */
	if (isoldfs(fs))
	{
	register struct filsys *fp;
	register struct buf *bp;
	int i;
	struct oinode *oi;
	ino_t inobas;
	int first;
	daddr_t adr;

	fp = fs->fs_oldfs;
	while (fp->s_ilock)
		sleep((caddr_t)&fp->s_ilock, PINOD);
loop:
	if (fp->s_ninode > 0) {
#if	CS_RPAUSE
		if (u.u_uid != 0 && fp->s_tinode <= FS_ILOWAT(fs))
			goto noinodes;
#endif	CS_RPAUSE
		ino = fp->s_inode[--fp->s_ninode];
		ip = iget(pip->i_dev, fs, ino);
		if (ip == NULL)
			return(NULL);
		if (ip->i_mode == 0) {
			for (i=0; i<(NDADDR+NIADDR); i++)
				ip->i_db[i] = 0;
			fs->fs_fmod = 1;
			fp->s_tinode--;
			return(ip);
		}
		/*
		 * Inode was allocated after all.
		 * Look some more.
		 */
		iput(ip);
		goto loop;
	}
	fp->s_ilock++;
	/*
	 * If less than 4*NICINOD inodes are known
	 * to be free behind the current search point,
	 * then search forward; else search from beginning.
	 */
	if (fp->s_nbehind < 4 * NICINOD) {
		first = 1;
		ino = fp->s_lasti;
		if (itooo(fs, ino))
			panic("ialloc");
		adr = itod(fs, ino);
	} else {
fromtop:
		first = 0;
		ino = 1;
		adr = SUPERB+1;
		fp->s_nbehind = 0;
	}
	/*
	 * This is the search for free inodes.
	 */
	for(; adr < fp->s_isize; adr++) {
		inobas = ino;
		bp = bread(pip->i_dev, fsbtodb(fs, adr), (int)fs->fs_bsize);
		if ((bp->b_flags&B_CACHE) == 0)
			u.u_ru.ru_inblock--;		/* no charge! */
		if (bp->b_flags & B_ERROR) {
			brelse(bp);
			ino += INOPB(fs);
			continue;
		}
		oi = (struct oinode *)bp->b_un.b_dino;
		for (i=0; i<INOPB(fs); i++) {
			extern struct inode *ifind();

			if (oi->oi_mode != 0 || ifind(pip->i_dev, ino))
				goto cont;
			fp->s_inode[fp->s_ninode++] = ino;
			if (fp->s_ninode >= NICINOD)
				break;
		cont:
			ino++;
			oi++;
		}
		brelse(bp);
		if (fp->s_ninode >= NICINOD)
			break;
	}
	/*
	 * If the search didn't net a full superblock of inodes,
	 * then try it again from the beginning of the ilist.
	 */
	if (fp->s_ninode < NICINOD && first)
		goto fromtop;
	fp->s_lasti = inobas;
	fp->s_ilock = 0;
	wakeup((caddr_t)&fp->s_ilock);
	if (fp->s_ninode > 0)
		goto loop;
	goto noinodes;
	}
#endif	CS_OLDFS
#if	CS_RPAUSE
	if (u.u_uid != 0 && fs->fs_cstotal.cs_nifree <= FS_ILOWAT(fs))
		goto noinodes;
#endif	CS_RPAUSE
	if (fs->fs_cstotal.cs_nifree == 0)
		goto noinodes;
#ifdef QUOTA
	u.u_error = chkiq(pip->i_dev, (struct inode *)NULL, u.u_uid, 0);
	if (u.u_error)
		return (NULL);
#endif
	if (ipref >= fs->fs_ncg * fs->fs_ipg)
		ipref = 0;
	cg = itog(fs, ipref);
	ino = (ino_t)hashalloc(pip, cg, (long)ipref, mode, ialloccg);
	if (ino == 0)
		goto noinodes;
	ip = iget(pip->i_dev, pip->i_fs, ino);
	if (ip == NULL) {
		ifree(pip, ino, 0);
		return (NULL);
	}
	if (ip->i_mode) {
		printf("mode = 0%o, inum = %d, fs = %s\n",
		    ip->i_mode, ip->i_number, fs->fs_fsmnt);
		panic("ialloc: dup alloc");
	}
	if (ip->i_blocks) {				/* XXX */
		printf("free inode %s/%d had %d blocks\n",
		    fs->fs_fsmnt, ino, ip->i_blocks);
		ip->i_blocks = 0;
	}
#if	NWB_SENS > 0
 	INodeCreate(ip->i_dev,ip->i_number);
#endif	NWB_SENS > 0
	return (ip);
noinodes:
#if	CS_RPAUSE
	fsfull(fs, FS_INOSPC);
#else	CS_RPAUSE
	fserr(fs, "out of inodes");
	uprintf("\n%s: create/symlink failed, no inodes free\n", fs->fs_fsmnt);
	u.u_error = ENOSPC;
#endif	CS_RPAUSE
	return (NULL);
}

/*
 * Find a cylinder to place a directory.
 *
 * The policy implemented by this algorithm is to select from
 * among those cylinder groups with above the average number of
 * free inodes, the one with the smallest number of directories.
 */
ino_t
dirpref(fs)
	register struct fs *fs;
{
	int cg, minndir, mincg, avgifree;

#if	CS_OLDFS
	if (isoldfs(fs))
		return(0);
#endif	CS_OLDFS
	avgifree = fs->fs_cstotal.cs_nifree / fs->fs_ncg;
	minndir = fs->fs_ipg;
	mincg = 0;
	for (cg = 0; cg < fs->fs_ncg; cg++)
		if (fs->fs_cs(fs, cg).cs_ndir < minndir &&
		    fs->fs_cs(fs, cg).cs_nifree >= avgifree) {
			mincg = cg;
			minndir = fs->fs_cs(fs, cg).cs_ndir;
		}
	return ((ino_t)(fs->fs_ipg * mincg));
}

/*
 * Select the desired position for the next block in a file.  The file is
 * logically divided into sections. The first section is composed of the
 * direct blocks. Each additional section contains fs_maxbpg blocks.
 * 
 * If no blocks have been allocated in the first section, the policy is to
 * request a block in the same cylinder group as the inode that describes
 * the file. If no blocks have been allocated in any other section, the
 * policy is to place the section in a cylinder group with a greater than
 * average number of free blocks.  An appropriate cylinder group is found
 * by using a rotor that sweeps the cylinder groups. When a new group of
 * blocks is needed, the sweep begins in the cylinder group following the
 * cylinder group from which the previous allocation was made. The sweep
 * continues until a cylinder group with greater than the average number
 * of free blocks is found. If the allocation is for the first block in an
 * indirect block, the information on the previous allocation is unavailable;
 * here a best guess is made based upon the logical block number being
 * allocated.
 * 
 * If a section is already partially allocated, the policy is to
 * contiguously allocate fs_maxcontig blocks.  The end of one of these
 * contiguous blocks and the beginning of the next is physically separated
 * so that the disk head will be in transit between them for at least
 * fs_rotdelay milliseconds.  This is to allow time for the processor to
 * schedule another I/O transfer.
 */
daddr_t
blkpref(ip, lbn, indx, bap)
	struct inode *ip;
	daddr_t lbn;
	int indx;
	daddr_t *bap;
{
	register struct fs *fs;
	register int cg;
	int avgbfree, startcg;
	daddr_t nextblk;

	fs = ip->i_fs;
	if (indx % fs->fs_maxbpg == 0 || bap[indx - 1] == 0) {
		if (lbn < NDADDR) {
			cg = itog(fs, ip->i_number);
			return (fs->fs_fpg * cg + fs->fs_frag);
		}
		/*
		 * Find a cylinder with greater than average number of
		 * unused data blocks.
		 */
		if (indx == 0 || bap[indx - 1] == 0)
			startcg = itog(fs, ip->i_number) + lbn / fs->fs_maxbpg;
		else
			startcg = dtog(fs, bap[indx - 1]) + 1;
		startcg %= fs->fs_ncg;
		avgbfree = fs->fs_cstotal.cs_nbfree / fs->fs_ncg;
		for (cg = startcg; cg < fs->fs_ncg; cg++)
			if (fs->fs_cs(fs, cg).cs_nbfree >= avgbfree) {
				fs->fs_cgrotor = cg;
				return (fs->fs_fpg * cg + fs->fs_frag);
			}
		for (cg = 0; cg <= startcg; cg++)
			if (fs->fs_cs(fs, cg).cs_nbfree >= avgbfree) {
				fs->fs_cgrotor = cg;
				return (fs->fs_fpg * cg + fs->fs_frag);
			}
		return (NULL);
	}
	/*
	 * One or more previous blocks have been laid out. If less
	 * than fs_maxcontig previous blocks are contiguous, the
	 * next block is requested contiguously, otherwise it is
	 * requested rotationally delayed by fs_rotdelay milliseconds.
	 */
	nextblk = bap[indx - 1] + fs->fs_frag;
	if (indx > fs->fs_maxcontig &&
	    bap[indx - fs->fs_maxcontig] + blkstofrags(fs, fs->fs_maxcontig)
	    != nextblk)
		return (nextblk);
	if (fs->fs_rotdelay != 0)
		/*
		 * Here we convert ms of delay to frags as:
		 * (frags) = (ms) * (rev/sec) * (sect/rev) /
		 *	((sect/frag) * (ms/sec))
		 * then round up to the next block.
		 */
		nextblk += roundup(fs->fs_rotdelay * fs->fs_rps * fs->fs_nsect /
		    (NSPF(fs) * 1000), fs->fs_frag);
	return (nextblk);
}

/*
 * Implement the cylinder overflow algorithm.
 *
 * The policy implemented by this algorithm is:
 *   1) allocate the block in its requested cylinder group.
 *   2) quadradically rehash on the cylinder group number.
 *   3) brute force search for a free block.
 */
/*VARARGS5*/
u_long
hashalloc(ip, cg, pref, size, allocator)
	struct inode *ip;
	int cg;
	long pref;
	int size;	/* size for data blocks, mode for inodes */
	u_long (*allocator)();
{
	register struct fs *fs;
	long result;
	int i, icg = cg;

	fs = ip->i_fs;
	/*
	 * 1: preferred cylinder group
	 */
	result = (*allocator)(ip, cg, pref, size);
	if (result)
		return (result);
	/*
	 * 2: quadratic rehash
	 */
	for (i = 1; i < fs->fs_ncg; i *= 2) {
		cg += i;
		if (cg >= fs->fs_ncg)
			cg -= fs->fs_ncg;
		result = (*allocator)(ip, cg, 0, size);
		if (result)
			return (result);
	}
	/*
	 * 3: brute force search
	 * Note that we start at i == 2, since 0 was checked initially,
	 * and 1 is always checked in the quadratic rehash.
	 */
	cg = (icg + 2) % fs->fs_ncg;
	for (i = 2; i < fs->fs_ncg; i++) {
		result = (*allocator)(ip, cg, 0, size);
		if (result)
			return (result);
		cg++;
		if (cg == fs->fs_ncg)
			cg = 0;
	}
	return (NULL);
}

/*
 * Determine whether a fragment can be extended.
 *
 * Check to see if the necessary fragments are available, and 
 * if they are, allocate them.
 */
daddr_t
fragextend(ip, cg, bprev, osize, nsize)
	struct inode *ip;
	int cg;
	long bprev;
	int osize, nsize;
{
	register struct fs *fs;
	register struct buf *bp;
	register struct cg *cgp;
	long bno;
	int frags, bbase;
	int i;

	fs = ip->i_fs;
	if (fs->fs_cs(fs, cg).cs_nffree < numfrags(fs, nsize - osize))
		return (NULL);
	frags = numfrags(fs, nsize);
	bbase = fragnum(fs, bprev);
	if (bbase > fragnum(fs, (bprev + frags - 1))) {
		/* cannot extend across a block boundry */
		return (NULL);
	}
	bp = bread(ip->i_dev, fsbtodb(fs, cgtod(fs, cg)), (int)fs->fs_cgsize);
	cgp = bp->b_un.b_cg;
	if (bp->b_flags & B_ERROR || cgp->cg_magic != CG_MAGIC) {
		brelse(bp);
		return (NULL);
	}
	cgp->cg_time = time.tv_sec;
	bno = dtogd(fs, bprev);
	for (i = numfrags(fs, osize); i < frags; i++)
		if (isclr(cgp->cg_free, bno + i)) {
			brelse(bp);
			return (NULL);
		}
	/*
	 * the current fragment can be extended
	 * deduct the count on fragment being extended into
	 * increase the count on the remaining fragment (if any)
	 * allocate the extended piece
	 */
	for (i = frags; i < fs->fs_frag - bbase; i++)
		if (isclr(cgp->cg_free, bno + i))
			break;
	cgp->cg_frsum[i - numfrags(fs, osize)]--;
	if (i != frags)
		cgp->cg_frsum[i - frags]++;
	for (i = numfrags(fs, osize); i < frags; i++) {
		clrbit(cgp->cg_free, bno + i);
		cgp->cg_cs.cs_nffree--;
		fs->fs_cstotal.cs_nffree--;
		fs->fs_cs(fs, cg).cs_nffree--;
	}
	fs->fs_fmod++;
	bdwrite(bp);
	return (bprev);
}

/*
 * Determine whether a block can be allocated.
 *
 * Check to see if a block of the apprpriate size is available,
 * and if it is, allocate it.
 */
daddr_t
alloccg(ip, cg, bpref, size)
	struct inode *ip;
	int cg;
	daddr_t bpref;
	int size;
{
	register struct fs *fs;
	register struct buf *bp;
	register struct cg *cgp;
	int bno, frags;
	int allocsiz;
	register int i;

	fs = ip->i_fs;
	if (fs->fs_cs(fs, cg).cs_nbfree == 0 && size == fs->fs_bsize)
		return (NULL);
	bp = bread(ip->i_dev, fsbtodb(fs, cgtod(fs, cg)), (int)fs->fs_cgsize);
	cgp = bp->b_un.b_cg;
	if (bp->b_flags & B_ERROR || cgp->cg_magic != CG_MAGIC ||
	    (cgp->cg_cs.cs_nbfree == 0 && size == fs->fs_bsize)) {
		brelse(bp);
		return (NULL);
	}
	cgp->cg_time = time.tv_sec;
	if (size == fs->fs_bsize) {
		bno = alloccgblk(fs, cgp, bpref);
		bdwrite(bp);
		return (bno);
	}
	/*
	 * check to see if any fragments are already available
	 * allocsiz is the size which will be allocated, hacking
	 * it down to a smaller size if necessary
	 */
	frags = numfrags(fs, size);
	for (allocsiz = frags; allocsiz < fs->fs_frag; allocsiz++)
		if (cgp->cg_frsum[allocsiz] != 0)
			break;
	if (allocsiz == fs->fs_frag) {
		/*
		 * no fragments were available, so a block will be 
		 * allocated, and hacked up
		 */
		if (cgp->cg_cs.cs_nbfree == 0) {
			brelse(bp);
			return (NULL);
		}
		bno = alloccgblk(fs, cgp, bpref);
		bpref = dtogd(fs, bno);
		for (i = frags; i < fs->fs_frag; i++)
			setbit(cgp->cg_free, bpref + i);
		i = fs->fs_frag - frags;
		cgp->cg_cs.cs_nffree += i;
		fs->fs_cstotal.cs_nffree += i;
		fs->fs_cs(fs, cg).cs_nffree += i;
		fs->fs_fmod++;
		cgp->cg_frsum[i]++;
		bdwrite(bp);
		return (bno);
	}
	bno = mapsearch(fs, cgp, bpref, allocsiz);
	if (bno < 0) {
		brelse(bp);
		return (NULL);
	}
	for (i = 0; i < frags; i++)
		clrbit(cgp->cg_free, bno + i);
	cgp->cg_cs.cs_nffree -= frags;
	fs->fs_cstotal.cs_nffree -= frags;
	fs->fs_cs(fs, cg).cs_nffree -= frags;
	fs->fs_fmod++;
	cgp->cg_frsum[allocsiz]--;
	if (frags != allocsiz)
		cgp->cg_frsum[allocsiz - frags]++;
	bdwrite(bp);
	return (cg * fs->fs_fpg + bno);
}

/*
 * Allocate a block in a cylinder group.
 *
 * This algorithm implements the following policy:
 *   1) allocate the requested block.
 *   2) allocate a rotationally optimal block in the same cylinder.
 *   3) allocate the next available block on the block rotor for the
 *      specified cylinder group.
 * Note that this routine only allocates fs_bsize blocks; these
 * blocks may be fragmented by the routine that allocates them.
 */
daddr_t
alloccgblk(fs, cgp, bpref)
	register struct fs *fs;
	register struct cg *cgp;
	daddr_t bpref;
{
	daddr_t bno;
	int cylno, pos, delta;
	short *cylbp;
	register int i;

	if (bpref == 0) {
		bpref = cgp->cg_rotor;
		goto norot;
	}
	bpref = blknum(fs, bpref);
	bpref = dtogd(fs, bpref);
	/*
	 * if the requested block is available, use it
	 */
	if (isblock(fs, cgp->cg_free, fragstoblks(fs, bpref))) {
		bno = bpref;
		goto gotit;
	}
	/*
	 * check for a block available on the same cylinder
	 */
	cylno = cbtocylno(fs, bpref);
	if (cgp->cg_btot[cylno] == 0)
		goto norot;
	if (fs->fs_cpc == 0) {
		/*
		 * block layout info is not available, so just have
		 * to take any block in this cylinder.
		 */
		bpref = howmany(fs->fs_spc * cylno, NSPF(fs));
		goto norot;
	}
	/*
	 * check the summary information to see if a block is 
	 * available in the requested cylinder starting at the
	 * requested rotational position and proceeding around.
	 */
	cylbp = cgp->cg_b[cylno];
	pos = cbtorpos(fs, bpref);
	for (i = pos; i < NRPOS; i++)
		if (cylbp[i] > 0)
			break;
	if (i == NRPOS)
		for (i = 0; i < pos; i++)
			if (cylbp[i] > 0)
				break;
	if (cylbp[i] > 0) {
		/*
		 * found a rotational position, now find the actual
		 * block. A panic if none is actually there.
		 */
		pos = cylno % fs->fs_cpc;
		bno = (cylno - pos) * fs->fs_spc / NSPB(fs);
		if (fs->fs_postbl[pos][i] == -1) {
			printf("pos = %d, i = %d, fs = %s\n",
			    pos, i, fs->fs_fsmnt);
			panic("alloccgblk: cyl groups corrupted");
		}
		for (i = fs->fs_postbl[pos][i];; ) {
			if (isblock(fs, cgp->cg_free, bno + i)) {
				bno = blkstofrags(fs, (bno + i));
				goto gotit;
			}
			delta = fs->fs_rotbl[i];
			if (delta <= 0 || delta > MAXBPC - i)
				break;
			i += delta;
		}
		printf("pos = %d, i = %d, fs = %s\n", pos, i, fs->fs_fsmnt);
		panic("alloccgblk: can't find blk in cyl");
	}
norot:
	/*
	 * no blocks in the requested cylinder, so take next
	 * available one in this cylinder group.
	 */
	bno = mapsearch(fs, cgp, bpref, (int)fs->fs_frag);
	if (bno < 0)
		return (NULL);
	cgp->cg_rotor = bno;
gotit:
	clrblock(fs, cgp->cg_free, (long)fragstoblks(fs, bno));
	cgp->cg_cs.cs_nbfree--;
	fs->fs_cstotal.cs_nbfree--;
	fs->fs_cs(fs, cgp->cg_cgx).cs_nbfree--;
	cylno = cbtocylno(fs, bno);
	cgp->cg_b[cylno][cbtorpos(fs, bno)]--;
	cgp->cg_btot[cylno]--;
	fs->fs_fmod++;
	return (cgp->cg_cgx * fs->fs_fpg + bno);
}
	
/*
 * Determine whether an inode can be allocated.
 *
 * Check to see if an inode is available, and if it is,
 * allocate it using the following policy:
 *   1) allocate the requested inode.
 *   2) allocate the next available inode after the requested
 *      inode in the specified cylinder group.
 */
ino_t
ialloccg(ip, cg, ipref, mode)
	struct inode *ip;
	int cg;
	daddr_t ipref;
	int mode;
{
	register struct fs *fs;
	register struct cg *cgp;
	struct buf *bp;
	int start, len, loc, map, i;

	fs = ip->i_fs;
	if (fs->fs_cs(fs, cg).cs_nifree == 0)
		return (NULL);
	bp = bread(ip->i_dev, fsbtodb(fs, cgtod(fs, cg)), (int)fs->fs_cgsize);
	cgp = bp->b_un.b_cg;
	if (bp->b_flags & B_ERROR || cgp->cg_magic != CG_MAGIC ||
	    cgp->cg_cs.cs_nifree == 0) {
		brelse(bp);
		return (NULL);
	}
	cgp->cg_time = time.tv_sec;
	if (ipref) {
		ipref %= fs->fs_ipg;
		if (isclr(cgp->cg_iused, ipref))
			goto gotit;
	}
	start = cgp->cg_irotor / NBBY;
	len = howmany(fs->fs_ipg - cgp->cg_irotor, NBBY);
	loc = skpc(0xff, len, &cgp->cg_iused[start]);
	if (loc == 0) {
		len = start + 1;
		start = 0;
		loc = skpc(0xff, len, &cgp->cg_iused[0]);
		if (loc == 0) {
			printf("cg = %s, irotor = %d, fs = %s\n",
			    cg, cgp->cg_irotor, fs->fs_fsmnt);
			panic("ialloccg: map corrupted");
			/* NOTREACHED */
		}
	}
	i = start + len - loc;
	map = cgp->cg_iused[i];
	ipref = i * NBBY;
	for (i = 1; i < (1 << NBBY); i <<= 1, ipref++) {
		if ((map & i) == 0) {
			cgp->cg_irotor = ipref;
			goto gotit;
		}
	}
	printf("fs = %s\n", fs->fs_fsmnt);
	panic("ialloccg: block not in map");
	/* NOTREACHED */
gotit:
	setbit(cgp->cg_iused, ipref);
	cgp->cg_cs.cs_nifree--;
	fs->fs_cstotal.cs_nifree--;
	fs->fs_cs(fs, cg).cs_nifree--;
	fs->fs_fmod++;
	if ((mode & IFMT) == IFDIR) {
		cgp->cg_cs.cs_ndir++;
		fs->fs_cstotal.cs_ndir++;
		fs->fs_cs(fs, cg).cs_ndir++;
	}
	bdwrite(bp);
	return (cg * fs->fs_ipg + ipref);
}

/*
 * Free a block or fragment.
 *
 * The specified block or fragment is placed back in the
 * free map. If a fragment is deallocated, a possible 
 * block reassembly is checked.
 */
free(ip, bno, size)
	register struct inode *ip;
	daddr_t bno;
	off_t size;
{
	register struct fs *fs;
	register struct cg *cgp;
	register struct buf *bp;
	int cg, blk, frags, bbase;
	register int i;

	fs = ip->i_fs;
#if	CS_OLDFS
	/*
 	 * old format:
	 *
	 * place the specified disk block
	 * back on the free list of the
	 * specified device.
	 */
	if (isoldfs(fs))
	{
	register struct filsys *fp;

	fp = fs->fs_oldfs;
	fs->fs_fmod = 1;
	while (fp->s_flock)
		sleep((caddr_t)&fp->s_flock, PINOD);
	if (badblock(fs, bno))
		return;
	if (fp->s_nfree <= 0) {
		fp->s_nfree = 1;
		fp->s_free[0] = 0;
	}
	if (fp->s_nfree >= NICFREE) {
		fp->s_flock++;
		bp = getblk(ip->i_dev, fsbtodb(fs, bno), (int)fs->fs_bsize);
		((FBLKP)(bp->b_un.b_addr))->df_nfree = fp->s_nfree;
		bcopy((caddr_t)fp->s_free,
		    (caddr_t)((FBLKP)(bp->b_un.b_addr))->df_free,
		    sizeof(fp->s_free));
		fp->s_nfree = 0;
		bwrite(bp);
		fp->s_flock = 0;
		wakeup((caddr_t)&fp->s_flock);
	}
	fp->s_free[fp->s_nfree++] = bno;
	++(fp->s_tfree);
	fs->fs_fmod = 1;
#if	CS_RPAUSE
	goto out;
#else	CS_RPAUSE
	return;
#endif	CS_RPAUSE
	}
#endif	CS_OLDFS
	if ((unsigned)size > fs->fs_bsize || fragoff(fs, size) != 0) {
		printf("dev = 0x%x, bsize = %d, size = %d, fs = %s\n",
		    ip->i_dev, fs->fs_bsize, size, fs->fs_fsmnt);
		panic("free: bad size");
	}
	cg = dtog(fs, bno);
	if (badblock(fs, bno)) {
		printf("bad block %d, ino %d\n", bno, ip->i_number);
		return;
	}
	bp = bread(ip->i_dev, fsbtodb(fs, cgtod(fs, cg)), (int)fs->fs_cgsize);
	cgp = bp->b_un.b_cg;
	if (bp->b_flags & B_ERROR || cgp->cg_magic != CG_MAGIC) {
		brelse(bp);
		return;
	}
	cgp->cg_time = time.tv_sec;
	bno = dtogd(fs, bno);
	if (size == fs->fs_bsize) {
		if (isblock(fs, cgp->cg_free, fragstoblks(fs, bno))) {
			printf("dev = 0x%x, block = %d, fs = %s\n",
			    ip->i_dev, bno, fs->fs_fsmnt);
			panic("free: freeing free block");
		}
		setblock(fs, cgp->cg_free, fragstoblks(fs, bno));
		cgp->cg_cs.cs_nbfree++;
		fs->fs_cstotal.cs_nbfree++;
		fs->fs_cs(fs, cg).cs_nbfree++;
		i = cbtocylno(fs, bno);
		cgp->cg_b[i][cbtorpos(fs, bno)]++;
		cgp->cg_btot[i]++;
	} else {
		bbase = bno - fragnum(fs, bno);
		/*
		 * decrement the counts associated with the old frags
		 */
		blk = blkmap(fs, cgp->cg_free, bbase);
		fragacct(fs, blk, cgp->cg_frsum, -1);
		/*
		 * deallocate the fragment
		 */
		frags = numfrags(fs, size);
		for (i = 0; i < frags; i++) {
			if (isset(cgp->cg_free, bno + i)) {
				printf("dev = 0x%x, block = %d, fs = %s\n",
				    ip->i_dev, bno + i, fs->fs_fsmnt);
				panic("free: freeing free frag");
			}
			setbit(cgp->cg_free, bno + i);
		}
		cgp->cg_cs.cs_nffree += i;
		fs->fs_cstotal.cs_nffree += i;
		fs->fs_cs(fs, cg).cs_nffree += i;
		/*
		 * add back in counts associated with the new frags
		 */
		blk = blkmap(fs, cgp->cg_free, bbase);
		fragacct(fs, blk, cgp->cg_frsum, 1);
		/*
		 * if a complete block has been reassembled, account for it
		 */
		if (isblock(fs, cgp->cg_free, fragstoblks(fs, bbase))) {
			cgp->cg_cs.cs_nffree -= fs->fs_frag;
			fs->fs_cstotal.cs_nffree -= fs->fs_frag;
			fs->fs_cs(fs, cg).cs_nffree -= fs->fs_frag;
			cgp->cg_cs.cs_nbfree++;
			fs->fs_cstotal.cs_nbfree++;
			fs->fs_cs(fs, cg).cs_nbfree++;
			i = cbtocylno(fs, bbase);
			cgp->cg_b[i][cbtorpos(fs, bbase)]++;
			cgp->cg_btot[i]++;
		}
	}
	fs->fs_fmod++;
	bdwrite(bp);
#if	CS_RPAUSE
out:
	if ((fs->fs_flags&FS_FNOSPC) && freefrags(fs) > FS_FHIWAT(fs))
	{
		wakeup((caddr_t)&fs->fs_cstotal.cs_nffree);
		fs->fs_flags &= ~FS_FNOSPC;
	}
#endif	CS_RPAUSE
}

/*
 * Free an inode.
 *
 * The specified inode is placed back in the free map.
 */
ifree(ip, ino, mode)
	struct inode *ip;
	ino_t ino;
	int mode;
{
	register struct fs *fs;
	register struct cg *cgp;
	register struct buf *bp;
	int cg;

	fs = ip->i_fs;
#if	CS_OLDFS
	/*
	 * Old format:
	 *
	 * Free the specified inode on the specified device.
	 * The algorithm stores up to NICINOD inodes in the super
	 * block and throws away any more.  It keeps track of the
	 * number of inodes thrown away which preceded the current
	 * search point in the file system.  This lets us rescan
	 * for more inodes from the beginning only when there
	 * are a reasonable number of inodes back there to reallocate.
	 */
	if (isoldfs(fs))
	{
	register struct filsys *fp;

	fp = fs->fs_oldfs;
	fp->s_tinode++;
	if (fp->s_ilock)
#if	CS_RPAUSE
		goto out;
#else	CS_RPAUSE
		return;
#endif	CS_RPAUSE
	if (fp->s_ninode >= NICINOD) {
		if (fp->s_lasti > ino)
			fp->s_nbehind++;
#if	CS_RPAUSE
		goto out;
#else	CS_RPAUSE
		return;
#endif	CS_RPAUSE
	}
	fp->s_inode[fp->s_ninode++] = ino;
	fs->fs_fmod = 1;
#if	CS_RPAUSE
	goto out;
#else	CS_RPAUSE
	return;
#endif	CS_RPAUSE
	}
#endif	CS_OLDFS
	if ((unsigned)ino >= fs->fs_ipg*fs->fs_ncg) {
		printf("dev = 0x%x, ino = %d, fs = %s\n",
		    ip->i_dev, ino, fs->fs_fsmnt);
		panic("ifree: range");
	}
#if	NWB_SENS > 0
	INodeDelete(ip->i_dev,ip->i_number);
#endif	NWB_SENS > 0
	cg = itog(fs, ino);
	bp = bread(ip->i_dev, fsbtodb(fs, cgtod(fs, cg)), (int)fs->fs_cgsize);
	cgp = bp->b_un.b_cg;
	if (bp->b_flags & B_ERROR || cgp->cg_magic != CG_MAGIC) {
		brelse(bp);
		return;
	}
	cgp->cg_time = time.tv_sec;
	ino %= fs->fs_ipg;
	if (isclr(cgp->cg_iused, ino)) {
		printf("dev = 0x%x, ino = %d, fs = %s\n",
		    ip->i_dev, ino, fs->fs_fsmnt);
		panic("ifree: freeing free inode");
	}
	clrbit(cgp->cg_iused, ino);
	if (ino < cgp->cg_irotor)
		cgp->cg_irotor = ino;
	cgp->cg_cs.cs_nifree++;
	fs->fs_cstotal.cs_nifree++;
	fs->fs_cs(fs, cg).cs_nifree++;
	if ((mode & IFMT) == IFDIR) {
		cgp->cg_cs.cs_ndir--;
		fs->fs_cstotal.cs_ndir--;
		fs->fs_cs(fs, cg).cs_ndir--;
	}
	fs->fs_fmod++;
	bdwrite(bp);
#if	CS_RPAUSE
out:
	if ((fs->fs_flags&FS_INOSPC) && freeinodes(fs) > FS_IHIWAT(fs))
	{
		wakeup((caddr_t)&fs->fs_cstotal.cs_nifree);
		fs->fs_flags &= ~FS_INOSPC;
	}
#endif	CS_RPAUSE
}

/*
 * Find a block of the specified size in the specified cylinder group.
 *
 * It is a panic if a request is made to find a block if none are
 * available.
 */
daddr_t
mapsearch(fs, cgp, bpref, allocsiz)
	register struct fs *fs;
	register struct cg *cgp;
	daddr_t bpref;
	int allocsiz;
{
	daddr_t bno;
	int start, len, loc, i;
	int blk, field, subfield, pos;

	/*
	 * find the fragment by searching through the free block
	 * map for an appropriate bit pattern
	 */
	if (bpref)
		start = dtogd(fs, bpref) / NBBY;
	else
		start = cgp->cg_frotor / NBBY;
	len = howmany(fs->fs_fpg, NBBY) - start;
	loc = scanc((unsigned)len, (caddr_t)&cgp->cg_free[start],
		(caddr_t)fragtbl[fs->fs_frag],
		(int)(1 << (allocsiz - 1 + (fs->fs_frag % NBBY))));
	if (loc == 0) {
		len = start + 1;
		start = 0;
		loc = scanc((unsigned)len, (caddr_t)&cgp->cg_free[0],
			(caddr_t)fragtbl[fs->fs_frag],
			(int)(1 << (allocsiz - 1 + (fs->fs_frag % NBBY))));
		if (loc == 0) {
			printf("start = %d, len = %d, fs = %s\n",
			    start, len, fs->fs_fsmnt);
			panic("alloccg: map corrupted");
			/* NOTREACHED */
		}
	}
	bno = (start + len - loc) * NBBY;
	cgp->cg_frotor = bno;
	/*
	 * found the byte in the map
	 * sift through the bits to find the selected frag
	 */
	for (i = bno + NBBY; bno < i; bno += fs->fs_frag) {
		blk = blkmap(fs, cgp->cg_free, bno);
		blk <<= 1;
		field = around[allocsiz];
		subfield = inside[allocsiz];
		for (pos = 0; pos <= fs->fs_frag - allocsiz; pos++) {
			if ((blk & field) == subfield)
				return (bno + pos);
			field <<= 1;
			subfield <<= 1;
		}
	}
	printf("bno = %d, fs = %s\n", bno, fs->fs_fsmnt);
	panic("alloccg: block not in map");
	return (-1);
}

/*
 * Fserr prints the name of a file system with an error diagnostic.
 * 
 * The form of the error message is:
 *	fs: error message
 */
fserr(fs, cp)
	struct fs *fs;
	char *cp;
{

	log(LOG_ERR, "%s: %s\n", fs->fs_fsmnt, cp);
}
