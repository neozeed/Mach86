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
/* $Header: autoconf.c,v 4.5 85/08/29 16:33:14 webb Exp $ */
/* $Source: /ibm/acis/usr/sys_ca/ca/RCS/autoconf.c,v $ */

#ifdef	CMU
/***********************************************************************
 * HISTORY
 * 24-Mar-86  Bill Bolosky (bolosky) at Carnegie-Mellon University
 *	Added code to turn of dds when SHOW_LOAD is off.
 *
 ***********************************************************************
 */
#include "show_load.h"
#include "mach_mp.h"
#endif	CMU

#include "../ca/reg.h"
#include "../ca/pte.h"
#include "../ca/rosetta.h"
#include "../ca/scr.h"
#include "../ca/debug.h"
#include "../h/param.h"
#include "../h/systm.h"
#include "../h/dir.h"
#include "../h/user.h"
#include "../h/kernel.h"
#include "../h/map.h"
#include "../h/vm.h"
#include "../h/proc.h"
#include "../h/buf.h"
#include "../h/reboot.h"
#include "../h/conf.h"
#include "../h/inode.h"
#include "../h/file.h"
#include "../h/text.h"
#include "../h/clist.h"
#include "../h/callout.h"
#include "../h/cmap.h"
#include "../h/mbuf.h"
#include "../h/msgbuf.h"
#include "../h/quota.h"
#include "../h/dk.h"
#include "../h/machine.h"
#include "../ca/rpb.h"
#include "../ca/io.h"
#include "../caio/ioccvar.h"
 /* #include "../h/dmap.h"		/* already included in user.h */

/*
 * Setup the system to run on the current machine.
 *
 * Configure() is called at boot time and initializes the
 * device tables and the memory controller monitoring.  Available
 * devices are determined (from possibilities mentioned in ioconf.c),
 * and the drivers are initialized.
 *
 */

int cold = 1;				  /* if 1, still working on cold-start */
int dkn;				  /* number of iostat dk numbers assigned so far 
					  */
int int_irq;				  /* the irq level of an interrupt (autoconf) */
int int_level;				  /* the CPU level of an interrupt (autoconf) */
int nulldev();				  /* a null routine */


configure()
{
	cold = 1;			  /* doing startup */
	DEBUGF(1, printf("rootdev=%x swapdev=%x argdev=%x dumpdev=%x\n", rootdev, swapdev, argdev, dumpdev));

	iow(ROSEBASE + ROSE_SER, 0);	  /* clear the SER! */
	DEBUGF(iodebug, printf("clear SER done\n"));
	autoconf();
#if GENERIC
	setconf();
#endif
#if	MACH_MP
	machine_conf();
#endif	MACH_MP
	swapconf();
	cold = 0;
	DEBUGF(1, printf("configure end\n"));
}

/*
 * configure the machine_slot table and machine_info record.
 */


machine_conf()

{
 /* XXX - just hacks up the table, doesn't really look at anything...*/
 machine_slot[0].is_cpu = TRUE;
 machine_slot[0].cpu_type = CPU_TYPE_ROMP;
 machine_slot[0].cpu_subtype = CPU_SUBTYPE_RT_PC;

 machine_info.max_cpus = NCPUS;
 machine_info.avail_cpus = 1;
 machine_info.memory_size = mem_size;
}

/*
 * print out warning for stray interrupts.
 */

devunk(info, icscs, iar)
	register int info, icscs, iar;
{
	register int s = spl7();	  /* inhibit interrupts */

	if (cold) {
		int_level = s & 0x07;
		DEBUGF(autodebug & 0x01, printf("autoconf interrupt: level %x irq %d\n", int_level, int_irq));
	} else {
		printf("Stray Interrupt! level=%d info=%x", s & 07, info);
		if (iodebug & 0x01)
			printf(" icscs=%x iar=%x", icscs, iar);
		printf("\n");
	}
	splx(s);			  /* return to previous interrupt level */
	return (0);			  /* last entry in list - must terminate search 
					  */
}


#ifdef IODEBUG

iotrap(info, icscs, iar, fn)
	register int info, icscs, iar;
	int (*fn)();
{
	register int n;

	DEBUGF(indebug, printf("call SLIH (prio=%x) at %x with %x %x %x\n",
	    mfsr(SCR_ICS) & 0x07, fn, info, icscs, iar));
	n = (*fn)(info, icscs, iar);
	DEBUGF(indebug, printf("SLIH returned %x\n", n));
	return (n);
}


#endif IODEBUG


#define MAX_8259_LEVELS 8		  /* max number of levels on a 8259 */
#define MAX_SLIH	24		  /* max number of SLIHs */
#define MAX_8259	2		  /* max number of 8259s */

int devunk();
int cnint();
int dmaint();

struct slihtab {
	int (*slih_rtn)();		  /* address of the actual slih */
	struct slihtab *slih_next;	  /* the address of the next slih entry */
	int slih_info;			  /* info for that SLIH */
	short slih_irq;			  /* irq level that caused it */
	short slih_flags;		  /* flag bits for autoconfig */
#	define FIXED	0x01		  /* if a fixed (predetermined) entry */

} slih_table[MAX_SLIH] =
{
/* rtn info irq flags */
	dmaint, 0, 0, 0, FIXED,		  /* 0 (Planar) 8237 terminal count */
	0, 0, 0, 10, 0,			  /* 1 IRQ 10 */
	0, 0, 0, 9, 0,			  /* 2 IRQ 9 */
	0, 0, 0, 3, 0,			  /* 3 IRQ 3 */
	0, 0, 0, 4, 0,			  /* 4 IRQ 4 */
	cnint, 0, 0, 1, FIXED,		  /* 5 (planar) keyboard */
	0, 0, 0, 2, 0,			  /* 6 8530 serial port */
	0, 0, 0, 7, 0,			  /* 7 IO BUS IRQ 7 */

	0, 0, 0, 8, 0,			  /* 8 (planar) reserved */
	0, 0, 0, 11, 0,			  /* 9 IRQ 11 */
	0, 0, 0, 14, 0,			  /* 10 IRQ 14 */
	0, 0, 0, 12, 0,			  /* 11 IRQ 12 */
	0, 0, 0, 6, 0,			  /* 12 IRQ 6 */
	0, 0, 0, 5, 0,			  /* 13 IRQ 5 */
	0, 0, 0, 15, 0,			  /* 14 IRQ 15 */
	0, 0, 0, 13, 0,			  /* 15 (planar) serial port */
};

/*
 * last_slih indicates where there is spare space for duplicate
 * (chain) entries.
 */
struct slihtab *last_slih = slih_table + (MAX_8259_LEVELS * MAX_8259);

/*
 * in "int_table" is passed to the 8259 interrupt controller service
 * routine for level 3 and level 4 interrupts.
 * it specifies the address of the 8259 and the SLIH table for the
 * 8 8259 levels. Each level is a linked list of driver interrupt service
 * routines that are called in turn. when one claims an interrupt the
 * list is terminated.
 * if none claim the interrupt then 'devunk' will be called.
 */


#define MAX_IRQ 16
char irq_map[MAX_IRQ];

struct int_table {
	char *addr_8259;		  /* address of appropriate 8259 */
	struct slihtab *slihtab;	  /* address of the SLIH table */
} int3table =
{
	(char *)Adr_8259A,		  /* the first 8259A */
	slih_table
};

struct int_table int4table = {
	(char *)Adr_8259B,		  /* the second 8259A */
	slih_table + MAX_8259_LEVELS
};

int ign_devunk = 0;			  /* can be patched to ignore devunks */

/*
 * int_8259 services an interrupt generated by an 8259A.
 * it determines which 8259 line caused the interrupt and then
 * calls the appropriate SLIH to service it.
 * if that SLIH does not claim the interrupt then the next SLIH
 * in the chain is called until either there are no more left or
 * one claims it.
 */

int_8259(table, iar, icscs)
	register struct int_table *table;
	register int iar, icscs;
{
	register char *adr_8259 = table->addr_8259;
	register int isr;
	register struct slihtab *s;
	register int i;
	register int irq;

	cnt.v_intr++;		/* count interrupts */
	DEBUGF(iodebug & 0x80, {
		*adr_8259 = GET_ISR;
		isr = *adr_8259;	  /* read the ISR */
		*adr_8259 = GET_IRR;
		i = *adr_8259;		  /* read the IRR */
		printf("8259 @ %x ISR=%x IRR=%x IMR=%x\n",
		    adr_8259, isr, i, *(adr_8259 + 1));
	}
	);

	DELAY(1);
	*adr_8259 = POLL_CMD;
	DELAY(2);			  /* just in case */
	i = *adr_8259;			  /* pick up the active device */
	DEBUGF(iodebug & 0x20, printf("8259 @ %x POLL=%x\n", adr_8259, i));
	if ((i & 0x80) == 0) {
		if (ign_devunk)
			return (0);
		printf("8259 @ %x (%x): ", adr_8259, i);
		return (devunk(table, iar, icscs)); /* complain */
	}
	i &= 0x07;			  /* remove any excess stuff */

	*adr_8259 = SEOI_CMD + i;	  /* specific EOI */
	s = table->slihtab + i;		  /* get the appropriate slih table */
	irq = s->slih_irq;		  /* the IRQ number that caused it */
	for (; s && s->slih_rtn; s = s->slih_next) {
		register int rc;

		DEBUGF(iodebug & 0x10, printf("8259 @ %x level %x slih=%x info=%x\n",
		    adr_8259, i, s->slih_rtn, s->slih_info));
		rc = (*s->slih_rtn)(s->slih_info, iar, icscs);
		if (rc == 0)
			return (rc);	  /* return SLIH results */
	}
	if (cold) {
		if (int_irq != -1 && int_irq != irq)
			printf("autoconf: help - interrupt at irq %d and irq %d!\n",
			    int_irq, irq);
		int_irq = irq;
	} else
		printf("8259 (%x) IRQ %d: %s ", i, irq, s ? "no SLIH" : "unclaimed");
	return (devunk(table, iar, icscs)); /* oops - no-one serviced it */
}


/* following table is MAX_SLIH long on the assumption that if that limit
 * is reasonable for slih_table it is also reasonable for the number
 * of device addresses.
 */
caddr_t probe_addrs[MAX_SLIH];		  /* table to remember used device addresses */
int trap();
int (*_trap)();
int autotrap();

autoconf()
{
/*
 * go thru the iocc_device structure and configure each existing device
 * into the system.
 */
	register struct iocc_ctlr *ic;
	register struct iocc_driver *id, *idr;
	register struct iocc_device *iod;
	register caddr_t * addrp;
	register caddr_t addr;		  /* address of adapter register */
	extern struct iocc_ctlr iocccinit[];
	extern struct iocc_device ioccdinit[];
	int s;

	_trap = autotrap;		  /* intercept traps due to autoconfig */
	printf("autoconf");
	s = spl1();			  /* allow interrupts (but keep as system ) */
	printf("\n");
	dkn = 0;
	init_slih();			  /* initialize slih_table */
/*BJB*/	printf("initialized slih.\n");
/*
 * scann thru the data structures resetting values
 * in case this is a restart of the kernel while in
 * memory.
 */
	for (ic = iocccinit; id = ic->ic_driver; ++ic) {
		ic->ic_alive = 0;
	}
	for (iod = ioccdinit; id = iod->iod_driver; ++iod) {
		iod->iod_alive = 0;
		iod->iod_forw = 0;
		iod->iod_mi = 0;
	}
/*
 * loop thru the controller structures
 * for each on which is potentially present check to see if it is
 * realy there.
 * if no address is given then check the standard list given in
 * the driver.
 */
	for (ic = iocccinit; id = ic->ic_driver; ++ic) {
		addr = ic->ic_addr;	  /* address from ctlr structure */
		for (addrp = id->idr_addr; addr || (addrp && (addr = *addrp++)); addr = 0)
			if (ctlr_probe(ic, addr, id))
				break;
		if (addr == 0)
			continue;	  /* this one is not here */
		id->idr_minfo[ic->ic_ctlr] = ic; /* remember controler */
		ic->ic_alive++;			 /* remember it's alive */
/*
 * loop thru the device structures looking for mass storage
 * peripherals attached to this adapter.
 */
		for (iod = ioccdinit; idr = iod->iod_driver; ++iod) {
			if (id != idr || iod->iod_alive ||
			    (ic->ic_ctlr != iod->iod_ctlr && iod->iod_ctlr != '?'))
				continue; /* not us */
			if (slave(id, iod, addr)) {
				iod->iod_alive = 1;
				iod->iod_ctlr = ic->ic_ctlr;
				iod->iod_addr = addr;
				if (iod->iod_dk && dkn < DK_NDRIVE)
					iod->iod_dk = dkn++;
				else
					iod->iod_dk = -1;
				iod->iod_mi = ic;
				printf("%s%d at %s%d slave %d\n",
				    id->idr_dname, iod->iod_unit,
				    id->idr_mname, ic->ic_ctlr, iod->iod_slave);
				attach(iod, id);
			}
		}
	}
/*
 * now look for non-mass storage peripherals.
 */
	for (iod = ioccdinit; id = iod->iod_driver; ++iod) {
		if (iod->iod_alive || iod->iod_slave != -1)
			continue;	  /* already done, or not present */
		addr = iod->iod_addr;	  /* address from device structure */
		for (addrp = id->idr_addr; addr || (addrp && (addr = *addrp++)); addr = 0) {
			if (!device_probe(iod, addr, id))
				continue;
			iod->iod_alive = 1;
			iod->iod_addr = addr;
			attach(iod, id);
			break;
		}
	}
	splx(s);			  /* restore interrupt state */
	_trap = trap;
	DEBUGF(autodebug, printf("autoconf end\n"));
}


ctlr_probe(ic, addr, id)
	register struct iocc_ctlr *ic;
	register caddr_t addr;
	register struct iocc_driver *id;
{
	register int result;
	DEBUGF(autodebug, printf("testing controller %s at %x\n", id->idr_dname, addr));
	if (present(addr + id->idr_csr) && (result = probe(addr, id)) != PROBE_BAD) {
		printf("%s%d adapter %x ", id->idr_mname, ic->ic_ctlr, addr);
		if (result == PROBE_BAD_INT) {
			printf("didn't interrupt\n");
			return (PROBE_BAD);
		}
		ic->ic_addr = addr;
		if (result == PROBE_OK)
			ic->ic_irq = int_irq; /* use actual */
		set_vector(ic->ic_irq, id->idr_intr, ic->ic_ctlr); /* set up IRQ */
		printf("\n");
		return (PROBE_OK);
	}
	return (PROBE_BAD);
}


device_probe(iod, addr, id)
	register struct iocc_device *iod;
	register caddr_t addr;
	register struct iocc_driver *id;
{
	register int result;

	DEBUGF(autodebug, printf("testing device %s at %x\n", id->idr_dname, addr));
	if (present(addr + id->idr_csr) && (result = probe(addr, id)) != PROBE_BAD) {
		DEBUGF(autodebug, printf("device %s found at %x\n", id->idr_dname, addr));
		printf("%s%d adapter %x ", id->idr_dname, iod->iod_unit, addr);
		if (result == PROBE_BAD_INT) {
			printf("didn't interrupt\n");
			return (PROBE_BAD);
		}
		iod->iod_alive = 1;
		if (result == PROBE_OK)
			iod->iod_irq = int_irq;	/* use actual interrupt level */
		set_vector(iod->iod_irq, id->idr_intr, iod->iod_unit); /* set up IRQ */
		printf("\n");
		return (1);
	} else {
		DEBUGF(autodebug, printf("device %s not found at %x\n", id->idr_dname, addr));
		return (0);
	}
}


/*
 * first check to see if the address is already used - if so then don't
 * regard it as present.
 * if probe succeedes then enter it in used address table.
 */
probe(addr, id)
	register struct iocc_driver *id;
	register caddr_t addr;
{
	register int result;
	register caddr_t * t;

	int_irq = -1;			  /* set up for interrupt determination */
	int_level = -1;
	for (t = probe_addrs; *t; ++t)
		if (*t == addr)
			return (PROBE_BAD); /* oops, already in use */
	*t = addr;			  /* device found - remember it's address */
	if (id->idr_probe == 0) {
		DEBUGF(autodebug, printf("no probe routine - ignored\n"));
		return (PROBE_BAD);	  /* assume that it exist if no probe rtn */
	}
	DEBUGF(autodebug, printf("calling probe routine at %x(%x)\n", id->idr_probe, addr));
	result = (*id->idr_probe)(addr);
	switch (result) {
	case PROBE_OK:			  /* device thinks it caused an interrupt */
		if (int_irq == -1 || int_level == -1) {
			result = PROBE_BAD_INT;	/* too bad */
			break;
		}
/* fall thru */
	case PROBE_NOINT:
		break;
	default:
		result = PROBE_BAD;
	case PROBE_BAD:
		break;
	}
	return (result);
}


slave(id, iod, addr)
	register struct iocc_driver *id;
	register struct iocc_device *iod;
	register int addr;
{
	register int result;
	if (id->idr_slave == 0) {
		DEBUGF(autodebug, printf("%s%d: no slave routine present - ignored\n",
		    id->idr_dname, iod->iod_unit));
		return (0);		  /* not there if no slave routine */
	}
	DEBUGF(autodebug, printf("calling slave at %x (%x,%x)\n", id->idr_slave, iod, addr));
	result = (*id->idr_slave)(iod, addr);
	return (result);
}


attach(iod, id)
	register struct iocc_device *iod;
	register struct iocc_driver *id;
{
	register int result;

	if (id->idr_dinfo)
		id->idr_dinfo[iod->iod_unit] = iod; /* link together */
	else
		printf("%s: dinfo pointer is null\n", id->idr_dname);
	if (id->idr_attach == 0) {
		printf("%s: no attach routine - ignored\n", id->idr_dname);
		return (0);		  /* assume that it doesn't exist if no attach
					     rtn */
	}
	DEBUGF(autodebug, printf("calling attach at %x(%x)\n", id->idr_attach, iod));
	result = (*id->idr_attach)(iod);
	return (result);
}


/*
 * store new interrupt service routine into slih_table.
 * scan thru slih_table until we find an entry with no slih_rtn
 * specified (better be the first one!)
 * or one with slih_next == NULL.
 */
set_vector(irq, rtn, info)
	int (*rtn)();
{
	register struct slihtab *s = &slih_table[irq_map[irq]], *p = 0;

	DEBUGF(autodebug, printf("set interrupt (IRQ %d) to %x info %x\n", irq, rtn, info));
	printf("IRQ %d ", irq);
	if (rtn == 0) {
		printf("null interrupt service routine for irq %d\n", irq);
		rtn = nulldev;
	}
	if (int_level >= 0)
		printf("CPU level %d ", int_level);
	for (; s && s->slih_rtn; s = s->slih_next)
		p = s;
	if (s == 0) {
		s = last_slih++;	  /* allocate new slih entry */
		s->slih_irq = irq;	  /* remember the irq number */
		p->slih_next = s;	  /* link it into the chain */
	}
	s->slih_rtn = rtn;
	s->slih_info = info;
	int_irq = -1;			  /* reset the interrupt info */
	int_level = -1;
}


#define UNDEFINED_ADDR	0xf0000000	  /* assume nothing at this address */
/*
 * we assume that a non-existant location will always return the same
 * value. We also assume that UNDEFINED_ADDR is just such an undefined
 * location. It appears that usually 0xff is returned but we have seen
 * cases where 0x1f came back.
 * the following should even work if the value returned is zero but in
 * that case we will write a non-zero (0xff) value into the register
 * to see if we get a zero back out again. If all this fails it is up
 * to the driver probe routine to figure out if the device is there.
 */
present(addr)
	register char *addr;
{
	register char *p = (char *)UNDEFINED_ADDR;

	addr[0] = 0;
	if (addr[0] == *p && addr[1] == *p) {
		addr[0] = ~*p;		  /* invert it just in case */
		return (addr[0] != *p);
	}
	return (1);
}


init_slih()
{
/*
 * scan thru slih_table and build the reverse mapping from IRQ number
 * to index into slih_tab.
 */
	register struct slihtab *s;
	register int i;

	for (i = 0, s = slih_table; i < MAX_8259_LEVELS * MAX_8259; ++i, ++s) {
		if ((s->slih_flags & FIXED) == 0) {
			s->slih_rtn = 0;  /* reset the pointer */
			s->slih_info = 0;
		}
		irq_map[s->slih_irq] = i; /* remember the mapping */
	}
	last_slih = s;			  /* first available spot */
	for (; i < MAX_SLIH; ++i, ++s) {
		s->slih_rtn = 0;
		s->slih_next = 0;
	}
}


autotrap(mcs_pcs, info, ics_cs, regs)
	register mcs_pcs, info;
	int ics_cs;			  /* must not be a register variable */
	int regs;			  /* must not be a register variable */
{

	prstate("trap", mcs_pcs, info, ics_cs, &regs);
	panic("trap during autoconfig");
}


/*
 * The following several variables are related to
 * the configuration process, and are used in initializing
 * the machine.
 */
#ifndef DMMIN
#define	DMMIN   32	/* units are atoms of swap device space (512) */
#endif DMMin

#ifndef DMMAX
#define	DMMAX   1024
#endif DMMAX
#ifndef DMTEXT
#define	DMTEXT  1024
#endif DMTEXT

#define	MAXDUMP 8194	/* room for a 4MB dump (and multiple of 17) */

int dumplo = 0;
int dmmin = DMMIN;
int dmmax = DMMAX;
int dmtext = DMTEXT;

/*
 * Configure swap space and related parameters.
 */

swapconf()
{
	register struct swdevt *swp;

	for (swp = swdevt; swp->sw_dev; swp++) {
		if (bdevsw[major(swp->sw_dev)].d_psize)
			swp->sw_nblks =
			    (*bdevsw[major(swp->sw_dev)].d_psize)(swp->sw_dev);
		if (swp->sw_nblks == 0)
			panic("swap blocks = 0");
	}
	if (!cold)			  /* in case called for mba device */
		return;
	if (dumplo == 0)
		dumplo = swdevt[0].sw_nblks - MAXDUMP;
	if (dumplo < 0)
		dumplo = 0;
	if (dmmin == 0)
		dmmin = DMMIN;
	if (dmmax == 0)
		dmmax = DMMAX;
	if (dmtext == 0)
		dmtext = DMTEXT;
	if (dmtext > dmmax)
		dmtext = dmmax;
}


slave_config()

{}
