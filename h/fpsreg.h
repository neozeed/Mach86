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
#
/*
 *	$Header: fpsreg.h,v 1.6 85/07/11 23:42:14 phb Exp $
 *
 *	(c) 1982 Copyright	Apunix Computer Services
 *
 *	$Log:	fpsreg.h,v $
 * Revision 1.6  85/07/11  23:42:14  phb
 * Added changes for Gould systems.
 * 
 * Revision 1.5  84/10/02  10:56:19  phb
 * Added driver commands to read and write the memory paging registers.
 * 
 * Revision 1.4  84/10/02  10:54:34  phb
 * Added Perkin Elmer changes and moved declartions of structures
 * to fps.c.
 * 
 * Revision 1.3  84/01/12  14:59:49  phb
 * Fixed bug caused by using 'f' character as command parameter in
 * ioctl.  Also added synchronization flags to mode word.
 * 
 * Revision 1.2  83/12/16  22:29:59  phb
 * Updated for Berkeley 4.2 BSD UNIX.
 * 
 */

/*
 *	OPTIONS:
 *		-DNEWFORMAT	The new vax interface boards support
 *				a format 5, which transfers 32 bit
 *				integers correctly on a vax.
 */

/*
 * FPS AP-120B registers
 */
#ifdef	KERNEL
struct	fpsdevice
  {
#ifdef PERKINELMER
	char	fpscsr;		/* control and status */
	char	fpsapad;	/* ap address */
	char	fpsfhigh;	/* formatter buffer high */
	char	fpsflow;	/* formatter buffer low */
	char	fpsfn;		/* function register */
	char	fpsswtch;	/* switch register */
	char	fpslite;	/* lights */
	char	fpsrset;	/* reset */
	char	fpswc;		/* word count */
	char	fpshad;		/* host address */
	char	fpsxhad;	/* xmem bits for host address */
	char	fpssfifo;	/* read fifostat / wrt mae */
	char	fpsxapad;	/* read broms / wrt xmem bits for ap address */
	char	fpsid;		/* read interface id register / wrt inc bprom */
	char	fpshma;		/* another hma register ? */
	char	fpsxhma;	/* another hma register ? */
	char	fpsxba;		/* buf x a */
	char	fpsxbb;		/* buf x b */
	char	fpshnop;	/* hma + nop */
	char	fpsxnop;	/* xhma + nop */
	char	fpssmdt;	/* start mdt */
	char	fpsflg;		/* flag */
	char	fpsfifo;	/* fifo */
	char	fpstemp;	/* temp */
	char	fpsctl;		/* ctl */
	char	fpsrhflg;	/* flg (r+h) */
	char	fpsifsr;	/* interface status and reset register */
	char	fpsrcflg;	/* flg (r+c) */
	char	fpshstbus;	/* read hst bus / wrt apma cnt */
	char	fpshdbus;	/* read hd bus / wrt wc.cnt */
	char	fpsdibus;	/* read di bus */
	char	fpsrobus;	/* read di bus / wrt mdt rst */
#else
#ifdef GOULD
	char	fpsswtch;	/* switch register */
	char	fpsfn;		/* function register */
	char	fpslite;	/* lights */
	char	fpsformat;	/* formatter buffer */
	char	fpsapad;	/* ap address */
	char	fpscsr;		/* control and status */
	char	fpsnop;		/* control and status */
	char	fpsrset;	/* reset */
#else
	struct	{
		u_short	fpsfhigh;
		u_short	fpsflow;
	} fpsfmt[16];
	short	fpswc;		/* word count */
	u_short	fpshad;		/* host address */
	u_short	fpscsr;		/* control and status */
	u_short	fpsapad;	/* ap address */
	u_short	fpsswtch;	/* switch register */
	u_short	fpsfn;		/* function register */
	u_short	fpslite;	/* lights, xmem bits (write) */
	u_short	fpsrset;	/* reset, xmem bits (read) */
#endif
#endif
  };

struct	fps_softc	/* software control registers */
  {
	struct proc	*sc_openf;
	struct buf	*sc_header;
	caddr_t		sc_iocbase;
#ifdef GOULD
	unsigned int	sc_ubinfo;
#else
	int		sc_ubinfo;
#endif
	u_short		sc_eword;
	u_short		sc_errcnt;
	u_short		sc_fpserr;
	u_short		sc_ccount;
	u_short		sc_cindex;
	u_short		sc_ndone;
	u_short		sc_dindex;
	u_short		sc_mode;
  };

#endif

/*
 * FPS - device register bit definitions
 */

/* fn register bits */
#define	FN_HALTED	0100000		/* AP-120B halted */
#define	FN_START	0040000		/* start */
#define	FN_CONT		0020000		/* continue */
#define	FN_STEP		0010000		/* step mode */
#define	FN_RESET	0004000		/* reset */
#define	FN_EXAM		0002000		/* examine */
#define	FN_DEPOS	0001000		/* deposit */
#define	FN_BREAK	0000400		/* set breakpoint */
#define	FN_MAINC	0000100		/* increment main data address */
#define	FN_DPAINC	0000200		/* increment dpa address */
#define	FN_TMAINC	0000300		/* increment tma address */
#define	FN_Q1		0000000		/* examine quarter one */
#define	FN_Q2		0000020		/* examine quarter two */
#define	FN_Q3		0000040		/* examine quarter three */
#define	FN_Q4		0000060		/* examin quarter four */

#define	FN_BITS \
"\10\20HALT\17START\16CONT\15STEP\14RESET\13EXAM\12DEPOS\11BREAK\10DPAINC\7MAINC"

#define	S_PSA		0000000		/* select psa */
#define	S_SPD		0000001		/* select s pad destination */
#define	S_MA		0000002		/* select main data address */
#define	S_TMA		0000003		/* select tma address */
#define	S_DPA		0000004		/* select dpa address */
#define	S_SPRG		0000005		/* select s pad function */
#define	S_STATUS	0000006		/* select ap status */
#define	S_DA		0000007		/* select device address */
#define	S_PS		0000010		/* select program source */
#define	S_DPX		0000013		/* select data pad X */
#define	S_DPY		0000014		/* select data pad Y */
#define	S_MD		0000015		/* select main data memory  */
#define	S_SPFN		0000016		/* select s pad function */
#define	S_TM		0000017		/* select table memory */
#define	S_MASK		0000377		/* mask for select field */

/* Control register bits */
#define	CS_WCZERO	0100000		/* word count 0 */
#define	CS_IAP		0040000		/* set INTRQ flag in AP */
#define	CS_IAPWC	0020000		/* set INTRQ flag on WC zero */
#define	CS_ONHALT	0010000		/* int. on halt */
#define	CS_ONZERO	0004000		/* int. on WC zero */
#define	CS_ONAPRQ	0002000		/* int. on AP120b request */
#define	CS_FERR		0001000		/* Format error */
#define	CS_DLATE	0000400		/* Data late */
#define	CS_CC		0000200		/* consecutive cycle */
#define	CS_DMAENB	0000100		/* enable DMA transfers */
#define	CS_APTOH	0000040		/* ap to host */
#define	CS_HTOAP	0000000		/* host to ap */
#ifdef	NEWFORMAT /* new vax formatter boards support 32 bit vax transfers */
#define	CS_FORMAT	0000016		/* Format bits */
#else
#define	CS_FORMAT	0000006		/* Format bits */
#endif
#define	CS_GO		0000001		/* go. */
#define	CS_HDMABUSY  	0000001		/* dma is busy */

#define	CS_BITS \
"\10\20WCZERO\17IAP\16IAPWC\15ONHALT\14ONZERO\13ONAPRQ\12FERR\11DLATE\10CC\7DMAENB\6APTOH\1GO"

/* STATUS register bits */
#define	ST_OVF		0100000
#define	ST_UNF		0040000
#define	ST_DIVZ		0020000
#define	ST_PERR		0000200
#define	ST_PENB		0000100
#define	ST_SRAO		0000040

#ifdef PERKINELMER
/*
 * Interface command bits (oc)
 */
#define	OC_IENB		0x40	/* enable device interrupts */
#define	OC_QINT		0x80	/* queue (and hold) device interrupts */
#define	OC_DISARM	0xC0	/* disarm device interrupts */

/*
 * Interface status bits (ss)
 */
#define	SS_DU		0001	/* device unavailable */
#define	SS_PFAIL	0020	/* power fail interrupt */
#define	SS_CTL5		0040	/* AP software interrupt */
#define	SS_HALT		0100	/* AP halt interrupt */
#define	SS_ILL		0200	/* illegal interface command interrupt */
#define	SS_SELCH	0400	/* selch interrupt (software state only) */
#endif

/*
 *	fps driver iocb structure
 */
struct fps_iopb	{
	char	pb_cmd;		/* command */
	char	pb_fmt;		/* format */
	u_short	pb_apad;	/* AP address / register select */
	caddr_t	pb_had;		/* host address */
	u_short	pb_dcnt;	/* data count */
};

#ifdef	BSD42
#ifndef	KERNEL
#ifndef	_IOCTL_
#include	<sys/ioctl.h>
#endif
#endif
#define	NFPSCOM		((IOCPARM_MASK+1) / sizeof(struct fps_iopb))
#else
#define	NFPSCOM		(BSIZE / sizeof(struct fps_iopb))
#define	_IO(x,y)	(('x'<<8)|y)
#define	_IOR(x,y,t)	(('x'<<8)|y)
#define	_IOW(x,y,t)	(('x'<<8)|y)
#define	_IOWR(x,y,t)	(('x'<<8)|y)
#endif


/* ioctl commands */
#define	IO_BIO		_IOW(a,1,0)	/* Block I/O commands */
#define	IO_SYNC		_IO(a,2)	/* Synchronize host with AP */
#define	IO_WR		_IO(a,3)	/* Synchronize AP Running with host */
#define	IO_WD		_IO(a,4)	/* Synchronize AP DMA with host */
#define	IO_FPSERR	_IOR(a,5,short)	/* return EFPS error information */
#define	IO_RCSR		_IOR(a,6,short)	/* read CSR register */
#define	IO_RFN		_IOR(a,7,short)	/* read FN register */
#define	IO_SETMODE	_IOW(a,8,short)	/* sets low byte of drive mode word */
#define	IO_GETMODE	_IOR(a,9,short)	/* gets driver mode word */
#define	IO_RESET	_IO(a,10)	/* reset the AP */

#define	IOC_READREG	1
#define	IOC_WRITEREG	2
#define	IOC_EXAM	3
#define	IOC_DEPOSIT	4
#define	IOC_PUT		5
#define	IOC_GET		6
#define	IOC_PSLOAD	7
#define	IOC_SPLOAD	8
#define	IOC_GO		9
#define	IOC_WD		10
#define	IOC_WR		11
#define	IOC_XCONT	12
#define	IOC_RMAE	13
#define	IOC_WMAE	14
#define	IOC_RAPMAE	15
#define	IOC_WAPMAE	16
#define	IOC_RMSK	17
#define	IOC_WMSK	18

#define	IOF_OUT		0177
#define	IOF_TWOSP	0020
#define	IOF_THREESP	0040

#define	PSL_FORMAT	0
#define	PSL_SIZEBOOT	15
#define	PSL_START	1
#define	PSL_SYSSTART	8	

/*
 *	Driver mode bits
 */
#define	M_DINTR		0000001	/* Disable software interrupt */
#define	M_NOCHK		0000002	/* No error checking when AP halts */
#define	M_HSDREGIO	0000040	/* gould: register i/o in progress */
#define	M_HSDBUSY	0000100	/* gould: HSD is busy */
#define	M_WAPINT	0000200	/* gould: would like to wait for AP intr */
#define	M_PSLOAD	0000400	/* driver is loading program source */
#define	M_WAITING	0001000	/* waiting for something to get done */
#define	M_WHALTINT	0002000	/* want (waiting for) a halt interrupt */
#define	M_WDMAINT	0004000	/* want (waiting for) a dma interrupt */
#define	M_DMABUSY	0010000	/* DMA is busy */
#define	M_DMAWAIT	0020000	/* Waiting on DMA to complete */
#define	M_INTLEVEL	0040000	/* driver is running at interrupt level */
#define	M_USERBITS	0000037	/* mode bits setable by the user */

#define	M_WANYINT	(M_WDMAINT | M_WHALTINT)

#ifdef PERKINELMER
#define	FPSNREG		32	/* # bytes of registers */

#define	FPSWC		8	/* offsets of particular registers */
#define	FPSHAD		9
#define	FPSCSR		0
#define	FPSAPAD		1
#define	FPSSWTCH	5
#define	FPSFN		4
#define	FPSLITE		6
#define	FPSRSET		7

#define	FPSFHIGH	2
#define	FPSFLOW		3
#define FPSLOW		0
#define FPSHIGH		31
#else
#ifdef	GOULD
#define	FPSNREG		8	/* # bytes of registers */

#define	FPSWC		6	/* offsets of particular registers */
#define	FPSNOP		6
#define	FPSHAD		6
#define	FPSCSR		5
#define	FPSAPAD		4
#define	FPSSWTCH	0
#define	FPSFN		1
#define	FPSLITE		2
#define	FPSRSET		7

#define	FPSFHIGH	3
#define	FPSFLOW		3
#define FPSLOW		0
#define FPSHIGH		7

#define	HSDM_HALTINT	020
#define	HSDM_APINT	010
#else
#define	FPSNREG		80	/* # bytes of registers */

#define	FPSWC		0100	/* offsets of particular registers */
#define	FPSHAD		0102
#define	FPSCSR		0104
#define	FPSAPAD		0106
#define	FPSSWTCH	0110
#define	FPSFN		0112
#define	FPSLITE		0114
#define	FPSRSET		0116

#define	FPSFHIGH	0000
#define	FPSFLOW		0002
#define FPSLOW		0000
#define FPSHIGH		0116
#endif
#endif

#define	IOQ_WAIT	_IOWR(a,0,short)
#define	IOQ_NOWAIT	_IOWR(a,1,short)

#ifdef	KERNEL
#define	NFPSQ		20	/* number of processes the queue can hold */

struct fpsQent	{		/* a single queue entry */
	struct proc	*q_procp;	/* the pointer to this process' info */
	unsigned int	q_pid;		/* the pid of the waiting process */
	unsigned int	q_apno;		/* ap no. the process is waiting for */
	struct fpsQent	*q_forward;	/* next entry in the list */
	struct fpsQent	*q_back;	/* preceeding entry in the list */
};
#endif
