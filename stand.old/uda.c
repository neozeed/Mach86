/*	uda.c	6.1	83/07/29	*/

/*
 * UDA50/RAxx disk device driver
 *
 **********************************************************************
 * HISTORY
 * 01-Jul-85  Mike Accetta (mja) at Carnegie-Mellon University
 *	CS_VAXM:  changed to allow 22-bit Q-bus address in buffer
 *	address; picked up bug fix from ULTRIX-32 which only
 *	initializes the controller and each unit once (allowing
 *	multiple uses of the driver).
 *
 **********************************************************************
 */
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

#include "../vaxuba/udareg.h"
#include "../vaxuba/ubareg.h"
#include "../vax/mscp.h"

u_short udastd[] = { 0772150 };

struct iob	cudbuf;

struct udadevice *udaddr = 0;

struct uda {
	struct udaca	uda_ca;
	struct mscp	uda_rsp;
	struct mscp	uda_cmd;
} uda;

struct uda *ud_ubaddr;			/* Unibus address of uda structure */

#ifdef	CMU
int uda_off[] = { 0, 15884, 0, -1, 0, 50714, 25650, 15884 };
#else	CMU
int uda_off[] = { 0, 15884, 0, -1, -1, -1, 49324, 131404 };
#endif	CMU

struct mscp *udcmd();

raopen(io)
	register struct iob *io;
{
	register struct mscp *mp;
	int i;

#ifdef	CMU
	/* CS_VAXM */
	static char online[8] = {0};
	static int init = 0;

	if (init == 0)
	{
	/* CS_VAXM */
#endif	CMU
	if (udaddr == 0)
		udaddr = (struct udadevice *)ubamem(io->i_unit, udastd[0]);
	if (ud_ubaddr == 0) {
#ifdef	CMU
		/* CS_VAXM */
		cudbuf.i_unit = io->i_unit;
		/* CS_VAXM */
#endif	CMU
		cudbuf.i_ma = (caddr_t)&uda;
		cudbuf.i_cc = sizeof(uda);
		ud_ubaddr = (struct uda *)ubasetup(&cudbuf, 2);
	}
	udaddr->udaip = 0;
	while ((udaddr->udasa & UDA_STEP1) == 0)
		;
	udaddr->udasa = UDA_ERR;
	while ((udaddr->udasa & UDA_STEP2) == 0)
		;
	udaddr->udasa = (short)&ud_ubaddr->uda_ca.ca_ringbase;
	while ((udaddr->udasa & UDA_STEP3) == 0)
		;
	udaddr->udasa = (short)(((int)&ud_ubaddr->uda_ca.ca_ringbase) >> 16);
	while ((udaddr->udasa & UDA_STEP4) == 0)
		;
	udaddr->udasa = UDA_GO;
	uda.uda_ca.ca_rspdsc[0] = (long)&ud_ubaddr->uda_rsp.mscp_cmdref;
	uda.uda_ca.ca_cmddsc[0] = (long)&ud_ubaddr->uda_cmd.mscp_cmdref;
	uda.uda_cmd.mscp_cntflgs = 0;
	if (udcmd(M_OP_STCON) == 0) {
		_stop("ra: open error, STCON");
		return;
	}
#ifdef	CMU
	/* CS_VAXM */
	init++;
	}
	/* CS_VAXM */
#endif	CMU
	uda.uda_cmd.mscp_unit = io->i_unit&7;
#ifdef	CMU
	/* CS_VAXM */
	if (online[uda.uda_cmd.mscp_unit] == 0)
	{
	/* CS_VAXM */
#endif	CMU
	if (udcmd(M_OP_ONLIN) == 0) {
		_stop("ra: open error, ONLIN");
		return;
	}
#ifdef	CMU
	/* CS_VAXM */
	online[uda.uda_cmd.mscp_unit]++;
	}
	/* CS_VAXM */
#endif	CMU
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

	ubinfo = ubasetup(io, 1);
	mp = &uda.uda_cmd;
	mp->mscp_lbn = io->i_bn;
	mp->mscp_unit = io->i_unit&7;
	mp->mscp_bytecnt = io->i_cc;
#ifdef	CMU
	/* CS_VAXM */
	mp->mscp_buffer = (ubinfo & 0x3fffff) | (((ubinfo>>28)&0xf)<<24);
	/* CS_VAXM */
#else	CMU
	mp->mscp_buffer = (ubinfo & 0x3ffff) | (((ubinfo>>28)&0xf)<<24);
#endif	CMU
	if ((mp = udcmd(func == READ ? M_OP_READ : M_OP_WRITE)) == 0) {
		printf("ra: I/O error\n");
		ubafree(io, ubinfo);
		return(-1);
	}
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
