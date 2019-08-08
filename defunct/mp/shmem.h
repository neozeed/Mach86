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
#ifdef	KERNEL
#include "mach_vm.h"
#else	KERNEL
#include <sys/features.h>
#endif	KERNEL

/*
 **************************************************************************
 * HISTORY
 * 11-Oct-85  Robert V Baron (rvb) at Carnegie-Mellon University
 *	Shared memory header to deescribe shared memory routines and
 *	constants.  using_shm() used to be in vaxmpm/...
 **************************************************************************
 */
#ifdef	romp
#define SHMUNIT 2048
#else	romp
#define SHMUNIT 1024
#endif	romp
#define SHMAPGS 4096
#define TOTAL_KMSG_PAGES 250
#define KMSG_HEADER_PAGES 50
#define rndshmbndry(size) ( ((size)+(SHMUNIT-1)) & (~(SHMUNIT-1)) )
#define btoshmunit(size) ((size) >> 10 )

#define using_shm(addr) (((char *)addr) >= shutl && ((char *)addr) < eshutl)

#ifndef LOCORE
struct aarea {
	int a_sh_limit;		/* per process shared memory limit */
	int a_sh_size;		/* current shared memory allocation */
	struct map a_sh_map[16];/* place holder for allocate shared
				   memory */
};

struct SHMstate {
	int physmem;
	int maxmem;
	int shareable_pages;
	int slop;
	int bytes;
};

#ifdef KERNEL
extern struct map *shrmap, *eshrmap;
extern int shareable_pages;
#if	MACH_VM
char *shutl, *eshutl;
#else	MACH_VM
extern struct pte shmap[];
extern char shutl[], eshutl[];
#endif	MACH_VM
extern struct aarea *aarea;
extern struct SHMstate *SHM;
#endif
#endif
