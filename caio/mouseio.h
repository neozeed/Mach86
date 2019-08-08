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
/* mouseio.h -- describes user interface to mouse driver\c
 \"This file is also included by a man page, hence this nroff code
.if 0 \{\
*/
/* $Header: mouseio.h,v 5.0 86/01/31 18:12:06 ibmacis ibm42a $ */
/* $Source: /ibm/acis/usr/sys_ca/caio/RCS/mouseio.h,v $ */

#if !defined(lint) && !defined(LOCORE)  && defined(RCS_HDRS)
static char *rcsidmouseio = "$Header: mouseio.h,v 5.0 86/01/31 18:12:06 ibmacis ibm42a $";
#endif
/*
.\}\" End of nroff code
*/

typedef union mouse {

	struct {	/* As data bytes */
		u_char MD_magic;	/* 0x0b = data report */
					/* 0xe? = status report */
					/* 0x6? = status report */
					/* 0xff = reset report */
		u_char MD_flags;	/* See below */
		u_char MD_x;	/* X-data (2's complement, bits 0-6) */
		u_char MD_y;	/* Y-data (2's complement, bits 0-6) */
	} m_D;

	struct {	/* As bits */
		unsigned int
			MB_status  :1,	/* 1 = ready, 0 = incomplete
						initialization */
			/* void */ :3,	/* 110 = status */
					/* 000 = data */
					/* 111 = reset */
			MB_scaling :1,	/* 1 = linear, 0 = exponential */
			MB_disabled:1,	/* 1 = disabled, 0 = enabled */
			MB_mode    :1,	/* 1 = remote, 0 = stream */
			/* void */ :1,	/* always 1 */

			MB_rbutton :1,	/* right button */
		 	MB_mbutton :1,  /* middle button (simulated) */
			MB_lbutton :1,	/* left button */
			/* void */ :1,	/* always 0 */
			/* void */ :1,	/* 1 = reset, 0 otherwise */
			MB_xsign   :1,	/* X-data sign bit */
			MB_ysign   :1,	/* Y-data sign bit */
			/* void */ :1,	/* always 0 */

			MB_xdata   :7,	/* X-data bits */
			/* void */ :1,	/* always 0 */

			MB_ydata   :7,	/* Y-data bits */
			/* void */ :1;	/* always 0 */
	} m_B;
} mousedata;

#ifndef _IO
#ifdef KERNEL
#include "../h/ioctl.h"
#else
#include <sys/ioctl.h>
#endif
#endif

/* Mouse interrupt level */
#define MS_SPL	spl5();

/* Mouse ioctl's */
#define MSIC_STREAM	_IO(m,0)
#define MSIC_REMOTE	_IO(m,1)
#define MSIC_STATUS	_IOR(m,2,long)
#define MSIC_READXY	_IOR(m,3,long)
#define MSIC_DISABLE	_IO(m,4)
#define MSIC_ENABLE	_IO(m,5)
#define MSIC_EXP	_IO(m,6)
#define MSIC_LINEAR	_IO(m,7)
#define MSIC_SAMP	_IOW(m,8,long)
#define MSIC_RESL	_IOW(m,9,long)

/* SAMPLE RATE CHART -- Data for MSIC_SAMP ioctl
 *	Sample Rate	Data
 */
#define MS_RATE_10	0x0A
#define MS_RATE_20	0x14
#define MS_RATE_40	0x28
#define MS_RATE_60	0x3C
#define MS_RATE_80	0x50
#define MS_RATE_100	0x64

/* SET RESOLUTION CHART -- Data for MSIC_RESL ioctl
 *						  RESOLUTION
 *			Data		Counts Per mm	Counts Per Inch
 */
#define MS_RES_200	0x00	/*	     8		      200	*/
#define MS_RES_100	0x01	/*	     4		      100	*/
#define MS_RES_50	0x02	/*	     2		       50	*/
#define MS_RES_25	0x03	/*	     1		       25	*/

/* Mouse line discipline number */
#define MSLINEDISC 5	/* This has to match the number in tty_conf.c */

/* Byte definitions */
#define m_magic	m_D.MD_magic
#define m_flags	m_D.MD_flags
#define m_x	m_D.MD_x
#define m_y	m_D.MD_y

/* Bit definitions */
#define m_status	m_B.MB_status	/* status report */
#define m_scaling	m_B.MB_scaling	/* status report */
#define m_disabled	m_B.MB_disabled	/* status report */
#define m_mode		m_B.MB_mode	/* status report */
#define m_rbutton	m_B.MB_rbutton	/* status and data */
#define m_lbutton	m_B.MB_lbutton	/* status and data */
#define m_xsign		m_B.MB_xsign	/* data report, else 0 */
#define m_ysign		m_B.MB_ysign	/* data report, else 0 */
#define m_error		m_ysign		/* reset report */

/* Bit field definitions */
#define m_xdata		m_B.MB_xdata	/* data report */
#define m_ydata		m_B.MB_ydata	/* data report */
#define m_resolution	m_B.MB_xdata	/* status report */
#define m_sample_rate	m_D.MD_y	/* status report */

/* Resolution values are in counts per inch */
#define M_RES200	0
#define M_RES100	1
#define M_RES050	2
#define M_RES025	3
#define M_RESOLUTION(x)	(200 >> (x).m_resolution)

/* Valid sample rate values (in reports per second) */
#define M_RATE100	100
#define	M_RATE080	80
#define	M_RATE060	60
#define	M_RATE040	40
#define	M_RATE020	20
#define	M_RATE010	10
#define M_RATE(x)	((x).m_sample_rate)	/* for consistency */

/* Mode values (for set rate command, below) */
#define M_MODE_STREAM	0x00
#define M_MODE_REMOTE	0x03

/* Report types */
#define M_ISDATA(x)	((x).m_magic == 0x0b)
#define M_ISSTATUS(x)	(((x).m_magic & 0x61) == 0x61)
#define M_ISRESET(x)	((x).m_magic == 0xff)

/* Data extraction */
#define M_XDATA(x) ((int)((x).m_xsign ? ((x).m_xdata | (~0x3f)) : (x).m_xdata))
#define M_YDATA(y) ((int)((y).m_ysign ? ((y).m_ydata | (~0x3f)) : (y).m_ydata))

/* Commands */
typedef enum mouse_cmd {
	mcmd_reset = 0x01,	/* returns a reset response */
	mcmd_read_config = 0x06,	/* returns one byte = 0x20 */
#define M_CONFIG	0x20
	mcmd_enable = 0x08,	/* no response */
	mcmd_disable = 0x09,	/* no response */
	mcmd_read_data = 0x0b,	/* returns a data report */
	mcmd_wrap_on = 0x0e,	/* no response; enters wrap mode */
	mcmd_wrap_off = 0x0f,	/* no response; leaves wrap mode */
	mcmd_set_scale_exponential = 0x78,	/* no response */
	mcmd_set_scale_linear = 0x6c,	/* no response */
	mcmd_read_status = 0x73,	/* returns a status report */
	mcmd_set_rate = 0x8a,	/* no response; rate byte must follow */
	mcmd_set_mode = 0x8d,	/* no response; mode byte must follow */
	mcmd_set_resolution = 0x89,	/* no response; resolution byte
						must follow */
} mouse_cmd_t;
