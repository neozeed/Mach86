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
 * 5799-CGZ (C) COPYRIGHT IBM CORPORATION  1986
 * LICENSED MATERIALS - PROPERTY OF IBM
 * REFER TO COPYRIGHT INSTRUCTIONS FORM NUMBER G120-2083
 */
/* $Header: fpa.h,v 5.1 86/03/24 18:27:20 webb Exp $ */
/* $Source: /ibm/acis/usr/sys_ca/ca/RCS/fpa.h,v $ */
#ifdef FPA

/*
 * this file contains those defines and declarations required 
 * for the floating point accelerator 
 */

/* 
 * fpasave is the save area for the floating point registers
 * used when it is necessary to take back a register set from
 * a process. these are kept in kernel memory so that it is
 * easy to do so. The space is allocated only when the FPA is 
 * present.
 * the current register set is given in the proc structure.
 *
 */

#define MAX_FPA_REGS	16	/* number of registers in register set */
#define MAX_FPA_SETS	32	/* number of fpa register sets */
#define FPA_RESERVED	0	/* how many are reserved (for kernel) */

/* following bits are in p_fpareg */
#define FPA_NONE	0xff	/* no FPA present */
#define FPA_LOCK	0x80	/* bit on if FPA is to be locked */
#define FPA_UNUSED	0xfe	/* fpa not used yet */
#define FPA_SAVED	0xfd	/* fpa saved - must be restored */

#ifndef LOCORE
/* saved fpa register sets are indexed by p-proc. */

struct fpasave 
{
	union {
		int fpa_int[MAX_FPA_REGS];
		double fpa_double[MAX_FPA_REGS/2];
		float fpa_float[MAX_FPA_REGS];
	} fpa_un;
#define fpa_intreg fpa_un.fpa_int
#define fpa_doublereg fpa_un.fpa_double
#define fpa_floatreg fpa_un.fpa_float
};

#ifdef KERNEL
struct fpasave *fpasave;		/* NPROC of these allocated (1 per process) */

long fpa_lastused[MAX_FPA_SETS];	/* last time this register set was allocated */
struct proc *fpa_proc[MAX_FPA_SETS];	/* current owner of this set */
int fpa_curreg;			/* current register set in use */
char fpa_present;		/* fpa is present */
char fpa_unused;		/* what to set fpa_reg to by default */
int fpa_regs;			/* bit mask for fpa registers */
int fpa_status;			/* status of last exception */
#endif KERNEL
#endif LOCORE

#define fpa_reset()	* (int *) FPA_RST = 0
#define fpa_rdscx()	* (int *) FPA_RDSCX
#define fpa_lockfp()	* (int *) FPA_LOCKFP = 0
#define fpa_tskswu(n)	* (int *) FPA_TSKSWU = n
#define fpa_wtstr(n)	* (int *) FPA_WTSTR = n
#define fpa_rdstr()	* (int *) FPA_RDSTR
#define fpa_hasreg(reg)	(((reg)&FPA_LOCK) == 0)

#define FPA_BASE	0xff000000
#define FPA_RST		(FPA_BASE + 0x20004)
#define FPA_LOCKFP	(FPA_BASE + 0x27000)
#define FPA_TSKSWU	(FPA_BASE + 0x23fb8)
#define FPA_RDSCX	(FPA_BASE + 0x2dfb8)
#define FPA_WTSTR	(FPA_BASE + 0x43fb8)
#define FPA_RDSTR	(FPA_BASE + 0x0dfb8)

#define FPA_TASK_EXCEPTION	0x1000	/* this task caused exception */
#endif FPA
