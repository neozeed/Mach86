/*
 * Copyright (c) 1982,1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)autoconf.c	7.1 (Berkeley) 6/6/86
 */

/*
 * Setup the system to run on the current machine.
 *
 * Configure() is called at boot time and initializes the uba and mba
 * device tables and the memory controller monitoring.  Available
 * devices are determined (from possibilities mentioned in ioconf.c),
 * and the drivers are initialized.
 */

#include "mba.h"
#include "uba.h"

#include "pte.h"

#include "param.h"
#include "systm.h"
#include "map.h"
#include "buf.h"
#include "dk.h"
#include "vm.h"
#include "conf.h"
#include "dmap.h"
#include "reboot.h"

#include "cpu.h"
#include "mem.h"
#include "mtpr.h"
#include "nexus.h"
#include "scb.h"
#include "ioa.h"
#include "../vaxmba/mbareg.h"
#include "../vaxmba/mbavar.h"
#include "../vaxuba/ubareg.h"
#include "../vaxuba/ubavar.h"

/*
 * The following several variables are related to
 * the configuration process, and are used in initializing
 * the machine.
 */
int	cold;		/* if 1, still working on cold-start */
int	nexnum;		/* current nexus number */
int	nsbi;		/* current sbi number */
int	dkn;		/* number of iostat dk numbers assigned so far */
int	cpuspeed = 1;	/* relative cpu speed */

/*
 * Addresses of the (locore) routines which bootstrap us from
 * hardware traps to C code.  Filled into the system control block
 * as necessary.
 */
#if NMBA > 0
int	(*mbaintv[4])() =	{ Xmba0int, Xmba1int, Xmba2int, Xmba3int };
#if NMBA > 4
	Need to expand the table for more than 4 massbus adaptors
#endif
#endif
#if defined(VAX780) || defined(VAX8600)
int	(*ubaintv[])() =
{
	Xua0int, Xua1int, Xua2int, Xua3int,
#if NUBA > 4
	Xua4int, Xua5int, Xua6int, Xua7int,
#endif
#if NUBA > 8
	Need to expand the table for more than 8 unibus adaptors
#endif
};
#endif

/*
 * This allocates the space for the per-uba information,
 * such as buffered data path usage.
 */
struct	uba_hd uba_hd[NUBA];

/*
 * Determine mass storage and memory configuration for a machine.
 * Get cpu type, and then switch out to machine specific procedures
 * which will probe adaptors to see what is out there.
 */
configure()
{
	union cpusid cpusid;
	register struct percpu *ocp;
	register int *ip;
	extern char Sysbase[];

	cpusid.cpusid = mfpr(SID);
	for (ocp = percpu; ocp->pc_cputype; ocp++)
		if (ocp->pc_cputype == cpusid.cpuany.cp_type) {
			cpuspeed = ocp->pc_cpuspeed;
			probeio(ocp);
			/*
			 * Write protect the scb and UNIBUS interrupt vectors.
			 * It is strange that this code is here, but this is
			 * as soon as we are done mucking with it, and the
			 * write-enable was done in assembly language
			 * to which we will never return.
			 */
			ip = (int *)Sysmap + 1; *ip &= ~PG_PROT; *ip |= PG_KR;
			ip++; *ip &= ~PG_PROT; *ip |= PG_KR;
#if NUBA > 1
			ip++; *ip &= ~PG_PROT; *ip |= PG_KR;
#endif
			mtpr(TBIS, Sysbase);
#if GENERIC
			if ((boothowto & RB_ASKNAME) == 0)
				setroot();
			setconf();
#else
			setroot();
#endif
			/*
			 * Configure swap area and related system
			 * parameter based on device(s) used.
			 */
			swapconf();
			cold = 0;
			memenable();
			return;
		}
	printf("cpu type %d not configured\n", cpusid.cpuany.cp_type);
	asm("halt");
}

/*
 * Probe the main IO bus(es).
 * The percpu structure gives us a handle on the addresses and/or types.
 */
probeio(pcpu)
	register struct percpu *pcpu;
{
	register struct iobus *iob;
	int ioanum;

	ioanum = 0;
	for (iob = pcpu->pc_io; ioanum < pcpu->pc_nioa; ioanum++, iob++) {

		switch (iob->io_type) {

#if VAX780 || VAX750 || VAX730 || VAX630
		case IO_SBI780:
		case IO_CMI750:
		case IO_XXX730:
		case IO_QBUS:
			probenexi((struct nexusconnect *)iob->io_details);
			break;
#endif

#if VAX8600
		case IO_ABUS:
			probe_Abus(ioanum, iob);
			break;
#endif
		default:
			if (iob->io_addr) {
			    printf(
		"IO adaptor %d, type %d, at address 0x%x is unsupported\n",
				ioanum, iob->io_type, iob->io_addr);
			} else
			    printf("IO adaptor %d, type %d, is unsupported\n",
				ioanum, iob->io_type);
			break;
		}
	}
}

#if VAX8600
probe_Abus(ioanum, iob)
	register struct iobus *iob;
{
	register struct ioa *ioap;
	union ioacsr ioacsr;
	int type;
	struct sbia_regs *sbiaregs;

	ioap = &ioa[ioanum];
	ioaccess(iob->io_addr, Ioamap[ioanum], iob->io_size);
	if (badaddr((caddr_t)ioap, 4))
		return;
	ioacsr.ioa_csr = ioap->ioacsr.ioa_csr;
	type = ioacsr.ioa_type & IOA_TYPMSK;

	switch (type) {

	case IOA_SBIA:
		printf("SBIA%d at IO adaptor %d address 0x%x\n",
		    nsbi, ioanum, iob->io_addr);
		probenexi((struct nexusconnect *)iob->io_details);
		nsbi++;
		sbiaregs = (struct sbia_regs *)ioap;
		sbiaregs->sbi_errsum = -1;
		sbiaregs->sbi_error = 0x1000;
		sbiaregs->sbi_fltsts = 0xc0000;
		break;

	default:
		printf("IOA%d at address 0x%x is unsupported (type = 0x%x)\n",
		    ioanum, iob->io_addr, ioacsr.ioa_type);
		break;
	}
}
#endif

/*
 * Probe nexus space, finding the interconnects
 * and setting up and probing mba's and uba's for devices.
 */
/*ARGSUSED*/
probenexi(pnc)
	register struct nexusconnect *pnc;
{
	register struct nexus *nxv;
	struct nexus *nxp = pnc->psb_nexbase;
	union nexcsr nexcsr;
	int i;
	
	nexnum = 0, nxv = &nexus[nsbi * NNEXSBI];
	for (; nexnum < pnc->psb_nnexus; nexnum++, nxp++, nxv++) {
		ioaccess((caddr_t)nxp, Nexmap[nsbi * NNEXSBI + nexnum],
		     sizeof(struct nexus));
		if (badaddr((caddr_t)nxv, 4))
			continue;
		if (pnc->psb_nextype && pnc->psb_nextype[nexnum] != NEX_ANY)
			nexcsr.nex_csr = pnc->psb_nextype[nexnum];
		else
			nexcsr = nxv->nexcsr;
		if (nexcsr.nex_csr&NEX_APD)
			continue;
		switch (nexcsr.nex_type) {

		case NEX_MBA:
			printf("mba%d at tr%d\n", nummba, nexnum);
			if (nummba >= NMBA) {
				printf("%d mba's", nummba++);
				goto unconfig;
			}
#if NMBA > 0
			mbafind(nxv, nxp);
			nummba++;
#endif
			break;

		case NEX_UBA0:
		case NEX_UBA1:
		case NEX_UBA2:
		case NEX_UBA3:
			printf("uba%d at tr%d\n", numuba, nexnum);
#if VAX750
			if (numuba >= 2 && cpu == VAX_750) {
				printf("More than 2 UBA's");
				goto unsupp;
			}
#endif
			if (numuba >= NUBA) {
				printf("%d uba's", ++numuba);
				goto unconfig;
			}
#if defined(VAX780) || defined(VAX8600)
			if ((cpu == VAX_780) || (cpu == VAX_8600))
				setscbnex(ubaintv[numuba]);
#endif
			i = nexcsr.nex_type - NEX_UBA0;
			unifind((struct uba_regs *)nxv, (struct uba_regs *)nxp,
			    umem[numuba], pnc->psb_umaddr[i], UMEMmap[numuba],
			    pnc->psb_haveubasr);
#if defined(VAX780) || defined(VAX8600)
			if ((cpu == VAX_780) || (cpu == VAX_8600))
				((struct uba_regs *)nxv)->uba_cr =
				    UBACR_IFS|UBACR_BRIE|
				    UBACR_USEFIE|UBACR_SUEFIE|
				    (((struct uba_regs *)nxv)->uba_cr&0x7c000000);
#endif
			numuba++;
			break;

		case NEX_DR32:
		/* there can be more than one... are there other codes??? */
			printf("dr32");
			goto unsupp;

		case NEX_MEM4:
		case NEX_MEM4I:
		case NEX_MEM16:
		case NEX_MEM16I:
			printf("mcr%d at tr%d\n", nmcr, nexnum);
			if (nmcr >= 4) {
				printf("5 mcr's");
				goto unsupp;
			}
			switch (cpu) {
			case VAX_780:
				mcrtype[nmcr] = M780C;
				break;
			case VAX_750:
				mcrtype[nmcr] = M750;
				break;
			case VAX_730:
				mcrtype[nmcr] = M730;
				break;
			}
			mcraddr[nmcr++] = (struct mcr *)nxv;
			break;

		case NEX_MEM64I:
		case NEX_MEM64L:
		case NEX_MEM64LI:
		case NEX_MEM256I:
		case NEX_MEM256L:
		case NEX_MEM256LI:
			printf("mcr%d (el) at tr%d\n", nmcr, nexnum);
			if (nmcr >= 4) {
				printf("5 mcr's");
				goto unsupp;
			}
			if (cpu == VAX_780)
				mcrtype[nmcr] = M780EL;
			mcraddr[nmcr++] = (struct mcr *)nxv;
			if (nexcsr.nex_type != NEX_MEM64I && 
			  nexcsr.nex_type != NEX_MEM256I)
				break;
			/* fall into ... */

		case NEX_MEM64U:
		case NEX_MEM64UI:
		case NEX_MEM256U:
		case NEX_MEM256UI:
			printf("mcr%d (eu) at tr%d\n", nmcr, nexnum);
			if (nmcr >= 4) {
				printf("5 mcr's");
				goto unsupp;
			}
			if (cpu == VAX_780)
				mcrtype[nmcr] = M780EU;
			mcraddr[nmcr++] = (struct mcr *)nxv;
			break;

		case NEX_MPM0:
		case NEX_MPM1:
		case NEX_MPM2:
		case NEX_MPM3:
			printf("mpm");
			goto unsupp;

		case NEX_CI:
			printf("ci");
			goto unsupp;

		default:
			printf("nexus type %x", nexcsr.nex_type);
unsupp:
			printf(" unsupported (at tr %d)\n", nexnum);
			continue;
unconfig:
			printf(" not configured\n");
			continue;
		}
	}
	if (nummba > NMBA)
		nummba = NMBA;
	if (numuba > NUBA)
		numuba = NUBA;
}

#if NMBA > 0
struct	mba_device *mbaconfig();
/*
 * Find devices attached to a particular mba
 * and look for each device found in the massbus
 * initialization tables.
 */
mbafind(nxv, nxp)
	struct nexus *nxv, *nxp;
{
	register struct mba_regs *mdp;
	register struct mba_drv *mbd;
	register struct mba_device *mi;
	register struct mba_slave *ms;
	int dn, dt, sn;
	struct mba_device fnd;

	mdp = (struct mba_regs *)nxv;
	mba_hd[nummba].mh_mba = mdp;
	mba_hd[nummba].mh_physmba = (struct mba_regs *)nxp;
	setscbnex(mbaintv[nummba]);
	fnd.mi_mba = mdp;
	fnd.mi_mbanum = nummba;
	for (mbd = mdp->mba_drv, dn = 0; mbd < &mdp->mba_drv[8]; mbd++, dn++) {
		if ((mbd->mbd_ds&MBDS_DPR) == 0)
			continue;
		mdp->mba_sr |= MBSR_NED;		/* si kludge */
		dt = mbd->mbd_dt & 0xffff;
		if (dt == 0)
			continue;
		if (mdp->mba_sr&MBSR_NED)
			continue;			/* si kludge */
		if (dt == MBDT_MOH)
			continue;
		fnd.mi_drive = dn;
#define	qeq(a, b)	( a == b || a == '?' )
		if ((mi = mbaconfig(&fnd, dt)) && (dt & MBDT_TAP))
		    for (sn = 0; sn < 8; sn++) {
			mbd->mbd_tc = sn;
		        for (ms = mbsinit; ms->ms_driver; ms++)
			    if (ms->ms_driver == mi->mi_driver &&
				ms->ms_alive == 0 && 
				qeq(ms->ms_ctlr, mi->mi_unit) &&
				qeq(ms->ms_slave, sn) &&
				(*ms->ms_driver->md_slave)(mi, ms, sn)) {
					printf("%s%d at %s%d slave %d\n"
					    , ms->ms_driver->md_sname
					    , ms->ms_unit
					    , mi->mi_driver->md_dname
					    , mi->mi_unit
					    , sn
					);
					ms->ms_alive = 1;
					ms->ms_ctlr = mi->mi_unit;
					ms->ms_slave = sn;
				}
		    }
	}
	mdp->mba_cr = MBCR_INIT;
	mdp->mba_cr = MBCR_IE;
}

/*
 * Have found a massbus device;
 * see if it is in the configuration table.
 * If so, fill in its data.
 */
struct mba_device *
mbaconfig(ni, type)
	register struct mba_device *ni;
	register int type;
{
	register struct mba_device *mi;
	register short *tp;
	register struct mba_hd *mh;

	for (mi = mbdinit; mi->mi_driver; mi++) {
		if (mi->mi_alive)
			continue;
		tp = mi->mi_driver->md_type;
		for (mi->mi_type = 0; *tp; tp++, mi->mi_type++)
			if (*tp == (type&MBDT_TYPE))
				goto found;
		continue;
found:
#define	match(fld)	(ni->fld == mi->fld || mi->fld == '?')
		if (!match(mi_drive) || !match(mi_mbanum))
			continue;
		printf("%s%d at mba%d drive %d\n",
		    mi->mi_driver->md_dname, mi->mi_unit,
		    ni->mi_mbanum, ni->mi_drive);
		mi->mi_alive = 1;
		mh = &mba_hd[ni->mi_mbanum];
		mi->mi_hd = mh;
		mh->mh_mbip[ni->mi_drive] = mi;
		mh->mh_ndrive++;
		mi->mi_mba = ni->mi_mba;
		mi->mi_drv = &mi->mi_mba->mba_drv[ni->mi_drive];
		mi->mi_mbanum = ni->mi_mbanum;
		mi->mi_drive = ni->mi_drive;
		/*
		 * If drive has never been seen before,
		 * give it a dkn for statistics.
		 */
		if (mi->mi_driver->md_info[mi->mi_unit] == 0) {
			mi->mi_driver->md_info[mi->mi_unit] = mi;
			if (mi->mi_dk && dkn < DK_NDRIVE)
				mi->mi_dk = dkn++;
			else
				mi->mi_dk = -1;
		}
		(*mi->mi_driver->md_attach)(mi);
		return (mi);
	}
	return (0);
}
#endif

/*
 * Fixctlrmask fixes the masks of the driver ctlr routines
 * which otherwise save r10 and r11 where the interrupt and br
 * level are passed through.
 */
fixctlrmask()
{
	register struct uba_ctlr *um;
	register struct uba_device *ui;
	register struct uba_driver *ud;
#define	phys(a,b) ((b)(((int)(a))&0x7fffffff))

	for (um = ubminit; ud = phys(um->um_driver, struct uba_driver *); um++)
		*phys(ud->ud_probe, short *) &= ~0xc00;
	for (ui = ubdinit; ud = phys(ui->ui_driver, struct uba_driver *); ui++)
		*phys(ud->ud_probe, short *) &= ~0xc00;
}

/*
 * Find devices on a UNIBUS.
 * Uses per-driver routine to set <br,cvec> into <r11,r10>,
 * and then fills in the tables, with help from a per-driver
 * slave initialization routine.
 */
unifind(vubp, pubp, vumem, pumem, memmap, haveubasr)
	struct uba_regs *vubp, *pubp;
	caddr_t vumem, pumem;
	struct pte *memmap;
	int haveubasr;
{
#ifndef lint
	register int br, cvec;			/* MUST BE r11, r10 */
#else
	/*
	 * Lint doesn't realize that these
	 * can be initialized asynchronously
	 * when devices interrupt.
	 */
	register int br = 0, cvec = 0;
#endif
	register struct uba_device *ui;
	register struct uba_ctlr *um;
	u_short *reg, *ap, addr;
	struct uba_hd *uhp;
	struct uba_driver *udp;
	int i, (**ivec)();
	caddr_t ualloc, zmemall();
	extern int catcher[256];

#if VAX630
	/*
	 * The map registers start right at 20088000 on the
	 * ka630, so we have to subtract out the 2k offset to make the
	 * pointers work..
	 */
	if (cpu == VAX_630) {
		vubp = (struct uba_regs *)(((u_long)vubp)-0x800);
		pubp = (struct uba_regs *)(((u_long)pubp)-0x800);
	}
#endif
	/*
	 * Initialize the UNIBUS, by freeing the map
	 * registers and the buffered data path registers
	 */
	uhp = &uba_hd[numuba];
	uhp->uh_map = (struct map *)calloc(UAMSIZ * sizeof (struct map));
	ubainitmaps(uhp);

	/*
	 * Save virtual and physical addresses
	 * of adaptor, and allocate and initialize
	 * the UNIBUS interrupt vector.
	 */
	uhp->uh_uba = vubp;
	uhp->uh_physuba = pubp;
	/*
	 * On the 8600, can't use UNIvec;
	 * the vectors for the second SBI overlap it.
	 */
	if (cpu == VAX_8600)
		uhp->uh_vec = (int(**)())calloc(512);
	else if (numuba == 0)
		uhp->uh_vec = UNIvec;
#if NUBA > 1
	else if (numuba == 1)
		uhp->uh_vec = UNI1vec;
	else
		uhp->uh_vec = (int(**)())calloc(512);
#endif
	for (i = 0; i < 128; i++)
		uhp->uh_vec[i] =
		    scbentry(&catcher[i*2], SCB_ISTACK);
	/*
	 * Set last free interrupt vector for devices with
	 * programmable interrupt vectors.  Use is to decrement
	 * this number and use result as interrupt vector.
	 */
	uhp->uh_lastiv = 0x200;

#if VAX630
	/*
	 * Kludge time again. The q22 memory and device reg. address spaces
	 * are not physically contiguous, so we need 2 loops to map them
	 * into contiguous virtual space.
	 */
	if (cpu == VAX_630) {
		ioaccess(pumem, memmap, (UBAPAGES-16)*NBPG);
		ioaccess(0x20000000, memmap+(UBAPAGES-16), 16*NBPG);
	} else
#endif
		ioaccess(pumem, memmap, UBAPAGES * NBPG);
#if defined(VAX780) || defined(VAX8600)
	if (haveubasr) {
		vubp->uba_sr = vubp->uba_sr;
		vubp->uba_cr = UBACR_IFS|UBACR_BRIE;
	}
#endif
	/*
	 * First configure devices that have unibus memory,
	 * allowing them to allocate the correct map registers.
	 */
	ubameminit(numuba);
	/*
	 * Grab some memory to record the umem address space we allocate,
	 * so we can be sure not to place two devices at the same address.
	 *
	 * We could use just 1/8 of this (we only want a 1 bit flag) but
	 * we are going to give it back anyway, and that would make the
	 * code here bigger (which we can't give back), so ...
	 *
	 * One day, someone will make a unibus with something other than
	 * an 8K i/o address space, & screw this totally.
	 */
	ualloc = zmemall(memall, 8*1024);
	if (ualloc == (caddr_t)0)
		panic("no mem for unifind");

	/*
	 * Map the first page of UNIBUS i/o
	 * space to the first page of memory
	 * for devices which will need to dma
	 * output to produce an interrupt.
	 */
	*(int *)(&vubp->uba_map[0]) = UBAMR_MRV;

#define	ubaoff(off)	((off)&0x1fff)
#define	ubaddr(off)	(u_short *)((int)vumem + (ubaoff(off)|0x3e000))
	/*
	 * Check each unibus mass storage controller.
	 * For each one which is potentially on this uba,
	 * see if it is really there, and if it is record it and
	 * then go looking for slaves.
	 */
	for (um = ubminit; udp = um->um_driver; um++) {
		if (um->um_ubanum != numuba && um->um_ubanum != '?')
			continue;
		addr = (u_short)um->um_addr;
		/*
		 * use the particular address specified first,
		 * or if it is given as "0", of there is no device
		 * at that address, try all the standard addresses
		 * in the driver til we find it
		 */
	    for (ap = udp->ud_addr; addr || (addr = *ap++); addr = 0) {

		if (ualloc[ubaoff(addr)])
			continue;
		reg = ubaddr(addr);
		if (badaddr((caddr_t)reg, 2))
			continue;
#if defined(VAX780) || defined(VAX8600)
		if (haveubasr && vubp->uba_sr) {
			vubp->uba_sr = vubp->uba_sr;
			continue;
		}
#endif
		cvec = 0x200;
		i = (*udp->ud_probe)(reg, um->um_ctlr, um);
#if defined(VAX780) || defined(VAX8600)
		if (haveubasr && vubp->uba_sr) {
			vubp->uba_sr = vubp->uba_sr;
			continue;
		}
#endif
		if (i == 0)
			continue;
		printf("%s%d at uba%d csr %o ",
		    udp->ud_mname, um->um_ctlr, numuba, addr);
		if (cvec == 0) {
			printf("zero vector\n");
			continue;
		}
		if (cvec == 0x200) {
			printf("didn't interrupt\n");
			continue;
		}
		printf("vec %o, ipl %x\n", cvec, br);
		um->um_alive = 1;
		um->um_ubanum = numuba;
		um->um_hd = &uba_hd[numuba];
		um->um_addr = (caddr_t)reg;
		udp->ud_minfo[um->um_ctlr] = um;
		for (ivec = um->um_intr; *ivec; ivec++) {
			um->um_hd->uh_vec[cvec/4] =
			    scbentry(*ivec, SCB_ISTACK);
			cvec += 4;
		}
		for (ui = ubdinit; ui->ui_driver; ui++) {
			if (ui->ui_driver != udp || ui->ui_alive ||
			    ui->ui_ctlr != um->um_ctlr && ui->ui_ctlr != '?' ||
			    ui->ui_ubanum != numuba && ui->ui_ubanum != '?')
				continue;
			if ((*udp->ud_slave)(ui, reg)) {
				ui->ui_alive = 1;
				ui->ui_ctlr = um->um_ctlr;
				ui->ui_ubanum = numuba;
				ui->ui_hd = &uba_hd[numuba];
				ui->ui_addr = (caddr_t)reg;
				ui->ui_physaddr = pumem + ubdevreg(addr);
				if (ui->ui_dk && dkn < DK_NDRIVE)
					ui->ui_dk = dkn++;
				else
					ui->ui_dk = -1;
				ui->ui_mi = um;
				/* ui_type comes from driver */
				udp->ud_dinfo[ui->ui_unit] = ui;
				printf("%s%d at %s%d slave %d\n",
				    udp->ud_dname, ui->ui_unit,
				    udp->ud_mname, um->um_ctlr, ui->ui_slave);
				(*udp->ud_attach)(ui);
			}
		}
		break;
	    }
	}
	/*
	 * Now look for non-mass storage peripherals.
	 */
	for (ui = ubdinit; udp = ui->ui_driver; ui++) {
		if (ui->ui_ubanum != numuba && ui->ui_ubanum != '?' ||
		    ui->ui_alive || ui->ui_slave != -1)
			continue;
		addr = (u_short)ui->ui_addr;

	    for (ap = udp->ud_addr; addr || (addr = *ap++); addr = 0) {
		
		if (ualloc[ubaoff(addr)])
			continue;
		reg = ubaddr(addr);
		if (badaddr((caddr_t)reg, 2))
			continue;
#if defined(VAX780) || defined(VAX8600)
		if (haveubasr && vubp->uba_sr) {
			vubp->uba_sr = vubp->uba_sr;
			continue;
		}
#endif
		cvec = 0x200;
		i = (*udp->ud_probe)(reg, ui);
#if defined(VAX780) || defined(VAX8600)
		if (haveubasr && vubp->uba_sr) {
			vubp->uba_sr = vubp->uba_sr;
			continue;
		}
#endif
		if (i == 0)
			continue;
		printf("%s%d at uba%d csr %o ",
		    ui->ui_driver->ud_dname, ui->ui_unit, numuba, addr);
		if (cvec == 0) {
			printf("zero vector\n");
			continue;
		}
		if (cvec == 0x200) {
			printf("didn't interrupt\n");
			continue;
		}
		printf("vec %o, ipl %x\n", cvec, br);
		while (--i >= 0)
			ualloc[ubaoff(addr+i)] = 1;
		ui->ui_hd = &uba_hd[numuba];
		for (ivec = ui->ui_intr; *ivec; ivec++) {
			ui->ui_hd->uh_vec[cvec/4] =
			    scbentry(*ivec, SCB_ISTACK);
			cvec += 4;
		}
		ui->ui_alive = 1;
		ui->ui_ubanum = numuba;
		ui->ui_addr = (caddr_t)reg;
		ui->ui_physaddr = pumem + ubdevreg(addr);
		ui->ui_dk = -1;
		/* ui_type comes from driver */
		udp->ud_dinfo[ui->ui_unit] = ui;
		(*udp->ud_attach)(ui);
		break;
	    }
	}

#ifdef	AUTO_DEBUG
	printf("Unibus allocation map");
	for (i = 0; i < 8*1024; ) {
		register n, m;

		if ((i % 128) == 0) {
			printf("\n%6o:", i);
			for (n = 0; n < 128; n++)
				if (ualloc[i+n])
					break;
			if (n == 128) {
				i += 128;
				continue;
			}
		}

		for (n = m = 0; n < 16; n++) {
			m <<= 1;
			m |= ualloc[i++];
		}

		printf(" %4x", m);
	}
	printf("\n");
#endif

	wmemfree(ualloc, 8*1024);
}

setscbnex(fn)
	int (*fn)();
{
	register struct scb *scbp = &scb;

	scbp = (struct scb *)((caddr_t)scbp + nsbi * 512);
	scbp->scb_ipl14[nexnum] = scbp->scb_ipl15[nexnum] =
	    scbp->scb_ipl16[nexnum] = scbp->scb_ipl17[nexnum] =
		scbentry(fn, SCB_ISTACK);
}

/*
 * Make an IO register area accessible at physical address physa
 * by mapping kernel ptes starting at pte.
 */
ioaccess(physa, pte, size)
	caddr_t physa;
	register struct pte *pte;
	int size;
{
	register int i = btop(size);
	register unsigned v = btop(physa);
	
	do
		*(int *)pte++ = PG_V|PG_KW|v++;
	while (--i > 0);
	mtpr(TBIA, 0);
}

/*
 * Configure swap space and related parameters.
 */
swapconf()
{
	register struct swdevt *swp;
	register int nblks;

	for (swp = swdevt; swp->sw_dev; swp++) {
		if (bdevsw[major(swp->sw_dev)].d_psize) {
			nblks =
			    (*bdevsw[major(swp->sw_dev)].d_psize)(swp->sw_dev);
			if (swp->sw_nblks == 0 || swp->sw_nblks > nblks)
				swp->sw_nblks = nblks;
		}
	}
	if (!cold)			/* in case called for mba device */
		return;
	if (dumplo == 0 && bdevsw[major(dumpdev)].d_psize)
		dumplo = (*bdevsw[major(dumpdev)].d_psize)(dumpdev) - physmem;
	if (dumplo < 0)
		dumplo = 0;
}

#define	DOSWAP			/* Change swdevt, argdev, and dumpdev too */
u_long	bootdev;		/* should be dev_t, but not until 32 bits */

static	char devname[][2] = {
	'h','p',	/* 0 = hp */
	0,0,		/* 1 = ht */
	'u','p',	/* 2 = up */
	'r','k',	/* 3 = hk */
	0,0,		/* 4 = sw */
	0,0,		/* 5 = tm */
	0,0,		/* 6 = ts */
	0,0,		/* 7 = mt */
	0,0,		/* 8 = tu */
	'r','a',	/* 9 = ra */
	0,0,		/* 10 = ut */
	'r','b',	/* 11 = rb */
	0,0,		/* 12 = uu */
	0,0,		/* 13 = rx */
	'r','l',	/* 14 = rl */
};

#define	PARTITIONMASK	0x7
#define	PARTITIONSHIFT	3

/*
 * Attempt to find the device from which we were booted.
 * If we can do so, and not instructed not to do so,
 * change rootdev to correspond to the load device.
 */
setroot()
{
	int  majdev, mindev, unit, part, adaptor;
	dev_t temp, orootdev;
	struct swdevt *swp;

	if (boothowto & RB_DFLTROOT ||
	    (bootdev & B_MAGICMASK) != (u_long)B_DEVMAGIC)
		return;
	majdev = (bootdev >> B_TYPESHIFT) & B_TYPEMASK;
	if (majdev > sizeof(devname) / sizeof(devname[0]))
		return;
	adaptor = (bootdev >> B_ADAPTORSHIFT) & B_ADAPTORMASK;
	part = (bootdev >> B_PARTITIONSHIFT) & B_PARTITIONMASK;
	unit = (bootdev >> B_UNITSHIFT) & B_UNITMASK;
	if (majdev == 0) {	/* MBA device */
#if NMBA > 0
		register struct mba_device *mbap;
		int mask;

/*
 * The MBA number used at boot time is not necessarily the same as the
 * MBA number used by the kernel.  In order to change the rootdev we need to
 * convert the boot MBA number to the kernel MBA number.  The address space
 * for an MBA used by the boot code is 0x20010000 + 0x2000 * MBA_number
 * on the 78? and 86?0, 0xf28000 + 0x2000 * MBA_number on the 750.
 * Therefore we can search the mba_hd table for the MBA that has the physical
 * address corresponding to the boot MBA number.
 */
#define	PHYSADRSHFT	13
#define	PHYSMBAMASK780	0x7
#define	PHYSMBAMASK750	0x3

		switch (cpu) {

		case VAX_780:
		case VAX_8600:
		default:
			mask = PHYSMBAMASK780;
			break;

		case VAX_750:
			mask = PHYSMBAMASK750;
			break;
		}
		for (mbap = mbdinit; mbap->mi_driver; mbap++)
			if (mbap->mi_alive && mbap->mi_drive == unit &&
			    (((long)mbap->mi_hd->mh_physmba >> PHYSADRSHFT)
			      & mask) == adaptor)
			    	break;
		if (mbap->mi_driver == 0)
			return;
		mindev = mbap->mi_unit;
#else
		return;
#endif
	} else {
		register struct uba_device *ubap;

		for (ubap = ubdinit; ubap->ui_driver; ubap++)
			if (ubap->ui_alive && ubap->ui_slave == unit &&
			   ubap->ui_ubanum == adaptor &&
			   ubap->ui_driver->ud_dname[0] == devname[majdev][0] &&
			   ubap->ui_driver->ud_dname[1] == devname[majdev][1])
			    	break;

		if (ubap->ui_driver == 0)
			return;
		mindev = ubap->ui_unit;
	}
	mindev = (mindev << PARTITIONSHIFT) + part;
	orootdev = rootdev;
	rootdev = makedev(majdev, mindev);
	/*
	 * If the original rootdev is the same as the one
	 * just calculated, don't need to adjust the swap configuration.
	 */
	if (rootdev == orootdev)
		return;

	printf("Changing root device to %c%c%d%c\n",
		devname[majdev][0], devname[majdev][1],
		mindev >> PARTITIONSHIFT, part + 'a');

#ifdef DOSWAP
	mindev &= ~PARTITIONMASK;
	for (swp = swdevt; swp->sw_dev; swp++) {
		if (majdev == major(swp->sw_dev) &&
		    mindev == (minor(swp->sw_dev) & ~PARTITIONMASK)) {
			temp = swdevt[0].sw_dev;
			swdevt[0].sw_dev = swp->sw_dev;
			swp->sw_dev = temp;
			break;
		}
	}
	if (swp->sw_dev == 0)
		return;

	/*
	 * If argdev and dumpdev were the same as the old primary swap
	 * device, move them to the new primary swap device.
	 */
	if (temp == dumpdev)
		dumpdev = swdevt[0].sw_dev;
	if (temp == argdev)
		argdev = swdevt[0].sw_dev;
#endif
}
