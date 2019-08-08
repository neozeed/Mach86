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
 * accenttype.h
 *
 * This file includes all the types exported by Accent, including those which
 * only exist for backwards compatibility with the perq native_c code.  The
 * similar file accent.h should be used instead of accenttype.h for all new
 * code, since it imports only the canonical names.
 *
 * Richard F. Rashid, Michael B. Jones
 *
 * Copyright (C) 1981, 1982, 1983, 1984, 1985
 *	Richard F. Rashid, Michael B. Jones, and Carnegie-Mellon University
 *
 * History:
 *
 *  24-Oct-85	Michael B. Jones
 *		Added include of perq_compat.h.
 *
 *  14-Aug-85	Michael B. Jones
 *		Broke up as per agreement between Bob and myself.
 *		This is now merely a stub for backwards compatibility.
 *
 *  27-Jun-85	Mark Hjelm
 *		Moved much to a new C_Types.h and reorganized this.
 *
 *  17-Jun-85	Michael B. Jones
 *		Added Align_Type to DefineString to combat compiler parameter-
 *		passing bug with strings.
 *
 *   6-May-85   Mark Hjelm
 *		Reincorporated C_Types.h from Perq
 *		Changed some constants to explicit longs
 *
 *  18-Mar-84	Michael B. Jones
 *		Added short_bool for use where compatibility is necessary.
 *
 *  24-Oct-84	Michael B. Jones
 *		Reserved three bytes at the beginning of Msg for expansion.
 *
 *  16-Oct-84	Michael B. Jones
 *		Added Msg definition, GeneralReturn codes and other defn's
 *		from old vmtypes.pas, vmipctypes.pas, and c_types.h files.
 *		Split off trap types into accenttrap.h.
 *
 *  13-Aug-84	Michael B. Jones
 *		Added Exit and Spawn trap codes.
 *
 *  20-Jul-84	Michael B. Jones
 *		Started by defining some trap codes.
 */

#ifndef _AccentType
#define _AccentType

#ifdef	KERNEL
#include "../accent/vax/machdep.h"	/* Machine dependencies */
#include "../accent/c_types.h"		/* Type definitions for C */
#include "../accent/vax/acc_impl.h"	/* Accent implementation parameters */
#include "../accent/acc_ipc.h"		/* IPC definitions */
#include "../accent/acc_vm.h"		/* Virtual memory definitions */
#include "../accent/acc_errors.h"		/* Error and status returns */
#include "../accent/vax/acc_part.h"	/* The partition table */
#include "../accent/acc_seg.h"		/* Accent segment management types */
#include "../accent/acc_proc.h"		/* Accent process management types */
#include "../accent/vax/acc_traps.h"	/* Trap definitions */

#include "../accent/perq_compat.h"		/* Obsolete definitions for native_c code */

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

#include <accent/perq_compat.h>		/* Obsolete definitions for native_c code */


extern Port NameServerPort;

#endif	KERNEL
#endif	_AccentType
