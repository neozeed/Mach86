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
 * 5799-CGZ (C) COPYRIGHT IBM CORPORATION 1986
 * LICENSED MATERIALS - PROPERTY OF IBM
 * REFER TO COPYRIGHT INSTRUCTIONS FORM NUMBER G120-2083
 */
/* $Header: apdefs.h,v 5.2 86/02/25 21:47:30 katherin Exp $ */
/* $Source: /ibm/acis/usr/sys_ca/caio/RCS/apdefs.h,v $ */

#if !defined(lint) && !defined(LOCORE)  && defined(RCS_HDRS)
static char *rcsidapdefs = "$Header: apdefs.h,v 5.2 86/02/25 21:47:30 katherin Exp $";
#endif


/*
 * apdefs.h contains the few things that tty or asy should
 * include so that they do not need to include apvar.h.
 */

/*
 * Codes given to aprcvint if an error or break was seen
 * instead of a character.
 */
#define APERROR (-1)
#define APBREAK (-2)
/*
 * Codes passed to t_oproc routine for our line discipline
 * instead of a character to write. These say to turn the
 * break transmit bit on or off.
 */
#define APBREAKON	(-1)
#define APBREAKOFF	(-2)
#define APINTMASK	(-3)
