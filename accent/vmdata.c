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

/****************************************************************************
*  File: 	VMData.PasMac
*
*  Author:	Richard F. Rashid
*
*  Copyright (C) 1981 Richard F. Rashid and Carnegie-Mellon University
*  Copyright (C) 1982 Richard F. Rashid and Carnegie-Mellon University
*  Copyright (C) 1983 Richard F. Rashid and Carnegie-Mellon University
*  Copyright (C) 1984 Richard F. Rashid and Carnegie-Mellon University
*  Copyright (C) 1985 Richard F. Rashid, Robert Baron and Carnegie-Mellon University
*  Copyright (C) 1986 Richard F. Rashid and Carnegie-Mellon University
****************************************************************************
* ABSTRACT:
*       Routines for allocating and deallocating data structures.
****************************************************************************
 * HISTORY
 * 25-May-86  Rick Rashid (rfr) at Carnegie-Mellon University
 *	Added code for establishing various lock data structures and for
 *	handling multiprocessor locks.  Note that we still need to
 *	replace references to processes with appropriate task and thread
 *	references.
 *	
 *
 * 13-May-86  Rick Rashid (rfr) at Carnegie-Mellon University
 *	Removed DBG calls. Removed bogus mpenqueue call.
 *
 *  6-May-86  Avadis Tevanian (avie) at Carnegie-Mellon University
 *	Fixed parameters to vm_allocate and vm_deallocate.
 *
 *  5-May-86  Michael Young (mwyoung) at Carnegie-Mellon University
 *	Corrected MoveMemory to NOT preserve pages at the end of regions,
 *	in order to prevent memory leaks, and in order to use the
 *	inherently more efficient "copy-with-deallocate" option to vm_map_copy.
 *
 * 29-Apr-86  Rick Rashid (rfr) at Carnegie-Mellon University
 *	Added new parameter to vm_allocate call.
 *
 * 29-Apr-86  Rick Rashid (rfr) at Carnegie-Mellon University
 *	Removed ipc_kmsg_zone since it was no longer needed.
 *
 * 20-Feb-86  Richard F. Rashid
 *		Reformatted.  Added allocation of ipc_kmsg_zone and ipc_soft_map.  
 * 11-Oct-85  Robert V Baron (rvb) at Carnegie-Mellon University
 *		Now uses new shared memory scheme.  Init and peek got murged.
 *		ProcArray and PortArray are now valloc'ed in machdep.c
 *
 * 19-Mar-85  Robert V Baron (rvb) at Carnegie-Mellon University
 *		DataAlloc now protects the memory it returns to URKW.
 *		*KMsgAllocate now DataAlloc's TOTAL_KMSG_SIZE bytes correctly.
 *	
 *  31-May-83  Richard F. Rashid
 *             	Made changes to SPMAllocate to reuse process map area
 *             	and to SVStkAllocate and SVStkFree to reuse SV stacks.
 * ...
 * ...
 *  07-Sep-81  Richard F. Rashid
 *             	Started.
 ****************************************************************************/

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/vmmac.h"
#ifdef	vax
#include "../vax/mtpr.h"
#endif	vax
#include "../machine/pte.h"


#include "../h/queue.h"
#include "../sync/lock.h"
#include "../accent/accent.h"
#include "../accent/kern_ipc.h"

#include "../h/zalloc.h"
#include "../machine/pmap.h"
#include "../vm/vm_param.h"
#include "../vm/vm_map.h"
#include "../vm/vm_kern.h"

struct IPCstate *IPC;
procp_ ProcArray, eProcArray;
portp_ PortArray, ePortArray;

queue_head_t FreeKMsgQ;
queue_head_t FreeProcQ;
queue_head_t FreePortQ;

lock_data_t KMsgLockData;
lock_t KMsgLock;

lock_data_t FreeProcLockData;
lock_t FreeProcLock;

lock_data_t FreePortLockData;
lock_t FreePortLock;

lock_data_t ipc_map_lock;

vm_map_t ipc_soft_map;

long PermSegPort;
struct Statistics Statistics;

procp_ *aproc;			/* the accent per processor proc table */
int ipc_debug = 0;		/* enable debugging */

#if FOSSIL
static int unxaccentprocfreerunning = 0;
unxaccentprocfree()
/****************************************************************************
*Abstract
*
*Parameters
*
*Environment
*
*Results
*
*SideEffects
*
*Errors
*
*Calls
*
*Design
****************************************************************************/
{
	printf("UNX_IPC_FREE:\n");	

	unxaccentprocfreerunning = 1;
	/* and then what */
	{
		register procp_ elt = &ProcArray[PAGER_PROCESS]+1;
		register int i = PAGER_PROCESS + 1;
		register int limit = LAST_USED_PROCESS;

		for (; i <= limit ; i++, elt++) {
			if (elt->Active) {
				printf("ProcRecord hit = %d\n", pidx(elt));
				unxIPCSuicide(elt);
			}
		}
	}
	printf("ProcArray = %x.  ", ProcArray);
	if (!queuecheck("ProcRecord Sanity", FreeProcQ, LAST_USED_PROCESS))
		panic("ProcQ?");

	{
		register portp_ elt = &PortArray[FIRST_NON_RESERVED_PORT];
		register int i = FIRST_NON_RESERVED_PORT;
		register int limit = NumPorts;
		for (; i < limit ; i++, elt++) {
			if (elt->InUse) {
				printf("PORTRecord hit = %d\n", i);
				DelPort(i);
			}
		}
	}
	printf("PortArray = %x.  ", PortArray);
	if (!queuecheck("PORTRecord Sanity", FreePortQ, NUM_PORTS))
		panic("PortQ?");
}
#endif FOSSIL

TPaccentinit()
{
	if (!suser())
		return NotSuperUser;
	return unxaccentinit();
}

unxaccentinit()
/****************************************************************************
* Abstract:
*	General initialization of IPC data structures.
****************************************************************************/
{
	if (show_space) {
		printf("UNX_IPC_INIT: %d Proc Records (L%d) = %d bytes\n",
		sizeof (ProcSize)/sizeof (struct ProcRecord),
		sizeof (struct ProcRecord), sizeof (ProcSize));
		printf("UNX_IPC_INIT: %d Port Records (L%d) = %d bytes\n",
		sizeof (PortSize)/ sizeof (struct PortRecord),
		sizeof (struct PortRecord), sizeof (PortSize));

		printf("sizeof IPCRECORD = %o(%d), PORTRECORD = %o(%d)\n",
		sizeof (struct ProcRecord), sizeof (struct ProcRecord),
		sizeof (struct PortRecord), sizeof (struct PortRecord));
	}

	if ( (sizeof (struct ProcRecord) % 8 ) ||
	    (sizeof (struct PortRecord) % 8) ) {
		printf("sizeof IPCRECORD = %o(%d), PORTRECORD = %o(%d)\n",
		sizeof (struct ProcRecord), sizeof (struct ProcRecord),
		sizeof (struct PortRecord), sizeof (struct PortRecord));
		panic("quad alignment");
	}

	KMsgLock = &KMsgLockData;
        simple_lock_init(KMsgLock);

	FreeProcLock = &FreeProcLockData;
        simple_lock_init(FreeProcLock);

	FreePortLock = &FreePortLockData;
        simple_lock_init(FreePortLock);

	simple_lock_init(&ipc_map_lock);

	queue_init(&FreeProcQ);
	IPC->FreeProcQ = (procp_)&FreeProcQ;
	IPC->ProcArray = ProcArray;

	{
		register procp_ elt = &ProcArray[PAGER_PROCESS]+1;
		register int i = PAGER_PROCESS + 1;
		register int limit = LAST_USED_PROCESS;
		for (; i <= limit ; i++, elt++) {
			queue_init(&(elt->MsgsWaiting));
			proc_lock_init(elt);
			elt->Active = 0;
			enqueue(&FreeProcQ, elt);
		}
	}

	queue_init(&(ProcArray[KERNEL_PROCESS].MsgsWaiting));
	queue_init(&(ProcArray[PAGER_PROCESS].MsgsWaiting));
	queue_init(&(ProcArray[NO_PROCESS].MsgsWaiting));
	queue_init(&(ProcArray[RIGHTS_IN_TRANSIT].MsgsWaiting));

	proc_lock_init((&(ProcArray[KERNEL_PROCESS])));
	proc_lock_init((&(ProcArray[PAGER_PROCESS])));
	proc_lock_init((&(ProcArray[NO_PROCESS])));
	proc_lock_init((&(ProcArray[RIGHTS_IN_TRANSIT])));

	queue_init(&FreePortQ);
	IPC->FreePortQ = (portp_)&FreePortQ;
	IPC->PortArray = PortArray;

	{
		register portp_ elt = &PortArray[FirstNonReservedPort];
		register int i = FirstNonReservedPort;
		register int limit = NumPorts;
		for (; i < limit ; i++, elt++) {
			elt->InUse = 0;
			port_lock_init(elt);
			queue_init(&(elt->SecondaryMsgQ));
			if (i > LastInitPort) enqueue(&FreePortQ, elt);
		}
	}

	queue_init(&FreeKMsgQ);
	IPC->FreeKMsgQ = (kmsgp_)&FreeKMsgQ;

	/*
	* Create an empty software map for later use in virtual memory copy operations:
	*/
	ipc_soft_map = vm_map_create(pmap_create(-1),VM_MIN_ADDRESS,VM_MAX_ADDRESS,TRUE);

	return SUCCESS;
}

void KMsgDeallocateAll(kmsgptr)
kmsgp_ kmsgptr;
/****************************************************************************
* Abstract:
*       Deallocate all storage associated with a kmsg (other than the
*       kmsg structure itself) and return kmsg to the kmsg free queue.
*
* Parameters:
*       kmsgptr - p to the kmsg whose storage is to be deallocated
*
* Side Effects:
*       Releases any pointed to data in the  kmsg.
*****************************************************************************/
{
	if (!kmsgptr->MsgHeader.SimpleMsg)
		CopyOutMsg (NULL, kmsgptr, TRUE);
	MKMsgFree(kmsgptr);
}

void KMsgDeallocateStorage (kmsgptr)
kmsgp_ kmsgptr;
/****************************************************************************
* Abstract:
*       Deallocate all storage associated with a kmsg (other than the
*       kmsg structure itself).
*
* Parameters:
*       kmsgptr - p to the kmsg whose storage is to be deallocated
*
* Side Effects:
*       Releases any pointed to data in the  kmsg.
*****************************************************************************/
{
	if (kmsgptr->MsgHeader.SimpleMsg) return;
	CopyOutMsg (NULL, kmsgptr, TRUE);
}

ptr_ MoveMemory(src_map,src_addr,dst_map,num_bytes,src_dealloc)
	vm_map_t	src_map;
	ptr_		src_addr;
	vm_map_t	dst_map;
	long		num_bytes;
	long_bool	src_dealloc;
/****************************************************************************
* Abstract:
*	Move memory from source to destination map, possibly deallocating
*	the source map reference to the memory.
*
* Parameters:
*	src_map	- Address map of source memory.
*	src_addr - Address of source data.
*	dst_map	- Address map of destination memory.
*	num_bytes- Number of bytes to copy or move.
*	src_dealloc - Flag to indicate source memory should be deallocated.
*
* Environment:
*	Assumes the src and dst maps are not already locked.
*
* Side effects:
*	May lock source and destination maps.
*
* Returns:
*	Returns new destination address or 0 (if a failure occurs).
*****************************************************************************/
{
	vm_offset_t	src_start;	/* Actual beginning of copied region */
	vm_offset_t	src_end;	/* Actual end */
	vm_size_t	src_size;	/* Size of rounded-out region */
	vm_offset_t	dst_start;	/* First address copied to destination */
	int		old_spl;
	kern_return_t	result;

	/*
	 *	Page-align the source region
	 */

	src_start = trunc_page(src_addr);
	src_size = (src_end = round_page(src_addr + num_bytes)) - src_start;

	/*
	 *	If there's no destination, we can be at most deallocating
	 *	the source range.
	 */
	if (dst_map == VM_MAP_NULL) {
		if (src_dealloc)
			result = vm_deallocate(src_map, src_start, src_size);
		return(0);
	}

	/*
	 *	Allocate a place to put the copy
	 */

	dst_start = (vm_offset_t) 0;
	if ((result = vm_allocate(dst_map, &dst_start, src_size, TRUE)) == KERN_SUCCESS) {
		/*
		 *	Perform the copy, asking for deallocation if desired
		 */
		old_spl = splvm();
		result = vm_map_copy(dst_map, src_map, dst_start, src_size, src_start, FALSE, FALSE);
		(void) splx(old_spl);
		if (result == KERN_SUCCESS) {
		  	if (src_dealloc) result = vm_deallocate(src_map,src_start,src_size);
		}
		 
	}

	/*
	 *	Return the destination address corresponding to
	 *	the source address given (rather than the front
	 *	of the newly-allocated page).
	 */

	return ((result == KERN_SUCCESS) ? (dst_start + (src_addr - src_start)) : 0);
}
