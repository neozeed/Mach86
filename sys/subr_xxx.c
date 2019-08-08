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
 *	@(#)subr_xxx.c	6.6 (Berkeley) 6/8/85
 */
#if	CMU
/*
 ************************************************************************
 * HISTORY
 * 25-Feb-86  Avadis Tevanian (avie) at Carnegie-Mellon University
 *	New calloc routine using VM routines.
 *
 * 25-Jan-86  Avadis Tevanian (avie) at Carnegie-Mellon University
 *	Upgraded to 4.3.
 *
 * 16-Nov-85  Avadis Tevanian (avie) at Carnegie-Mellon University
 *	CS_BUGFIX:  Fixed off by one error in ffs.
 *
 ************************************************************************
 */

#include "cs_bugfix.h"
#include "mach_vm.h"
#endif	CMU

#if	MACH_VM
#include "../vm/vm_map.h"
#include "../vm/vm_kern.h"
#endif	MACH_VM

#include "../machine/pte.h"

#include "param.h"
#include "systm.h"
#include "conf.h"
#include "inode.h"
#include "dir.h"
#include "user.h"
#include "buf.h"
#include "proc.h"
#include "fs.h"
#include "vm.h"
#include "cmap.h"
#include "uio.h"

/*
 * Routine placed in illegal entries in the bdevsw and cdevsw tables.
 */
nodev()
{

	return (ENODEV);
}

/*
 * Null routine; placed in insignificant entries
 * in the bdevsw and cdevsw tables.
 */
nulldev()
{

	return (0);
}

#ifndef vax
imin(a, b)
{

	return (a < b ? a : b);
}

imax(a, b)
{

	return (a > b ? a : b);
}

unsigned
min(a, b)
	u_int a, b;
{

	return (a < b ? a : b);
}

unsigned
max(a, b)
	u_int a, b;
{

	return (a > b ? a : b);
}
#endif not vax

#if	MACH_VM
caddr_t calloc(size)
	int size;
{
	return((caddr_t)kmem_alloc(kernel_map, size, TRUE));
}
#else	MACH_VM
extern	cabase, calimit;
extern	struct pte camap[];

caddr_t	cacur = (caddr_t)&cabase;
caddr_t	camax = (caddr_t)&cabase;
int	cax = 0;
/*
 * This is a kernel-mode storage allocator.
 * It is very primitive, currently, in that
 * there is no way to give space back.
 * It serves, for the time being, the needs of
 * auto-configuration code and the like which
 * need to allocate some stuff at boot time.
 */
caddr_t
calloc(size)
	int size;
{
	register caddr_t res;
	register int i;

	if (cacur+size >= (caddr_t)&calimit)
		panic("calloc");
	while (cacur+size > camax) {
		(void) vmemall(&camap[cax], CLSIZE, &proc[0], CSYS);
		vmaccess(&camap[cax], camax, CLSIZE);
		for (i = 0; i < CLSIZE; i++)
			clearseg(camap[cax++].pg_pfnum);
		camax += NBPG * CLSIZE;
	}
	res = cacur;
	cacur += size;
	return (res);
}
#endif	MACH_VM

/*
 * Stub routine in case it is ever possible to free space.
 */
cfreemem(cp, size)
	caddr_t cp;
	int size;
{
	printf("freeing %x, size %d\n", cp, size);
}

#ifndef vax
ffs(mask)
	register long mask;
{
	register int i;

#if	CS_BUGFIX
	for(i = 1; i <= NSIG; i++) {
#else	CS_BUGFIX
	for(i = 1; i < NSIG; i++) {
#endif	CS_BUGFIX
		if (mask & 1)
			return (i);
		mask >>= 1;
	}
	return (0);
}

bcmp(s1, s2, len)
	register char *s1, *s2;
	register int len;
{

	while (len--)
		if (*s1++ != *s2++)
			return (1);
	return (0);
}

strlen(s1)
	register char *s1;
{
	register int len;

	for (len = 0; *s1++ != '\0'; len++)
		/* void */;
	return (len);
}
#endif
