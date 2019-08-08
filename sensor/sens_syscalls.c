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
static char rcsid[] = "$Header: local_syscalls.c V1.1 85/07/28 17:18:21 duncans Exp $";

/* local_syscalls.c -- syslocal, monitor, Wraparound */
#include "wb_sens.h"

#include "../h/param.h"
#include "../h/dir.h"
#include "../h/systm.h"
#include "../h/user.h"
#include "../h/proc.h"

#include "../sensor/mondefs.h"
#include "../sensor/montypes.h"
#include "../sensor/kernel.h"


/*
 *---------------------------------------------------------------------
 * This file should contain all system calls that use syslocal.
 * Each local routine should 
 *	- be of type static to prevent interference with the
 *	  rest of the system.
 *	- have its global variables and defines with a unique prefix
 *	- have #ifdefs to control its compilation in the system
 *---------------------------------------------------------------------
 */

#if 	NWB_SENS > 0
/*
 * Declarations for the monitor system call.
 */

#define MON_OFLOWRECSIZE	sizeof(mon_errrec) /* In chars		  */
#define MON_ENABLEVECTORSIZE	12      /* Chars in enable vector	  */
#define MON_REQLISTSIZE		256     /* No. of entries in request list */
#define MON_REQOPENSLOT		0	/* Marks open slot in req. list   */
#define MON_SUPERUSERUID	0	/* uid of root			  */
#define MON_BADPID		-1	/* For marking accountant_pid	  */
#define CALLERID u.u_procp->p_pid       /* process id of caller		*/
#if	MON_ASSERT
#define mon_assert(a,b)		if (a) panic(b)
#else	MON_ASSERT
#define mon_assert(a,b)
#endif	MON_ASSERT

/*
 *  monitor global ring buffer variables,
 *  initialized at compile time and when
 *  sensors are turned OFF
 */

u_char	*mon_write_ptr,		/* Write pointer in mon_eventvector 	 */
       	*mon_read_ptr,		/* Read pointer in mon_eventvector  	 */
       	*mon_eventvector_end;	/* First pos after buffer, start of appx */
int    	mon_eventvector_count;	/* No. of chars of valid event records in mon_eventvector */
int	mon_semaphore = 0;	/* Used to detect concurrancy	    	 */
int     mon_oflow_count = 0;	/* Event record overflow	    	 */
u_short	mon_enablevector[MON_ENABLEVECTORSIZE];
				/* enable flags for sensors		 */

/*
 * Local variables for monitor
 */

u_char	mon_eventvector[MON_EVENTVECSIZE + MON_EVENTRECSIZE];
					/* Event record ring buffer	*/
mon_errrec	mon_oflowrec = {	/* Buffer full indicator	*/
    {MONOP_OFLOW,MON_OFLOWRECSIZE/2},	/* struct mon_cmd		*/
    0 };				/* count			*/
mon_errrec	mon_noreq = {		/* Err rec for no req in queue	*/
    {MONOP_NO_REQ, MON_OFLOWRECSIZE/2}, /* struct mon_cmd		*/
    0 };				/* pid				*/
int     mon_initflag = FALSE;		/* initialize one time only	*/
int     mon_accountant_pid = MON_BADPID; /* identity of accountant	*/
struct	mon_request mon_requests[MON_REQLISTSIZE]; /* request list for users */
/*
 *---------------------------------------------------------------------
 * monitor --
 *    The purpose of this system call is to allow communication
 *  between sensors in target programs and a monitoring process.
 *  Sensors send event records to and retrieve commands from
 *  monitor while the ACCOUNTANT sends commands and retrieves event
 *  records.
 * 
 *  Written by Dave Doerner for the Monitor project (CS145) 5/2/83
 *  Modified by Stephen Duncan as part of MS project,
 *    Changed data buffer to a circular queue
 *    Changed to utilize structs in buffer
 *    Revamped much of the code: mnemonics, flow inside cases
 *----------------------------------------------------------------------
 */

static
monitor (buffer)               		/* SYSTEM CALL */
struct mon_cmd *buffer;			/* Address of command */
{
    mon_command  u_command; 		/* Receives command 		*/
    mon_command	*u_cmd_ptr = &u_command;

    u_char	*Wraparound();		/* handles ring buffer wraparound */

    int     i, j;			/* loop temporaries		*/
    int     cmd_length;			/* length of command in chars	*/
    int     notzero,			/* booleans			*/
            match;

    mon_printf (("********SYSMON CALLED************\n"));
    if (mon_semaphore)			/* concurrency check		*/
    {
	/*
	 * A concurrency error has occurred.
	 * - Turn off the kernel sensors.
	 * - Return an error.
	 * Note that until the error passes, no data can be read from
	 *	the event vector, or added to it by PUTEVENTS.  This is
	 *	to try and minimize problems with the pointers.  This
	 *	gets cleared only when and if mon_semaphore is
	 *	appropriately decremented.  If two kernel sensors
	 *	caused the concurrency, it will never clear, only
	 *	rebooting will help then.  Since this indicates buggy
	 *	code, the code with sensors should be recompilied with
	 *	the -DMON_ASSERT option.
	 */
	for (i = 0; i < MON_ENABLEVECTORSIZE; i++)
	    mon_enablevector[i] = 0;	/* turn off sensors		*/
	u.u_r.r_val1 = MON_CONCURRENCY_ERR;
	return;
    }
    else mon_semaphore++;		/* begin critical section	*/

    u.u_r.r_val1 = 0;			/* return val is initially 0	*/

    /*
     * Copy in command
     *	first copy in struct that starts command to determine length
     *	then copy in whole command overtop of the struct for that length
     *	Since the type of command isn't known yet, the union version
     *	of a command is used.  This prevents any alignment problems that
     *	might occur with the structs.
     */
    mon_printf (("buffer: 0x%x\n", (int) buffer));
    if ( copyin ((caddr_t)buffer, (caddr_t)u_cmd_ptr, sizeof(struct mon_cmd)) )
    {
        u.u_error = EFAULT;
	u.u_r.r_val1 = MON_SYS_ERR;	/* signifies system error */
	mon_semaphore--;		/* exit critical section  */
        return;
    }

    mon_printf (("command = %d\n", u_cmd_ptr->u_event.cmd.type));
    cmd_length = (int)u_cmd_ptr->u_event.cmd.length * 2;
					/* copyin deals in chars */
    mon_printf (("length = %d\n", cmd_length));

    if (cmd_length > MON_EVENTRECSIZE)
    {
	u.u_error = EINVAL;		/* invalid argument to system call */
	u.u_r.r_val1 = MON_SYS_ERR;	/* signifies system error	   */
	mon_semaphore--;		/* exit critical section	   */
	return;
    }

    if ( copyin((caddr_t)buffer, (caddr_t) u_cmd_ptr, cmd_length) )
    {
        u.u_error = EFAULT;
	u.u_r.r_val1 = MON_SYS_ERR;	/* signifies system error */
	mon_semaphore--;		/* exit critical section  */
        return;
    }
    mon_printf (("length = %d\n", u_cmd_ptr->u_event.cmd.length));

    mon_printf(("Right before switch: oflow = %d, noreqflag = %d\n",
	mon_oflow_count, mon_noreqflag));

    /*
     * This switch is the driver of the system call.  Each case
     * corresponds to a command.  A pointer to a specific type
     * of command is cast to the generic command for each case.
     * The command structure and return values are command dependent.
     */
    switch (u_cmd_ptr->u_event.cmd.type)
    {
    case MONOP_INIT: 
    /*
     * Initialization
     *	The request and event record vectors are initialized.
     *	Can only be called once before a shutdown, the caller
     *  becomes the accountant.  If called a second time,
     *  MON_ALRDY_INIT is returned.
     *	Normally returns the size of the event vector in chars.
     */
        if (mon_initflag)          /* must already be initialized    */
        {
            u.u_r.r_val1 = MON_ALRDY_INIT;
            mon_printf (("Already Initialized\n"));
            break;
        }
	/*
	 * Set up pointers and counters for event vector
	 */
        mon_write_ptr	      = mon_eventvector;
        mon_read_ptr	      = mon_eventvector;
        mon_eventvector_end   = &mon_eventvector[MON_EVENTVECSIZE];
        mon_eventvector_count = 0;
        mon_oflow_count	      = 0;

        mon_initflag	   = TRUE;	/* records whether initialized */
        mon_accountant_pid = CALLERID;	/* initializer is accountant   */
        mon_printf (("Accountant is %d\n", CALLERID));
	/*
	 * Turn off kernel sensors, just in case
	 */
        for (i = 0; i < MON_ENABLEVECTORSIZE; i++)
	    mon_enablevector[i] = 0;
	/*
	 * Initialize request vector to all entries open
	 */
        for (i = 0; i < MON_REQLISTSIZE; i++)
            mon_requests[0].eventnumber = MON_REQOPENSLOT;
        mon_printf (("Initialization done: oflow = %d, noreqflag = %d\n",
		 mon_oflow_count,
		 mon_noreqflag));
        u.u_r.r_val1 = MON_EVENTVECSIZE;	/* return size of ring buffer */
        break;


    case MONOP_PUTEVENT_INT: 
    case MONOP_PUTEVENT_EXT: 
    /*
     * Write event record in vector
     *	Performer and timestamp fields are filled in and
     *  the record is put into the event vector.
     *	Returns 0 if the put was successful,
     *		MON_NOT_INIT if before the initialization,
     *		MON_BUF_FULL if no room in the vector.
     */
        if (!(mon_initflag))            /* not initialized */
        {
            u.u_r.r_val1 = MON_NOT_INIT;
            mon_printf (("Not Initialized\n"));
            break;
        }
        else
        {
            register mon_putevent *pevt;

	    /*
	     * fill in certain fields of event record
	     */
	    pevt = (mon_putevent *) u_cmd_ptr;
            pevt->performer = (short) CALLERID; /* fill in pid */
            pevt->timestamp = (long) (time.tv_sec << 15 | time.tv_usec >> 5);
                                    /* Generate time stamp */

            mon_printf (("Time stamp taken   time= %d\n", pevt->timestamp));
            if ((mon_eventvector_count + cmd_length < MON_EVENTVECSIZE - MON_OFLOWRECSIZE)
		&& !mon_oflow_count)
            {
		/*
		 * Still room in ring buffer
		 *	copy in the event record, point to the next opening,
		 *	handle the wraparound condition.
		 */
		register mon_putevent* pevt_ptr = (mon_putevent *)mon_write_ptr;

		*pevt_ptr	= *(mon_putevent *)u_cmd_ptr;
		mon_write_ptr  += cmd_length;
		if (mon_write_ptr >= mon_eventvector_end)
		    mon_write_ptr = Wraparound(mon_write_ptr);
                mon_eventvector_count += cmd_length;
            }  /* if */
            else 
            {
		/*
		 * We ran out of room in the buffer --
		 *	set a flag so that an error record will
		 *	be put in at GETEVENTS
		 */
	    	mon_oflow_count++;
                u.u_r.r_val1 = MON_BUFF_FULL;
                mon_printf (("OVERFLOW:   vecptr = %D \n",
                    (mon_write_ptr - mon_eventvector)));
                break;
            }  /* else we overflowed*/

            mon_printf (("vecptr= %d \n", (mon_write_ptr - mon_eventvector)));
            u.u_r.r_val1 = 0;
            break;
        }  /* else we are initialized */

    case MONOP_GETEVENTS: 
    /*
     * Read all event records in vector.
     * Copies event records into buffer specified by acct_buf_ptr.
     * Accountant only.
     * Returns number of chars written out when successful,
     *		MON_NOT_INIT if called before initialization
     *		MON_NOT_ACCT if called is not accountant or superuser
     * Handles writing of error records.
     * Must handle four cases:
     *		1) wraparound and event count > requested
     *		2) wraparound and event count > requested
     *		3) no wraparound and event count !> requested
     *		4) no wraparound and event count !> requested
     * The count vs request will be handled first,
     * then the presence or absence of wraparound.
     */

        if (!(mon_initflag))		/* not initialized    */
        {
            u.u_r.r_val1 = MON_NOT_INIT;
            mon_printf (("Not Initialized\n"));
	    break;
        }

        else if ( !(CALLERID == mon_accountant_pid
		|| u.u_uid == MON_SUPERUSERUID) )
	{
            u.u_r.r_val1 = MON_NOT_ACCTNT;
	    break;
	}

        else				/* all ok, proceed */
        {
            register mon_getevent *gevt = (mon_getevent *) u_cmd_ptr;
					/* command is a get event    */
            register int      req_length = gevt->req_length * 2;
					/* requested length in chars */
	    register caddr_t  acct_buf_ptr  = (caddr_t)(gevt->acct_buf_ptr);
					/* ptr to accountants buffer */
            register int      char_count;
					/* char count to write out   */
	    int               transfer_count;
					/* counts chars to transfer  */

            mon_printf (("Read out of vector to Accountant\n"));

	    if (mon_oflow_count != 0)	/* ran out of room before call */
	    {
		/*
		 * Copy in error record
		 *	treat it just like an event record
		 *	guaranteed room for it
		 */
		register mon_errrec *w = (mon_errrec *)mon_write_ptr;

		*w = mon_oflowrec;
		w->val = mon_oflow_count;
		mon_write_ptr += sizeof(mon_errrec);
		mon_eventvector_count += sizeof(mon_errrec);
		if (mon_write_ptr >= mon_eventvector_end)
		    mon_write_ptr = Wraparound(mon_write_ptr);
		if (mon_write_ptr >= mon_eventvector_end
		    || mon_write_ptr < mon_eventvector)
		    panic("monitor: GET_EVENTS: err recd: pointers invalid\n");
                mon_oflow_count = 0;
            }

    /*
     * First, calculate the number of chars to write out.
     * This handles the cases relating to the size of the
     * accountant's buffer.
     */
            if (mon_eventvector_count > req_length)
	    {
		/*
		 * Determine length of events that will fit in
		 * req_length integrally
		 */
		register u_char *p;			/* ptr into vector    */
		register int    accum_length = 0;	/* accumulated length */
		register int	short_count = req_length / 2;
							/* decremented length */

		mon_printf(("GET_EVENT: ->length = %d, req_length = %d\n",
			((struct mon_cmd *)mon_read_ptr)->length,
			short_count));

		/*
		 * while room left, reduce room and point to next recd
		 *	accumulate length in accum_length
		 *	remember to handle wraparound
		 */
		for (p = mon_read_ptr;
		     short_count > ((struct mon_cmd *)p)->length;
		     short_count -= ((struct mon_cmd *)p)->length
		    )
		{
		    accum_length += ((struct mon_cmd *)p)->length; /* shorts */
		    mon_printf(("GET_EVENT: p->len=%d, alen=%d, scount=%d \n",
			((struct mon_cmd *)p)->length,
			accum_length,
			short_count));
		    p += ((struct mon_cmd *)p)->length * 2;	   /* u_chars */
		    if (p >= mon_eventvector_end)   p -= MON_EVENTVECSIZE;
		}
		char_count = accum_length * 2;

		/* 
		 * char_count now has length of whole
		 * recds that will fit in request
		 */

	    } /* if less requested than available */
	    else /* more requested than available */
		char_count = mon_eventvector_count;

    /*
     * Time to copy out to the user area.
     * This handles the cases relating to wraparound.
     */
	    /* 
	     * transfer_count is amount actually transferred in a
	     * given invocation of copyout.
	     * transfer_count is set to the size in chars of logical
	     * older part of the ring buffer only when there is
	     * wraparound physically starting at mon_read_ptr.
	     */

	    mon_printf(("IN GET_EVENT: wrap = %d, out = %d, req = %d\n",
		(mon_eventvector_end - mon_read_ptr),
		char_count,
		req_length));
	    mon_printf(("Writer=%d reader=%d\n",
		(mon_write_ptr - mon_eventvector),
		(mon_read_ptr - mon_eventvector)));
	    if (mon_write_ptr < mon_read_ptr)
            {
		mon_printf(("In Wrap.\n"));
		/*
		 * Write out all or portion of older part
		 */
	        transfer_count =
		    ((mon_eventvector_end - mon_read_ptr) > char_count) ?
		    char_count  :  (mon_eventvector_end - mon_read_ptr);
                if (copyout ((caddr_t) mon_read_ptr,
                             (acct_buf_ptr),
                             transfer_count) < 0)
		{
                    u.u_error = EFAULT;
		    u.u_r.r_val1 = MON_SYS_ERR;
		    break;
		}
                else	/* copy succeeded */
                {
		    /*
		     * Adjust pointers and counts,
		     * do sanity checks
		     */
		    mon_assert( (transfer_count < 0),
			"monitor: GET_EVENT: wrap value invalid");
		    mon_read_ptr   += transfer_count;
                    if (mon_read_ptr >= mon_eventvector_end)
			mon_read_ptr -= MON_EVENTVECSIZE;
                    mon_eventvector_count    -= transfer_count;
		    mon_assert( (mon_eventvector_count < 0),
			"monitor: GET_EVENT: wrap: vec count invalid\n");
		    char_count   -= transfer_count;
		    mon_assert( (char_count < 0),
			"monitor: GET_EVENT: wrap: char_count invalid\n");
		    u.u_r.r_val1 += transfer_count;
		    acct_buf_ptr  += transfer_count;	/* adjust pointer for next copy */
                }
            }
	    mon_printf(("GET_EVENT: &acctbuf= %d, char_count= %d, r_val1= %d\n",
		acct_buf_ptr,
		char_count,
		u.u_r.r_val1));
            if ( char_count > 0)
	    {
		transfer_count = char_count;
	        if ( copyout ((caddr_t) mon_read_ptr,
                                           acct_buf_ptr,
                                           char_count) < 0)
		    {
			u.u_error    = EFAULT;
			u.u_r.r_val1 = MON_SYS_ERR;
			break;
		    }
                else 
		    {
			/*
			 * Adjust pointers and counts,
			 * do sanity check.
			 * Note that wraparound has already been handled.
			 */
			mon_read_ptr          += char_count;
			mon_eventvector_count -= char_count;
			u.u_r.r_val1          += char_count;
						/* add count from this copy */
			u.u_r.r_val1 /= 2;	/* convert to shorts	*/
			mon_assert( (mon_write_ptr > mon_eventvector_end
			    || mon_write_ptr < mon_eventvector
			    || mon_eventvector_count < 0
			    || char_count < 0),
			    "monitor: GET_EVENT: invalid pointers\n");
		    }
	    }
	    else
	    {
		u.u_r.r_val1 = 0;		/* didn't do anything */
	    }
	}

        break;                      	    /* only break in this case	*/

    case MONOP_PUTREQ: 
    /*
     * Write command
     *	This command is used by the accountant to enable
     *		kernel sensors and to put requests in to user
     *		programs.
     *	A request is stored in the request vector at the first
     *		available slot based on the targetpid.
     *		The accountant uses a 0 target pid to indicate
     *		that the kernel is the target.
     *	Returns 0 if successful
     *		MON_REQ_OFLOW if the request vector is full
     *		MON_NOT_INIT if called before initialization
     *		MON_NOT_ACCTNT if called not the accountant or
     *			superuser.
     */
        if (!(mon_initflag))        /* not initialized    */
        {
            u.u_r.r_val1 = MON_NOT_INIT;
            mon_printf (("Not Initialized\n"));
            break;
        }
        mon_printf (("PID of writer: %d\n",
	    ((mon_putreq *)(u_cmd_ptr))->req.targetpid));
	/*
	 * Only the accountant or the superuser are allowed
	 */
        if (CALLERID == mon_accountant_pid || u.u_uid == MON_SUPERUSERUID)
        {
            register mon_putreq *preq = (mon_putreq *) u_cmd_ptr;

	    if (preq->req.eventnumber <= 0)	/* validate eventnumber */
	    {
		u.u_r.r_val1 = MON_SYS_ERR;
		u.u_error = EINVAL;
                break;
	    }
            mon_printf (("Enabling sensors\n"));
            if ( preq->req.targetpid == 0 )	/* in kernel */
            {
                short pos = preq->req.eventnumber;
                if ( preq->req.enablevalue )
                    mon_enablevector[pos / 16] |= 1 << (pos % 16);
                else
		    mon_enablevector[pos / 16] &= ~(1 << (pos % 16));
		mon_printf(("PUTREQ: enablevector: %d pos: %d enableval: %d",
			mon_enablevector[0] & (1<<(pos%16)),
			pos,
			preq->req.enablevalue));
		mon_printf(("pos/16= %d, pos%%16= %d, &enablevec[pos/16]= %d\n",
			pos/16,
			pos%16,
			&mon_enablevector[pos/16]));
                u.u_r.r_val1 = 0;
            }
            else    /* for user preq->req.targetpid */
            {
                /*
		 * Check for space in array
		 */

                for (i = 0; (i < MON_REQLISTSIZE && 
		    mon_requests[i].eventnumber != MON_REQOPENSLOT); i++)
                        ;
                if (i = MON_REQLISTSIZE)
		/* No room to store command */
                {
                    mon_printf (("Command queue overflow!\n"));
                    u.u_r.r_val1 = MON_REQ_OFLOW;
                }
                else    /* put in new commands for user processes */
                {
                    mon_requests[i] = preq->req;
                    u.u_r.r_val1 = 0;
                }
            }  /* else for user */
        }  /* if CALLERID */
	else  /* not the accountant or superuser */
	{
	    u.u_r.r_val1 = MON_NOT_ACCTNT;
	    mon_printf(("PUTREQ: not the accountant"));
	    return;
	}
        break;

    case MONOP_GETREQ: 
    /*
     * Read a request
     *	The request vector is searched for a request with
     *		a pid that matches the calling program.
     *  If the search is successful, copy the request back
     *		into the struct req in the command,
     *		clear the slot in the vector.
     *  else write an error record into the event vector.
     *	Return 0 if successful,
     *		MON_NOT_INIT if called before initialization,
     *		MON_REQ_NOT_FND if no request is found.
     */
        if (!(mon_initflag))            /* not initialized    */
        {
            u.u_r.r_val1 = MON_NOT_INIT;
            mon_printf (("Not Initialized\n"));
        }
	else
	{
            match = FALSE;
            i = 0;
            while (i < MON_REQLISTSIZE && !match)
            {
                match = ((int) mon_requests[i].targetpid == CALLERID);
                i++;
            }
            if (match)             /* Return command     */
            {
		mon_getreq	*greq = (mon_getreq *)u_cmd_ptr;

                i--;
		greq->req = mon_requests[i];
                mon_requests[i].eventnumber = MON_REQOPENSLOT;
                u.u_r.r_val1 = 0;
            }
else				/* Can't find command     */
{
    mon_printf (("Command not found in queue!\n"));
    if (mon_oflow_count == 0
	    && mon_eventvector_count <
	    (MON_EVENTVECSIZE - 2 * sizeof (mon_errrec))) {
	register    mon_errrec * w = (mon_errrec *) mon_write_ptr;
	*w = mon_noreq;
	w -> val = 0;
	mon_write_ptr += w -> cmd.length * 2;
	if (mon_write_ptr >= mon_eventvector_end)
	    mon_write_ptr = Wraparound (mon_write_ptr);
    }
    else
	mon_oflow_count++;
    u.u_r.r_val1 = MON_REQ_NOT_FND;
}
}
break;

case MONOP_SHUTDOWN: 
 /* 
  * Shut down monitoring
  * The accountant should close down all sensors and call GETEVENTS
  *		until it returns 0 before calling SHUTDOWNS,
  *		otherwise data will be left in the buffer and
  *		will be lost.
  * Closes down all kernel sensors, and prevents any other command
  *		other than init from being executed.
  * Returns 0 when successful,
  *		MON_NOT_INIT if called before initialization,
  *		MON_NOT_ACCTNT if caller is not the accountant or
  *			the superuser
  */
if (u.u_procp -> p_pid == mon_accountant_pid
    || u.u_uid == MON_SUPERUSERUID) {
    if (!(mon_initflag)) {	/* not initialized    */
	u.u_r.r_val1 = MON_NOT_INIT;
	mon_printf (("Not Initialized\n"));
	break;
    }
 /* 
  * close down all sensors
  */
    for (i = 0; i < MON_ENABLEVECTORSIZE; i++)
	mon_enablevector[i] = 0;
    mon_initflag = FALSE;
    mon_printf (("Sensors shut down by Accountant"));
    u.u_r.r_val1 = 0;

 /* 
  * reinitialize for next run
  */
    mon_write_ptr = mon_eventvector;
    mon_read_ptr = mon_eventvector;
    mon_eventvector_end = mon_eventvector + MON_EVENTVECSIZE;
    mon_eventvector_count = 0;
    mon_oflow_count = 0;
}
else
    u.u_r.r_val1 = MON_NOT_ACCTNT;
break;

default: 
u.u_r.r_val1 = MON_INV_CMD;
mon_printf (("\n********INVALID COMMAND****** CMD = %d\n",
	u_cmd_ptr -> u_event.cmd.type));
u.u_error = EINVAL;
break;
}				/* End switch */
mon_printf (("At end of call: mon_oflow_count = %d, mon_noreqflag = %d\n",
	mon_oflow_count,
	mon_noreqflag));
mon_printf (("at end of call: mon_enablevector = %x\n",
	mon_enablevector[0]));
mon_semaphore--;
return;
}

/*
 *--------------------------------------------------------------
 *  Implements wraparound for the ring buffer.  The buffer
 *  has an extra tail equal to the length of the longest rec-
 *  ord, if data is written in here, it is copied onto the
 *  beginning of the file.  The reason for doing this is so
 *  that one doesn't have to check for the end of buffer on
 *  every byte of a record, only at end of write.
 *  It is the responsibility of the user to insure that the
 *  end of the tail is respected.
 *--------------------------------------------------------------
 */

u_char*
Wraparound(reg_ptr)
register u_char *reg_ptr;        /* position in ringbuffer */
{
    register u_char *buf_end=mon_eventvector_end;  /* first pos of tail */
    register u_char *buf_start=mon_eventvector;    /* start of buffer   */

    if (reg_ptr > buf_end + MON_EVENTRECSIZE || reg_ptr < buf_start)
	panic("Wraparound: pointer out of range\n");

    while (  buf_end <= reg_ptr )
        *buf_start++ = *buf_end++;

    if (buf_start > buf_end + MON_EVENTRECSIZE || buf_start < mon_eventvector)
	panic("Wraparound: pointer out of range\n");

    return(buf_start -1);          /* next write position    */
}
#endif	NWB_SENS > 0

