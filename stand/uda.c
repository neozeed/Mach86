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
/*	uda.c	6.1	83/07/29	*/

/*
 * UDA50/RAxx disk device driver
 *
 **********************************************************************
 * HISTORY
 * 11-May-86  Avadis Tevanian (avie) at Carnegie-Mellon University
 *	Support for KDB50 controller on a BI bus.  Note that conditional
 *	compilation was not used.  The main reason was because it was
 *	difficult to fit bootra in 7.5K with conditional compilation.
 *	Besides, I'm getting sick and tired of conditional compilation
 *	for features we obviously always want.
 *
 * 01-Jul-85  Mike Accetta (mja) at Carnegie-Mellon University
 *	CS_VAXM:  changed to allow 22-bit Q-bus address in buffer
 *	address; picked up bug fix from ULTRIX-32 which only
 *	initializes the controller and each unit once (allowing
 *	multiple uses of the driver).
 *
 **********************************************************************
 */

#include "../h/features.h"

#include "../machine/pte.h"

#include "../h/param.h"
#include "../h/inode.h"
#include "../h/fs.h"

#include "saio.h"
#include "savax.h"

/*
 * Parameters for the communications area
 */
#define	NRSPL2	0
#define	NCMDL2	0
#define	NRSP	(1<<NRSPL2)
#define	NCMD	(1<<NCMDL2)

#define NBI 1

#include "../vaxuba/udareg.h"
#include "../vaxuba/ubareg.h"
#include "../vax/mscp.h"

#include "../vax/bi_node.h"

u_short udastd[] = { 0772150 };

struct iob	cudbuf;

struct udadevice *udaddr = 0;
int	iskdb50 = 0;

struct uda {
	struct udaca	uda_ca;
	struct mscp	uda_rsp;
	struct mscp	uda_cmd;
} uda;

struct uda *ud_ubaddr;			/* Unibus address of uda structure */

int uda_off[] = { 0, 15884, 0, -1, -1, -1, 49324, 131404 };

struct mscp *udcmd();

raopen(io)
	register struct iob *io;
{
	register struct mscp *mp;
	short *wr;
	int i;

#if	CS_VAXM
	static char online[8] = {0};
	static int init = 0;

	if (init == 0)
	{
#endif	CS_VAXM
	if (udaddr == 0)
		udaddr = (struct udadevice *)ubamem(io->i_unit, udastd[0]);
	if (ud_ubaddr == 0) {
#if	CS_VAXM
		cudbuf.i_unit = io->i_unit;
#endif	CS_VAXM
		cudbuf.i_ma = (caddr_t)&uda;
		cudbuf.i_cc = sizeof(uda);
		if (iskdb50)
			ud_ubaddr = (struct uda *) &uda;
		else
			ud_ubaddr = (struct uda *)ubasetup(&cudbuf, 2);
	}
	udaddr->udaip = 0;
	if (iskdb50)
		wr = &udaddr->udawr;
	else
		wr = &udaddr->udasa;
	while ((udaddr->udasa & UDA_STEP1) == 0)
		;
	*wr = UDA_ERR;
	while ((udaddr->udasa & UDA_STEP2) == 0)
		;
	*wr = (short)&ud_ubaddr->uda_ca.ca_ringbase;
	while ((udaddr->udasa & UDA_STEP3) == 0)
		;
	*wr = (short)(((int)&ud_ubaddr->uda_ca.ca_ringbase) >> 16);
	while ((udaddr->udasa & UDA_STEP4) == 0)
		;
	*wr = UDA_GO;
	uda.uda_ca.ca_rspdsc[0] = (long)&ud_ubaddr->uda_rsp.mscp_cmdref;
	uda.uda_ca.ca_cmddsc[0] = (long)&ud_ubaddr->uda_cmd.mscp_cmdref;
	uda.uda_cmd.mscp_cntflgs = 0;
	if (udcmd(M_OP_STCON) == 0) {
		_stop("ra: open error, STCON");
		return;
	}
#if	CS_VAXM
	init++;
	}
#endif	CS_VAXM
	uda.uda_cmd.mscp_unit = io->i_unit&7;
#if	CS_VAXM
	if (online[uda.uda_cmd.mscp_unit] == 0)
	{
#endif	CS_VAXM
	if (udcmd(M_OP_ONLIN) == 0) {
		_stop("ra: open error, ONLIN");
		return;
	}
#if	CS_VAXM
	online[uda.uda_cmd.mscp_unit]++;
	}
#endif	CS_VAXM
	if (io->i_boff < 0 || io->i_boff > 7 || uda_off[io->i_boff] == -1)
		_stop("ra: bad unit");
	io->i_boff = uda_off[io->i_boff];
}

struct mscp *
udcmd(op)
	int op;
{
	struct mscp *mp;
	int i;

	uda.uda_cmd.mscp_opcode = op;
	uda.uda_rsp.mscp_header.uda_msglen = sizeof (struct mscp);
	uda.uda_cmd.mscp_header.uda_msglen = sizeof (struct mscp);
	uda.uda_ca.ca_rspdsc[0] |= UDA_OWN|UDA_INT;
	uda.uda_ca.ca_cmddsc[0] |= UDA_OWN|UDA_INT;
	i = udaddr->udaip;
	for (;;) {
		if (uda.uda_ca.ca_cmdint)
			uda.uda_ca.ca_cmdint = 0;
		if (uda.uda_ca.ca_rspint)
			break;
	}
	uda.uda_ca.ca_rspint = 0;
	mp = &uda.uda_rsp;
	if (mp->mscp_opcode != (op|M_OP_END) ||
	    (mp->mscp_status&M_ST_MASK) != M_ST_SUCC)
		return(0);
	return(mp);
}

rastrategy(io, func)
	register struct iob *io;
{
	register struct mscp *mp;
	int ubinfo;


	if (!iskdb50)
		ubinfo = ubasetup(io, 1);
	mp = &uda.uda_cmd;
	mp->mscp_lbn = io->i_bn;
	mp->mscp_unit = io->i_unit&7;
	mp->mscp_bytecnt = io->i_cc;
#if	CS_VAXM
	if (iskdb50)
		mp->mscp_buffer = (int)io->i_ma;
	else
		mp->mscp_buffer = (ubinfo & 0x3fffff) | (((ubinfo>>28)&0xf)<<24);
#else	CS_VAXM
	mp->mscp_buffer = (ubinfo & 0x3ffff) | (((ubinfo>>28)&0xf)<<24);
#endif	CS_VAXM
	if ((mp = udcmd(func == READ ? M_OP_READ : M_OP_WRITE)) == 0) {
		printf("ra: I/O error\n");
		if (!iskdb50)
			ubafree(io, ubinfo);
		return(-1);
	}
	if (!iskdb50)
		ubafree(io, ubinfo);
	return(io->i_cc);
}

/*ARGSUSED*/
raioctl(io, cmd, arg)
	struct iob *io;
	int cmd;
	caddr_t arg;
{

	return (ECMD);
}
