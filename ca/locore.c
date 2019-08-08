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
/* Warning: It is possible that turning on interrupt debugging can 
 * cause problems when there are lots of interrupts happening
 * simultaneously.  It should only be used when problems
 * are occuring very early during the boot sequence (before the
 * clock is started).
 */

/* $Header: locore.c,v 4.9 85/09/16 17:00:31 webb Exp $ */
/* $Source: /ibm/acis/usr/sys_ca/ca/RCS/locore.c,v $ */

#if	CMU
 #######################################################################
 # HISTORY
 #  5-Mar-86  Bill Bolosky (bolosky) at Carnegie-Mellon University
 #	Changed map of locore to allow sufficient room for the Mach
 #	shared memory map, the Mach Symbol table (if the debugger is in
 #	place) and the debugger.  Locore now is much bigger than it was
 #	in the ibm version.
 #
 #######################################################################
#include "romp_debug.h"
#include "mach_shm.h"
#include "mach_load.h"
#include "mach_vm.h"
#endif	CMU

 #  This module contains the low level code for Mach/4.3 running on the
 #  IBM AUSTIN SAILBOAT prototype workstation.
 #  This is a modification of the Original YORKTOWN port for a PC/XT ROMP
 #  configuration by IRIS at Brown University.
 #  The major functions of this code are:
 #
 #  . Initialize the ROMP after a "power on reset"
 #  . Intercept program checks, machine checks, and trap instructions.
 #  . Field and route processor interrupts.
 #  . Provide subroutines to access the special machine registers.
 #  . Provide highly efficient subroutines for some common functions.
 #
 # An interface it provided to a debugger (originally "RDB") that
 # co-operates with the kernel in order to provide a debugging
 # environment with the kernel.
 # the debugger is linked to run in real mode, while the kernel is linked
 # for virtual mode. the addresses are allocated as follows:
 # (when the debugger is present)
 # 0-ff          not really used much
 # 100-1af       interrupt vectors (shared)
 # 1b0-200       unused
 # 200-800       debugger stack (runs down from 800)
 # 800-fff       POST (must be preserved for control-alt-del to work)
 # 1000-4fff     kernel locore code
 # 5000-efff    kernel symbol table
 # f800-1ffff   debugger (DEBUG, used to be RDB)
 # ...           rest of the kernel (C code)
 #
 # the interrupt vectors are shared as follows:
 # 1. the kernel initializes then statically
 # 2. the debugger takes over 100, 170, 180, 190, 1a0 (pseudo vector)
 # 3. the kernel takes back   100, 170, 180, 190 and remembers the
 #       debuggers values so that it can pass control to the debugger
 #         
 # when the debugger is not present then the original static values
 # remain in effect.
 #         
	  
 
         .text                   # everything goes into text (even data)

 # locore.h defines constants etc. but does not generate any code/data.
#include "../ca/locore.h"
#if	MACH_VM
#include "../ca/pmap.h"		# useful #defines
#endif	MACH_VM

 # lointpsw sets up the interrupt vectors and leaves space for POST,
 # debugger etc.

#include "../ca/lointpsw.s"
 #       JUMP TO the REAL Locore START .....  WHEREEVER THAT MAY BE
 #
 #       following code normally lands at 0x1000 (on Model).
 #
	.globl _start
#ifndef	RDB
_start:
#endif	RDB
	.using	real0,r0	# tell the assembler to use r0 as base
	cal16	r15,start	# use 'cal16' to ignore segment number
	balr	r15,r15		# transfer control to real 'start'
origin: .short 0x8080            # trap for undefined external references
	.align	2

#if	MACH_SHM
#include "../mp/shmem.h"
#endif	MACH_SHM
#include "../ca/scr.h"
#include "../ca/rosetta.h"
#include "../ca/pte.h"
#include "../h/param.h"
#include "proc.m"
#include "../ca/pcb.m"

/****************************************************************************/
/* THESE SHOULD BE AUTOMATICALLY GENERATED LIKE WITH genassym.c FOR THE VAX */

	/* From h/param.h */
/* #define NBPG 2048  Should replace PAGESIZE, below */
/* #define PGSHIFT 11 Should replace LOG2PAGESIZE, below */
#undef NPTEPG
#define NPTEPG 512	/* NBPG / sizeof(struct(pte)) */
#define LOG2NPTEPG 9	/* (well, param.h is where it SHOULD be!) */
/* #define MAXBSIZE 8192	/* max file system block size */
 
	/* From romp/vmparam.h */
#define ENDOFP1 0x20000000
#define USTRUCT ENDOFP1-USIZE
#if	MACH_VM
#define UAREA	(ENDOFP1 - UPAGES * NBPG)
#endif	MACH_VM
#define KERNSTACK ENDOFP1-USIZE
#define P1PAGES 0x20000
#define USRPTSIZE 1*NPTEPG	/* one page worth of page table entries */
 
	/* From h/msgbuf.h */
#define MSGBUFPAGES 2		/* Size of console message buffer in pages */

	/* From h/mbuf.h */
#define NMBCLUSTERS 256		/* Number of memory buffer clusters */

	/* Should be in h/buf.h! */
#define MAXBUFFERPAGES 512	/* 1MB worth of buffers should be enough */

/* There's probably more than this */
/****************************************************************************/
 
 #==================| hardware related constants |==================|
 
#define SEGSIZE  0x10000000		/* 256M bytes per rosetta segment */
#define PAGESIZE  2048			/* 2K bytes per rosetta page */
#define LOG2PAGESIZE 11		/* shift count for 2K page size */

#define MAXPHYSMEM  0x1000000		/* (bytes) Not more than 16M real mem */
 
#define UPPER 0x10000			/* scale factor for UPPER instructions */

 #==============| HAT-IPT - Located in the last pages of memory |=============|
 
#define HATIPTSIZE 16                  /* size of one HAT IPT entry */
#define LOG2HATIPTSIZE  4               /* shift for HAT IPT entry */
 
 #  fields of a HAT-IPT entry:
 
#define SYS_ADDRTAG RTA_SID_SYSTEM*P1PAGES /* k1,k2 = 0,0; sid = system */

#define IPTADDRTAG  0                   /* offset of IPT ADDR TAG field */
#define IPTHATPTR  4                    /* offset of IPT HAT PTR field */
#define IPTIPTPTR  6                    /* offset of IPT IPT PTR field */
#define IPTLOCK  8                      /* offset of IPT LOCK WORD field */

 #=====================| constants and variables |======================|

	.globl	_u
.set	_u,	USTRUCT		# address of u area for debugging
         .globl _RTA_HATIPT
	 .globl _RTA_HASHMASK
         .globl _firstaddr
	 .globl _maxmem
	 .globl _physmem
	 .globl _freemem
         .globl _nrpages
         .globl _noproc
	 .globl _runrun
	 .globl _whichqs
	 .globl _qs
	 .globl _boothowto
	 .globl	_memconfig
	.globl	_ser
	.globl	_sear
	.globl	__trap
	.globl	_trap
         .align 2		# *tjm
 # 
 #  Masterpaddr is the p->p_addr of the running process on the master
 #  processor.  When a multiprocessor system, the slave processors will have
 #  an array of slavepaddr's.
 #
	.globl	_masterpaddr
_masterpaddr:
	.long	0
#if	MACH_MP
	.globl	_slave_proc
_slave_proc:
	.long	0
	.long	0
	.long	0
	.long	0
	.globl	_master_idle
_master_idle:
	.long	0
#endif	MACH_MP
#if	MACH_LOAD
	.globl	_loadpg
_loadpg:
	.long	0
#endif	MACH_LOAD
 #
 # note: autoconfig will change the value of _trap to intercept
 # traps during autoconfig that would confuse the regular
 # trap routine.
 # it will reset it back before returning control to the caller
 #
__trap:	.long	_trap		# address of trap routine
	.globl	_cnt
__cnt:	.long	_cnt		# address of cnt
 # hatipt is used in non-translated code, _RTA_HATIPT when translate on
 # note: following 16 words are set to zero at 'start' time.
 #

hatipt:		.long	0		# real address of hatipt
_RTA_HATIPT:	.long	0		# virtual address of hatipt
_RTA_HASHMASK:	.long	0		# mask for virt addr hashing
_nrpages:      .long 0		# number of pages installed
_firstaddr:	.long	0		# first page past static kernel
_maxmem:	.long	0		# top of available memory (pages)
_physmem:	.long	0		# size of installed memory (pages)
_freemem:	.long	0		# size of allocatable memory (pages)
_noproc:	.long	0		# no process running now
_runrun:	.long	0		# something to run
_whichqs:	.long	0		# "queue non-empty" flags
_qs:     .fill 2*4*16,4,0        # 16 queue headers: RLINK, LINK
_boothowto:	.long	0		# passed from boot to init (in r11)
_memconfig:	.long	0		# software memory config
_ser:		.long	0		# SER from last trap
_sear:		.long	0		# SEAR from last trap
_cnt:		.fill 32,4,0		# meter counts (see ../h/vmmeter.h)
	 
#ifdef RDB
#define RDBADRLOC  0x210	/* Address where rdb's address will be storred by build */
callrdb_ps:
	.long  RDBADRLOC		# new iar (Will be set to rdb's addr)
	.short NOTRANS_ICS+INT_PRI0	# new ics: xlate off, int level 0
	.short 0			# reserved
#endif RDB
 
callmain_ps:
         .long  _main               # new iar
         .short TRANS_ICS+INT_PRI0  # new ics: xlate on, int level 0
         .short 0                   # reserved
go_callslih:
         .long  callslih            # new iar
         .short TRANS_ICS           # new ics: xlate on
         .short 0                   # reserved
go_afterslih:
         .long  afterslih-real0     # new iar
         .short NOTRANS_ICS+INT_PRI1# new ics: xlate off, PRIO 1
         .short 0                   # reserved
go_ps:
         .long  go                  # new iar
         .short TRANS_ICS+INTMASK_ICS+INT_PRI1 # new ics: xlate off, interrupts off
         .short 0                   # reserved
realgo_ps:
         .long  realgo-real0            # new iar
         .short NOTRANS_ICS+INTMASK_ICS+INT_PRI1 # xlate off, interrupts off
         .short 0                       # reserved
go_now_ps:
	 .long  go_now-real0
	 .short NOTRANS_ICS+INTMASK_ICS+INT_PRI1
	 .short 0
fault_ps:
         .long  fault               # new iar
         .short TRANS_ICS+INTMASK_ICS # new ics: xlate on, interrupts off
         .short 0                   # reserved
aftermain_ni_ps:
	 .long  aftermain_ni	  #iar
	 .short TRANS_ICS+INTMASK_ICS+INT_PRI1 # xlate off, interrupts off
	 .short 0
#if	MACH_VM
callsetup_ps:
         .long  _setup_main               # new iar
         .short TRANS_ICS+INT_PRI1  # new ics: xlate on, int level 1
         .short 0                   # reserved
callrompinit_ps:
         .long  _romp_init               # new iar
         .short TRANS_ICS+INT_PRI1  # new ics: xlate on, int level 1
         .short 0                   # reserved
#else	MACH_VM
zzswun_ps:
         .long  zzswun-real0        # new iar
         .short NOTRANS_ICS+INTMASK_ICS # new ics: xlate off, interrupts off
         .short 0                   # reserved
zzswxl_ps:
         .long  zzswxl              # new iar
         .short TRANS_ICS+INTMASK_ICS # new ics: xlate on, interrupts off
         .short 0                   # reserved
#endif	MACH_VM
csegxlat_ps:
         .long  csegxlat            # new iar
         .short TRANS_ICS           # new ics: xlate on
         .short 0                   # reserved

 
 #  register save areas:

low_ps     :.long 0,0           # addressable space for lps
low_save0  :.long 0             # save for r0
low_save1  :.long 0             # save for r1
low_save2  :.long 0             # save for r2
low_save3  :.long 0             # save for r3
low_save4  :.long 0             # save for r4
low_save5  :.long 0             # save for r5
low_save6  :.long 0             # save for r6
low_save7  :.long 0             # save for r7
low_save8  :.long 0             # save for r8
low_save9  :.long 0             # save for r9
low_save10 :.long 0             # save for r10
low_save11 :.long 0             # save for r11
low_save12 :.long 0             # save for r12
low_save13 :.long 0             # save for r13
low_save14 :.long 0             # save for r14
low_save15 :.long 0             # save for r15

#ifdef VTL_RELOCATE
pck_save9  :.long 0             # save for r9 for pck
pck_save10 :.long 0             # save for r10 for pck
pck_save11 :.long 0,0,0,0,0     # save for r11,12,13,14,15 for pck
#endif
flipsave   :.long 0,0,0,0       # save for flipmode subroutine

#if	MACH_VM
/*
 *	The VM system initializes the sigcode out of setup_main, so it
 *	needs to be global.
 */
	.globl	_sigcode
_sigcode:
#endif	MACH_VM
 # Signal trampoline code
sigcode: svc   139(r0)              # template moved to u area for signals

 # External I/O Interrupt Save Area Storage
 # Moved here from lointr for addressability
	.align 2
#if	MACH_VM
		.globl  _iosavep
_iosavep:
#endif	MACH_VM
iosavep:	.long	iounmask-real0	# Interrupt Level Stack Pointer
					# (Initially Points to iounmask)
iosavea:	.space 19*IOSAVEL 	# Interrupt Level Save Area Stack
iounmask:	.long	0xFFFFFFFF	# Main Level "Interrupt Enable" Mask
					# (Must Immediately Follow iosavea)
ioenable:	.long   SETIO		# Master Interrupt Enable Word
iosave6:	.long	0x00000000	# R6 save Area for I/O Interrupts

 
 #  kernel profiling data
 
#define kpf_base 0x3000         /* base address of kernel profiling vector */
#define kpf_hz    10            /* (profiling ticks) / (Unix hardclock ints) */
           .globl _kpf_onoff
_kpf_onoff :
kpf_onoff  :.long 0             # kernel profiling on/off (initially off)
kpf_ticks  :.long 0             # counter
 
 #  literals:

_origin$   :.long  origin       # base address of locore
_edata$    :.long  _edata
_end$      :.long  _end
#ifdef RDB
_szsymtbl$ :.long  0x200	# Hard wired address of size of symbol tbl.
#endif RDB
#if	MACH_VM
	   .globl  _loadpt
_loadpt    :.long  0xe0000000	# Hard wired load point address XXX
#else	MACH_VM
_Usrptmap$ :.long  _Usrptmap
_usrpt$    :.long  _usrpt
_vtop$     :.long  _vtop
#endif	MACH_VM
_trap$     :.long  _trap
aftermain$ :.long  aftermain
#ifndef ROMPXT
_printf$   :.long  _printf
_panic$    :.long  _panic
real0$    :.long  real0
#endif ROMPXT
#if	MACH_VM
 a_tmp_stk:
	.long	tmp_stack
 a_after_init:
	.long	after_init
 a_after_setup:
	.long	after_setup
 a_active_threads:
	.long	_active_threads
 a_load_context:
	.long	_load_context
#endif	MACH_VM
#ifdef VTL_RELOCATE
 #================|  tlb lru flags (one byte per tlb pair) |===============|

tlblru:     .fill RTA_NTLBS/4,4,0
.set tlbvec , tlblru - real0
#endif

#if	MACH_VM
#else	MACH_VM
 #========== SYSTEM PAGE TABLES AND DYNAMICALLY MAPPED DATA AREAS ===========|

/* Assign the specified virtual address to the given label */
#define SET_ADDR(name, addr) \
	.globl _/**/name; \
	.set _/**/name , addr
	
/* Allocate some pages in system segment, location unimportant */
#define GRAB_SOME_VMEM(vname, npages) \
	SET_ADDR(vname, nextvaddr); \
	.set nextvaddr , npages*PAGESIZE + nextvaddr

/* Allocate some VAX page table space in locore data area */
#define GRAB_SOME_PT(ptname, npte) \
	.globl _/**/ptname; \
	_/**/ptname:; \
	.fill	npte,4,0

/* Dedicate npages virtual address space, assign to given label */
#define MAPSYS(vname, npages) \
	GRAB_SOME_VMEM(vname, npages)

/* Same as above, except also allocate a VAX page table to map it */
#define MAPSYS_PT(ptname, vname, npages) \
	GRAB_SOME_VMEM(vname, npages); \
	GRAB_SOME_PT(ptname, npages)
/* Simply equate the given label with the given address */
#define MAPSYS_ADDR(vname, vaddr) \
	SET_ADDR(vname, vaddr)

/* Declare that npages at vaddr are dedicated to vname;
   also allocate enough VAX page table to map it */
#define MAPSYS_PT_ADDR(ptname, vname, npages, vaddr) \
	SET_ADDR(vname, vaddr); \
	GRAB_SOME_PT(ptname, npages)

.set nextvaddr , SYS_ORG + MAXPHYSMEM

/* Rationale behind this is at most MAXBUFFERPAGES buffers (one page
   per buffer) times size of buffer (MAXBSIZE) */
#define BUFFERSPACE MAXBSIZE/PAGESIZE*MAXBUFFERPAGES

	.align 2
 #                 PT NAME   VNAME    SIZE (PAGES)        MAP AT VADDR
 #		  -------   -----    ------------        ------------
          MAPSYS           (bufbase  ,BUFFERSPACE                         )
	  MAPSYS           (buflimit ,0                                   )
       MAPSYS_PT (Usrptmap ,usrpt    ,USRPTSIZE                           )
       MAPSYS_PT (camap    ,cabase   ,16*CLSIZE                           )
          MAPSYS           (calimit  ,0                                   )
       MAPSYS_PT (Mbmap    ,mbutl    ,NMBCLUSTERS*CLSIZE                  )
          MAPSYS           (msgbuf   ,MSGBUFPAGES                         )
#if	MACH_SHM
       MAPSYS_PT (shmap	   ,shutl    ,SHMAPGS				  )
       MAPSYS_PT (eshmap   ,eshutl   ,0					  )
#endif 	MACH_SHM
  MAPSYS_PT_ADDR (Forkmap  ,forkutl  ,UPAGES            ,1*SEGSIZE+USTRUCT)
  MAPSYS_PT_ADDR (Xswapmap ,xswaputl ,UPAGES            ,2*SEGSIZE+USTRUCT)
  MAPSYS_PT_ADDR (Xswap2map,xswap2utl,UPAGES            ,3*SEGSIZE+USTRUCT)
  MAPSYS_PT_ADDR (Swapmap  ,swaputl  ,UPAGES            ,4*SEGSIZE+USTRUCT)
  MAPSYS_PT_ADDR (Pushmap  ,pushutl  ,UPAGES            ,5*SEGSIZE+USTRUCT)
  MAPSYS_PT_ADDR (Vfmap    ,vfutl    ,UPAGES            ,6*SEGSIZE+USTRUCT)
     MAPSYS_ADDR           (copybase                    ,8*SEGSIZE        )

     .globl _sys_seg_end
.set _sys_seg_end , nextvaddr

	.globl _buffers
_buffers:	.long	_bufbase		# pointer to buffer pool

 
 #====================== USERS PAGE TABLES ========================|
 
 #  virtual address of the user page table
 
.set USRPT   , _usrpt - SYS_ORG  # offset of user page table in segment
.set USRPT_PAGE , USRPT / PAGESIZE # page number of user page table
.set USRPT_TLB , USRPT_PAGE/RTA_NTLBS*RTA_NTLBS*-1+USRPT_PAGE # low 6 bits of pg
.set USRPT_TLBW1 , USRPT_PAGE / RTA_NTLBS * 0x200 + 0x00000000 # sid 000
.set USRPT_TLBW2 , 0xffffc000 # lock bits 1's, w=1,k=1,0 (public r/w)
.set USRPT_ADDRTAG , SYS_ADDRTAG + USRPT_PAGE
 
 #  virtual address of the user structure and system stack
 
.set UAREA , -UPAGES*NBPG + ENDOFP1 # address of u area
.set UAREA_PAGE , (UAREA - SEGSIZE) / PAGESIZE # page number of u area
.set UAREA_TLB , UAREA_PAGE/RTA_NTLBS*RTA_NTLBS*-1+UAREA_PAGE # low 6 bits of pg 1
.set UAREA_TLBW1 , UAREA_PAGE / RTA_NTLBS * 0x200 + 0x00000000 # sid 000
.set UAREA_TLBW2 , 0xffffc000 # lock bits 1's, w=1,k=1,0 (public r/w)
.set UAREA_ADDRTAG , UAREA_PAGE
 
 #========================| execution starts here |======================|
#endif	MACH_VM
 
        .globl start
eye_catcher(start):

 #  zero registers (as after a POR reset)
 
	st	r11,_boothowto		# save first reg arg for init
 s r0,r0;s r1,r1;s r2,r2;s r3,r3;s r4,r4;s r5,r5;s r6,r6
 s r7,r7;s r8,r8;s r9,r9;s r10,r10;s r11,r11;s r12,r12
 s r13,r13;s r14,r14;s r15,r15
 
         mts scr_ts,r15			# reset romp timer status
#if defined(RDB)
	stm	r0,hatipt		# reset initial values
	stm	r0,_qs			# clear 16 entries
	stm	r0,_qs+(64*1)		# clear 16 entries
	stm	r0,_qs+(64*2)		# clear 16 entries
	stm	r0,_qs+(64*3)		# clear 16 entries
	stm	r0,_qs+(64*4)		# clear 16 entries
	stm	r0,_qs+(64*5)		# clear 16 entries
	stm	r0,_qs+(64*6)		# clear 16 entries
	stm	r0,_qs+(64*7)		# clear 16 entries
	cal	r0,iounmask-real0(r0)	# get the interrupt stack base
	st	r0,iosavep		# more re-entrant than before
#else
 # following is not done for RDB so that one can step thru locore
 # using the debugger (otherwise the following turns of IRQ_0)
	mts scr_irb,r15			# reset romp interrupt request buffer
#endif
	cau r2,(ROSEBASE)>>16(r0)     	# i/o base address of Rosetta
	oil r2,r2,(ROSEBASE)&0xffff     # i/o base address of Rosetta
        iow   r14,ROSE_SER(r2) 		# clear the exception register

 # Initialize the Interrupt system

 # Initially turn off all I/O BUS Interrupt mapping
         cau     r2,IOBASE(r0)           # Get IO base bus address
	 cau     r0,(SETIO)>>16(r0)      # Load SET IO INT to ROMP INT constant
	 oil     r0,r0,(SETIO)&0xffff

         st      r0,ioenable             # Save initial IOENABLE
#ifdef SBPROTO 
 # Set bit pattern to map Interrupts in SBPROTOTYPE
         cau     r2,IOBASE               # Get IO base bus address
         stc     r0,IMRD(r2)             # Load interupts on IRQ0->IRQ3
         sri     r0,8
         stc     r0,IMRC(r2)             # Load interupts on IRQ4->IRQ7
         sri     r0,8
         stc     r0,IMRB(r2)             # Load interupts on IRQ8->IRQ11
         sri     r0,8
         stc     r0,IMRA(r2)             # Load interupts on IRQ12->IRQ15
#endif SBPROTO
 
#ifdef RDB
 #  save pck new iar (only once per memory load)
 #  the interrupt vectors were set by us, and have been taken
 #  over by the debugger.
 #  we will take back the program check and save the debugger's new iar
 #  so that we can call it when we have to.
 #  interrupt level 0 and svc areas are treated similarly.
 #
 
         l     r0,pck_ps+new_iar       # pick up existing pck iar
         cal16 r1,pck0                 # pick up my pck iar
         c     r0,r1                   # ?. points to us?
         je    pck_ok                  # no need to insert our pck addr
         cal16 r2,pck_jump_to_dan      # address of br instr.
         s     r0,r2                   # offset to him
         sri   r0,1                    # divide by 2
         niuo  r0,r0,0x000f               # clear top 12 bits
         oiu   r0,r0,0x8880               # branch uncond
         sts   r0,0(r2)                # make it jump to him
#if !defined(LORDB)
         st    r1,pck_ps+new_iar       #  set up pck new iar
#endif
pck_ok:
         l     r0,mck_ps+new_iar       # pick up existing mck iar
         cal16 r1,mck0                 # pick up my mck iar
         c     r0,r1                   # ?. points to us?
         je    mck_ok                  # no need to insert our mck addr
         cal16 r2,mck_jump_to_dan      # address of br instr.
         s     r0,r2                   # offset to him
         sri   r0,1                    # divide by 2
         niuo  r0,r0,0x000f               # clear top 12 bits
         oiu   r0,r0,0x8880               # branch uncond
         sts   r0,0(r2)                # make it jump to him
#if !defined(LORDB)
         st    r1,mck_ps+new_iar       #  set up mck new iar
#endif
mck_ok:
#ifdef notdev
         l     r0,_vtop$               # get address of vtop routine
         st    r0,0xF0(r0)             # plop it down in low core for debugger
         cau   r0,0x0280(r0)           # get ics and cs for vtop routine
         st    r0,0xF4(r0)             # plop it down in low core for debugger
#endif notdev
 
 #  set up svc new ps:  svc0, no translation, interrupts masked, level 0
 
        cal16 r0,svc0                  # address of svc handler
        st    r0,svc_ps+new_iar        # store in new ps
	cau   r0,(NOTRANS_ICS+INTMASK_ICS)>>16(r0) #ics: no transl, ints masked
	oil   r0,r0,(NOTRANS_ICS+INTMASK_ICS) &0xffff
        sth   r0,svc_ps+new_ics        # store in new ps
 
 #  save int0 new iar (only once per memory load)
 
         l     r0,int0_ps+new_iar      # pick up existing int 0 iar
         cal16 r1,int0                 # pick up my int 0 iar
         c     r0,r1                   # ?. points to us?
         je    int0_ok                 # no need to insert our int addr
         cal16 r2,int0_jump_to_dan     # address of br instr.
         s     r0,r2                   # offset to him
         sri   r0,1                    # divide by 2
         niuo  r0,r0,0x000f               # clear top 12 bits
         oiu   r0,r0,0x8880               # branch uncond
         sts   r0,0(r2)                # make it jump to him
#if !defined(LORDB)
	 st	r1,int0_ps+new_iar	# set up int 0 new iar
#endif
int0_ok:
#endif RDB
 
	.globl _clrloop
_clrloop:


#include "../ca/lohatipt.s"

#if	MACH_VM
	/*
	 *	Perform basic Romp initialization. Done in virtual mode, thus
	 *	it is necessary to have set up the hat/ipt by now (done by
	 *	lohatipt.s)
	 */
	
	.using	real0,r0		#set up base register (no base mode)
	l	sp,a_tmp_stk		#temporary stack for setup stuff.
	l	r15,a_after_init
	lps	0,callrompinit_ps

	
	/*
	 *	Turn on virtual memory & call setup_main.
	 */
after_init:
	bali	r14,new_base
new_base:
	.using	new_base,r14
	l	r15,a_after_setup	#sp left over from last one.
	lps	0,callsetup_ps

	/*
	 *	Set up the initial PCB.
	 */

after_setup:
	bali	r12,another_base	#where I come from, r12 is the base...
another_base:
	.using	another_base,r12
	cas	r2,r0,r0		# parameter to load_context
	l	r14,THREAD_PCB(r2)
	st	r14,PCB_R14(r14)
	l	r15,a_load_context
	balr	r15,r15			# load_context
	.globl	_after_lc
_after_lc:
	bali	r12,yab
yab: 					#yet another base
	.using	yab,r12
/* initialize signal trampoline in u area */
	l	r0,sigcode			# svc 139 instruction
	st	r0,PCB_SIGC(r14)		# place to return to after a handler

        l	r15,aftermain$
	lps	0,callmain_ps
	.align	2
	.fill	1024,4,0
tmp_stack:					# label comes after, because
						# stack builds down

#else	MACH_VM
 # initialize the pcb's
	.globl  _lopcb
_lopcb:
 #  now r2 is page number of first u area page.
 
         slpi  r2,LOG2PAGESIZE      # r3 -> u area
	 cau   r0,(UPAGES*NBPG-USIZE)>>16(r0) # offset in the u area of the u struct
	 oil   r0,r0,(UPAGES*NBPG-USIZE)&0xffff
         a     r3,r0                # r3 -> u struct
 #  initialize (slightly) the first pcb
	 cau   sp,(KERNSTACK-12)>>16(r0) # reserve 12 bytes for parms to main
	 oil   sp,sp,(KERNSTACK-12)&0xffff
         st    r1,PCB_KSP(r3)       # kernal stack pointer
         st    r8,PCB_ESP(r3)       # -1 (from before)
         st    r8,PCB_SSP(r3)       # ditto
         st    r3,PCB_USP(r3)       # not exactly correct!!!
         l     r13,_usrpt$          # address of page table map
         st    r13,PCB_P0BR(r3)     # point to user page table map
         s     r15,r15              # no p0 pages
         st    r15,PCB_P0LR(r3)     # set length register
	 cau   r14,(P1PAGES-UPAGES)>>16(r0) # number of unused page in P1
	 oil   r14,r14,(P1PAGES-UPAGES)&0xffff
         st    r14,PCB_P1LR(r3)     # P1 page tables for all of it !?
         sli   r14,2                # times 4 bytes per pte
         cal   r15,-4*UPAGES+NBPG(r13) # offset of ptes for u area
         s     r15,r14              # back up by size of p1 page table
         st    r15,PCB_P1BR(r3)     # thats p1 base
	 cau   r0,(CLSIZE)>>16(r0)   # number of pages in a click
	 oil   r0,r0,(CLSIZE)&0xffff
         st    r0,PCB_SZPT(r3)      # number of ptes
 #  initialize signal trampoline in u area
         l     r0,sigcode           # svc 139 instruction
         st    r0,PCB_SIGC(r3)      # place to return to after a handler
 #  compute page number past u area
         ai    r2,r2,UPAGES         # past u area
 #  set return address to continue after calling main
         l     r15,aftermain$
 #  set up r11 to contain the 'howto' value from boot to be passed to init
	l       r11,_boothowto
 #  go to translate mode, interrupts disabled, level 0
         lps   0,callmain_ps        # translate mode, routine _main
#endif	MACH_VM
 #         .cnop 0,4 **MOVED TO LOWER CORE FOR ADDRESSABILITY**
 #sigcode: svc   139(r0)              | template moved to u area for signals
1:       .using 1b
 #
 #  resume here after _main, in proc 0 (/etc/init);  jump to icode
 #  note: This code now munges the sp, so it is important that the
 #  interrupt mask gets turned on here.
 #
eye_catcher(aftermain):
	 .using aftermain,r15	    # use the return register as base
	 lps   0,aftermain_ni_ps    # mask interrupts
aftermain_ni:
         lis   r0,0                 # begin at address zero
         st    r0,low_ps            # place in low_ps

	 # transl on, prob stt, lev 7
	 cau   r0,(TRANS_ICS+PROBSTATE_ICS+INT_PRI7)>>16(r0)
	 oil   r0,r0,(TRANS_ICS+PROBSTATE_ICS+INT_PRI7)&0xffff
         sth   r0,low_ps+4          # place in low_ps
         cau   sp,(USTRUCT+PCB_ICSCS)>>16(r0) # sp-> icscs field of pcb
         oil   sp,sp,(USTRUCT+PCB_ICSCS)&0xffff # sp-> icscs field of pcb
	 sth   r0,0(sp)		    # plop in pcb, too.
	 l     r11,_boothowto
	 lps   0,go_ps		    # jump to go, interrupts off.
1:       .using 1b                  # drop base register
 #
 #  go - reload registers and old ps from low core
 #      on entry:
 #          interrupts are masked <=== VERY IMPORTANT AS SP INVALIDATED
 #          translation is on
 #          low_save1,low_save15, and low_ps are set
 #	   r2 ... r14 have the required final values (lm already done)
eye_catcher(go):
         mfs   scr_iar,r15          # use r15 as base register
1:	 .using 1b,r15              # tell assembler r15 is base register
         l     sp,low_ps+old_ics_cs # get old icscs
         mttbiu sp,PROBSTATE-16     # problem state?
         jntb  switch_n_go          # no...returning to system	 
         cau   sp,(USTRUCT+PCB_ICSCS)>>16(r0) # sp-> icscs field of pcb
         oil   sp,sp,(USTRUCT+PCB_ICSCS)&0xffff # sp-> icscs field of pcb
         ls    sp,0(sp)             # sp = icscs field of pcb
         lps   0,realgo_ps          # turn off translation, interrupts off
switch_n_go:
	 lps   0,go_now_ps
         .using real0,r0            # tell assembler r0 is base register
realgo:
         mttbiu sp,AST_USER_BIT     # if VAX AST schedlued
         jtb   go_ast               # then jump
         bali  sp,flipmode          # flip into user mode
         mttbiu r15,INSTSTEP-16     # instruction step?
         jntb  go_now               # no...turn on user full blast
         setsb scr_irb,IRB_IRQ_0    # request level 0 interrupt
         mfs   scr_ics,r15          # copy processor level into r15
         nilz  r15,r15,0x07             # if on level 0
         jz    go_step              # then jump
go_stop:
         l     r1,low_save1         # restore old r1
         l     r15,low_save15       # restore old r15
         lps   0,low_ps             # reload old ps, interrupt before inst
go_step:
         l     r1,low_save1         # restore old r1
         l     r15,low_save15       # restore old r15
         lps   1,low_ps             # reload old ps, no interrupts for 1 inst
go_now:
         l     r1,low_save1         # restore old r1
         l     r15,low_save15       # restore old r15
         lps   0,low_ps             # reload old ps, immediate interrupts ok
go_ast:
	 cau   sp,(KERNSTACK)>>16(r0) # switch to kernal stack
	 oil   sp,sp,(KERNSTACK)&0xffff
         l     r15,low_save15       # restore old r15
         stm   r10,low_save10       # assure low_save1 and low_save10 thru 15
	 cau   r10,(VAST)>>16(r0)     # mcs_pcs value for VAX AST
	 oil   r10,r10,(VAST)&0xffff
         x     r11,r11              # no more info needed
         l     r12,low_ps+old_iar   # retieve old iar
         l     r13,low_ps+old_ics_cs# retieve old ics_cs
         lps   0,fault_ps           # short circuit interrupt, handle fault
1:       .using 1b
 #
 #  flipmode - flip the segment registers from user/kernel to kernel/user
 #      on entry:
 #          interrupts are masked
 #          translation is off
 #          sp contains the return address
         .using real0,r0
	.globl _flipmode
_flipmode:
eye_catcher(flipmode):

#ifdef FASTFLIP
         stm   r13,flipsave            # save caller's registers
	 cau   r15,(RTA_SEGREG0)>>16(r0) # i/o address of first segment register
	 oil   r15,r15,(RTA_SEGREG0)&0xffff
         lis   r14,1                   # bit mask for key bit in segment reg

#define FLIPSEG(segreg) ior r13,segreg(r15);  /* set r13 to value of segment register */ \
         x     r13,r14;                 /* flip key bit value */ \
         iow   r13,segreg(r15)              /* set segment reg to value of r13ister */
	 FLIPSEG(0)
	 FLIPSEG(1)
#if	MACH_VM
	 FLIPSEG(2)
	 FLIPSEG(3)
	 FLIPSEG(4)
	 FLIPSEG(5)
	 FLIPSEG(6)
	 FLIPSEG(7)
	 FLIPSEG(8)
	 FLIPSEG(9)
	 FLIPSEG(10)
	 FLIPSEG(11)
	 FLIPSEG(12)
#endif	MACH_VM
	 FLIPSEG(13)
	 FLIPSEG(14)

         brx   sp                      # return to caller...
         lm    r13,flipsave            # with caller's registers restored
1:	.using 1b                      # tell assembler no base register
#else

         stm   r12,flipsave            # save caller's registers
	 cau   r15,(RTA_SEGREG0)>>16(r0) # i/o address of first segment register
	 oil   r15,r15,(RTA_SEGREG0)&0xffff
	 cau   r14,(RTA_NSEGREGS)>>16(r0)# number of segment registers
	 oil   r14,r14,(RTA_NSEGREGS)&0xffff
         lis   r13,1                   # bit mask for key bit in segment reg
fliploop:
         ior   r12,0(r15)              # set r12 to value of segment register
         x     r12,r13                 # flip key bit value
         iow   r12,0(r15)              # set segment reg to value of r12ister
         sis   r14,1                   # decr loop counter
         bpx   fliploop                # loop if count not zero...
         ai    r15,r15,RTA_SEGREGSTEP  # and advance to next seg reg

         brx   sp                      # return to caller...
         lm    r12,flipsave            # with caller's registers restored
1:       .using 1b                     # tell assembler no base register
#endif FASTFLIP
 #
 #  svc level 0 interrupt service routine
 #
	.globl  svc0
         .using real0,r0
svc0:
         st    r1,low_save1      # save r1
         stm   r10,low_save10    # save r12-r15
         lis   r10,0             # indicate an svc fault
         lh    r11,svc_ps+svc_code    # get svc code
         l     r12,svc_ps+old_iar     # get interrupt time iar
         l     r13,svc_ps+old_ics_cs  # get interrupt time ics and cs
         bali  sp,flipmode       # switch to kernel mode
	 cau   sp,(KERNSTACK)>>16(r0) # switch to kernal stack
	 oil   sp,sp,(KERNSTACK)&0xffff
         lps   0,fault_ps        # goto fault: translation on, interrupts off
1:       .using 1b

 #  fault handler, called from level 0 interrupt service routines
 #
 #  on entry:
 #     r1 = sp
 #     r10 = mcs_pcs (check) or zero (svc)
 #     r11 = exception information (check) or svc number (svc)
 #     r12 = interrupt old_iar
 #     r13 = interrupt old_ics_cs
 #     r14 = unused
 #     r15 = unused
 #     low_save1,10,11,12,13,14,15 are values at time of interrupt
 #
 #  function:
 #     save complete context on the stack
 #     call trap
 #     if called from user mode
 #       reschedule
 #     else
 #       restore complete context and return
 #
 #  parameters passed to trap (stack offsets):
 #
 
.set FAULT_MCS_PCS , 0x00  # value in r2
.set FAULT_INFO    , 0x04  # value in r3
.set FAULT_ICS_CS  , 0x08  # value in r4
.set FAULT_R0      , 0x0C
.set FAULT_R1      , 1*4 + FAULT_R0
.set FAULT_R2      , 2*4 + FAULT_R0
.set FAULT_R10     ,10*4 + FAULT_R0
.set FAULT_R15     ,15*4 + FAULT_R0
.set FAULT_IAR     , 1*4 + FAULT_R15
.set FAULT_MQ      , 2*4 + FAULT_R15

.set FAULTSAVE     , 4 + FAULT_MQ # size of stack area needed for parameters to trap
	.globl	_fault
_fault:
	.globl	fault
fault:
         ai    sp,sp,-FAULTSAVE  # make room for context
faultstm:stm   r0,FAULT_R0(sp)   # save all registers (some incorrect)
         st    r12,FAULT_IAR(sp) # save iar at time of interrupt
         cas   r2,r10,r0         # 1st parm to trap
         cas   r3,r11,r0         # 2nd parm to trap
         cas   r4,r13,r0         # 3rd parm to trap
         sts   r2,0(sp)          # 1st parm to trap
         sts   r3,4(sp)          # 2nd parm to trap
         sts   r4,8(sp)          # 3rd parm to trap
 
         shr   r13,16            # shift ics to low order bits
         nilz  r13,r13,7             # use only level bits
         mfs   scr_ics,r15       # get current ics
         nilo  r15,r15,0xfff8        # zero only level bits
         o     r15,r13           # set level at time of fault
         mts   scr_ics,r15       # change processor level
 
         mfs   scr_mq,r15        # get current multiplier/quotient
         st    r15,FAULT_MQ(sp)  # save context
         mfs   scr_iar,r6        # get addressability to low storage
1:	 .using 1b,r6            # tell the assembler (C preserves R6)
         l     r15,low_save1     # value of r1 at time of interrupt
         sts   r15,FAULT_R1(sp)  # correct value on stack
         lm    r10,low_save10    # values of r10-r15 at time of interrupt
         stm   r10,FAULT_R10(sp) # correct values on stack
 
#if !defined(LORDB)
         mfs   scr_irb,r15       # get copy of irb
         mttbil r15,IRB_IRQ_0    # get int level 0 bit
         jntb  fault_call_c      # jump if no instruction step pending
         clrsb scr_irb,IRB_IRQ_0 # cancel level 0 interrupt request
         oiu   r4,r4,ICSCS_INSTSTEP/UPPER  # repeat inst step after fault
#endif
 #
 #  call trap() to handle the fault
 #
fault_call_c:
         clrsb scr_ics,INTMASK-16# enable interrupts now
         l     r15,__trap	 # point to trap handler routine
         balrx r15,r15           # call trap
         st    r2,FAULT_MCS_PCS(sp)  # save on stack for SVC check after trap
 #
 #  restore complete context and return
 #
         setsb scr_ics,INTMASK-16# disable interrupts
         l     r14,FAULT_IAR(sp) # get old_iar (MAY BE CHANGED)
         ls    r15,FAULT_ICS_CS(sp)  # get old_ics_cs (MAY BE CHANGED)
         stm   r14,low_ps        # save in low core
         l     r15,FAULT_MQ(sp)  # get old multiplier/quotient (UNCHANGED)
         mts   scr_mq,r15        # restore multiplier/quotient
         ls    r15,FAULT_R1(sp)  # get old r1 value (UNCHANGED)
         st    r15,low_save1     # save in low core for go
         l     r15,FAULT_R15(sp) # get old r15 value (UNCHANGED)
         st    r15,low_save15    # save in low core for go
         ls    r0,FAULT_R0(sp)   # restore 0 (UNCHANGED)
         lm    r2,FAULT_R2(sp)   # restore 2 through 15 (UNCHANGED)
         ls    r15,FAULT_MCS_PCS(sp)  # get MCS and PCS causing fault
         cli   r15,0             # if it was an SVC
         je    svc_complete      # then jump
         mfs   scr_iar,r15       # get addressability again
1:	 .using 1b,r15           # tell assembler
         lps   0,go_ps           # jump to go with interrupts off
svc_complete:
         mfs   scr_iar,r15       # get addressability again
1:	 .using 1b,r15            # tell assembler
         lps   0,go_ps           # jump to "go" with interrupts off
1:       .using 1b               # tell assembler no base register

 #  program check that is really a page fault
 #
 #  on entry:
 #     r10 = mcs_pcs
 #     r11 = exception information
 #     interrupt old_iar
 #     interrupt old_ics_cs
 #     low_save1,10,11,12,13,14,15 are values at time of interrupt
 
         .using real0,r0
	 .globl pck_fault
eye_catcher(pck_fault):
         l     r12,pck_ps+old_iar     # get interrupt time iar
         l     r13,pck_ps+old_ics_cs  # get interrupt time ics and cs
         mttbiu r13,PROBSTATE-16 # copy problem state bit from old ics
         jtb   user_fault        # jump if we were in problem state

#if	MACH_VM
/* 
 * 	The MACH_VM code causes faults to occur MUCH earlier in the
 *	boot sequence than normal unix vm.  In particular, we take
 *	faults off of the tmp_stack BEFORE a u-area has been created.
 *	Thus, we must give up the error checking doen here to make
 *	sure that the kernel stack is something valid, and just hope
 *	that we never really trash the kernel stack.  As it is, it 
 *	will prgram check in program check state and thus check-stop.
 *
 *	Bill Bolosky, 4/10/86
 */
	j	goto_fault
#else	MACH_VM
	 cau   r15,(UAREA)>>16(r0) # address of 1st byte of kernel stack
	 oil   r15,r15,(UAREA)&0xffff
         cl    sp,r15            # if kernel stack invalid
         bl    pck_real          # then jump
	 cau   r15,(KERNSTACK)>>16(r0) # address of last byte of kernel stack
	 oil   r15,r15,(KERNSTACK)&0xffff
         cl    sp,r15            # if kernel stack invalid
         bh    pck_real          # then jump
         cal16 r15,faultstm      # this inst catches bad kernel segments
         cl    r12,r15           # if fault anywhere else
         jne   goto_fault        # then jump
         ai    sp,sp,FAULTSAVE      # undo subi before faultstm
         b     pck_real          # deep trouble
#endif	MACH_VM
user_fault:
         bali  sp,flipmode       # switch to kernel mode
	 cau   sp,(KERNSTACK)>>16(r0) # switch to kernel stack
	 oil   sp,sp,(KERNSTACK)&0xffff
goto_fault:
         lps   0,fault_ps        # goto fault: translation on, interrupts off
1:       .using 1b               # tell assembler no base register


#ifdef ROSETTA_0
#include "../ca/lopckrt0.s"
#endif
 
#include "../ca/lointr.s"
#if	MACH_VM
/*#include "../ca/lovm.s"*/
#else	MACH_VM
#include "../ca/loswap.s"
#endif	MACH_VM
#include "../ca/loutil.s"


	.ltorg
 
	.globl _endlocore
_endlocore:                             # last of locore

#if defined(SBMODEL) && defined(RDB)
	.globl	_symsize

/* These BUMPLC's are to get around the PHASE ERRORs that the assembler
 * generates because of the buggy way it handles .align's.  They can be
 * removed once the assembler has been fixed.  If code changes in locore
 * and new phase errors are introduced, add or remove BUMPLC's as needed.
 */
#define	BUMPLC	.short 0; .align 1;
	BUMPLC
	BUMPLC
	BUMPLC
	BUMPLC
	BUMPLC
	BUMPLC
	BUMPLC
	BUMPLC
	BUMPLC
	BUMPLC
	.set	rdb_start,0xf800
	.org	real0+rdb_start-6		# to start of debugger area
	.align	3
1: .set _symsize,	1b - _endlocore		# size of symbol table
_start:
rdb:
        .using real0,r0          # tell the assembler to use r0 as base
	cal16	r15,start	# use 'cal16' to ignore segment number
	balr	r15,r15		# transfer control to real 'start'
 # This value must be bigger than the _end of rdb.out
	.set rdb_end, 0x1f800	#compare with _end in rdb.out
	.org	real0+rdb_end		# to end of debugger area
#endif

