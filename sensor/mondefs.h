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
#if	NWB_SENS > 0
/*
 * Declarations for the monitor system call.
 */
#include "../sensor/monops.h"	/* operator defines and MONOPS macro */
#include "../sensor/monerrcds.h"	/* error codes from syscall */

#define REQ_LENGTH 1024*50
#define ACCT_BUFFER 0x10be8
#if	NWB_SENS > 0
#define MON_EVENTVECSIZE  35000
#define mon_printf(a) uprintf a	/* Monitor kernel debugging statements */
#else
#define MON_EVENTVECSIZE  50000	/* Size of vect that stores event recs */
#define mon_printf(a)
#endif
#define MON_EVENTRECSIZE sizeof (mon_command) /* Max size of event record */
#ifdef	SHORTALIGN		/* For fetches that may need aliignment */
#define PackStr(ptr) ( ntohs((*ptr<<8) | (*(ptr+1))) )
#define NotEOS(ptr,last) (ptr <= last && *ptr++&0xff && *ptr++&0xff)
#else	SHORTALAIGN
#define PackStr(ptr) ((short)*ptr)
#define NotEOS(ptr,last) (ptr <= last && *ptr&0x00ff && *ptr++&0xff00)
#endif	SHORTALIGN

#define TIMESTAMP(timeval) \
mon_propri = spl7(); \
mon_time = time; \
splx (mon_propri); \
timeval = (int) ((mon_time.tv_sec << 15) | (mon_time.tv_usec >> 5));

extern u_char	*mon_write_ptr,		/* Write pointer in mon_eventvector 	 */
       	*mon_read_ptr,		/* Read pointer in mon_eventvector  	 */
       	*mon_eventvector_end;	/* First pos after buffer, start of appx */
extern int    	mon_eventvector_count;	/* No. of chars of valid event records in mon_eventvector */
extern int	mon_semaphore;	/* Used to detect concurrancy	    	 */
extern int     mon_oflow_count;	/* Event record overflow	    	 */
extern u_short	mon_enablevector[];
				/* enable flags for sensors		 */
extern struct timeval mon_time;	/* TIMESTAMP FIX */
extern int mon_propri;		/* Save Processor priority */

extern u_char *Wraparound();
#endif	NWB_SENS > 0
