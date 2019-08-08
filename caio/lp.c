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
/* $Header: lp.c,v 5.0 86/01/31 18:11:28 ibmacis ibm42a $ */
/* $Source: /ibm/acis/usr/sys_ca/caio/RCS/lp.c,v $ */

#include "romp_debug.h"
#include "lp.h"
#if NLP > 0
 /*
  * LP Line printer driver for IBM graphics printer
  */

#include "../h/ioctl.h"
#include "../h/param.h"
#include "../h/errno.h"
#include "../h/uio.h"
#include "../h/tty.h"
#include "../h/time.h"
#include "../ca/pte.h"
#include "../ca/io.h"
#include "../h/buf.h"
#include "../caio/ioccvar.h"
#include "../ca/debug.h"
#include "../h/kernel.h"

int lpprobe();
int lpattach();
int lpintr();
int lpwatch();

caddr_t lpstd[] = {			  /* standard line printers */
        (caddr_t)0xf00003bc,
        (caddr_t)0xf0000378,
        (caddr_t)0xf0000278,
        0
};

struct iocc_device *lpdinfo[NLP];

struct iocc_driver lpdriver = {
        lpprobe, 0, lpattach,
        0 /* dgo */ , lpstd, "lp", lpdinfo, 0, 0, lpintr
};

/* LP commands */
#define LP_STROBE_HIGH	0x0D		  /* Strobe HIGH      */
#define LP_STROBE_LOW	0x0C		  /* Strobe LOW       */
#define LP_RESET_LOW	0x08		  /* Reset line LOW   */
#define LP_RESET_HIGH	0x0C		  /* Reset line HIGH  */
#define LP_INTR_ENABLE	0x10		  /* Enable interrupts */

/* LP status bits */
#define LP_BUSY		0x80		  /* Printer busy */
#define LP_ACK		0x40		  /* Pinter acknowledgement */
#define LP_NOPAPER	0x20		  /* Printer out of paper */
#define LP_SELECT	0x10		  /* Printer Selected for printing */
#define LP_ERROR	0x08		  /* Printer error */
#define	LP_ERROR_BITS	LP_ERROR+LP_NOPAPER /* Bits on when printer is off */
					  /* or disconnected */

#define LP_STATUS_BITS	LP_ACK + LP_NOPAPER + LP_BUSY + LP_SELECT + LP_ERROR
#define LP_INVERT	LP_ACK + LP_ERROR + LP_BUSY
#define LPBITS		"\20\10Busy\7Acknowledge\6OutOfPaper\5Selected\4Error"
#define LPNEXT(unit)	(unit == NLP - 1 ? 0 : unit+1)

#define LPPRI		(PZERO + 5)
#define LPLOWAT		50
#define LPHIWAT		300
#define LPMAXTIME	20	/* Set timeout to 20 seconds */
#define LPUNIT(dev)		(minor(dev) >> 3)
#define LPSTATUS(lpaddr)	((lpaddr->stat & LP_STATUS_BITS) ^ LP_INVERT)
#define LPSPL() 		_spl3()


struct lpdevice {
        u_char data;			  /* Data register    */
        u_char stat;			  /* Status  register */
        u_char cmd;			  /* Command register */
};

struct lp_softc {
        struct clist sc_outq;
        int sc_state;
        int sc_timer;
} lp_softc[NLP];

/* bits for state */
#define LP_OPEN		0x01		  /* Device is open */
#define LP_ASLEEP	0x02		  /* Awaiting draining of printer */
#define LP_ACTIVE	0x04		  /* Device is active */
#define LP_DEV_ERROR	0x08		  /* Device error detected */
#define LP_TIMER_ON	0x10		  /* Lpwatch timer is on */
#define LPSTATEBITS	"\20\1Open\2Asleep\3Active\4Error\5TimeoutOn"
#if	ROMP_DEBUG
u_char lpdebg2 = 0;
u_char lpdebug = 0;
#endif 	ROMP_DEBUG

lpprobe(lpaddr)
        register struct lpdevice *lpaddr;
{
/* Don't try to generate interupt. Printer must be on to do so */
        return (PROBE_NOINT); 
}

lpattach(iod)
        register struct iocc_device *iod;
{
        register struct lpdevice *lpaddr;

        lpaddr = (struct lpdevice *)iod->iod_addr;
        lpreset(lpaddr);
}


lpopen(dev, flag)
        dev_t dev;
        int flag;
{
        register int unit = LPUNIT(dev);
        register struct lpdevice *lpaddr;
        register struct lp_softc *sc = &lp_softc[unit];
        register struct iocc_device *iod = lpdinfo[unit];

        if (unit >= NLP || iod == 0 || iod->iod_alive == 0) {
                return (ENODEV);
        }
        lpaddr = (struct lpdevice *)iod->iod_addr;
        if ((sc->sc_state & LP_OPEN) == 0) {
                sc->sc_state |= LP_OPEN;
                return (0);
        }
                DEBUGF(lpdebug,printf("\nLP: PRINTER %d ALREADY OPENED\n", unit););
        return (EBUSY);
}


lpclose(dev, flag)
        dev_t dev;
        int flag;
{
        register struct lpdevice *lpaddr = (struct lpdevice *)lpdinfo[LPUNIT(dev)]->iod_addr;
        register struct lp_softc *sc = &lp_softc[LPUNIT(dev)];
        int s;

        s = LPSPL();
        DEBUGF(lpdebg2,printf("Close: State=0x%b \n",sc->sc_state,LPSTATEBITS););
        while ((sc->sc_state & LP_ACTIVE) && ((sc->sc_state & LP_DEV_ERROR) == 0)) {
                sc->sc_state |= LP_ASLEEP;
                DEBUGF(lpdebug,printf("\nLP: LPCLOSE() SLEEPING UNTIL PRINTER DRAINED\n"););
                sleep((caddr_t)sc, LPPRI);
        }
        DEBUGF(lpdebg2,printf("Close: State=0x%b \n",sc->sc_state,LPSTATEBITS););
        sc->sc_state &= ~LP_OPEN;
        if (sc->sc_state & LP_DEV_ERROR) {		/* Clean up if error */
                while ((getc(&sc->sc_outq)) >= 0);			/* flush the queue */
                sc->sc_state &= ~LP_ACTIVE & ~LP_DEV_ERROR;  /* tidy up the states */
                lpaddr->cmd &= ~LP_INTR_ENABLE;		     /* No more messages */
        }
        splx(s);
        DEBUGF(lpdebug,printf("\nLP: LPCLOSE() PRINTER %d NOW CLOSED\n", LPUNIT(dev)););
}


lpwrite(dev, uio)
        dev_t dev;
        register struct uio *uio;
{
        register int unit = LPUNIT(dev);
        register struct lp_softc *sc = &lp_softc[unit];
        register int c;
        int s;

        while (uio->uio_resid != 0) {
                if ((c = uwritec(uio)) == -1) {
                        return (EFAULT);
                }
                DEBUGF(lpdebug > 1,printf("\nLP: RECIEVED CHARACTER FROM USER 0x%x ", c););
                s = LPSPL();
                while (sc->sc_outq.c_cc >= LPHIWAT) {
                        DEBUGF(lpdebug > 1,printf("\nLP: REACHED HIGH WATER MARK...\n"););
                        lpstart(unit);
                        sc->sc_state |= LP_ASLEEP;
                        sleep((caddr_t)sc, LPPRI);
                }
                DEBUGF(lpdebug > 1,printf("\nLP: PUTTING CHARACTER ON QUEUE 0x%x", c););
                while (putc(c, &sc->sc_outq))
                        sleep((caddr_t)&lbolt, LPPRI);
                lpstart(unit);
                splx(s);
        }
        DEBUGF(lpdebug > 1,printf("\nLP: NO MORE CHARS FROM USER... DRAINING QUEUE\n"););
                return(0);
}


lpstart(unit)
{
        register struct lpdevice *lpaddr = (struct lpdevice *)lpdinfo[unit]->iod_addr;
        register struct lp_softc *sc = &lp_softc[unit];

        if (sc->sc_state & LP_ACTIVE) {
                return;
        }
        DEBUGF(lpdebug > 1,printf("\nLP: STARTING PRINTER...\n"););
        sc->sc_state |= LP_ACTIVE;
        lpoutput(unit);
}

/*
 * Lp interupt routine. Since two of the printers come in at the same interupt 
 * level it is necessary to check which board actually generated the interupt.
 * To maintain fairness, the board with was last serviced will be the board 
 * that is check last in each round.
 */

int lpnextu = 0;

lpintr()
{
        register int unit;
        register struct lpdevice *lpaddr;
        register int lpfirst = 0;

        for (unit = lpnextu; ((lpfirst == 0) || (unit != lpnextu)); unit = LPNEXT(unit)) {
                lpfirst++;			/* first loop is done */
                if (lp_softc[unit].sc_state & LP_ACTIVE) {
                        lpaddr = (struct lpdevice *)lpdinfo[unit]->iod_addr;
                        if (( LPSTATUS(lpaddr) & LP_BUSY) == 0 ) {
#ifdef NLP > 1
                                lpnextu = LPNEXT(unit);
#endif NLP
                                return(lpoutput(unit));
                        }
                 }
        }
        return(1);
}

lpoutput(unit)
{
        register struct lp_softc *sc = &lp_softc[unit];
        register struct lpdevice *lpaddr = (struct lpdevice *)lpdinfo[unit]->iod_addr;
        register int lpchar;
        register int lpstat;
        int lpunit;

        lpaddr->cmd &= ~LP_INTR_ENABLE;
        while ((((lpstat=LPSTATUS(lpaddr)) & LP_BUSY) == 0) && ((lpstat & LP_ERROR_BITS) == 0)) {
                sc->sc_timer=0;			/* did not time out */
                sc->sc_state &= ~LP_DEV_ERROR;
                if ((lpchar = getc(&sc->sc_outq)) >= 0) {
                        DEBUGF(lpdebug > 1,{
                                printf("\nLP: LPINTR() GOT ");
                                printf("CHAR 0x%x OFF QUEUE", lpchar);
                        });
                        lpaddr->data = lpchar;
                        DELAY(1);
                        lpaddr->cmd = LP_STROBE_HIGH;
                        DELAY(1);
                        lpaddr->cmd = LP_STROBE_LOW;
                        DELAY(1);

                } else {
                        DEBUGF((lpdebug == 1),{
                                printf("\nLP: LPINTR() NO MORE ");
                                printf("CHARS ON QUEUE\n");
                        });
                        break;
                }
        }
        if (sc->sc_outq.c_cc > 0) {
                lpaddr->cmd |= LP_INTR_ENABLE;
        }
        if (((lpstat & LP_ERROR_BITS) != 0) || (sc->sc_state & LP_DEV_ERROR)) {
                lpdiagnose(unit,lpstat);        /* find error type and notify user */
                sc->sc_timer=0;			/* Kick lpoutout again in 20 sec */
        }
        if ((sc->sc_outq.c_cc <= LPLOWAT) && (sc->sc_state & LP_ASLEEP)); {
                DEBUGF(lpdebug,{
                        printf("\nLP: LPINTR() WAKEUP UPPER ");
                        printf("HALF OF DRIVER...\n");
                });
                sc->sc_state &= ~LP_ASLEEP;
                wakeup((caddr_t)sc);	  /* top half should go on */
        }
        if (sc->sc_outq.c_cc <= 0) {
                DEBUGF(lpdebug,{
                        printf("\nLP: LPINTR() ALL DONE...  ");
                        printf("INDICATE PRINTER INACTIVE\n");
                });
                sc->sc_state &= ~LP_ACTIVE;
                if (sc->sc_state & LP_ASLEEP) {
                        sc->sc_state &= ~LP_ASLEEP;
                        wakeup((caddr_t)sc);
                }
                for (lpunit=LPNEXT(unit);lpunit != unit;lpunit=LPNEXT(lpunit))
                        if (lp_softc[lpunit].sc_state & LP_ACTIVE) {
                                lpnextu = LPNEXT(lpunit);
                                (void) lpoutput(lpunit);
                        }
                return (0);
        }
        if ((sc->sc_state & LP_TIMER_ON) == 0) {
                sc->sc_state |= LP_TIMER_ON;
                timeout(lpwatch,(caddr_t) unit,hz);
        }
        return (0);
}

lpdiagnose (unit,error)
{
        if (error & LP_NOPAPER)
                printf ("lp%d: Out of paper.\n",unit);
        else if ((error & LP_SELECT) == 0)
                printf ("lp%d: Offline.\n",unit);
        else
                printf ("lp%d: Unknown printer error, status=0x%b.\n",unit,error,LPBITS);
}


lpwatch(unit)
{
        register struct lp_softc *sc = &lp_softc[unit];
        register struct lpdevice *lpaddr = (struct lpdevice *)lpdinfo[unit]->iod_addr;
        register int s=LPSPL();

        if (lpaddr->cmd & LP_INTR_ENABLE) {
            if (sc->sc_timer++ >= LPMAXTIME) {
                sc->sc_state |= LP_DEV_ERROR;
                sc->sc_state &= ~LP_TIMER_ON;
                lpoutput(unit);
                splx(s);
                return;
            }
            timeout(lpwatch,(caddr_t) unit,hz);

        } else
                sc->sc_state &= ~LP_TIMER_ON; 
        splx(s);
}

lpreset(lpaddr)
        register struct lpdevice *lpaddr;
{

        lpaddr->cmd = LP_RESET_LOW;
        DELAY(100);		/* 100 uS delay */
        lpaddr->cmd = LP_RESET_HIGH;
}
#endif
