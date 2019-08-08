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
#ifdef SBMODEL
#define AED_MMAP                0xf40a0000
#else SBPROTO
#define AED_MMAP                0xf10a0000
#endif
#define AED_DELAY               DELAY(2)

#define AEDOPEN                 0x01
#define AED_WM_MODE             0x02
#define AED_LOADING_CODE        0x04
#define AED_RUN_CODE            0x08
#define AED_SEMAPHORE           0x4000
#define CTRL_STORE_SIZE         0x1000

#define PRI_AED (PZERO + 5)

struct aed {
        unsigned long sram_loc;
        long ctrl_offset;
        unsigned long state;
        short pgrp;
};
typedef struct aed AED;
