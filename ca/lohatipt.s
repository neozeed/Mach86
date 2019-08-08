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
 #----------------------------------------------------------------------|
 #		MEMORY/ROSETTA INITIALIZATION MODULE
 #----------------------------------------------------------------------|
 #  ASSUMES r8 - r15 ARE ZEROED
 #  ASSUMES (maybe) PAGESIZE = 2K
 #  WARNING this routine knows about the console message buffer at top
 #          of memory (just preceding the HATIPT table), in order not
 #          clear it out.

 # XXX FOLLOWING SHOULD PROBABLY BE SOMEWHERE ELSE
#define MEMCONFREG 0xF0008C80

	#  There are two 4-bit quantities in the m.c.r., one for
	#  each memory socket. The mapping is as follows:
	#
	#  m.c.r. value:  0-7  8   9  10  11  12  13   14    15
	#  size of card:  --- 4MB 1MB --- --- 8MB 2MB 512KB NONE

	.globl	_memcon
	b	_memcon
/* some new variables */
	.globl	_holestart
	.globl	_holelength
	.globl	_endmem

	.align	2
_holestart:	.long	0	# start of memory hole (in pages)
holestart:	.long	0	# start of memory hole (in bytes)
_holelength:	.long	0	# length of memory hole (in pages)
holelength:	.long	0	# length of memory hole (in bytes)
_endmem:	.long	0	# end of memory  (in pages)

/* configuration register values for various card sizes */
#define C_NONE	15
#define C_500K	14
#define C_1MEG	9
#define C_2MEG	13
#define C_4MEG	8
#define C_8MEG	12

/* memory sizes in units of 500K for current configurations */
#define K_500	1	/* 500 K of memory */
#define K_1000	2	/* 1 Meg of memory */
#define K_1500	3	/* 1.5 Meg of memory */
#define K_2000	4	/* 2 Meg of memory */
#define K_2500	5	/* 2.5 Meg of memory */
#define K_3000	6	/* 3 Meg of memory */
#define K_3500	7	/* 3.5 Meg of memory */
#define K_4000	8	/* 4 Meg of memory */

/* values for RAM register (add 9) for various memory sizes */
#define R_500	1
#define R_1000	2
#define R_2000	3
#define R_4000	4
#define R_8000	5

#define MEM_CONFIG(slot1,slot2,ram_value,phys_mem,size1,holesize) \
	.byte	(slot2<<4)+slot1, ram_value, phys_mem, size1, holesize, 0

 # following table contains the allowed memory configurations
 # all others are invalid and are rejected
 # note that since the kernel requires more than 500K it is not viable
 # to have a 500K card followed by anything other than another 500K card.
 #
#define	CONFIG	0	/* memory config table value */
#define RAM	1	/* is value for the RAM specification register */
#define	PHYS	2	/* is the size that includes all of RAM (including hole) */
#define	SIZE1	3	/* is the size of the first (or only) block of memory */
#define HOLESIZE 4	/* is the size of the hole (if any) */

#define MEMSIZE	6		/* number of bytes/entry in table */
.globl _memtable
_memtable:

	/*		slot1	slot2	RAM	PHYS	SIZE1	HOLESIZE */

	/* standard (supported) configurations */
	MEM_CONFIG(	C_2MEG,	C_2MEG, R_4000,	K_4000,	K_2000,	0	)
	MEM_CONFIG(	C_1MEG,	C_1MEG, R_4000,	K_2000,	K_1000,	K_1000	)
	MEM_CONFIG(	C_2MEG,	C_500K, R_4000,	K_2500,	K_2000,	0	)
	/* non-standard configurations */
	MEM_CONFIG(	C_500K,	C_500K, R_1000,	K_1000,	K_500,	0	)
	MEM_CONFIG(	C_1MEG,	C_NONE, R_1000,	K_1000,	K_1000,	0	)
	MEM_CONFIG(	C_NONE,	C_1MEG, R_1000,	K_1000,	K_1000,	0	)
	MEM_CONFIG(	C_2MEG,	C_NONE, R_2000,	K_2000,	K_2000,	0	)
	MEM_CONFIG(	C_NONE,	C_2MEG, R_2000,	K_2000,	K_2000,	0	)
	MEM_CONFIG(	C_2MEG,	C_1MEG, R_4000,	K_3000,	K_2000,	0	)
	MEM_CONFIG(	C_1MEG,	C_2MEG, R_4000,	K_3000,	K_1000,	K_1000	)
	MEM_CONFIG(	C_1MEG,	C_500K, R_4000,	K_1500,	K_1000,	K_1000	)
	/* non-viable configurations */
 #	MEM_CONFIG(	C_500K,	C_NONE, R_500,	K_500,	K_500,	0	)
 #	MEM_CONFIG(	C_NONE,	C_500K, R_500,	K_500,	K_500,	0	)
 #	MEM_CONFIG(	C_500K, C_1MEG,	R_4000,	K_1500,	K_500,	K_1500	)
 #	MEM_CONFIG(	C_500K, C_2MEG,	R_4000,	K_2500,	K_500,	K_1500	)
	/*  end of configurations */
	MEM_CONFIG(	0, 0,	0,	0,	0,	0	)

 #	.align 1
 #  Determine installed memory size from Memory Configuration Register.
 #  do this by looking up the value from the configuration register 
 #  (or the software override of it) in the above table.
 #  a configuration that isn't found will (for now) result in a hard
 #  loop. (To be done: set a error value into the LED's)
 #
_memcon:

	l	r2,_memconfig		# get software memory config
	cis	r2,0			# test and
	jne	1f			# if non-zero use it
	cau	r1,(MEMCONFREG)>>16(r0)	# r1-> mem config reg
	oil	r1,r1,(MEMCONFREG)&0xffff	# r1-> mem config reg
	lcs	r2,0(r1)		# check it out
	st	r2,_memconfig		# save for later reference
1:

	#  Convert the codes to 0=0 1=512K 2=1M 3=2M 4=4M 5=8M
	#  by table lookup (sorry about that Mike)
	# r2 is the value we are looking up
	# r5 is the pointer in the above table
	# r0 is the test value
	cal	r5,_memtable
1:	lcs	r0,CONFIG(r5)		# get the test value
	cis	r0,0			# test to see if done
	jeq	0f			# unsupported memory configuration
	c	r0,r2			# test if this one
	jeq	1f			# yes
	bx	1b
		inc	r5,MEMSIZE	
0:	
	get	r2,$0x97		# display for invalid configuration
	bali	r5,display		# and show it
	j	0b			# loop if bad
/* we now have the proper table entry pointed to by r5 */
1:

 # calculate the number of bytes and the number of pages
 # of actual memory 
 # r0 will contain the number of bytes of memory present
	lcs	r0,PHYS(r5)
	sli	r0,19-LOG2PAGESIZE	# compute number of pages
	st	r0,_physmem		# save for posterity

 # calculate hole position (in pages) 
	lcs	r4,SIZE1(r5)
	sli	r4,19-LOG2PAGESIZE	# compute number of pages
	st	r4,_holestart

 # calculate hole size (in pages) 
	lcs	r1,HOLESIZE(r5)
	sli	r1,19-LOG2PAGESIZE	# compute number of pages
	st	r1,_holelength
 #
 # calculate the end address of the memory present (for HAT/IPT)
 # and end page number
 # it is calculated from _physmem + _holelength
	cas	r1,r0,r1
	st	r1,_endmem		# save for posterity
	sli	r1,LOG2PAGESIZE		# convert to bytes

 #  Compute number of entries in the HAT/IPT table
 #  this is the RAM size / page size
 #  or (# of pages in 256k) << log (# of 256k segments)
 #  the 256k is because we start at 1 instead of 0

	lcs	r2,RAM(r5)		# get RAM size
	cau	r3,(256*1024/PAGESIZE)>>16(r0)# 256KB in pages
	oil	r3,r3,(256*1024/PAGESIZE)&0xffff# 256KB in pages
	sl	r3,r2			# r3 = number of HAT/IPT entries

 #  Compute hash mask for HAT chain head offset calculation

	ai	r0,r3,-1		# compute HAT hashing mask
	st	r0,_RTA_HASHMASK	# save for future use

 #  Load up the ROSETTA RAM Specification Register 
 #  Note: this is NOT the size of installed memory, it's
 #  the size of memory (with holes) rounded up to (512K * 2**N).

	inc	r2,9			# encode size for Rosetta
					# (10=512K, 11=1M, 12=2M, ...)
	cau	r7,(ROSEBASE)>>16(r0)	# base i/o address of Rosetta
	oil	r7,r7,(ROSEBASE)&0xffff	# base i/o address of Rosetta
	iow	r2,ROSE_RAM(r7)		# set Rosetta ram spec register
					# (set ram start address to 0)

 #  Compute HATIPT address and tell ROSETTA where it is.

	sli	r3,LOG2HATIPTSIZE	# r3 = size of hatipt table needed
	sf	r3,r1			# r3 = address of hatipt table
	inc	r2,2			# r2 = log2 hatipt base addr multiplier
	srp 	r3,r2			# r2 = rosetta hatipt address code
	iow	r2,ROSE_TCR(r7)		# set Rosetta translation control reg
					# (all flags in t.c.r. init'ed to 0)
	st	r3,hatipt		# remember it
	oiu	r2,r3,SYS_ORG/UPPER	# add system segment for translate mode
	st	r2,_RTA_HATIPT		# remember translated hatipt addr

 #  Record the usable memory size.
	
	srpi	r3,LOG2PAGESIZE		# r2 = number of pages (sans hatipt)
	st	r2,_maxmem		# max available memory
	st	r2,_freemem		# initialize free memory size

 #  Clear memory from end of kernel data to up near top of memory.
 #  Do not clear the msgbuf which is the MSGBUFPAGES pages before the
 #  HATIPT table, which we also don't clear here.
	
	l	r1,_edata$		# end of kernel initialized data
	niuo	r1,r1,0x0fff		# mask off segment number
	l	r2,hatipt		# don't clear HATIPT at top of mem
	ai	r2,r2,-MSGBUFPAGES*PAGESIZE - 32 # or msgbuf (may have valid data)

	#  Clear to next higher 32 byte boundary.

clrloopA:
	nilz	r0,r1,0x1f		# 32 byte boundary?
	jz	endclrloopA		# yes-->go to next phase
	sts	r8,0(r1)		# zero another word
	inc	r1,4			# point to next word
	j	clrloopA		# -->try again
endclrloopA:
		
	#  If we have a hole in the address space then clear
	#  up to the start of the hole
	#  then bump the pointer to the end of the hole
	#  if we are already past the hole then start clearing
	#  from here (but we probably won't get this far in that case)
	l	r3,_holestart		# get page number of hole
	l	r4,_holelength		# get length of hole
	cis	r4,0			# got a hole?
	jeq	1f			# no hole
	sli	r3,LOG2PAGESIZE		# get actual address
	st	r3,holestart
	sli	r4,LOG2PAGESIZE		# get actual address
	st	r3,holelength
	c	r1,r3
	jh	1f			# already past the hole
0:
	stm	r8,0(r1)		# zero 8 words
	c	r1,r3
	blx	0b			# no-->keep looping...
		ai	r1,r1,8*4	#   while bumping the pointer
	cas	r1,r3,r4		# point to start of next card
1:


	#  Clear from kernel data to the upper bound computed above
	#  r1 points to current 32byte block being cleared
	#  r2 points to the end of the region to clear

clrloopB:     				# do while more to clear:
	stm	r8,0(r1)		# zero 8 words
	c	r1,r2			# Done yet?
	blx	clrloopB		# no-->keep looping...
	ai	r1,r1,8*4		#   while bumping the pointer


 #  Initialize some more ROSETTA stuff.

	s	r0,r0			# zippo
	iow	r0,ROSE_SER(r7)		# clear exception bits
	iow	r0,ROSE_TLBS(r7)	# blast the entire tlb
	iow	r0,ROSE_TIDR(r7)	# set tid reg = 0 (user)
	
 #  Loop through HATIPT mapping system page i to real page i.

	cau	r0,(0x01ffffff)>>16(r0)	# Protection bits:
	oil	r0,r0,(0x01ffffff)&0xffff	# Protection bits:
			# spare(7),write(1)=1,tid(8)=ff,lockbits(16)=ffff	
	cau	r1,(SYS_ADDRTAG)>>16(r0)	# start with page zero
	oil	r1,r1,(SYS_ADDRTAG)&0xffff	# start with page zero
	l	r3,hatipt		# point to HATIPT at end of memory
	l	r4,_RTA_HASHMASK	# hashing significant bits
	ai	r2,r4,1			# r2 = number of HATIPT entries
	nilz	r4,r4,RTA_SID_SYSTEM	# partial result for hash calculations
	cal16	r5,RTA_UNDEF_PTR(r0)	# Invalid IPTPTR value for debug
	l	r7,_endmem		# pointer to end of valid memory
	cal	r8,-1(r0)		# invalid HATPTR
	l	r9,_holestart		# start hole
	l	r10,_holelength		# length of hole
	cas	r10,r9,r10		# end of hole

hatiptloop:
	sts	r1,IPTADDRTAG(r3)	# set ADDRTAG field to sys seg addr
	sths	r8,IPTHATPTR(r3)	# in case the hatptr is invalid here
	niuo	r6,r1,0x0001ffff/UPPER	# copy while nuking segment ID
	x	r6,r4			# "unhash" to get page pointer
	c	r6,r7			# beyond end of installed memory?
	jnl	hatinvalid		# yes, leave hat ptr invalid
	c	r6,r10			# past end of hole
	jnl	0f			# yes, make it valid
	c	r6,r9			# before start of hole
	jnl	hatinvalid		# nope, must be in hole
0:
#ifdef ROSETTA_0
	oil	r6,r6,RTA_HATIPT_PTRBUG	# circumvent bug in Rosetta 0 chip
#endif ROSETTA_0
	sths	r6,IPTHATPTR(r3)	# set HATPTR field to this page
hatinvalid:
	sths	r5,IPTIPTPTR(r3)	# invalid IPTPTR
	sts	r0,IPTLOCK(r3)		# set lock bits and flags
	inc	r1,1			# next page
	sis	r2,1			# decrement counter
	bnzx	hatiptloop		# loop if not zero
	    ai	    r3,r3,HATIPTSIZE	# ...while pointing to next entry

 #  Set up segment registers (for system mode)

_loadseg0:
	#  Segment 0 = user code/data (initially sid 0)

	cau	r0,(RTA_SEG_PRESENT)>>16(r0)# present = 1, sid = 0
	oil	r0,r0,(RTA_SEG_PRESENT)&0xffff# present = 1, sid = 0
	cau	r1,(RTA_SEGREG0)>>16(r0)	# load address of seg reg 0
	oil	r1,r1,(RTA_SEGREG0)&0xffff	# load address of seg reg 0
	iow	r0,0(r1)		# set seg reg 0 = 0 (user)

	#  Segment 1 = user stack / u struct (set to sid 0 initially)
_loadseg1:

	cal	r1,RTA_SEGREGSTEP(r1)	# address seg reg 1
	iow	r0,0(r1)		# set seg reg 1 = 0 (user)

	#  Segments 2 through D - unused for now

	cau	r0,(RTA_SID_UNUSED*4+RTA_SEG_PRESENT)>>16(r0)# sid unused, present = 1
	oil	r0,r0,(RTA_SID_UNUSED*4+RTA_SEG_PRESENT)&0xffff# sid unused, present = 1
	cau	r2,(RTA_NSEGREGS-4)>>16(r0)# loop count for rest of seg regs
	oil	r2,r2,(RTA_NSEGREGS-4)&0xffff# loop count for rest of seg regs

segregloop:
	cal	r1,RTA_SEGREGSTEP(r1)	# point to next seg reg
	iow	r0,0(r1)		# set to unused sid
	sis	r2,1			# decrement counter
	jnz	segregloop		# loop back

	#  Segment E = system

	cau	r0,(RTA_SID_SYSTEM*4+RTA_SEG_PRESENT)>>16(r0)# sid system, present = 1
	oil	r0,r0,(RTA_SID_SYSTEM*4+RTA_SEG_PRESENT)&0xffff# sid system, present = 1
	cal	r1,RTA_SEGREGSTEP(r1)	# address seg reg E
	iow	r0,0(r1)		# set seg reg E = system

	#  Set segment F not present; it belongs to the IO controller.

	s	r0,r0			
	iow	r0,RTA_SEGREGSTEP(r1) #	 Seg f not present.

 #  Determine the first page past unix - set _firstaddr

	l	r2,_end$		# point to end of unix
	niuo	r2,r2,0x0fffffff/UPPER	# mask off high 4 bits (seg #)
	ai	r2,r2,PAGESIZE-1	# past next page boundary
	sri	r2,LOG2PAGESIZE		# now just page number past unix
	slpi	r2,LOG2PAGESIZE		# r3 now a byte address
	st	r3,_firstaddr		# save for later

#if	MACH_VM
#else	MACH_VM
 #  This value is needed to hash system segment addresses

	l	r10,_RTA_HASHMASK	# mask of significant bits
	nilz	r10,r10,RTA_SID_SYSTEM	# system sid for hash
	sli	r10,LOG2HATIPTSIZE	# convert to table offset

 #  Map in _usrpt at a virtual address not congruent to its real address

	l	r9,hatipt		# origin of hatipt
	slpi	r2,LOG2HATIPTSIZE	# r3 = offset into ipt for the real page
	a	r3,r9			# r3-> hatipt entry for usrpt
	cas	r1,r10,r0		# get partially computed hash value
	x	r1,r3			# r1 = old hash anchor address
	sths	r8,IPTHATPTR(r1)	# zap invalidated hash pointer
	cau	r0,(USRPT_ADDRTAG)>>16(r0)# addrtag of _usrpt
	oil	r0,r0,(USRPT_ADDRTAG)&0xffff# addrtag of _usrpt
	st	r0,IPTADDRTAG(r3)	# set addrtag field
	cas	r1,r10,r0		# get partially computed hash value
	x	r1,r9			# r1 = new hash anchor address
	lhas	r0,IPTHATPTR(r1)	# grab what hashed here
	sths	r0,IPTIPTPTR(r3)	# chain that other page from this one
#ifdef ROSETTA_0
	oil	r0,r2,RTA_HATIPT_PTRBUG	# circumvent bug in Rosetta 0 chip
	sths	r0,IPTHATPTR(r1)	# put pointer to us at hashed address
#else !ROSETTA_0
	sths	r2,IPTHATPTR(r1)	# put pointer to us at hashed address
#endif ROSETTA_0


	#  add to _Usrptmap

	cau	r4,(PG_V+PG_KW)>>16(r0)	# "valid" and "kernel-r/w"
	oil	r4,r4,(PG_V+PG_KW)&0xffff	# "valid" and "kernel-r/w"
	o	r4,r2			# add valid and protection bits
	st	r4,_Usrptmap		# store in user page table map
	cas	r4,r2,r0		# get page number again
	sli	r4,LOG2PAGESIZE		# get actual address

 #  Map in the u area pages
_u_map:

	ai	r2,r2,UPAGES+1		# point past the last u area page
	ai	r3,r3,UPAGES*HATIPTSIZE	# point to the last u area hatipt
	cal	r4,NBPG-4(r4)		# index into _usrpt
	cau	r5,(UPAGES-1+UAREA_ADDRTAG)>>16(r0)# addrtag for that page
	oil	r5,r5,(UPAGES-1+UAREA_ADDRTAG)&0xffff# addrtag for that page
	l	r7,_RTA_HASHMASK	# get mask to compute uarea hash
	cau	r6,(UAREA_PAGE+UPAGES-1)>>16(r0)# Virtual page of last uarea page
	oil	r6,r6,(UAREA_PAGE+UPAGES-1)&0xffff# Virtual page of last uarea page
	n	r6,r7			# compute uarea hatipt hash anchor
	sli	r6,LOG2HATIPTSIZE	# convert to HATIPT table offset
	a	r6,r9			# now hatipt addr
	cal	r7,UPAGES(r0)		# how many pages to map
upage_loop:
	ai	r2,r2,-1			# map in one page
	cas	r1,r10,r0		# get partially computed hash value
	x	r1,r3			# r1 = old hash anchor address
	sths	r8,IPTHATPTR(r1)	# invalidate old hash pointer
	st	r5,IPTADDRTAG(r3)	# set addrtag field
	lh	r0,IPTHATPTR(r6)	# get pointer to other page in chain
	sths	r0,IPTIPTPTR(r3)	# set ipt pointer there
#ifdef ROSETTA_0
	oil	r0,r2,RTA_HATIPT_PTRBUG	# circumvent bug in Rosetta 0 chip
	sths	r0,IPTHATPTR(r6)	# u area page first on chain
#else !ROSETTA_0
	sths	r2,IPTHATPTR(r6)	# u area page first on chain
#endif ROSETTA_0

	#  add to _Usrptmap

	cau	r0,(PG_V+PG_KW)>>16(r0)	# "valid" and "kernel r/w"
	oil	r0,r0,(PG_V+PG_KW)&0xffff	# "valid" and "kernel r/w"
	o	r0,r2			# add valid protection bits
	sts	r0,0(r4)	

	#  go on to the next u area page

	ai	r3,r3,-HATIPTSIZE		# next u area hatipt entry
	ai	r4,r4,-4			# next _Usrptmap entry
	ai	r5,r5,-1			# next addrtag
	ai	r6,r6,-HATIPTSIZE		# next u area hatipt entry
	ai	r7,r7,-1			# decrement counter
	jnz	upage_loop		# loop
#endif	MACH_VM

 # CAREFUL!! r2 assumed to contain page num of first uarea page and
 #           r8 assumed to contain -1 by following code

 #--------------------END OF HATIPT TABLE INITIALIZATION---------------|
