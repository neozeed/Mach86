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
 *	@(#)init_main.c	6.12 (Berkeley) 9/16/85
 */
#if	CMU

/*
 **********************************************************************
 * HISTORY
 *  1-Jun-86  Avadis Tevanian (avie) at Carnegie-Mellon University
 *	Conditional compilation for kernel semaphores under MACH_SEM.
 *
 * 25-May-86  Avadis Tevanian (avie) at Carnegie-Mellon University
 *	Dynamically allocate stack space for all processors except the
 *	first one up (which is already running in it's stack).
 *
 * 14-May-86  Avadis Tevanian (avie) at Carnegie-Mellon University
 *	Use rlim_cur for stack size for init (instead of rlim_max).
 *
 * 25-Apr-86  Mike Accetta (mja) at Carnegie-Mellon University
 *	CS_GENERIC:  Force UMODE_ALLOWOLD by default for now.
 *
 *  7-Apr-86  Bill Bolosky (bolosky) at Carnegie-Mellon University
 *	romp: fixed code in setup_main to get the right address for the
 *	uarea on the romp.
 *
 * 22-Mar-86  Avadis Tevanian (avie) at Carnegie-Mellon University
 *	Merged VM and Romp versions.
 *
 * 01-Mar-86  Mike Accetta (mja) at Carnegie-Mellon University
 *	CS_OLDDIR:  Force UMODE_NEWDIR for all processes if
 *	the root file system is not in the old format.
 *
 * 25-Feb-86  Avadis Tevanian (avie) at Carnegie-Mellon University
 *	Installed VM changes.
 *
 * 13-Feb-86  Bill Bolosky (bolosky) at Carnegie-Mellon University
 *	Merged in ROMP changes.
 *
 * 11-Feb-86  Mike Accetta (mja) at Carnegie-Mellon University
 *	CS_GENERIC:  Add UMODE variable to establish the default
 *	process modes for the system at boot time (and especially to
 *	allow this to be patched in).
 *
 * 25-Jan-86  Avadis Tevanian (avie) at Carnegie-Mellon University
 *	Upgraded to 4.3
 *
 * 02-Jan-86  Mike Accetta (mja) at Carnegie-Mellon University
 *	CS_RFS:  Split initialization into two parts since queue
 *	headers must now be valid before the possible multi-processor
 *	newproc() set up but the root file system isn't available until
 *	after.
 *	[V1(1)]
 *
 * 23-Oct-85  Avadis Tevanian (avie) at Carnegie-Mellon University
 *	Create kernel idle processes.
 *
 * 20-Sep-85  Robert V Baron (rvb) at Carnegie-Mellon University
 *	some code was spliced in twice both inline in binit after the
 * 	call to swapconf() and in swapconf proper.  The code can only
 *	be called once.  Now swapconf does ALL the swap stuff and binit
 *	none.
 *
 * 20-Aug-85  Robert V Baron (rvb) at Carnegie-Mellon University
 *	accept mapaddr argument in main() and pass it on to startup()
 *
 * 03-Aug-85  Mike Accetta (mja) at Carnegie-Mellon University
 *	CS_RPAUSE:  Changed to initially enable all resource pause
 *	flags.
 *	[V1(1)]
 *
 * 2-Aug-85  David L. Black (dlb) at CMU.  Added initialization
 *	of user timer for process 0.  [Remaining processes are 
 *	created via newproc in kern_fork.c, init is there.]
 *
 * 29-Jun-85  Mike Accetta (mja) at Carnegie-Mellon University
 *	CS_OLDFS: fixed main() to also initialize mount name in old
 *	format root file system super-block.
 *	[V1(1)]
 *
 * 08-May-85  Glenn Marcy (gm0w) at Carnegie-Mellon University
 *	Upgraded from 4.1BSD.
 *	[V1(1)].
 *
 * 13-Nov-84  Avadis Tevanian (avie) at Carnegie-Mellon University
 *	Added calls to smallocinit (for memory allocator initialization)
 *	and sem_init (for semaphore initialization).  Mlattach is no
 *	longer done here, it is now done in configure().
 *
 *  4-Apr-84  Avadis Tevanian (avie) at Carnegie-Mellon University
 *	Added mlattach call during initialization.
 *
 * 25-Mar-82  Mike Accetta (mja) at Carnegie-Mellon University
 *	CS_XLDS:  Added setting of standard maximum data size limit
 *	so that when an extra large data segment is optionally included
 *	the default limit is identical to the standard segment size (V3.04a).
 *
 * 20-Jan-82  Mike Accetta (mja) at Carnegie-Mellon University
 *	CS_RFS:  Added call to rfs_init() to initialize the remote file
 *	access mechanism (V3.04b).
 *
 **********************************************************************
 */
 
#include "cs_generic.h"
#include "cs_olddir.h"
#include "cs_oldfs.h"
#include "cs_rfs.h"
#include "cs_rpause.h"
#include "cs_xlds.h"
#include "mach_mp.h"
#include "mach_sem.h"
#include "mach_time.h"
#include "mach_tt.h"
#include "mach_vm.h"
#include "cpus.h"
#endif	CMU

#include "../machine/pte.h"

#include "param.h"
#include "systm.h"
#include "dir.h"
#include "user.h"
#include "kernel.h"
#include "fs.h"
#if	CS_OLDFS
#include "filsys.h"
#endif	CS_OLDFS
#include "mount.h"
#include "map.h"
#include "proc.h"
#include "inode.h"
#include "seg.h"
#include "conf.h"
#include "buf.h"
#include "vm.h"
#include "cmap.h"
#include "text.h"
#include "clist.h"
#include "protosw.h"
#include "quota.h"
#include "../machine/reg.h"

#include "../machine/cpu.h"

#if	MACH_TIME > 0
#include "uproc.h"
#endif	MACH_TIME > 0
#if	MACH_MP
#include "../h/thread.h"
#include "../h/task.h"
#include "../h/machine.h"
#endif	MACH_MP
#if	MACH_VM
#include "../machine/pmap.h"
#include "../vm/vm_param.h"
#include "../vm/vm_page.h"
#include "../vm/vm_map.h"
#include "../vm/vm_kern.h"
#include "../vm/vm_object.h"
#endif	MACH_VM
#if	MACH_TT
#include "../h/zalloc.h"
#endif	MACH_TT

int	cmask = CMASK;
#if	CS_GENERIC
int UMODE = UMODE_ALLOWOLD;	/* to establish default modes for all processes */
#endif	CS_GENERIC
/*
 * Initialization code.
 * Called from cold start routine as
 * soon as a stack and segmentation
 * have been established.
 * Functions:
 *	clear and free user core
 *	turn on clock
 *	hand craft 0th process
 *	call all initialization routines
 *	fork - process 0 to schedule
 *	     - process 1 execute bootstrap
 *	     - process 2 to page out
 */
#if	MACH_VM
thread_t setup_main()
/*
 *	first_addr contains the first available physical address
 *	running in virtual memory on the interrupt stack
 *
 *	returns initial thread to run
 */
{
	vm_offset_t		u_addr;
	extern vm_offset_t	virtual_avail;
	port_t			dummy_port;
	vm_offset_t		end_stack, cur_stack;
	int			i;

	rqinit();
#include "loop.h"

	vm_mem_init();

	startup(virtual_avail);

	/*
	 *	Create stacks for other processors (the first
	 *	processor up uses a preallocated stack).
	 */

	cur_stack = kmem_alloc(kernel_map, NCPUS*INTSTACK_SIZE, TRUE);
	end_stack = cur_stack + NCPUS*INTSTACK_SIZE;
	for (i = 0; i < NCPUS; i++) {
			if (machine_slot[i].is_cpu && (i != master_cpu)) {
				interrupt_stack[i] = cur_stack;
				cur_stack += INTSTACK_SIZE;
			}
			else {
				interrupt_stack[i] = NULL;
			}
	}

	/*
	 *	Free up any stacks we really didn't need.
	 */

	if (end_stack != cur_stack)
		kmem_free(kernel_map, cur_stack, end_stack - cur_stack);

	/*
	 *	Initialize the task and thread subsystems.
	 */

#if	MACH_TT
	/*
	 * This is a convienent place to do this.  This
	 * keeps us from including user.h in thread.c
	 */
	{
#include "../h/mach_param.h"
		extern struct zone *u_zone;
		u_zone = zinit(sizeof(struct user),
			THREAD_MAX * sizeof(struct user),
			THREAD_CHUNK * sizeof(struct user),
			FALSE, "u-areas");
	}
#endif	MACH_TT
	task_init();
	thread_init();

	/*
	 *	Create proc[0]'s u area.
	 */

	(void) task_create(TASK_NULL, FALSE, &task_table[0], &dummy_port);

#if	MACH_TT
#else	MACH_TT
#ifdef	romp
	u_addr = (vm_offset_t) UAREA;
#else	romp
	u_addr = (vm_offset_t) &u;
#endif	romp
	if (vm_map_find(task_table[0]->map, vm_object_allocate(round_page(sizeof(u))),
		(vm_offset_t) 0, &u_addr, ptob(UPAGES), FALSE) != KERN_SUCCESS)
		panic("main: cannot alloc proc[0] map");
	if (vm_map_pageable(task_table[0]->map, u_addr, round_page(u_addr + ptob(UPAGES)), FALSE)
		!= KERN_SUCCESS)
		panic("main: cannot wire down proc[0] u");

#endif	MACH_TT
	(void) thread_create(task_table[0], &thread_table[0], &dummy_port);
	initial_context(thread_table[0]);

	/*
	 *	Return to assembly code to start the first process.
	 */

	return(thread_table[0]);
}
main()
#else	MACH_VM
main(firstaddr)
#ifdef	romp
	register int firstaddr;  /* MUST BE REGISTER FOR ROMP as of 9/84 */
#else	romp
	int firstaddr;
#endif	romp
#endif	MACH_VM
{
	register int i;
	register struct proc *p;
	struct fs *fs;
	int s;
#if	MACH_MP
	struct proc *init_proc, *pager_proc;
#endif	MACH_MP
#if	MACH_TIME
	register struct uproc *up;
#endif	MACH_TIME

#if	MACH_VM
	vm_offset_t	init_addr;
	vm_size_t	init_size;
#ifdef	romp
	extern int	sigcode;	/* XXX - shouldn't be an int really */
#endif	romp
#else	MACH_VM

	rqinit();
#include "loop.h"
	startup(firstaddr);
#endif	MACH_VM

	/*
	 * set up system process 0 (swapper)
	 */
	p = &proc[0];
#if	MACH_VM
#ifdef	romp
	bcopy(&sigcode,u.u_pcb.pcb_sigc,sizeof(u.u_pcb.pcb_sigc));
#endif	romp
	p->p_p0br = (struct pte *) u.u_pcb.pcb_p0br;
#else	MACH_VM
	p->p_p0br = u.u_pcb.pcb_p0br;
#endif	MACH_VM
	p->p_szpt = 1;
	p->p_addr = uaddr(p);
	p->p_stat = SRUN;
	p->p_flag |= SLOAD|SSYS;
	p->p_nice = NZERO;
#if	MACH_VM
	pmap_redzone(vm_map_pmap(task_table[0]->map), round_page(((&u) + 1)));
#else	MACH_VM
	setredzone(p->p_addr, (caddr_t)&u);
#endif	MACH_VM
	u.u_procp = p;
#ifdef vax
	/*
	 * These assume that the u. area is always mapped 
	 * to the same virtual address. Otherwise must be
	 * handled when copying the u. area in newproc().
	 */
	u.u_nd.ni_iov = &u.u_nd.ni_iovec;
	u.u_ap = u.u_arg;
#endif
	u.u_nd.ni_iovcnt = 1;
	u.u_cmask = cmask;
	u.u_lastfile = -1;
	for (i = 1; i < NGROUPS; i++)
		u.u_groups[i] = NOGROUP;
	for (i = 0; i < sizeof(u.u_rlimit)/sizeof(u.u_rlimit[0]); i++)
		u.u_rlimit[i].rlim_cur = u.u_rlimit[i].rlim_max = 
		    RLIM_INFINITY;
#if	CS_GENERIC
	u.u_modes = UMODE;
#endif	CS_GENERIC
#if	MACH_VM
        u.u_rlimit[RLIMIT_STACK].rlim_cur = ctob(DFLSSIZ);
        u.u_rlimit[RLIMIT_STACK].rlim_max = ctob(MAXSSIZ);
        u.u_rlimit[RLIMIT_DATA].rlim_cur = ctob(DFLDSIZ);
        u.u_rlimit[RLIMIT_DATA].rlim_max = ctob(MAXDSIZ);
#else	MACH_VM
	/*
	 * configure virtual memory system,
	 * set vm rlimits
	 */
	vminit();
#endif	MACH_VM

#if defined(QUOTA)
	qtinit();
	p->p_quota = u.u_quota = getquota(0, 0, Q_NDQ);
#endif
#if	MACH_TIME
	up = &uproc[0];
	up->up_utime.tr_timestamp = 0;
	up->up_utime.tr_elapsed.tv_sec = 0;
	up->up_utime.tr_elapsed.tv_usec = 0;
#endif	MACH_TIME
	startrtclock();

#ifdef	vax
#include "kg.h"
#if NKG > 0
	startkgclock();
#endif
#endif	vax

	/*
	 * Initialize tables, protocols, and set up well-known inodes.
	 */
	mbinit();
#if	CS_RFS
	/*
	 *  RFS initialization part 1.
 	 *
	 *  Initialize all data structures which could not be handled at
	 *  compile-time.  This includes, in particular, the parallel process
	 *  table queue headers which must be valid before the newproc() call
	 *  below.
	 */
	rfs_init();
#endif	CS_RFS
#if	MACH_SEM
	sem_init();			/* initialize semaphores */
#endif	MACH_SEM
	cinit();
#if NLOOP > 0
	loattach();			/* XXX */
#endif
	/*
	 * Block reception of incoming packets
	 * until protocols have been initialized.
	 */
	s = splimp();
	ifinit();
	domaininit();
	splx(s);
	pqinit();
	ihinit();
	bhinit();
#if	MACH_MP
	/*
	 *	Create kernel idle cpu processes.  This must be done
 	 *	before a context switch can occur (and hence I/O can
	 *	happen in the binit() call).
	 */
	proc[0].p_szpt = clrnd(ctopt(UPAGES));
	u.u_rdir = NULL;
	u.u_cdir = NULL;
	/*
	 *	HACK - HACK - HACK
	 *	Unix MUST have init and swapper in proc slots
	 *	1 and 2 (well maybe not init).  So, move these
	 *	proc slots off the free list for now.
	 */
	init_proc = freeproc;
	pager_proc = init_proc->p_nxt;
	freeproc = pager_proc->p_nxt;
	mpid = 2;
	for (i = 0; i < NCPUS; i++) {
		if (machine_slot[i].is_cpu == FALSE)
			continue;
		if (newproc(0)) {
			u.u_procp->p_flag |= SLOAD|SSYS|SIDLE;
#if	MACH_VM
			thread_table[u.u_procp-proc]->whichq =
#else	MACH_VM
			thread[u.u_procp-proc].whichq =
#endif	MACH_VM
					&local_runq[i];
			bcopy("idle_cpu", u.u_comm, 9);		/* XXX */
			idle_thread();
			/*NOTREACHED*/
		}
	}
	/*
	 *	Now put them back.
	 */
	pager_proc->p_nxt = freeproc;
	init_proc->p_nxt = pager_proc;
	freeproc = init_proc;
	mpid = 0;
#endif	MACH_MP
	binit();
	bswinit();
	nchinit();
#ifdef GPROF
	kmstartup();
#endif

	fs = mountfs(rootdev, 0, (struct inode *)0);
	if (fs == 0)
		panic("iinit");
	bcopy("/", fs->fs_fsmnt, 2);

	inittodr(fs->fs_time);
	boottime = time;

/* kick off timeout driven events by calling first time */
	roundrobin();
	schedcpu();
#if	MACH_VM
	compute_mach_factor();
#else	MACH_VM
	schedpaging();
#endif	MACH_VM

/* set up the root file system */
	rootdir = iget(rootdev, fs, (ino_t)ROOTINO);
	iunlock(rootdir);
	u.u_cdir = iget(rootdev, fs, (ino_t)ROOTINO);
	iunlock(u.u_cdir);
	u.u_rdir = NULL;
#if	CS_RFS
	/*
	 *  RFS initialization part 2.
	 *
	 *  Switch over to the local root.
	 */
	rfs_initroot();
#endif	CS_RFS
#if	CS_OLDDIR
	if (!isolddir(rootdir))
		u.u_modes |= UMODE_NEWDIR;
#endif	CS_OLDDIR
#if	CS_RPAUSE
	/*
	 *  Default to pausing process on these errors.
	 */
	u.u_rpause = (URPS_AGAIN|URPS_NOMEM|URPS_NFILE|URPS_NOSPC);
#endif	CS_RPAUSE

	u.u_dmap = zdmap;
	u.u_smap = zdmap;

	/*
	 * make init process
	 */

	proc[0].p_szpt = CLSIZE;
	if (newproc(0)) {
#if	CS_GENERIC
		bcopy("init", u.u_comm, 4);	/* XXX */
#endif	CS_GENERIC
#if	MACH_VM
		/*
		 *	Create space for icode.
		 */
		init_size = round_page(szicode);
		init_addr = VM_MIN_ADDRESS;
		vm_map_find(current_task()->map,
			vm_object_allocate(init_size), (vm_offset_t) 0, 
			&init_addr, init_size, FALSE);
		/*
		 *	Create some stack space.
		 */
		init_size = round_page(u.u_rlimit[RLIMIT_STACK].rlim_cur);
		init_addr = ((vm_offset_t) &u)- init_size;
		vm_map_find(task_table[u.u_procp-proc]->map, 
			vm_object_allocate(init_size), (vm_offset_t) 0, 
			&init_addr, init_size, FALSE);
#else	MACH_VM
		expand(clrnd((int)btoc(szicode)), 0);
		(void) swpexpand(u.u_dsize, 0, &u.u_dmap, &u.u_smap);
#endif	MACH_VM
		(void) copyout((caddr_t)icode, (caddr_t)0, (unsigned)szicode);
		/*
		 * Return goes to loc. 0 of user init
		 * code just copied out.
		 */
		return;
	}
#if	MACH_VM
	/*
	 *	Crank up the pageout daemon.
	 */

	if (newproc(0)) {
		u.u_procp->p_flag |= SLOAD|SSYS;
		bcopy("page-out", u.u_comm, 9);
		vm_pageout();
		/*NOTREACHED*/
	}
#else	MACH_VM
	/*
	 * make page-out daemon (process 2)
	 * the daemon has ctopt(nswbuf*CLSIZE*KLMAX) pages of page
	 * table so that it can map dirty pages into
	 * its address space during asychronous pushes.
	 */
	proc[0].p_szpt = clrnd(ctopt(nswbuf*CLSIZE*KLMAX + UPAGES));
	if (newproc(0)) {
		proc[2].p_flag |= SLOAD|SSYS;
		proc[2].p_dsize = u.u_dsize = nswbuf*CLSIZE*KLMAX; 
#if	CS_GENERIC
		bcopy("page-out", u.u_comm, 9);	/* XXX */
#endif	CS_GENERIC
		pageout();
		/*NOTREACHED*/
	}
#endif	MACH_VM

#if	MACH_MP
	mpid = 2 + NCPUS;
#endif	MACH_MP
#if	MACH_VM
	/*
	 *	We don't have a swapper yet, so just sleep.
	 */
	bcopy("swapper", u.u_comm, 7);
	sleep(main);
#else	MACH_VM
	/*
	 * enter scheduling loop
	 */
#if	CS_GENERIC
	bcopy("swapper", u.u_comm, 7);		/* XXX */
#endif	CS_GENERIC
	proc[0].p_szpt = 1;
	sched();
#endif	MACH_VM
}

/*
 * Initialize hash links for buffers.
 */
bhinit()
{
	register int i;
	register struct bufhd *bp;

	for (bp = bufhash, i = 0; i < BUFHSZ; i++, bp++)
		bp->b_forw = bp->b_back = (struct buf *)bp;
}

/*
 * Initialize the buffer I/O system by freeing
 * all buffers and setting all device buffer lists to empty.
 */
binit()
{
	register struct buf *bp, *dp;
	register int i;
	struct swdevt *swp;
	int base, residual;

	for (dp = bfreelist; dp < &bfreelist[BQUEUES]; dp++) {
		dp->b_forw = dp->b_back = dp->av_forw = dp->av_back = dp;
		dp->b_flags = B_HEAD;
	}
	base = bufpages / nbuf;
	residual = bufpages % nbuf;
	for (i = 0; i < nbuf; i++) {
		bp = &buf[i];
		bp->b_dev = NODEV;
		bp->b_bcount = 0;
		bp->b_un.b_addr = buffers + i * MAXBSIZE;
#if	MACH_VM
		if (i < residual)
			bp->b_bufsize = (base + 1) * page_size;
		else
			bp->b_bufsize = base * page_size;
#else	MACH_VM
		if (i < residual)
			bp->b_bufsize = (base + 1) * CLBYTES;
		else
			bp->b_bufsize = base * CLBYTES;
#endif	MACH_VM
		binshash(bp, &bfreelist[BQ_AGE]);
		bp->b_flags = B_BUSY|B_INVAL;
		brelse(bp);
	}
	/*
	 * Count swap devices, and adjust total swap space available.
	 * Some of this space will not be available until a vswapon()
	 * system is issued, usually when the system goes multi-user.
	 */
#if	MACH_VM
#else	MACH_VM
	nswdev = 0;
	nswap = 0;
	for (swp = swdevt; swp->sw_dev; swp++) {
		nswdev++;
		if (swp->sw_nblks > nswap)
			nswap = swp->sw_nblks;
	}
	if (nswdev == 0)
		panic("binit");
	if (nswdev > 1)
		nswap = ((nswap + dmmax - 1) / dmmax) * dmmax;
	nswap *= nswdev;
	/*
	 * If there are multiple swap areas,
	 * allow more paging operations per second.
	 */
	if (nswdev > 1)
		maxpgio = (maxpgio * (2 * nswdev - 1)) / 2;
	swfree(0);
#endif	MACH_VM
}

/*
 * Initialize linked list of free swap
 * headers. These do not actually point
 * to buffers, but rather to pages that
 * are being swapped in and out.
 */
bswinit()
{
	register int i;
	register struct buf *sp = swbuf;

	bswlist.av_forw = sp;
	for (i=0; i<nswbuf-1; i++, sp++)
		sp->av_forw = sp+1;
	sp->av_forw = NULL;
}

/*
 * Initialize clist by freeing all character blocks, then count
 * number of character devices. (Once-only routine)
 */
cinit()
{
	register int ccp;
	register struct cblock *cp;

	ccp = (int)cfree;
	ccp = (ccp+CROUND) & ~CROUND;
	for(cp=(struct cblock *)ccp; cp < &cfree[nclist-1]; cp++) {
		cp->c_next = cfreelist;
		cfreelist = cp;
		cfreecount += CBSIZE;
	}
}
