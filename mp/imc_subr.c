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
 *	File:	imc_subr.c
 *	Author:	Avadis Tevanian, Jr.
 *
 *	Inter-Machine Communication (IMC).
 *
 *	Copyright (C) 1985, Avadis Tevanian, Jr.
 *
 * HISTORY
 * 22-Mar-86  Bill Bolosky (bolosky) at Carnegie-Mellon University
 *	Changed includes of ../vax/* to ../machine/*
 *
 * 11-Oct-85  Robert V Baron (rvb) at Carnegie-Mellon University
 *	Doesnot use MPS any more
 *
 * 16-Jan-85  Avadis Tevanian (avie) at Carnegie-Mellon University
 *	Created.
 *
 ***********************************************************************
 *
 *	This module provides for very simple intermachine communication
 *	between processes executing on different processors.  There is
 *	fixed set of ports to which any process may send messages.
 *
 */

#include "wb_ml.h"
#include "mach_mp.h"
#include "mach_mpm.h"

#include "../accent/acc_errors.h"

#include "../h/map.h"
#include "../h/param.h"
#include "../h/dir.h"
#include "../h/user.h"
#include "../h/proc.h"
#include "../machine/pte.h"

#include "../machine/cpu.h"

#include "../sync/mp_queue.h"
#include "../mp/remote_prim.h"
#include "../mp/remote_sw.h"

#include "../mp/imc.h"


char	imc_buffers[IMC_PORTS][IMC_BUFFERSIZE];	/* imc buffers */
long	imc_sender[IMC_PORTS];		/* senders */
long	imc_len[IMC_PORTS];		/* lengths */
long	imc_valid[IMC_PORTS];		/* valid flags */

int	wait_channels[IMC_PORTS];		/* channels waiting (local) */

/*
 *	get_cpu_number(cpu) returns the current cpu number to the
 *	calling process.
 */

get_cpu_number(cpu)
int *cpu;
{
	if (suword(cpu, cpu_number()))
		return(NotAUserAddress);
	else
		return(Success);
}

/*
 *	send_imc_msg(cpu, port, buf, len) sends an imc message to the
 *	specified port on the specified cpu.
 */

send_imc_msg(cpu, port, buf, len)
register cpu, port, len;
register caddr_t buf;
{
	register caddr_t port_buf;

	if ((port < 0) || (port > (IMC_PORTS - 1)))
		return(WrongArgs);

	if ((len < 0) || (len > IMC_BUFFERSIZE))
		return(WrongArgs);
	port_buf = imc_buffers[port];		/* get buffer */

	if (copyin(buf, port_buf, len))
		return(NotAUserAddress);

	imc_sender[port] = cpu_number();
	imc_len[port] = len;
	imc_valid[port] = 1;

	/* notify the other processor */

	request_processor(cpu, TYPE_IMC, port, 0, 0, 0);
	return(Success);
}

/*
 *	receive_imc_msg(cpu, port, buf, len) receives an IMC message
 *	on the specified port, saving the data at the address specified
 *	by buf.  The cpu that sent the message is returned along with
 *	the length of the packet.
 */

receive_imc_msg(cpu, port, buf, len)
register *cpu, port, *len;
register caddr_t buf;
{
	register caddr_t port_buf;
	register mylen, wake_reason;

	if ((port < 0) || (port > (IMC_PORTS - 1)))
		return(WrongArgs);

	while (!imc_valid[port]) {
		wait_channels[port] = 1;
		wake_reason = asleep(&wait_channels[port], PIPC, 0);
		if (wake_reason == -1) {
			wait_channels[port] = 0;
			return(EINTR);
		}
	}

	port_buf = imc_buffers[port];		/* get buffer */
	mylen = imc_len[port];			/* get length */
	imc_valid[port] = 0;

	if ((mylen < 0) || (mylen > IMC_BUFFERSIZE))
		return(-1);

	if (copyout(port_buf, buf, mylen))
		return(NotAUserAddress);

	if (suword(cpu, imc_sender[port]))
		return(NotAUserAddress);

	if (suword(len, mylen))
		return(NotAUserAddress);

	return(Success);
}

handle_remote_imc(msg)
register struct msg_buffer *msg;
{
	register port;

	/* Extract the parameters contained in the message */

	port = msg->param1;

	/* check validity of parameters */

	if ((port < 0) || (port > (IMC_PORTS - 1)))
		return;

	if (wait_channels[port])		/* anyone waiting? */
		wakeup(&wait_channels[port]);	/* wake him up */
}
