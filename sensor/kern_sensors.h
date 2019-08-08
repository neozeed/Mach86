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
/* Sensor macros for the Kern process.
   Generated from kernel.sen on June 14, 1984 (by hand).
   Contains the following macros:
	FileClose(device,inumber,finalsize)
*/

#ifdef KERNEL
#include "../sensor/mondefs.h"
#include "../sensor/montypes.h"
#endif

#ifndef ntohs
#include "../netinet/in.h"
#endif

#if	NWB_SENS  > 0
#define FileClose(device,inumber,finalsize)				\
    if (*(mon_enablevector + 0) & (1 << 6)) { \
	if (mon_semaphore++ == 0) { \
	    if (mon_eventvector_count < \
		    MON_EVENTVECSIZE - 14 * 2 - sizeof (mon_errrec)) { \
		register    mon_putevent * reg_ptr = (mon_putevent *) mon_write_ptr; \
		register short *sen_fields = reg_ptr -> fields; \
		mon_printf (("FileClose: mon_write_ptr = %d\n", mon_write_ptr)); \
		reg_ptr -> cmd.type = MONOP_PUTEVENT_INT; \
		reg_ptr -> cmd.length = sen_fields + 2 - (short *) reg_ptr; \
		mon_eventvector_count += reg_ptr -> cmd.length * 2; \
		reg_ptr -> eventnumber = (short) 6; \
		reg_ptr -> performer = 0; \
		reg_ptr -> object = ((short) device << 16) | \
		    ((short) inumber & 0xffff); \
		reg_ptr -> initiator = u.u_procp -> p_pid; \
		TIMESTAMP (reg_ptr -> timestamp); \
		*(int *) (sen_fields) = (int) finalsize; \
		sen_fields += 2; \
		if (sen_fields > (short *) mon_eventvector_end) \
		    mon_write_ptr = Wraparound ((unsigned char *) sen_fields);\
		else \
		    mon_write_ptr = (unsigned char *) sen_fields; \
		mon_printf (("%dn", mon_write_ptr)); \
	    } \
	    else \
		mon_oflow_count++; \
	    mon_semaphore--; \
	} \
    }				/* end FileClose */
#else	NWB_SENS  > 0
#define FileClose(device,inumber,finalsize)
#endif	NWB_SENS  > 0
