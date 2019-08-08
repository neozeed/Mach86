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
 *	File:	inode_pager.c
 *	Author:	David Golub
 *
 *	Copyright (C) 1986, David Golub
 *
 *	"Swap" pager that pages to/from Unix inodes.  Also
 *	handles demand paging from files.
 *
 * HISTORY
 *  7-Jun-86  Avadis Tevanian (avie) at Carnegie-Mellon University
 *	Detect sequential reads in pagein and use readahead if
 *	appropriate.
 *
 *  6-Jun-86  Avadis Tevanian (avie) at Carnegie-Mellon University
 *	Changed iswap_allocate to never panic.  Instead it will return a
 *	null pager id, which will delay the inevitable.
 *
 * 31-May-86  Avadis Tevanian (avie) at Carnegie-Mellon University
 *	Set the pager_id field in the inode structure.  This is a back
 *	pointer to the real pager id which will allow for pager id's to
 *	be different than inode numbers.
 *
 * 23-May-86  David Golub (dbg) at Carnegie-Mellon University
 *	Restored the text pager so that text files can be protected from
 *	writing (the rest of unix looks at the ITEXT bit to keep from
 *	overwriting text files in use).  Header file inode_pager.h defines
 *	the pagein/pageout routines to be the same for inode and text
 *	pagers.
 *
 * 19-May-86  Avadis Tevanian (avie) at Carnegie-Mellon University
 *	Purge "text" pager, the "inode" pager can handle text.
 *
 * 19-May-86  Avadis Tevanian (avie) at Carnegie-Mellon University
 *	Make sure that the appropriate calls are running on the master.
 *	We do this here now since the VM code is allowed to run in
 *	parallel.
 *
 * 17-May-86  Avadis Tevanian (avie) at Carnegie-Mellon University
 *	Call pmap_clear_modify after paging out a page, this is the
 *	wrong place, but suffices for now.
 *
 *  5-Apr-86  Avadis Tevanian (avie) at Carnegie-Mellon University
 *	Skip the root file system when searching for a place to page out
 *	to.
 *
 * 28-Mar-86  David Golub (dbg) at Carnegie-Mellon University
 *	inode_dealloc and text_dealloc now just call irele.
 *	(No point in locking to call iput, since iput just
 *	unlocks and calls irele!)
 *
 * 12-Mar-86  David Golub (dbg) at Carnegie-Mellon University
 *	Created.
 */

#include "cs_oldfs.h"

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/map.h"
#include "../h/dir.h"
#include "../h/user.h"
#include "../h/fs.h"
#include "../h/buf.h"
#include "../h/uio.h"
#include "../h/inode.h"
#include "../h/mount.h"

#include "../vm/vm_page.h"
#include "../vm/vm_map.h"
#include "../vm/vm_kern.h"

#if	CS_OLDFS
#include "../h/filsys.h"
#endif	CS_OLDFS

#include "../machine/cpu.h"

/*
 *	(unix) Disk files are allocated by blocks.  A block is (I assume)
 *	some multiple of the page size.  We don't allocate a partial block;
 *	therefore, an inode used for shadowing another object must shadow
 *	an entire (fs) block's worth of that object.  Pageout may therefore
 *	have to ask the pager for the other pages that make up that block;
 *	in turn, the pager may have to ask the pagein routine for the shadowed
 *	object for those pages (!).
 *
 *	Not quite true.  The block size may instead be a multiple of the page
 *	size.  The SUN3 has 8K pages - what if we page out to a file system
 *	with 4K blocks?
 *
 *	To further complicate matters, the offset may not be a multiple of
 *	either... the exec-file pager starts at 1K bytes into the file.
 *	With a 2K page size and a 4K block size, the 1K doesn't fit anywhere.
 *	At least we don't page out to these things.
 */

/*
 *	Pagein
 *	Parameters:
 *
 *	m:	physical memory structure describing page allocated
 *		to page into
 */

boolean_t inode_pagein(m)
	vm_page_t	m;		/* page to read */

{
	register struct inode	*ip;
	vm_offset_t	f_offset;	/* byte offset within file block */
	vm_offset_t	p_offset;	/* byte offset within physical page */
	dev_t		dev;
	register struct fs	*fs;
	daddr_t		lbn, bn;
	int		size;
	long		bsize;
	int		csize, on, n, save_error, err, diff;
	struct buf	*bp;

	/*
	 *	Get the inode and the offset within it to read from.
	 *	Lock the inode while we play with it.
	 */
	ip = (struct inode *)m->object->paging_space;
	f_offset = m->offset + m->object->paging_offset;

	if (cpu_number() != master_cpu)
		unix_swtch(u.u_procp, 1);

	ilock(ip);

	p_offset = 0;

	dev = ip->i_dev;
	fs = ip->i_fs;
	bsize = fs->fs_bsize;
	csize = page_size;

	/*
	 * Be sure that data not in the file is zero filled.
	 * The easiest way to do this is to zero the entire
	 * block now.
	 */

	if (ip->i_size < (f_offset + csize))
		vm_page_zero_fill(m);

	/*
	 *	Read from the inode until we've filled the page.
	 */
	do {
		/*
		 *	Find block and offset within it for our data.
		 */
		lbn = lblkno(fs, f_offset);	/* logical block number */
		on  = blkoff(fs, f_offset);	/* byte offset within block */

		/*
		 *	Find the size we can read - don't go beyond the
		 *	end of a block.
		 */
		n = MIN((unsigned)(bsize - on), csize);
		diff = ip->i_size - f_offset;
		if (diff <= 0) {
			if (p_offset == 0) {
				/*
				 * entire block beyond end of file -
				 * doesn't exist
				 */
				iunlock(ip);
				return FALSE;
			}
			/*
			 * block partially there - zero the rest of it
			 */
			break;
		}
		if (diff < n)
			n = diff;

		/*
		 *	Read the index to find the disk block to read
		 *	from.  If there is no block, report that we don't
		 *	have this data.
		 *
		 *	!!! Assumes that:
		 *		1) Any offset will be on a fragment boundary
		 *		2) The inode won't have just part of a page
		 */
		save_error = u.u_error;
		/* changes u.u_error! */
		bn = fsbtodb(fs,
			bmap(ip, lbn, B_READ, (int)(on+n) ));
		err = u.u_error;
		u.u_error = save_error;

		if (err) {
			panic("IO error on swap_pagein");
		}

		if ((long)bn < 0) {
			iunlock(ip);
			return(FALSE);	/* page not in inode */
		}

		/*
		 *	Read the block through the buffer pool,
		 *	then copy it to the physical memory already
		 *	allocated for this page.
		 */
		size = blksize(fs, ip, lbn);
		if ((ip->i_lastr + 1) == lbn)
			bp = breada(dev, bn, size, rablock, rasize);
		else
			bp = bread(dev, bn, size);
		ip->i_lastr = lbn;

		n = MIN(n, size - bp->b_resid);
		if (bp->b_flags & B_ERROR) {
			brelse(bp);
			panic("IO error on swap_pagein");
		}
		copy_to_phys(bp->b_un.b_addr+on,
				VM_PAGE_TO_PHYS(m) + p_offset,
				n);

		/*
		 *	Account for how much we've read this time
		 *	around.
		 */
		csize -= n;
		p_offset += n;
		f_offset += n;

		if (n + on == bsize || f_offset == ip->i_size)
			bp->b_flags |= B_AGE;
		brelse(bp);

	} while (csize > 0 && n != 0);
	iunlock(ip);
	return(TRUE);
}

inode_pageout(m)
	vm_page_t	m;

{
	register struct inode	*ip;
	vm_offset_t	f_offset;	/* byte offset within file block */
	vm_offset_t	p_offset;	/* byte offset within physical page */
	dev_t		dev;
	register struct fs	*fs;
	daddr_t		lbn, bn;
	int		size;
	long		bsize;
	int		csize, on, n, save_error, err;
	struct buf	*bp;

	ip = (struct inode *)m->object->paging_space;
	f_offset = m->offset + m->object->paging_offset;

	if (cpu_number() != master_cpu)
		unix_swtch(u.u_procp, 1);

	ilock(ip);

	p_offset = 0;

	dev = ip->i_dev;
	fs = ip->i_fs;
	bsize = fs->fs_bsize;
	csize = page_size;

	do {
		lbn = lblkno(fs, f_offset);	/* logical block number */
		on  = blkoff(fs, f_offset);	/* byte offset within block */

		n   = MIN((unsigned)(bsize - on), csize);

		save_error = u.u_error;
		/* changes u.u_error! */
		bn = fsbtodb(fs,
			bmap(ip, lbn, B_WRITE, (int)(on+n) ));
		err = u.u_error;
		u.u_error = save_error;

		if (err || (long) bn < 0) {
			panic("IO error on swap_pageout");
		}

		if (f_offset + n > ip->i_size)
			ip->i_size = f_offset + n;

		size = blksize(fs, ip, lbn);

		if (n == bsize)
			bp = getblk(dev, bn, size);
		else
			bp = bread(dev, bn, size);

		n = MIN(n, size - bp->b_resid);
		if (bp->b_flags & B_ERROR) {
			brelse(bp);
			panic("IO error on swap_pageout");
		}
		copy_from_phys(VM_PAGE_TO_PHYS(m) + p_offset,
				bp->b_un.b_addr+on,
				n);
		csize -= n;
		p_offset += n;
		f_offset += n;

		if (n + on == bsize) {
			bp->b_flags |= B_AGE;
			bawrite(bp);
		}
		else
			bdwrite(bp);
		ip->i_flag |= IUPD|ICHG;

	} while (csize > 0 && n != 0);
	m->clean = TRUE;			/* XXX - wrong place */
	pmap_clear_modify(VM_PAGE_TO_PHYS(m));	/* XXX - wrong place */
	iunlock(ip);
}

/*
 *	Thanks to Mike Accetta for this stuff
 */

/*
 *	Allocate an inode for paging out a shadow object.
 *	Takes the cluster size for the object it is shadowing.
 *	The paging id returned must have a cluster size <=
 *	the cluster size of the object being shadowed...
 *	if this is not true, one "cluster" of the object may
 *	have to shadow several pages of the shadowed object,
 *	thus making page-out do page-ins!
 *
 *	If the object being shadowed is on a file system,
 *	then there is at least one file system (the right one)
 *	with a suitable cluster size... though it may be full.
 *
 *	The inode is returned unlocked.
 */
vm_pager_id_t iswap_allocate(bsize)
	vm_size_t	bsize;
{
	struct inode	*ip, *pip;

	int 		mostf;
	int 		mostidx, midx;
	struct mount	*mp;
	struct fs	*fs;

	/*
	 *	Look thru all mounted file systems for the one with
	 *	the most free space.  It should also have the largest
	 *	block size <= shadowed_cluster_size.
	 *
	 *	The search loop starts at 1 to skip the root.
	 */

	mostidx = -1;
	mostf   = 0;

	if (cpu_number() != master_cpu)
		unix_swtch(u.u_procp, 1);

	for (midx = 1; midx < NMOUNT; midx++) {
		int ffrags;

		mp = &mount[midx];
		if (mp->m_bufp == NULL || mp->m_dev == NODEV)
			continue;

		fs = mp->m_bufp->b_un.b_fs;
		if (fs->fs_ronly)
			continue;
		if (fs->fs_bsize > bsize)
			continue;
		ffrags = freespace(fs, fs->fs_minfree);
		if (ffrags > mostf) {
			mostf = ffrags;
			mostidx = midx;
		}
	}
	if (mostidx == -1) {
		return(VM_PAGER_ID_NULL);
	}

	fs = mount[mostidx].m_bufp->b_un.b_fs;
	pip = iget(mount[mostidx].m_dev, fs, ROOTINO);
	if (pip == NULL) {
		return(VM_PAGER_ID_NULL);
	}

	ip = ialloc(pip, (ino_t)0, 0);
	iput(pip);
	if (ip == NULL) {
		return(VM_PAGER_ID_NULL);
	}

	ip->i_flag |= IACC|IUPD|ICHG;
	ip->i_nlink = 0;	/* fsck will remove */
	ip->i_uid = 0;		/* which user? */
	ip->i_gid = 0;
	ip->i_mode = IFREG;
	iunlock(ip);

	return((vm_pager_id_t) ip);
}


/*
 *	Paging to/from inode - file
 */

void	inode_pager_init()
{
}

vm_pager_id_t inode_alloc(size)
	vm_size_t	size;
{
	return(iswap_allocate(page_size));
}

vm_pager_id_t inode_pager_setup(ip)
	struct inode	*ip;
{
#if	CS_ICHK
	iincr_chk(ip);
#else	CS_ICHK
	ip->i_count++;
#endif	CS_ICHK
	ip->pager_id = (vm_pager_id_t) ip;
	return(ip->pager_id);
}

boolean_t inode_dealloc(id)
	vm_pager_id_t	id;
{
	struct inode *ip;

	if (cpu_number() != master_cpu)
		unix_swtch(u.u_procp, 1);

	ip = (struct inode *) id;
	irele(ip);
}

/*
 *	paging from executable text file
 */
vm_pager_id_t text_pager_setup(ip)
	struct inode	*ip;
{
#if	CS_ICHK
	iincr_chk(ip);
#else	CS_ICHK
	ip->i_count++;
#endif	CS_ICHK
	ip->i_flag |= ITEXT;
	ip->pager_id = (vm_pager_id_t) ip;
	return(ip->pager_id);
}

boolean_t text_dealloc(id)
	vm_pager_id_t	id;
{
	struct inode *ip;

	if (cpu_number() != master_cpu)
		unix_swtch(u.u_procp, 1);

	ip = (struct inode *) id;
	ip->i_flag &= ~ITEXT;
	irele(ip);
}

/*
 *	Remove a text file from the object cache.
 */

void text_uncache(ip)
	struct inode	*ip;
{
	vm_object_uncache(vm_pager_text, ip->pager_id);
}
