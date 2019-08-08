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
 * 5799-CGZ (C) COPYRIGHT IBM CORPORATION  1986
 * LICENSED MATERIALS - PROPERTY OF IBM
 * REFER TO COPYRIGHT INSTRUCTIONS FORM NUMBER G120-2083
 */
/* $Header: adunload.c,v 5.0 86/01/31 18:05:26 ibmacis ibm42a $ */
/* $Source: /ibm/acis/usr/sys_ca/caio/RCS/adunload.c,v $ */

typedef int hd_data_t;
#define HDXFERSIZE (sizeof (hd_data_t))

#define COUNT	12
 /*
  * this block read routine is used to copy data out of the adapter
  * for cases when 'buffaddr' is on a page boundary.
  * since it is on a page boundary it is also on a 32bit and a 16 bit
  * boundary.
  * flag = 0	for write
  * flag != 0 	for read
  */
adunload(buffaddr, adreg, hdcnt, flag)
	register hd_data_t * buffaddr;
	register int hdcnt;
	register hd_data_t * adreg;
	register int flag;
{
	register int r1;

/*	HDDEBUG(SHOW_COUNT,printf("addr=%x count=%d ",buffaddr,hdcnt)); */
	if (flag) {
		if (buffaddr == 0) {
			register int i;
			while ((hdcnt -= HDXFERSIZE) >= 0)
				i = *adreg;
			return;
		}
		for (; (hdcnt -= HDXFERSIZE * COUNT) >= 0;) {
			buffaddr[0] = *adreg;
			buffaddr[1] = *adreg;
			buffaddr[2] = *adreg;
			buffaddr[3] = *adreg;
			buffaddr[4] = *adreg;
			buffaddr[5] = *adreg;
			buffaddr[6] = *adreg;
			buffaddr[7] = *adreg;
			buffaddr[8] = *adreg;
			buffaddr[9] = *adreg;
			buffaddr[10] = *adreg;
			buffaddr[11] = *adreg;
			buffaddr += COUNT;
		}
		hdcnt += HDXFERSIZE * COUNT; /* correct overshoot */
		for (; hdcnt > 0; hdcnt -= HDXFERSIZE)
			*buffaddr++ = *adreg;
	} else {
		if (buffaddr == 0) {
			register int i;
			while ((hdcnt -= HDXFERSIZE) >= 0)
				*adreg = 0;
			return;
		}
		for (; (hdcnt -= HDXFERSIZE * COUNT) >= 0;) {
			*adreg = buffaddr[0];
			*adreg = buffaddr[1];
			*adreg = buffaddr[2];
			*adreg = buffaddr[3];
			*adreg = buffaddr[4];
			*adreg = buffaddr[5];
			*adreg = buffaddr[6];
			*adreg = buffaddr[7];
			*adreg = buffaddr[8];
			*adreg = buffaddr[9];
			*adreg = buffaddr[10];
			*adreg = buffaddr[11];
			buffaddr += COUNT;
		}
		hdcnt += HDXFERSIZE * COUNT; /* correct overshoot */
		for (; hdcnt > 0; hdcnt -= HDXFERSIZE)
			*adreg = *buffaddr++;
	}
}
