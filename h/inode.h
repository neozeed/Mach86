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
 *	@(#)inode.h	6.8 (Berkeley) 6/8/85
 */
#if	CMU
/*
 **********************************************************************
 * HISTORY
 * 31-May-86  Avadis Tevanian (avie) at Carnegie-Mellon University
 *	Set the pager_id field in the inode structure.  This is a back
 *	pointer to the real pager id which will allow for pager id's to
 *	be different than inode numbers.
 *
 * 25-Jan-86  Avadis Tevanian (avie) at Carnegie-Mellon University
 *	Upgraded to 4.3.
 *
 * 18-Feb-86  Bill Bolosky (bolosky) at Carnegie-Mellon University
 *	Added different definitions of i_size and di_size for Sailboat
 *	from IBM code under switch ROMP.  These differences are
 *	presumably because of byte ordering differences.
 *
 * 16-Aug-85  Mike Accetta (mja) at Carnegie-Mellon University
 *	CS_OLDDIR: defined new INEWDIR and IOLDDIR flag bits and
 *	changed isolddir() macro to use these to determine directory
 *	formats.
 *
 * 20-Jul-85  Mike Accetta (mja) at Carnegie-Mellon University
 *	CS_RFS:  Changed to declare cnamei() rather than namei.
 *	[V1(1)]
 *
 * 13-May-85  Mike Accetta (mja) at Carnegie-Mellon University
 *	Upgraded to 4.2BSD.  Carried over change below.
 *	CS_OLDDIR: added new isolddir() macro to detect old-style
 *	directories;
 *	CS_COMPAT:  retained exclusive use bits, MPX and old inode
 *	format for now.
 *	[V1(1)]
 *
 * 20-Feb-82  Mike Accetta (mja) at Carnegie-Mellon University
 *	CS_ICHK: Added iincr_chk() and idecr_chk() macros to modify inode
 *	reference counts and check for consistency (V3.04c).
 *
 **********************************************************************
 */
 
#if	KERNEL
#include "cs_compat.h"
#include "cs_ichk.h"
#include "cs_oldfs.h"
#include "cs_olddir.h"
#include "cs_rfs.h"
#include "cs_xmod.h"
#include "mach_vm.h"
#else	KERNEL
#include <sys/features.h>
#endif	KERNEL
#endif	CMU

#if	MACH_VM
#include "../vm/vm_pager.h"
#endif	MACH_VM
/*
 * The I node is the focus of all file activity in UNIX.
 * There is a unique inode allocated for each active file,
 * each current directory, each mounted-on file, text file, and the root.
 * An inode is 'named' by its dev/inumber pair. (iget/iget.c)
 * Data in icommon is read in from permanent inode on volume.
 */

#if	CS_RFS || CS_COMPAT
/*
 *  The following macro is used to identify the special CMU file type which is
 *  currently used to implement both old style symbolic links and remote node
 *  pointer files.
 */
#define	isesctype(ip)	\
	((ip)->i_gid == 64)
 
#endif	CS_RFS || CS_COMPAT
#if	CS_RFS
/*
 *  The following macro is used to distinguish the special CMU remote node pointer.
 */
#define	isrfslnk(ip)	\
	(isesctype(ip) && (((ip)->i_mode&(IFMT|IEXEC|(IEXEC>>3)|(IEXEC>>6))) == (IFREG|IEXEC)))
 
#endif	CS_RFS
#if	CS_COMPAT
/*
 *  The following macros are used to distinguish between old and new style
 *  symbolic links.
 */
#define	isnewlnk(ip)	\
	(((ip)->i_mode&IFMT) == IFLNK)

#define	isoldlnk(ip)	\
	(isesctype(ip) && (((ip)->i_mode&(IFMT|IEXEC|(IEXEC>>3)|(IEXEC>>6))) == IFREG))
 
#define	issymlnk(ip)	\
	(isnewlnk(ip) || isoldlnk(ip))

#define	NINDEX		6

struct group
{
	short	g_state;
	char	g_index;
	char	g_rot;
	struct	group	*g_group;
	struct	inode	*g_inode;
	struct	file	*g_file;
	short	g_rotmask;
	short	g_datq;
	struct	chan *g_chans[NINDEX];
};
#endif	CS_COMPAT
#if	CS_OLDFS
#define	NOADDR	13

/*
 * Old file system inode as it appears in
 * a disk block.
 */

struct oinode
{
	unsigned short oi_mode;	/* mode and type of file */
	short	oi_nlink;	/* number of links to file */
	short	oi_uid;		/* owner's user id */
	short	oi_gid;		/* owner's group id */
	off_t	oi_size;	/* number of bytes in file */
	char	oi_addr[40];	/* disk block addresses */
	time_t	oi_atime;	/* time last accessed */
	time_t	oi_mtime;	/* time last modified */
	time_t	oi_ctime;	/* time created */
};
#endif	CS_OLDFS
#define	NDADDR	12		/* direct addresses in inode */
#define	NIADDR	3		/* indirect addresses in inode */

struct inode {
	struct	inode *i_chain[2];	/* must be first */
#if	MACH_VM
	vm_pager_id_t pager_id;	/* corresponding pager id (if any) */
#endif	MACH_VM
#if	CS_OLDDIR
	u_long	i_flag;
	u_long	i_count;	/* reference count */
#else	CS_OLDDIR
	u_short	i_flag;
	u_short	i_count;	/* reference count */
#endif	CS_OLDDIR
	dev_t	i_dev;		/* device where inode resides */
	u_short	i_shlockc;	/* count of shared locks on inode */
	u_short	i_exlockc;	/* count of exclusive locks on inode */
	ino_t	i_number;	/* i number, 1-to-1 with device address */
	long	i_id;		/* unique identifier */
	struct	fs *i_fs;	/* file sys associated with this inode */
	struct	dquot *i_dquot;	/* quota structure controlling this file */
	union {
		daddr_t	if_lastr;	/* last read (read-ahead) */
		struct	socket *is_socket;
		struct	{
			struct inode  *if_freef;	/* free list forward */
			struct inode **if_freeb;	/* free list back */
		} i_fr;
	} i_un;
	struct 	icommon
	{
		u_short	ic_mode;	/*  0: mode and type of file */
		short	ic_nlink;	/*  2: number of links to file */
		short	ic_uid;		/*  4: owner's user id */
		short	ic_gid;		/*  6: owner's group id */
		quad	ic_size;	/*  8: number of bytes in file */
		time_t	ic_atime;	/* 16: time last accessed */
		long	ic_atspare;
		time_t	ic_mtime;	/* 24: time last modified */
		long	ic_mtspare;
		time_t	ic_ctime;	/* 32: last time inode changed */
		long	ic_ctspare;
#if	CS_COMPAT
		union
		{
		    struct
		    {
			/*
			 *  NDADDR+NIADDR = 12+3 = 15 > NOADDR = 13
			 */
			daddr_t	Ib_db[NDADDR];	/* 40: disk block addresses */
			daddr_t	Ib_ib[NIADDR];	/* 88: indirect blocks */
		    } ic_Ib;
#define	ic_db	ic_Iun.ic_Ib.Ib_db
#define	ic_ib	ic_Iun.ic_Ib.Ib_ib
		    struct
		    {
		        daddr_t	Ig_obs;		/* ic_db[0] */
		        struct	group Ig_group;	/* multiplexor group file */
		    } ic_Ig;
#define	ic_group ic_Iun.ic_Ig.Ig_group
		} ic_Iun;
#else	CS_COMPAT
		daddr_t	ic_db[NDADDR];	/* 40: disk block addresses */
		daddr_t	ic_ib[NIADDR];	/* 88: indirect blocks */
#endif	CS_COMPAT
		long	ic_flags;	/* 100: status, currently unused */
		long	ic_blocks;	/* 104: blocks actually held */
		long	ic_spare[5];	/* 108: reserved, currently unused */
	} i_ic;
};

struct dinode {
	union {
		struct	icommon di_icom;
		char	di_size[128];
	} di_un;
};

#define	i_mode		i_ic.ic_mode
#define	i_nlink		i_ic.ic_nlink
#define	i_uid		i_ic.ic_uid
#define	i_gid		i_ic.ic_gid
/* ugh! -- must be fixed */
#ifdef vax
#define	i_size		i_ic.ic_size.val[0]
#endif
#ifdef romp
#define i_size		i_ic.ic_size.val[1]
#endif romp
#define	i_db		i_ic.ic_db
#define	i_ib		i_ic.ic_ib
#define	i_atime		i_ic.ic_atime
#define	i_mtime		i_ic.ic_mtime
#define	i_ctime		i_ic.ic_ctime
#define i_blocks	i_ic.ic_blocks
#define	i_rdev		i_ic.ic_db[0]
#define	i_lastr		i_un.if_lastr
#define	i_socket	i_un.is_socket
#define	i_forw		i_chain[0]
#define	i_back		i_chain[1]
#define	i_freef		i_un.i_fr.if_freef
#define	i_freeb		i_un.i_fr.if_freeb

#define di_ic		di_un.di_icom
#define	di_mode		di_ic.ic_mode
#define	di_nlink	di_ic.ic_nlink
#define	di_uid		di_ic.ic_uid
#define	di_gid		di_ic.ic_gid
#ifdef vax
#define	di_size		di_ic.ic_size.val[0]
#endif
#ifdef romp
#define di_size		di_ic.ic_size.val[1]
#endif romp
#define	di_db		di_ic.ic_db
#define	di_ib		di_ic.ic_ib
#define	di_atime	di_ic.ic_atime
#define	di_mtime	di_ic.ic_mtime
#define	di_ctime	di_ic.ic_ctime
#define	di_rdev		di_ic.ic_db[0]
#define	di_blocks	di_ic.ic_blocks

#ifdef KERNEL
/*
 * Invalidate an inode. Used by the namei cache to detect stale
 * information. At an absurd rate of 100 calls/second, the inode
 * table invalidation should only occur once every 16 months.
 */
#define cacheinval(ip)	\
	(ip)->i_id = ++nextinodeid; \
	if (nextinodeid == 0) \
		cacheinvalall();

struct inode *inode;		/* the inode table itself */
struct inode *inodeNINODE;	/* the end of the inode table */
int	ninode;			/* number of slots in the table */
long	nextinodeid;		/* unique id generator */

struct	inode *rootdir;			/* pointer to inode of root directory */
#if	CS_COMPAT
struct	inode *mpxip;		/* mpx virtual inode */
#endif	CS_COMPAT

struct	inode *ialloc();
struct	inode *iget();
#ifdef notdef
struct	inode *ifind();
#endif
struct	inode *owner();
struct	inode *maknode();
#if	CS_RFS
struct	inode *cnamei();
#else	CS_RFS
struct	inode *namei();
#endif	CS_RFS

ino_t	dirpref();
#endif

/* flags */
#define	ILOCKED		0x1		/* inode is locked */
#define	IUPD		0x2		/* file has been modified */
#define	IACC		0x4		/* inode access time to be updated */
#define	IMOUNT		0x8		/* inode is mounted on */
#define	IWANT		0x10		/* some process waiting on lock */
#define	ITEXT		0x20		/* inode is pure text prototype */
#define	ICHG		0x40		/* inode has been changed */
#define	ISHLOCK		0x80		/* file has shared lock */
#define	IEXLOCK		0x100		/* file has exclusive lock */
#define	ILWAIT		0x200		/* someone waiting on file lock */
#define	IMOD		0x400		/* inode has been modified */
#define	IRENAME		0x800		/* inode is being renamed */
#define	IXMOD		0x8000		/* inode is text, but impure (XXX) */
#if	CS_XMOD
				/* the next two bits must remain as ordered */
#define	IXREAD	0x1000		/* exclusive read */
#define	IXWRITE	0x2000		/* exclusive write */
#endif	CS_XMOD
#if	CS_OLDDIR
#define	IOLDDIR	0x10000			/* old directory format */
#define	INEWDIR	0x20000			/* new directory format */
/*
 *  This macro is used to distinguish between old and new directory formats.
 *  It tests the bits defined above which are set in namei() when a directory
 *  is first searched.  One of either the OLD or NEW format bits must be on
 *  as a consistency check to ensure that we never use the macro without
 *  namei() first having made the format decision.
 */
extern char *PANICMSG_ISOLDDIR;
#define	isolddir(ip)			\
(					\
  ((ip)->i_flag&(IOLDDIR|INEWDIR))	\
  ?					\
    ((ip)->i_flag&IOLDDIR)		\
  :					\
    panic(PANICMSG_ISOLDDIR)		\
)
#endif	CS_OLDDIR

/* modes */
#define	IFMT		0170000		/* type of file */
#define	IFCHR		0020000		/* character special */
#define	IFDIR		0040000		/* directory */
#define	IFBLK		0060000		/* block special */
#define	IFREG		0100000		/* regular */
#define	IFLNK		0120000		/* symbolic link */
#define	IFSOCK		0140000		/* socket */
#if	CS_COMPAT
#define	IFMPC		0030000		/* multiplexed char special */
#define	IFMPB		0070000		/* multiplexed block special */
#endif	CS_COMPAT

#define	ISUID		04000		/* set user id on execution */
#define	ISGID		02000		/* set group id on execution */
#define	ISVTX		01000		/* save swapped text even after use */
#define	IREAD		0400		/* read, write, execute permissions */
#define	IWRITE		0200
#define	IEXEC		0100

#define	ILOCK(ip) { \
	while ((ip)->i_flag & ILOCKED) { \
		(ip)->i_flag |= IWANT; \
		sleep((caddr_t)(ip), PINOD); \
	} \
	(ip)->i_flag |= ILOCKED; \
}

#define	IUNLOCK(ip) { \
	(ip)->i_flag &= ~ILOCKED; \
	if ((ip)->i_flag&IWANT) { \
		(ip)->i_flag &= ~IWANT; \
		wakeup((caddr_t)(ip)); \
	} \
}

#define	IUPDAT(ip, t1, t2, waitfor) { \
	if (ip->i_flag&(IUPD|IACC|ICHG|IMOD)) \
		iupdat(ip, t1, t2, waitfor); \
}
#if	CS_ICHK

/*
 *  Macros for modifying inode reference counts - used to check for consistency
 *  at each reference.
 *
 *  The idecr_chk() macro is defined such that it doesn't decrement the
 *  reference count when already zero.  This is in order to avoid the nested
 *  iincr panic which would otherwise occur when update() tries to sync the
 *  inode before halting and increments the count back to zero.
 *
 *  The panic message strings are initialized variables rather than constant
 *  strings since they are potentially used in many places.
 */
extern char *PANICMSG_IINCR;
extern char *PANICMSG_IDECR;

#define	iincr_chk(ip)	{ if (++((ip)->i_count) == 0) panic(PANICMSG_IINCR); }
#define	idecr_chk(ip)	{ if (((ip)->i_count) != 0) ((ip)->i_count--); else panic(PANICMSG_IDECR); }
#endif	CS_ICHK
#define	ITIMES(ip, t1, t2) { \
	if ((ip)->i_flag&(IUPD|IACC|ICHG)) { \
		(ip)->i_flag |= IMOD; \
		if ((ip)->i_flag&IACC) \
			(ip)->i_atime = (t1)->tv_sec; \
		if ((ip)->i_flag&IUPD) \
			(ip)->i_mtime = (t2)->tv_sec; \
		if ((ip)->i_flag&ICHG) \
			(ip)->i_ctime = time.tv_sec; \
		(ip)->i_flag &= ~(IACC|IUPD|ICHG); \
	} \
}
