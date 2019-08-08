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
#ifdef	CMU
/*
 ***********************************************************************
 * HISTORY
 * 27-Mar-86  Bill Bolosky (bolosky) at Carnegie-Mellon University
 *	Wrote copy{,in,out}str routines for romp.  MACH_VM: Turned off
 *	module.
 *
 ***********************************************************************
 */

#include "mach_vm.h"
#endif	CMU

#if	MACH_VM
#else	MACH_VM
/* $Header: vmaccess.c,v 4.0 85/07/15 00:50:24 ibmacis GAMMA $ */
/* $Source: /ibm/acis/usr/sys_ca/ca/RCS/vmaccess.c,v $ */

#ifndef lint
static char *rcsid = "$Header: vmaccess.c,v 4.0 85/07/15 00:50:24 ibmacis GAMMA $";
#endif

/*     vmcopy.c    84/03/30        */

#include "../machine/debug.h"
#include "../machine/pte.h"
#include "../machine/rosetta.h"
#include "../h/types.h"
#include "../h/buf.h"
#include "../h/errno.h"
#include "../machine/machparam.h"

/****************************************************************************
  useracc - verify user access to virtual memory
 ****************************************************************************/
useracc(vaddr, length, rw)
	register caddr_t vaddr;
	register length;
	register rw;
{
	return (isitok(vaddr, length, (rw == B_READ ? UREAD_OK : UWRITE_OK)));
}


/****************************************************************************
  kernacc - verify kernel access to virtual memory
 ****************************************************************************/
kernacc(vaddr, length, rw)
	register caddr_t vaddr;
	register length;
	register rw;
{
	return (isitok(vaddr, length, (rw == B_READ ? KREAD_OK : KWRITE_OK)));
}


/****************************************************************************
  fubyte - fetch byte from user data space
 ****************************************************************************/
fubyte(vaddr)
	register char *vaddr;
{
	return (isitok(vaddr, 1, UREAD_OK) ? *vaddr : -1);
}


/****************************************************************************
  fuibyte - fetch byte from user instruction space
 ****************************************************************************/
asm(".globl _fuibyte");
asm(".set _fuibyte , _fubyte");

/****************************************************************************
  fuword - fetch word from user data space
  if the word is not aligned on a word boundary return error indication
  as most likely the user-level code is using non-aligned memory allocator
  and this will make it easier to find.
 ****************************************************************************/
fuword(vaddr)
	register unsigned *vaddr;
{
	if ((int)vaddr & 3)
		return (-1);
	return (isitok(vaddr, 4, UREAD_OK) ? *vaddr : -1);
}


/****************************************************************************
  fuiword - fetch word from user instruction space
 ****************************************************************************/
asm(".globl _fuiword");
asm(".set _fuiword , _fuword");

/****************************************************************************
  subyte - set byte in user data space
 ****************************************************************************/
subyte(vaddr, value)
	register char *vaddr;
	register char value;
{
	if (isitok(vaddr, 1, UWRITE_OK)) {
		*vaddr = value;
		return (0);
	} else
		return (-1);
}


/****************************************************************************
  suibyte - set byte in user instruction space
 ****************************************************************************/
asm(".globl _suibyte");
asm(".set _suibyte , _subyte");

/****************************************************************************
  suword - set word in user data space
  (see alignment comments in fuword)
 ****************************************************************************/
suword(vaddr, value)
	register unsigned *vaddr;
	register unsigned value;
{
	if ((int)vaddr & 3)
		return (-1);
	if (isitok(vaddr, 4, UWRITE_OK)) {
		*vaddr = value;
		return (0);
	} else
		return (-1);
}


/****************************************************************************
  suiword - set word in user instruction space
 ****************************************************************************/
asm(".globl _suiword");
asm(".set _suiword , _suword");

/****************************************************************************
  copyin - copy from user data space to kernel data space
 ****************************************************************************/
copyin(from, to, length)
	register caddr_t from, to;
	register unsigned length;
{
	if (isitok(from, length, UREAD_OK)) {
		bcopy(from, to, length);
		return (0);
	} else
		return (EFAULT);
}


/****************************************************************************
  copyout - copy from kernel data space to user data space
 ****************************************************************************/
copyout(from, to, length)
	register caddr_t from, to;
	register unsigned length;
{
	if (isitok(to, length, UWRITE_OK)) {
		bcopy(from, to, length);
		return (0);
	} else
		return (EFAULT);
}


/***********************************************************************
 copystr - copy a string subject to maxlength and return length
           in lencopied (if not NULL)
 **********************************************************************/

copystr(fromaddr, toaddr, maxlength, lencopied)
register char *fromaddr, *toaddr;
register int	maxlength, *lencopied;
{
 register int bytes_copied = 0;

 while ((bytes_copied < maxlength) && (*toaddr++ = *fromaddr++))
	bytes_copied++;
 if (lencopied != NULL) *lencopied = bytes_copied + 1;
 if (*(toaddr - 1) == '\0')
 	return(0);
 else
	return(ENOENT);
}

/***********************************************************************
 copyinstr - copy a string from user memory to kernel memory subject
	     to maxlength and user validation.
 ***********************************************************************/

copyinstr(fromaddr, toaddr, maxlength, lencopied)
register char *fromaddr, *toaddr;
register int  maxlength, *lencopied;
{
 register int bytes_copied = 0;
 register char *pg_break;

/* Do up to the first page boundary first. */
 pg_break = fromaddr +  NBPG - ((int) fromaddr % NBPG);
 
 for(;;) {
  if (!isitok(fromaddr, (pg_break - fromaddr) - 1,UREAD_OK)) 
	goto checkBytes;

  while ((fromaddr < pg_break) && (bytes_copied < maxlength) &&
	 (bytes_copied++,*toaddr++ = *fromaddr++))
	;

  if (lencopied != NULL) *lencopied = bytes_copied;

  if (*(toaddr - 1) == '\0') return(0);

  if (!(bytes_copied < maxlength)) {
	return(ENOENT);
  }
  
  pg_break += NBPG;
 } 
/*NOTREACHED*/
checkBytes: while ((isitok(fromaddr, 1,UREAD_OK)) && 
	          (bytes_copied < maxlength) &&
		  (bytes_copied++,*toaddr++ = *fromaddr++))
	    ;
  if (lencopied != NULL) *lencopied = bytes_copied;
  if ((bytes_copied > 0) && (*(toaddr - 1) == '\0')) return(0);

 if (!(bytes_copied <= maxlength)) return(ENOENT);
 if (!isitok(fromaddr,1,UREAD_OK)) return(EFAULT);
 return (0);
}


/***********************************************************************
 copyoutstr - copy a string from kernel memory to user memory subject
	      to maxlength and user validation.
 ***********************************************************************/

copyoutstr(fromaddr, toaddr, maxlength, lencopied)
register char *fromaddr, *toaddr;
register int  maxlength, *lencopied;
{
 register int bytes_copied = 0;
 register char *pg_break;

/* Do up to the first page boundary first. */
 pg_break = toaddr +  NBPG - ((int) toaddr % NBPG);
 
 for(;;) {
  if (!isitok(toaddr, (pg_break - toaddr) - 1,UWRITE_OK)) 
	goto checkBytes;

  while ((toaddr < pg_break) && (bytes_copied < maxlength) &&
	 (bytes_copied++,*toaddr++ = *fromaddr++))
    ;

  if (lencopied != NULL) *lencopied = bytes_copied;

  if ((bytes_copied > 0) && ((*(fromaddr - 1) == '\0')))  return(0);

  if (!(bytes_copied < maxlength)) return(ENOENT);
  
  pg_break += NBPG;
 } 
/*NOTREACHED*/
checkBytes: while ((isitok(toaddr, 1,UWRITE_OK)) && 
	          (bytes_copied < maxlength) &&
		  (bytes_copied++,*toaddr++ = *fromaddr++))
		;

  if (lencopied != NULL) *lencopied = bytes_copied;

  if ((bytes_copied > 0) && ((*(fromaddr - 1) == '\0'))) return(0);

 if (!(bytes_copied <= maxlength)) return(ENOENT);
 if (!isitok(toaddr,1,UWRITE_OK)) return(EFAULT);
 return(0);
}
#endif	MACH_VM
