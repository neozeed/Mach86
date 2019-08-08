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
 *	@(#)kern_exec.c	6.14 (Berkeley) 8/12/85
 */
#if	CMU
/*
 **********************************************************************
 * HISTORY
 * 11-Jun-86  Bill Bolosky (bolosky) at Carnegie-Mellon University
 *	romp: Added stupid exect() call for adb on the RT.
 *
 *  1-Jun-86  Avadis Tevanian (avie) at Carnegie-Mellon University
 *	Use VM_OBJECT_NULL in calls to vm_map_find instead of creating a
 *	dummy object.  (The VM code will generate an object when [and
 *	if] necessary).
 *
 * 23-May-86  David Golub (dbg) at Carnegie-Mellon University
 *	Use text pager to protect executing files from being overwritten
 *	(using the ITEXT bit).
 *
 * 14-May-86  Avadis Tevanian (avie) at Carnegie-Mellon University
 *	Use rlim_cur instead of rlim_max for stack size during exec.
 *
 *  6-May-86  Avadis Tevanian (avie) at Carnegie-Mellon University
 *	Changed to use vm_allocate_with_pager (rather than vm_allocate).
 *
 * 25-Apr-86  Mike Accetta (mja) at Carnegie-Mellon University
 *	CS_COMPAT: Changed to clear 4.2 mode across exec() only when
 *	UMODE_ALLOWOLD has been set or if ISVTX bit is on in mode.
 *
 * 23-Apr-86  David Golub (dbg) at Carnegie-Mellon University
 *	Protect code before zeroing bss - we will get fewer objects
 *	this way if the process forks.
 *
 * 14-Apr-86  David Golub (dbg) at Carnegie-Mellon University
 *	Lock and disable interrupts around call to vm_map_copy.
 *
 * 28-Mar-86  David Golub (dbg) at Carnegie-Mellon University
 *	Remember that the loader's page-size is still
 *	(CLSIZE*NBPG), and that text, data and bss end on the old
 *	page boundaries, not the new ones (or we'd have to relink all
 *	programs whenever we changed the page size!).
 *
 * 22-Mar-86  Avadis Tevanian (avie) at Carnegie-Mellon University
 *	Merged VM and Romp versions.
 *
 * 25-Feb-86  David Golub (dbg) at Carnegie-Mellon University
 *	Converted to new virtual memory code.
 *
 * 21-Feb-86  Mike Accetta (mja) at Carnegie-Mellon University
 *	CS_COMPAT:  Changed to clear read directory bit in accounting
 *	flags across execs.
 *	[V1(1)]
 *
 * 25-Jan-86  Avadis Tevanian (avie) at Carnegie-Mellon University
 *	Upgraded to 4.3.
 *
 * 23-Nov-85  Mike Accetta (mja) at Carnegie-Mellon University
 *	CS_RFS:  enabled remote namei() processing for all
 *	routines in this module.
 *	[V1(1)]
 *
 * 12-Aug-85  Mike Accetta (mja) at Carnegie-Mellon University
 *	CS_COMPAT:  Changed to reset signal trampoline code across
 *	execs.
 *
 * 26-Jul-85  Mike Accetta (mja) at Carnegie-Mellon University
 *	CS_COMPAT:  Changed to clear 4.1/4.2 mode bits in accounting
 *	flags across execs.
 *	[V1(1)]
 *
 * 25-May-85  Mike Accetta (mja) at Carnegie-Mellon University
 *	CS_COMPAT:  changed to clear SJCSIG across exec().
 *	[V1(1)].
 *
 * 21-May-85  Glenn Marcy (gm0w) at Carnegie-Mellon University
 *	Upgraded from 4.1BSD.  Carried over changes below.
 *
 *	CS_XONLY:  Added setting of execute only bit and clearing of
 *	trace bit in process status if image is not readable (V3.00).
 *	[V1(1)]
 *
 **********************************************************************
 */
 
#include "cs_compat.h"
#include "cs_rfs.h"
#include "cs_xonly.h"

#include "mach_only.h"
#include "mach_vm.h"
#endif	CMU

#include "../machine/reg.h"
#include "../machine/pte.h"
#ifdef	romp
#include "../machine/scr.h"
#else	romp
#include "../machine/psl.h"
#endif	romp

#include "param.h"
#include "systm.h"
#include "map.h"
#include "dir.h"
#include "user.h"
#include "kernel.h"
#include "proc.h"
#include "buf.h"
#include "inode.h"
#include "seg.h"
#include "vm.h"
#include "text.h"
#include "file.h"
#include "uio.h"
#include "acct.h"
#include "exec.h"
#if	CS_RFS
 
/*
 *  Force all namei() calls to permit remote names since this module has
 *  been updated.
 */
#undef	namei
#define	namei	rnamei
#endif	CS_RFS

#ifdef	romp
#include "../machine/debug.h"
#endif	romp

#ifdef vax
#include "../vax/mtpr.h"
#endif

#if	MACH_VM
#include "../h/task.h"
#include "../h/thread.h"

#include "../vm/vm_param.h"
#include "../vm/vm_map.h"
#include "../vm/vm_object.h"
#include "../vm/vm_kern.h"
#include "../vm/vm_user.h"
#include "../h/zalloc.h"
extern struct zone	*arg_zone;

#define	LOADER_PAGE_SIZE	(CLSIZE*NBPG)
#define loader_round_page(x)	((vm_offset_t)((((vm_offset_t)(x)) \
						+ LOADER_PAGE_SIZE - 1) \
					& ~(LOADER_PAGE_SIZE-1)))
#define loader_trunc_page(x)	((vm_offset_t)(((vm_offset_t)(x)) \
					& ~(LOADER_PAGE_SIZE-1)))

#endif	MACH_VM

/*
 * exec system call, with and without environments.
 */
struct execa {
	char	*fname;
	char	**argp;
	char	**envp;
};

execv()
{
	((struct execa *)u.u_ap)->envp = NULL;
	execve();
}

#ifdef romp

exect()	/* New RXTUnix system call for execve with single step active */
{
	execve();
	if( u.u_error );
	else u.u_ar0[ICSCS] |= ICSCS_INSTSTEP;
}

#endif romp

execve()
{
	register nc;
	register char *cp;
	register struct buf *bp;
	register struct execa *uap;
	int na, ne, ucp, ap, len, cc;
	int indir, uid, gid;
	char *sharg;
	struct inode *ip;
#if	MACH_VM
	caddr_t exec_args;
#else	MACH_VM
	swblk_t bno;
#endif	MACH_VM
	char cfname[MAXCOMLEN + 1];
#define	SHSIZE	32
	char cfarg[SHSIZE];
	union {
		char	ex_shell[SHSIZE];	/* #! and name of interpreter */
		struct	exec ex_exec;
	} exdata;
	register struct nameidata *ndp = &u.u_nd;
	int resid, error;

	ndp->ni_nameiop = LOOKUP | FOLLOW;
	ndp->ni_segflg = UIO_USERSPACE;
	ndp->ni_dirp = ((struct execa *)u.u_ap)->fname;
	if ((ip = namei(ndp)) == NULL)
		return;
#if	MACH_VM
	exec_args = NULL;
#else	MACH_VM
	bno = 0;
	bp = 0;
#endif	MACH_VM
	indir = 0;
	uid = u.u_uid;
	gid = u.u_gid;
	if (ip->i_mode & ISUID)
		uid = ip->i_uid;
	if (ip->i_mode & ISGID)
		gid = ip->i_gid;

  again:
	if (access(ip, IEXEC))
		goto bad;
	if ((u.u_procp->p_flag&STRC) && access(ip, IREAD))
		goto bad;
	if ((ip->i_mode & IFMT) != IFREG ||
	   (ip->i_mode & (IEXEC|(IEXEC>>3)|(IEXEC>>6))) == 0) {
		u.u_error = EACCES;
		goto bad;
	}

	/*
	 * Read in first few bytes of file for segment sizes, magic number:
	 *	407 = plain executable
	 *	410 = RO text
	 *	413 = demand paged RO text
	 * Also an ASCII line beginning with #! is
	 * the file name of a ``shell'' and arguments may be prepended
	 * to the argument list if given here.
	 *
	 * SHELL NAMES ARE LIMITED IN LENGTH.
	 *
	 * ONLY ONE ARGUMENT MAY BE PASSED TO THE SHELL FROM
	 * THE ASCII LINE.
	 */
	exdata.ex_shell[0] = '\0';	/* for zero length files */
	u.u_error = rdwri(UIO_READ, ip, (caddr_t)&exdata, sizeof (exdata),
	    0, 1, &resid);
	if (u.u_error)
		goto bad;
#ifndef lint
	if (resid > sizeof(exdata) - sizeof(exdata.ex_exec) &&
	    exdata.ex_shell[0] != '#') {
		u.u_error = ENOEXEC;
		goto bad;
	}
#endif
	switch (exdata.ex_exec.a_magic) {

#ifndef	romp  /* Complain and I'll fix it.  When I get time, that is. -BjB */
	case 0407:
		exdata.ex_exec.a_data += exdata.ex_exec.a_text;
		exdata.ex_exec.a_text = 0;
		break;
#endif	romp

	case 0413:
	case 0410:
		if (exdata.ex_exec.a_text == 0) {
			u.u_error = ENOEXEC;
			goto bad;
		}
		break;

	default:
		if (exdata.ex_shell[0] != '#' ||
		    exdata.ex_shell[1] != '!' ||
		    indir) {
			u.u_error = ENOEXEC;
			goto bad;
		}
		cp = &exdata.ex_shell[2];		/* skip "#!" */
		while (cp < &exdata.ex_shell[SHSIZE]) {
			if (*cp == '\t')
				*cp = ' ';
			else if (*cp == '\n') {
				*cp = '\0';
				break;
			}
			cp++;
		}
		if (*cp != '\0') {
			u.u_error = ENOEXEC;
			goto bad;
		}
		cp = &exdata.ex_shell[2];
		while (*cp == ' ')
			cp++;
		ndp->ni_dirp = cp;
		while (*cp && *cp != ' ')
			cp++;
		cfarg[0] = '\0';
		if (*cp) {
			*cp++ = '\0';
			while (*cp == ' ')
				cp++;
			if (*cp)
				bcopy((caddr_t)cp, (caddr_t)cfarg, SHSIZE);
		}
		indir = 1;
		iput(ip);
		ndp->ni_nameiop = LOOKUP | FOLLOW;
		ndp->ni_segflg = UIO_SYSSPACE;
		ip = namei(ndp);
		if (ip == NULL)
			return;
		bcopy((caddr_t)ndp->ni_dent.d_name, (caddr_t)cfname,
		    MAXCOMLEN);
		cfname[MAXCOMLEN] = '\0';
		goto again;
	}

	/*
	 * Collect arguments on "file" in swap space.
	 */
	na = 0;
	ne = 0;
	nc = 0;
	cc = 0;
	uap = (struct execa *)u.u_ap;
#if	MACH_VM
	exec_args = zalloc(arg_zone);
	cp = exec_args;		/* running pointer for copy */
	cc = NCARGS;		/* size of exec_args */
#else	MACH_VM
	bno = rmalloc(argmap, (long)ctod(clrnd((int)btoc(NCARGS))));
	if (bno == 0) {
		swkill(u.u_procp, "exec: no swap space");
		goto bad;
	}
	if (bno % CLSIZE)
		panic("execa rmalloc");
#endif	MACH_VM
	/*
	 * Copy arguments into file in argdev area.
	 */
	if (uap->argp) for (;;) {
		ap = NULL;
		sharg = NULL;
		if (indir && na == 0) {
			sharg = cfname;
			ap = (int)sharg;
			uap->argp++;		/* ignore argv[0] */
		} else if (indir && (na == 1 && cfarg[0])) {
			sharg = cfarg;
			ap = (int)sharg;
		} else if (indir && (na == 1 || na == 2 && cfarg[0]))
			ap = (int)uap->fname;
		else if (uap->argp) {
			ap = fuword((caddr_t)uap->argp);
			uap->argp++;
		}
		if (ap == NULL && uap->envp) {
			uap->argp = NULL;
			if ((ap = fuword((caddr_t)uap->envp)) != NULL)
				uap->envp++, ne++;
		}
		if (ap == NULL)
			break;
		na++;
		if (ap == -1) {
			u.u_error = EFAULT;
			break;
		}
		do {
#if	MACH_VM
			if (nc >= NCARGS-1) {
				error = E2BIG;
				break;
			}
#else	MACH_VM
			if (cc <= 0) {
				/*
				 * We depend on NCARGS being a multiple of
				 * CLSIZE*NBPG.  This way we need only check
				 * overflow before each buffer allocation.
				 */
				if (nc >= NCARGS-1) {
					error = E2BIG;
					break;
				}
				if (bp)
					bdwrite(bp);
				cc = CLSIZE*NBPG;
				bp = getblk(argdev, bno + ctod(nc/NBPG), cc);
				cp = bp->b_un.b_addr;
			}
#endif	MACH_VM
			if (sharg) {
				error = copystr(sharg, cp, cc, &len);
				sharg += len;
			} else {
				error = copyinstr((caddr_t)ap, cp, cc, &len);
				ap += len;
			}
			cp += len;
			nc += len;
			cc -= len;
		} while (error == ENOENT);
		if (error) {
			u.u_error = error;
#if	MACH_VM
#else	MACH_VM
			if (bp)
				brelse(bp);
			bp = 0;
#endif	MACH_VM
			goto badarg;
		}
	}
#if	MACH_VM
#else	MACH_VM
	if (bp)
		bdwrite(bp);
	bp = 0;
#endif	MACH_VM
	nc = (nc + NBPW-1) & ~(NBPW-1);
	getxfile(ip, &exdata.ex_exec, nc + (na+4)*NBPW, uid, gid);
	if (u.u_error) {
badarg:
#if	MACH_VM
	/*
	 *	NOTE: to prevent a race condition, getxfile had
	 *	to temporarily unlock the inode.  If new code needs to
	 *	be inserted here before the iput below, and it needs
	 *	to deal with the inode, keep this in mind.
	 */
#else	MACH_VM
		for (cc = 0; cc < nc; cc += CLSIZE*NBPG) {
			bp = baddr(argdev, bno + ctod(cc/NBPG), CLSIZE*NBPG);
			if (bp) {
				bp->b_flags |= B_AGE;		/* throw away */
				bp->b_flags &= ~B_DELWRI;	/* cancel io */
				brelse(bp);
				bp = 0;
			}
		}
#endif	MACH_VM
		goto bad;
	}
	iput(ip);
	ip = NULL;

	/*
	 * Copy back arglist.
	 */
	ucp = USRSTACK - nc - NBPW;
	ap = ucp - na*NBPW - 3*NBPW;
	u.u_ar0[SP] = ap;
	(void) suword((caddr_t)ap, na-ne);
	nc = 0;
	cc = 0;
#if	MACH_VM
	cp = exec_args;
	cc = NCARGS;
#endif	MACH_VM
	for (;;) {
		ap += NBPW;
		if (na == ne) {
			(void) suword((caddr_t)ap, 0);
			ap += NBPW;
		}
		if (--na < 0)
			break;
		(void) suword((caddr_t)ap, ucp);
		do {
#if	MACH_VM
#else	MACH_VM
			if (cc <= 0) {
				if (bp)
					brelse(bp);
				cc = CLSIZE*NBPG;
				bp = bread(argdev, bno + ctod(nc / NBPG), cc);
				bp->b_flags |= B_AGE;		/* throw away */
				bp->b_flags &= ~B_DELWRI;	/* cancel io */
				cp = bp->b_un.b_addr;
			}
#endif	MACH_VM
			error = copyoutstr(cp, (caddr_t)ucp, cc, &len);
			ucp += len;
			cp += len;
			nc += len;
			cc -= len;
		} while (error == ENOENT);
		if (error == EFAULT)
			panic("exec: EFAULT");
	}
	(void) suword((caddr_t)ap, 0);

	/*
	 * Reset caught signals.  Held signals
	 * remain held through p_sigmask.
	 */
	while (u.u_procp->p_sigcatch) {
		nc = ffs(u.u_procp->p_sigcatch);
		u.u_procp->p_sigcatch &= ~sigmask(nc);
		u.u_signal[nc] = SIG_DFL;
	}
	/*
	 * Reset stack state to the user stack.
	 * Clear set of signals caught on the signal stack.
	 */
	u.u_onstack = 0;
	u.u_sigsp = 0;
	u.u_sigonstack = 0;

	for (nc = u.u_lastfile; nc >= 0; --nc) {
		if (u.u_pofile[nc] & UF_EXCLOSE) {
			closef(u.u_ofile[nc]);
			u.u_ofile[nc] = NULL;
			u.u_pofile[nc] = 0;
		}
		u.u_pofile[nc] &= ~UF_MAPPED;
	}
	while (u.u_lastfile >= 0 && u.u_ofile[u.u_lastfile] == NULL)
		u.u_lastfile--;
	setregs(exdata.ex_exec.a_entry);
	/*
	 * Remember file name for accounting.
	 */
#if	CS_COMPAT
	u.u_acflag &= ~(AFORK|A41MODE|AREADIR);
#if	MACH_ONLY
	/*
	 *	always clear 4.2 mode.
	 */
#else	MACH_ONLY
	if (u.u_modes&UMODE_ALLOWOLD)
#endif	MACH_ONLY
		u.u_acflag &= ~(A42MODE);
	{
	    	extern int nsigcode[5];

		/*
		 *  The signal trampoline code may have been set to the
		 *  4.1 version if the previous process used the old
		 *  signal mechanism.  If so, reset it to the 4.2 version
		 *  until this process indicates otherwise (e.g. by
		 *  making an old signal call itself).
		 */
		if (u.u_pcb.pcb_sigc[0] != nsigcode[0])
			bcopy((caddr_t)nsigcode, (caddr_t)u.u_pcb.pcb_sigc,
			      sizeof(nsigcode));
	}
#else	CS_COMPAT
	u.u_acflag &= ~AFORK;
#endif	CS_COMPAT
	if (indir)
		bcopy((caddr_t)cfname, (caddr_t)u.u_comm, MAXCOMLEN);
	else {
		if (ndp->ni_dent.d_namlen > MAXCOMLEN)
			ndp->ni_dent.d_namlen = MAXCOMLEN;
		bcopy((caddr_t)ndp->ni_dent.d_name, (caddr_t)u.u_comm,
		    (unsigned)(ndp->ni_dent.d_namlen + 1));
	}
bad:
#if	MACH_VM
	if (exec_args)
		zfree(arg_zone, exec_args);
#else	MACH_VM
	if (bp)
		brelse(bp);
	if (bno)
		rmfree(argmap, (long)ctod(clrnd((int) btoc(NCARGS))), bno);
#endif	MACH_VM
	if (ip)
		iput(ip);
}

/*
 * Read in and set up memory for executed file.
 */
getxfile(ip, ep, nargc, uid, gid)
	register struct inode *ip;
	register struct exec *ep;
	int nargc, uid, gid;
{
	size_t ts, ds, ids, uds, ss;
	int pagi;
#if	MACH_VM
	vm_offset_t	addr;
	vm_size_t	size;
	vm_map_t	my_map;
	vm_map_t	temp_map;
	vm_pager_id_t	pager_id;
	int		cluster_size;
	vm_size_t	copy_size;
	vm_offset_t	copy_end, data_end;
	int		s;
#endif	MACH_VM

	if (ep->a_magic == 0413)
		pagi = SPAGI;
	else
		pagi = 0;
	if (ip->i_flag & IXMOD) {			/* XXX */
		u.u_error = ETXTBSY;
		goto bad;
	}
	if (ep->a_text != 0 && (ip->i_flag&ITEXT) == 0 &&
	    ip->i_count != 1) {
		register struct file *fp;

		for (fp = file; fp < fileNFILE; fp++) {
			if (fp->f_type == DTYPE_INODE &&
			    fp->f_count > 0 &&
			    (struct inode *)fp->f_data == ip &&
			    (fp->f_flag&FWRITE)) {
				u.u_error = ETXTBSY;
				goto bad;
			}
		}
	}

	/*
	 * Compute text and data sizes and make sure not too large.
	 * NB - Check data and bss separately as they may overflow 
	 * when summed together.
	 */
	ts = clrnd(btoc(ep->a_text));
	ids = clrnd(btoc(ep->a_data));
	uds = clrnd(btoc(ep->a_bss));
	ds = clrnd(btoc(ep->a_data + ep->a_bss));
	ss = clrnd(SSIZE + btoc(nargc));
#if	MACH_VM
#else	MACH_VM
	if (chksize((unsigned)ts, (unsigned)ids, (unsigned)uds, (unsigned)ss))
		goto bad;

	/*
	 * Make sure enough space to start process.
	 */
	u.u_cdmap = zdmap;
	u.u_csmap = zdmap;
	if (swpexpand(ds, ss, &u.u_cdmap, &u.u_csmap) == NULL)
		goto bad;

	/*
	 * At this point, committed to the new image!
	 * Release virtual memory resources of old process, and
	 * initialize the virtual memory of the new process.
	 * If we resulted from vfork(), instead wakeup our
	 * parent who will set SVFDONE when he has taken back
	 * our resources.
	 */
	if ((u.u_procp->p_flag & SVFORK) == 0)
		vrelvm();
	else {
		u.u_procp->p_flag &= ~SVFORK;
		u.u_procp->p_flag |= SKEEP;
		wakeup((caddr_t)u.u_procp);
		while ((u.u_procp->p_flag & SVFDONE) == 0)
			sleep((caddr_t)u.u_procp, PZERO - 1);
		u.u_procp->p_flag &= ~(SVFDONE|SKEEP);
	}
#endif	MACH_VM

#if	CS_XONLY || CS_COMPAT
	u.u_procp->p_flag &= ~(SPAGI|SSEQL|SUANOM|SOUSIG
#if	CS_COMPAT
			       |SJCSIG
#endif	CS_COMPAT
#if	CS_XONLY
			       |SXONLY
#endif	CS_XONLY
			      );
#else	CS_XONLY || CS_COMPAT
	u.u_procp->p_flag &= ~(SPAGI|SSEQL|SUANOM|SOUSIG);
#endif	CS_XONLY || CS_COMPAT
#if	CS_XONLY
	if (access(ip, IREAD))
	{
		u.u_procp->p_flag |= SXONLY;
		u.u_procp->p_flag &= ~STRC;
		u.u_error = 0;
	}
#endif	CS_XONLY
#if	CS_COMPAT
	if (ip->i_mode&ISVTX)
		u.u_acflag &= ~(A42MODE);
#endif	CS_COMPAT
	u.u_procp->p_flag |= pagi;
#if	MACH_VM

#define	unix_break_size	(8*1024*1024)	/* XXX must be constant for persistent shared memory */
#define	unix_stack_size	(u.u_rlimit[RLIMIT_STACK].rlim_cur)

#ifdef	romp
	romp_getxfile(ip, ep, pagi);
#else	romp

	my_map = current_task()->map;
	/*
	 *	Even if we are exec'ing the same image (the rem server
	 *	does this, for example), we don't have to unlock the
	 *	inode; deallocating it doesn't require it to be locked.
	 *
	 */
	vm_map_remove(my_map, vm_map_min(my_map), trunc_page((vm_offset_t) &u));
/*	vm_map_remove(my_map, vm_map_min(my_map),
				vm_map_min(my_map) + unix_break_size);*/
	
	/*
	 *	Allocate low-memory stuff: text, data, bss, space for brk calls.
	 *	Read text&data into lowest part, then make text read-only.
	 */

	addr = VM_MIN_ADDRESS;
/*	size = (vm_size_t) unix_break_size;
	size = round_page(size);*/

	size = round_page(ep->a_text + ep->a_data + ep->a_bss);
	if (vm_map_find(my_map, VM_OBJECT_NULL, (vm_offset_t) 0, &addr, size, FALSE)
			!= KERN_SUCCESS)
		panic("getxfile: cannot find space for exec image");

	u.u_error = 0;

	if (pagi == 0) {	/* not demand paged */
		/*
		 *	Read in the data segment (0407 & 0410).  It goes on the
		 *	next loader_page boundary after the text.
		 */
		u.u_error = rdwri(UIO_READ, ip,
				(caddr_t) VM_MIN_ADDRESS + loader_round_page(ep->a_text),
				(int)ep->a_data,
				(int)(sizeof(struct exec)+ep->a_text),
				0, (int *)0);
		/*
		 *	Read in text segment if necessary (0410), 
		 *	and read-protect it.
		 */
		if ((u.u_error == 0) && (ep->a_text > 0)) {
			u.u_error = rdwri(UIO_READ, ip,
				(caddr_t) VM_MIN_ADDRESS,
				(int)ep->a_text,
				(int)sizeof(struct exec), 2, (int *) 0);
			if (u.u_error == 0) {
				(void) vm_map_protect(my_map,
					 VM_MIN_ADDRESS,
					 VM_MIN_ADDRESS + trunc_page(ep->a_text),
					 VM_PROT_READ|VM_PROT_EXECUTE,
					 FALSE);
			}
		}
	}
	else {
		/*
		 *	Allocate a region backed by the exec'ed inode.
		 */

		copy_size = round_page(ep->a_text + ep->a_data);

		temp_map = vm_map_create(pmap_create(copy_size), VM_MIN_ADDRESS,
				VM_MIN_ADDRESS + copy_size, TRUE);
		pager_id = text_pager_setup(ip);
		addr = VM_MIN_ADDRESS;
		if (vm_allocate_with_pager(temp_map, &addr, copy_size, FALSE,
		    vm_pager_text, pager_id, (vm_offset_t) 1024) != KERN_SUCCESS)
			panic("getxfile: cannot map text file into user address space");
		/*
		 *	Copy the region into the target address map, so that it is a
		 *	copy(on-write) of the file.
		 */
		s = splvm();

		if (vm_map_copy(my_map, temp_map, VM_MIN_ADDRESS, copy_size,
			addr, FALSE, FALSE) != KERN_SUCCESS)	/* XXX */
				panic("getxfile: vm_map_copy of inode region failed\n");

		splx(s);

		vm_map_deallocate(temp_map);
		/*
		 *	Read-protect just the text region.  Do this before 
		 *	we zero the bss area, so that we have only one copy
		 *	of the text.
		 */

		(void) vm_map_protect(my_map,
			 VM_MIN_ADDRESS,
			 VM_MIN_ADDRESS + trunc_page(ep->a_text),
			 VM_PROT_READ|VM_PROT_EXECUTE,
			 FALSE);

		/*
		 *	If the data segment does not end on a VM page boundary,
		 *	we have to clear the remainder of the VM page it ends on
		 *	so that the bss segment will (correctly) be zero.
		 *	The loader has already guaranteed that the (text+data)
		 *	segment ends on a loader_page boundary.
		 */

		data_end = VM_MIN_ADDRESS + loader_round_page(ep->a_text + ep->a_data);
		copy_end = VM_MIN_ADDRESS + copy_size;
		if (copy_end > data_end) {
			/*
			 *	Unfortunately, we have to unlock the inode, since the
			 *	inode pager locks it!  (Yes, this bzero will fault on
			 *	the inode to bring in part of the text.)
			 */
			iunlock(ip);
			bzero((caddr_t)data_end, (vm_size_t) copy_end - data_end);
			ilock(ip);
		}
	}

	/*
	 *	Create the stack.  (Deallocate the old one and create a 
	 *	new one).
	 */

	size = round_page(unix_stack_size);
	addr = trunc_page((vm_offset_t)USRSTACK - size);
	vm_map_remove(my_map, addr, addr + size);
	if (vm_map_find(my_map, VM_OBJECT_NULL, (vm_offset_t) 0, &addr, size, FALSE) != KERN_SUCCESS)
		panic("getxfile: cannot find space for stack");

#endif	romp
#else	MACH_VM
	u.u_dmap = u.u_cdmap;
	u.u_smap = u.u_csmap;
	vgetvm(ts, ds, ss);

	if (pagi == 0)
		u.u_error =
		    rdwri(UIO_READ, ip,
			(char *)ctob(dptov(u.u_procp, 0)),
			(int)ep->a_data,
			(int)(sizeof (struct exec) + ep->a_text),
			0, (int *)0);
	xalloc(ip, ep, pagi);
	if (pagi && u.u_procp->p_textp)
		vinifod((struct fpte *)dptopte(u.u_procp, 0),
		    PG_FTEXT, u.u_procp->p_textp->x_iptr,
		    (long)(1 + ts/CLSIZE), (int)btoc(ep->a_data));

#ifdef vax
	/* THIS SHOULD BE DONE AT A LOWER LEVEL, IF AT ALL */
	mtpr(TBIA, 0);
#endif

	if (u.u_error)
		swkill(u.u_procp, "exec: I/O error mapping pages");
#endif	MACH_VM
	/*
	 * set SUID/SGID protections, if no tracing
	 */
	if ((u.u_procp->p_flag&STRC)==0) {
		u.u_uid = uid;
		u.u_procp->p_uid = uid;
		u.u_gid = gid;
	} else
		psignal(u.u_procp, SIGTRAP);
	u.u_tsize = ts;
	u.u_dsize = ds;
	u.u_ssize = ss;
	u.u_prof.pr_scale = 0;
bad:
	return;
}

#ifdef	MACH_VM
#ifdef	romp
romp_getxfile(ip, ep, pagi)
	struct inode	*ip;
	struct exec	*ep;
	int		pagi;
{
	vm_map_t	my_map, temp_map;
	vm_pager_id_t	pager_id;
	vm_offset_t	addr;
	long		data_end, copy_end, size, text_size, data_size;
	long		file_size;

	my_map = current_task()->map;

	/*
	 *	This is how unix processes are mapped onto romp/rosetta:
	 *
	 *	0 - 0x10000000				text
	 *	0x10000000 - 0x10800000			data/bss
	 *	USRSTACK - unix_stack_size - USRSTACK	stack
	 */

#define DATA_START	0x10000000

	/*
	 *	Deallocate the previous text, data, bss & stack segments.
	 *
	 *	Q: What about lisp processes which validate stuff above
	 *	   UAREA?  Soln: move the U-Area......
	 */

	vm_map_remove(my_map, vm_map_min(my_map),
				UAREA);

	/*
	 *	Allocate enough space for everything (esp. since
	 *	we don't really know what to allocate anyway).
	 */

	addr = VM_MIN_ADDRESS;
	if (vm_map_find(my_map, VM_OBJECT_NULL, (vm_offset_t) 0, &addr,
				round_page(ep->a_text), FALSE)
			!= KERN_SUCCESS)
		panic("getxfile: cannot find space for text segment");

	addr = DATA_START;
	size = round_page(ep->a_data + ep->a_bss);
	if (vm_map_find(my_map, VM_OBJECT_NULL, (vm_offset_t) 0,
			&addr, size, FALSE)
			!= KERN_SUCCESS)
		panic("getxfile: cannot find space for data/bss");

	u.u_error = 0;

	/*
	 *	Map the text into the temp map.
	 */

	file_size = round_page(ep->a_text)  + round_page(ep->a_data);
	temp_map = vm_map_create(pmap_create(file_size), VM_MIN_ADDRESS,
				VM_MIN_ADDRESS + file_size, TRUE);

	pager_id = text_pager_setup(ip);
	addr = VM_MIN_ADDRESS;
	text_size = round_page(ep->a_text);
	if (vm_allocate_with_pager(temp_map, &addr, text_size, FALSE,
	    vm_pager_text, pager_id,(vm_offset_t) 2048) != KERN_SUCCESS)
		panic("getxfile: cannot map text file");
	/*
	 *	Copy the text into the target address map, so that it is a
	 *	copy(on-write) of the file.
	 */
	if (vm_map_copy(my_map, temp_map, VM_MIN_ADDRESS, text_size,
			addr, FALSE, FALSE) != KERN_SUCCESS)	/* XXX */
		panic("getxfile: vm_map_copy of text failed\n");

	/*
	 *	Now copy the data segment.
	 */
	pager_id = inode_pager_setup(ip);
	addr = round_page(ep->a_text); 
	data_size = round_page(ep->a_data);
	if (vm_allocate_with_pager(temp_map, &addr, data_size, FALSE,
	    	vm_pager_inode, pager_id,
		(vm_offset_t) loader_round_page(ep->a_text) + 2048) 
	     != KERN_SUCCESS)
		panic("getxfile: cannot map data from file");

	if (vm_map_copy(my_map, temp_map, DATA_START, data_size,
			VM_MIN_ADDRESS + text_size,
			FALSE, FALSE) != KERN_SUCCESS)	/* XXX */
		panic("getxfile: vm_map_copy of data region failed\n");

	vm_map_deallocate(temp_map);

	/*
	 *	If the data segment does not end on a VM page boundary,
	 *	we have to clear the remainder of the VM page it ends on
	 *	so that the bss segment will (correctly) be zero.
	 *	The loader has already guaranteed that the (text+data)
	 *	segment ends on a loader_page boundary.
	 */

	data_end = DATA_START + loader_round_page(ep->a_data);
	copy_end = DATA_START + round_page(ep->a_data);
	if (copy_end > data_end) {
		/*
		 *	Unfortunately, we have to unlock the inode, since the
		 *	inode pager locks it!  (Yes, this bzero will fault on
		 *	the inode to bring in part of the text.)
		 */
		iunlock(ip);
		bzero((caddr_t)data_end, (vm_size_t) copy_end - data_end);
		ilock(ip);
	}
	/*
	 *	Reprotect just the text region.
	 *	Does nothing if there isn't one (407 execs).
	 */

	(void) vm_map_protect(my_map,
		 VM_MIN_ADDRESS,
		 VM_MIN_ADDRESS + trunc_page(ep->a_text),
		 VM_PROT_READ|VM_PROT_EXECUTE,
		 FALSE);

	/*
	 *	Create the stack.  (Deallocate the old one and create a new one).
	 */

	size = round_page(unix_stack_size);
	addr = trunc_page((vm_offset_t)USRSTACK - size);
	vm_map_remove(my_map, addr, addr + size);
	if (vm_map_find(my_map, VM_OBJECT_NULL, (vm_offset_t) 0,
			&addr, size, FALSE) != KERN_SUCCESS)
		panic("getxfile: cannot find space for stack");

}
#endif	romp
#endif	MACH_VM
