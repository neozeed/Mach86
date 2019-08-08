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
/*      enet.c	Stanford	25 April 1983 */

/*
 *  Ethernet packet filter layer,
 *  	formerly: Ethernet interface driver
 *
 **********************************************************************
 * HISTORY
 * 17 October 1984	Jeff Mogul	Stanford
 *    More performance improvements:
 *	Added ENF_CAND, ENF_COR, ENF_CNAND, and ENF_CNOR, short-circuit
 *	operators, to make filters run faster.
 *	All evaluate "(*sp++ == *sp++)":
 *	ENF_CAND: returns false immediately if result is false, otherwise
 *		continue
 *	ENF_COR: returns true immediately if result is true, otherwise
 *		continue
 *	ENF_CNAND: returns true immediately if result is false, otherwise
 *		continue
 *	ENF_CNOR: returns false immediately if result is true, otherwise
 *		continue
 *	Also added ENF_NEQ to complement ENF_EQ
 *    - Maintain count of received packets per filter, dynamically
 *	re-organize filter queue to keep highly active filters in
 *	front of queue (but maintaining priority order), if they are
 *	"high priority" filters.
 *
 * 2 October 1984	Jeff Mogul	Stanford
 *	Made a few changes to enDoFilter() to speed it up, since profiling
 *	shows it to be rather popular:
 *	- precompute maximum word in packet and address of end of
 *	filters (thereby moving this code out of "inner loop").
 *	- minor re-arrangement to avoid re-evaluating a
 *	common subexpression.
 *	- changed #ifdef DEBUG in a few routines to #ifdef INNERDEBUG,
 *	so that code in inner loops isn't always testing the enDebug
 *	flag; this not only costs directly, but also breaks up some
 *	basic blocks that the optimizer could play with.
 *	- added enOneCopy flag; if true, then never deliver more than
 *	one copy of a packet.  This is equivalent to giving everyone
 *	a "high priority" device, and cuts down the number of superfluous
 *	calls to enDoFilter(). [Temporary hack, will remove this!]
 *
 * 24 August 1984	Jeff Mogul	Stanford
 *	YA bug with sleeping in enetwrite(); straightened out handling
 *	of counts in enKludgeSleep so that they indicate the number
 *	of sleeps in progress. Maybe I've got this right, now?
 *	Also, don't sleep forever (since the net might be down).
 *
 * 17 July 1984	Jeff Mogul	Stanford
 *	Bug fix: in enetwrite(), several problems with sleeping on
 *	IF_QFULL:
 *	- don't do it for kernel mode writes.
 *	- count # of procs sleeping, to avoid lost wakeups.  Old
 *		scheme would only wake up the first sleeper.
 *	- using sleeper-count, avoid using more than one timeout
 *		table entry per device; old scheme caused timeout table panics
 *	- trap interupted sleeps using setjmp, so that we can deallocate
 *		packet header and mbufs; otherwise we lost them and panicked.
 *
 * 5 July 1984	Jeff Mogul	Stanford
 *	Bug fix: in enetwrite() make sure enP_RefCount is zero before
 *	deallocating "packet".  Otherwise, "packets" get lost, and
 *	take mbufs (and ultimately, the system) with them.
 *
 * 8 December 1983	Jeffrey Mogul	Stanford
 *	Fixed bug in enetwrite() that eventually caused	allocator
 *	to run out of packets and panic.  If enetwrite() returns
 *	an error it should first deallocate any packets it has allocated.
 *
 * 10 November 1983	Jeffrey Mogul	Stanford
 *	Slight restructuring for support of 10mb ethers;
 *	- added the EIOCDEVP ioctl
 *	- removed the EIOCMTU ioctl (subsumed by EIOCDEVP)
 *	This requires an additional parameter to the enetattach
 *	call so that the device driver can specify things.
 *
 *	Also, cleaned up the enDebug scheme by adding symbolic
 *	definitions for the bits.
 *
 * 25-Apr-83	Jeffrey Mogul	Stanford
 *	Began conversion to 4.2BSD.  This involves removing all
 *		references to the actual hardware.
 *	Changed read/write interface to use uio scheme.
 *	Changed ioctl interface to "new style"; this places a hard
 *		limit on the size of a filter (about 128 bytes).
 *	"Packets" now point to mbufs, not private buffers.
 *	Filter can only access data in first mbuf (about 50 words worst case);
 *		this is long enough for all Pup purposes.
 *	Added EIOCMTU ioctl to get MTU (max packet size).
 *	Added an enetselect() routine and other select() support.
 *	Other stuff is (more or less) left intact.
 *	Most previous history comments removed.
 *	Changed some names from enXXXX to enetXXXX to avoid confusion(?)
 *
 * 10-Aug-82  Mike Accetta (mja) at Carnegie-Mellon University
 *	Added new EIOCMBIS and EIOCMBIC ioctl calls to set and clear
 *	bits in mode word;  added mode bit ENHOLDSIG which suppresses
 *	the resetting of an enabled signal after it is sent (to be
 *	used inconjunction with the SIGHOLD mechanism);  changed
 *	EIOCGETP to zero pad word for future compatibility;  changed enwrite()
 *	to enforce correct source host address on output packets (V3.05e).
 *	(Stanford already uses long timeout value and has no pad word - JCM)
 *	[Last change before 4.2BSD conversion starts.]
 *
 * 01-Dec-81  Mike Accetta (mja) at Carnegie-Mellon University
 *	Fixed bug in timeout handling caused by missing "break" in the
 *	"switch" state check within enetread().  This caused all reads
 *	to be preceeded by a bogus timeout.  In addition, fixed another
 *	bug in signal processing by also recording process ID of
 *	process to signal when an input packet is available.  This is
 *	necessary because it is possible for a process with an enabled
 *	signal to fork and exit with no guarantee that the child will
 *	reenable the signal.  Thus under appropriately bizarre race
 *	conditions, an incoming packet to the child can cause a signal
 *	to be sent to the unsuspecting process which inherited the
 *	process slot of the parent.  Of course, if the PID's wrap around
 *	AND the inheriting process has the same PID, well ... (V3.03d).
 *
 * 22-Feb-80  Rick Rashid (rfr) at Carnegie-Mellon University
 *	Rewritten to provide multiple user access via user settable
 *	filters (V1.05).
 *
 * 18-Jan-80  Mike Accetta (mja) at Carnegie-Mellon University
 *      Created (V1.00).
 *
 **********************************************************************
 */

#include "enet.h"

#if (NENET > 0)

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/mbuf.h"
#include "../h/buf.h"
#include "../h/dir.h"
#include "../h/user.h"
#include "../h/ioctl.h"
#include "../h/map.h"
#include "../machine/pte.h"
#include "../h/proc.h"
#include "../h/inode.h"
#include "../h/file.h"
#include "../h/tty.h"
#include "../h/uio.h"
#include "../h/queue.h"

#include "../h/protosw.h"
#include "../h/socket.h"
#include "../net/if.h"

#include "../h/enet.h"
#include "../vaxif/enetdefs.h"

#define	boolean bool

#define DEBUG	1
/* #define INNERDEBUG 1 */	/* define only when debugging enDoFilter()
					or enInputDone()  */

#define	enprintf(flags)	if (enDebug&(flags)) printf

/*
 * Symbolic definitions for enDebug flag bits
 *	ENDBG_TRACE should be 1 because it is the most common
 *	use in the code, and the compiler generates faster code
 *	for testing the low bit in a word.
 */

#define	ENDBG_TRACE	1	/* trace most operations */
#define	ENDBG_DESQ	2	/* trace descriptor queues */
#define	ENDBG_INIT	4	/* initialization info */
#define	ENDBG_SCAV	8	/* scavenger operation */
#define	ENDBG_ABNORM	16	/* abnormal events */


#define min(a,b)        ( ((a)<=(b)) ? (a) : (b) )

#define	splenet	splimp	/* used to be spl6 but I'm paranoid */

#define PRINET  26			/* interruptible */

/*
 *  'enQueueElts' is the pool of packet headers used by the driver.
 *  'enPackets'   is the pool of packets used by the driver (these should
 *	          be allocated dynamically when this becomes possible).
 *  'enFreeq'     is the queue of available packets
 *  'enState'     is the driver state table per logical unit number
 *  'enUnit'  	  is the physical unit number table per logical unit number;
 *		  the first "attach"ed ethernet is logical unit 0, etc.
 *  'enFreeqMin'  is the minimum number of packets ever in the free queue
 *		  (for statistics purposes)
 *  'enAllocCount'is the number of packets allocated to external kernel
 *		  applications
 *  'enScavenges' is the number of scavenges of the active input queues
 *		  (for statustics purposes)
 *  'enDebug'	  is a collection of debugging bits which enable trace and/or
 *		  diagnostic output as defined above (ENDBG_*)
 *  'enUnits'	  is the number of attached units
 *  'enOneCopy'   if true, then no packet is delivered to more than one minor
 *		  device
 */
struct enPacket	enQueueElts[ENPACKETS];
struct enQueue	enFreeq;
struct enState  enState[NENET];
int		enFreeqMin = ENPACKETS;
int		enAllocCount = 0;
int		enScavenges = 0;
int		enDebug = ENDBG_ABNORM;
int		enUnits = 0;
int		enOneCopy = 0;

/*
 *  Forward declarations for subroutines which return other
 *  than integer types.
 */
extern boolean enDoFilter();


/*
 * Linkages to if_en.c
 */

struct enet_info {
	struct	ifnet *ifp;	/* which ifp for output */
} enet_info[NENET];

struct sockaddr enetaf = { AF_IMPLINK };


/****************************************************************
 *								*
 *		Various Macros & Routines			*
 *								*
 ****************************************************************/

/*
 *  forAllOpenDescriptors(p) -- a macro for iterating
 *  over all currently open devices.  Use it in place of
 *      "for ( ...; ... ; ... )"
 *  and supply your own loop body.  The loop variable is the
 *  parameter p which is set to point to the descriptor for
 *  each open device in turn.
 */

#define forAllOpenDescriptors(p)					\
	for ((p) = (struct enOpenDescriptor *)enDesq.enQ_F;		\
	      (struct Queue *)(&enDesq) != &((p)->enOD_Link);		\
	      (p) = (struct enOpenDescriptor *)(p)->enOD_Link.F)

/*
 *  enInitQueue - initialize ethernet queue
 */

#define	enInitQueue(q)							\
{									\
	initQueue((struct Queue *)(q));					\
	(q)->enQ_NumQueued = 0;						\
}



/*
 *  enEnqueue - add an element to a queue
 */

#define	enEnqueue(q, elt)						\
{									\
	enQueue((struct Queue *)(q), (struct Queue *)(elt));		\
	(q)->enQ_NumQueued++;						\
}



#ifdef	UNDEF
/*
 *  enFlushQueue - release all packets from queue, freeing any
 *  whose reference counts drop to 0.  Assumes caller
 *  is at high IPL so that queue will not be modified while
 *  it is being flushed.
 */

enFlushQueue(q)
register struct enQueue *q;
{

    register struct enPacket *qelt;

    while((qelt=(struct enPacket *)deQueue((struct Queue *)q)) != NULL)
    {
	if (0 == --(qelt->enP_RefCount))
	{
	    enEnqueue(&enFreeq, qelt);
	}
    }
    q->enQ_NumQueued = 0;

}
#endif	UNDEF

/*
 *  enInitWaitQueue - initialize an empty packet wait queue
 */

enInitWaitQueue(wq)
register struct enWaitQueue *wq;
{			

    wq->enWQ_Head = 0;
    wq->enWQ_Tail = 0;
    wq->enWQ_NumQueued = 0;
    wq->enWQ_MaxWaiting = ENDEFWAITING;

}



/*
 *  enEnWaitQueue - add a packet to a wait queue
 */

enEnWaitQueue(wq, p)
register struct enWaitQueue *wq;
struct enPacket *p;
{

    wq->enWQ_Packets[wq->enWQ_Tail] = p;
    wq->enWQ_NumQueued++;
    enNextWaitQueueIndex(wq->enWQ_Tail);

}



/*
 *  enDeWaitQueue - remove a packet from a wait queue
 */

struct enPacket *
enDeWaitQueue(wq)
register struct enWaitQueue *wq;
{

    struct enPacket *p;

    if (wq->enWQ_NumQueued == 0)
	panic("enDeWaitQueue"); 
    wq->enWQ_NumQueued--;
    p = wq->enWQ_Packets[wq->enWQ_Head];
    enNextWaitQueueIndex(wq->enWQ_Head);

    return(p);

}



/*
 *  enTrimWaitQueue - cut a wait queue back to size
 */
enTrimWaitQueue(wq, threshold)
register struct enWaitQueue *wq;
{

    register int Counter = (wq->enWQ_NumQueued - threshold);
    register struct enPacket *p;

#ifdef	DEBUG
    enprintf(ENDBG_SCAV)
    		("enTrimWaitQueue(%x, %d): %d\n", wq, threshold, Counter);
#endif
    while (Counter-- > 0)
    {
	wq->enWQ_NumQueued--;
	enPrevWaitQueueIndex(wq->enWQ_Tail);
	p = wq->enWQ_Packets[wq->enWQ_Tail];
	if (0 == --(p->enP_RefCount))
	{
	    m_freem(p->enP_mbuf);
	    enEnqueue(&enFreeq, p);
	}
    }
}



/*
 *  enFlushWaitQueue - remove all packets from wait queue
 */

#define	enFlushWaitQueue(wq)	enTrimWaitQueue(wq, 0)

/*
 *  scavenging thresholds:
 *
 *  index by number of active files;  for N open files, each queue may retain
 *  up to 1/Nth of the packets not guaranteed to be freed on scavenge.  The
 *  total number of available packets is computed less any which are currently
 *  allocated to other kernel applications and one for sending.
 *
 *  (assumes high IPL)
 */
char enScavLevel[(NENET*ENMAXOPENS)+1];

/*
 *  enInitScavenge -- set up ScavLevel table
 */
enInitScavenge()
{
    register int PoolSize = (ENPACKETS-enAllocCount-ENMINSCAVENGE);
    register int i = (NENET*ENMAXOPENS)+1;

    PoolSize--;		/* leave one for transmitter */
    while (--i>0)
        enScavLevel[i] = (PoolSize / i);
}

/*
 *  enScavenge -- scan all OpenDescriptors for all ethernets, releasing
 *    any queued buffers beyond the prescribed limit and freeing any whose
 *    refcounts drop to 0.
 *    Assumes caller is at high IPL so that it is safe to modify the queues.
 */
enScavenge()
{

    register struct enOpenDescriptor *d;
    register int threshold = 0;
    register struct enState *enStatep;

    for (enStatep=enState; enStatep < &enState[NENET]; enStatep++)
	threshold += enCurOpens;
    threshold = enScavLevel[threshold];

    /* recalculate thresholds based on current allocations */
    enInitScavenge();

    enScavenges++;
#ifdef	DEBUG
    enprintf(ENDBG_SCAV)("enScavenge: %d\n", threshold);
#endif
    for (enStatep=enState; enStatep < &enState[NENET]; enStatep++)
    {
	if (enDesq.enQ_F == 0)
	    continue;			/* never initialized */
	forAllOpenDescriptors(d)
	{
	    enTrimWaitQueue(&(d->enOD_Waiting), threshold);
	}
    }

}



/*
 *  enAllocatePacket - allocate the next packet from the free list
 *
 *  Assumes IPL is at high priority so that it is safe to touch the
 *  packet queue.  If the queue is currently empty, scavenge for
 *  more packets.
 */

struct enPacket *
enAllocatePacket()
{

    register struct enPacket *p;

    if (0 == enFreeq.enQ_NumQueued)
	enScavenge();
    p = (struct enPacket *)deQueue((struct Queue *)&enFreeq);
    if (p == NULL)
	panic("enAllocatePacket");
    if (enFreeqMin > --enFreeq.enQ_NumQueued)
	enFreeqMin = enFreeq.enQ_NumQueued;

    p->enP_RefCount = 0;	/* just in case */

    return(p);

}



/*
 *  enDeallocatePacket - place the packet back on the free packet queue
 *
 *  (High IPL assumed).
 */

#define	enDeallocatePacket(p)						\
{									\
	if (p->enP_RefCount) panic("enDeallocatePacket: refcount != 0");\
	enQueue((struct Queue *)&enFreeq, (struct Queue *)(p));		\
	enFreeq.enQ_NumQueued++;					\
}

/****************************************************************
 *								*
 *	    Routines to move uio data to/from mbufs		*
 *								*
 ****************************************************************/

/*
 * These two routines were inspired by/stolen from ../sys/uipc_socket.c
 *	   Both return error code (or 0 if success).
 */

/*
 * read: return contents of mbufs to user.  DO NOT free them, since
 *	there may be multiple claims on the packet!
 */
enrmove(m, uio, count)
register struct mbuf *m;
register struct uio *uio;
register int count;
{
	register int len;
	register int error = 0;
	
	count = min(count, uio->uio_resid);	/* # of bytes to return */
	
	while ((count > 0) && m && (error == 0)) {
	    len = min(count, m->m_len);	/* length of this transfer */
	    count -= len;
	    error = uiomove(mtod(m, caddr_t), (int)len, UIO_READ, uio);
	    
	    m = m->m_next;
	}

	return(error);
}

enwmove(uio, mbufp)
register struct uio *uio;
register struct mbuf **mbufp;	/* top mbuf is returned by reference */
{
	struct mbuf *mtop = 0;
	register struct mbuf *m;
	register struct mbuf **mp = &mtop;
	register struct iovec *iov;
	register int len;
	int error = 0;
	
	while ((uio->uio_resid > 0) && (error == 0)) {
	    iov = uio->uio_iov;
	    
	    if (iov->iov_len == 0) {
	    	uio->uio_iov++;
		uio->uio_iovcnt--;
		if (uio->uio_iovcnt < 0)
		{
		    printf(
		    	"uio_iovcnt %d < 0 while uio_resid %d > 0 - ",
				uio->uio_iovcnt, uio->uio_resid);
		    panic("enwmove");
		}
	    }
	    MGET(m, M_WAIT, MT_DATA);
	    if (m == NULL) {
	    	error = ENOBUFS;
		break;
	    }
	    if (iov->iov_len >= CLBYTES) {	/* big enough to use a page */
	    	register struct mbuf *p;
		MCLGET(p, 1);
		if (p == 0)
		    goto nopages;
		m->m_off = (int)p - (int)m;
		len = CLBYTES;
	    }
	    else {
nopages:
		len = MIN(MLEN, iov->iov_len);
	    }
	    error = uiomove(mtod(m, caddr_t), len, UIO_WRITE, uio);
	    m->m_len = len;
	    *mp = m;
	    mp = &(m->m_next);
	}

	if (error) {		/* probably uiomove fouled up */
	    if (mtop)
		m_freem(mtop);
	}
	else {
	    *mbufp = mtop;	/* return ptr to top mbuf */
	}

	return(error);
}

/*
 *  enetopen - open ether net device
 *
 *  Callable from user or kernel mode.  Bit ENKERNEL of the flag argument
 *  indicates whether or not the open is being done by the kernel.  This
 *  bit is remembered for later use by read, write, and ioctl.
 *
 *  Errors:	ENXIO	- illegal minor device number
 *		EBUSY	- minor device already in use
 */

/* ARGSUSED */
enetopen(dev, flag)
{
    register int md = ENINDEX(dev);
    register struct enState *enStatep;

    /*
     * Each open enet file has a different minor device number.
     * In general, a prospective net user must try successively
     * to open the devices 'enet', 'enet1', ... 'enetn' where n
     * is MAXOPENS-1.  This is not elegant, but UNIX will call
     * open for each new open file using the same inode but calls
     * close only when the last open file referring to the inode 
     * is released. This means that we cannot know inside the
     * driver code when the resources associated with a particular
     * open of the same inode should be deallocated.  Thus, we have
     * simply made up a number of different inodes to represent the
     * simultaneous opens of the ethernet.  Each of these has
     * a different minor device number.
     *
     * When opening an ethernet device in the kernel, simply iterate
     * through minor device numbers until an open fails to return
     * EBUSY.  
     */

#ifdef	DEBUG
    enprintf(ENDBG_TRACE)("enetopen(%d, %x):\n", md, flag);
#endif
    /*
     *  This driver is a compatibility mode feature for 4.1 mode only.
     */
    if (setcompat("enet open", 0) == 0)
	return(u.u_error);

    /* check for illegal minor dev */
    if (md >= NENET*ENMAXOPENS		/* minor dev too high */
	|| (((u_char)minor(dev)>>ENINDEXBITS) >= enUnits) /* bad unit */
        || (enet_info[ENUNIT(dev)].ifp == 0)		/* ifp not known */
	|| ((enet_info[ENUNIT(dev)].ifp->if_flags & IFF_UP) == 0) )
					/* or if down */
    {
	return(ENXIO);
    }
    enStatep = &enState[ENUNIT(dev)];
    if (enOpenFlag[md])
    {
	return(EBUSY);
    }
    enOpenFlag[md] = TRUE;

    enprintf(ENDBG_DESQ)
    	("enetopen: Desq: %x, %x\n", enDesq.enQ_F, enDesq.enQ_B);
    enInitDescriptor(&enAllDescriptors[md], flag);
    enInsertDescriptor(&(enDesq), &enAllDescriptors[md]);

    return(0);
}


/*
 *  enInit - intialize ethernet unit (called by enetattach)
 */

enInit(enStatep)
register struct enState *enStatep;
{

#ifdef	DEBUG
    enprintf(ENDBG_INIT)("enInit(%x):\n", enStatep);
#endif

    /*  initialize free queue if not already done  */
    if (enFreeq.enQ_F == 0)
    {
	register int i;

	initQueue((struct Queue *)&enFreeq);
	for (i=0; i<ENPACKETS; i++)
	{
	    register struct enPacket *p;

	    p = &enQueueElts[i];
	    p->enP_Func = NULL;
	    p->enP_RefCount = 0;
	    enDeallocatePacket(p);
	}
    }
    initQueue((struct Queue *)&enDesq);	/* init descriptor queue */

}        

/*
 *  enetclose - ether net device close routine
 *
 *  Callable from user or kernel mode.
 */

/* ARGSUSED */
enetclose(dev, flag)
{
    register int md = ENINDEX(dev);
    register struct enState *enStatep = &enState[ENUNIT(dev)];
    register struct enOpenDescriptor *d = &enAllDescriptors[md];
    int ipl;

    enOpenFlag[md] = FALSE;      

#ifdef	DEBUG
    enprintf(ENDBG_TRACE)("enetclose(%d, %x):\n", md, flag);
#endif

    /*
     *  insure that receiver doesn't try to queue something
     *  for the device as we are decommissioning it.
     *  (I don't think this is necessary, but I'm a coward.)
     */
    ipl = splenet();
    deQueue((struct Queue *)d->enOD_Link.B);
    enCurOpens--;
    enprintf(ENDBG_DESQ)
    		("enetclose: Desq: %x, %x\n", enDesq.enQ_F, enDesq.enQ_B);
    enFlushWaitQueue(&(d->enOD_Waiting));
    splx(ipl);

}

/*
 *  enetread - read next packet from net
 *
 *  Callable from user or kernel mode (checks OD_Flag)
 */

/* VARARGS */
enetread(dev, uio)
dev_t dev;
register struct uio *uio;
{

    register struct enState *enStatep = &enState[ENUNIT(dev)];
    register struct enOpenDescriptor *d = &enAllDescriptors[ENINDEX(dev)];
    register struct enPacket *p;
    int ipl;
    int error;
    extern enTimeout();

#if	DEBUG
    enprintf(ENDBG_TRACE)("enetread(%x):", dev);
#endif

    ipl = splenet();
    /*
     *  If nothing is on the queue of packets waiting for
     *  this open enet file, then set timer and sleep until
     *  either the timeout has occurred or a packet has
     *  arrived.
     */

    while (0 == d->enOD_Waiting.enWQ_NumQueued)
    {
	if (d->enOD_Timeout < 0)
	{
	    splx(ipl);
	    return(0);
	}
        if (d->enOD_Timeout)
	{
	    /*
	     *  If there was a previous timeout pending for this file,
	     *  cancel it before setting another.  This is necessary since
	     *  a cancel after the sleep might never happen if the read is
	     *  interrupted by a signal.
	     */
	    if (d->enOD_RecvState == ENRECVTIMING)
		untimeout(enTimeout, (caddr_t)d);
            timeout(enTimeout, (caddr_t)d, (int)d->enOD_Timeout);
            d->enOD_RecvState = ENRECVTIMING;
	}
        else
            d->enOD_RecvState = ENRECVIDLE;

        sleep((caddr_t)d, PRINET);

        switch (d->enOD_RecvState)
	{
            case ENRECVTIMING:
	    {
                untimeout(enTimeout, (caddr_t)d);
                d->enOD_RecvState = ENRECVIDLE;
		break;
	    }
            case ENRECVTIMEDOUT:
	    {
                splx(ipl);
		return(0);
	    }
	}
    }

    p = enDeWaitQueue(&(d->enOD_Waiting));
    splx(ipl);

    /*
     * Kernel mode read
     *
     * Return pointer to packet.  It must be subsequently freed via an
     * EIOCDEALLOCP ioctl() call and may not be changed since it may
     * have also been on some other wait queue.
     *
     * (we don't use any fields of the U area since we can't guarantee
     * our context).
     */
    if (d->enOD_Flag & ENKERNEL)
    {
	enAllocCount++;		/* packet allocated to kernel */
	*(struct enPacket **)uio = p;
	return(0);
    }

    /*  
     * User mode read
     *
     * Move data from packet into user space.
     */
    error = enrmove(p->enP_mbuf, uio, p->enP_ByteCount);

    ipl = splenet();
    if (0 == --(p->enP_RefCount))	/* if no more claims on this packet */
    {
	m_freem(p->enP_mbuf);	/* release mbuf */
	enDeallocatePacket(p);	/* and packet */
    }
    splx(ipl);

    return(error);
}



/*
 *  enTimeout - process ethernet read timeout
 */

enTimeout(d)
register struct enOpenDescriptor * d;
{
    register int ipl;

#ifdef	DEBUG
    enprintf(ENDBG_TRACE)("enTimeout(%x):\n", d);
#endif
    ipl = splenet();
    d->enOD_RecvState = ENRECVTIMEDOUT;
    wakeup((caddr_t)d);
    enetwakeup(d);
    splx(ipl);

}

/*
 *  enetwrite - write next packet to net
 *
 *  Callable from user or kernel mode (checks OD_Flag).
 */

int enKludgeSleep[NENET];	/* Are we sleeping on IF_QFULL? */
				/*  really, # of procs sleeping on IF_QFULL */

/* VARARGS */
enetwrite(dev, uio)
dev_t dev;
register struct uio *uio;
{
    register struct enState *enStatep = &enState[ENUNIT(dev)];
    register struct enOpenDescriptor *d = &enAllDescriptors[ENINDEX(dev)];
    register struct enPacket *p;
    register struct ifnet *ifp = enet_info[ENUNIT(dev)].ifp;
    int ipl;
    int error;
    int sleepcount;
    int enKludgeTime();

#if	DEBUG
    enprintf(ENDBG_TRACE)("enetwrite(%x):\n", dev);
#endif

    /*
     * User mode write
     *
     * Allocate packet for transmit.
     *
     * Copy user data into packet (if ubaremap() handled user page tables
     * we might be able to get away with direct mapping without copying here).
     */
    if ((d->enOD_Flag&ENKERNEL) == 0)
    {
	if (uio->uio_resid == 0)
	    return(0);
	ipl = splenet();
	p = enAllocatePacket();
	p->enP_RefCount++;
	splx(ipl);

	if (uio->uio_resid > ifp->if_mtu) {	/* too large */
	    ipl = splenet();
	    p->enP_RefCount--;
	    enDeallocatePacket(p);
	    splx(ipl);
	    return(EMSGSIZE);
	}

	p->enP_ByteCount = uio->uio_resid;

	if (error = enwmove(uio, &(p->enP_mbuf))) {
	    ipl = splenet();
	    p->enP_RefCount--;
	    enDeallocatePacket(p);
	    splx(ipl);
	    return(error);
	}

	p->enP_Data = mtod(p->enP_mbuf, u_short *);
		/* get address of beginning of data area */
    }
    else
    /*
     * Kernel mode write
     *
     * Use packet supplied by caller.
     *
     * (Again, we avoid using any fields of the U area since we may be called
     * in interrupt context).
     */
    {
	p = (struct enPacket *)uio;
    }

    ipl = splenet();
    /*
     * if the queue is full, and write is not kernel mode,
     * hang around until there's room or until process is interrupted
     */
    sleepcount = 0;
    while (IF_QFULL(&(ifp->if_snd)) && ((d->enOD_Flag&ENKERNEL) == 0)) {
	extern int hz;
	if (sleepcount++ > 2) {	/* don't sleep too long */
	    p->enP_RefCount--;
	    m_freem(p->enP_mbuf);
	    enDeallocatePacket(p);
	    splx(ipl);
	    return(ETIMEDOUT);
	}
	/* if nobody else has a timeout pending for this unit, set one */
	if (enKludgeSleep[ENUNIT(dev)] == 0)
	    timeout(enKludgeTime, (caddr_t)ENUNIT(dev), 2 * hz);
	enKludgeSleep[ENUNIT(dev)]++;	/* record that we are sleeping */
	if (setjmp(&u.u_qsave)) {
	    /* sleep (following) was interrupted, clean up */
#if	DEBUG
	    enprintf(ENDBG_ABNORM)
	    	("enetwrite(%x): enet%d sleep %d interrupted\n", dev,
			ENUNIT(dev), enKludgeSleep[ENUNIT(dev)]);
#endif	DEBUG
	    enKludgeSleep[ENUNIT(dev)]--;	/* we're no longer sleeping */
	    p->enP_RefCount--;
	    m_freem(p->enP_mbuf);
	    enDeallocatePacket(p);
	    splx(ipl);
	    return(EINTR);
	}
	sleep((caddr_t)&(enKludgeSleep[ENUNIT(dev)]), PRINET);
	enKludgeSleep[ENUNIT(dev)]--;	/* we are no longer sleeping */
    }
    
    /* place mbuf chain on outgoing queue & start if necessary */
    error = (*ifp->if_output)(ifp, p->enP_mbuf, &enetaf);
    					/* returns error number */
	/* this always frees the mbuf */
    enXcnt++;
    
    if (p->enP_Func != NULL)		/* kernel mode, presumably */
        (*(p->enP_Func))(p, ENUNIT(dev));
    else if (--(p->enP_RefCount) == 0) {
    	enDeallocatePacket(p);
    }
    else
    {
        printf("packet alloc count %d should be 0 - ", p->enP_RefCount);
        panic("enetwrite");
    }
    splx(ipl);
    
    return(error);
}

enKludgeTime(unit)
int unit;
{
	/* XXX perhaps we should always wakeup? */
	if (enKludgeSleep[unit]) {
		wakeup((caddr_t)&(enKludgeSleep[unit]));
		/* XXX should we restart transmitter? */
	}
}

/*
 *  enetioctl - ether net control
 *
 *  Callable from user or kernel mode (checks OD_Flag)
 *
 *  EIOCGETP	 - get ethernet parameters
 *  EIOCSETP	 - set ethernet read timeout
 *  EIOCSETF	 - set ethernet read filter
 *  EIOCENBS	 - enable signal when read packet available
 *  EIOCINHS     - inhibit signal when read packet available
 *  FIONREAD	 - check for read packet available
 *  EIOCSETW	 - set maximum read packet waiting queue length
 *  EIOCFLUSH	 - flush read packet waiting queue
 *  EIOCALLOCP	 - allocate packet (kernel only)
 *  EIOCDEALLOCP - deallocate packet (kernel only)
 *  EIOCMBIS	 - set mode bits
 *  EIOCMBIC	 - clear mode bits
 *  ECIODEVP	 - get device parameters
 *
 * Note: the following two are obsolete! should use bcopy or something.
 *  encopyin()  copies data from caller to driver based on mode of open
 *  encopyout() copies data from driver to caller based on mode of open
 */

#define encopyin(from, to, size)	bcopy(from, to, size)
#define	encopyinOLD(from, to, size)					\
	if (d->enOD_Flag&ENKERNEL)					\
		bcopy(from, to, size);					\
	else								\
		if (copyin(from, to, size))				\
		{							\
			return(EFAULT);					\
		}

#define	encopyout(from, to, size)	bcopy(from, to, size)
#define	encopyoutOLD(from, to, size)					\
	if (d->enOD_Flag&ENKERNEL)					\
		bcopy(from, to, size);					\
	else								\
		if (copyout(from, to, size))				\
		{							\
			return(EFAULT);					\
		}

/* ARGSUSED */
enetioctl(dev, cmd, addr, flag)
caddr_t addr;
dev_t flag;
{

    register struct enState *enStatep = &enState[ENUNIT(dev)];
    register struct enOpenDescriptor * d = &enAllDescriptors[ENINDEX(dev)];
    int ipl;

#if	DEBUG
    enprintf(ENDBG_TRACE)
	    	("enetioctl(%x, %x, %x, %x):\n", dev, cmd, addr, flag);
#endif

    switch (cmd)
    {
	case EIOCGETP:
	{
            struct eniocb t;

	    t.en_maxwaiting = ENMAXWAITING;
	    t.en_maxpriority = (ENINDEX(dev) < ENHIPRIDEV)
	    				? (ENHIPRI-1)
					: ENMAXPRI;
            t.en_rtout = d->enOD_Timeout;
	    t.en_addr = enDevParams.end_addr[0];
	    t.en_maxfilters = ENMAXFILTERS;

	    bcopy((caddr_t)&t, addr, sizeof t);
	}
        endcase

        case EIOCSETP:
	{
            struct eniocb t;

            bcopy(addr, (caddr_t)&t, sizeof t);
            d->enOD_Timeout = t.en_rtout;
	}
        endcase

        case EIOCSETF:
	{
            struct enfilter f;
	    unsigned short *fp;

            bcopy(addr, (caddr_t)&f, sizeof f);
	    if ((ENINDEX(dev) < ENHIPRIDEV && f.enf_Priority >= ENHIPRI) ||
		f.enf_FilterLen > ENMAXFILTERS)
	    {
		return(EINVAL);
	    }
            /* insure that filter is installed indivisibly */
            ipl = splenet();
            bcopy((caddr_t)&f, (caddr_t)&(d->enOD_OpenFilter), sizeof f);
	    /* pre-compute length of filter */
	    fp = &(d->enOD_OpenFilter.enf_Filter[0]);
	    d->enOD_FiltEnd = &(fp[d->enOD_OpenFilter.enf_FilterLen]);
	    d->enOD_RecvCount = 0;	/* reset count when filter changes */
	    deQueue((struct Queue *)d->enOD_Link.B);
	    enDesq.enQ_NumQueued--;
	    enInsertDescriptor(&(enDesq), d);
            splx(ipl);
	}
        endcase

	/*
	 *  Enable signal n on input packet
	 */
	case EIOCENBS:
	{
            union {int n;  int (*func)();} un;

	    bcopy(addr, (caddr_t)&un, sizeof un);
	    if ((d->enOD_Flag & ENKERNEL) == 0)
	    {
		if (un.n < NSIG)
		{
		    d->enOD_SigProc = u.u_procp;
		    d->enOD_SigPid  = u.u_procp->p_pid;
		    d->enOD_SigNumb = un.n;	/* This must be set last */
		}
		else
		{
		    goto bad;
		}
	    }
	    else
	    {
		d->enOD_SigFunc = un.func;
		d->enOD_SigNumb = NSIG;		/* This must be set last */
	    }
	}
	endcase

	/*
	 *  Disable signal on input packet
	 */
	case EIOCINHS:
	{
	    if ((d->enOD_Flag & ENKERNEL) == 0)
	    {
		d->enOD_SigNumb = 0;
	    }
	    else
	    {
		d->enOD_SigFunc = NULL;
	    }
	}
	endcase

	/*
	 *  Check for packet waiting
	 */
	case FIONREAD:
	{
            int n;
            register struct enWaitQueue *wq;

	    ipl = splenet();
	    if ((wq = &(d->enOD_Waiting))->enWQ_NumQueued)
		n = wq->enWQ_Packets[wq->enWQ_Head]->enP_ByteCount;
	    else
		n = 0;
	    splx(ipl);
	    bcopy((caddr_t)&n, addr, sizeof n);
	}
	endcase

	/*
	 *  Set maximum recv queue length for a device
	 */
	case EIOCSETW:
	{
            unsigned un;

	    bcopy(addr, (caddr_t)&un, sizeof un);
	    /*
             *  unsigned un         MaxQueued
             * ----------------    ------------
             *  0               ->  DEFWAITING
	     *  1..MAXWAITING   ->  un
	     *  MAXWAITING..-1  ->  MAXWAITING
             */
	    d->enOD_Waiting.enWQ_MaxWaiting = (un) ? min(un, ENMAXWAITING)
                                        : ENDEFWAITING;
	}
	endcase

	/*
	 *  Flush all packets queued for a device
	 */
	case EIOCFLUSH:
	{
	    ipl = splenet();
	    enFlushWaitQueue(&(d->enOD_Waiting));
	    splx(ipl);
	}
	endcase

	/*
	 *  Set mode bits
	 */
	case EIOCMBIS:
	{
	    u_short mode;

	    bcopy(addr, (caddr_t)&mode, sizeof mode);
	    if (mode&ENPRIVMODES)
		return(EINVAL);
	    else
		d->enOD_Flag |= mode;
	}
	endcase

	/*
	 *  Clear mode bits
	 */
	case EIOCMBIC:
	{
	    u_short mode;

	    bcopy(addr, (caddr_t)&mode, sizeof mode);
	    if (mode&ENPRIVMODES)
		return(EINVAL);
	    else
		d->enOD_Flag &= ~mode;
	}
	endcase

	/*
	 *  Allocate an ethernet packet (kernel only)
	 */
	case EIOCALLOCP:
	{
	    register struct enPacket *p;

	    if ((d->enOD_Flag&ENKERNEL) == 0)
		goto bad;
	    ipl = splenet();
	    p = enAllocatePacket();
	    p->enP_RefCount++;
	    enAllocCount++;
	    splx(ipl);
	    *(struct enPacket **)addr = p;
	}
	endcase

	/*
	 *  Deallocate an ethernet packet (kernel only)
	 */
	case EIOCDEALLOCP:
 	{
	    register struct enPacket *p;

	    if ((d->enOD_Flag & ENKERNEL) == 0)
		goto bad;
	    p = *(struct enPacket **)addr;
	    ipl = splenet();
	    enAllocCount--;
	    if (--(p->enP_RefCount) == 0)
		enDeallocatePacket(p);
	    splx(ipl);
	}
	endcase

	/*
	 * Return hardware-specific device parameters.
	 */
	case EIOCDEVP:
	{
	    bcopy((caddr_t)&(enDevParams), addr, sizeof(struct endevp));
	}
	endcase;

        default:
	{
	bad:
	    return(EINVAL);
	}
    }

    return(0);

}
				
/****************************************************************
 *								*
 *		Support for select() system call		*
 *								*
 *	Other hooks in:						*
 *		enInitDescriptor()				*
 *		enInputDone()					*
 *		enTimeout()					*
 ****************************************************************/
/*
 * inspired by the code in tty.c for the same purpose.
 */

/*
 * enetselect - returns true iff the specific operation
 *	will not block indefinitely.  Otherwise, return
 *	false but make a note that a selwakeup() must be done.
 */
enetselect(dev, rw)
register dev_t dev;
int rw;
{
	register struct enState *enStatep;
	register struct enOpenDescriptor *d;
	register int ipl;
	register int avail;
	
	switch (rw) {
	
	case FREAD:
		/*
		 * an imitation of the FIONREAD ioctl code
		 */
		enStatep = &(enState[ENUNIT(dev)]);
		d = &(enAllDescriptors[ENINDEX(dev)]);
		
		ipl = splenet();
		if ((&(d->enOD_Waiting))->enWQ_NumQueued)
			avail = 1;	/* at least one packet queued */
		else {
			avail = 0;	/* sorry, nothing queued now */
			/*
			 * If there's already a select() waiting on this
			 * minor device then this is a collision.
			 * [This shouldn't happen because enet minors
			 * really should not be shared, but if a process
			 * forks while one of these is open, it is possible
			 * that both processes could select() us.]
			 */
			if (d->enOD_SelProc
			     && d->enOD_SelProc->p_wchan == (caddr_t)&selwait)
			     	d->enOD_SelColl = 1;
			else
				d->enOD_SelProc = u.u_procp;		
		}
		splx(ipl);	
		return(avail);

	case FWRITE:
		/*
		 * since the queueing for output is shared not just with
		 * the other enet devices but also with the IP system,
		 * we can't predict what would happen on a subsequent
		 * write.  However, since we presume that all writes
		 * complete eventually, and probably fairly fast, we
		 * pretend that select() is true.
		 */
		return(1);

	default:		/* hmmm. */
		return(1);		/* don't block in select() */
	}
}

enetwakeup(d)
register struct enOpenDescriptor *d;
{
	if (d->enOD_SelProc) {
		selwakeup(d->enOD_SelProc, d->enOD_SelColl);
		d->enOD_SelColl = 0;
		d->enOD_SelProc = 0;
	}
}

/*
 * enetFilter - incoming linkage from ../vaxif/if_en.c
 */

enetFilter(en, m, count)
register int en;
register struct mbuf *m;
register int count;
{

    register struct enState *enStatep = &enState[en];
    register struct enPacket *p;
    register int pullcount;	/* bytes, not words */
    int s = splenet();

#if	DEBUG
    enprintf(ENDBG_TRACE)("enetFilter(%d):\n", en);
#endif

    p = enAllocatePacket();	/* panics if not possible */

    p->enP_ByteCount = count;
    
    pullcount = min(MLEN, count);	/* largest possible first mbuf */
    if (m->m_len < pullcount) {
    	/* first mbuf not as full as it could be - fix this */
	if ((m = m_pullup(m, pullcount)) == 0) {
	    /* evidently no resources; bloody m_pullup discarded mbuf */
	    enDeallocatePacket(p);
	    enRdrops++;
	    goto out;
	}
    }
    
    p->enP_mbuf = m;
    p->enP_Data = mtod(m, u_short *);

    enInputDone(enStatep, p);
out:
    splx(s);
}

/*
 * enInputDone - process correctly received packet
 */

enInputDone(enStatep, p)
register struct enState *enStatep;
register struct enPacket *p;
{
    register struct enOpenDescriptor *d;
    int queued = 0;
    register int maxword;
    register unsigned long rcount;
    register struct enOpenDescriptor *prevd;

#if	INNERDEBUG
    enprintf(ENDBG_TRACE)("enInputDone(%x): %x\n", enStatep, p);
#endif
    /* precompute highest possible word offset */
    /* can't address beyond end of packet or end of first mbuf */
    maxword = (min(p->enP_ByteCount, p->enP_mbuf->m_len)>>1);

    forAllOpenDescriptors(d)
    {
	if (enDoFilter(p, d, maxword))
	{
            if (d->enOD_Waiting.enWQ_NumQueued < d->enOD_Waiting.enWQ_MaxWaiting)
	    {
                enEnWaitQueue(&(d->enOD_Waiting), p);
                p->enP_RefCount++;
		queued++;
                wakeup((caddr_t)d);
		enetwakeup(d);
#if	INNERDEBUG
                enprintf(ENDBG_TRACE)("enInputDone: queued\n");
#endif
	    }
	    /*  send notification when input packet received  */
	    if (d->enOD_SigNumb)
	    {
		if (d->enOD_SigNumb < NSIG)
		{
		    if (d->enOD_SigProc->p_pid == d->enOD_SigPid)
			psignal(d->enOD_SigProc, d->enOD_SigNumb);
		    if ((d->enOD_Flag & ENHOLDSIG) == 0)
			d->enOD_SigNumb = 0;		/* disable signal */
		}
		else
		{
		    register int enunit;
		    
		    enunit = enStatep - enState;	/* find unit # */

		    (*(d->enOD_SigFunc))(enunit);
		}
	    }
	    rcount = ++(d->enOD_RecvCount);
	    
	    /* see if ordering of filters is wrong */
	    if (d->enOD_OpenFilter.enf_Priority >= ENHIPRI) {
	    	prevd = (struct enOpenDescriptor *)d->enOD_Link.B;
		/*
		 * If d is not the first element on the queue, and
		 * the previous element is at equal priority but has
		 * a lower count, then promote d to be in front of prevd.
		 */
		if (((struct Queue *)prevd != &(enDesq.enQ_Head)) &&
	    	    (d->enOD_OpenFilter.enf_Priority ==
				prevd->enOD_OpenFilter.enf_Priority)) {
		    /* threshold difference to avoid thrashing */
		    if ((100 + prevd->enOD_RecvCount) < rcount) {
			enReorderQueue(&(prevd->enOD_Link), &(d->enOD_Link));
		    }
		}
		break;	/* high-priority filter => no more deliveries */
	    }
	    else if (enOneCopy)
		break;
	}
    }
    if (queued == 0)			/* this buffer no longer in use */
    {
	m_freem(p->enP_mbuf);			/* free mbuf */
	enDeallocatePacket(p);			/*  and packet */
	enRdrops++;
    }
    else
	enRcnt++;

}

#define	opx(i)	(i>>ENF_NBPA)

boolean
enDoFilter(p, d, maxword)
struct enPacket *p;
struct enOpenDescriptor *d;
register int maxword;
{

    register unsigned short *sp;
    register unsigned short *fp;
    register unsigned short *fpe;
    register unsigned op;
    register unsigned arg;
    unsigned short stack[ENMAXFILTERS+1];
    struct fw {unsigned arg:ENF_NBPA, op:ENF_NBPO;};

#ifdef	INNERDEBUG
    enprintf(ENDBG_TRACE)("enDoFilter(%x,%x):\n", p, d);
#endif
    sp = &stack[ENMAXFILTERS];
    fp = &d->enOD_OpenFilter.enf_Filter[0];
    fpe = d->enOD_FiltEnd;
	/* ^ is really: fpe = &fp[d->enOD_OpenFilter.enf_FilterLen]; */
    *sp = TRUE;

    for (; fp < fpe; )
    {
	op = ((struct fw *)fp)->op;
	arg = ((struct fw *)fp)->arg;
	fp++;
	switch (arg)
	{
	    default:
	    	arg -= ENF_PUSHWORD;
		/*
		 * This next test is a little bogus; since arg
		 * is unsigned, it is always >= 0 (the compiler
		 * knows this and emits no code).  If arg were
		 * less than ENF_PUSHWORD before the subtract,
		 * it is certaintly going to be more than maxword
		 * afterward, so the code does work "right"
		 */
		if (/*(arg >= 0) && */(arg < maxword))
		    *--sp = p->enP_Data[arg];
		else
		{
#ifdef	INNERDEBUG
		    enprintf(ENDBG_TRACE)("=>0(len)\n");
#endif
		    return(false);
		}
		break;
	    case ENF_PUSHLIT:
		*--sp = *fp++;
		break;
	    case ENF_PUSHZERO:
		*--sp = 0;
	    case ENF_NOPUSH:
		break;
	}
	if (sp < &stack[2])	/* check stack overflow: small yellow zone */
	{
	    enprintf(ENDBG_TRACE)("=>0(--sp)\n");
	    return(false);
	}
	if (op == ENF_NOP)
	    continue;
	/*
	 * all non-NOP operators binary, must have at least two operands
	 * on stack to evaluate.
	 */
	if (sp > &stack[ENMAXFILTERS-2])
	{
	    enprintf(ENDBG_TRACE)("=>0(sp++)\n");
	    return(false);
	}
	arg = *sp++;
	switch (op)
	{
	    default:
#ifdef	INNERDEBUG
		enprintf(ENDBG_TRACE)("=>0(def)\n");
#endif
		return(false);
	    case opx(ENF_AND):
		*sp &= arg;
		break;
	    case opx(ENF_OR):
		*sp |= arg;
		break;
	    case opx(ENF_XOR):
		*sp ^= arg;
		break;
	    case opx(ENF_EQ):
		*sp = (*sp == arg);
		break;
	    case opx(ENF_NEQ):
		*sp = (*sp != arg);
		break;
	    case opx(ENF_LT):
		*sp = (*sp < arg);
		break;
	    case opx(ENF_LE):
		*sp = (*sp <= arg);
		break;
	    case opx(ENF_GT):
		*sp = (*sp > arg);
		break;
	    case opx(ENF_GE):
		*sp = (*sp >= arg);
		break;

	    /* short-circuit operators */

	    case opx(ENF_COR):
	    	if (*sp++ == arg) {
#ifdef	INNERDEBUG
		    enprintf(ENDBG_TRACE)("=>COR %x\n", *sp);
#endif
		    return(true);
		}
		break;
	    case opx(ENF_CAND):
	    	if (*sp++ != arg) {
#ifdef	INNERDEBUG
		    enprintf(ENDBG_TRACE)("=>CAND %x\n", *sp);
#endif
		    return(false);
		}
		break;
	    case opx(ENF_CNOR):
	    	if (*sp++ == arg) {
#ifdef	INNERDEBUG
		    enprintf(ENDBG_TRACE)("=>COR %x\n", *sp);
#endif
		    return(false);
		}
		break;
	    case opx(ENF_CNAND):
	    	if (*sp++ != arg) {
#ifdef	INNERDEBUG
		    enprintf(ENDBG_TRACE)("=>CAND %x\n", *sp);
#endif
		    return(true);
		}
		break;
	}
    }
#ifdef	INNERDEBUG
    enprintf(ENDBG_TRACE)("=>%x\n", *sp);
#endif
    return((boolean)*sp);

}

enInitDescriptor(d, flag)
register struct enOpenDescriptor *d;
{

#if	DEBUG
    enprintf(ENDBG_TRACE)("enInitDescriptor(%x):\n", d);
#endif
    d->enOD_RecvState = ENRECVIDLE;
    d->enOD_OpenFilter.enf_FilterLen = 0;
    d->enOD_OpenFilter.enf_Priority = 0;
    d->enOD_FiltEnd = &(d->enOD_OpenFilter.enf_Filter[0]);
    d->enOD_RecvCount = 0;
    d->enOD_Timeout = 0;
    d->enOD_SigNumb = 0;
    d->enOD_Flag = (flag&ENKERNEL);
    d->enOD_SelColl = 0;
    d->enOD_SelProc = 0;		/* probably unnecessary */
    /*
     * Remember the PID that opened us, at least until some process
     * sets a signal for this minor device
     */
    if ((flag&ENKERNEL) == 0) {
	d->enOD_SigPid = u.u_procp->p_pid;
    }
    enInitWaitQueue(&(d->enOD_Waiting));
#if	DEBUG
    enprintf(ENDBG_TRACE)("=>eninitdescriptor\n");
#endif

}

/*
 *  enInsertDescriptor - insert open descriptor in queue ordered by priority
 */

enInsertDescriptor(q, d)
register struct enQueue *q;
register struct enOpenDescriptor *d;
{
    struct enOpenDescriptor * nxt;
    register int ipl;

    ipl = splenet();
    nxt = (struct enOpenDescriptor *)q->enQ_F;
    while ((struct Queue *)q != &(nxt->enOD_Link))
    {
        if (d->enOD_OpenFilter.enf_Priority > nxt->enOD_OpenFilter.enf_Priority)
	    break;
        nxt = (struct enOpenDescriptor *)nxt->enOD_Link.F;
    }
    enQueue((struct Queue *)&(nxt->enOD_Link),(struct Queue *)&(d->enOD_Link));
    enprintf(ENDBG_DESQ)("enID: Desq: %x, %x\n", q->enQ_F, q->enQ_B);
    q->enQ_NumQueued++;
    splx(ipl);

}

/*
 * enReorderQueue - swap order of two elements in queue
 *	assumed to be called at splenet
 */
enReorderQueue(first, last)
register struct Queue *first;
register struct Queue *last;
{
	register struct Queue *prev;
	register struct Queue *next;

	enprintf(ENDBG_DESQ)("enReorderQ: %x, %x\n", first, last);
	
	/* get pointers to other queue elements */
	prev = first->B;
	next = last->F;
	
	/*
	 * no more reading from queue elements; this ensures that
	 * the code works even if there are fewer than 4 elements
	 * in the queue.
	 */

	prev->F = last;
	next->B = first;
	
	last->B = prev;
	last->F = first;
	
	first->F = next;
	first->B = last;
}

enetattach(ifp, devp)
struct ifnet *ifp;
struct endevp *devp;
{
    register struct enState *enStatep = &(enState[enUnits]);

#ifdef	DEBUG
    enprintf(ENDBG_INIT) ("enetattach: type %d, addr ", devp->end_dev_type);
    if (enDebug&ENDBG_INIT) {
	register int i;
	for (i = 0; i < devp->end_addr_len; i++)
	    printf("%o ", devp->end_addr[i]);
	printf("\n");
    }
#endif	DEBUG

    enet_info[enUnits].ifp = ifp;

    bcopy((caddr_t)devp, (caddr_t)&(enDevParams), sizeof(struct endevp));

    enInit(enStatep);
    
    return(enUnits++);
}

#endif	(NENET > 0)
