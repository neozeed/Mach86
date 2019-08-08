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
 * Copyright (c) 1982 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)autoconf.c	6.3 (Berkeley) 8/2/85
 */
#if	CMU

/*
 **********************************************************************
 * HISTORY
 * 11-May-86  Avadis Tevanian (avie) at Carnegie-Mellon University
 *	Support for BI bus, in particular, KDB50 as boot device.
 *
 * 01-Jul-85  Mike Accetta (mja) at Carnegie-Mellon University
 *	CS_VAXM:  added support for MicroVax-I and -II (a la ULTRIX-32).
 *
 * 15-Sep-84  Robert V Baron (rvb) at Carnegie-Mellon University
 *	two basic major changes:
 * 1)	Coded has been added to support MA780
 * 2)	uba0, mba0, now correspond to the first unibus, (massbus)
 *	independent of wether they are tr3 and tr8.  The code mimic's
 *	autoconf of the real OS
 **********************************************************************
 */

#include "../h/features.h"
#endif	CMU

#include "../machine/pte.h"

#include "../h/param.h"

#include "../vax/cpu.h"
#include "../vax/nexus.h"
#include "../vaxuba/ubareg.h"
#if	CS_VAXM
#include "../vaxuba/qbreg.h"
#endif	CS_VAXM
#include "../vaxmba/mbareg.h"
#if	CS_BOOT
#include "../vax/ma780reg.h"
#include "../vax/ma780var.h"
#include "../vax/bi_node.h"
#endif	CS_BOOT
#include "../vax/mtpr.h"

#include "savax.h"

#define	UTR(i)	((struct uba_regs *)(NEX780+(i)))
#define	UMA(i)	((caddr_t)UMEM780(i))
#define	MTR(i)	((struct mba_regs *)(NEX780+(i)))

#if	CS_BOOT
#define MTC ((struct mpm_regs *) -1)

struct	mpm_regs *mpmaddr[4] = { MTC, MTC, MTC, MTC };
/*
 * NOTE!!!!: umaddr780 must be the address of the device
 * registers i.e. 0x20100000+0760000+0x40000*i
 */
#endif	CS_BOOT
struct	uba_regs *ubaddr780[] = { UTR(3), UTR(4), UTR(5), UTR(6) };
caddr_t	umaddr780[] = { UMA(0), UMA(1), UMA(2), UMA(3) };
struct	mba_regs *mbaddr780[] = { MTR(8), MTR(9), MTR(10), MTR(11) };

#undef	UTR
#undef	UMA
#undef	MTR

#define	UTR(i)	((struct uba_regs *)(NEX750+(i)))
#define	UMA(i)	((caddr_t)UMEM750(i))
#define	MTR(i)	((struct mba_regs *)(NEX750+(i)))

struct	uba_regs *ubaddr750[] = { UTR(8), UTR(9) };
caddr_t	umaddr750[] = { UMA(0), UMA(1) };
struct	mba_regs *mbaddr750[] = { MTR(4), MTR(5), MTR(6), MTR(7) };

#undef	UTR
#undef	UMA
#undef	MTR

#define	UTR(i)	((struct uba_regs *)(NEX730+(i)))
#define	UMA	((caddr_t)UMEM730)

struct	uba_regs *ubaddr730[] = { UTR(3) };
caddr_t	umaddr730[] = { UMA };

#undef	UTR
#undef	UMA
#if	CS_VAXM

caddr_t qmaddrMI[] = { (caddr_t)0x20000000 };
struct uba_regs *qbaddrMII[] = { &((struct qb_regs *)NEXMII)->qb_uba.uba };
caddr_t qmaddrMII[] = { (caddr_t)0x20000000-0760000 };
#endif	CS_VAXM

#if	CS_BOOT
static int nmba, nuba, nmcr, nmpm;

#endif	CS_BOOT
configure()
{
	union cpusid cpusid;
#if	CS_BOOT
	int i;
#else	CS_BOOT
	int nmba, nuba, i;
#endif	CS_BOOT

	cpusid.cpusid = mfpr(SID);
	cpu = cpusid.cpuany.cp_type;
	switch (cpu) {

	case KA_820:
		ubaddr = ubaddr780;
		umaddr = umaddr780;
		probe_bi();
		break;
	case VAX_8600:
	case VAX_780:
		mbaddr = mbaddr780;
		ubaddr = ubaddr780;
		umaddr = umaddr780;
#if	CS_BOOT
		nmba = nuba = nmcr = nmpm = 0;
#if 0
		probe780nexus();
#endif 0
#else	CS_BOOT
		nmba = sizeof (mbaddr780) / sizeof (mbaddr780[0]);
		nuba = sizeof (ubaddr780) / sizeof (ubaddr780[0]);
#endif	CS_BOOT
		break;

	case VAX_750:
		mbaddr = mbaddr750;
		ubaddr = ubaddr750;
		umaddr = umaddr750;
		nmba = sizeof (mbaddr750) / sizeof (mbaddr750[0]);
		nuba = 0;
		break;

	case VAX_730:
		ubaddr = ubaddr730;
		umaddr = umaddr730;
		nmba = nuba = 0;
		break;
#if	CS_VAXM
 
	case VAX_MI:
		umaddr = qmaddrMI;
		nmba = nuba = 0;
		break;

	case VAX_MII:
		ubaddr = qbaddrMII;
		umaddr = qmaddrMII;
		nmba = nuba = 0;
		break;

#endif	CS_VAXM
	}
	/*
	 * Forward into the past...
	 */
/*
	for (i = 0; i < nmba; i++)
		if (!badloc(mbaddr[i]))
			mbaddr[i]->mba_cr = MBCR_INIT;
*/
#if	KA820
	if (cpu != KA_820)
#endif	KA820
	for (i = 0; i < nuba; i++)
#if	CS_VAXM
		if (!badloc(ubaddr[i], sizeof(int)))
#else	CS_VAXM
		if (!badloc(ubaddr[i]))
#endif	CS_VAXM
			ubaddr[i]->uba_cr = UBACR_ADINIT;
#if	CS_VAXM
	switch (cpu)
	{
	    case VAX_750:
	    case VAX_730:
		mtpr(IUR, 0);
		break;
	    case VAX_MI:
	    {
		u_short *qmcsr;

		mtpr(IUR, 0);
		for (qmcsr = (u_short *)0x20001440;
		     qmcsr < (u_short *)0x20001460;
		     qmcsr++)
			if (!badloc(qmcsr, sizeof(*qmcsr)))
				*qmcsr = 1;
		break;
	    }

	    case VAX_MII:
	    {
		u_int *qmbase;

		mtpr(IUR, 0);
		/* enable parity detection */
		*((u_int *)0x20080004) = 1;
		/* initialize Q-bus map */
		for (qmbase = (u_int *)0x20088000, i=0;
		     i < 8192;
		     qmbase++, i++)
			*qmbase = i;
		/* who knows? */
		*((u_short *)0x20001f40) = 0x20;
		break;
	    }
	}
#else	CS_VAXM
	if ((cpu != VAX_780) && (cpu != VAX_8600))
		mtpr(IUR, 0);
#endif	CS_VAXM
	/* give unibus devices a chance to recover... */
	if (nuba > 0)
		DELAY(2000000);
#if	CS_VAXM
	if (!badloc(0x20001e80, sizeof(short)))
		qv_init(0x20001e80);
#endif	CS_VAXM
}

#if	CS_BOOT
/*
 * Probe nexus space, finding the interconnects
 * and setting up and probing mba's and uba's for devices.
 */

#if 0

probe780nexus()
{
	struct nexus *phys_addr_nexusp = (struct nexus *) NEX780;
	union nexcsr nexcsr;
	int i;
	int	nexnum;

	nexnum = 0;

	for (; nexnum < NNEX780; nexnum++, phys_addr_nexusp++) {
#if	CS_VAXM
		if (badloc((caddr_t)phys_addr_nexusp, sizeof(int))) {
#else	CS_VAXM
		if (badloc((caddr_t)phys_addr_nexusp)) {
#endif	CS_VAXM
			continue;
		}

		nexcsr = phys_addr_nexusp->nexcsr;
		if (nexcsr.nex_csr&NEX_APD)
			continue;

		switch (nexcsr.nex_type) {
		case NEX_MBA:
			if (nmba < MAXNMBA) {
				mbaddr[nmba++] = (struct mba_regs *)phys_addr_nexusp;
			}
			break;
		case NEX_UBA0:
		case NEX_UBA1:
		case NEX_UBA2:
		case NEX_UBA3:
			if (nuba < 4) {
				i = nexcsr.nex_type - NEX_UBA0;
				umaddr[nuba] = (caddr_t) UMEM780(i);
				printf("\tuba%d at tr%d umem%d (%x)\n",
					nuba, nexnum, i, umaddr[i]);
				ubaddr[nuba++] = (struct uba_regs *)phys_addr_nexusp;
			}
			break;

		case NEX_MPM0:
		case NEX_MPM1:
		case NEX_MPM2:
		case NEX_MPM3:
			if (nmpm >= 4)
				break;
			mpmaddr[nmpm++] = (struct mpm_regs *)phys_addr_nexusp;

			{ register struct mpm_regs *mpmp = 
					(struct mpm_regs *) phys_addr_nexusp;

				mpmp->mpm_cr &= ~(MPCR_IVDTACK|MPCR_MSTRIE);
				mpmp->mpm_sr &= ~ MPSR_INVALDIS;
				mpmp->mpm_invld = (mpmp->mpm_invld & 
						    ~(MPADDR_MSK<<MPADDR_SFT))
						   | (((MPMEM_ORIGIN/0x40000)
							 | 8 * (nexnum - 2))
							  << MPADDR_SFT);
			}
			break;
		default:
			printf("nexus %d, type 0x%x ignored.\n", nexnum,
				nexcsr.nex_type);
		}
	}
}
#endif	0
#endif	CS_BOOT

/*
 *	Probe BI node space looking for kdb50.
 */

probe_bi()
{
	register struct bi_node *bi;
	caddr_t		phys;
	int		p, i;
	int		nodenum;
	short		dev_type;
	int		nkdb = 0;
	extern caddr_t	udaddr;		/* for uda driver */
	extern		iskdb50;

	phys = BI_ADDR(0);
	nodenum = 0;

	for (; nodenum < BI_MAX_NODE; nodenum++, phys += BI_NODE_SIZE) {
		if (badloc((caddr_t)phys, sizeof(int))) {
			continue;
		}
		bi = (struct bi_node *) phys;
		dev_type = bi->dev_type;
		bi->ber = 0x3fff0007;		/* clear bus error register */

		switch (dev_type) {
		case BI_UBA:
			if (nuba < 4) {
				ubaddr[nuba] = (struct uba_regs *)(0x20000000 + 0x2000*nodenum);
				umaddr[nuba] = (caddr_t)(0x20400000 + 40000*nodenum);
			}
			nuba++;
			break;
		case BI_KDB50:
			if (nkdb == 0) {
				int i;

				bi->intrdes = 0xffff;
				for (i = 0; i < 10000; i++) {
					if (i == 1)
						bi->bicsr |= BICSR_NRST;
				}
				udaddr = phys + 0xf2;
				iskdb50 = 1;
			}
			nkdb++;
			break;
		}
	}
}
