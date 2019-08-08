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
/*	psl.h	6.1	83/07/29	*/

/*
 * VAX program status longword
 */

#define	PSL_C		0x00000001	/* carry bit */
#define	PSL_V		0x00000002	/* overflow bit */
#define	PSL_Z		0x00000004	/* zero bit */
#define	PSL_N		0x00000008	/* negative bit */
#define	PSL_ALLCC	0x0000000f	/* all cc bits - unlikely */
#define	PSL_T		0x00000010	/* trace enable bit */
#define	PSL_IV		0x00000020	/* integer overflow enable bit */
#define	PSL_FU		0x00000040	/* floating point underflow enable */
#define	PSL_DV		0x00000080	/* decimal overflow enable bit */
#define	PSL_IPL		0x001f0000	/* interrupt priority level */
#define	PSL_PRVMOD	0x00c00000	/* previous mode (all on is user) */
#define	PSL_CURMOD	0x03000000	/* current mode (all on is user) */
#define	PSL_IS		0x04000000	/* interrupt stack */
#define	PSL_FPD		0x08000000	/* first part done */
#define	PSL_TP		0x40000000	/* trace pending */
#define	PSL_CM		0x80000000	/* compatibility mode */

#define	PSL_MBZ		0x3020ff00	/* must be zero bits */

#define	PSL_USERSET	(PSL_PRVMOD|PSL_CURMOD)
#define	PSL_USERCLR	(PSL_IS|PSL_IPL|PSL_MBZ)
