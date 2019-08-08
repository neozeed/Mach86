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
 # 1 "blt.o"




	.data
rcsid:	.asciz	"$Header: blt.s,v 4.0 85/07/15 00:40:10 ibmacis GAMMA $"
	.text

 #	Block transfer subroutine supporting overlapping moves.
 #	dest = blt(destination, source, length)
 #		       r2	   r3	   r4
 #	The structure operations performed by the ROMP PCC use this routine,
 #	and provide no stack space for the parameters passed in r2, r3, and r4.
 #	The destination address must be returned for structure assigns.
 #
 #	An alternate entry point is provided for the equivalent function:
 #
 #		bcopy(source, destination, length)
	.text

	 .globl _bcopy
	 .globl _blkcpy
	 .globl _ovbcopy
_blkcpy:
_ovbcopy:
_bcopy:
	 x	r2,r3			#exchange values in r2 and r3
	 x	r3,r2			#exchange values in r2 and r3
	 x	r2,r3			#exchange values in r2 and r3
					#and fall into blt
	.globl	_blt
_blt:
bblt:
	#set up work registers

	ai	r0,r4,0			#count (# of bytes to copy)
	bzx	leturn			#count is 0 -- return immediately
	cas	r5,r2,r0		#save destination address for return
 #
 #			Following are two similar pieces of code --
 #			one working l-to-r through the source, the other
 #			working r-to-l.	 Overlap causes no problems, since
 #			the move's always in the safe direction.
 #
	c	r2,r3			#move to high addr from low addr?
	bh	rtol			#yes, use r-to-l version

	#copy single bytes until the source address is full-word aligned

lsfwa:
	cas	r4,r3,r0		#current source address
	nilz	r4,r4,3			#isolate low order two bits
	jz	lcfwcy			#jump if source address on full word
	lcs	r4,0(r3)		#fetch one byte from the source
	inc	r3,1			#incr source address by one byte
	stcs	r4,0(r2)		#store one byte at the destination
	sis	r0,1			#decr total count by one byte
	bpx	lsfwa			#loop if count > 0
	inc	r2,1			#incr destination address by one byte

	#return to the calling routine

leturn:
	brx	r15			#return to calling routine
	cal	r0,0(r5)		#always return destination address

	#check the word alignment of the destination address

lcfwcy:
	cas	r4,r2,r0		#destination address
	nilz	r4,r4,3			#isolate low order two bits
	jnz	lchwcy			#jump if not full-word aligned

	#the source and destination addresses are on full word boundaries

	ai	r0,r0,-48		#uncopied byte count - 48
	jl	lcwds			#jump if < 48 bytes uncopied (12 words)

	#save more of the caller's registers on the stack
 # the global label is here so that the stack traceback works properly
	.globl	_bcopyblt
_bcopyblt:

	ai	sp,sp,-44		#push 11 words of junk onto the stack
	stm	r5,0(sp)		#save caller's r5 thru r15 on the stack

	#copy blocks of 12 words
lm12ws: lm	r4,0(r3)		#r4-r15 = next 12 words of source
	ai	r3,r3,48		#incr source address by 12 words
	stm	r4,0(r2)		#next 12 words of destination = r4-r15
	ai	r0,r0,-48		#decr total uncopied byte count
	bnmx	lm12ws			#loop if another 12 words remain uncopyd
	ai	r2,r2,48		#incr destination address by 12 words

	#restore most of the caller's registers from the stack

	lm	r5,0(sp)		#restore caller's r5-r15 from the stack
	ai	sp,sp,44		#pop 11 words off the stack

	#check the residual byte count

lcwds:	ai	r0,r0,44		#uncopied byte count - 4
	jl	lcbyts			#jump if < 4 bytes uncopied

	#copy single words

lmword: ls	r4,0(r3)		#r4 = next source word
	inc	r3,4			#incr source address by one word
	sts	r4,0(r2)		#next destination word = r4
	sis	r0,4			#decr total uncopied byte count
	bpx	lmword			#loop if another word remains uncopied
	inc	r2,4			#incr destination address by one word

	#check the residual byte count

lcbyts: ais	r0,4			#uncopied byte count
	jz	leturn			#jump if no bytes uncopied

	#copy single bytes

lmbyte: lcs	r4,0(r3)		#fetch one byte from the source
	inc	r3,1			#incr source address by one byte
	stcs	r4,0(r2)		#store one byte at the destination
	sis	r0,1			#decr total uncopied byte count
	bpx	lmbyte			#loop if count > 0
	inc	r2,1			#incr destination address by one byte
	j	leturn			#refuse to do nibbles

	#the destination address is not on a full word boundary

lchwcy: sis	r0,4			#uncopied byte count - 4
	jl	lcbyts			#jump if < 4 bytes uncopied
	cas	r4,r2,r0		#destination address
	nilz	r4,r4,1			#isolate 2**0 bit
	jnz	lmbbbb			#jump if not half word aligned

	#the destination address is on a half word boundary

lmhwd:	ls	r4,0(r3)		#r4 = next source word
	inc	r3,4			#incr source address by one word
	sths	r4,2(r2)		#copy 3rd and 4th bytes of word to dest
	sri16	r4,0			#shift 1st and 2nd bytes to 3rd and 4th
	sths	r4,0(r2)		#copy 1st and 2nd bytes of word to dest
	sis	r0,4			#decr total uncopied byte count
	bpx	lmhwd			#loop if count > 0
	inc	r2,4			#incr destination address by one word
	j	lcbyts			#copy odd bytes

	#the destination address is not on a full or half word boundary

lmbbbb: ls	r4,0(r3)		#r4 = next source word
	inc	r3,4			#incr source address by one word
	stcs	r4,3(r2)		#copy 4th byte of word to destination
	mc32	r4,r4			#move 3rd byte to to 4th byte
	stcs	r4,2(r2)		#copy 3rd byte of word to destination
	mc31	r4,r4			#move 2nd byte to to 4th byte
	stcs	r4,1(r2)		#copy 2nd byte of word to destination
	mc30	r4,r4			#move 1st byte to to 4th byte
	stcs	r4,0(r2)		#copy 1st byte of word to destination
	sis	r0,4			#decr total uncopied byte count
	bpx	lmbbbb			#loop if count > 0
	inc	r2,4			#incr destination address by one word
	j	lcbyts			#copy odd bytes

 #		Now for similar code, but moving right to left.
 #
 #*	.cnop	0,4
rtol:	cas	r3,r3,r4		#point r3 to last word of source,
	dec	r3,4
	cas	r2,r2,r4		#and r2 past last byte of sink.

	#copy single bytes until the source address is full-word aligned

rsfwa:	nilz	r4,r3,3			#low two source addr bits zero?
	jz	rcfwcy			#jump if source address on full word
	lcs	r4,3(r3)		#fetch one byte from the source
	dec	r3,1			#decr source address by one byte
	dec	r2,1			#decr destination address by one byte
	sis	r0,1			#decr total count by one byte
	bpx	rsfwa			#loop if count > 0
	stcs	r4,0(r2)		#store one byte at the destination

	#Return to the calling routine.  Note that r2 has naturally landed
	#at the start of the destination.

rret:
	brx	r15			#return to calling routine
	cas	r0,r2,r0		#always return destination address

	#check the word alignment of the destination address

rcfwcy:	nilz	r4,r2,3			#low two destination bits zero?
	jnz	rmob			#jump if not full-word aligned

	#the source and destination addresses are on full word boundaries

	ai	r0,r0,-48		#uncopied byte count - 48
	jl	rcwds			#jump if < 48 bytes uncopied (12 words)

	.globl	_bltbcopy
_bltbcopy:
	#save most of the caller's registers on the stack

	ai	sp,sp,-40		#push 11 words of junk onto the stack
	stm	r6,0(sp)		#save caller's r6 thru r15 on the stack

	#copy blocks of 12 words
rm12ws: ai	r3,r3,-48		#decr source address by 12 words
	lm	r4,4(r3)		#r4-r15 = next 12 words of source
	ai	r2,r2,-48		#decr destination address by 12 words
	ai	r0,r0,-48		#decr total uncopied byte count
	bnmx	rm12ws			#loop if another 12 words remain uncopyd
	stm	r4,0(r2)		#next 12 words of destination = r4-r15

	#restore most of the caller's registers from the stack

	lm	r6,0(sp)		#restore caller's r5-r15 from the stack
	ai	sp,sp,40		#pop 11 words off the stack

	#check the residual byte count

rcwds:	ai	r0,r0,44		#uncopied byte count - 4
	jl	rcbyts			#jump if < 4 bytes uncopied

	#Copy single words

rmword: ls	r4,0(r3)		#r4 = next source word
	dec	r3,4			#decr source address by one word
	dec	r2,4			#decr destination address by one word
	sis	r0,4			#decr total uncopied byte count
	bnmx	rmword			#loop if another word remains uncopied
	sts	r4,0(r2)		#next destination word = r4

	#check the residual byte count

rcbyts: ais	r0,4			#uncopied byte count
	jz	rret			#jump if no bytes uncopied

			#copy 1, 2, or 3 bytes from final word

	ls	r4,0(r3)		#all remaining bytes come from here
	sis	r0,2			#2 or 3 bytes?
	jm	rm1b
	dec	r2,2			#yes, store together.
	cas	r0,r2,r0
	bzrx	r15			#Return if exactly 2
	sths	r4,0(r2)
	sri16	r4,0
rm1b:	cal	r0,-1(r2)		#Address of string for return
	brx	r15
	stc	r4,-1(r2)		#and for storing final byte

	#The destination address is not on a full word boundary

rmob:
	ai	sp,sp,-40		#may be wasteful if short string
	stm	r6,0(sp)
	sfi	r5,r4,4			#r5 = no. of bytes to next word bdy
 #			Make rightmost store look word-aligned by bumping
 #			count to cover following innocent bytes, then
 #			loading and eventually re-storing them.
 #			Move long strings 6 words at a time.
	a	r0,r5			#bump count
	a	r2,r5			#and sink address correspondingly
	sli	r4,3			#shift source right r4 bytes or left r5
	sli	r5,3			#bytes to align with sink words.
	l	r7,-4(r2)		#1 to 3 innocent bytestanders
	sl	r7,r4
	ai	r0,r0,-24
	bmx	rmwob
	sr	r7,r4

 #			Move off-boundary words, 6 at a time or 1 at a time.
 #			r4 = right shift count
 #			r5 = left shift count
 #			r6 = for next pass, partial word not stored by this pass
 #			r7 = for this pass, partial word not stored by last pass
 #			r8 = partial-word scratch register
 #			r9   unused
 #		   r10-r15 = lm/stm data
 #
 # Move multiple off-boundary words.  Comment field shows example with sink
 # ending one byte past a word, for which  r4=8 and r5=24;
 # r7 holds the three innocent bytes that follow the sink -- say, _OLD.
 #
 #			     r6    r7    r8    r10   r11   r12   r13   r14   r15
rmmob:	lm	r10,-20(r3)#      0OLD	      abcd  efgh  ijkl  mnop  qrst  uvwx
	cas	r6,r10,r0  #abcd
	sl	r10,r5	   #		      d000
	cas	r8,r11,r0  #            efgh
	sr	r8,r4	   #		0efg
	o	r10,r8	   #		      defg
	sl	r11,r5	   #			    h000
	cas	r8,r12,r0  #		ijkl
	sr	r8,r4	   #		0ijk
	o	r11,r8	   #			    hijk
	sl	r12,r5	   #				  l000
	cas	r8,r13,r0  #		mnop
	sr	r8,r4	   #		0mno
	o	r12,r8	   #				  lmno
	sl	r13,r5	   #					p000
	cas	r8,r14,r0  #		qrst
	sr	r8,r4	   #		0qrs
	o	r13,r8	   #					pqrs
	sl	r14,r5	   #					      t000
	cas	r8,r15,r0  #		uvwx
	sr	r8,r4	   #		0uvw
	o	r14,r8	   #					      tuvw
	sl	r15,r5	   #						    x000
	o	r15,r7	   #						    xOLD
	srp	r6,r4	   #      0abc
	ai	r2,r2,-24
	ai	r3,r3,-24
	ai	r0,r0,-24
	bnmx	rmmob
	stm	r10,0(r2)  # 		      defg  hijk  lmno  pqrs  tuvw  xOLD

rmwob:	ai	r0,r0,20
	jm	rmb			#0, 1, 2, or 3 bytes yet to store
rmwo1:	ls	r15,0(r3)		#At least one full word to store.
	slp	r15,r5			#Use the same technique as above,
	o	r14,r7			#looping one word at a time.
	cas	r7,r15,r0
	sr	r7,r4
	dec	r3,4
	dec	r2,4
	sis	r0,4
	bnmx	rmwo1
	sts	r14,0(r2)

 #			|Store final (leftmost) 1, 2, or 3 bytes.
 #			|r5/8 of these are in r7.
rmb:	ais	r0,4
	jz	robret			#Word moves above finished us off.
rmb1:	dec	r2,1
	sis	r0,1
	bzx	robret
	stcs	r7,0(r2)
	sis	r5,8			#Shift count in r5 tells how many
	bnzx	rmb1			#bytes remain in r7
	sri	r7,8
	bx	rmb1			#Remaining 1 or 2 bytes come from
	ls	r7,0(r3)		#next source word
			#Unrolling above loop could save up to 8 cycles

robret:	lm	r6,0(sp)
	ai	sp,sp,40
	brx	r15
	cas	r0,r2,r0

 #	The ROMP PCC also uses the entry point blt$$ for doing structure
 #	assigns, when the scratch registers are not all free.  Currently,
 #	this is only when passing a structure as an argument to a function.

	.globl	blt$$

blt$$:
	#save caller's scratch registers

	ai	sp,sp,-20		#stack space for r2,r3,r4,r5,r15
	sts	r2,0(sp)		#save r2
	sts	r3,4(sp)		#save r3
	sts	r4,8(sp)		#save r4
	sts	r5,12(sp)		#save r5
	sts	r15,16(sp)		#save r15

	#call the normal blt routine

	ls	r2,20(sp)		#set r2 to 1st parm (destination)
	ls	r3,24(sp)		#set r3 to 2nd parm (source)
	balix	r15,bblt
	ls	r4,28(sp)		#set r4 to 3rd parm (length)
	cas	r0,r0,r0		#this inst is never executed

	#restore caller's scratch registers

	ls	r15,16(sp)		#restore r15
	ls	r5,12(sp)		#restore r5
	ls	r4,8(sp)		#restore r4
	ls	r3,4(sp)		#restore r4
	ls	r2,0(sp)		#restore r4

	#return to calling proc

	brx	r15			#return to caller
	ai	sp,sp,20		#stack space for r2,r3,r4,r5,r15
