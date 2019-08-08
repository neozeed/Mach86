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
/*
 * this file contains symbolic form of numbers displayed in the console
 * panel leds.
 */
#define LED_BLANK	0xff	/* to blank the leds */

/* kernel warning LED value */
#define LED_NO_SCREEN	0x99	/* no output screen found */

/* non-recoverable kernel errors */
#define LED_BAD_SP	0x94	/* bad kernel stack */
#define LED_INT_SP	0x96	/* bad kernel stack (interrupt) */
#define LED_MEM_CONFIG	0x97	/* invalid memory configuration */
/* standalone errors */
#define LED_NOBOOT	0x98	/* bootxx could not find /boot */

/* other known values for the led's (from ROS) */
#define LED_KEY_LOCK	0x99	/* key is in lock position */
#define LED_MC_CHECK	0x88	/* machine check during machine check ?? */
#define LED_PC_CHECK	0x89	/* program check during program check ?? */
