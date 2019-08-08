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
/* $Header: mem.c,v 4.2 85/08/25 19:28:22 chessin Exp $ */
/* $Source: /ibm/acis/usr/sys_ca/ca/RCS/mem.c,v $ */

#ifndef lint
static char *rcsid = "$Header: mem.c,v 4.2 85/08/25 19:28:22 chessin Exp $";
#endif

/*     mem.c   6.1     83/07/29        */

/*
 * Memory special file
 */

#include "../machine/pte.h"
#include "../h/param.h"
#include "../h/dir.h"
#include "../h/user.h"
#include "../h/conf.h"
#include "../h/buf.h"
#include "../h/systm.h"
#include "../h/vm.h"
#include "../h/cmap.h"
#include "../h/uio.h"
#include "../machine/rosetta.h"


mmread(dev, uio)
	dev_t dev;
	struct uio *uio;
{

	return (mmrw(dev, uio, UIO_READ));
}


mmwrite(dev, uio)
	dev_t dev;
	struct uio *uio;
{

	return (mmrw(dev, uio, UIO_WRITE));
}


mmrw(dev, uio, rw)
	dev_t dev;
	struct uio *uio;
	enum uio_rw rw;
{
	register int o;
	register u_int c, v;
	register struct iovec *iov;
	int error = 0;
	struct buf *bp;

	while (uio->uio_resid > 0 && error == 0) {
		iov = uio->uio_iov;
		if (iov->iov_len == 0) {
			uio->uio_iov++;
			uio->uio_iovcnt--;
			if (uio->uio_iovcnt < 0)
				panic("mmrw");
			continue;
		}
		switch (minor(dev)) {

/* minor device 0 is physical memory */
/* Strategy:
 * 1. verify the address is within physical memory (taking into account
 *	the possible existance of a hole in the address space).
 * 2. verity that the end address does not exceed memory
 *     if write, uimove user's data to kernel buffer
 * 3. go to V=R mode (target address cannot be in segment 0 as this is
 *	read-only text (note: should check to make sure that this gets
 *	picked up properly if it is in segment 0)
 * 4. do the copy via uiomove
 * 5. restore previous (presumably non V=R).
 *     if read, uiomove the kernel buffer to the user's area
 */
		case 0:
			v = btop(uio->uio_offset);
			if (v >= endmem || ishole(v))
				goto fault; /* start address too big */
			v = btop(uio->uio_offset + iov->iov_len + -1);
			if (v >= endmem || ishole(v))
				goto fault; /* end address too big */
			c = MIN((u_int)iov->iov_len, MAXBSIZE);
			bp = geteblk(c);
			if (rw == UIO_READ) {
				register int s, x;
				s = spl5(); /* 5??? */
				GET_VR0(x);
				bcopy((caddr_t)uio->uio_offset,
					bp->b_un.b_addr,
					c);
				SET_VR(x); /* restore virtual */
				splx(s);  /* restore prio */
				error = uiomove(bp->b_un.b_addr,
					(int)c, rw, uio);
			} else {
				register int s, x;
				error = uiomove(bp->b_un.b_addr,
					(int)c, rw, uio);
				s = spl5(); /* 5??? */
				GET_VR0(x);
				bcopy(bp->b_un.b_addr,
					(caddr_t)uio->uio_offset, c);
				SET_VR(x); /* restore virtual */
				splx(s);  /* restore prio */
			}
			brelse(bp);
			continue;

/* minor device 1 is kernel memory */
		case 1:
			c = iov->iov_len;
			if (!kernacc((caddr_t)uio->uio_offset,
				c, rw == UIO_READ ? B_READ : B_WRITE))
				goto fault;
			error = uiomove((caddr_t)uio->uio_offset, (int)c, rw, uio);
			continue;

/* minor device 2 is EOF/RATHOLE */
		case 2:
			if (rw == UIO_READ)
				return (0);
			c = iov->iov_len;
			break;

/* minor device 3 is kernel memory, addressed as bytes */
/* minor device 4 is kernel memory, addressed as shorts */
/* minor device 5 is kernel memory, addressed as longs */
		case 3:
		case 4:
		case 5:
			c = iov->iov_len;
			if (!kernacc((caddr_t)uio->uio_offset,
				c, rw == UIO_READ ? B_READ : B_WRITE))
				goto fault;
			if (!useracc(iov->iov_base,
				c, rw == UIO_READ ? B_READ : B_WRITE))
				goto fault;
			error = IOcpy((caddr_t)uio->uio_offset, iov->iov_base,
				(int)c, rw, minor(dev) - 3);
			break;

		default:
			goto fault;
		}
		if (error)
			break;
		iov->iov_base += c;
		iov->iov_len -= c;
		uio->uio_offset += c;
		uio->uio_resid -= c;
	}
	return (error);
fault:
	return (EFAULT);
}

/*
 * Aligned Kernel Address Space <--> User Space transfer
 */
IOcpy(ioadd, usradd, n, rw, log2length)
	caddr_t ioadd, usradd;
	register int n;
	enum uio_rw rw;
	int log2length;
{
	register caddr_t from, to;
	register int length;
	register int mask;

	/*
	 * Check for length and alignment commensurate
	 * with the request
	 */
	mask = 3 >> (2 - log2length);
	if (n & mask || (int) ioadd & mask || (int) usradd & mask)
		return (EINVAL);

	if (rw == UIO_READ) {
		from = ioadd;
		to = usradd;
	} else {
		from = usradd;
		to = ioadd;
	}

	n >>= log2length;
	length = 1 << log2length;

	switch(log2length) {

	case 0:
		for (; n > 0; n--) {
			*(char *)to = *(char *)from;
			to += length;
			from += length;
		}
		break;

	case 1:
		for (; n > 0; n--) {
			*(short *)to = *(short *)from;
			to += length;
			from += length;
		}
		break;

	case 2:
		for (; n > 0; n--) {
			*(long *)to = *(long *)from;
			to += length;
			from += length;
		}
		break;

	default:
		panic("IOcpy");
	}
	return (0);
}
