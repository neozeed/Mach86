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
#include "wb_sens.h"


#include "../h/param.h"
#include "../h/dir.h"
#include "../h/systm.h"
#include "../h/user.h"
#include "../h/proc.h"
#include "../h/kernel.h"

#if	NWB_SENS > 0
#include "../sensor/syslocal.h"
#include "../sensor/mondefs.h"
#include "../sensor/montypes.h"
#endif	NWB_SENS > 0


#define MON_OFLOWRECSIZE	sizeof(mon_errrec) /* In chars		  */
#define MON_ENABLEVECTORSIZE	12      /* Chars in enable vector	  */
#define MON_REQLISTSIZE		256     /* No. of entries in request list */
#define MON_REQOPENSLOT		0	/* Marks open slot in req. list   */
#define MON_SUPERUSERUID	0	/* uid of root			  */
#define MON_BADPID		-1	/* For marking accountant_pid	  */


extern u_char	*mon_write_ptr,		/* Write pointer in mon_eventvector 	 */
       	*mon_read_ptr,		/* Read pointer in mon_eventvector  	 */
       	*mon_eventvector_end;	/* First pos after buffer, start of appx */
extern int    	mon_eventvector_count;	/* No. of chars of valid event records in mon_eventvector */
extern int	mon_semaphore = 0;	/* Used to detect concurrancy	    	 */
extern int     mon_oflow_count = 0;	/* Event record overflow	    	 */
extern u_short	mon_enablevector[MON_ENABLEVECTORSIZE];
				/* enable flags for sensors		 */
extern struct timeval mon_time;		/* TIMESTAMP FIX */
extern int mon_propri;


int
sens_init() {
        if (mon_initflag)          /* must already be initialized    */
        {
            u.u_r.r_val1 = MON_ALRDY_INIT;
            mon_printf (("Already Initialized\n"));
        }
	/*
	 * Set up pointers and counters for event vector
	 */
        mon_write_ptr	      = mon_eventvector;
        mon_read_ptr	      = mon_eventvector;
        mon_eventvector_end   = &mon_eventvector[MON_EVENTVECSIZE];
        mon_eventvector_count = 0;
        mon_oflow_count	      = 0;

        mon_initflag	   = TRUE;	/* records whether initialized */
        mon_accountant_pid = CALLERID;	/* initializer is accountant   */
        mon_printf (("Accountant is %d\n", CALLERID));
	/*
	 * Turn off kernel sensors, just in case
	 */
        for (i = 0; i < MON_ENABLEVECTORSIZE; i++)
	    mon_enablevector[i] = 0;
	/*
	 * Initialize request vector to all entries open
	 */
        for (i = 0; i < MON_REQLISTSIZE; i++) {
            mon_requests[0].eventnumber = MON_REQOPENSLOT;
	}
        mon_printf (("Initialization done: oflow = %d\n",
		 mon_oflow_count));
        u.u_r.r_val1 = MON_EVENTVECSIZE;	/* return size of ring buffer */
}

int
sens_shutdown() {
	uprintf ("enter sens_shutdown\n");
}
