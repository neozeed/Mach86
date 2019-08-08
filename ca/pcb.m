/* $Header: pcb.m,v 4.1 85/08/22 11:37:28 webb Exp $ */
/* $Source: /ibm/acis/usr/sys_ca/ca/RCS/pcb.m,v $ */

	.data
rcsidpcb:	.asciz	"$Header: pcb.m,v 4.1 85/08/22 11:37:28 webb Exp $"
	.text

/*     pcb.m   6.1     83/07/29        */

/*
 * VAX process control block
 */
#define PCB_KSP   0
#define PCB_ESP   4
#define PCB_SSP   8
#define PCB_USP   16
#define PCB_R0   12
#define PCB_R1   16
#define PCB_R2   20
#define PCB_R3   24
#define PCB_R4   28
#define PCB_R5   32
#define PCB_R6   36
#define PCB_R7   40
#define PCB_R8   44
#define PCB_R9   48
#define PCB_R10   52
#define PCB_R11   56
#define PCB_R12   60
#define PCB_R13   64
#define PCB_R14   68
#define PCB_R15   72
#define PCB_IAR   76
#define PCB_ICSCS   80
#define PCB_P0BR   84
#define PCB_P0LR   88
#define PCB_P1BR   92
#define PCB_P1LR   96
/*
 * Software pcb extension
 */
#define PCB_SZPT   100 /* number of user page table pages */
#define PCB_CMAP2   104 /* saved cmap2 across cswitch (ick) */
#define PCB_SSWAP   108 /* flag for non-local goto */
#define PCB_SIGC   112 /* signal trampoline code */
#define PCB_CCR	   124	/* CCR value */

#define AST_USER   0x80000000 /* VAX AST simulation for user mode in ics*/
#define AST_USER_BIT   0 /* VAX AST simulation for user mode in ics*/
