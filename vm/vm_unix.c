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
 *
 * HISTORY
 * 21-May-86  David Golub (dbg) at Carnegie-Mellon University
 *	Removed xumount.
 *
 ****************************************************************
 *
 */
#include "mach_tt.h"

#include "../h/task.h"
#include "../h/thread.h"
#include "../vm/vm_param.h"
#include "../vm/vm_map.h"

#include "../machine/pte.h"

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/cmap.h"
#include "../h/dir.h"
#include "../h/user.h"
#include "../h/proc.h"
#include "../h/text.h"
#include "../h/vm.h"
#include "../h/file.h"
#include "../h/inode.h"
#include "../h/buf.h"
#include "../h/mount.h"
#include "../h/trace.h"
#include "../h/map.h"
#include "../h/kernel.h"
#include "../h/seg.h"

#include "../vm/vm_kern.h"

/*
 *	Why I'm here:
 *		caller		routine
 *		------------------------
 *		physio		useracc, vslock, vsunlock
 *		execve		suword
 *		mmrw (/dev/mem)	kernacc
 *		?		swpexpand
 */

useracc(addr, len, prot)
	caddr_t addr;
	u_long len;
	int prot;
{
	return (vm_map_check_protection(
			current_task()->map,
			trunc_page(addr), round_page(addr+len-1),
			prot == B_READ ? VM_PROT_READ : VM_PROT_WRITE));
}

vslock(addr, len)
	caddr_t addr;
	u_long len;
{
	vm_map_pageable(current_task()->map, trunc_page(addr), round_page(addr+len-1), FALSE);
}

vsunlock(addr, len, dirtied)
	caddr_t addr;
	u_long len;
	int dirtied;
{
	vm_map_pageable(current_task()->map, trunc_page(addr), round_page(addr+len-1), TRUE);
}

subyte(addr, byte)
	caddr_t addr;
	char byte;
{
	return (copyout((caddr_t) &byte, addr, sizeof(char)) == 0 ? 0 : -1);
}

suibyte(addr, byte)
	caddr_t addr;
	char byte;
{
	return (copyout((caddr_t) &byte, addr, sizeof(char)) == 0 ? 0 : -1);
}

char fubyte(addr)
	caddr_t addr;
{
	char byte;

	if (copyin(addr, (caddr_t) &byte, sizeof(char)))
		return(-1);
	return((unsigned) byte);
}

suword(addr, word)
	caddr_t addr;
	int word;
{
	return (copyout((caddr_t) &word, addr, sizeof(int)) == 0 ? 0 : -1);
}

int fuword(addr)
	caddr_t addr;
{
	int word;

	if (copyin(addr, (caddr_t) &word, sizeof(int)))
		return(-1);
	return(word);
}

/* suiword and fuiword are the same as suword and fuword, respectively */

suiword(addr, word)
	caddr_t addr;
	int word;
{
	return (copyout((caddr_t) &word, addr, sizeof(int)) == 0 ? 0 : -1);
}

int fuiword(addr)
	caddr_t addr;
{
	int word;

	if (copyin(addr, (caddr_t) &word, sizeof(int)))
		return(-1);
	return(word);
}

swapon()
{
}

swpexpand(ds, ss, dmp, smp)
/*	size_t ds, ss;
	register struct dmap *dmp, *smp; */
{
	return(TRUE);
}

swfree(index)
	int index;
{
}

swstrategy(bp)
{
    	panic("swstrategy: called");
}
swread(bp)
{
	printf("swread: called");
}
swwrite(bp)
{
	printf("swwrite: called");
}

vmmeter()
{
}

/*
 */

procdup(child, parent)
	struct proc *child, *parent;
{
	struct user	*addr;
	vm_map_t	new_map;
	int		is_child;		/* This MUST NOT be in a register */
	port_t		dummy_port;

	is_child = TRUE;
	(void) task_create(task_table[parent-proc], TRUE, &task_table[child-proc], &dummy_port);
	is_child = FALSE;			/* child retains previous value on stack */
	new_map = task_table[child-proc]->map;
	(void) thread_create(task_table[child-proc], &thread_table[child-proc], &dummy_port);

	/* XXX Cheat to get proc pointer into task structure */
	task_table[child-proc]->proc = child;

#if	MACH_TT
	bcopy(task_table[child-proc]->u_address, task_table[parent-proc]->u_address,
		sizeof(struct user));
	bcopy(thread_table[child-proc]->u_address.uthread,
		thread_table[parent-proc]->u_address.uthread,
		sizeof(struct user));
#else	MACH_TT
	/*
	 *	Wire down the child's u area (vm_map_fork does not
	 *	automatically wire down pages in the child).
	 */

#ifdef	romp
	if (vm_map_pageable(new_map, (vm_offset_t) UAREA, 
				((vm_offset_t) UAREA) + ptob(UPAGES),
				FALSE) != KERN_SUCCESS)
		panic("couldn't wire down u");
#else	romp
	if (vm_map_pageable(new_map, (vm_offset_t) &u, 
				((vm_offset_t) &u) + ptob(UPAGES),
				FALSE) != KERN_SUCCESS)
		panic("couldn't wire down u");

	pmap_redzone(vm_map_pmap(new_map), round_page((&u) + 1));
#endif	romp
#endif	MACH_TT

	/*
	 *	Save a correct PCB in the parent's pcb,
	 */
	if (save_context()) {
		if (is_child) {
			/*
			 * Identify child, and clear statistics.
			 */
			u.u_procp = child;
			bzero((caddr_t)&u.u_ru, sizeof (struct rusage));
			bzero((caddr_t)&u.u_cru, sizeof (struct rusage));
			u.u_outime = 0;
			return(1);
		}
		return(0);
	}

	/*
	 *	... and then copy it to the child's pcb.
	 */

	*thread_table[child-proc]->pcb = *thread_table[parent-proc]->pcb;

	load_context(thread_table[parent-proc]);
	/*NOTREACHED*/
}

chgprot(addr, prot)
	vm_offset_t	addr;
	int		prot;
{
	return(vm_map_protect(current_task()->map,
				trunc_page(addr),
				round_page(addr + 1),
				prot == RO ? VM_PROT_READ :
				VM_PROT_READ|VM_PROT_WRITE, FALSE) == KERN_SUCCESS);
}

task_t current_task()
{
	return(current_thread()->task);
}
