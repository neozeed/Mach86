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
#ifdef KERNEL
#include "../sensor/mondefs.h"
#include "../sensor/montypes.h"
#endif
#ifndef ntohs
#include "../netinet/in.h"
#endif

#if	NWB_SENS > 0
#define kern_fork_sensor \
    if (*(mon_enablevector + 0) & (1 << 11)) { \
	mon_printf (("doing fork\n")); \
    }
#else	NWB_SENS > 0
#define kern_fork_sensor
#endif	NWB_SENS > 0
