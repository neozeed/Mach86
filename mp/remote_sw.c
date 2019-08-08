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
 *	File:	remote_sw.c
 *	Author:	Avadis Tevanian, Jr.
 *
 *	Copyright (C) 1984, Avadis Tevanian (rvb)		CMU
 *
 *	Define entry points for interprocessor communication.
 *
 * HISTORY
 * 24-Feb-85	Robert V Baron
 *	Add untimeout and unasleep
 *
 * 16-Jan-85  Avadis Tevanian (avie) at Carnegie-Mellon University
 *	Add IMC handler.
 *
 * 23-Sep-84  Avadis Tevanian (avie) at Carnegie-Mellon University
 *	Created
 *
 */

#include "wb_ml.h"
#include "../mp/remote_sw.h"

extern int nomp();
extern int mlrint();
extern int handle_remote_wakeup();
extern int handle_remote_psignal();
extern int handle_remote_imc();
extern int handle_remote_unasleep();
extern int handle_remote_untimeout();

struct mp_comm_entry mp_comm_ent[NCOMMTYPES] = {
#if	WB_ML
	mlrint,				/* 0 = ml driver */
#else	WB_ML
	nomp,				/* 0 = nomp */
#endif
	handle_remote_wakeup,		/* 1 = handle remote wakeup */
	handle_remote_psignal,		/* 2 = handle remote psignal */
	handle_remote_imc,		/* 3 = handle remote imc */
	handle_remote_unasleep,		/* 4 = handle remote unasleep */
	handle_remote_untimeout		/* 5 = handle remote timeout */
};
