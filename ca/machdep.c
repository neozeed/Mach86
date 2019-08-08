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
/* $Header: machdep.c,v 4.9 85/09/05 22:46:39 webb Exp $ */
/* $Source: /ibm/acis/usr/sys_ca/ca/RCS/machdep.c,v $ */

#if CMU
/***********************************************************************
 * HISTORY
 * 27-Mar-86  Bill Bolosky (bolosky) at Carnegie-Mellon University
 *	MACH_VM: Merged in VM changes.
 *
 * 28-Feb-86  Bill Bolosky (bolosky) at Carnegie-Mellon University
 *	Added definitions of bb{ss,cc}i.
 *
 * 25-Feb-86  Bill Bolosky (bolosky) at Carnegie-Mellon University
 *	Renamed mdsetregs to be setregs and added process entry point as
 *	a paramerer, since u_exdata.ux_entloc is no more.  This
 *	corresponds to 4.3.
 *
 * 25-Feb-86  Bill Bolosky (bolosky) at Carnegie-Mellon University
 *	Changed sigmask in sendsig to me sigmsk to avoid collision with
 *	CMU macro.
 *
 */
#include "cs_generic.h"
#include "cmu_bugfix.h"
#include "cs_boot.h"
#include "cs_compat.h"
#include "cs_ipc.h"
#include "cs_kdb.h"
#include "cs_lint.h"
#include "cs_rfs.h"
#include "romp_debug.h"
#include "mach_acc.h"
#include "mach_load.h"
#include "mach_mp.h"
#include "mach_mpm.h"
#include "wb_ml.h"
#include "mach_shm.h"
#include "mach_time.h"
#include "mach_vm.h"
 
#include "cpus.h"
#endif CMU

/*     machdep.c       6.2     83/10/02        */

#include "../machine/reg.h"
#include "../machine/pte.h"
#include "../machine/rosetta.h"
#include "../machine/scr.h"
#include "../machine/debug.h"
#ifdef DUALCALL
#include "../machine/frame.h"
#endif DUALCALL
#include "../h/param.h"
#include "../h/systm.h"
#include "../h/dir.h"
#include "../h/user.h"
#include "../h/kernel.h"
#include "../h/map.h"
#include "../h/vm.h"
#include "../h/proc.h"
#include "../h/buf.h"
#include "../h/reboot.h"
#include "../h/conf.h"
#include "../h/inode.h"
#include "../h/file.h"
#include "../h/text.h"
#include "../h/clist.h"
#include "../h/callout.h"
#include "../h/cmap.h"
#include "../h/mbuf.h"
#include "../h/msgbuf.h"
#include "../h/quota.h"
#include "../machine/rpb.h"
#include "../machine/io.h"
#include "../h/acct.h"
#if	CS_RFS
#include "rfs.h"
#endif	CS_RFS
#if	MACH_SHM
#include "../mp/shmem.h"
#endif	MACH_SHM
#if	MACH_MP
#include "../sync/mp_queue.h"
#include "../mp/remote_prim.h"
#include "../mp/mach_stat.h"
#endif	MACH_MP

#if	MACH_ACC
#undef	boolean
#include "../accent/accent.h"
#endif	MACH_ACC

#if	MACH_MP
#include "../mp/mp_proc.h"
#include "thread.h"
#include "../sync/lock.h"
#endif	MACH_MP

#if	MACH_VM
#include "../vm/vm_kern.h"
#include "../vm/vm_param.h"
#include "../h/zalloc.h"
struct zone	*arg_zone;
extern char	*shutl, *eshutl;
#endif	MACH_VM

short  icode[] =
{/* romp version:                                                           */
 /* 0000 */                /* front: .using front,r0                        */
 /* 0000 */ 0xc810,0x07f0, /*  cal r1,0x7F0  | stack                        */
 /* 0004 */ 0xc820,0x0020, /*  cal r2,fsold                                 */
 /* 0008 */ 0xc830,0x0018, /*  cal r3,argp1                                 */
 /* 000c */ 0xc840,0x001c, /*  cal r4,envp1                                 */
 /* 0010 */ 0xa400,        /*  lis r0,0                                     */
 /* 0012 */ 0x3001,        /*  sts r0,0(sp)                                 */
 /* 0014 */ 0xc000,0x003b, /*  svc 59(r0)    | exec no return if ok)        */
 /* 0018 */ 0x0000,0x0020, /* argp1: .long fsold                            */
 /* 001c */ 0x0000,0x0000, /* envp1: .long 0                                */
 /* 0020 */ 0x2f65,0x7463, /* fsold: .asciz "/etc/init"                     */
 /* 0024 */ 0x2f69,0x6e69, /*                                               */
 /* 0028 */ 0x7400,        /*                                               */
 /* 0030 */ 0x0000,        /*  .align 4                                     */
 /* 00bc */                /*  .end                                         */
};
int    szicode = sizeof(icode);

/*
 * Declare these as initialized data so we can patch them.
 */
int    nbuf = 0;
int    nswbuf = 0;
int    bufpages = 0;
char	ccr_default = { CCR_DEFAULT };		/* see ../machine/scr.h for bit definitions */

#if	MACH_VM
vm_map_t	buffer_map;
#endif	MACH_VM

/*
 * Machine-dependent startup code
 * an important thing to remember is that 'physmem' is the count of the actual
 * physical memory; 'endmem' is the size that includes all of memory and
 * 'holelength' is the size of the hole in memory (if there is one).
 * maxmem is the page address of the actual effective end of memory.
 */
startup(firstaddr)
#if	MACH_VM
	caddr_t	firstaddr;
#else	MACH_VM
	int firstaddr;
#endif	MACH_VM
{
	int unixsize;
	register unsigned i, j;
	register caddr_t v;
	struct pte tmp_pte;
	int vpage, frame, skippages;
	int base, residual;
	extern char start, etext, edata, end;
	extern bufbase, buflimit;
#if	MACH_VM
	vm_size_t	size;
	kern_return_t	ret;
	vm_offset_t	trash_offset;
#endif	MACH_VM

	INIT_INTR();			  /* initialize interrupt system */
	/*
	 * THIS SHOULD BE DONE BEFORE ANY PRINTFs OR PUTCHARs.
	 * otherwise any messages won't appear in the console message
	 * buffer. 'msgbufp' is used to prevent any messages that
	 * appear before this point (panics?) from causing more
	 * traps due to non-existance of the virtual address of the 
	 * message buffer.
	 * Grab a couple of real pages for the console message
	 * buffer at the end of core just before the HAT/IPT table
	 * and map them to a virtual address arbitrarily assigned
	 * in locore. Reduce available memory by the amount grabbed.
	 * Remember to map out the pages before remapping them!
	 * VAX PTEs can be totally ignored here, since the pages
	 * will never be remapped.
	 */
#if	MACH_ACC
	if (nproc > LAST_USED_PROCESS)
		nproc = LAST_USED_PROCESS;		/* for now */
#endif	MACH_ACC
#if	MACH_VM
	/* XXX - need to handle message buffer */
#else	MACH_VM
/* WARNING: Code in romp_init.c assumes that msgbuf is allocated here and
	    is of this size.  If you change this, change that, too.
 */
	maxmem -= btoc(sizeof (struct msgbuf));
	frame = maxmem;
	vpage = btop(&msgbuf);
	for (i = 0; i < btoc(sizeof (struct msgbuf)); i++) {
		mapout(&frame, 1);
		mapin(&tmp_pte, vpage++, PG_KW|frame++, 1);
	}
	msgbufp = (struct msgbuf *) vtop(&msgbuf);	/* mark it */
#endif	MACH_VM

	/*
	 * Initialize the console port
	 */

#if	MACH_VM
	/*
	 *	The following constants are set to keep a bunch
	 *	of Unix code happy, all uses of these should eventually
	 *	be fixed.
	 */
	physmem = atop(mem_size);			/* XXX */
	freemem = atop(mem_size);			/* XXX */
#else	MACH_VM
	cnatch(0);
/*BJB*/	init_kbd();
	* (char *) CCR = u.u_pcb.pcb_ccr = ccr_default;	/* set up default user access */
#endif	MACH_VM

	/*
	 * Good {morning,afternoon,evening,night}.
	 */
	printf("%s", version);

#if	MACH_VM
#define MEG	(1024*1024)
	printf("physical memory = %d.%d%d megabytes.\n", mem_size/MEG,
		((mem_size%MEG)*10)/MEG,
		((mem_size%(MEG/10))*100)/MEG);
	/*
	 *	It is possible that someone has allocated some stuff
	 *	before here, (like vcons_init allocates the unibus map),
	 *	so, we look for enough space to put the dynamic data
	 *	structures, then free it with the assumption that we
	 *	can just get it back later (at the same address).
	 */
	firstaddr = (caddr_t) round_page(firstaddr);
	/*
	 *	Between the following find, and the next one below
	 *	we can't cause any other memory to be allocated.  Since
	 *	below is the first place we really need an object, it
	 *	will cause the object zone to be expanded, and will
	 *	use our memory!  Therefore we allocate a dummy object
	 *	here.  This is all a hack of course.
	 */
	ret = vm_map_find(kernel_map, vm_object_allocate(0), (vm_offset_t) 0,
				&firstaddr,8*1024*1024, TRUE);
					/* size big enough for worst case */
	if (ret != KERN_SUCCESS) {
		printf("startup: no space for dynamic structures.\n");
		panic("startup");
	}
	vm_map_remove(kernel_map, firstaddr, firstaddr + 8*1024*1024);
	v = firstaddr;
#if	MACH_MP
#define	valloc(name, type, num) \
	(name) = (type *)(v); (v) = (caddr_t)((name)+(num));\
	if (show_space) \
		printf("name = %d(0x%x) bytes @%x, %d cells @ %d bytes\n",\
		 num*sizeof(type), num*sizeof(type), name, num, sizeof (type))
#define	valloclim(name, type, num, lim) \
	(name) = (type *)(v); (v) = (caddr_t)((lim) = ((name)+(num)));\
	if (show_space) \
		printf("name = %d(0x%x) bytes @%x, %d cells @ %d bytes\n",\
		 num*sizeof(type), num*sizeof(type), name, num, sizeof (type))
#define vround() (v = (caddr_t) ( ( ((int) v) + (NBPG-1)) & ~(NBPG-1)))
#define vquadround() (v = (caddr_t) ( ( ((int) v) + (8-1)) & ~(8-1)))
#else	MACH_MP
#define	valloc(name, type, num) \
	    (name) = (type *)v; v = (caddr_t)((name)+(num))
#define	valloclim(name, type, num, lim) \
	    (name) = (type *)v; v = (caddr_t)((lim) = ((name)+(num)))
#endif	MACH_MP

#if	MACH_SHM
	/*
	 * Allocate our stuff that will be user readable...
	 * This stuff really shouldn't be user readable.
	 */
	valloclim(shrmap, struct map, 3*nproc, eshrmap);
	valloc(aarea, struct aarea, nproc);

	valloc(SHM, struct SHMstate, 1);
#endif	MACH_SHM
#if	MACH_MP
	if (sizeof (struct MACHstate) % 8)
		panic("MACHstate not quad\n");
	vquadround();
	valloc(mach_states, struct MACHstate, NCPUS);
	mach_state_init();
#endif	MACH_MP

#if	MACH_ACC
	valloc(IPC, struct IPCstate, 1);
#endif	MACH_ACC
#if	WB_ML
	valloc(mlbuf, struct mlbuf, 1);
#endif	WB_ML
#if	MACH_ACC
	vround();
	valloclim(ProcArray, struct ProcRecord, NumProcs, eProcArray);
	vround();
	valloclim(PortArray, struct PortRecord, NumPorts, ePortArray);
#endif	MACH_ACC

#if	MACH_ACC ||	MACH_SHM
	/*
	 *	It is assumed that vm_map_find will always
	 *	return the next address in the kernel's address
	 *	space.  This assumption can be released only
	 *	by rewriting the way this file allocates memory.
	 */

	v = (caddr_t) round_page(v);
	size = (vm_size_t) (v - firstaddr);
	ret = vm_map_find(kernel_map, vm_object_allocate(size), 
				(vm_offset_t) 0,&firstaddr,size, FALSE);
	if (ret != KERN_SUCCESS) {
		printf("startup: address for shared memory moved!\n");
		panic("startup");
	}
	vm_map_pageable(kernel_map, firstaddr, v, FALSE);
	/* set firstaddr to the next address to be allocated */
	firstaddr = v;
	/*
	 * the stuff here will again be PG_KW following the 4.2BSD
	 * tradition.
	 */
#endif	MACH_ACC ||	MACH_SHM
#if	CS_RFS
	valloc(rfsProcessTable, struct rfsProcessEntry, nproc);
#endif	CS_RFS
#if	MACH_MP
	valloc(mp_proc, struct mp_proc, nproc);
	valloc(thread_table, thread_t, nproc);
	valloc(task_table, task_t, nproc);
#endif	MACH_MP
#if	MACH_ACC
	valloc(aproc, struct ProcRecord *, nproc);
#endif	MACH_ACC
#if	MACH_TIME
	valloclim(uproc, struct uproc, nproc, uprocNPROC);
#endif	MACH_TIME

	valloclim(inode, struct inode, ninode, inodeNINODE);
	valloclim(file, struct file, nfile, fileNFILE);
	valloclim(proc, struct proc, nproc, procNPROC);
	valloclim(text, struct text, ntext, textNTEXT);
	valloc(cfree, struct cblock, nclist);
	valloc(callout, struct callout, ncallout);
	valloc(swapmap, struct map, nswapmap = nproc * 2);
	arg_zone = zinit(NCARGS, NCARGS*100, NCARGS, FALSE,"exec args");
	valloc(nch, struct nch, nchsize);
#ifdef QUOTA
	valloclim(quota, struct quota, nquota, quotaNQUOTA);
	valloclim(dquot, struct dquot, ndquot, dquotNDQUOT);
#endif
	
	/*
	 * Determine how many buffers to allocate.
	 * Use 10% of memory for the first 2 Meg, 5% of the remaining
	 * memory. Insure a minimum of 16 buffers.
	 * We allocate 1/2 as many swap buffer headers as file i/o buffers.
	 */
	if (bufpages == 0)
		if (mem_size < (2 * 1024 * 1024))
			bufpages = atop(mem_size / 10);
		else
/*			bufpages = atop((2 * 1024 * 1024 + mem_size) / 20);*/
			bufpages = atop(mem_size / 10);
	if (nbuf == 0) {
		/*
		 *	Put ~2K in each buffer.
		 */
		nbuf = (bufpages * page_size) / 2048;
		if (nbuf < 16)
			nbuf = 16;
	}
	if (nswbuf == 0) {
		nswbuf = (nbuf / 2) &~ 1;	/* force even */
		if (nswbuf > 256)
			nswbuf = 256;		/* sanity */
	}
	valloc(swbuf, struct buf, nswbuf);

	/*
	 * Now the amount of virtual memory remaining for buffers
	 * can be calculated, estimating needs for the cmap.
	 */
#define MAXBUFFERPAGES 512 /* Should be in h/buf.h */
	if (bufpages == 0) {
		bufpages = (physmem * NBPG) / 10 / CLBYTES;
		if (bufpages > MAXBUFFERPAGES)
			bufpages = MAXBUFFERPAGES;
	}
	if (nbuf == 0) {
		/* Nominal 2 pages per buffer */
		nbuf = bufpages / 2;
		if (nbuf < 16)
			nbuf = 16;
	}

	/*
	 * Don't bite off more real pages than we can map!
	 */
	if (bufpages > nbuf * (MAXBSIZE / CLBYTES))
		bufpages = nbuf * (MAXBSIZE / CLBYTES);

	valloc(buf, struct buf, nbuf);

	/*
	 * Clear space allocated thus far, and make r/w entries
	 * for the space in the kernel map.
	 */
	v = (caddr_t) round_page(v);
	size = (vm_size_t) (v - firstaddr);
	ret = vm_map_find(kernel_map, vm_object_allocate(size), 
				(vm_offset_t) 0,&firstaddr, size, FALSE);
	if (ret != KERN_SUCCESS) {
		panic("startup: unable to allocate kernel data structures");
	}
	vm_map_pageable(kernel_map, firstaddr, v, FALSE);
	/* set firstaddr to the next address to be allocated */
	firstaddr = v;

	/*
	 * Now allocate buffers proper.  They are different than the above
	 * in that they usually occupy more virtual memory than physical.
	 */
	valloc(buffers, char, MAXBSIZE * nbuf);
	base = bufpages / nbuf;
	residual = bufpages % nbuf;
#if	MACH_MP
	if (show_space) {
		printf("bufpages = %d, nbuf = %d, base = %d, residual = %d\n",
				bufpages, nbuf, base, residual);
	}
#endif	MACH_MP
	/*
	 *	Allocate virtual memory for buffer pool.
	 */
	v = (caddr_t) round_page(v);
	size = (vm_size_t) (v - firstaddr);
	buffer_map = kmem_suballoc(kernel_map, &firstaddr, &trash_offset /* max */, size,TRUE);
	ret = vm_map_find(buffer_map, vm_object_allocate(size), 
				(vm_offset_t) 0, &firstaddr, size, FALSE);
	if (ret != KERN_SUCCESS) {
		panic("startup: unable to allocate buffer pool");
	}

	for (i = 0; i < nbuf; i++) {
		vm_size_t	thisbsize;
		vm_offset_t	curbuf;

		/*
		 * First <residual> buffers get (base+1) physical pages
		 * allocated for them.  The rest get (base) physical pages.
		 *
		 * The rest of each buffer occupies virtual space,
		 * but has no physical memory allocated for it.
		 */

		thisbsize = page_size*(i < residual ? base+1 : base);
		curbuf = (vm_offset_t)buffers + i * MAXBSIZE;
		vm_map_pageable(buffer_map, curbuf, curbuf+thisbsize, FALSE);
	}


	/*
	 * Initialize callouts
	 */
	callfree = callout;
	for (i = 1; i < ncallout; i++)
		callout[i-1].c_next = &callout[i];

	/*
	 * Initialize memory allocator and swap
	 * and user page table maps.
	 *
	 * THE USER PAGE TABLE MAP IS CALLED ``kernelmap''
	 * WHICH IS A VERY UNDESCRIPTIVE AND INCONSISTENT NAME.
	 */
	{
		register int	nbytes;
		extern int	vm_page_free_count;

		nbytes = ptoa(vm_page_free_count);
		printf("available memory = %d.%d%d megabytes.\n", nbytes/MEG,
			((nbytes%MEG)*10)/MEG,
			((nbytes%(MEG/10))*100)/MEG);
		nbytes = ptoa(bufpages);
		printf("using %d buffers containing %d.%d%d megabytes of memory\n",
			nbuf,
			nbytes/MEG,
			((nbytes%MEG)*10)/MEG,
			((nbytes%(MEG/10))*100)/MEG);
	}
	mb_map = kmem_suballoc(kernel_map, &mbutl, &embutl, 
			ptoa((nmbclusters)),FALSE);
	{
		int	foo, bar; /* XXX */
		extern int	rose_page_size;
		user_pt_map = kmem_suballoc(kernel_map, &foo, &bar, 
				ptoa(100), FALSE);
#if	MACH_ACC
		ipc_map = kmem_suballoc(kernel_map, &foo, &bar, 
					1024*1024, FALSE);
	/* XXX - using rose_page_size  and foo and bar here is a hack. */
#endif	MACH_ACC
	}
#else	MACH_VM
	printf("real mem  = %d\n", ctob(physmem));

	/*
	 * Determine how many buffers to allocate
	 * and how many real pages to spread around the
	 * buffers. (Each buffer occupies MAXBSIZE virtual
	 * address space and has 0 to MAXBSIZE/CLBYTES
	 * real page (cluster) frames mapped to that space
	 * at any given time, according to need.)
	 * Use 10% of memory pages, with min of 16 buffers,
	 * max of 512 pages. These limits are arbitrary.
	 * XXX [This allocation strategy needs to be looked at].
	 */
#define MAXBUFFERPAGES 512 /* Should be in h/buf.h */
	if (bufpages == 0) {
		bufpages = (physmem * NBPG) / 10 / CLBYTES;
		if (bufpages > MAXBUFFERPAGES)
			bufpages = MAXBUFFERPAGES;
	}
	if (nbuf == 0) {
		/* Nominal 2 pages per buffer */
		nbuf = bufpages / 2;
		if (nbuf < 16)
			nbuf = 16;
	}

	/*
	 * Don't bite off more real pages than we can map!
	 */
	if (bufpages > nbuf * (MAXBSIZE / CLBYTES))
		bufpages = nbuf * (MAXBSIZE / CLBYTES);

	/*
	 * Grab "bufpages" real pages from the top of memory
	 * just below the pages we grabbed for the message
	 * buffer above. Remap these frames according to
	 * an algorithm which is well known in binit() in
	 * sys/init_main.c . Each buffer is allocated at
	 * least round(bufpages/nbuf) pages, and the first
	 * "bufpages%nbuf" buffers get an extra page each.
	 * ("skippages" is how many unmapped pages to skip.)
	 * The virtual address we map the buffers at is
	 * an arbitrary one in the system segment determined
	 * in locore. Again, VAX PTEs are not necessary since
	 * the pages we grab are permanently allocated to us
	 * (though we move them around between buffers).
	 */
	maxmem -= bufpages;
	base = bufpages / nbuf;
	residual = bufpages % nbuf;
	frame = maxmem;
	vpage = btop(&bufbase);
	skippages = (MAXBSIZE / CLBYTES) - (base + 1);

	for (i = 0; i < residual; i++) {
		for (j = 0; j < (base + 1) * CLSIZE; j++) {
			mapout(&frame, 1);
			mapin(&tmp_pte, vpage++, PG_KW|frame++, 1);
		}
		vpage += skippages;
	}
	skippages++;
	for (i = residual; i < nbuf; i++) {
		for (j = 0; j < base * CLSIZE; j++) {
			mapout(&frame, 1);
			mapin(&tmp_pte, vpage++, PG_KW|frame++, 1);
		}
		vpage += skippages;
	}

	/*
	 * Make sure we didn't crash into the user page tables.
	 */
	 if ( ((int *) ctob(vpage)) >= &buflimit)
		panic("startup buffers");
	/*
	 * Make sure we didn't crash into memory hole
	 */
	 if (ishole(vpage))
		panic("startup buffers hit hole");
	
	/*
	 * We allocate 1/2 as many swap buffer headers as
	 * file i/o buffers (don't ask me why).
	 */
	if (nswbuf == 0) {
		nswbuf = (nbuf / 2) &~ 1;       /* force even */
		if (nswbuf > 256)
			nswbuf = 256;           /* sanity */
	}

	/*
	 * Allocate space for system data structures.
	 * The first available real memory page is "firstaddr".
	 * The first available kernel virtual page is "v".
	 * Note that these pages were pre-mapped by locore when
	 * the HAT/IPT table was initialized, so no remapping need
	 * be done here.
	 */
	v = (caddr_t)(SYSBASE | (firstaddr * NBPG));

#if	MACH_MP
#define	valloc(name, type, num) \
	(name) = (type *)(v); (v) = (caddr_t)((name)+(num));\
	if (0) \
		printf("name = %d(0x%x) bytes @%x, %d cells @ %d bytes\n",\
		 num*sizeof(type), num*sizeof(type), name, num, sizeof (type))
#define	valloclim(name, type, num, lim) \
	(name) = (type *)(v); (v) = (caddr_t)((lim) = ((name)+(num)));\
	if (0) \
		printf("name = %d(0x%x) bytes @%x, %d cells @ %d bytes\n",\
		 num*sizeof(type), num*sizeof(type), name, num, sizeof (type))
#define vround() (v = (caddr_t) ( ( ((int) v) + (NBPG-1)) & ~(NBPG-1)))
#define vquadround() (v = (caddr_t) ( ( ((int) v) + (8-1)) & ~(8-1)))
#else	MACH_MP
#define valloc(name, type, num) \
	  (name) = (type *)(v); (v) = (caddr_t)((name)+(num))
#define valloclim(name, type, num, lim) \
	  (name) = (type *)(v); (v) = (caddr_t)((lim) = ((name)+(num)))
#endif	MACH_MP

	if (ishole(firstaddr))
		panic("hit hole in memory");
	valloc(buf, struct buf, nbuf);
	valloc(swbuf, struct buf, nswbuf);
#if	MACH_SHM
	/*
	 * Allocate our stuff that will be user readable...
	 * This stuff really shouldn't be user readable.
	 */
	valloclim(shrmap, struct map, 3*nproc, eshrmap);
	valloc(aarea, struct aarea, nproc);

	valloc(SHM, struct SHMstate, 1);
	valloc(IPC, struct IPCstate, 1);

	if (sizeof (struct MACHstate) % 8)
		panic("MACHstate not quad\n");
	vquadround();
	valloc(mach_states, struct MACHstate, NCPUS);

#endif	MACH_SHM
#if	MACH_ACC
	vround();
	valloclim(ProcArray, struct ProcRecord, NumProcs, eProcArray);
	vround();
	valloclim(PortArray, struct PortRecord, NumPorts, ePortArray);
#endif	MACH_ACC
#if	CS_RFS
	valloc(rfsProcessTable, struct rfsProcessEntry, nproc);
#endif	CS_RFS
#if	MACH_MP
	valloc(mp_proc, struct mp_proc, nproc);
	valloc(thread, struct thread, nproc);
#endif	MACH_MP
#if	MACH_ACC
	valloc(aproc, struct ProcRecord *, nproc);
#endif	MACH_ACC

	valloclim(inode, struct inode, ninode, inodeNINODE);
	valloclim(file, struct file, nfile, fileNFILE);
	valloclim(proc, struct proc, nproc, procNPROC);
	valloclim(text, struct text, ntext, textNTEXT);
	valloc(cfree, struct cblock, nclist);
	valloc(callout, struct callout, ncallout);
	valloc(swapmap, struct map, nswapmap = nproc * 2);
	valloc(argmap, struct map, ARGMAPSIZE);
	valloc(kernelmap, struct map, nproc);
	valloc(mbmap, struct map, nmbclusters/4);
	valloc(nch, struct nch, nchsize);
#ifdef QUOTA
	valloclim(quota, struct quota, nquota, quotaNQUOTA);
	valloclim(dquot, struct dquot, ndquot, dquotNDQUOT);
#endif
	/*
	 * Now allocate space for core map.
	 * Allow space for all of physical memory minus the amount
	 * dedicated to the system. The amount of physical memory
	 * dedicated to the system is everything below the real
	 * page corresponding to "v" at this point minus everything
	 * above "maxmem", i.e. the pages previously allocated for
	 * the HAT/IPT table, the buffers, and the console message
	 * buffer. Some trickiness here because we also want to put
	 * the core map in system memory!
	 */
	ncmap = (maxmem * NBPG - ((int)v &~ SYSBASE)) /
			(NBPG*CLSIZE + sizeof(struct cmap));
	ncmap += 2; /* SLOP -- Seems to be necessary, can't hurt */
	valloclim(cmap, struct cmap, ncmap, ecmap);
	unixsize = btoc((int)(ecmap+1) &~ SYSBASE);
	if (unixsize >= maxmem - 8*UPAGES)
		panic("no memory");

	if (ishole(unixsize))
		panic("valloc: hit hole in memory");

	/*
	 * Initialize memory allocator and swap
	 * and user page table maps.
	 *
	 * THE USER PAGE TABLE MAP IS CALLED "kernelmap"
	 * WHICH IS A VERY UNDESCRIPTIVE AND INCONSISTENT NAME.
	 *
	 * [I love this comment which is the only reason I
	 *  don't rename the stupid variable]
	 */
	meminit(unixsize, maxmem);
	rminit(kernelmap, (long)USRPTSIZE, (long)1, "usrpt", nproc);
	rminit(mbmap, (long)((nmbclusters - 1) * CLSIZE), (long)CLSIZE,
				"mbclusters", nmbclusters/4);
#endif	MACH_VM
#if	MACH_SHM
		/*
		 * shareable pages should figure into the
		 * shmap size calculation, someday
		 */
#if	MACH_VM
	shareable_pages = btop(mem_size) >> 3;
#else	MACH_VM
	shareable_pages = (physmem - firstaddr) >> 3;
#endif	MACH_VM
	if (shareable_pages + TOTAL_KMSG_PAGES + 1 > SHMAPGS) {
		printf("you need to make shmem.h: SHMAPGS bigger\n");
		shareable_pages = SHMAPGS - 1 - TOTAL_KMSG_PAGES;
	}
#if	MACH_VM
	sh_map = kmem_suballoc(kernel_map, &shutl, &eshutl, ptob(shareable_pages),FALSE);
#else	MACH_VM
	rminit(shrmap, shareable_pages + TOTAL_KMSG_PAGES, 1, "shrmap", eshrmap-shrmap);
#endif	MACH_VM
	SHM->physmem = physmem;
	SHM->maxmem = maxmem;
	SHM->shareable_pages = shareable_pages;
	/*
	 * Set up CPU-specific registers, cache, etc.
	 */
/*	initcpu();*/

	unxshminit();
#endif	MACH_SHM

#if	MACH_VM
	mpq_allochead_init();
#endif	MACH_VM
#if	MACH_ACC
	unxaccentinit();
#endif	MACH_ACC


#if	MACH_VM
#else	MACH_VM
	/*
	 * Print some memory statistics.
	 */
	printf("Using %d buffers containing %dK bytes\n",
				nbuf, ctob(bufpages)/1024);
	printf("available %dK (0x%x)\n",
				ctob(freemem)/1024, ctob(freemem));
	
	/*
	 * From now on, maxmem is the max FREE mem
	 */
	maxmem = freemem;


	/*
	 * Initialize callouts
	 */
	callfree = callout;
	for (i = 1; i < ncallout; i++)
		callout[i-1].c_next = &callout[i];
#endif	MACH_VM


	/*
	 * Configure the system.
	 */
	configure();

       /* enable interrupts */
       spl0();
}

#ifdef PGINPROF
/*
 * Return the difference (in microseconds)
 * between the  current time and a previous
 * time as represented  by the arguments.
 * If there is a pending clock interrupt
 * which has not been serviced due to high
 * ipl, return error code.
 */
vmtime(otime, olbolt, oicr)
register int otime, olbolt, oicr;
{
	printf("vmtime called!\n");
	/*     if (mfpr(ICCS)&ICCS_INT)
		       return(-1);
	       else
		       return(((time.tv_sec-otime)*60 + lbolt-olbolt)*16667 + mfpr(ICR)-oicr);
	*/
}
#endif

/*
 * Send an interrupt to process.
 *
 * Stack is set up to allow sigcode stored
 * in u. to call routine, followed by chmk
 * to sigcleanup routine below.  After sigcleanup
 * resets the signal mask and the stack, it
 * returns to user who then unwinds with the
 * rei at the bottom of sigcode.
 */
struct sigframe {  /* the following is pushed on the stack during a signal */
	int sf_signum;               /* space for first parameter */
	int sf_code;                 /* space for second parameter */
	struct sigcontext *sf_scp;   /* space for third parameter */
	struct sigcontext sf_sc;     /* actual sigcontext */
	int sf_regs[MQ-ICSCS+1];     /* space to save registers */
};

sendsig(p, sig, sigmsk)
int (*p)(), sig, sigmsk;
{
	register int *regs,*rp1,*rp2;
	register struct sigframe *fp;
	int oonstack;

	DEBUGF(sydebug,
		printf("sendsig, p=%x,sig=%x,sigmsk=%x\n",p,sig,sigmsk));
	regs = u.u_ar0;
	oonstack = u.u_onstack;

#define mask(s) (1<<((s)-1))
	/*
	 * Push a stack frame for this signal
	 */
	if (!oonstack && (u.u_sigonstack & mask(sig))) {
		/* wasn't on onstack, and wants to be */
		fp = (struct sigframe *)u.u_sigsp - 1;
		u.u_onstack = 1;
	}
	else
		/* was on onstack, or doesn't want to be */
#ifdef DUALCALL
		fp = (struct sigframe *)
			(regs[SP] + (u.u_calltype?FRMPROTECT:0)) - 1;
#else
		fp = (struct sigframe *)regs[SP] - 1;
#endif DUALCALL

	/*
	 * Make sure the stack will hold the new context
	 */
	if (!u.u_onstack && (unsigned)fp < USRSTACK - ctob(u.u_ssize))
#ifdef DUALCALL
		grow((unsigned)fp+(u.u_calltype ? FRMPROTECT : 0));
#else
		grow((unsigned)fp);
#endif DUALCALL

	/*
	 * Make sure we can access the new signal frame
	 */
	if (!useracc((caddr_t)fp,sizeof(struct sigframe),1)) {
		/* can't get there! */
		/* process has trashed its stack; give it an illegal */
		/* instruction to halt it in its tracks */
		u.u_signal[SIGILL] = SIG_DFL;
		sig = mask(SIGILL);
		u.u_procp->p_sigignore &= ~sig;
		u.u_procp->p_sigcatch &= ~sig;
		u.u_procp->p_sigmask &= ~sig;
		psignal(u.u_procp, SIGKILL);
		return;
	}

	/*
	 * Set up the sigcontext to be passed to p()
	 */
	fp->sf_sc.sc_onstack = oonstack;
	fp->sf_sc.sc_mask = sigmsk;
	fp->sf_sc.sc_sp = regs[SP];
	fp->sf_sc.sc_iar = regs[IAR];
	fp->sf_sc.sc_icscs = regs[ICSCS];

	/*
	 * Save the current registers
	 */
	rp1 = &regs[MQ];
	rp2 = &(fp->sf_regs[MQ-ICSCS]);
	while (rp1 >= regs)
		*rp2-- = *rp1--;

	/*
	 * Set up registers to call p()
	 */
	regs[R2] = fp->sf_signum = sig; /* 1st parameter: sf_signum */
	if (sig == SIGILL || sig == SIGFPE) {
		regs[R3] = fp->sf_code = u.u_code; /* 2nd parameter: sf_code */
		u.u_code = 0;
	}
	else
		regs[R3] = fp->sf_code = 0; /* 2nd parameter: sf_code */
	regs[R4] = (int)(fp->sf_scp = &(fp->sf_sc));/* 3rd parameter: sf_scp */
	regs[R15] = (int)u.u_pcb.pcb_sigc;  /* return addr:  svc 139 */
#ifdef DUALCALL
	if (u.u_calltype) {
	    regs[R0] = (int)p;
	    regs[IAR] = fuword((caddr_t)p);
	} else
	    regs[IAR] = (int)p;                 /* addr of signal handler */
#else
	regs[IAR] = (int)p;                 /* addr of signal handler */
#endif DUALCALL
	regs[SP] = (int)fp;                 /* push signal context */

	/*
	 * "Return" to the handler
	 */
	return;
}

/*
 * Routine to cleanup state after a signal
 * has been taken.  Reset signal mask and
 * stack state from context left by sendsig (above).
 * Pop these values in preparation for rei which
 * follows return from this routine.
 */
sigcleanup()
{
	register struct sigframe *fp;
	register int *regs, *rp1, *rp2;

	DEBUGF(sydebug, printf("sigcleanup\n"));
	regs = u.u_ar0;
	fp = (struct sigframe *)regs[SP];
	if (!useracc((caddr_t)fp,sizeof(struct sigframe),1))
		return;

	/*
	 * Restore registers
	 */
	rp1 = &regs[MQ];
	rp2 = &(fp->sf_regs[MQ-ICSCS]);
	while (rp1 >= regs)
		*rp1-- = *rp2--;

	/*
	 * Alter registers available to signal handler
	 */
	regs[IAR] = fp->sf_sc.sc_iar;
	regs[SP]  = fp->sf_sc.sc_sp;
	regs[ICSCS] = (fp->sf_sc.sc_icscs) | ICSCS_USERSET & ~ICSCS_USERCLR;

	/*
	 * Restore signal values
	 */
	u.u_onstack = fp->sf_sc.sc_onstack & 01;
	u.u_procp->p_sigmask = fp->sf_sc.sc_mask & ~(mask(SIGKILL) |
	    mask(SIGCONT) | mask(SIGSTOP));
}

#ifdef notdef
dorti()
{
       struct frame frame;
       register int sp;
       register int reg, mask;
       extern int ipcreg[];

       (void) copyin((caddr_t)u.u_ar0[FP], (caddr_t)&frame, sizeof (frame));
       sp = u.u_ar0[FP] + sizeof (frame);
       u.u_ar0[PC] = frame.fr_savpc;
       u.u_ar0[FP] = frame.fr_savfp;
       u.u_ar0[AP] = frame.fr_savap;
       mask = frame.fr_mask;
       for (reg = 0; reg <= 11; reg++) {
	       if (mask&1) {
		       u.u_ar0[ipcreg[reg]] = fuword((caddr_t)sp);
		       sp += 4;
	       }
	       mask >>= 1;
       }
       sp += frame.fr_spa;
       u.u_ar0[PS] = (u.u_ar0[PS] & 0xffff0000) | frame.fr_psw;
       if (frame.fr_s)
	       sp += 4 + 4 * (fuword((caddr_t)sp) & 0xff);
       /* phew, now the rei */
       u.u_ar0[PC] = fuword((caddr_t)sp);
       sp += 4;
       u.u_ar0[PS] = fuword((caddr_t)sp);
       sp += 4;
       u.u_ar0[PS] |= PSL_USERSET;
       u.u_ar0[PS] &= ~PSL_USERCLR;
       u.u_ar0[SP] = (int)sp;
}
#endif

/*
 * Invalidate single all pte's in a cluster
 */
#if	ROMP_DEBUG
tbiscl(v)
unsigned v;
{
	printf("tbiscl(v=%x)\n",v);
}
#endif	ROMP_DEBUG

/*
 * Reboot the system, with various options
 */
int    waittime = -1;

boot(paniced, arghowto)
int paniced, arghowto;
{
       register int howto;             /* r11 == how to boot */
       register int devtype;           /* r10 == major of root dev */
       register int x;

       DEBUGF(svdebug, printf("paniced=%x,arghowto=%x\n", paniced, arghowto));
       x = * (int *) CSR;		/* clear csr lock */
       * (int *) CSR = 0;		/* and reset it */
       (void) spl1();                  /* allow level 5 disk intr DOB/STB */
       howto = arghowto;
       if ((howto&RB_NOSYNC)==0 && waittime < 0 && bfreelist[0].b_forw) {
	       waittime = 0;
	       update();
	       printf("syncing disks... ");
	       { register struct buf *bp;
		 int iter, nbusy = 0, lbusy = 0;

		 for (iter = 0; iter < 20; iter++) {
		       nbusy = 0;
		       for (bp = &buf[nbuf]; --bp >= buf; )
			       if ((bp->b_flags & (B_BUSY|B_INVAL)) == B_BUSY)
				       nbusy++;
		       if (nbusy < lbusy)
			       iter = 0;
		       if ((lbusy = nbusy) == 0)
			       break;
		       printf("%d ", nbusy);
		       for (nbusy=100000; nbusy; --nbusy)
		       		;	/* a bit of a delay */
		 }
	       }
	       printf("done\n");
       }
       (void) spl7();                         /* extreme priority */
       devtype = major(rootdev);
       if (howto&RB_HALT) {
	       printf("halting (via wait);\n");
	       DISPLAY(0xff);		/* turn out the light */
	       for (;;)
		       asm(" wait ");	/* closest thing to a halt */
       } else {
	       if (paniced == RB_PANIC) {
		       dumpsys();              /* dump to swap area */
	       }
	       for (;;)
			_reboot(howto,devtype);
	}
}


int    dumpmag = 0x8fca0101;   /* magic number for savecore */
int    dumpsize = 0;           /* also for savecore */
/*
 * Doadump comes here after turning off memory management and
 * getting on the dump stack, when called by the system attention code
 * or more directly from above during 'boot' when called from 'panic'. 
 */
dumpsys()
{

       dumpsave();		/* save important MM stuff */
       dumpsize = endmem;	/* dump includes the hole */
       printf("\ndumping to dev %x, offset %d\n", dumpdev, dumplo);
       printf("dump ");
       switch ((*bdevsw[major(dumpdev)].d_dump)(dumpdev)) {

	case ENODEV:
		printf("driver can't dump yet\n");
		break;

       case ENXIO:
               printf("device bad\n");
               break;

       case EFAULT:
               printf("device not ready\n");
               break;

       case EINVAL:
               printf("area improper\n");
               break;

       case EIO:
               printf("i/o error");
               break;

       default:
               printf("succeeded");
               break;
       }
}

/*
 * I'm not sure what is machine dependent about this one XXX
 */
physstrat(bp, strat, prio)
struct buf *bp;
int (*strat)(), prio;
{
       int s;

       (*strat)(bp);
       /* pageout daemon doesn't wait for pushed pages */
       if (bp->b_flags & B_DIRTY)
	       return;
       s = spl6();
       while ((bp->b_flags & B_DONE) == 0)
	       sleep((caddr_t)bp, prio);
       splx(s);
}



/*
 * This is where the "soft network" interrupt comes to. The
 * hardware interrupt routine has pulled the packet off the
 * interface and now wants us to run (at lower priority) the
 * protocol family specific routine to munch the packet.
 */
softnet()
{
#ifdef INET
#define LOCORE /* *sigh* */
#include "../net/netisr.h"
#undef LOCORE /* unsigh */
	extern int netisr;
	if (netisr & (1<<NETISR_RAW)) {
		netisr &= ~(1<<NETISR_RAW);
		rawintr();
		return(0);
	}
	if (netisr & (1<<NETISR_IP)) {
		netisr &= ~(1<<NETISR_IP);
		ipintr();
		return(0);
	}
	netisr = 0;
#endif INET
	return (1);
}

/*
 * Debugger -- go enter the debugger, if we have one
 * We return when done debugging (or if there is no debugger)
 */

Debugger(s)
char *s;
{
	/*      extern char etext, end; /* */

	/*
	 * Check validity of s before we try to use it
	 */
	printf("Debugger called from %s.\n",
		( ((u_int) s > (u_int) &etext) &&
		  ((u_int) s < (u_int) &end) && *s)? s : "someplace");
#ifdef RDB
	printf("Type \"go iar+2\" to continue execution.\n");
	asm(" tgte r1,r1 ");
#else
	printf("(But we weren't compiled with RDB!  Sorry.)\n");
#endif RDB
}

/*
 * Clear registers on exec
 */
int
setregs(entry)
	u_long entry;	
{
	register int i, j;
	register struct proc *p = u.u_procp;

	/*
	 * Reset caught signals.  Held signals
	 * remain held through p_sigmask.
	 */
	while (p->p_sigcatch) {
		(void) spl6();
		i = ffs(p->p_sigcatch);
		p->p_sigcatch &= ~(1 << (i - 1));
		u.u_signal[i] = SIG_DFL;
		(void) spl0();
	}
#ifdef notdef
	/* should pass args to init on the stack */
	for (rp = &u.u_ar0[0]; rp < &u.u_ar0[16];)
		*rp++ = 0;
#endif
	i = u.u_ar0[IAR] = entry;
#ifdef DUALCALL
	/* Old calling sequence crt0 begins with

		start:	j   next
			.ascii "<start>"
	   and expects kframe (struct argument) to be on the stack.

	   New calling sequence crt0 begins with

		start:  j   .start
			.short 1	# version number, eventually
			.long  magicno  # magic number -- to be determined
			.long  _start   # to let execve load data ptr in r0
			.globl __fpfpr
			.long  __fpfpr  # location of software fp status
			.ascii "<start>"
			.start:
	   and expects the first four words of kframe to be in r2-r5,
	   which it will store on the stack. 
	*/
	j = fuiword((caddr_t) i) & 0xffff;	/* .short 1  or "<s" */
	if (j < 256) { 	/* Must be NCS */
	    u.u_calltype = j;
	    u.u_r.r_val1 = fuiword((caddr_t) i+NBPW*2);/* val1->R0 in trap.c */
#ifdef notdef
	    u.something = fuiword((caddr_t) i+NBPW*3) /* record fp status addr*/
#endif notdef
		/* Copy first 4 words of kframe struct into user's r2-r5, 
		   and bump sp past them.
		 */
	    for (i=R2; i<R6; i++, u.u_ar0[SP] += NBPW)
		u.u_ar0[i] = fuword((caddr_t) u.u_ar0[SP]); 
	    u.u_r.r_val2 = u.u_ar0[R2];		/* val2 -> R2 in trap.c */
	}
	else u.u_calltype = 0;			/* OCS */ 
#endif DUALCALL
	for (i=0; i<NOFILE; i++) {
		if (u.u_pofile[i]&UF_EXCLOSE) {
			closef(u.u_ofile[i]);
			u.u_ofile[i] = NULL;
			u.u_pofile[i] = 0;
		}
		u.u_pofile[i] &= ~UF_MAPPED;
	}

#ifdef notdef /* XXX - BJB */
	/*
	 * Remember file name for accounting.
	 */
	u.u_acflag &= ~AFORK;
	bcopy((caddr_t)u.u_dent.d_name, (caddr_t)u.u_comm,
	    (unsigned)(u.u_dent.d_namlen + 1));
#endif notdef
	/*
	 * restore ccr to default state 
	 * by clearing all bits not on in 
	 * default case.
	 */
	set_ccr(0x80000000 | (int) ccr_default);
	/* 
	 * restore consdev by clearings bits for
	 * all possible console devices
	 */
	set_consdev(0x80000000 | (int) 0);
#ifdef FPA
	/*
	 * upon exec release the current register set (as is done
	 * upon exit).
	 */
	fpa_exit(u.u_procp);		/* release register set */
#endif
}

/*
 * set/clear bits in the consdev (console device flags) for this process
 * if top bit in value is on then we clear the given
 * bits otherwise set them.
 */
set_consdev(value) register int value;
{

if (value < 0)
	u.u_pcb.pcb_consdev &= value;
else
	u.u_pcb.pcb_consdev |= value;
}

/* 
 * return the value in the console device flags for this process.
 */
get_consdev()
{
	return(u.u_pcb.pcb_consdev);
}


#ifdef UPANIC
/*
 * panic initiated from user code (root only)
 * for testing dump taking.
 */
upanic()
{
	register struct a {
		char *	opt;
	};

	if (suser())
		panic(((struct a *)u.u_ap)->opt);
}
#endif

/*
 * set/clear bits in the ccr for this process
 * if top bit in value is on then we clear the given
 * bits otherwise set them.
 */
set_ccr(value) register int value;
{

if (value < 0)
	u.u_pcb.pcb_ccr &= value;
else
	u.u_pcb.pcb_ccr |= value;
* (char *) CCR = u.u_pcb.pcb_ccr;
}

#define DELAY_COUNT 1000

int delay_count = DELAY_COUNT;

/* 
 * give 'n' milliseconds of real-time delay.
 */
delay(n)
	register int n;
{
	register int i;

	while (--n >= 0)
		for (i = delay_count; --i >= 0;)
			* (char *) DELAY_ADDR = 0xFF;
}

int bbssi(bit,address)
int	bit;
caddr_t	address;

{
 setbit((char *)address + 3,bit);
 return(0);
}

int bbcci(bit,address)
int	bit;
caddr_t	address;

{
 clrbit((char *)address + 3,bit);
 return(0);
}

cpu_number()

{ 
 return(0);
}

set_cpu_number()
{}

sigreturn()
{
 panic("sigreturn not implemented");
}
