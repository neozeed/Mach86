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
	.comm	_upbad,512
	.comm	_sectsiz,4
	.comm	_updebug,4
	.globl	_up_offset
_up_offset:
	.long	0x90109010
	.long	0xa020a020
	.long	0xb030b030
	.long	0x0
	.text
	.align	1
	.globl	_upopen
_upopen:
	.word	L35
	jbr 	L37
L38:
	movl	4(ap),r11
	.data	1
L41:
	.ascii	"up bad unit\0"
	.text
L39:
L42:
	jbr 	L42
L43:
	.data	1
L47:
	.ascii	"unknown drive type\0"
	.text
L46:
	.data	1
L49:
	.ascii	"up bad unit\0"
	.text
L48:
L52:
	jbr 	L51
L54:
L50:
	jbr 	L52
L51:
	.data	1
L57:
	.ascii	"Unable to read bad sector table\12\0"
	.text
L60:
L58:
	jbr 	L60
L59:
L55:
L44:
	ret
	.set	L35,0xf80
L37:
	movab	-16604(sp),sp
	jbr 	L38
	.data
	.text
	.align	1
	.globl	_upstrategy
_upstrategy:
	.word	L61
	jbr 	L63
L64:
	movl	4(ap),r11
L65:
L66:
	.data	1
L68:
	.ascii	"up not ready\0"
	.text
L67:
L70:
	.data	1
L72:
	.ascii	"wc=%d o=%d i_bn=%d bn=%d\12\0"
	.text
L71:
L73:
	jbr 	L73
L74:
	ret
L76:
L80:
L81:
	jbr 	L81
L82:
L79:
L78:
	jbr 	L84
L83:
	.data	1
L86:
	.ascii	"up error\72 (cyl,trk,sec)=(%d,%d,%d) \0"
	.text
	.data	1
L87:
	.ascii	"cs2=%b er1=%b er2=%b wc=%d\12\0"
	.text
	.data	1
L88:
	.ascii	"\10\20DLT\17WCE\16UPE\15NED\14NEM\13PGE\12MXF\11MDPE\10OR\7IR\6CLR\5PAT\4BAI\0"
	.text
	.data	1
L89:
	.ascii	"\10\20DCK\17UNS\16OPI\15DTE\14WLE\13IAE\12AOE\11HCRC\10HCE\7ECH\6WCF\5FER\4PAR\3RMR\2ILR\1I"
	.ascii	"LF\0"
	.text
	.data	1
L90:
	.ascii	"\10\20BSE\17SKI\16OPE\15IVC\14LSC\13LBC\12MDS\11DCU\10DVC\7ACU\4DPE\0"
	.text
L85:
L91:
L93:
	jbr 	L93
L94:
	jbr 	L91
L92:
	.data	1
L96:
	.ascii	"up%d\72 write locked\12\0"
	.text
	ret
L95:
	jbr 	L100
L99:
L101:
L102:
	.data	1
L103:
	.ascii	"up error\72 (cyl,trk,sec)=(%d,%d,%d) cs2=%b er1=%b er2=%b\12\0"
	.text
	.data	1
L104:
	.ascii	"\10\20DLT\17WCE\16UPE\15NED\14NEM\13PGE\12MXF\11MDPE\10OR\7IR\6CLR\5PAT\4BAI\0"
	.text
	.data	1
L105:
	.ascii	"\10\20DCK\17UNS\16OPI\15DTE\14WLE\13IAE\12AOE\11HCRC\10HCE\7ECH\6WCF\5FER\4PAR\3RMR\2ILR\1I"
	.ascii	"LF\0"
	.text
	.data	1
L106:
	.ascii	"\10\20BSE\17SKI\16OPE\15IVC\14LSC\13LBC\12MDS\11DCU\10DVC\7ACU\4DPE\0"
	.text
L108:
L110:
	jbr 	L110
L111:
	jbr 	L108
L109:
L107:
	ret
L97:
	jbr 	L100
L113:
	jbr 	L102
L112:
	ret
L116:
	jbr 	L100
L115:
	jbr 	L102
L114:
	jbr 	L102
L117:
L119:
L121:
	jbr 	L121
L122:
	jbr 	L119
L120:
L123:
L125:
	jbr 	L125
L126:
	jbr 	L123
L124:
L118:
L128:
L130:
	jbr 	L130
L131:
	jbr 	L128
L129:
L127:
	jbr 	L70
L100:
	jbr 	L70
L132:
L84:
L134:
L136:
	jbr 	L136
L137:
	jbr 	L134
L135:
L133:
	ret
	ret
	.set	L61,0xf80
L63:
	subl2	$36,sp
	jbr 	L64
	.data
	.text
	.align	1
	.globl	_upecc
_upecc:
	.word	L138
	jbr 	L140
L141:
	movl	4(ap),r11
L142:
	.data	1
L144:
	.ascii	"npf=%d mask=0x%x ec1=%d wc=%d\12\0"
	.text
L143:
	.data	1
L146:
	.ascii	"up%d\72 soft ecc sn%d\12\0"
	.text
L147:
	.data	1
L151:
	.ascii	"addr=0x%x old=0x%x \0"
	.text
L150:
	.data	1
L153:
	.ascii	"new=0x%x\12\0"
	.text
L152:
L149:
	ret
L154:
	jbr 	L147
L148:
	ret
L155:
	ret
L145:
	ret
L158:
	.data	1
L160:
	.ascii	"revector sn %d to %d\12\0"
	.text
L159:
L161:
	jbr 	L161
L162:
	ret
L163:
L166:
L167:
	jbr 	L167
L168:
L165:
L164:
	ret
L169:
L156:
L170:
	ret
	ret
	.set	L138,0xfc0
L140:
	subl2	$48,sp
	jbr 	L141
	.data
	.text
	.align	1
	.globl	_upstart
_upstart:
	.word	L171
	jbr 	L173
L174:
	movl	4(ap),r11
	jbr 	L176
L177:
	jbr 	L175
L178:
	jbr 	L175
L179:
	jbr 	L175
L180:
	jbr 	L175
L181:
L182:
	jbr 	L175
L183:
L184:
	jbr 	L175
L185:
	ret
L176:
	cmpl	r0,$256
	jeql	L177
	cmpl	r0,$512
	jeql	L178
	cmpl	r0,$1280
	jeql	L179
	cmpl	r0,$1536
	jeql	L180
	cmpl	r0,$2304
	jeql	L182
	cmpl	r0,$2560
	jeql	L181
	cmpl	r0,$4352
	jeql	L184
	cmpl	r0,$4608
	jeql	L183
	jbr 	L185
L175:
	ret
	ret
	.set	L171,0xe00
L173:
	subl2	$8,sp
	jbr 	L174
	.data
	.text
	.align	1
	.globl	_upioctl
_upioctl:
	.word	L187
	jbr 	L189
L190:
	jbr 	L192
L193:
	jbr 	L195
L194:
L195:
	ret
L196:
	ret
L192:
	cmpl	r0,$25608
	jeql	L196
	cmpl	r0,$25612
	jeql	L193
L191:
	ret
	ret
	.set	L187,0x0
L189:
	subl2	$16,sp
	jbr 	L190
	.data
