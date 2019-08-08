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
/*	syscmu.c	CMU	82/06/22	*/

/*
 **********************************************************************
 * HISTORY
 * 23-Mar-86  Bill Bolosky (bolosky) at Carnegie-Mellon University
 *	Removed includes of ../vax/mtpr.h and ../vax/reg.h.
 *
 * 30-Aug-85  Mike Accetta (mja) at Carnegie-Mellon University
 *	Added U_TTYD table.
 *	[V1(1)]
 *
 * 30-Mar-83  Mike Accetta (mja) at Carnegie-Mellon University
 *	Added table() system call.  We only need this to retrieve the
 *	terminal location information at the moment but the interface
 *	has been designed with a general purpose mechanism in mind.
 *	It may still need some work (V3.06h).
 *
 * 22-Jun-82  Mike Accetta (mja) at Carnegie-Mellon University
 *	Created (V3.05a).
 *
 **********************************************************************
 */

#include "cs_ttyloc.h"
#include "cs_syscall.h"

#if	CS_SYSCALL

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/dir.h"
#include "../h/user.h"
#include "../h/buf.h"
#include "../h/inode.h"
#include "../h/file.h"
#include "../h/proc.h"
#include "../h/conf.h"
#include "../h/ioctl.h"
#include "../h/tty.h"
#include "../h/table.h"



/*
 *  Set account ID of process.
 */

setaid()
{
    register struct a
    {
	int aid;
    } *uap;

    uap = (struct a *)u.u_ap;
    if (suser())
    {
	u.u_aid = uap->aid;
    }
}



/*
 *  Get account ID of process.
 */

getaid()
{
    u.u_r.r_val1 = u.u_aid;
}



/*
 *  Set process modes.
 */

setmodes()
{
    register struct a
    {
	int modes;
    } *uap;
    int modes = UMODE_SETGROUPS;

    uap = (struct a *)u.u_ap;
    /*
     *  Only super-user may set new directory mode but anyone may clear.
     */
    if (u.u_uid != 0 && (u.u_modes&UMODE_NEWDIR) == 0)
	modes |= UMODE_NEWDIR;
    u.u_modes = (u.u_modes&UMODE_SETGROUPS) | (uap->modes&~modes);
}



/*
 *  Get process modes.
 */

getmodes()
{
    u.u_r.r_val1 = (u_char)u.u_modes;
}



/*
 *  table - get element(s) from system table
 *
 *  This call is intended as a general purpose mechanism for returning
 *  individual or sequential elements of various system tables and
 *  data structures.  At the moment, we only need it for the terminal
 *  location information and future uses are not clear.  One possible
 *  use might be to make most of the standard system system tables
 *  available via this mechanism so as to permit non-privileged programs
 *  to access these common SYSTAT types of data.
 *
 *  Parameters:
 *
 *  id		= an identifer indicating the table in question
 *  index	= an index into this table specifying the starting
 *		  position at which to begin the data copy
 *  addr	= address in user space to receive the data
 *  nel		= number of table elements to retrieve
 *  lel		= expected size of a single element of the table.  If this
 *		  is smaller than the actual size, extra data will be
 *		  truncated from the end.  If it is larger, holes will be
 *		  left between elements copied into the user address space.
 *
 *		  The intent of separately distinguishing these final two
 *		  arguments is to insulate user programs as much as possible
 *		  from the common change in the size of system data structures
 *		  when a new field is added.  This works so long as new fields
 *		  are added only to the end, none are removed, and all fields
 *		  remain a fixed size.
 *
 *  Returns:
 *
 *  val1	= number of elements retrieved (this may be fewer than
 *		  requested if more elements are requested than exist in
 *		  the table from the specified index).
 *
 *  Note:
 *
 *  A call with lel == 0 and nel == MAXSHORT can be used to determine the
 *  length of a table (in elements) before actually requesting any of the
 *  data.
 */

table()
{
    register struct a
    {
	int id;
	int index;
	caddr_t addr;
	u_int nel;
	u_int lel;
    } *uap = (struct a *)u.u_ap;
    register caddr_t from;
    register unsigned size;
    int error;

    u.u_r.r_val1 = 0;
    while (uap->nel != 0)
    {
	dev_t nottyd = -1;

	switch (uap->id)
	{
#if	CS_TTYLOC
	    case TBL_TTYLOC:
	    {
		int majdev, mindev;
		extern int ntty;
		extern int *nttysw[];

		majdev = major(uap->index);
		mindev = minor(uap->index);
		if (majdev >= ntty || mindev >= *(nttysw[majdev]))
		    goto bad;
		from = (caddr_t)&((&(cdevsw[majdev].d_ttys)[mindev])->t_ttyloc);
		size = sizeof (struct ttyloc);
		break;
	    }
#endif	CS_TTYLOC
	    case TBL_U_TTYD:
	    {
		if (uap->index != u.u_procp->p_pid && uap->index != 0)
		    goto bad;
		if (u.u_ttyp)
		    from = (caddr_t)&u.u_ttyd;
	        else
		    from = (caddr_t)&nottyd;
		size = sizeof (u.u_ttyd);
		break;
	    }
	    default:
	    bad:
		u.u_error = EINVAL;
		return;
	}
	/*
	 *  This code should be generalized if/when other tables are added
	 *  to handle single element copies where the actual and expected
	 *  sizes differ or the table entries are not contiguous in kernel
	 *  memory (as with TTYLOC) and also efficiently copy multiple element
	 *  tables when contiguous and the sizes match.
	 */
	size = MIN(size, uap->lel);
	if (size && (error=copyout(from, uap->addr, size)))
	{
	    u.u_error = error;
	    return;
	}
	uap->addr += uap->lel;
	uap->nel -= 1;
	uap->index += 1;
	u.u_r.r_val1 += 1;
    }
}
#endif	CS_SYSCALL
