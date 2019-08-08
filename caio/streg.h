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
/* $Header: streg.h,v 5.0 86/01/31 18:14:13 ibmacis ibm42a $ */
/* $Source: /ibm/acis/usr/sys_ca/caio/RCS/streg.h,v $ */

#if !defined(lint) && !defined(LOCORE) && defined(RCS_HDRS)
static char *rcsidstreg = "$Header: streg.h,v 5.0 86/01/31 18:14:13 ibmacis ibm42a $";
#endif

/*	streg.h	6.1	83/07/29	*/

/*
 * ST adapter registers
 */
struct stdevice {		/*      must cast data when used         */
	unsigned short stsec;	/* 2:   sector buffer *//*aka ram_port */
	unsigned char stcs;	/* 0:   control reg *//*aka cntl_port status_port*/
	unsigned char empty2;	/* 3:   */
	unsigned short stdata;	/* 4:   data port *//*aka data_port */
/*	unsigned char empty3;	/* 5:   */
	unsigned char stcmd;	/* 4:   command port *//*aka cmd_port*/
};

#define	b_repcnt  b_bcount
#define	b_command b_resid

/* commands in stcmd */
#define	ST_RESET	0x00	/* reset adapter */
#define ST_SELECT	0x01	/* select drive */
#define	ST_REW		0x02	/* rewind */
#define	ST_ERASE	0x03	/* ERASE entire tape and rewind */
#define ST_RETENSION	0x04	/* retention tape */
#define	ST_SENSE	0x05	/* sense (issue READ STATUS, 8 bytes returned */
#define	ST_WCOM		0x06	/* write */
#define	ST_WIRG		0x06 	/*XXX*//* write with xtra interrecord gap */
#define	ST_WEOF		0x07	/* write-eof */
#define	ST_RCOM		0x08	/* read */
#define ST_REOF		0x09	/* read eof */
#define	ST_SFORW	0x0c	/* space forward */
#define	ST_TERMINATE	0x0d	/* terminate read/write */
#define	ST_CMD_AA	0xaa	/* ?? */
#define ST_OFFL		0x00		/* treat as reset */
#define ST_SREV		0x0b	/* a bad command */

#define ST_DATA	0x80000000	/* special flag for stcmd() */


#define	streverseop(cmd)		((cmd)==ST_SREV || (cmd)==ST_REW)


#define ST_CTLR_BITS STCFMT		/* "control" register bits */
#define STCFMT	"\20\1IBF\2OBF\3RESERVED\4INTERRUPT\5EXCEPTION\6ONLINE\7READY\10BUSY"

/* read-only bits in stcs */
#define STC_IBF	0x01	/* input buffer full */
#define STC_OBF	0x02	/* output buffer full */
#define STC_XX4 0x04	/* reserved */
#define STC_INT 0x08	/* INTERRUPT */
#define STC_EXC 0x10	/* EXCEPTION */
#define STC_ONL 0x20	/* ONLINE */
#define STC_RDY 0x40	/* READY */
#define STC_BSY 0x80	/* BUSY */

/* write-only bits in stcs */
#define STC_ENABLE	0x01	/* enable interrupts */
#define STC_NORESET	0x02	/* don't reset */
#define STC_RESET	0x00	/* reset */
#define STC_PARITY	0x04	/* enable parity */

/* bits in status byte 0 */
#define ST0_ST0	0x80	/* status bit for this byte */
#define ST0_CNI	0x40	/* cartridge not in place */
#define ST0_USL	0x20	/* unselected drive */
#define ST0_WRP	0x10	/* write protected cartridge */
#define ST0_EOM	0x08	/* end of media */
#define ST0_UDA	0x04	/* unrecoverable data error */
#define ST0_BNL	0x02	/* bad block not located */
#define ST0_FIL	0x01	/* file mark located */

#define ST0_BAD	ST0_CNI+ST0_USL+ST0_EOM+ST0_UDA+ST0_BNL

/* bits in status byte 1 */
#define ST1_ST1	0x80	/* status bit for this byte */
#define ST1_ILL	0x40	/* illegal command */
#define ST1_NTD	0x20	/* no data detected */
#define ST1_MBD	0x10	/* marginal block detected */
#define ST1_BOM	0x08	/* beginning of media */
#define ST1_BUS	0x04	/* reserved for bus parity error */
#define ST1_ERM	0x02	/* reserved for end of recorded media */
#define ST1_POR	0x01	/* power on/reset occurred */

/* streaming tape adapter completion codes */
#define CC_DONE		0x00
#define CC_LD_COUNT	0x01
#define CC_POR_AA	0x02
#define CC_RD_STATUS	0x10
#define CC_RD_VERS	0x20
#define CC_READ		0x30
#define CC_READ_LAST	0x31
#define CC_WRITE	0x40
#define CC_POR		0x55
#define CC_EXCEPTION	0x80
#define CC_CMD_REJ_EXCEPT 0x90

/* error values that can be returned */
#define CC_ROSERR	0xa0
#define CC_RAMERR	0xa1
#define CC_FPLSERR	0xa2
#define CC_DRVERR	0xa3
#define CC_BUSERR	0xa4


/* %b string for error status bits */
#define ST0FMT	"\20\10ST0\7NO-CARTRIDGE\6UNSELECTED\5WRITE-PROTECTED\4END-OF-MEDIA\3UNREC-ERROR\2BAD-BLOCK-NOT-LOCATED\1FILE-MARK"
#define ST1FMT	"\20\10ST1\7ILLEGAL-CMD\6NO-DATA\5MARGINAL-BLOCK\4BEGIN-MEDIA\3BUS-PARITY-ERROR\2END-OF-RECORDED-MEDIA\1POWER-RESET"

#define ST_ERR_BITS "\20\10ST0\7NO-CARTRIDGE\6UNSELECTED\5WRITE-PROTECTED\4END-OF-MEDIA\3UNREC-ERROR\2BAD-BLOCK-NOT-LOCATED\1FILE-MARK\20ST1\17ILLEGAL-CMD\16NO-DATA\15MARGINAL-BLOCK\14BEGIN-MEDIA\13BUS-PARITY-ERROR\12END-OF-RECORDED-MEDIA\11POWER-RESET"

