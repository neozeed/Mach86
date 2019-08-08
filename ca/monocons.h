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
/* $Header: monocons.h,v 4.6 85/08/31 13:03:03 webb Exp $ */
/* $Source: /ibm/acis/usr/sys_ca/ca/RCS/monocons.h,v $ */

#if !defined(lint) && !defined(LOCORE) && defined(RCS_HDRS)
static char *rcsidsari = "$Header: monocons.h,v 4.6 85/08/31 13:03:03 webb Exp $";
#endif

/* sari.h -- standalone constants for the IBM RI */

#define CTL(x) ('x'&037)	/* get a control character */
#define min(x,y) x < y ? x : y
#define max(x,y) x > y ? x : y

#define ROMP_BASE 0xf0000000	       /* I/O base address */

#define in(port) * (( char *) (ROMP_BASE + (port)))
#define out(port,value) in(port) = value
 /* output a PC word (= short) */
#define inw(port) * (( short *) (ROMP_BASE + (port)))
#define outw(port,value) inw(port) = value

#define CRT_1 0x3b8		       /* crt port 1 */
#define PUT_SCR_REG(reg,value) out(0x3b4,reg); out(0x3b5,value);

#ifdef CNDEBUG
#define LOCAL			       /* make it external */
#else
#define LOCAL	static		       /* make it local */
#endif
#ifdef SBPROTO
#define    _CDB        0x082A	       /* Hex Display Register  */
#endif

#ifdef SBMODEL
#define    _CDB        0x8ce0	       /* Hex Display Register  */
#endif
/* $Header: monocons.h,v 4.6 85/08/31 13:03:03 webb Exp $ */
/* $Source: /ibm/acis/usr/sys_ca/ca/RCS/monocons.h,v $ */

#if !defined(lint) && !defined(LOCORE) && defined(RCS_HDRS)
static char *rcsidkeyboard = "$Header: monocons.h,v 4.6 85/08/31 13:03:03 webb Exp $";
#endif


#if defined(SBMODEL) && !defined(DEBOUNCE)
#define DEBOUNCE	1
#endif

#define NONE	0377			  /* code for no code defined */
#define END	NONE, NONE		  /* no code for end */
#define E ESC_FLAG +

#define ESC_MARK 0x100
#define ALT_MARK 0x80

int getchar_timeout;			  /* used to provide 'default' input */
char getchar_char;			  /* the character to return */


#ifdef SBPROTO
#define KYB_CNTIR	0x0cc2a		  /* 8255 control port read */
#define KYB_CNTIW	0x0ec2a		  /* 8255 control port write */
#define KYB_READ	0x08c2a		  /* 8255 data read */
#define KYB_WRITE	0x00c2a		  /* 8255 & command data */
#define KYB_RESET	0x0082d		  /* 8255 & 8051 reset */
#define CDB		0x0082a		  /* hex display */
#define _reboot start			  /* restart our code */
#endif

#ifdef SBMODEL
#define KYB_CNTIW 0x8407		  /* 8255 control port write */
#define KYB_CNTIR 0x8406		  /* 8255 control port read */
#define KYB_READ  0x8404		  /* 8255 data read */
#define KYB_WRITE 0x8400		  /* 8255 data write */
#define KBD_ADAPTOR_RESET 0xFB		  /* adaptor reset */
#define KBD_ADAPTOR_RELEASE 0x04	  /* adaptor release */
#define CRRB	0x8c60			  /* conponent reset reg B */
#endif

#define KYB_IIC_MASK	0x0F		  /* Interrupt Ident Code Mask */
#define KYB_INT		0x08		  /* Interrupt Received bit */
#define KYB_INFO	0x08		  /* Information Interrupt */
#define KYB_DATA	0x09		  /* Keyboard data int */
#define UART_DATA	0x0A		  /* Uart data int */
#define KYB_REQD	0x0B		  /* Return requested byte int */
#define UART_BLK	0x0C		  /* Block Transfer int */
#define KYB_UNASS	0x0D		  /* Unassigned */
#define KYB_SR		0x0E		  /* Softr Reset/Completion code */
#define KYB_EC		0x0F		  /* Detected an error condition */
#define KYB_BUSY	0x10		  /* Keyboard busy bit           */

#define IID_MASK	0x07		  /* mask for Ident code */
#define KBD_INFO	0x00		  /* information */
#define KBD_DATA	0x01		  /* value for data present */
#define KBD_UART	0x02		  /* byte from UART */
#define KBD_REQ		0x03		  /* returned requested byte */
#define KBD_BLOCK	0x04		  /* block request */
#define KBD_RESERVED	0x05		  /* reserved */
#define KBD_SELF_TEST	0x06
#define KBD_ERROR	0x07
#define KBD_TIMEOUT	0x100		  /* if we have timed out */

/*    Adapter commands                                               */

#define       EXTCMD      0x00		  /*  Extended command select  */
#define	        READ_STAT 0x12		  /* Read status byte from shared mem */
#define		SP_MEDIUM	0x42	  /*  set speaker medium	   */
#define       UARTCMD     0x04		  /*  Command to uart          */
 /*  These cmds return 2 datablocks                         */
#define         RSMOUSE   0x01		  /*  Reset  mouse             */
#define         QYMOUSE   0x73		  /*  Query  mouse             */
#define       UARTCNT     0x03		  /*  Control to uart          */
#define         MSTRANS   0x08		  /*  Enable mouse for transm. */
#define       KYBDCMD     0x01		  /*  Command to kybd          */
#define         KRESET    0xFF		  /*  Keyboard reset           */
#define         KDEFDS    0xF5		  /*  Kybd default disable     */
#define         KSCAN     0xF4		  /*  Kybd start scanning      */
#define         READID    0xF2		  /*  Read kybd id             */
#define         SETLED    0xED		  /*  Set kybd LEDs            */
#define       WRRAM       0x10		  /*  Write shr RAM            */
#define       RDRAM       0x00		  /*  Read  shr RAM            */
#define       RD1C        0x1C		  /*  Read shr RAM 0x1C.     */
#define       SETFCHI     0x08		  /*  Set freq counter hi byte */
#define         FCHI      0x01		  /*  Hi frequency byte        */
#define       SETFCLO     0x09		  /*  Set freq counter lo byte */
#define         FCLO      0xAC		  /*  Lo frequency byte        */
#define       SPKRON      0x02		  /*  Turn on speaker          */
#define         SPTIME    0X04		  /*  Speaker duration         */
#define       ENKYBD      0x3B		  /*  Enable kybd mode bit 11  */
#define       ENUART      0x3C		  /*  Enable UART mode bit 12  */
#define       RDVERS      0xE0		  /*  Read version             */
#define       DSLOCK      0x2D		  /*  Disable keylock, bit 13  */
#define       CMDREJ      0x7F		  /*  Command reject received  */
#define       POER        0xFF		  /*  Power-on error report.   */
#define       INFO        0x00		  /*  Int id for information   */


#define KYB_CONFIG	0xc3		  /* 8255 configuration value */

/* Keyboard leds */
#define	      NUM_LED     0x04		  /* Num lock led             */
#define	      CAPS_LED    0x02		  /* Caps lock led            */
#define	      SCROLL_LED  0x01		  /* Scroll lock led          */

/*
 * following bits are in KYB_CNTIR
 */
#define KYB_IBF		0x20		  /* IBF (input buffer full?) */
#define KYB_OBF		0x80		  /* OBF (output buffer full?) */
#define CMD(cmd,data) cmd + (data <<8)	/* pack data and command */

int debug_key;
#ifdef CNDEBUG
#define TRACEF(x) if (cndebug) printf x			/* cndebugging stuff */
#else
#define TRACEF(x) 					/* cndebugging stuff */
#endif

static int last_char, key_count;
static last_scan;
static int state, make_break;
static char kbd_leds_state = 0;
int _init_kbd;				  /* flag to determine if init_kbd done */



#define SHIFT1	0x12
#define SHIFT2	0x59
#define ALT1	0x19
#define ALT2	0x39
#define BREAK	0xf0
#ifdef PC_KEYBOARD
#define SHIFT_LOCK 0x11		/* used to be control */
#define CNTRL	0x14		/* used to be shift lock */
#else
#define SHIFT_LOCK 0x14		/* key labeled caps lock */
#define CNTRL	0x11		/* key labeled control */
#endif
#define DEBUG_CHAR 0x62
#define PRINT_CHAR 0x57		/* print the screen */
#define ESC_FLAG	0x80	/* generate ESC + character */
#define ALT_FLAG	0x100	/* generate ALT + character */
#define NUM_LOCK 0x76		/* numeric lock */
#define SWITCH_CHAR 0x5f	/* scroll lock */
#define CLICK_CHAR 0x76		/* use num lock as it is useless */
#ifdef ESC2_CHAR
#define ESC1_CHAR	(033 + ESC_MARK)
#else
#define ESC1_CHAR	033
#endif
#define MAX_CODES  133
char codes[(MAX_CODES+1) * 2 ]  = {
    NONE,     NONE,		/*   0 0x00	 */
    NONE,     NONE,		/*   1 0x01	 */
    NONE,     NONE,		/*   2 0x02	 */
    NONE,     NONE,		/*   3 0x03	 */
    NONE,     NONE,		/*   4 0x04	 */
    NONE,     NONE,		/*   5 0x05	 */
    NONE,     NONE,		/*   6 0x06	 */
   E 'S',    E 'I',		/*   7 0x07	 f1 */
   '\33',    '\33',		/*   8 0x08	 */
    NONE,     NONE,		/*   9 0x09	 */
    NONE,     NONE,		/*  10 0x0a	 */
    NONE,     NONE,		/*  11 0x0b	 */
    NONE,     NONE,		/*  12 0x0c	 */
    '\t',     '\b',		/*  13 0x0d	 */
     '`',      '~',		/*  14 0x0e	 */
   E 'T',    E 'J',		/*  15 0x0f	 f2 */
    NONE,     NONE,		/*  16 0x10	 */
    NONE,     NONE,		/*  17 0x11	 */
    NONE,     NONE,		/*  18 0x12	 */
     '<',      '>',		/*  19 0x13	 */
    NONE,     NONE,		/*  20 0x14	 */
     'q',      'Q',		/*  21 0x15	 */
     '1',      '!',		/*  22 0x16	 */
   E 'U',    E 'K',		/*  23 0x17	 f3 */
    NONE,     NONE,		/*  24 0x18	 */
    NONE,     NONE,		/*  25 0x19	 */
     'z',      'Z',		/*  26 0x1a	 */
     's',      'S',		/*  27 0x1b	 */
     'a',      'A',		/*  28 0x1c	 */
     'w',      'W',		/*  29 0x1d	 */
     '2',      '@',		/*  30 0x1e	 */
   E 'V',    E 'L',		/*  31 0x1f	 f4 */
    NONE,     NONE,		/*  32 0x20	 */
     'c',      'C',		/*  33 0x21	 */
     'x',      'X',		/*  34 0x22	 */
     'd',      'D',		/*  35 0x23	 */
     'e',      'E',		/*  36 0x24	 */
     '4',      '$',		/*  37 0x25	 */
     '3',      '#',		/*  38 0x26	 */
   E 'W',    E 'M',		/*  39 0x27	 f5 */
    NONE,     NONE,		/*  40 0x28	 */
   '\40',    '\40',		/*  41 0x29	 */
     'v',      'V',		/*  42 0x2a	 */
     'f',      'F',		/*  43 0x2b	 */
     't',      'T',		/*  44 0x2c	 */
     'r',      'R',		/*  45 0x2d	 */
     '5',      '%',		/*  46 0x2e	 */
   E 'P',    E 'N',		/*  47 0x2f	 f6 */
    NONE,     NONE,		/*  48 0x30	 */
     'n',      'N',		/*  49 0x31	 */
     'b',      'B',		/*  50 0x32	 */
     'h',      'H',		/*  51 0x33	 */
     'g',      'G',		/*  52 0x34	 */
     'y',      'Y',		/*  53 0x35	 */
     '6',      '^',		/*  54 0x36	 */
   E 'Q',    E 'O',		/*  55 0x37	 f7 */
    NONE,     NONE,		/*  56 0x38	 */
    NONE,     NONE,		/*  57 0x39	 */
     'm',      'M',		/*  58 0x3a	 */
     'j',      'J',		/*  59 0x3b	 */
     'u',      'U',		/*  60 0x3c	 */
     '7',      '&',		/*  61 0x3d	 */
     '8',      '*',		/*  62 0x3e	 */
   E 'R',    E 'X',		/*  63 0x3f	 f8 */
    NONE,     NONE,		/*  64 0x40	 */
     ',',      '<',		/*  65 0x41	 */
     'k',      'K',		/*  66 0x42	 */
     'i',      'I',		/*  67 0x43	 */
     'o',      'O',		/*  68 0x44	 */
     '0',      ')',		/*  69 0x45	 */
     '9',      '(',		/*  70 0x46	 */
   E 'Y',    E '-',		/*  71 0x47	 f9 */
    NONE,     NONE,		/*  72 0x48	 */
     '.',      '>',		/*  73 0x49	 */
     '/',      '?',		/*  74 0x4a	 */
     'l',      'L',		/*  75 0x4b	 */
     ';',      ':',		/*  76 0x4c	 */
     'p',      'P',		/*  77 0x4d	 */
     '-',      '_',		/*  78 0x4e	 */
   E '<',    E '>',		/*  79 0x4f	 f10 */
    NONE,     NONE,		/*  80 0x50	 */
    NONE,     NONE,		/*  81 0x51	 */
    '\'',      '"',		/*  82 0x52	 */
    NONE,     NONE,		/*  83 0x53	 */
     '[',      '{',		/*  84 0x54	 */
     '=',      '+',		/*  85 0x55	 */
   E '[',    E ']',		/*  86 0x56	 f11 */
    NONE,     NONE,		/*  87 0x57	 */
    NONE,     NONE,		/*  88 0x58	 */
    NONE,     NONE,		/*  89 0x59	 */
    '\r',     '\r',		/*  90 0x5a	 */
     ']',      '}',		/*  91 0x5b	 */
    '\\',      '|',		/*  92 0x5c	 */
    NONE,     NONE,		/*  93 0x5d	 */
   E ',',    E '.',		/*  94 0x5e	 f12 */
    NONE,     NONE,		/*  95 0x5f	 */
   E 'B',    E 'B',		/*  96 0x60	 down arrow */
   E 'D',    E 'D',		/*  97 0x61	 left arrow */
    NONE,     NONE,		/*  98 0x62	 */
   E 'A',    E 'A',		/*  99 0x63	 up arrow */
  '\177',   '\177',		/* 100 0x64	 */
   E 'F',    E 'f',		/* 101 0x65	 end */
    '\b',     '\b',		/* 102 0x66	 */
    '\0',   E '\0',		/* 103 0x67	 ins */
    NONE,     NONE,		/* 104 0x68	 */
     '1',      '1',		/* 105 0x69	 */
   E 'C',    E 'C',		/* 106 0x6a	 right arrow */
     '4',      '4',		/* 107 0x6b	 */
     '7',      '7',		/* 108 0x6c	 */
   E 'E',    E 'e',		/* 109 0x6d	 page down */
   E 'H',    E 'H',		/* 110 0x6e	 home */
   E 'G',    E 'g',		/* 111 0x6f	 page up */
     '0',      '0',		/* 112 0x70	 */
     '.',      '.',		/* 113 0x71	 */
     '2',      '2',		/* 114 0x72	 */
     '5',      '5',		/* 115 0x73	 */
     '6',      '6',		/* 116 0x74	 */
     '8',      '8',		/* 117 0x75	 */
    NONE,     NONE,		/* 118 0x76	 */
     '/',      '/',		/* 119 0x77	 */
    NONE,     NONE,		/* 120 0x78	 */
    '\r',     '\r',		/* 121 0x79	 enter key */
     '3',      '3',		/* 122 0x7a	 */
    NONE,     NONE,		/* 123 0x7b	 */
     '+',      '+',		/* 124 0x7c	 */
     '9',      '9',		/* 125 0x7d	 */
     '*',      '*',		/* 126 0x7e	 */
    NONE,     NONE,		/* 127 0x7f	 */
    NONE,     NONE,		/* 128 0x80	 */
    NONE,     NONE,		/* 129 0x81	 */
    NONE,     NONE,		/* 130 0x82	 */
    NONE,     NONE,		/* 131 0x83	 */
     '-',      '-',		/* 132 0x84	 */
NONE, NONE };
