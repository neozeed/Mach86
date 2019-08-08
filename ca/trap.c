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
/* $Header: trap.c,v 4.4 85/08/30 12:02:30 webb Exp $ */
/* $Source: /ibm/acis/usr/sys_ca/ca/RCS/trap.c,v $ */

#if CMU
/***********************************************************************
 * HISTORY
 * 27-Mar-86  Bill Bolosky (bolosky) at Carnegie-Mellon University
 *	MACH_VM: Merged in VM changes.
 *
 * 18-Mar-86  Bill Bolosky (bolosky) at Carnegie-Mellon University
 *	MACH_MP: Merged in changes related to swtch and unix_swtch
 *
 * 14-Mar-86  Bill Bolosky (bolosky) at Carnegie-Mellon University
 *	CS_RFS, CS_RPAUSE, CS_SYSCALL: Merged in CMU changes.
 *
 * 25-Feb-86  Bill Bolosky (bolosky) at Carnegie-Mellon University
 *	Removed line wherein trap set u.u_dirp, since it no longer
 *	exists.
 ***********************************************************************
 */

#include "cs_rfs.h"
#include "cs_syscall.h"
#include "cs_rpause.h"
#include "mach_mp.h"
#include "mach_vm.h"
#endif CMU

#define SYSCALLTRACE
 /*     trap.c  6.1     83/08/18        */

#include "../ca/debug.h"
#include "../ca/scr.h"
#include "../ca/rosetta.h"
#include "../ca/reg.h"

#if	MACH_MP
#include "../ca/cpu.h"
#endif	MACH_MP
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
#ifdef SYSCALLTRACE
#include "../sys/syscalls.c"
#endif SYSCALLTRACE
#include "../ca/io.h"

#if	MACH_VM
#include "../h/kern_return.h"
#include "../h/task.h"
#include "../h/thread.h"
#include "../vm/vm_map.h"
#include "../vm/vm_kern.h"
#include "../vm/pmap.h"
#include "../vm/vm_param.h"
#endif	MACH_VM

#if	MACH_MP
#include "../mp/syscall_sw.h"
#endif	MACH_MP


#define IPAGE_FAULT (PCS_KNOWN+PCS_IADDR)
#define DPAGE_FAULT (PCS_KNOWN+PCS_DADDR)
#define ILL_OP	(PCS_KNOWN+PCS_BAD_I)
#define PRIV_OP	(PCS_KNOWN+PCS_PRIV_I)

#if	CS_SYSCALL
extern struct	sysent	cmusysent[];
extern int ncmusysent;
extern int nallsysent;
#endif	CS_SYSCALL
#if	MACH_MP
extern long master_idle;
#endif	MACH_MP
struct sysent sysent[];
int nsysent;
char *mcpcfmt = MCPCFMT;

char *icsfmt = ICSFMT;
int _csr;				  /* the csr contents after trap */
					  /* just ICS without CS */

/*
 * Called from the trap handler when a processor trap occurs.
 *   mcs_pcs   = machine check status and program check status
 *   info = specific to type of check (eg rosetta exception register)
/* Called from the trap handler when a system call occurs with
 *   mcs_pcs   = 0
 *   info = svc number
 * Note that some high order bits of the mcs_pcs are used to simulate the
 *   breakpoint and AST conditions of the VAX.
 */
/*ARGSUSED*/
/*BJB*/	b_mcs_pcs, b_info, b_ics_cs, b_error, b_iar, b_regaddr, b_ft;
trap(mcs_pcs, info, ics_cs, regs)
	register mcs_pcs, info;
	int ics_cs;			  /* must not be a register variable */
	int regs;			  /* must not be a register variable */
{
	register int *locr0 = &regs;
	register int i;
	register struct proc *p;
	struct timeval syst;
	char *epitaph;
#if	MACH_VM
	vm_map_t	map;
	label_t 	*recover;
	vm_prot_t	fault_type;
	extern	int	iosavep;
#endif	MACH_VM


	DEBUGF(trdebug,
	    prstate("trap", mcs_pcs, info, ics_cs, (int *)0));

/*     syst = u.u_ru.ru_stime;  This gets compile errors JEC */
	cnt.v_trap++;		/* count traps */


	if (ICSCS_PROBSTATE & ics_cs) {
		mcs_pcs |= USER;
		u.u_ar0 = locr0;
	}
	switch (mcs_pcs) {
	default:
		if (mcs_pcs & USER) {
			printf("trap: unknown trap (0x%b) in user mode, info = 0x%x\n", mcs_pcs, mcpcfmt, info);
			i = SIGSEGV;
			break;
		}
		printf("unknown trap type: mcs_pcs = 0x%x.\n",mcs_pcs);
		epitaph = "trap";
		goto grave;

	case MCS_CHECK + MCS_DATA_TO + USER:	/* machine check in user mode */
	case MCS_CHECK + MCS_INS_TO + USER:	/* machine check in user mode */
	case MCS_CHECK + USER:		/* machine check in user mode */
		printf("machine check in user mode ... ");
		init_kbd();		  /* reinitialize keyboard */
		printf("keyboard init done\n");
		i = SIGBUS;
		break;

	case MCS_CHECK:		/* machine check in kernel mode */
		epitaph = "machine check";
		goto grave;

	case DPAGE_FAULT + USER:	  /* data page fault */
	case IPAGE_FAULT + USER:	  /* inst page fault */
#if	MACH_VM
	case DPAGE_FAULT:
		if ((current_thread() != THREAD_NULL)
		   && (ICSCS_PROBSTATE & ics_cs)) {
			i = u.u_error;
			u.u_error = 0;
		}
		/*
		 *	Determine which map to "fault" on.
		 *	If the faulting address is a kernel
		 *	address, make sure we were executing
		 *	in kernel mode.
		 *
		 *	XXX - The following check for system
		 *	segment should be in a macro somewhere
		 *	just in case we change it someday - Avie
		 */
		if ((((((vm_offset_t)info) & 0xf0000000) >> 28) == SYS_SEG) &&
			!(ICSCS_PROBSTATE & ics_cs))
			map = kernel_map;
		else
			map = current_task()->map;
		if (info & RTA_EX_HACK)
			fault_type =  VM_PROT_READ;
		else
			fault_type = VM_PROT_READ|VM_PROT_WRITE;


/*BJB*/		b_info = info; b_mcs_pcs = mcs_pcs; b_ics_cs = ics_cs;
/*BJB*/		b_iar = locr0[IAR]; b_regaddr = (int)locr0; 
/*BJB*/		b_ft = fault_type;
		if (/*BJB*/b_error = (vm_fault(map,
			trunc_page((vm_offset_t) info), fault_type,FALSE)
					!= KERN_SUCCESS)) {
			/*
			 *	If we're in the copyin/copyout routines,
			 *	we want to return control to the failure
			 *	return address that has been saved in
			 *	the thread table.
			 */
		if ((current_thread() != THREAD_NULL)
		   && (ICSCS_PROBSTATE & ics_cs)) 
				u.u_error = i;
			recover = (label_t *)current_thread()->recover;
			if (recover != NULL) {
				iosavep -= IOSAVEL;
				longjmp(recover);
			}
			i = SIGBUS;
			break;
		}
		if ((current_thread() != THREAD_NULL)
		   && (ICSCS_PROBSTATE & ics_cs)) 
			u.u_error = i;
		return;
#else	MACH_VM
		if (info & RTA_EX_FAULT) {
			i = u.u_error;
			if (pagein(info & (-NBPG), 0)) { /* VAX segment fault */
				if (grow((unsigned)locr0[SP]))
					goto out;
				i = SIGSEGV;
				break;
			}
			u.u_error = i;
			goto out;
		}
		if (info & RTA_EX_KEYS) {
			i = SIGSEGV;
			break;
		}
		/* assume reference to bad address (segment F?) and give SIGBUS */
		i = SIGBUS;		  /* not rosetta related */
		break;

	case DPAGE_FAULT:		  /* allow data page faults in kernel mode */
		if ((info & RTA_EX_FAULT) == 0) {
			epitaph = "kernel trap not tlb";
			goto grave;
		}
		i = u.u_error;
#ifdef	notdef
		if (pagein(info & (-NBPG), 0)) { /* VAX segment fault */
			epitaph = "kernel trap";
			goto grave;
		}
#else	notdef
		pagein(info & (-NBPG), 0);
#endif	notdef
		u.u_error = i;
		return;
#endif	MACH_VM
	case USER:{			  /* allow svc only in user mode */
		register caddr_t params;
		register struct sysent *callp;
		int opc, pno;

		cnt.v_trap--;			/* fix trap count */
		cnt.v_syscall++;		/* count sys calls */
		if (info == 139) { /* This undefined syscall is used by */
			sigcleanup(); /* the signal trampoline code, to do */
			goto out; /* the return from the user signal   */
		}
		/* catcher proc to the point of int. */
#ifdef KPROF
		if (info == 199) { /* kernel profiling added by JEC */
			kernprof(locr0[R2], locr0[R3]);
			goto out;
		}
#endif KPROF
			if (locr0[SP] < USRSTACK - ctob(u.u_ssize))
				grow(locr0[SP]); /* added 7/84 for ROMP */

			opc = locr0[IAR] - 4; /* address of svc inst */
			u.u_error = 0;	  /* optimistic */
/* This strange left-shift/right shift sign extends i(nfo) into an int */
			if ((i = ((info<<16)>>16))) {
				u.u_arg[0] = locr0[R2];
				u.u_arg[1] = locr0[R3];
				u.u_arg[2] = locr0[R4];
				pno = 3;
			} else {	  /* indirection */
				i = locr0[R2];
				u.u_arg[0] = locr0[R3];
				u.u_arg[1] = locr0[R4];
				pno = 2;
			}
			if ((i < -ncmusysent) && 
				!(i < (-ncmusysent - nmp_sysent))) {
					locr0[R0] = do_mp_syscall(info);
					break;
			}
			callp = ((u_short)(i + ncmusysent) >= nallsysent) 
				? &sysent[63] : &sysent[i];
			params = (caddr_t)locr0[SP] + 3 * sizeof(int);
			for (; pno < callp->sy_narg; ++pno) {
				u.u_arg[pno] = fuword(params);
				params += sizeof(int);
			}
			u.u_ap = u.u_arg;
			u.u_r.r_val1 = 0;
			u.u_r.r_val2 = locr0[R2];
			if (setjmp(&u.u_qsave)) {
				if (u.u_error == 0 && u.u_eosys == JUSTRETURN)
					u.u_error = EINTR;
			} else {
				u.u_eosys = JUSTRETURN;
#ifdef SYSCALLTRACE
				if (svdebug) {
					register int i;
					char *cp;

					if (info >= nsysent)
						printf("0x%x", info);
					else
						printf("%s", syscallnames[info]);
					cp = "(";
					for (i = 0; i < callp->sy_narg; i++) {
						printf("%s%x", cp, u.u_arg[i]);
						cp = ", ";
					}
					if (i)
						putchar(')', 0);
					printf(" pid=%d\n", u.u_procp->p_pid);
				}
#else
				if (svdebug)
					printf("Syscall %d routed for pid %d\n", info, u.u_procp->p_pid);
#endif
#if	CS_RFS
		/*
		 *  Remember the system call we are executing so that it
		 *  can be handled remotely if need be.
		 */
		u.u_rfsncnt = 0;
		u.u_rfscode = info;
#endif CS_RFS
#if	CS_RPAUSE
		/*
		 *  Show no resource pause conditions.
		 */
		u.u_rpswhich = 0;
		u.u_rpsfs = 0;
#endif	CS_RPAUSE
				(*(callp->sy_call))();
				if (svdebug && (svdebug > 1 || u.u_error))
					printf("%s completed for pid %d error %d returns 0x%x 0x%x\n",
					    syscallnames[info], u.u_procp->p_pid, u.u_error, u.u_r.r_val1, u.u_r.r_val2);
			}
#if	CS_RFS
	/*
	 *  The special error number EREMOTE is used by the remote system call
	 *  facility to short-circuit standard system call processing when the
	 *  equivalent has already been done remotely.  It serves simply to
	 *  unwind the call stack back to this point when the call has actually
	 *  been completed successfully.  It is not an error and should not be
	 *  relected back to the user process.
	 *
	 *  Also, clear the system call code value to indicate that we are no
	 *  longer in a system call.  I don't think this is actually necessary
	 *  since any calls on namei() will probably have to have come through
	 *  a system call and remote processing must have been specifically
	 *  enabled by the caller of namei().  Nevertheless, namei() will still
	 *  check this value before actually making a remote call and it nevers
	 *  hurts to be safe.
	 */
	if (u.u_error == EREMOTE)
		u.u_error = 0;
	u.u_rfscode = 0;
#endif	CS_RFS
#if	CS_RPAUSE
	switch (u.u_error)
	{
	    case ENOSPC:
	    {
		/*
		 *  The error number ENOSPC indicates disk block or inode
		 *  exhaustion on a file system.  When this occurs during a
		 *  system call, the fsfull() routine will record the file
		 *  system pointer and type of failure (1=disk block, 2=inode)
		 *  in the U area.  If we return from a system call with this
		 *  error set, invoke the fspause() routine to determine
		 *  whether or not to enter a resource pause.  It will check
		 *  that the resource pause fields have been set in the U area
		 *  (then clearing them) and that the process has enabled such
		 *  waits before clearing the error number and pausing.  If a
		 *  signal occurs during the sleep, we will return with a false
		 *  value and the error number set back to ENOSPC.  If the wait
		 *  completes successfully, we return here with a true value.
		 *  In this case, we simply restart the current system call to
		 *  retry the operation.
		 *
		 *  Note:  Certain system calls can not be restarted this
		 *  easily since they may partially complete before running
		 *  into a resource problem.  At the moment, the read() and
		 *  write() calls and their variants have this characteristic
		 *  when performing multiple block operations.  Thus, the
		 *  resource exhaustion processing for these calls must be
		 *  handled directly within the system calls themselves.  When
		 *  they return to this point (even with ENOSPC set), the
		 *  resource pause fields in the U area will have been cleared
		 *  by their previous calls on fspause() and no action will be
		 *  taken here.
		 */
		if (fspause())
		    u.u_eosys = RESTARTSYS;
		break;
	    }
	    /*
	     *  TODO:  Handle these error numbers, also.
	     */
	    case EAGAIN:
	    case ENOMEM:
	    case ENFILE:
		break;
	}
#endif	CS_RPAUSE
			ics_cs &= ~ICSCS_TESTBIT;
			if (u.u_eosys == RESTARTSYS)
				locr0[IAR] = opc; /* backup IAR to re-execute svc */
#ifdef notdef
			else if (u.u_eosys == SIMULATERTI)
				dorti();
#endif
			else if (u.u_error) {
				locr0[R0] = u.u_error;
				ics_cs |= ICSCS_TESTBIT;
			} else {
				locr0[R0] = u.u_r.r_val1;
				locr0[R2] = u.u_r.r_val2;
			}
			goto out;	  /* skip psignal */
		}
		/* end of case USER */

	case BKPT + USER:		  /* allow breakspoint inst only in user mode */
	case STEP + USER:		  /* allow inst step only in user mode */
		ics_cs &= ~ICSCS_INSTSTEP; /* probably off already */
		i = SIGTRAP;
		break;

	case VAST + USER:		  /* allow VAX AST only in user mode */
		astoff();		  /* once is enough */
		goto out;		  /* note: add profiling code before here */

	case ILL_OP + USER:		  /* illegal instruction */
	case PRIV_OP + USER:		  /* privileged instruction */
		u.u_code = mcs_pcs - USER;
		i = SIGILL;
		break;

	}
	/* end of switch mcs_pcs */
	 psignal(u.u_procp, i);
out:
	p = u.u_procp;
#if	MACH_MP
	if (p->p_cursig || ISSIG(p)) {
		if (CPU_NUMBER() != master_cpu) {
			unix_swtch(p, 1);
		}
		psig();
	}
#else	MACH_MP
	if (p->p_cursig || ISSIG(p))
		psig();
#endif	MACH_MP
	p->p_pri = p->p_usrpri;
#if	MACH_MP
	if (runrun && !master_idle) {
#else	MACH_MP
	if (runrun) {
#endif	MACH_MP
		/*
		 * Since we are u.u_procp, clock will normally just change
		 * our priority without moving us from one queue to another
		 * (since the running process is not on a queue.)
		 * If that happened after we setrq ourselves but before we
		 * swtch()'ed, we might not be on the queue indicated by
		 * our priority.
		 */
		(void) spl6();
#if	MACH_MP
/*		if (USERMODE(locr0[PS])) {  XXX - Just assume we're in user
						  mode for now. */
			u.u_ru.ru_nivcsw++;
			unix_swtch(p, 0);
/*		}
		else {
			u.u_ru.ru_nivcsw++;
			unix_swtch(p, 1);
		}*/
#else	MACH_MP
		setrq(p);
		u.u_ru.ru_nivcsw++;
		swtch();
#endif	MACH_MP
	}
/*     if (u.u_prof.pr_scale) {    More compile errors on this stuff  JEC
/*             int ticks;
/*             struct timeval *tv = &u.u_ru.ru_stime;
/*
/*             ticks = ((tv->tv_sec - syst.tv_sec) * 1000 +
/*                     (tv->tv_usec - syst.tv_usec) / 1000) / (tick / 1000);
/*             if (ticks)
/*                     addupc(locr0[PC], &u.u_prof, ticks);
/*     }
	 */ curpri = p->p_pri;
	return;

grave:
	spl7();
	prstate("trap", mcs_pcs, info, ics_cs, locr0);
	panic(epitaph);
}


/*
 * the register address map such that regs[regmap[0]] gets r0.
 */

char regmap[] = {
	R0, R1, R2, R3, R4, R5, R6, R7, R8, R9, R10, R11, R12, R13, R14, R15 };

/*
 * print the machine state
 * iar, mcs_pcs, ics_cs, general registers
 */
prstate(name, mcs_pcs, info, ics_cs, regs)
	register char *name;
	register int mcs_pcs, info, ics_cs;
	register int *regs;
{
	register int i;
	printf("%s: mcs_pcs=0x%b, info=0x%x, iar=0x%x, ics_cs=0x%b\n",
	    name, mcs_pcs, mcpcfmt,
	    info, regs[IAR], ics_cs, icsfmt);
	csr_print();
	if (regs) {
		for (i=0; i<16; ) {
			if ((i & 0x03) == 0)
				printf("R%x ... R%x ",i,i+3);
			prhex((char *) (regs + regmap[i]), sizeof (int));
			if ((++i & 0x03) == 0)
				printf("\n");
		}
	}
	printser();
}

prhex(ptr,n)
	register char * ptr;
	register int n;
{
/*
 * print "n" bytes pointed to by "ptr" in hex
 * we do this because the kernel printf isn't smart
 * enought to print out a specified number of digits.
 */
	while (--n >= 0) {
		putnibble(*ptr >> 4);
		putnibble(*ptr);
		++ptr;
	}
	printf(" ");
}

putnibble(n)
	register int n;
{
	printf("%c","0123456789abcdef"[n&0x0f]);
}

/*
 * nonexistent system call-- signal process (may want to handle it)
 * flag error if process won't see signal immediately
 * Q: should we do that all the time ??
 */
nosys()
{
	if (u.u_signal[SIGSYS] == SIG_IGN || u.u_signal[SIGSYS] == SIG_HOLD)
		u.u_error = EINVAL;
	psignal(u.u_procp, SIGSYS);
}


csr_print()
{
	register int csr = *(int *)CSR;
	printf("CSR=%b\n", csr,
#ifdef SBPROTO
	    "\20\13PANIC\14LOAD\15POWER\16MULTIPLE-ERROR\17PENDING\24TEST&SET\25DMA-READ-REPLY\26PROTECTION\27READ-WRITE\30RSC-RETRY\31DMA1\32DMA2\33DMA4\36VIRTUAL\37DMA-READ/WRITE\40RSC-TERMINATED");
#endif
#ifdef SBMODEL
			"\20\11PIO-PENDING\12PLANAR-BUSY\13CH-RESET-CAPTURED\14DMA-EXECPTION\15I/O-CHECK\16INV-OPERATION\17PROT-VIOLATION\20PIO-DMA\21DMA-ERR-CH8\22DMA-ERR-CH7\23DMA-ERR-CH6\24DMA-ERR-CH5\25DMA-ERR-CH4\26DMA-ERR-CH3\27DMA-ERR-CH2\30DMA-ERR-CH0\31PIO-ERR\33SYSTEM-ATTN\34SOFT-RESET\35POWER\37INTR-PENDING\40EXCEPTION");
#endif
	_csr = csr;		/* save it for later */
	}


#include "../machine/fp.h"
#include "../machine/softint.h"

void
saveFP(savearea)
	FP_MACH * savearea;
{
	*savearea = machine;
}


void
restoreFP(savearea)
	FP_MACH * savearea;
{
	machine = *savearea;
}


void
slih6(dev, icscs, iar)
	register dev_t dev;
	register int icscs;
	register int iar;
{
	FP_MACH fpregs;

	saveFP(&fpregs);

	while (softlevel) {

		if (softlevel & SOFT_CLOCK) {
			softlevel &= ~SOFT_CLOCK;
			softclock(dev, icscs, iar);
		}
		if (softlevel & SOFT_NET) {
			softlevel &= ~SOFT_NET;
#ifdef INET
			softnet();
#endif
		}
		if (softlevel & ~SOFT_ALL) {
			printf("slih6: undefined bit(s) set in softlevel: %x\n",
			    softlevel);
			softlevel &= SOFT_ALL;
		}
	}
	restoreFP(&fpregs);
}


#define ON(x) (x) ? "1" : "0"

#define K * 1024
#define M * 1024 * 1024

static int memsizes[16] = {
	0, 64 K, 64 K, 64 K, 64 K, 64 K, 64 K, 64 K,
	128 K, 256 K, 512 K, 1 M, 2 M, 4 M, 8 M, 16 M
};

printser()
{
/* cannot use ROSE_IOBASE because it doesn't return a value (DUMB!) */
	register int rosebase = ROSEBASE; /* get base address */
	register int ser;
	register int i;
	register int reg;
	register int hatipt;
	register int ramsize;
	register int pagesize;

#ifndef KERNEL
	for (i = 0; i < RTA_NSEGREGS; i += RTA_SEGREGSTEP) {
		reg = ior(rosebase + ROSE_SEGR + i);
		printf("SR%02d = %8x (P=%s, C=%s, I=%s, SID=%x S=%s K=%s)\n",
		    i, reg, ON(reg & SEG_P), ON(reg & SEG_C), ON(reg & SEG_I),
		    (reg & SEG_SID) >> SEG_SID_SHIFT, ON(reg & SEG_S), ON(reg & SEG_K));
	}
/* printf("I/O Base address=%x (%x)\n",rosebase>>16,rosebase);	/* debug */
#endif KERNEL
	ser = ior(rosebase + ROSE_SER);	/* pick up the SER */
	printf("SER=%b ", ser,
	    "\20\1DATA\2PROTECTION\3TLB-SPECIFICATION\4PAGE-FAULT\5MULTIPLE\6EXTERNAL-DEVICE\7IPT-SPECIFICATION\10ROS-WRITE\12TLB-RELOAD\13CORRECTABLE-ECC\14STORAGE\15LOAD\16I/O-ADDRESS\17STORAGE-ADDRESS\20RSC-NAKDN\21RSC-NAKDA\22SEGMENT-VIOLATION");
	printf("SEAR=%x ", ior(rosebase + ROSE_SEAR));
	printf("TRAR=%x ", ior(rosebase + ROSE_TRAR));
	reg = ior(rosebase + ROSE_TCR);
	printf("TCR=%b HAT/IPT=%x\n", reg,
	    "\20\114K-PAGE\12RESERVED\13INTR-TLB-RELOAD\14INTR-CECC\15TLIPT\16RAS-DIAG\17ISPER\20V=R", hatipt = reg & 0xff);
#ifndef KERNEL
	pagesize = (reg & 0x100) ? 4096 : 2048;
	reg = ior(rosebase + ROSE_RAM);
	ramsize = memsizes[reg & 0x0f];
	printf("RAM=%x (start=%x, size=%x) ", reg, (reg & 0xff0) << 12, ramsize);

	reg = ior(rosebase + ROSE_ROS);
	i = memsizes[reg & 0x0f];
	printf("ROS=%x (PARITY=%s start=%x, size=%x)\n", reg, ON(reg & 0x1000), (reg & 0xff0) << 12, i);
	printf("HAT/IPT addr=%x\n", hatipt * (ramsize * 32 / pagesize));
#endif
}



/*
 *	This handles a mach syscall (one whose number is less than the cmu
 *	extra syscalls in this implementation).
 */

do_mp_syscall(info)
register info;

{
 int arg0 = u.u_arg[0], arg1 = u.u_arg[1], arg2 = u.u_arg[2];
 int syscall_no;

 syscall_no = -(info<<16)>>16;	/* Extend sign bit hack */

 return(u.u_r.r_val1 = (*(mp_sysent[syscall_no].a_fn))(arg0,arg1,arg2)); 
}
