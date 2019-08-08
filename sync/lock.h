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
 *	File:	sync/lock.h
 *	Author:	Avadis Tevanian, Jr., Michael Wayne Young
 *
 *	Copyright (C) 1985, Avadis Tevanian, Jr., Michael Wayne Young
 *
 *	Locking primitives definitions
 *
 * HISTORY
 * 11-Jun-86  Bill Bolosky (bolosky) at Carnegie-Mellon University
 *	Removed ';' from definitions of locking macros (defined only
 *	when NCPU < 2). so as to make things compile.
 *
 * 28-Feb-86  Bill Bolosky (bolosky) at Carnegie-Mellon University
 *	Defined adawi to be add when not on a vax.
 *
 * 07-Nov-85  Michael Wayne Young (mwyoung) at Carnegie-Mellon University
 *	Overhauled from previous version.
 */

#ifndef	_LOCK_
#define	_LOCK_

#ifdef	KERNEL
#include "cpus.h"
#else	KERNEL
#include <sys/features.h>
#endif	KERNEL

#ifdef	KERNEL
#include "../h/types.h"
#else	KERNEL
#include <sys/types.h>
#endif	KERNEL

struct lock {
#ifdef	vax
	/*
	 *	Efficient VAX implementation -- see field description below.
	 */
	int		read_count:16,
			want_upgrade:1,
			want_write:1,
			:0;

	int		interlock;
#else	vax
	/*	Only the "interlock" field is used for hardware exclusion;
	 *	other fields are modified with normal instructions after
	 *	acquiring the interlock bit.
	 */
	int		interlock;	/* Interlock for remaining fields */
	boolean_t	want_write;	/* Writer is waiting */
	boolean_t	want_upgrade;	/* Read-to-write upgrade waiting */
	int		read_count;	/* Number of accepted readers */
#endif	vax
};

typedef struct lock	lock_data_t;
typedef struct lock	*lock_t;

typedef int		simple_lock_data_t;	/* 1 bit is sufficient */
typedef int		*simple_lock_t;

#if	NCPUS > 1
void		simple_lock_init();
void		simple_lock();
void		simple_unlock();
void		lock_init();
void		lock_write();
void		lock_read();
void		lock_done();
boolean_t	lock_read_to_write();
void		lock_write_to_read();

#define		lock_write_done(l)	lock_done(l)
#define		lock_read_done(l)	lock_done(l)
#else	NCPUS > 1
/*
 *	No multiprocessor locking is necessary.
 */
#define simple_lock_init(l)	
#define simple_lock(l)		
#define simple_unlock(l)	
#define lock_init(l)		
#define lock_write(l)		
#define lock_read(l)		
#define lock_done(l)		
#define lock_read_to_write(l)	(0)
#define lock_write_to_read(l)	

#define lock_write_done(l)	
#define lock_read_done(l)	
#endif	NCPUS > 1

/* For compatibility: */

#define RD_LOCK(lock)	lock_read(lock)
#define RD_UNLOCK(lock)	lock_read_done(lock)
#define EX_LOCK(lock)	lock_write(lock)
#define EX_UNLOCK(lock)	lock_write_done(lock)

#define BUSYP(s, bit) if (bbssi(bit, (caddr_t) (s))) busyP((caddr_t) (s), bit)
#define BUSYV(s, bit) bbcci(bit, (caddr_t) (s))

#ifndef vax
#define _adawi(increment,address)	{*(address) += (increment);}
#endif vax
#define mpinc(place) _adawi(1, (caddr_t)&place)
#define mpdec(place) _adawi(-1, (caddr_t) &place)

#endif	_LOCK_
