/*
 * Copyright (c) 1982, 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)mount.h	7.1 (Berkeley) 6/4/86
 */

/*
 * Mount structure.
 * One allocated on every mount.
 * Used to find the super block.
 */
struct	mount
{
	dev_t	m_dev;		/* device mounted */
	struct	buf *m_bufp;	/* pointer to superblock */
	struct	inode *m_inodp;	/* pointer to mounted on inode */
	struct	inode *m_qinod;	/* QUOTA: pointer to quota file */
};
#ifdef KERNEL
struct	mount mount[NMOUNT];
#endif
