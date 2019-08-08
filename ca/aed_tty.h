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
#define AED_MMAP	0xf40a0000
#define AED_DELAY	DELAY(2)
#else SBPROTO
#define AED_MMAP	0xf10a0000
#define AED_DELAY
#endif SBPROTO

#define aedrd(X,Y,Z) bcopy(AED_MMAP+Z, X, Y+Y)
#define aedwr(X,Y,Z) bcopy(X, (AED_MMAP+Z), Y+Y)


#define term_mode	0x0200
#define setup_mode	0x0300
#define data_port	0x4000
#define status_port	0x4002

char *aedbase = (char *)AED_MMAP;

short *aed_data;
short *aed_status;

#define AEDTMO 50000
#define vinit() aed_data = (short *)(aedbase + data_port);

#define vwait()	{register int tmo = AEDTMO;\
			while (*aed_data) {\
				if(tmo-- < 0) {\
					aed_screen_init();\
					tmo = AEDTMO;\
				}\
				AED_DELAY; \
			} }
#define vterm(c)	{ \
			vwait(); \
			AED_DELAY;\
			*aed_data = (term_mode | c); \
			AED_DELAY;}

#define vsetup(c)	{ while (*aed_data) AED_DELAY; \
			AED_DELAY;\
			*aed_data = (setup_mode | c); \
			AED_DELAY; }
