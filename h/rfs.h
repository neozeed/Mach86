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
/*	rfs.h	CMU	82/01/21	*/

/*
 *  Remote file system - definitions and data structures
 *
 **********************************************************************
 * HISTORY
 * 13-Dec-85  Mike Accetta (mja) at Carnegie-Mellon University
 *	Reorganized for new RFS name.
 *
 * 18-Jul-85  Mike Accetta (mja) at Carnegie-Mellon University
 *	Begin conversion for 4.2BSD.
 *
 * 21-Jan-82  Mike Accetta (mja) at Carnegie-Mellon University
 *	Created.
 *
 **********************************************************************
 */

#ifdef	KERNEL
#include "../h/queue.h"
#ifndef	S_IFMT
#include "../h/stat.h"
#endif	S_IFMT
#ifndef	s_addr
#include "../netinet/in.h"
#endif	s_addr
#else	KERNEL
#include "../h/queue.h"
#ifndef	S_IFMT
#include <sys/stat.h>
#endif	S_IFMT
#ifndef	s_addr
#include <netinet/in.h>
#endif	s_addr
#endif	KERNEL


/*
 *  Fundamental constants of the implementation.
 */

#define	RFSPORT		0100		/* UNIX system call port */
#define	RFSMAXRETRY	10		/* times to retry refused connection */
					/*  attempts */
#define	RFSMAXCONN	8		/* maximum connections per process */
#define	RFSMAXPSWD	8		/* maximum password length (temp) */
#define	RFSMAXCACHE	32		/* maximum execution cache entries */

#define	NRFSCB		100		/* number of connection blocks */
#define	NRFSCACHE	31		/* length of cache array */

#define	RFSIOCTLLEN	20		/* maximum longwords per ioctl */


#ifdef	KERNEL
/*
 *  Define this to compile-in debugging code statements.  The initial
 *  contents of the debug flag bits will be the value of the symbol.
 */
#define	RFSDEBUG	0	/* default debug flags */

#ifdef	RFSDEBUG
/*
 *  Debug flag bits.
 */
#define	RFSD_LOG	01	/* general log messages */
#define	RFSD_PROTOCOL	02	/* protocol dialogue */
#define	RFSD_S_TRACEIN	04	/* syscall procedure entry trace */
#define	RFSD_S_TRACEOUT	010	/* syscall procedure exit trace */
#define	RFSD_C_TRACEIN	020	/* connection procedure entry trace */
#define	RFSD_C_TRACEOUT	040	/* connection procedure exit trace */
#define	RFSD_P_TRACEIN	0100	/* protocol procedure entry trace */
#define	RFSD_P_TRACEOUT	0200	/* protocol procedure exit trace */
#define	RFSD_U_TRACEIN	0400	/* utility procedure entry trace */
#define	RFSD_U_TRACEOUT	01000	/* utility procedure exit trace */

/*
 *  Common prefix and suffix for all debugging print statements.  
 *
 *  Avoid printing any additional messages until the controlling terminal
 *  output queue has drained.
 */
#define	RFS_PRF(flags)		\
    if (rfsDebug&(flags))	\
    {				\
	rfsWaitTTY(u.u_ttyp);	\
	uprintf			\
	(
#define	RFS_FRP()		\
	);			\
    }
	
/*
 *  The pre-processor doesn't really support macros with variable numbers of
 *  arguments so we're stuck with defining as many of these as there are
 *  invocations with different numbers of arguments.  We really need to have
 *  all the parameters to the printf() inside the macro definition so they will
 *  completely disappear when debugging is turned off (including string
 *  constants).
 */
#define	rfs_printf1(flags, arg1) \
    RFS_PRF(flags) arg1 RFS_FRP()
#define	rfs_printf2(flags, arg1, arg2) \
    RFS_PRF(flags) arg1, arg2 RFS_FRP()
#define	rfs_printf3(flags, arg1, arg2, arg3) \
    RFS_PRF(flags) arg1, arg2, arg3 RFS_FRP()
#define	rfs_printf4(flags, arg1, arg2, arg3, arg4) \
    RFS_PRF(flags) arg1, arg2, arg3, arg4 RFS_FRP()
#define	rfs_printf5(flags, arg1, arg2, arg3, arg4, arg5) \
    RFS_PRF(flags) arg1, arg2, arg3, arg4, arg5 RFS_FRP()
#define	rfs_printf6(flags, arg1, arg2, arg3, arg4, arg5, arg6) \
    RFS_PRF(flags) arg1, arg2, arg3, arg4, arg5, arg6 RFS_FRP()
#define	rfs_printf7(flags, arg1, arg2, arg3, arg4, arg5, arg6, arg7) \
    RFS_PRF(flags) arg1, arg2, arg3, arg4, arg5, arg6, arg7 RFS_FRP()

extern char *syscallnames[];

#else	RFSDEBUG
/*
 *  Without debugging on, expand these to nothing.
 */
#define	rfs_printf1(flags, arg1) 
#define	rfs_printf2(flags, arg1, arg2) 
#define	rfs_printf3(flags, arg1, arg2, arg3) 
#define	rfs_printf4(flags, arg1, arg2, arg3, arg4) 
#define	rfs_printf5(flags, arg1, arg2, arg3, arg4, arg5) 
#define	rfs_printf6(flags, arg1, arg2, arg3, arg4, arg5, arg6) 
#define	rfs_printf7(flags, arg1, arg2, arg3, arg4, arg5, arg6, arg7) 
#endif	RFSDEBUG
#endif	KERNEL



/*
 *  Connect request/reply message format
 */

struct rfsConnectMsg
{
    short rcm_type;		/* message type (common) */
    short rcm_port;		/* connect port back to us */
    short rcm_uid;		/* local effective user ID */
    short rcm_gid;		/* local effective group ID */
    short rcm_ruid;		/* local real user ID */
    short rcm_rgid;		/* local real group ID */
};
#define	rcm_errno rcm_port	/* reply error number */

/*
 *  Open/Creat system call request/reply message format
 */

struct rfsOpenMsg
{
    short rom_type;		/* message type (common) */
    short rom_mode;		/* file mode */
    short rom_count;		/* pathname length (common) */
    short rom_port;		/* connect port back to us */
};
#define	rom_errno rom_mode	/* reply error number */

/*
 *  Read/Write system call request/reply message format
 */

struct rfsRWMsg
{
    short rrwm_type;		/* message type (common) */
    short rrwm_errno;		/* reply error number */
    long  rrwm_count;		/* transfer byte count */
};

/*
 *  Ioctl system call request/reply message format
 */

struct rfsIoctlMsg
{
    short rim_type;		/* message type (common) */
    short rim_cmd;		/* command word */
    long rim_rval;		/* return value */
    caddr_t rim_argp;		/* user data pointer */
    long rim_arg[RFSIOCTLLEN];	/* data contents */
};
#define	rim_errno rim_cmd	/* reply error number */

/*
 *  Lseek system call request/reply message format
 */

struct rfsSeekMsg
{
    short rsm_type;		/* message type (common) */
    short rsm_sbase;		/* seek base indicator */
    long  rsm_off;		/* seek offset */
};
#define	rsm_errno rsm_sbase	/* reply error number */

/*
 *  Stat/Fstat system call request message format
 */

struct rfsStatMsg
{
    short rstm_type;		/* message type (common) */
    short rstm_errno;		/* (currently unused) */
    long rstm_count;		/* pathname length */
};

/*
 *  Stat/Fstat system call reply message format
 */

struct rfsStatReply
{
    short rstr_type;		/* message type (common) */
    short rstr_errno;		/* reply error number */
    struct stat rstr_statb;	/* returned status information */
};

/*
 *  Utility request/reply message format.
 */

struct rfsMsg
{
    short rm_type;		/* message type (common) */
    short rm_errno;		/* reply error number */
    long  rm_count;		/* pathname length */
};

/*
 *  Common system call request/reply message format
 */

struct rfsSysMsg
{
    short rsym_type;		/* message type (common) */
    short rsym_arg1;		/* first short argument (if any) */
    short rsym_count;		/* pathname length */
    short rsym_arg2;		/* second short argument (if any) */
};
#define	rsym_errno rsym_arg1	/* reply error number */

/*
 *  Utime system call request/reply message format
 */

struct rfsUtimeMsg
{
    short  rutm_type;		/* system call code number */
    short  rutm_errno;		/* reply error number */
    short  rutm_count;		/* pathname length */
    short  rutm_pad;		/* (unused) */
    time_t rutm_tv[2];		/* time vector */
};



/*
 *  System call message types
 */

#define	RFST_FORK	2
#define	RFST_READ	3
#define	RFST_WRITE	4
#define	RFST_OPEN	5
#define	RFST_CLOSE	6
#define	RFST_CREAT	8
#define	RFST_LINK	9
#define	RFST_UNLINK	10
#define	RFST_EXEC	11
#define	RFST_CHDIR	12
#define	RFST_MKNOD	14
#define	RFST_CHMOD	15
#define	RFST_CHOWN	16
#define	RFST_STAT	18
#define	RFST_SEEK	19
#define	RFST_FSTAT	28
#define	RFST_UTIME	30
#define	RFST_ACCESS	33
#define	RFST_NSTAT	38
#define	RFST_LSTAT	40
#define	RFST_IOCTL	54
#define	RFST_SYMLINK	57
#define	RFST_READLINK	58
#define	RFST_EXECE	59
#define	RFST_UMASK	60
#define	RFST_CHROOT	61
#define	RFST_NFSTAT	62
#define	RFST_FCNTL	92
#define	RFST_FSYNC	95
#define	RFST_FCHOWN	123
#define	RFST_FCHMOD	124
#define	RFST_RENAME	128
#define	RFST_TRUNCATE	129
#define	RFST_FTRUNCATE	130
#define	RFST_FLOCK	131
#define	RFST_MKDIR	136
#define	RFST_RMDIR	137
#define	RFST_UTIMES	138

/*
 *  Control message types  
 */
#define	RFST_CONNECT	0200

/*
 *  Message type modifiers
 */
#define	RFST_REPLY	0100		/* reply bit */
#define	RFST_KERNEL	0100		/* XXX */



#ifdef	KERNEL
/*
 *  Remote link inode contents layout
 */

struct rfsLink
{
    struct in_addr rl_addr;		/* remote IP addr */
    u_short 	   rl_fport;		/* remote contact port */
    u_char  	   rl_pvers;		/* RFS protocol version */
    u_char	   rl_pad;		/* (unused) */
};



/*
 *  Remote file system connection block
 */

#define	rfsConnectionQueue	Queue

struct rfsConnectionBlock
{
    struct rfsConnectionQueue
	           rcb_link;		/* forward/back link */
    struct socket *rcb_so;		/* socket pointer */
    struct inode  *rcb_ip;		/* remote link inode pointer */
    struct rfsLink rcb_rl;		/* remote IP addr */
    u_short        rcb_refc;		/* reference count */
    u_short	   rcb_lport;		/* local port */
    u_char	   rcb_majx;		/* remote block device # bias */
    char	   rcb_pswd[RFSMAXPSWD+1];/* password (temp) */
};
#define	rcb_addr	rcb_rl.rl_addr
#define	rcb_fport	rcb_rl.rl_fport
#define	rcb_pvers	rcb_rl.rl_pvers
#define	rcb_flags	rcb_rl.rl_pad


/* flag bits */
#define	RFSF_LOCKED	01	/* block in use */
#define	RFSF_WANTED	02	/* block needed for another use */
#define	RFSF_ERROR	04	/* error on connection */
#define	RFSF_RETRY	010	/* recoverable error on connection */

/*
 *  rfsConnectionFirst - return first connection block in queue
 *
 *  rcbqp = queue of connection blocks
 *
 *  Return: the first connection block in the queue
 */
#define	rfsConnectionFirst(rcbpq)	\
    ((struct rfsConnectionBlock *)((rcbpq)->F))

/*
 *  rfsConnectionNext - advance to the next connection block in a queue 
 *
 *  rcbp = current connection block
 *
 *  Return: the next connection block in the queue
 */
#define	rfsConnectionNext(rcbp)	\
    ((struct rfsConnectionBlock *)(rcbp->rcb_link.F))

/*
 *  rfsConnectionLast - test for end of connection block queue
 *
 *  rcbpq = connection block queue
 *  rcbp  = current connection block in queue
 *
 *  Return: true if the current connection block indicates the end of the
 *  queue, otherwise false.
 */
#define	rfsConnectionLast(rcbpq, rcbp)	\
     ((rcbpq) == &(rcbp->rcb_link))

/*
 *  rfsEnQueueConnection - enqueue a connection block
 *
 *  rcbqp = queue to modify
 *  rcbp  = connection block pointer to enqueue
 */
#define	rfsEnQueueConnection(rcbqp, rcbp) \
    enQueue(rcbqp, (struct Queue *)(rcbp)) 

/*
 *  rfsDeQueueConnection - dequeue a connection block
 *
 *  rcbqp = queue to modify
 *  rcbp  = connection block pointer to remqueue
 *
 *  Return: the dequeued connection block pointer or NULL if the queue was
 *  empty.
 */
#define	rfsDeQueueConnection(rcbqp) \
    (struct rfsConnectionBlock *)deQueue(rcbqp) 

/*
 *  rfsRemQueueConnection - remqueue a connection block
 *
 *  rcbqp = queue to modify
 *  rcbp  = connection block pointer to remqueue
 */
#define	rfsRemQueueConnection(rcbqp, rcbp) \
    remQueue((struct Queue *)(rcbp)) 


/*
 *  rfsIncrCheck - increment remote connection reference count
 *
 *  rcbp = remote connection blcok for increment
 *
 *  Increment the reference count and panic on overflow back to zero.
 */

#define	rfsIncrCheck(rcbp)	\
	if (++((rcbp)->rcb_refc) == 0) panic(rfsPanicMsg_Incr)
extern char *rfsPanicMsg_Incr;

/*
 *  rfsDecrCheck - decrement remote connection reference count
 *
 *  rcbp = remote connection blcok for decrement
 *
 *  Decrement the reference count and panic on underflow below zero.
 */

#define	rfsDecrCheck(rcbp)	\
	if (((rcbp)->rcb_refc)-- == 0) panic(rfsPanicMsg_Decr)
extern char *rfsPanicMsg_Decr;



/*
 *  Parallel process table entry
 */

struct rfsProcessEntry
{
    struct Queue rpe_rcbq;		/* remote connection block queue */
};

extern struct rfsProcessEntry *rfsProcessTable;

#define	rfsCurrentProcessEntry	(&rfsProcessTable[u.u_procp - proc])



/*
 *  Remote inode cache entry
 */

struct rfsCacheEntry
{
    struct rfsCacheEntry
	            *rce_next;		/* next entry in hash chain */
    struct in_addr   rce_addr;		/* remote IP addr */
    struct inode    *rce_ip;		/* local inode copy */
    time_t	     rce_used;		/* time last used */
    long	     rce_size;		/* size from remote inode */
    time_t	     rce_mtime;		/* modification time from remote inode */
    ino_t	     rce_ino;		/* number of remote inode */
    dev_t	     rce_dev;		/* device of remote inode */
    short	     rce_mt;		/* mount table index of inode file system */
};
extern struct rfsCacheEntry *rfsCacheTable[];
    
/*
 *  rfsCacheHash - map remote inode to cache entry chain
 *
 *  addr = remote IP address
 *  ino  = remote i-number
 *  dev  = remote device number
 *
 *  Return:  index into cache array corresponding to the chain
 *  of entries which will contain the sought inode (if it exists).
 */

#define rfsCacheHash(addr, ino, dev)			\
(							\
    ((unsigned)((addr)->s_addr*(ino)*(dev)))%NRFSCACHE	\
)


/*
 *  Remote inode cache parallel mount table
 */

struct rfsMountEntry
{
    long rme_frags;			/* fragments used in cache */
};
extern struct rfsMountEntry rfsMountTable[];



/*
 *  Dynamic memory free structure template.
 *
 *  This structure is used to maintain the linked list of free stuctures in a
 *  memory allocation descriptor.  All dynamically allocated structures are
 *  at least this large (sufficient to hold a link pointer).
 */

struct rfsFreeOverlay
{
    struct rfsFreeOverlay *rfo_next;
};


/*
 *  Dynamic memory structure allocation descriptor
 *
 *  One of these descriptors exists for each type of dynamically allocated
 *  structure.  It contains the structure size and a count of the number of
 *  strcutures which can be carved out of each allocated mbuf as well as a list
 *  of free structures which are available for reallocation.
 */

struct rfsFreeDescriptor
{
    struct rfsFreeOverlay
		     *rfd_head;		/* head of free structure list */
    short	      rfd_size;		/* size of a structure */
    short	      rfd_max;		/* maximum structures per mbuf */
};

/*
 *  Macro to (compile-time) initialize descriptor for a structure type
 */
#define	rfsInitDescriptor(type)	\
    {NULL, sizeof (struct type), MLEN/sizeof(struct type)}


/*
 *  Cover definitions for specific structure allocation routines
 */

#define	rfsCacheAllocate()		\
    (struct rfsCacheEntry *)rfsAllocate(&rfsCacheDescriptor)

#define	rfsCacheFree(rce)	\
    rfsFree(&rfsCacheDescriptor, (struct rfsFreeOverlay *)(rce))

extern struct rfsFreeDescriptor rfsCacheDescriptor;


#define	rfsConnectionAllocate()		\
    (struct rfsConnectionBlock *)rfsAllocate(&rfsConnectionDescriptor)

#define	rfsConnectionFree(rcb)	\
    rfsFree(&rfsConnectionDescriptor, (struct rfsFreeOverlay *)(rcb))

extern struct rfsFreeDescriptor rfsConnectionDescriptor;



/*
 *  Global variables
 */

extern struct inode *(*rfs_sysent[])();	/* system call dispatch table */

extern int rfsDebug;			/* debugging flags */



/*
 *  Procedure declarations
 */

extern struct inode		 *rfs_copen();
extern struct inode		 *rfs_stat1();
extern struct inode		 *rfs_syscall();

extern struct rfsFreeOverlay	 *rfsAllocate();
extern struct rfsConnectionBlock *rfsAttach();
extern struct inode		 *rfsCache();
extern bool		 	  rfsCachePurge();
extern void		 	  rfsCacheSweep();
extern struct rfsConnectionBlock *rfsCopy();
extern void		 	  rfsDetach();
extern void		 	  rfsError();
extern void		 	  rfsFree();
extern void		 	  rfsFork();
extern void		 	  rfsForkDir();
extern struct rfsConnectionBlock *rfsGet();
extern struct rfsCacheEntry	 *rfsInCache();
extern struct inode		 *rfsInodeAllocate();
extern void		 	  rfsLock();
extern struct rfsConnectionBlock *rfsOpCr();
extern struct rfsConnectionBlock *rfsProcessConnection();
extern void		 	  rfsShutdown();
extern struct mbuf		 *rfsSocketName();
extern void		 	  rfsUnLock();
extern void		 	  rfsUnEstablish();
extern struct inode		 *rfsValidCache();
extern void		 	  rfsWaitTTY();
#endif	KERNEL
