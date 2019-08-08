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
#include <sys/signal.h>
#include <sys/pcb.h>
#include <sys/psl.h>

int *
findsp(x)
{
    return(&x);
}

struct regframe
{
    int rf_r7;
    int rf_r8;
    int rf_r9;
    int rf_r10;
    int rf_r11;
    int rf_exc;
    int rf_state;
    int rf_ap;
    int rf_fp;
    int rf_upc;
    int rf_r0;
    int rf_r1;
    int rf_r2;
    int rf_r3;
    int rf_r4;
    int rf_r5;
    int rf_r6;
    int rf_argn;
    int rf_n;
    int rf_code;
    int rf_param;
    int rf_pc;
    int rf_ps;
};
    

catch(n, code, param, pc, psl)
{
    extern struct pcb kdbpcb;
    register int r11,r10;
    register struct regframe *rf = 0;
    register struct pcb *pcb = &kdbpcb;
    register int i;
    register int *sp = findsp(sp);

    rf = (struct regframe *)((int)&n-(int)&rf->rf_n);

    signal(SIGINT, catch);
    if (n == SIGINT)
    {
	psl |= PSL_T;
	return;
    }
    if (*(char *)pc != 3)
	psl |= PSL_T;
    printf("sp=%X, rf=%x, n=%d, code=%d, param=%X, pc=%X, psl=%X\n",
	   &n, rf, n, code, param, pc, psl);

#ifdef	UNDEF
    for (i=0; i<64; i++)
    {
	printf("%X: %X\n", sp, *sp);
	sp++;
    }
#endif	UNDEF
    pcb->pcb_r0 = rf->rf_r0;
    pcb->pcb_r1 = rf->rf_r1;
    pcb->pcb_r2 = rf->rf_r2;
    pcb->pcb_r3 = rf->rf_r3;
    pcb->pcb_r4 = rf->rf_r4;
    pcb->pcb_r5 = rf->rf_r5;
    pcb->pcb_r6 = rf->rf_r6;
    pcb->pcb_r7 = rf->rf_r7;
    pcb->pcb_r8 = rf->rf_r8;
    pcb->pcb_r9 = rf->rf_r9;
    pcb->pcb_r10 = rf->rf_r10;
    pcb->pcb_r11 = rf->rf_r11;
    pcb->pcb_ap = rf->rf_ap;
    pcb->pcb_fp = rf->rf_fp;
    pcb->pcb_usp = (int)(rf+1);
    pcb->pcb_pc = pc;
    pcb->pcb_psl = psl;
    kdb(psl&PSL_T);
    rf->rf_r0 = pcb->pcb_r0;
    rf->rf_r1 = pcb->pcb_r1;
    rf->rf_r2 = pcb->pcb_r2;
    rf->rf_r3 = pcb->pcb_r3;
    rf->rf_r4 = pcb->pcb_r4;
    rf->rf_r5 = pcb->pcb_r5;
    rf->rf_r6 = pcb->pcb_r6;
    rf->rf_r7 = pcb->pcb_r7;
    rf->rf_r8 = pcb->pcb_r8;
    rf->rf_r9 = pcb->pcb_r9;
    rf->rf_r10 = pcb->pcb_r10;
    rf->rf_r11 = pcb->pcb_r11;
    rf->rf_ap = pcb->pcb_ap;
    rf->rf_fp = pcb->pcb_fp;
    pc = pcb->pcb_pc;
    psl = pcb->pcb_psl;
}

main(argc, argv)
{
    register int r11 =0x11111111;
    register int r10 =0x10101010;
    register int r9  =0x09090909;
    register int r8 =0x08080808;
    register int r7 =0x07070707;
    register int r6 =0x06060606;
    int i;

    signal(SIGINT, catch);
    signal(SIGTRAP, catch);

    kdbsetsym(0,0,0);

    asm("movl $0x05050505,r5");
    asm("movl $0x04040404,r4");
    asm("movl $0x03030303,r3");
    asm("movl $0x02020202,r2");
    asm("movl $0x01010101,r1");
    asm("movl $0x00000000,r0");

    for (;;)
    {
	int i;
	printf("pausing...\n");
	for (i=1000000; i--;);
	printf("... done\n");
    }
}

kdbwrite(fd, base, len)
{
    return(write(1, base, len));
}

kdbread(fd, base, len)
{
    return(read(0, base, len));
}

kdbsbrk(n)
{
    return(sbrk(n));
}

kdbrlong(addr, p)
long *addr;
long *p;
{
    *p = *addr;
}

kdbwlong(addr, p)
long *addr;
long *p;
{
    *addr = *p;
}
