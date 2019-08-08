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
/* $Header: mousereg.h,v 5.0 86/01/31 18:12:20 ibmacis ibm42a $ */
/* $Source: /ibm/acis/usr/sys_ca/caio/RCS/mousereg.h,v $ */

#ifndef lint
static char *rcsidmousevar = "$Header: mousereg.h,v 5.0 86/01/31 18:12:20 ibmacis ibm42a $";
#endif

/*	Commands to be issued to the UART adapter to interface with
 * the Locater (mouse) device.
 *	These commands are issued using the uart_cmd call which is mapped
 * to the kbd_cmd call.  The associated mouse commands (descriptions
 * following) are included and passed along to the mouse device.
 */
#define	UART_ENABLE	0x3C	/* Extended Command - UART is enable */
#define	UART_SET_BK	0x35	/* Ext Cmd - Set blocking of rec bytes m5=1 */
#define	UART_RESET_BK	0x25	/* Ext Cmd - Reset blocking of rec bytes m5=0 */
#define	UART_WR_CONTROL	0x03	/* Write UART - control command */
#define	UART_WR_QUERY	0x04	/* Write UART - Query command */
#define	UART_SET_BAUD	0x05	/* Set UART Baud rate command */
#define	UART_INIT_FRM	0x06	/* Initialize UART Framing */

/*	Mouse Commands to be issued with a UART_WRITE_CONTROL command to the
 * adapter because they do not have a defined response.
 */

#define MS_ENABLE	0x08	/* Allow mouse to start transmission used */
				/* after a POR, RESET, or DISABLE. */
#define MS_DISABLE	0x09	/* Disable mouse data trans. Needed when  */
				/* issuing commands in stream mode. */
#define MS_SET_WM	0x0E	/* Set WRAP mode, mouse will echo data */
				/* sent from host, (except 0x01 or 0x0F)  */
#define MS_RESET_WM	0x0F	/* Reset WRAP mode */
#define MS_SET_EXP	0x78	/* Set EXPONENTIAL Scaling, only is perf- */
				/* ormed in stream mode. */
#define MS_RESET_EXP	0x6C	/* Reset EXPONENTIAL Scaling, set LINEAR  */
#define MS_SET_SAMP	0x8A00	/* Set SAMPLING Rate, only in stream mode */
				/* low byte is Sample rate. (See chart) */
#define MS_SET_RESL	0x8900	/* Set RESOLUTION low byte is Resolution  */
				/* (See resolution chart). */
#define MS_STREAM	0x8D00	/* Set STREAM Mode, date is sent when the */
				/* mouse has moved 1 count of resolution  */
				/* or any button pushed */
#define MS_REMOTE	0x8D03	/* Set REMOTE Mode, Mouse only reports */
				/* data on READ X,Y DATA Command */

/*	Mouse Commands to be issued with a UART_WR_QUERY command to the
 * adapter because they have a defined response.  Each command has a
 * required mode bit 5 setting (UART date byte Blocking 0 - off (UART_RESET_BK)
 * and 1 - on (UART_SET_BK)).
 */
#define MS_RESET	0x01	/* Same as POR, M5=1, after internal */
				/* tests mouse sends 4 bytes - FF,08,00,00 */
#define MS_RD_CONF	0x06	/* Read CONFIGURATION, M5=0 resp: 0x20 */
#define MS_CONFIGURED	0x20	/* Read configuration response */
#define MS_RD_DATA	0x0B	/* Read X,Y DATA, M5=1, in remote or stream*/
				/* mode. resp: one data report (See Chart) */
#define MS_STATUS	0x73	/* Query Mouse Status, M5=1, resp: 4 status*/
				/* bytes, (See status Chart) */


/* QUERY STATUS CHART -- Response from mouse on a MS_STATUS command
 *
 * Byte 1	  bit
 *		    0	1 = ready; 0 = init incomplete
 *		  1,2	1
 *		    3	0
 *		    4	1 = Linear scaling; 0 = Exponential scaling
 *		    5	0 = Enabled; 1 = Disabled
 *		    6	0 = Stream mode; 1 = Remote mode
 *		    7	1
 * Byte 2
 *		    0	1 = Right key depressed ; 0 = up
 *		    1	0
 *		    2	1 = Left key depressed ; 0 = up
 *		  3-7	0
 * Byte 3
 *		  0-6	Current Resolution setting (6 = LSB)
 *		    7	0
 * Byte 4
 *		  0-7	Current Sampling rate (7 = LSB)
 */
#define MS_STATUS_B1 "\20\10READY\04LINEAR-SCALE\03DISABLED\02REMOTE-MODE"
#define MS_STATUS_B2 "\20\10RIGHT-KEY\06LEFT-KEY"

/* MOUSE DATA REPORT CHART -- Data sent in streaming mode or in response
 * from a READ X,Y DATA (MS_RD_DATA) command in either mode
 *
 * Byte 1	  bit
 *		  0-7	00001011
 * Byte 2
 *		    0	1 = Right key depressed ; 0 = up
 *		    1	0
 *		    2	1 = Left key depressed ; 0 = up
 *		  3-4	00
 *		    5	X Data Sign bit (1 = Negative value)
 *		    6	Y Data Sign bit (1 = Negative value)
 *		    7	0
 * Byte 3
 *		  0-6	X Data (6 = LSB)
 *		    7	0
 * Byte 4
 *		  0-6	Y Data (6 = LSB)
 *		    7	0
 */
#define MS_DATA_SYNC	0x0B
#define MS_DATA_B2	"\20\02Y-NEGATIVE\03-X-NEGATIVE\06LEFT-KEY\08RIGHT-KEY"

/* BAUD RATE TABLE used to specify the baud rate for UART_SET_BAUD
 * calculated ad COUNTER = 256 - (9.216 MHZ / (192 * BAUD))
 */
static struct asybaudtbl {
	int rate;
} msbaudtbl [] = {
	/* B0 */	00,
	/* B50 */	50,
	/* B75 */	75,
	/* B110 */	110,
	/* B134 */	134,
	/* B150 */	150,
	/* B200 */	200,
	/* B300 */	300,
	/* B600 */	600,
	/* B1200 */	1200,
	/* B1800 */	1800,
	/* B2400 */	2400,
	/* B4800 */	4800,
	/* B9600 */	9600,
	/* EXTA */	7200,
	/* EXTA */	19200,
};

/* BAUD_RATE		COUNTER */
#define UART_B24000	254
#define UART_B9600	251
#define UART_B4800	246
#define UART_B2400	236
#define UART_B1200	215
#define UART_B600	176
#define UART_B300	96
#define OSC		9216000		/* Hz */
#define MSBAUD(s)	(256-(OSC/((msbaudtbl[s].rate)*192)))

/* BIT Definitions for UART INITIALIZE FRAMING (UART_INIT_FRM) command
 *     bit 7 high bit
 *
 * Bit	7:	1 = Odd parity (default) ; 0 = Even parity
 *    6-3:	0
 *    2-0:	Blocking Factor 2-6 (4 default) 4 need for data report
 *		from mouse when Blocking is active M5=1
 */
#define UART_FRM_ODDP	0x84
#define UART_FRM_EVENP	0x4


#define IO_BASE 0xf0000000	/* I/O base address */

#define in(port) * (( char *) (IO_BASE + (port)))
#define out(port,value) in(port) = value
/* output a PC word (= short) */
#define inw(port) * (( short *) (IO_BASE + (port)))
#define outw(port,value) inw(port) = value

#define KYB_CNTIW 0x8407		/* 8255 control port write */
#define KYB_CNTIR 0x8406		/* 8255 control port read */
#define KYB_READ  0x8404		/* 8255 data read */
#define KYB_WRITE 0x8400		/* 8255 data write */
#define KBD_ADAPTOR_RESET 0xFB		/* adaptor reset */
#define KBD_ADAPTOR_RELEASE 0x04	/* adaptor release */
#define CRRB	0x8c60			/* conponent reset reg B */

#define KYB_IIC_MASK	0x0F		/* Interrupt Ident Code Mask */
#define KYB_INT		0x08		/* Interrupt Received bit */
#define KYB_INFO	0x08		/* Information Interrupt */
#define KYB_DATA	0x09		/* Keyboard data int */
#define UART_DATA	0x0A		/* Uart data int */
#define KYB_REQD	0x0B		/* Return requested byte int */
#define UART_BLK	0x0C		/* Block Transfer int */
#define KYB_UNASS	0x0D		/* Unassigned */
#define KYB_SR		0x0E		/* Softr Reset/Completion code */
#define KYB_EC		0x0F		/* Detected an error condition */
#define UART_BUSY       0x08         	/* UART (mouse) busy bit */

#define IID_MASK	0x07		/* mask for Ident code */
#define KBD_INFO	0x00		/* information */
#define KBD_DATA	0x01		/* value for data present */
#define KBD_UART	0x02		/* byte from UART */
#define KBD_REQ		0x03		/* returned requested byte */
#define KBD_BLOCK	0x04		/* block request */
#define KBD_RESERVED	0x05		/* reserved */
#define KBD_SELF_TEST	0x06
#define KBD_ERROR	0x07
#define KBD_TIMEOUT	0x100		/* if we have timed out */

/*    Adapter commands */

#define	EXTCMD		0x00	/* Extended command select */
#define READ_STAT       0x12    /* Read status style from shared memory */
#define	SP_MEDIUM	0x42	/* set speaker medium */
#define	UARTCMD		0x04	/* Command to uart */
	/* These cmds return 2 datablocks */
#define	RSMOUSE		0x01	/* Reset mouse */
#define	QYMOUSE		0x73	/* Query mouse */
#define	UARTCNT		0x03	/* Control to uart */
#define	MSTRANS		0x08	/* Enable mouse for transm. */
#define	KYBDCMD		0x01	/* Command to kybd */
#define	KRESET		0xFF	/* Keyboard reset */
#define	KDEFDS		0xF5	/* Kybd default disable */
#define	KSCAN		0xF4	/* Kybd start scanning */
#define	READID		0xF2	/* Read kybd id */
#define	SETLED		0xED	/* Set kybd LEDs */
#define	WRRAM		0x10	/* Write shr RAM */
#define	RDRAM		0x00	/* Read	shr RAM */
#define	RD1C		0x1C	/* Read shr RAM 0x1C. */
#define	SETFCHI		0x08	/* Set freq counter hi byte */
#define	FCHI		0x01	/* Hi frequency byte */
#define	SETFCLO		0x09	/* Set freq counter lo byte */
#define	FCLO		0xAC	/* Lo frequency byte */
#define	SPKRON		0x02	/* Turn on speaker */
#define	SPTIME		0X04	/* Speaker duration */
#define	ENKYBD		0x3B	/* Enable kybd mode bit 11 */
#define	ENUART		0x3C	/* Enable UART mode bit 12 */
#define	RDVERS		0xE0	/* Read version */
#define	DSLOCK		0x2D	/* Disable keylock, bit 13 */
#define	CMDREJ		0x7F	/* Command reject received */
#define	POER		0xFF	/* Power-on error report. */
#define	INFO		0x00	/* Int id for information */


#define KYB_CONFIG	0xc3			/* 8255 configuration value */
/*
 * following bits are in KYB_CNTIR
 */
#define KYB_IBF		0x20			/* IBF (input buffer full?) */
#define KYB_OBF		0x80			/* OBF (output buffer full?) */
#define CMD(cmd,data) (cmd + (data <<8) & 0xffff) /* pack data and command */
#define LCMD(cmd,data) (cmd + (data & 0xff00))	/* pack data and command */

/* Device errors returned from 8051 */
#define MS_ERROR	0xE0

static char *MS_ERROR_MSG [] = {
	"Keyboard_transmit_timeout",				/* E0 */
	"Keyboard_receive_timeout",				/* E1 */
	"Keyboard_ack_not_received",				/* E2 */
	"Unexpected_keyboard_ack_received",			/* E3 */
	"Hard_error_on_keyboard_frame_receive",			/* E4 */
	"Hard_error_on_keyboard_frame_transmit",		/* E5 */
	"Keyboard_clock_pin_not_plus",				/* E6 */
	"keyboard_clock_pin_not_minus",				/* E7 */
	"uart_interrupt_without_tran_rec_identifier",		/* E8 */
	"Uart_transmit_timeout",				/* E9 */
	"Uart_ack_timeout",					/* EA */
};

#define UART_ERROR	0x41
#define MAX_UART_ERROR	0x51
static char *UART_ERROR_MSG [] = {
	"Keyboard transmit is busy",				/* 41 */
	"Keylock Switch is ON",					/* 42 */
	"Keyboard interface is disabled",			/* 43 */
	"Invalid Keyboard data",				/* 44 */
	"UNKOWN ERROR CODE",					/* 45 */
	"UNKOWN ERROR CODE",					/* 46 */
	"Invalid Speaker Duration",				/* 47 */
	"Invalid Speaker frequency",				/* 48 */
	"UNKOWN ERROR CODE",					/* 49 */
	"Speaker queue full",					/* 4A */
	"UART interface is disabled",				/* 4B */
	"Adapter is busy with a previous Write to UART",	/* 4C */
	"Invalid Uart Baud rate",				/* 4D */
	"Invalid Uart framing",					/* 4E */
	"UNKOWN ERROR CODE",					/* 4F */
	"Invalid Count (set_sequence)",				/* 50 */
	"Illegal mode (Diagnostic)",				/* 51 */
};
