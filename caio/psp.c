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
/* $Header: psp.c,v 5.0 86/01/31 18:12:36 ibmacis ibm42a $ */
/* $Source: /ibm/acis/usr/sys_ca/caio/RCS/psp.c,v $ */

/*
 *
 */
#include "psp.h"
#if NPSP > 0
#include "../h/param.h"
#include "../h/conf.h"
#include "../h/dir.h"
#include "../h/user.h"
#include "../h/proc.h"
#include "../h/ioctl.h"
#include "../h/tty.h"
#include "../h/systm.h"
#include "../h/uio.h"
#include "../h/file.h"
#include "../ca/debug.h"
#include "../caio/pspreg.h"
#include "../ca/io.h"
#include "../h/buf.h"
#include "../caio/ioccvar.h"

#define PSPSPL() _spl3()


/* following taken from hd.c */
#define	SHOW_CHARS	0x01
#define SHOW_RDWR	0x02
#define SHOW_INTR	0x04
#define SHOW_INIT	0x08
#define SHOW_IO		0x10
#define SHOW_WAIT	0x20	/* show wait */
#define SHOW_REGS	0x40
#define SHOW_OPEN	0x80

char pspdebug;
struct pspdevice device_a = {	
		0xF0008001,	/* control channel A */
		0xF0008003,  /* data    channel A */
		0xF0008020,  /* ext_reg channel A */
		0xF0008060,  /* int_ack */
		};
struct pspdevice device_b = {

		0xF0008000,	/* control channel B */
		0xF0008002,  /* data    channel B */
		0xF0008040,  /* ext_reg channel B */
		0xF0008060,  /* int_ack */
			};

static struct pspbaudtbl {
	unsigned char msb;
	unsigned char lsb;
} pspbaudtbl [NSPEEDS] = {
/* B0 */	0x00, 0x00,
/* B50 */	0x77, 0xfe,
/* B75 */	0x4f, 0xfe,
/* B110 */	0x36, 0x8a,
/* B134 */	0x2c, 0x9a,
/* B150 */	0x27, 0xfe,
/* B200 */	0x1d, 0xfe,
/* B300 */	0x13, 0xfe,
/* B600 */	0x09, 0xfe,
/* B1200 */	0x04, 0xfe,
/* B1800 */	0x03, 0x53,
/* B2400 */	0x02, 0x7e, 
/* B4800 */	0x01, 0x3e,
/* B9600 */	0x00, 0x9e,
/* EXTA */	0x00, 0x4e	/* 19200 */
/* EXTB                            38400  not supported */
} ;
#define ISPEED B9600
#define NPORT  2              /* Number of ports per card */
#define NREGS  16             /* Number of registers */
#define PSP_CHANA  0          /* psp channel A */
#define PSP_CHANB  1          /* psp channel B */

static int	overrun_error[NPSP*NPORT];
static int	parity_error[NPSP*NPORT];
static char	pspstat[NPSP*NPORT];
char pspsoftCAR;	/* carrier detect flags */


char psp_write_regs [NPORT][NREGS];
struct	tty	psp[NPSP*NPORT];

int	pspstart();
int	ttrstrt();
int	pspattach();
int	pspprobe();
int	pspint();
int 	pspmprobe();
int	pspmatch();
int	pspmint();
char	psp_read_reg();



struct	iocc_device *pspdinfo[NPSP];
struct	iocc_device *pspmdinfo[NPSP*NPORT];

struct iocc_driver pspdriver = { pspprobe, 0, pspattach, 0, /* pspstd */ 0,
	"psp", pspdinfo, 0, 0, pspint };

pspmint (unit, icscs)
register int unit;
register int icscs;

{
	register int c;
	register struct tty *tp = &psp[unit];
	register char savemsr;
	register int s;
	savemsr = psp_read_reg(unit,R0);
	DEBUGF(pspdebug & SHOW_INTR, printf ("In pspmint (%d) R0=%x \n",unit,savemsr));
	if  (savemsr & R0_DCD )	/* if carrier transition */
/*		{*/
/*		psp_write_reg(unit,W0,RST_EXT_INT); /*garanties the current state */
/*		if ((savemsr =psp_read_reg(unit,R0) )& R0_DCD) /* if carrier on */
			{
			DEBUGF(pspdebug & SHOW_INTR ,printf("Carrier detected \n"));
			if ((tp->t_state & TS_CARR_ON) == 0)
				{
				tp->t_state |= TS_CARR_ON;
				wakeup((caddr_t)&tp->t_rawq);
				}
			}
		else
			{
			DEBUGF(pspdebug & SHOW_INTR ,printf("Lost carrier \n"));
			if (tp->t_state & TS_CARR_ON) 
				{
		   		gsignal (tp-> t_pgrp, SIGHUP);
		 		gsignal (tp-> t_pgrp, SIGCONT);
				ttyflush(tp,FREAD|FWRITE);
				}				
			tp->t_state &= ~ TS_CARR_ON;
			}


/*		}*/
psp_write_reg(unit,W0,RST_EXT_INT); 
psp_write_reg(unit,W0,RST_IUS);
DEBUGF(pspdebug , printf("asymint%d end ",unit));
return(0);
}


/*
 * Pspprobe will try to generate an interrupt so that the system finds out
 * if the specific psp is present in the system (floor model /desktop).
 */
pspprobe(addr)
register caddr_t addr;
{

	/* initialize the 8530 both channels */
	psp_write_reg(0,W9,RST_HARD);	/* hardware reset */
	psp_write_reg(1,W9,RST_HARD);	/* hardware reset */
	psp_write_reg(0,W11,0x56);		/* Clock register */
	psp_write_reg(1,W11,0x56);		/* Clock register */
	psp_write_reg(0,W13,0xFF);		/* load counters */
	psp_write_reg(0,W12,0xFF);
	psp_write_reg(0,W9,MIE);		/* enable master int. */
	psp_write_reg(0,W1,EN_EXT_INT);
	psp_write_reg(0,W15,2); 		/* enable zero count int. */
	psp_write_reg(0,W11,1); 		/* enable baud rate */
	DELAY(100000);
	psp_write_reg(0,W11,0); 		/* disable baud rate generation */
	psp_write_reg(0,W15,0);
	*(char *) 0xf0008060 = 0; /* interrupt acknowlege */
	 psp_write_reg(0,W0,RST_IUS);
	
	psp_write_reg(0,W9,RST_HARD);	/* hardware reset */
	psp_write_reg(1,W9,RST_HARD);	/* hardware reset */
	psp_write_reg(0,W11,0x56);		/* Clock register */
	psp_write_reg(1,W11,0x56);		/* Clock register */
	psp_set_baud(0,ISPEED);
	psp_set_baud(1,ISPEED);
	psp_write_reg(0,W9,MIE);
	return(PROBE_OK);
}

pspattach (iod)
register struct iocc_device *iod;
{
	register struct tty *tp;
	register int ctlr = iod->iod_unit;
	register int subunit;

	for (subunit=0 ;subunit <= 1;subunit++)
	{
		tp = &psp[subunit];
		tp->t_addr = (subunit) ? (caddr_t) &device_b:(caddr_t) &device_a; 
		tp->t_state = 0;
		ttychars(tp);
		tp->t_ospeed = tp->t_ispeed = ISPEED;
		tp->t_flags = EVENP|ODDP|ECHO;
	};
	pspsoftCAR = iod->iod_flags; /*modem control flags  */
}
	
/*ARGSUSED*/
pspopen(dev, flag)
	dev_t dev;
{
	register struct tty *tp;
	register int unit = minor (dev);
	register struct iocc_device *iod;
	register int s;
	register caddr_t addr;
	register struct pspdevice *ppsp;

	DEBUGF(pspdebug & SHOW_OPEN, printf ("In PSPOPEN unit=%x\n",unit));
	if (unit >= NPSP*NPORT || (iod =pspdinfo[0]) ==0 || iod->iod_alive ==0)
		return(ENXIO);
	parity_error[unit]=0;
	overrun_error[unit]=0;
	tp = &psp[unit];
	ppsp = (struct pspdevice *) tp->t_addr;
	tp->t_oproc = pspstart;
	tp->t_state |= TS_WOPEN;	
	if ((tp->t_state&TS_ISOPEN) == 0)
		 {
		ttychars(tp);
		tp->t_ospeed = tp->t_ispeed = ISPEED;
		tp->t_flags = EVENP|ODDP|ECHO;
		pspparam (unit);
		s= PSPSPL();
		psp_set_bits(unit,W5,RTS); /* request to send ( on the chip ) */
		*(char *)ppsp->ext_reg = EXT_DTR;	/* dtr on */	
		 psp_set_bits(unit,W1,RX_INT_MODE| EN_EXT_INT);
		(void) splx(s);
		}

	if (tp->t_state&TS_XCLUDE && u.u_uid != 0)
		return (EBUSY);

	psp_set_bits(unit,W5,RTS); /* request to send ( on the chip ) */
	*(char *)ppsp->ext_reg = 1;	/* dtr on */	

	psp_write_reg(unit,W15,W15_DCD_IE);
	if (( psp_read_reg(unit,R0) & R0_DCD) ||pspsoftCAR & (1 << unit & 3))
		tp->t_state |= TS_CARR_ON;
	s= PSPSPL();
	while ((tp-> t_state & TS_CARR_ON)==0)
		{
		tp-> t_state = TS_WOPEN;
		sleep((caddr_t) &tp->t_rawq, TTIPRI);
		}
	(void) splx(s);
	DEBUGF(pspdebug & SHOW_OPEN, printf ("PSPOPEN: call ttyopen (%x)\n", linesw[tp->t_line].l_open));
	return ((*linesw[tp->t_line].l_open)(dev, tp));
}

/*ARGSUSED*/
pspclose(dev)
	dev_t dev;
{
	register int unit = minor(dev);
	register struct tty *tp = &psp[unit];
	register struct pspdevice *ppsp;

	DEBUGF(pspdebug & SHOW_OPEN, printf ("In PSPCLOSE\n"));

	ppsp = (struct pspdevice *) tp->t_addr;

	(*linesw[tp->t_line].l_close)(tp);
	psp_reset_bits(unit,W5,BREAK);	/* reset break */

	if ((tp->t_state&TS_WOPEN|TS_HUPCLS) || (tp->t_state&TS_ISOPEN) == 0)

	{
	psp_reset_bits(unit,W1,EN_EXT_INT |RX_INT_MODE); /* disable ints. */	
	psp_reset_bits(unit,W5,RTS); /* turn rts off*/
	*(char *)ppsp->ext_reg = 0;	/* dtr off */	
	}

	ttyclose(tp);
	DEBUGF(pspdebug & SHOW_OPEN, printf ("PSPCLOSE (%d) end\n",unit));
}

/*ARGSUSED*/
pspread(dev, uio)
	register dev_t dev;
	register struct uio *uio;
{
	register struct tty *tp = &psp[minor(dev)];
	return ((*linesw[tp->t_line].l_read)(tp, uio));
}

/*ARGSUSED*/
pspwrite(dev, uio)
	dev_t dev;
	struct uio *uio;
{
	register struct tty *tp = &psp[minor(dev)];

	DEBUGF(pspdebug & SHOW_RDWR, printf ("In PSPWRITE\n"));
	return ((*linesw[tp->t_line].l_write)(tp, uio));
}

psprint(unit)
	register int unit;
{
	register int c,s;
	register struct tty *tp = &psp[unit];
	register struct pspdevice *ppsp = (struct pspdevice *)tp->t_addr; 
	register char savelsr;

	DEBUGF(pspdebug & SHOW_INTR, printf ("In PSPRINT (%d)",unit));

	while (( savelsr= psp_read_reg(unit, R0)) & R0_CHAR_AVAIL )
	{
	savelsr= psp_read_reg(unit, R1);  /* read status */
	c =  psp_read_reg(unit,R8);		/* read char */

	if (savelsr & PARITY_ERR)
		if (((tp->t_flags & (EVENP|ODDP)) == EVENP)
		  || ((tp->t_flags & (EVENP|ODDP)) == ODDP))
		  {
			++parity_error[unit];
			DEBUGF(pspdebug, printf(" psprint:Parity error %d\n",
					parity_error[unit]));
			psp_write_reg(unit,W0,RST_ERROR);
			continue;
		  }
		if (savelsr & OVERRUN_ERR)
			{
			printf ("psprint%d overrun error\n",unit);
			++overrun_error[unit];
			psp_write_reg(unit,W0,RST_ERROR);
			continue;
			}
		if (savelsr & (FRAM_ERR ))
		{
			psp_write_reg(unit,W0,RST_ERROR);
			DEBUGF(pspdebug,printf(" framming error"));
			if (tp->t_flags & RAW)
				c = 0;
			else
				c = tp->t_intrc;
		}

	DEBUGF(pspdebug & SHOW_CHARS,printf("%c", c));
		(*linesw[tp->t_line].l_rint)(c, tp);
	}
done:
	DEBUGF(pspdebug,printf(" psprint  done \n"));
	 psp_write_reg(unit,W0,RST_IUS);
}

/*ARGSUSED*/
pspioctl(dev, cmd, addr, flag)
	dev_t dev;
	caddr_t addr;
{
	register int unit = minor(dev);
	register struct tty *tp = &psp[unit];
	register struct pspdevice *ppsp = (struct pspdevice *)tp->t_addr; 
	register int error,s;
	DEBUGF(pspdebug,printf(" In pspioctl (%d) ",unit)); 
	error = (*linesw[tp->t_line].l_ioctl)(tp, cmd, addr,flag);
	if (error >= 0)
		return (error);
	error = ttioctl(tp, cmd, addr, flag);
	if (error >= 0) {
		if (cmd == TIOCSETP || cmd == TIOCSETN) {
			pspparam(unit);
		}
		return (error);
	}
	s=PSPSPL();
	switch (cmd){
	case TIOCSBRK:psp_set_bits(unit,W5,BREAK);
			DEBUGF(pspdebug,printf("set break"));break;
 	case TIOCCBRK:psp_reset_bits(unit,W5,BREAK);
			DEBUGF(pspdebug,printf("clr break"));break;
	case TIOCSDTR:psp_set_bits(unit,W5,RTS); 
			*(char *)ppsp->ext_reg = EXT_DTR;
			DEBUGF(pspdebug,printf("set DTR RTS"));break;
	case TIOCCDTR:psp_reset_bits(unit,W5,RTS);
			*(char *)ppsp->ext_reg = 0;
			DEBUGF(pspdebug,printf("clr DTR RTS"));break;
	default:
		DEBUGF(pspdebug & SHOW_INTR,printf(" cmd unsupported "));
		(void) splx(s);
		return(ENOTTY);
	}
	
	DEBUGF(pspdebug & SHOW_INTR  ,printf(" pspioctl (%d) end\n",unit)); 
	(void) splx(s);
	return(0);
}

/*
 * Got a transmitter empty interrupt 
 * the psp wants another character.
 */
pspxint(unit,icscs)
	register int unit;
{
	register struct tty *tp = &psp[unit];
 

	DEBUGF(pspdebug & SHOW_INTR,printf ("In PSPXINT (%d)  ",unit));
	tp->t_state &= ~TS_BUSY;
	psp_write_reg(unit,W0,RST_TX_INT); /* reset tx int */
	if (tp->t_outq.c_cc <= 0) {
		if(psp_read_reg(unit,R1) & ALL_SENT)
		psp_reset_bits(unit,W1,EN_TX_INT); /* disable tx int */
	}
	if (tp->t_line)
		(*linesw[tp->t_line].l_start)(tp);
	else
		pspstart(tp);
	DEBUGF(pspdebug & SHOW_INTR, printf ("pspxint end\n"));
	 psp_write_reg(unit,W0,RST_IUS);

}

pspstart(tp)
	register struct tty *tp;
{
	register int c, s;
	register int unit = minor(tp->t_dev);

	DEBUGF(pspdebug & SHOW_IO, printf ("In PSPSTART\n"));
	s = PSPSPL();
	if (tp->t_state & (TS_TIMEOUT|TS_BUSY|TS_TTSTOP))
		goto out2;
	if (tp->t_outq.c_cc <= TTLOWAT(tp)) {
		if (tp->t_state&TS_ASLEEP) {
			tp->t_state &= ~TS_ASLEEP;
			wakeup((caddr_t)&tp->t_outq);
		}
		if (tp->t_wsel) {
			selwakeup(tp->t_wsel, tp->t_state & TS_WCOLL);
			tp->t_wsel = 0;
			tp->t_state &= ~TS_WCOLL;
		}
	}
	while ( psp_read_reg(unit,R0) & TX_EMPTY) {
		if ((tp->t_outq.c_cc <= 0) || (c = getc(&tp->t_outq)) == -1) {
			goto out;
		}

	DEBUGF(pspdebug & SHOW_CHARS,
		printf ("%c", (char)(c)));
		if (tp->t_flags&(RAW|LITOUT)) {
			psp_write_reg(unit,W8,c);
		} else if (c <= 0177) {
			psp_write_reg(unit,W8,c);
			
		} else {
			timeout(ttrstrt, (caddr_t)tp, (c&0177));
			tp->t_state |= TS_TIMEOUT;
	DEBUGF(pspdebug & SHOW_CHARS, 	printf ("pspstart timeout\n"));
			goto out;
		}
	}
		tp->t_state |= TS_BUSY;
		psp_set_bits(unit,W1,EN_TX_INT);
		goto out2;
out:	
	if(psp_read_reg(unit,R1) & ALL_SENT)
		{
		psp_reset_bits(unit,W1,EN_TX_INT);
		tp->t_state &= ~TS_BUSY;
		}
out2:
/*	 psp_write_reg(unit,W0,RST_IUS);*/
	(void) splx(s);
	DEBUGF(pspdebug & SHOW_IO, printf (" pspstart end\n"));
}


/*
 setting the async parameter ( baud rate , data length ...)
 */

pspparam (unit)
register int unit;
{
	register struct tty *tp = &psp[unit];
	register char txbits, mode, rxbits;
	register int s;		
	register struct pspdevice *ppsp = (struct pspdevice *)tp->t_addr; 
	register char flushchar;

	DEBUGF(pspdebug&SHOW_IO, {printf ("In PSPPARAM  ");
	printf ("tp->t_flags = (%x)\n", tp->t_flags);});

	if (tp->t_ispeed == B0) {
		tp->t_state |= TS_HUPCLS;
		psp_reset_bits(unit,W5,RTS); /* turn rts off*/
		*(char *)ppsp->ext_reg = 0;	/* dtr off */	
		return;
	}
	s= PSPSPL();
	/* Set the baud rate and initial pspnc settings */
	psp_set_baud(unit, tp->t_ispeed );
	if (tp->t_flags & (RAW|LITOUT))
		{
		rxbits= RX_8BITS| RX_ENABLE;
		txbits= TX_8BITS| TX_ENABLE;
		mode =0;
		}
	else
		{
		rxbits= RX_7BITS| RX_ENABLE;
		txbits= TX_7BITS| TX_ENABLE;
		mode = EN_PARITY;
		}

	if (tp->t_flags & EVENP)  mode |=PARITY_EV; 
	if (tp->t_ispeed == B110)
		mode |= STOP2;
	else  mode |= STOP1;
	psp_write_reg(unit,W3,rxbits );
	psp_write_reg(unit,W4,mode );
	psp_write_reg(unit,W5,txbits  );
	DEBUGF(pspdebug,printf(" pspparam unit %d ",unit));
	(void) splx(s);
}

/*
 * Detrmine the interrupt source
 */
#define TXINT_A	0x10
#define RXINT_A	0x20
#define MOINT_A 0x08
#define TXINT_B	0x02
#define RXINT_B 0x04
#define MOINT_B 0x01

pspint (ctlr, icscs)
register int ctlr;
register int icscs;
{
	register unsigned char port;
	register  int didit=0;


	*(char *) 0xf0008060 = 0; /* interrupt acknowlege */
	while((	port = psp_read_reg(PSP_CHANA,R3)) !=0)
	{
	DEBUGF(pspdebug & SHOW_INTR, printf ("pspint port =%x\n",port));
		if ( port & RXINT_A ) { psprint(PSP_CHANA,icscs);}; 
		if ( port & TXINT_A ) { pspxint(PSP_CHANA,icscs);}; 
		if ( port & MOINT_A ) { pspmint(PSP_CHANA,icscs);};
		if ( port & RXINT_B ) { psprint(PSP_CHANB,icscs);}; 
		if ( port & TXINT_B ) { pspxint(PSP_CHANB,icscs);}; 
		if ( port & MOINT_B ) { pspmint(PSP_CHANB,icscs);};
		didit++;
	}
return (didit  == 0);
}

/*
 * The following routines handle the reading and writing to the 8530 chip
 */

char psp_read_reg(unit,reg)
register int unit;
register int reg;
{
	register struct pspdevice  *ppsp; 
	ppsp= (unit) ? &device_b :&device_a; 
	*(char *) ppsp->control = reg & 0xf;   
	 return( *(char *) ppsp->control); 
}

psp_write_reg(unit,reg,data)
register int unit;
register int reg;
char data;
{
	register struct pspdevice  *ppsp; 
	ppsp= (unit) ? &device_b :&device_a; 

	psp_write_regs[unit][reg]=data;	/* save area */

	*(char *) ppsp->control = reg & 0xf;   
	*(char *)ppsp->control = data;
        return;
}

psp_set_bits(unit,reg,bits)
register int unit;
register int reg;
char bits;
{

	register struct pspdevice  *ppsp; 
	ppsp= (unit) ? &device_b :&device_a; 

	*(char *) ppsp->control = reg & 0xf;   
	psp_write_regs[unit][reg] |= bits;
	return( *(char *) ppsp->control =  psp_write_regs[unit][reg]); 
}

psp_reset_bits(unit,reg,bits)
register int unit;
register int reg;
char bits;
{

	register struct pspdevice  *ppsp; 
	ppsp= (unit) ? &device_b :&device_a; 

	*(char *) ppsp->control = reg & 0xf;   
	psp_write_regs[unit][reg] &=  ~bits;
	return( *(char *) ppsp->control =  psp_write_regs[unit][reg]); 
}
psp_set_baud(unit,baud)
int unit,baud;
{

if ( baud < EXTB )
	{
	psp_write_reg(unit,W14,0);		/* disable baud rate generation*/
	psp_write_reg(unit,W13,pspbaudtbl[baud].msb); /* high order byte  */
	psp_write_reg(unit,W12,pspbaudtbl[baud].lsb);
	psp_write_reg(unit,W14,1);		/* enable baud rate generation*/
	}
}

#endif NPSP
