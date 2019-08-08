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
/* $Header: dma.c,v 5.0 86/01/31 18:08:22 ibmacis ibm42a $ */
/* $Source: /ibm/acis/usr/sys_ca/caio/RCS/dma.c,v $ */

/* #include "saio.h" */

#include "../h/param.h"
#include "../h/vm.h"
#include "../h/buf.h"
#include "../h/time.h"
#include "../h/proc.h"
#include "../h/errno.h"
#include "../ca/pte.h"
#include "../ca/io.h"
#include "../caio/ioccvar.h"

#include "../ca/mmu.h"
caddr_t real_buf_addr();

/* the next 5 includes are not needed? */
#include "../h/systm.h"
#include "../h/conf.h"
#include "../h/kernel.h"
#include "../h/uio.h"
#include "../h/file.h"

/* #include "dmavar.h"		/* not needed any more */
#include "../caio/dmareg.h"





/* global variables */

#define  ctl1_smask ((char *) CTL1_SMASK)
#define  crrb      ((char *) CRRB)
#define  ctl1_mode ((char *) CTL1_MODE)
#define  ctl1_base ((char *) CTL1_BASE)

#define  ctl1_req  ((char *) CTL1_REQ)
#define  ctl1_cmd  ((char *) CTL1_CMD)
#define  dbra      ((char *) DBRA)
#define  dmra      ((char *) DMRA)
#define  ctl1_ff   ((char *) CTL1_FF)


#define BUSY  1
#define PAGESIZE 2048
struct dma_8bit_device *regp = (struct dma_8bit_device *)CTL1_BASE;




/* global variables */
int first_time = 0;			  /* initialization cluge */

struct iocc_hd iocc_hdr[4];		  /*  header structures per channel */






/*****************************************************************************

1) queues device structure on corresponding ic_hd structure.
2) if channel not busy calls dmastart()

*****************************************************************************/





int dma_setup(dm)

	register struct iocc_ctlr *dm;

{


	short channel;
	register struct iocc_hd *dhp = &iocc_hdr[dm->ic_channel];

	if (first_time == 0) {
		dma_init();
		first_time++;
	}
	;				  /* cluge init */

	dm->ic_forw = NULL;		  /* queue request */
	if (dhp->dh_actf == NULL)
		dhp->dh_actf = dm;
	else
		dhp->dh_actl->ic_forw = dm;
	dhp->dh_actl = dm;

	if (dhp->dh_state != BUSY) {	  /* if channel  not busy  process */
		dhp->dh_state = BUSY;
		dmastart(dm->ic_channel);
	}
	;
	return (0);
}


/*****************************************************************************
Name            : dmastart(chan)
Date created    : 4-21-1985
Last modified   : 4-23-1985
Version         : 1
Parameters      : dma channel number
Return          : error status
                :
                : Boundary error => panics
Author          : Robert M. Hadaya
Abstract        :

1) dequeues controller structure
2) allocate TCWs
3) calls device_dgo()



*****************************************************************************/


dmastart(channel)
	short channel;
{
	register struct iocc_ctlr *dmp;
	register struct iocc_hd *hdp = &iocc_hdr[channel];
	register struct buf *bp;
	int status;

	/* dequeue */


	if ((dmp = hdp->dh_actf) == NULL)
		return (0);		  /* no requests are pending */
	if ((hdp->dh_actf = dmp->ic_forw) == NULL)
		hdp->dh_actl = NULL;
	bp = dmp->ic_tab.b_actf->b_actf;

	/* see if request is 3rd party */
	if (dmp->ic_party != 3) {
		printf(" First party DMA not supported\n");
		bp->b_error = -3;
		return;
	} else {			  /* third party DMA */

		switch (channel) {
		case 0:			  /* 8bit controller 1  */
		case 1:
		case 2:
		case 3:
			status = ctl1_setup(dmp);
			break;
		case 4:
			status = -15;
			break;		  /* channel 4 unused   */
		case 5:			  /* 16 bit controller2 */
		case 6:
		case 7:
			status = ctl2_setup(dmp);
			break;
		case 8:
			status = copro_setup(dmp);
			break;		  /* coprocessor channel */

		default:
			status = -16;	  /* invalid channel # */
		}
	}
	bp->b_error = status;
	return;
}


/*
Set up the first 8237 controller which corresponds to the 4 8 bit channels.
*/

int ctl1_setup(dm)
	struct iocc_ctlr *dm;
{
	register struct iocc_hd *dhp = &iocc_hdr[dm->ic_channel];
	register struct dma_8bit_device *regp = dhp->dh_8237_reg;
	register struct buf *bp = dm->ic_tab.b_actf->b_actf;

	short displacement;		  /* 2k displacement	 */
	short len;			  /* transfer length	 */
	short start_prefix;		  /* first tcw prefix	 */
	short prefix;			  /* tcw prefix		 */
	char *real_addr;
	char *vir_addr;			  /* passed virtual addr	 */
	char *max_vir_addr;		  /* upper bound of transfer */
	int start_addr;
	char *boundary_addr;		  /* 2k alligned addr	 */
	int ch;				  /* 8237 subchannel	 */
	char operation;			  /* read or write	 */
	int chan;
	char mode = 0;			  /* mode byte		 */
	char cmd = 0;			  /* command byte		 */
	char mask = 0;			  /* mask byte		 */
	int i, done;
	int status;			  /* returned status	 */
	int temp1, temp2;
	short tcw_num;			  /* tcw number		 */

	chan = dm->ic_channel;		  /* find which channel 	 */
	ch = dhp->dh_subchan;		  /* find which subchan	 */
	vir_addr = bp->b_un.b_addr;
	real_addr = real_buf_addr(bp, vir_addr);
	start_addr = (int)real_addr;
	/* setup IOCC registers		 */
	*dmra = (*dmra) & (~(0x80 >> ch));
	/* set up 8237 controller	 */
	/* determine operation read/write */
	operation = (char)(bp->b_flags & B_READ) ? DM_WRITE : DM_READ;
	mode = (mode | (dm->ic_transfer) | operation | ch);
	regp->dm_mode = mode;
	displacement = (short)start_addr & 0x000007ff;

	len = (bp->b_bcount) - 1;
	if (len <= 0) {
		bp->b_error = -10;
		printf(" length is <= 0  \n");
		return (-1);
	}
	;
	/* base, low order byte first   */
	regp->dm_ff = 0;
	regp->dm_chan[ch].dm_base = displacement;
	regp->dm_chan[ch].dm_base = displacement >> 8;

	regp->dm_ff = 0;
	regp->dm_chan[ch].dm_count = len;
	regp->dm_chan[ch].dm_count = len >> 8;
	/* set up corresponding TCWs */
	tcw_num = ((len + 1) / PAGESIZE) + 1; /* find # of tcw to set up   */
	if (displacement != 0)
		tcw_num++;
	if (tcw_num > 32) {
		bp->b_error = -12;
		printf(" more than 32 TCWs => boundary error \n");
		return (-1);
	}
	;
	/* Calculate  TCWs */
	/* write prefixes in TCWs    */

	max_vir_addr = vir_addr + len;	  /* upper virtual addr */

	i = 0;
	done = 0;
	boundary_addr = (char *)((int)vir_addr & ~(PAGESIZE - 1));
	while (i < 33 && done != 1) {
		real_addr = real_buf_addr(bp, boundary_addr);
		prefix = (short)(((int)real_addr) >> 11);
		*(short *)(TCW_BASE + (2 * (chan * 64 + i))) =
		    prefix | RSC_ACC | REAL_ACC;

		boundary_addr += PAGESIZE;
		if (boundary_addr >= max_vir_addr)
			done = 1;
		i++;			  /* next TCW index */

	}
	;
	/* call xxdgo */
	(*dm->ic_driver->idr_dgo)(dm);
	return (0);			  /* everything is ok */

}


int dma_go(channel)
	int channel;
{

	register struct iocc_hd *dhp = &iocc_hdr[channel];
	register struct dma_8bit_device *regp = dhp->dh_8237_reg;
	/* enable specific channel */
	regp->dm_smask = dhp->dh_subchan;
	return;
}

dmaint()
{
	return(0);
}


dma_init()
{
	register struct dma_8bit_device *regp = (struct dma_8bit_device *)CTL1_BASE;

/*
initialize per channel data structure. Take care of 8237 subchannel
assignement (weird assignment).
note that 16 bit support is not included .
*/
	static int sub_chan[9] = {
		2, 1, 0, 3
	};
	int i;

	for (i = 0; i < 4; i++) {
		iocc_hdr[i].dh_subchan = sub_chan[i];
		iocc_hdr[i].dh_state = 0; /* mark them not busy */
		iocc_hdr[i].dh_8237_reg = regp;	/* hardware registers */
		/*  *ctl1_smask = 0x4 |i;				/* mask 8237 */
		regp->dm_smask = 0x4 | i;
		/* *ctl1_req   =i;           /* disable software dma request  */
		regp->dm_req = i;
	}
	*crrb = (*crrb) | DM_CTL1_ENABLE | 0x20; /* arbitrer enable */
	/* *ctl1_cmd= 0x20;          	     /* enable 8237  */
	regp->dm_cmd = 0x20;
	return;

}


/*  This routine will be called from the requesting device interrupt handler*/

dma_done(channel)
	int channel;
{

	register struct iocc_hd *dhp = &iocc_hdr[channel];
	register struct dma_8bit_device *regp = dhp->dh_8237_reg;
	char ch = dhp->dh_subchan;
	char status;
	/* disable specific channel */
	status = *ctl1_cmd;
	regp->dm_smask = 0x4 | ch;
	dhp->dh_state = 0;		  /* not busy */
/* check if more work need to be done */

	if (dhp->dh_actf != NULL)
		dmastart(channel);
	return;

}

unsigned
dmaminphys(bp)
	struct buf *bp;
{
	if (bp->b_bcount > DM_MAXPHYS)
		bp->b_bcount = DM_MAXPHYS;
}

ctl2_setup()
{
/* not implemented yet */
}


copro_setup()
{
/* not implemented yet */
}


;
