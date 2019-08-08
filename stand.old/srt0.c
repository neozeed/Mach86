/*	srt0.c	6.1	83/07/29	*/

#include "../vax/mtpr.h"
#define	LOCORE
#include "../vax/cpu.h"

/*
 **********************************************************************
 * HISTORY
 * 01-Jul-85  Mike Accetta (mja) at Carnegie-Mellon University
 *	CS_VAXM:  added support for MicroVAX-I and -II (a la ULTRIX-
 *	32).
 *
 **********************************************************************
 *
 * Startup code for standalone system
 * Non-relocating version -- for programs which are loaded by boot
 */

	.globl	_end
	.globl	_edata
	.globl	_main
	.globl	__rtt
	.globl	_configure
	.globl	_cpu
	.globl	_openfirst

	.set	HIGH,31		# mask for total disable

entry:	.globl	entry
#ifdef	CMU
	/* CS_BOOT */
	nop; nop
	mtpr	$HIGH,$IPL		# just in case
	/* CS_BOOT */
	/* CS_VAXM */
#ifdef	uVPROM
	/*
	 *  When started via the MicroVax Boot Prom, this is the first executed
	 *  code.  It must take advantage of the boot parameter block passed in
	 *  by the Prom to size memory before going much further (and
	 *  destroying the parameter block).
	 */
	movl	$9,r10			# only RQDXn for now (ra major #)
	insv	0x64(r11),$8,$8,r10	# save boot unit in next byte

	movl	0x48(r11),r1		# page frame map start
	movl	0x44(r11),r0		# page frame map length
	clrl	r2			# clear page frame count

1:	cmpb	(r1)+,$0xff		# check next map entry
	bneq	2f			# if not 8 frames (bits) then done
	addl2	$8,r2			#  otherise show 8 more page frames
	sobgtr	r0,1b			#  and check next set

2:	ashl	$9,r2,*$0xf0000		# convert page count to bytes
					#  There must be a better place to
					#  store this!

	movl	0x30(r11),r11		# put boot flags in R11 for UNIX
#endif	uVPROM
	/* CS_VAXM */
#else	CMU
	.word	0x0
	mtpr	$HIGH,$IPL		# just in case
#endif	CMU
#ifdef REL
	movl	$RELOC,sp
#else
	movl	$RELOC-0x2400,sp
#endif
start:
#ifdef	CMU
	/* CS_BOOT */
	movab	_edata,r0
	/* CS_BOOT */
#else	CMU
	movl	aedata,r0
#endif	CMU
clr:
	clrl	(r0)+
	cmpl	r0,sp
	jlss	clr
#ifdef REL
#ifdef	CMU
	/* CS_BOOT */
	movab	_edata,r0
	movab	entry,r1
	subl2	r1,r0
	movc3	r0,entry,(sp)
	/* CS_BOOT */
#else	CMU
	movc3	aedata,*$0,(sp)
#endif	CMU
/*
 * Reclear bss segment separately from text and data
 * since movc3 can't move more than 64K bytes
 */
dclr:
	clrl	(r3)+
	cmpl	r3,$_end
	jlss	dclr
#ifdef	CMU
/* CS_VAXM */
/*
 *  I don't believe this.  Since we don't use RA81's here, we'll try doing
 *  without it and see what breaks (at least we can put a more informative
 *  comment here if it does turn out to be true). -mja 7/1/85
 *
 *  N.B.  The code if included as it stands may need to be changed for the
 *  micro-VAX to avoid trampling on the PFN stuff on 1Mb systems.
 */
/* CS_VAXM */
#else	CMU
/* this loop shouldn't be necessary, but is when booting from an ra81 */
xclr:	
	clrl	(r3)+
	cmpl	r3,$0x100000
	jlss	xclr
#endif	CMU
	jmp	*abegin
begin:
#endif
	mtpr	$0,$SCBB
	calls	$0,_configure
	movl	$1,_openfirst
	calls	$0,_main
#ifndef TP
	jmp	start
#else
	ret
#endif

	.data
#ifdef REL
abegin:	.long	begin
aedata:	.long	_edata-RELOC
#else
aedata:	.long	_edata
#endif

__rtt:
	.word	0x0
	jmp	start

	.globl	_badloc
_badloc:
	.word	0
	movl	$1,r0
	movl	4(ap),r3
	movl	$4,r2
	movab	9f,(r2)
#ifdef	CMU
	/* CS_VAXM */
	bbc	$0,8(ap),1f; tstb (r3)
1:	bbc	$1,8(ap),1f; tstw (r3)
1:	bbc	$2,8(ap),1f; tstl (r3)
	/* CS_VAXM */
#else	CMU
	tstl	(r3)
#endif	CMU
1:	clrl	r0			# made it w/o machine checks
2:	movl	$4,r2
	clrl	(r2)
	ret
	.align	2
9:
	casel	_cpu,$1,$VAX_MAX
0:
	.word	8f-0b		# 1 is 780
	.word	5f-0b		# 2 is 750
	.word	5f-0b		# 3 is 730
#ifdef	CMU
	/* CS_VAXM */
	.word	5f-0b		# 4 is ?
	.word	5f-0b		# 5 is ?
	.word	5f-0b		# 6 is ?
	.word	5f-0b		# 7 is MicroVAX-I
	.word	5f-0b		# 8 is MicroVAX-II
	/* CS_VAXM */
#endif	CMU
5:
	mtpr	$0xf,$MCESR
	brb	1f
8:
	mtpr	$0,$SBIFS
1:
	addl2	(sp)+,sp		# discard mchchk trash
	movab	2b,(sp)
	rei
