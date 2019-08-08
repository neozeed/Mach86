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
/* $Header: fpa.c,v 5.5 86/03/24 20:40:44 webb Exp $ */
/* $Source: /ibm/acis/usr/sys_ca/ca/RCS/fpa.c,v $ */


#include "../ca/debug.h"
#include "../ca/scr.h"
#include "../ca/mmu.h"
#include "../ca/reg.h"

#include "../h/param.h"
#include "../h/vmparam.h"
#include "../h/systm.h"
#include "../h/dir.h"
#include "../h/user.h"
#include "../h/proc.h"
#include "../h/seg.h"
#include "../h/acct.h"
#include "../h/kernel.h"
#include "../h/vmmeter.h"
#include "../ca/io.h"
#include "../ca/fpa.h"

#ifdef NFL
int _FPemulate();
#endif

#ifdef FPA

int (*_trap)();
int fpa_trap();
extern int fpa_valid;

/*
 * initialize the FPA, including determining if it is present.
 * we determine if it is present by trapping faults (via _trap)
 * and marking the fpa not present.
 * note that this routine MUST be invoked before any fork's have
 * been done in order to set p_fpareg appropriately.
 * WARNING:
 *	it is arranged that fpa_curreg = FPA_NONE, and
 *		proc[0].p_fpareg = FPA_NONE
 *	during startup. This prevents 'resume' from changing the fpa 
 *	register set or locking the fpa. Changing fpa_curreg or p_fpareg's 
 *	value is done here (in sync) to FPA_NONE if there is no fpa, or to
 *	FPA_UNUSED (fpa present, not being used by this process).
 */

int fpa_reset_done = 0;			/* 0 until fpa has been reset */
label_t fpa_label;
char fpa_unused = FPA_NONE;
int fpa_regs = (1<<FPA_RESERVED)-1;	/* register allocation bit map */
int fpa_nextgrab = FPA_RESERVED;	/* net register to grab */
int fpa_initial[MAX_FPA_REGS] = 
{	0,	0,	0,	0,	
	0,	0,	0,	0,	
	0,	0,	0,	0,	
	0,	0,	FP_S_unsgd,	0 };

/* fpa exceptions in string form */
char *fpa_tft[8] = {
"no exception", "underflow", "overflow", "zero devide",
"reserved", "operand error", "inexact result", "illegal command" };

/* see above comment */
fpa_init()
{
	register int (*oldtrap)() = _trap;
	register int i, x;

	fpa_curreg = proc[0].p_fpareg = fpa_unused = FPA_NONE;	 /* assume no FPA present */
	_trap = fpa_trap;
	if (setjmp(&fpa_label)) {
		fpa_curreg = proc[0].p_fpareg = fpa_unused = FPA_NONE;	 /* no FPA present */
		printf("FPA not %s\n",fpa_reset_done ? "operational" : "present");
		_trap = oldtrap;
		return;
	}
#ifdef FPA_RESET
	if (!fpa_reset_done) {
		DEBUGF(fpadebug, printf("resetting fpa\n"));
		fpa_reset();
		DELAY(10);		/* wait 10 usec */
		++fpa_reset_done;
	}
#endif FPA_RESET
	fpa_lockfp();
/* 
 * for each register set do a taskswitch and read status & clear exception 
 * and then write the status register.
 */
	for (i=0; i<MAX_FPA_SETS; ++i) {
		fpa_tskswu(i);
		x = fpa_rdscx();
		fpa_wtstr(0);
	}
	fpa_lockfp();

	fpa_curreg = proc[0].p_fpareg = fpa_unused = FPA_UNUSED;	/* FPA present */
	++fpa_present;
	_trap = oldtrap;
	if ((fpasave = (struct fpasave *) calloc(nproc * sizeof (struct fpasave))) == 0)
		panic("fpasave calloc");
	printf("FPA initialized %s\n",fpa_reset_done != 2 ? "normally" : " (after machine check)");
}


/*
 * trap intercept routine in effect during fpa initialization.
 * this is done so that we can test to see if the fpa is there, and
 * also to handle the machine check that the fpa produces if it is 
 * reset more than once (sigh).
 */
fpa_trap(mcs_pcs, info, ics_cs, regs)
	register mcs_pcs, info;
	int ics_cs;			  /* must not be a register variable */
	int regs;			  /* must not be a register variable */
{
	register int *locr0 = &regs;

	DEBUGF(fpadebug,
	    prstate("fpa_trap", mcs_pcs, info, locr0[IAR], ics_cs, (int *)0));
	if (mcs_pcs & MCS_CHECK && fpa_reset_done == 0) {
		fpa_reset_done = 1;
		printf("FPA caused machine check ignored\n");
		init_kbd();	/* re-init keyboard (sigh) */
		return;		/* try it again */
	}
	longjmp(&fpa_label);
}

/*
 * fork 3 cases
 * 1. old process has a register set (save it for newp)
 * 2. old process has a saved register set (copy it for newp)
 * 3. old process has not been allocated a set (newp has none too)
 */
fpa_fork(oldp,newp)
register struct proc *oldp, *newp;
{
	register struct fpasave *newsavep = &fpasave[newp-proc];

	if (fpa_hasreg(oldp->p_fpareg)) {	/* has register set */
		DEBUGF(fpadebug,
			printf("fpa_fork: save pid %d regs for pid %d\n",
				oldp->p_pid,newp->p_pid));
		fpa_save(newsavep->fpa_intreg);	/* save it in new area */
		newp->p_fpareg = FPA_SAVED;	/* mark as saved */
	}
	else if (oldp->p_fpareg == FPA_SAVED) {
		register struct fpasave *oldsavep = &fpasave[oldp-proc];

		DEBUGF(fpadebug,
			printf("fpa_fork: copy saved pid %d regs for pid %d\n",
				oldp->p_pid,newp->p_pid));
		*newsavep = *oldsavep;
		newp->p_fpareg = FPA_SAVED;	/* mark as saved */
	}
	else
		newp->p_fpareg = fpa_unused;	/* not used yet */

}

/*
 * invoked at exit or exec to release a register set
 */
fpa_exit(p)
register struct proc *p;
{
	register int reg = p->p_fpareg;

	if (fpa_hasreg(reg)) {
		DEBUGF(fpadebug,
			printf("fpa_exit: release pid %d regs %d\n",
				p->p_pid,reg));
		fpa_lockfp();
		fpa_regs &= ~(1<<reg);		/* release register set */
		fpa_proc[reg] = 0;		/* forget it */
	}
	fpa_curreg = p->p_fpareg = fpa_unused;		/* mark as unused */
}

/*
 * allocate a register set to the current process
 * 1. find a set
 * 2. remember that we found it
 */
fpa_alloc()
{
	register int reg = fpa_findreg();	/* get a set */
	register struct proc *p = u.u_procp;	/* my proc entry */
	
	fpa_regs |= 1 << reg;		/* allocate it */
	p->p_fpareg = reg;
	fpa_proc[reg] = p;		/* remember who has it */
	fpa_curreg = reg;		/* current register set */
}

/*
 * locate a free register if possible - otherwise steal one
 */
fpa_findreg()
{
	register int i;
	
	if (fpa_regs == 0xffffffff)
		return(fpa_grabreg());		/* steal one */
	for (i = FPA_RESERVED; i < MAX_FPA_SETS; ++i)
		if ((fpa_regs & (1 << i)) == 0) 
			break;
	DEBUGF(fpadebug, printf("fpa_findreg: pid %d reg set %d\n",
		u.u_procp->p_pid,i));
	fpa_tskswu(i);			/* switch to it */
	return(i);
}

/*
 * grab a register set from some other process.
 */
fpa_grabreg()
{
	register int reg = fpa_nextgrab;
	register struct proc *p;

	if (++reg > MAX_FPA_SETS)
		reg = FPA_RESERVED;
	DEBUGF(fpadebug, printf("fpa_grabreg: grabbing reg set %d\n",reg));
	fpa_nextgrab = reg;
	p = fpa_proc[reg];
	if (p->p_fpareg != reg)
		panic("fpa_grabreg");	/* oops */
	fpa_tskswu(reg);		/* make it current */
	fpa_save(fpasave[p-proc].fpa_intreg);	/* save it */
	p->p_fpareg = FPA_SAVED;	/* we saved it */
	return(reg);
}

/*
 * save the current register set into save area provided
 */
fpa_save(savefp)
register int *savefp;
{
	register int i;
	register int *FPA_ptr = (int *) (FPA_BASE + 0x02f000);

	i = fpa_rdstr();		/* store status */
	for (i=0; i<MAX_FPA_REGS; ++i)
		*savefp++ = * (i + FPA_ptr);
}


/*
 * restore the current register set into restore area provided
 */
fpa_restore(restorefp)
register int *restorefp;
{
	register int i;
	register int *FPA_ptr = (int *) (FPA_BASE + 0x025000);

	for (i=0; i<MAX_FPA_REGS; ++i)
		* (i + FPA_ptr) = *restorefp++ ;
	fpa_wtstr(restorefp[-2]);		/* store status */
}
#endif FPA

/*
 * system call to set return fpaemulator address
 */
getfpemulator()
{
#ifdef NFL
#ifdef FPA
	if (fpa_present) 
		u.u_r.r_val1 = (int) &fpa_valid;
 	else
#endif FPA
		u.u_r.r_val1 = (int) _FPemulate;
	u.u_r.r_val2 = (int) _FPemulate;
#endif NFL
}
/*
 * Kernel call to get fp
 */
kgetfpemulator()
{
#ifdef NFL
		return	( (int) _FPemulate);
#endif NFL
}

fp_mach_init()
{
#ifdef NFL
	*(unsigned long *) &(((FP_MACH *)USER_FPM)->status) = FP_S_unsgd;
#endif
}

/*
 * trap has detected a invalid data reference that might be 
 * fpa related.
 * we return 0 if it isn't ours (fpa reg set already allocated)
 *	     1 if we have now allocated register set
 * to be done:
 *	check to see if the reference is to '0xff......'
 */
fpa_pci(mcs_pcs,info,locr0)
	register int mcs_pcs;
	register int info;
	register int *locr0;
{
#ifdef FPA
	register struct proc *p = u.u_procp;
	register int fp_reg = p->p_fpareg;
	register int reg;
	register int status;
	extern short resume_rdstr;	/* read status instruction */

	DEBUGF(fpadebug&0x80,
		prstate("fpa_pci", mcs_pcs, info, locr0[IAR], locr0[ICSCS],locr0));
	if (fpa_present) {
		status = fpa_rdscx();
		fpa_status = status;		/* remember the status */
		DEBUGF(fpadebug&0x40,
			printf("fpa_status=0x%b tft=%s\n", status, "\20\4UNDER-ENABLE\5UNDERFLOW\6INEXACT-ENABLE\7INEXACT\10ROUND0\11ROUND1\12COND-Z\13COND-N\14PCK-DISABLE\15TASK-EXCEPTION\16BUS-EXCEPTION\17ABORTED\20PZERO",fpa_tft[status&0x07]));
		if ((status & FPA_TASK_EXCEPTION) == 0)
			return(0);		/* not fpa related */

		if (!fpa_hasreg(fp_reg)) {
			reg = fpa_alloc();	/* get a register set */
			fpa_restore((fp_reg == FPA_SAVED) ?
				fpasave[p-proc].fpa_intreg :
				fpa_initial);
			return(1);		/* exception handled */
		}
		fpa_status = status;		/* remember the status */

	/* Call IEEE fixup routine */
#ifdef NFL
		if (!_fpfpx(status)) {
			DEBUGF(fpadebug&0x80, printf("FPA/IEEE fixup done\n"));
			return(1); 
		}
		copyout((caddr_t)(&machine),(caddr_t)USER_FPM,sizeof(FP_MACH));
#else  NFL
		u.u_code = status;		/* pass to handler */
#endif NFL
		if ((short *) (locr0[IAR]) == &resume_rdstr) {
			register int mask = p->p_sigmask;
			p->p_sigmask |= 1<<(SIGFPE-1);
			DEBUGF(fpadebug&0x80, printf("masking SIGFPE during resume\n"));
			psignal(p, SIGFPE);		/* signal FPE */
			p->p_sigmask = mask;
		} else
			psignal(p, SIGFPE);		/* signal FPE */
		return(1);			/* now handled */
	}
#endif FPA
	return(0);		/* not FPA related */
}

/*
 * Alter floating point registers 
 * (should be available to signal handler in the sigcontext structure;
 * an "fpvmach" (defined in ieeetrap.h) which user can modify and
 * results get put back into emulator machine (if no FPA) or back
 * on card (if FPA)).
 */
fpa_sigcleanup(sf_scp)
struct sigcontext *sf_scp;
{
#ifdef NFL
	fptrap *fpt;
	FP_STATUS fpstat;
	FP_MACH usermach;
	int destreg;
	FLOAT floatres;
	DOUBLE doubleres;

	copyin((caddr_t)USER_FPM,(caddr_t)(&usermach),sizeof(FP_MACH));

	/* 
	 * NOTE:  I am not sure that we really want to update the status
	 * register with that returned by the signal handler.  User can
	 * easily screw it up, and if he/she uses the swapfptrap function
	 * from the signal handler to do what seems reasonable, this
	 * would tromp on that one...
	 */
	fpt = &(sf_scp->fpvmp->fptrap);
	fpstat = sf_scp->fpvmp->statusreg;

	destreg = fpt->fptrapinfo.dest;

	switch(fpt->fptrapinfo.operation) {
		case FP_abf:		/* There should be a better way... */
		case FP_adf:
		case FP_adfi:
		case FP_d2f:
		case FP_d2fi:
		case FP_dvf:
		case FP_dvfi:
		case FP_flf:
		case FP_flfi:
		case FP_i2f:
		case FP_mlf:
		case FP_mlfi:
		case FP_ngf:
		case FP_ntf:
		case FP_ntfi:
		case FP_rmf:
		case FP_rmfi:
		case FP_rnf:
		case FP_rnfi:
		case FP_sbf:
		case FP_sbfi:
		case FP_sqf:
		case FP_sqfi:
		case FP_trf:
		case FP_trfi:
				/* floatres should be pulled from fpvmach !! */
				floatres = sf_scp->fpvmp->fpreg[destreg/2].u.hp;
#ifdef FPA
				if (fpa_present)
				{
				    _FPAws(destreg, floatres);
				    _FPAputs( fpstat );
				}
				else
#endif FPA
				{
				    usermach.dreg[destreg/2].dfracth = floatres;
				    usermach.status = fpstat;
				    copyout((caddr_t)(&usermach),
					(caddr_t)USER_FPM,sizeof(FP_MACH));
				}
				break;

		case FP_abd:
		case FP_add:
		case FP_addi:
		case FP_dvd:
		case FP_dvdi:
		case FP_f2d:
		case FP_f2di:
		case FP_fld:
		case FP_fldi:
		case FP_i2d:
		case FP_mld:
		case FP_mldi:
		case FP_ngd:
		case FP_ntd:
		case FP_ntdi:
		case FP_rmd:
		case FP_rmdi:
		case FP_rnd:
		case FP_rndi:
		case FP_sbd:
		case FP_sbdi:
		case FP_sqd:
		case FP_sqdi:
		case FP_trd:
		case FP_trdi:
				/* doubleres should be pulled from fpvmach !! */
				doubleres.dfracth = 
					sf_scp->fpvmp->fpreg[destreg/2].u.hp;
				doubleres.dfractl = 
					sf_scp->fpvmp->fpreg[destreg/2].u.lp;

#ifdef FPA
				if (fpa_present)
				{
				    _FPAwl(destreg, doubleres);
				    _FPAputs( fpstat );
				}
				else
#endif FPA
				{
				    usermach.dreg[destreg/2].dfracth = 
						doubleres.dfracth;
				    usermach.dreg[destreg/2].dfractl =
						doubleres.dfractl;
				    usermach.status = fpstat;
				    copyout((caddr_t)(&usermach),
					(caddr_t)USER_FPM,sizeof(FP_MACH));
				}
				break;

		case FP_cmf:			/* no result from compares */
		case FP_cmd:
#ifdef FPA
				if (fpa_present)
				    _FPAputs( fpstat );
				else
#endif FPA
				{
				    usermach.status = fpstat;
				    copyout((caddr_t)(&usermach),
					(caddr_t)USER_FPM,sizeof(FP_MACH));
				}
				break;

		default:	/* unused or undefined operation */
				break;
	}
#endif NFL
}
