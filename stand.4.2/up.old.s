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
LL0:
	.data
	.comm	_devsw,0
	.comm	_b,32768
	.comm	_blknos,16
	.comm	_iob,66416
	.comm	_cpu,4
	.comm	_mbaddr,4
	.comm	_mbaact,4
	.comm	_umaddr,4
	.comm	_ubaddr,4
	.data
	.align	1
	.globl	_ubastd
_ubastd:
	.long	0xfdc0
	.comm	_up_gottype,1
	.comm	_up_type,1
	.text
	.align	1
	.globl	_upopen
_upopen:
	.word	L31
	jbr 	L33
L34:
	movl	4(ap),r11
L35:
	jbr 	L35
L36:
	.data	1
L41:
	.ascii	"unknown drive type\0"
	.text
L39:
L37:
	.data	1
L43:
	.ascii	"up bad unit\0"
	.text
L42:
	ret
	.set	L31,0xe00
L33:
	jbr 	L34
	.data
	.text
	.align	1
	.globl	_upstrategy
_upstrategy:
	.word	L45
	jbr 	L47
L48:
	movl	4(ap),r11
L49:
	.data	1
L51:
	.ascii	"up not ready\0"
	.text
L50:
	jbr 	L54
L53:
L54:
L57:
L58:
	jbr 	L58
L59:
L56:
L55:
	.data	1
L62:
	.ascii	"up error\72 (cyl,trk,sec)=(%d,%d,%d) cs2=%b er1=%b er2=%b\12\0"
	.text
	.data	1
L63:
	.ascii	"\10\20DLT\17WCE\16UPE\15NED\14NEM\13PGE\12MXF\11MDPE\10OR\7IR\6CLR\5PAT\4BAI\0"
	.text
	.data	1
L64:
	.ascii	"\10\20DCK\17UNS\16OPI\15DTE\14WLE\13IAE\12AOE\11HCRC\10HCE\7ECH\6WCF\5FER\4PAR\3RMR\2ILR\1I"
	.ascii	"LF\0"
	.text
	.data	1
L65:
	.ascii	"\10\20BSE\17SKI\16OPE\15IVC\14LSC\13LBC\12MDS\11DCU\10DVC\7ACU\4DPE\0"
	.text
	ret
L60:
	ret
	ret
	.set	L45,0xf00
L47:
	subl2	$32,sp
	jbr 	L48
	.data
	.text
	.align	1
	.globl	_upioctl
_upioctl:
	.word	L68
	jbr 	L70
L71:
	ret
	ret
	.set	L68,0x0
L70:
	jbr 	L71
	.data
