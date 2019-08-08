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
/* $Header: swapconf.c,v 4.0 85/07/15 00:48:32 ibmacis GAMMA $ */
/* $Source: /ibm/acis/usr/sys_ca/ca/RCS/swapconf.c,v $ */

#ifndef lint
static char *rcsid = "$Header: swapconf.c,v 4.0 85/07/15 00:48:32 ibmacis GAMMA $";
#endif

/*     autoconf.c      6.3     83/08/11        */
/*
 * Setup the system to run on the current machine.
 *
 * Configure() is called at boot time and initializes the uba and mba
 * device tables and the memory controller monitoring.  Available
 * devices are determined (from possibilities mentioned in ioconf.c),
 * and the drivers are initialized.
 *
 */
#include "../machine/pte.h"
#include "../h/param.h"
#include "../h/systm.h"
#include "../h/map.h"
#include "../h/buf.h"
#include "../h/dk.h"
#include "../h/vm.h"
#include "../h/conf.h"
#include "../h/dmap.h"
 /*
  * The following several variables are related to
  * the configuration process, and are used in initializing
  * the machine.
  */
int cold = 1;				  /* if 1, still working on cold-start */
#define        DMMIN   32		  /* units are atoms of swap device space (512)  
					     */

#ifdef ROMPTXT
#define        DMMAX   64		  /* was 1024 -- zapped 4/84 by JEC for tiny disk
					     */
#define        DMTEXT  64		  /* was 1024 -- zapped 4/84 by JEC for tiny disk
					     */
#else
#define        DMMAX   1024		  /* was 1024 -- upped 3/85 by WEW for 70meg disk
					     */
#define        DMTEXT  1024		  /* was 1024 -- upped 3/85 by WEW for 70meg disk
					     */
#endif

#define        MAXDUMP 0x1000		  /* 2**12 sectors of 2**9 bytes = 2M */
int dumplo = 0;
int dmmin = DMMIN;
int dmmax = DMMAX;
int dmtext = DMTEXT;
/*
 * Configure swap space and related parameters.
 */
swapconf()
{
	register struct swdevt *swp;

	for (swp = swdevt; swp->sw_dev; swp++) {
		if (bdevsw[major(swp->sw_dev)].d_psize)
			swp->sw_nblks =
			    (*bdevsw[major(swp->sw_dev)].d_psize)(swp->sw_dev);
		if (swp->sw_nblks == 0)
			panic("swap blocks = 0");
	}
	if (!cold)			  /* in case called for mba device */
		return;
	if (dumplo == 0)
		dumplo = swdevt[0].sw_nblks - MAXDUMP;
	if (dumplo < 0)
		dumplo = 0;
	if (dmmin == 0)
		dmmin = DMMIN;
	if (dmmax == 0)
		dmmax = DMMAX;
	if (dmtext == 0)
		dmtext = DMTEXT;
	if (dmtext > dmmax)
		dmtext = dmmax;
	cold = 0;
}
