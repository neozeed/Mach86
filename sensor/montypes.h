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
#define EVENT_LIMIT 256
#ifdef SHORTALIGN
typedef char *mon_string;
#else
typedef short *mon_string;
#endif

struct  mon_cmd
	{
        char	type,
	        length;
	};

struct mon_pevt
	{
	struct mon_cmd	cmd;
	short		eventnumber,
			performer;
	long		object;
	short		initiator;
	long		timestamp;
	short		fields[EVENT_LIMIT];
	};
typedef struct mon_pevt mon_putevent;

struct mon_erec
	{
	struct mon_cmd	cmd;
	long		val;
	};
typedef struct mon_erec mon_errrec;

struct mon_gevt
	{
	struct mon_cmd	cmd;
	short		req_length,
			*acct_buf_ptr; /* This is a buffer in user's area */
	};
typedef struct mon_gevt mon_getevent;

struct mon_request
	{
	short	   targetpid,
		   eventnumber,
		   enablevalue;
	};

struct mon_preq
	{
	struct mon_cmd	   cmd;
	struct mon_request req;
	};
typedef struct mon_preq mon_putreq;
typedef struct mon_preq mon_getreq;

struct mon_command
	{
	union	{
		struct mon_cmd cmd; /* other cmds only have first 2 fields */
		mon_putevent   pevt;
		mon_getevent   gevt;
		mon_putreq     preq;
		} u_event;
	};
typedef struct mon_command mon_command;
