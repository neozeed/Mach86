/*	autoconf.c	6.1	83/07/29	*/

/*
 **********************************************************************
 * HISTORY
 * 01-Jul-85  Mike Accetta (mja) at Carnegie-Mellon University
 *	CS_VAXM:  added support for MicroVax-I and -II (a la ULTRIX-32).
 *
 **********************************************************************
 */

#include "../machine/pte.h"

#include "../h/param.h"

#include "../vax/cpu.h"
#include "../vax/nexus.h"
#include "../vaxuba/ubareg.h"
#ifdef	CMU
/* CS_VAXM */
#include "../vaxuba/qbreg.h"
/* CS_VAXM */
#endif	CMU
#include "../vaxmba/mbareg.h"
#include "../vax/mtpr.h"

#include "savax.h"

#define	UTR(i)	((struct uba_regs *)(NEX780+(i)))
#define	UMA(i)	((caddr_t)UMEM780(i))
#define	MTR(i)	((struct mba_regs *)(NEX780+(i)))

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
#ifdef	CMU
/* CS_VAXM */
 
caddr_t qmaddrMI[] = { (caddr_t)0x20000000 };

struct uba_regs *qbaddrMII[] = { &((struct qb_regs *)NEXMII)->qb_uba.uba };
caddr_t qmaddrMII[] = { (caddr_t)0x20000000-0760000 };
/* CS_VAXM */
#endif	CMU

configure()
{
	union cpusid cpusid;
	int nmba, nuba, i;

	cpusid.cpusid = mfpr(SID);
	cpu = cpusid.cpuany.cp_type;
	switch (cpu) {

	case VAX_780:
		mbaddr = mbaddr780;
		ubaddr = ubaddr780;
		umaddr = umaddr780;
		nmba = sizeof (mbaddr780) / sizeof (mbaddr780[0]);
		nuba = sizeof (ubaddr780) / sizeof (ubaddr780[0]);
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
#ifdef	CMU
	/* CS_VAXM */
 
	case VAX_MI:
		umaddr = qmaddrMI;
		nmba = nuba = 0;
		break;

	case VAX_MII:
		ubaddr = qbaddrMII;
		umaddr = qmaddrMII;
		nmba = nuba = 0;
		break;
	/* CS_VAXM */
#endif	CMU
	}
	/*
	 * Forward into the past...
	 */
/*
	for (i = 0; i < nmba; i++)
		if (!badloc(mbaddr[i]))
			mbaddr[i]->mba_cr = MBCR_INIT;
*/
	for (i = 0; i < nuba; i++)
#ifdef	CMU
		/* CS_VAXM */
		if (!badloc(ubaddr[i], sizeof(int)))
		/* CS_VAXM */
#else	CMU
		if (!badloc(ubaddr[i]))
#endif	CMU
			ubaddr[i]->uba_cr = UBACR_ADINIT;
#ifdef	CMU
	/* CS_VAXM */
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
	/* CS_VAXM */
#else	CMU
	if (cpu != VAX_780)
		mtpr(IUR, 0);
#endif	CMU
	/* give unibus devices a chance to recover... */
	if (nuba > 0)
		DELAY(2000000);
#ifdef	CMU
	/* CS_VAXM */
	if (!badloc(0x20001e80, sizeof(short)))
		qv_init(0x20001e80);
	/* CS_VAXM */
#endif	CMU
}
