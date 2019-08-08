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
/* $Header: dmareg.h,v 5.0 86/01/31 18:08:39 ibmacis ibm42a $ */
/* $Source: /ibm/acis/usr/sys_ca/caio/RCS/dmareg.h,v $ */

#if !defined(lint) && !defined(LOCORE) && defined(RCS_HDRS)
static char *rcsiddmareg = "$Header: dmareg.h,v 5.0 86/01/31 18:08:39 ibmacis ibm42a $";
#endif

/* DMA controller register
 */


#define CTL1_BASE       0xF0008840        /*    Controller 1 base register */
#define CTL1_CMD        0xF0008848        /* W  Command register           */
#define CTL1_STAT       0xF0008848        /* R  Status register            */
#define CTL1_REQ        0xF0008849        /* W  Request register           */
#define CTL1_SMASK      0xF000884A        /* W  Single bit mask register   */
#define CTL1_MODE       0xF000884B        /* W  Mode register              */
#define CTL1_FF         0xF000884C        /* W  Internal Flip-Flop         */
#define CTL1_TMP        0xF000884D        /* R  Temporary register         */
#define CTL1_CLR        0xF000884E        /* W  Clear mask  ???            */
#define CTL1_CLRA       0xF000884F        /* W  All mask register bits     */
struct tcw_reg {
	short  tcw_vir:1;	/* 0 = real, 1 = virtual */
	short  tcw_iob:1;	/* 0 = main memory, 1 = i/o bus */
	short	:1;
	short	tcw_prefix:13;
	};

/* tcw_vir values */
#define	TCW_VIR_REAL	0	/* Addresses are real */
#define	TCW_VIR_VIRTUAL	1	/* Addresses are virtual */

/* tcw_iob values */
#define	TCW_IOB_MAIN	0	/* Use main memory */
#define	TCW_IOB_IOBUS	1	/* Use I/O bus */

/* temporary defines , just for testing */


#define  REAL_ACC              0             /* Real access       */
#define  VIRTUAL_ACC      0x8000             /* Virtual access    */

/* BIT 14 */

#define  RSC_ACC               0             /* RSC access        */
#define  IOB_ACC          0X4000             /* IO BUS access     */


#define TCW_BASE        0xF0010000            /* TCWs base address */
/*
 * Note: not all combinations of iob and vir make sense for all types of
 * dma (page mode vs region mode, system dma vs alternate master dma).
 * See the Hardware documentation (insert pub number here) for details.
 */

/* 8237 dma controller chip registers */

struct dma_8bit_device {
	struct {
		char dm_base;
		char dm_count;
		} dm_chan[4];	/* base /count registers */

	char  dm_cmd;		/* command status */
	char  dm_req;		/* request  register */
	char  dm_smask;		/* single mask */
	char  dm_mode;		/* mode register */
	char  dm_ff;		/* internal flip-flop */
	char  dm_temp_mclr;		/* R:temp reg , W: Master clear */
	char  dm_clr_mask;		/* W:clear mask */
	char  dm_all_mask;		/* W:write all mask reg */
	};








#define  CRRB           0xF0008C60        /* component reset reg B    */
#define  DMRA           0xF00088E0        /* DMA mode reg. A          */
#define  DBRA           0xF00088C0        /* DMA buffering reg A      */

#define  DM_CTL1_ENABLE          0x08        /*  8 bit controller        */
#define  DM_CTL2_ENABLE          0x10        /* 16 bit controller        */




/*
 * Controller_2  registers  (channel 5-7) 16 bit
 *
 * Current address register (16 bit)    R/W      CTL2_BASE + (4 * ch)
 * Current count register   (16 bit)    R/W      CTL2_BASE + (4 * ch) + 2
 */

#define Ctl2_base       0xF0008860        /*    Controller 2 base register */
#define CTL2_CMD        0xF0008870        /* W  Command register           */
#define CTL2_STAT       0xF0008870        /* R  Status register            */
#define CTL2_REQ        0xF0008872        /* W  Request register           */
#define CTL2_SMASK      0xF0008874        /* W  Single bit mask register   */
#define CTL2_MODE       0xF0008876        /* W  Mode register              */
#define CTL2_FF         0xF0008878        /* W  Internal Flip-Flop         */
#define CTL2_TMP        0xF000887A        /* R  Temporary register         */
#define CTL2_CLR        0xF000887C        /* W  Clear mask  ???            */
#define CTL2_CLRA       0xF000887E        /* W  All mask register bits     */


/* dm_channel */

#define DM_CHAN0   0
#define DM_CHAN1   1
#define DM_CHAN2   2
#define DM_CHAN3   3
#define DM_CHAN5   5
#define DM_CHAN6   6
#define DM_CHAN7   7
#define DM_CHAN8   8

/* dm_operation  */

#define  DM_READ        0x8     /* in the mode byte of the 8237 */
#define  DM_WRITE       0x4


/* dm_transfer   */

#define DM_DEMAND  0x0          /* Demand transfer mode */
#define DM_SINGLE  0x40         /* single mode transfer */
#define DM_BLOCK   0x80         /* Block transfer mode  */
#define DM_CASCADE 0xC0         /* Cascade mode         */

/* dm_party      */

#define DM_FIRST   1            /* first party DMA      */
#define DM_THIRD   3            /* third party DMA      */

/* phys xfer size */
#define DM_MAXPHYS (62 * 1024)	/* 62K max tranfser	*/
