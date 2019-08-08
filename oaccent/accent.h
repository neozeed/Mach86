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
 * accent.h
 *
 * This file includes all the types exported by Accent.  Only the canonical
 * types are imported.  This file should normally be used instead of the
 * older accenttype.h, which includes a number of types strictly for
 * backwards compatibility with the older perq native_c code.
 *
 * Richard F. Rashid, Michael B. Jones
 *
 * Copyright (C) 1981, 1982, 1983, 1984, 1985
 *	Richard F. Rashid, Michael B. Jones, and Carnegie-Mellon University
 *
 * History:
 *
 *  24-Oct-85	Michael B. Jones
 *		Created accent.h to use only the canonical types.
 */

#ifndef _Accent_
#define _Accent_

/*
 * If this file is included before accenttype.h, the newer definitions of
 * pascal strings and msg type descriptors (TypeType) are used automatically.
 */

#ifndef _AccentType	/* Test for previous inclusion of AccentType */
#ifndef NewStrings
#define NewStrings		/* Use new string definitions */
#endif
#ifndef BIT_STRUCTURES
#define BIT_STRUCTURES 1	/* Use bit structures instead of shifting */
#endif
#endif
#ifdef	KERNEL
#include "../accent/vax/machdep.h"	/* Machine dependencies */
#include "../accent/c_types.h"		/* Type definitions for C */
#include "../accent/vax/acc_impl.h"	/* Accent implementation parameters */
#include "../accent/acc_macro.h"	/* general macros */
#include "../accent/vax/impl_macros.h"	/* strange macros */
#include "../accent/acc_ipc.h"		/* IPC definitions */
#include "../accent/kern_ipc.h"		/* kmsg, port, ... */
#include "../accent/vax/impl_proc.h"	/* low level proces structure */
#include "../accent/acc_errors.h"	/* Error and status returns */
#include "../accent/acc_debug.h"	/* kern_ debugging switches */
#else	KERNEL
#include <accent/vax/machdep.h>		/* Machine dependencies */
#include <accent/c_types.h>		/* Type definitions for C */
#include <accent/vax/acc_impl.h>	/* Accent implementation parameters */
#include <accent/acc_ipc.h>		/* IPC definitions */
#include <accent/acc_vm.h>		/* Virtual memory definitions */
#include <accent/acc_errors.h>		/* Error and status returns */
#include <accent/vax/acc_part.h>	/* The partition table */
#include <accent/acc_seg.h>		/* Accent segment management types */
#include <accent/acc_proc.h>		/* Accent process management types */
#include <accent/vax/acc_traps.h>	/* Trap definitions */

#define NameServerPort 3		/* we need this on the vax */

#endif	KERNEL
#endif	_Accent_
