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
 *	File:	h/machine.h
 *	Author:	Avadis Tevanian, Jr.
 *
 *	Copyright (C) 1986, Avadis Tevanian, Jr.
 *
 *	Machine independent machine abstraction.
 *
 ************************************************************************
 * HISTORY
 * 16-May-86  Avadis Tevanian (avie) at Carnegie-Mellon University
 *	Created.
 *
 ************************************************************************
 */

#ifdef	KERNEL
#include "cpus.h"
#include "../vm/vm_param.h"
#else	KERNEL
#include <sys/features.h>
#include <vm/vm_param.h>
#endif	KERNEL

/*
 *	For each host, there is a maximum possible number of
 *	cpus that may be available in the system.  This is the
 *	compile-time constant NCPUS, which is defined in cpus.h.
 *
 *	In addition, there is a machine_slot specifier for each
 *	possible cpu in the system.
 */

struct machine_info {
	int		max_cpus;	/* max number of cpus compiled */
	int		avail_cpus;	/* number actually available */
	vm_size_t	memory_size;	/* size of memory in bytes */
};

typedef struct machine_info	*machine_info_t;

typedef int	cpu_type_t;
typedef int	cpu_subtype_t;

struct machine_slot {
	boolean_t	is_cpu;		/* is there a cpu in this slot? */
	cpu_type_t	cpu_type;	/* type of cpu */
	cpu_subtype_t	cpu_subtype;	/* subtype of cpu */
	boolean_t	running;	/* is cpu running */
};

typedef struct machine_slot	*machine_slot_t;

#ifdef	KERNEL
struct machine_info		machine_info;
struct machine_slot		machine_slot[NCPUS];

long		tick_count[NCPUS];
boolean_t	should_exit[NCPUS];
vm_offset_t	interrupt_stack[NCPUS];
#endif	KERNEL

/*
 *	Machine types known by all.
 */

#define	CPU_TYPE_VAX		((cpu_type_t) 1)
#define	CPU_TYPE_ROMP		((cpu_type_t) 2)
#define	CPU_TYPE_MC68020	((cpu_type_t) 3)
#define CPU_TYPE_NS32032	((cpu_type_t) 4)

/*
 *	Machine subtypes (these are defined here, instead of in a machine
 *	dependent directory, so that any program can get all definitions
 *	regardless of where is it compiled).
 */

/*
 *	VAX subtypes (these do *not* necessary conform to the actual cpu
 *	ID assigned by DEC available via the SID register).
 */

#define CPU_SUBTYPE_VAX780	((cpu_subtype_t) 1)
#define CPU_SUBTYPE_VAX785	((cpu_subtype_t) 2)
#define CPU_SUBTYPE_VAX750	((cpu_subtype_t) 3)
#define CPU_SUBTYPE_VAX730	((cpu_subtype_t) 4)
#define CPU_SUBTYPE_UVAXI	((cpu_subtype_t) 5)
#define CPU_SUBTYPE_UVAXII	((cpu_subtype_t) 6)
#define CPU_SUBTYPE_VAX8200	((cpu_subtype_t) 7)
#define CPU_SUBTYPE_VAX8500	((cpu_subtype_t) 8)
#define CPU_SUBTYPE_VAX8600	((cpu_subtype_t) 9)
#define CPU_SUBTYPE_VAX8650	((cpu_subtype_t) 10)
#define CPU_SUBTYPE_VAX8800	((cpu_subtype_t) 11)

/*
 *	ROMP subtypes.
 */

#define CPU_SUBTYPE_RT_PC	((cpu_subtype_t) 1)

/*
 *	68020 subtypes.
 */

#define CPU_SUBTYPE_SUN3	((cpu_subtype_t) 1)

/*
 *	32032 subtypes.
 */

#define CPU_SUBTYPE_MMAX	((cpu_subtype_t) 1)
