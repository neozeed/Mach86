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
/* $Header: rosetta.h,v 4.0 85/07/15 00:47:44 ibmacis GAMMA $ */
/* $Source: /ibm/acis/usr/sys_ca/ca/RCS/rosetta.h,v $ */

#ifdef	CMU
/***********************************************************************
 * HISTORY
 * 31-Mar-86  Bill Bolosky (bolosky) at Carnegie-Mellon University
 *	Added definition of RTA_NSEGS (number of segments).
 *
 ***********************************************************************
 */
#endif	CMU

#if !defined(lint) && !defined(LOCORE) && defined(RCS_HDRS)
static char *rcsidrosetta = "$Header: rosetta.h,v 4.0 85/07/15 00:47:44 ibmacis GAMMA $";
#endif

#ifdef VTL_RELOCATE  /* VTL version of relocate hardware */

#define RTA_NSEGREGS 16
#define RTA_NTLBS 64
#define RTA_SEGREG0 0xF80000
#define RTA_SEGREGSTEP 0x2000
#define RTA_LOG2SEGREGSTEP 13
#define RTA_TLBAW1 0xE80000
#define RTA_TLBAW2 0xEA0000
#define RTA_TLBBW1 0xEC0000
#define RTA_TLBBW2 0xEE0000
#define RTA_TLBSTEP 0x800
#define RTA_REFBITS 0xF00000
#define RTA_MODBITS 0xF40000
#define RTA_REFMOD_STEP 4
#define RTA_TID_ADDR 0xFA0000

#define RTA_EXCEPTION 0xFF0000
#define RTA_EX_FAULT  0x40  /* no tlb match */
/*efine               0x20  /* both tlb's matched */
#define RTA_EX_KEYS   0x10  /* invalid keys */
/*efine               0x08  /* lockbit check */
/*efine               0x04  /* tid mismatch */
/*efine               0x02  /* pio in problem state (huh) */
/*efine               0x01  /* read/write (-/+) */
#define RTA_EX_OHOH   0x2E  /* all unexpected exceptions */

#define RTA_UNDEF_PTR 0xFFFF  /* undefined pointer value for debugging */

#define get_mod_bit(rpage) (ior(0xF40000+((rpage)<<2))&1)
#define set_mod_bit(rpage,v) (iow(0xF40000+((rpage)<<2),(v)))

#define get_ref_bit(rpage) (ior(0xF00000+((rpage)<<2))&1)
#define set_ref_bit(rpage,v) (iow(0xF00000+((rpage)<<2),(v)))

#define set_refmod_bits(rpage,ref,mod) (set_ref_bit(rpage,ref),\
set_mod_bit(rpage,mod))

#define get_hatptr(ipte) ((ipte)->hat_ptr)
#define set_hatptr(ipte,v) ((ipte)->hat_ptr = v)

#define get_iptptr(ipte) ((ipte)->ipt_ptr)
#define set_iptptr(ipte,v) ((ipte)->ipt_ptr = v)

#define RTA_SID_SYSTEM 0x800
#define RTA_SID_UNUSED 0x801
#define RTA_TLBUNUSED  0x801000FF  /* sid=unused, tid=0xFF */

#define make407sid(p) (p->p_ndx)
#define make410sid(xp) (0x7FF-(xp-text))
#define makeUsid(p) (~p->p_ndx&0xFFF)
#define makeP0sid(p) (~p->p_sid1&0xFFF)

#define get_segreg(reg) (ior(0xF80000+((reg)<<13))>>2)
#define set_segreg(reg,sid) (iow(0xF80000+((reg)<<13),(sid)<<2))
#endif

#ifdef ROSETTA_0
/* early Rosetta relocate hardware -- known bugs are:
 *
 * - the (17-bit) segment register must have the following bits set:
 *   XXX 00XXXXXXXXXX XX
 *
 * - the HAT and IPT (16-bit) pointer fields in the inverted page table entries
 *   must have the following bits set: X 00 10XXXXXXXXXXX
*
 * all register address must be specified w/o parentheses for locore
 */
#define ROSEBASE 0x810000
#define ROSE_SEGR 0
#define ROSE_IOBR 0x10
#define ROSE_SER  0x11
#define ROSE_SEAR 0x12
#define ROSE_TRAR 0x13
#define ROSE_TIDR 0x14
#define ROSE_TCR  0x15
#define ROSE_RAM  0x16
#define ROSE_TLBS 0x80
#define ROSE_CRA  0x83

#define TCR_S	  0x100

#define RTA_NSEGREGS 16
#define RTA_SEGREG0  0x810000
#define RTA_SEGREGSTEP 1
#define	RTA_NSEGS 4096

#define RTA_IOBASE   0x810010
#define RTA_EXC_ADDR 0x810012
#define RTA_REALADDR 0x810013
#define RTA_TID_ADDR 0x810014
#define RTA_INV_TLB  0x810080
#define RTA_INV_SEG  0x810081
#define RTA_INV_ADDR 0x810082
#define RTA_VTOP     0x810083

#ifdef LOCORE

#endif

/* These TLB address should not be used.  Rather the "translate assist"
 * functions of the ROSETTA hardware should be used instead.
 */
#define RTA_NTLBS 16
#define RTA_TLBAW1   0x810040
#define RTA_TLBBW1   0x810050
#define RTA_TLBSTEP 1
#define RTA_TLBUNUSED 0  /* valid bit is zero */

#define RTA_REFBITS  0x811000
#define RTA_MODBITS  0x811000
#define RTA_REFMOD_STEP 1

#define RTA_CONTROL 0x810015
/*efine             0x8000  /* virtual = real for segment register zero */
/*efine             0x4000  /* interrupt on successful parity error retry */
/*efine             0x2000  /* enable RAS diagnostic mode */
#define RTA_IPTLOOP 0x1000  /* terminate IPT search after 128 links */
/*efine             0x0800  /* interrupt on correctable ECC error */
/*efine             0x0400  /* interrupt on successful TLB reload */
/*efine             0x0200  /* ref/mod array has parity */
/*efine             0x0100  /* 4K page size (bit on), 2K page size (bit off) */
#define RTA_HATBASE 0x00FF  /* HATIPT base address */

#define RTA_EXCEPTION 0x810011
/*efine               0x20000  /* segment protection (P,R,I bits in seg reg) */
/*efine               0x10000  /* RSC ACKD */
/*efine               0x08000  /* RSC NAKD */
/*efine               0x04000  /* invalid storage address (ROS/RAM spec reg)*/
/*efine               0x02000  /* invalid i/o address (Rosetta's 64K block) */
#define RTA_EX_LOAD   0x01000  /* load access (on), store access (off) */
/*efine               0x00800  /* uncorrectable storage error */
#define RTA_EX_CECC   0x00400  /* correctable ECC error */
/*efine               0x00200  /* successful TLB reload */
/*efine               0x00100  /* ref/mod array parity error */
/*efine               0x00080  /* write to ROS */
/*efine               0x00040  /* IPT specification (128 links or end chain?) */
/*efine               0x00020  /* external device error */
#define RTA_EX_MULTX  0x00010  /* multiple exception */
#define RTA_EX_FAULT  0x00008  /* page fault */
#define RTA_EX_HACK   0x00004  /* TLB specification (both TLB's matched) */
#define RTA_EX_KEYS   0x00002  /* storage protection (S,K bits in seg reg,
				  and key bits in tlb) */
/*efine               0x00001  /* data lock (S in seg reg, tid reg,
				  and tid,write,lock bits in tlb) */
#define RTA_EX_OHOH   0x3EBE5  /* all unexpected exceptions */
#define RTA_EX_OHOH_UPPER 0x3
#define RTA_EX_OHOH_LOWER 0xEBE5

#define RTA_HATIPT_PTRBUG 0x1000  /* OR mask for hat or ipt pointers */
#define RTA_HATIPT_PTRUNBUG 0xEFFF  /* AND mask for hat or ipt pointers */
#define RTA_UNDEF_PTR 0x97FF  /* undefined pointer value for debugging */

#define get_mod_bit(rpage) (ior(ROSEBASE+0x1000+(rpage))&1)
#define set_mod_bit(rpage,v) (iow(ROSEBASE+0x1000+(rpage),\
(ior(ROSEBASE+0x1000+(rpage))&2)|(v)))

#define get_ref_bit(rpage) ((ior(ROSEBASE+0x1000+(rpage))>>1)&1)
#define set_ref_bit(rpage,v) (iow(ROSEBASE+0x1000+(rpage),\
(ior(ROSEBASE+0x1000+(rpage))&1)|((v)<<1)))

#define set_refmod_bits(rpage,ref,mod) iow(ROSEBASE+0x1000+(rpage),\
((ref)<<1)|(mod))

#define get_hatptr(ipte) ((ipte)->hat_ptr & 0x87FF)
#define set_hatptr(ipte,v) ((ipte)->hat_ptr = (v)|0x1000)

#define get_iptptr(ipte) ((ipte)->ipt_ptr & 0x87FF)
#define set_iptptr(ipte,v) ((ipte)->ipt_ptr = (v)|0x1000)

#define RTA_SID_SYSTEM 0x200
#define RTA_SID_UNUSED 0x201
#define RTA_SEG_PRESENT 0x10000

#define make407sid(p) (p->p_ndx)
#define make410sid(xp) (0x1FF-(xp-text))
#define makeUsid(p) (~p->p_ndx&0x3FF)
#define makeP0sid(p) (~p->p_sid1&0x3FF)

#define get_segreg(reg) ((ior(ROSEBASE+reg)>>2)&0x3FF)
#define set_segreg(reg,sid) (iow(ROSEBASE+reg,((sid)<<2)|0x10000))
#endif

#ifdef ROSETTA_1
/* later Rosetta relocate hardware */
#endif


/***** Universal Truths *****/

#define RTA_VPAGE_BITS 17
#define RTA_VPAGE_MASK ((1<<RTA_VPAGE_BITS)-1)
#define RTA_SID_MASK (0xFFF<<RTA_VPAGE_BITS)

#define RTA_ADDRTAG_MASK 0x1FFFFFFF
#define RTA_SEGSHIFT 24

/* storage protection variables */

#define RTA_KEY_SHIFT 30
#define RTA_KEY_BITS 0xC0000000
#define RTA_KEY_KW   0  /* kernel write, user none */
#define RTA_KEY_URKW 1  /* kernel write, user read */
#define RTA_KEY_UW   2  /* kernel write, user write */
#define RTA_KEY_URKR 3  /* kernel read, user read */
/*      RTA_KEY_KR      /* kernel read, user none */
/*      RTA_KEY_NOACC   /* kernel none, user none */

#define RTA_KEYBIT(k) (1<<((unsigned)(k)>>30))  /* k = hatipt.key_addrtag */
#define RTA_KEY_OK(k) (1<<(k))  /* k = RTA_KEY_KW,URKW,UW,URKR */

#define UWRITE_OK (1<<RTA_KEY_UW)
#define UREAD_OK ((1<<RTA_KEY_UW)|(1<<RTA_KEY_URKW)|(1<<RTA_KEY_URKR))
#define KWRITE_OK (1<<RTA_KEY_KW)
#define KREAD_OK ((1<<RTA_KEY_KW)|(1<<RTA_KEY_UW)|\
(1<<RTA_KEY_URKW)|(1<<RTA_KEY_URKR))

#ifndef LOCORE

struct hatipt_entry  /* more or less */
  {
    long key_addrtag;
    unsigned short hat_ptr,ipt_ptr;
    unsigned char w,tid;
    unsigned short lockbits;
    long reserved;
  };

struct ipt_entry  /* all fields broken out */
  {
    unsigned ipt_key   :2;
    unsigned ipt_r     :1;
    unsigned ipt_sid   :12;
    unsigned ipt_vpage :17;
    unsigned ipt_hat_e :1;
    unsigned           :2;
    unsigned ipt_hatptr:13;
    unsigned ipt_ipt_l :1;
    unsigned           :2;
    unsigned ipt_iptptr:13;
    unsigned           :7;
    unsigned ipt_w     :1;
    unsigned ipt_tid   :8;
    unsigned ipt_locks :16;
    unsigned           :32;
  };

#define RTA_ENDCHAIN(ptr) ((ptr)&0x8000)

extern struct hatipt_entry *RTA_HATIPT;
extern int RTA_HASHMASK;
#define RTA_HASH(seg,vpage) ((seg ^ vpage) & RTA_HASHMASK)

#endif
#define VR0_BIT         (1<<(31-16))
#define GET_ROS_TCR     (ior(ROSEBASE+ROSE_TCR))
#define SET_ROS_TCR(v)  (iow(ROSEBASE+ROSE_TCR, v))
#define SET_VR0         (SET_ROS_TCR (GET_ROS_TCR | VR0_BIT))
#define CLR_VR0         (SET_ROS_TCR (GET_ROS_TCR & ~VR0_BIT))
#define GET_VR0(x)	(SET_ROS_TCR ((x = GET_ROS_TCR) | VR0_BIT))
#define SET_VR(x)	(SET_ROS_TCR (x))
