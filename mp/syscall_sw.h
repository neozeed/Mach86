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
 * HISTORY
 * 10-Nov-85  Avadis Tevanian (avie) at Carnegie-Mellon University
 *	Moved lots of junk to user library header file.
 *
 * 15-Aug-85  Avadis Tevanian (avie) at Carnegie-Mellon University
 *	Add MP_swtch and MP_system_name.
 */

/*	flag values 	*/

#define ASETJMP 0
#define APSIG 2


#define FN(name, argno) {4*(argno+1),0,name}
#define FF1(name, argno, flags) {4*(argno+1),1<<flags,name}


/*
 *			NOTE
 *	The syscall handler fetches an entire entry by a move quadword. 
 * Thus the first two shorts will appear in reverse order in the first long of
 * the quad word.
 */

#ifndef LOCORE
#ifdef KERNEL
struct mp_sysent {
	u_short a_len;
	short a_flag;
	int (*a_fn)();
};
extern struct mp_sysent mp_sysent[] ;
extern int nmp_sysent ;
#endif KERNEL
#endif LOCORE
