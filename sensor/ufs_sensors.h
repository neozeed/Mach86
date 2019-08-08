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
/* Sensor macros for the UFS process.
   Generated from kernel.sen on June 14, 1984 (by hand).
   June 16,1985: added macros for char string handling for
	machines that can't fetch from arbitrary boundaries:
	PackStr(ptr)		- mondefs.h
	NotEOS(ptr,last)	- mondefs.h
	type mon_string		- montypes.h
	All of these are dependent on SHORTALIGN
   Contains the following macros:
	OpenSuccessful(mode, initsize)
	NameStart(device,inumber)
	NextComponent(device,inumber,filename)
	INodeCreate(device,inumber)
	INodeDelete(device,inumber)
*/
#ifdef KERNEL
#include "../sensor/mondefs.h"
#include "../sensor/montypes.h"
#ifndef ntohs
#include "../netinet/in.h"
#endif	ntohs
#endif	KERNEL

/* The parameters for OpenSuccessful are:
	  mode :	   mode;	  
	  initsize :	   ip->di_size;	  
*/
#if	NWB_SENS > 0
#define OpenSuccessful(mode,initsize)					\
if (mon_enablevector[0] & 1<<5)						\
    {									\
    if (mon_semaphore++ == 0)						\
	{								\
	if (mon_eventvector_count <					\
		MON_EVENTVECSIZE - 13*2 - sizeof(mon_errrec))		\
	    {								\
	    register mon_putevent *reg_ptr = (mon_putevent *)mon_write_ptr;\
	    register short	*sen_fields = reg_ptr->fields;		\
	    reg_ptr->cmd.type	 = MONOP_PUTEVENT_INT;			\
	    reg_ptr->cmd.length	 = sen_fields+3-(short *)reg_ptr;	\
	    mon_eventvector_count	+= reg_ptr->cmd.length*2;	\
	    reg_ptr->eventnumber	 = (short)5;	/* id of sensor */\
	    reg_ptr->performer 	 = 0;					\
	    reg_ptr->object		 = 0;		/* no object */	\
	    reg_ptr->initiator	 = u.u_procp->p_pid;			\
	    TIMESTAMP(reg_ptr->timestamp);				\
	    *sen_fields++	 = (short)mode;		/* int */	\
	    *(int *)sen_fields	 = (int)initsize;	/* dint */	\
	    sen_fields		+= 2;					\
	    if ( sen_fields > (short *)mon_eventvector_end )		\
	        mon_write_ptr = Wraparound((unsigned char *)sen_fields);\
	    else							\
	        mon_write_ptr = (unsigned char *)sen_fields;		\
	    }								\
	else mon_oflow_count++;						\
   	mon_semaphore--;						\
	}								\
   }	/* end OpenSuccessful */
#else	NWB_SENS > 0
#define OpenSuccessful(mode,initsize)
#endif	NWB_SENS > 0

/* parameters for NameStart are from:
    device :   ip->i_dev	   
    inumber:   ip->i_number
*/
#if	NWB_SENS > 0
#define NameStart(device,inumber)					\
if (mon_enablevector[0] & 1)						\
    {									\
    if (mon_semaphore++ == 0)						\
	{								\
	if (mon_eventvector_count <					\
		MON_EVENTVECSIZE - 12*2 - sizeof(mon_errrec))		\
	    {								\
	    register mon_putevent *reg_ptr = (mon_putevent *)mon_write_ptr;\
	    register short	*sen_fields = reg_ptr->fields;		\
	    reg_ptr->cmd.type	 = MONOP_PUTEVENT_INT;			\
	    reg_ptr->cmd.length	 = sen_fields - (short *)reg_ptr;	\
	    mon_eventvector_count += reg_ptr->cmd.length*2;		\
	    reg_ptr->eventnumber = (short)1;	/* id of sensor */	\
	    reg_ptr->performer	 = 0;					\
	    reg_ptr->object	 = ((short)device<<16) |    /* int */	\
	    			   ((short)inumber&0xffff); /* int */	\
	    reg_ptr->initiator	 = u.u_procp->p_pid;			\
	    TIMESTAMP(reg_ptr->timestamp);				\
	    if ( sen_fields > (short *)mon_eventvector_end )		\
	        mon_write_ptr = Wraparound((unsigned char *)sen_fields);\
	    else							\
	        mon_write_ptr = (unsigned char *)sen_fields;		\
	    }								\
	else mon_oflow_count++;						\
	mon_semaphore--;						\
	}								\
   }	/* end NameStart      */
#else	NWB_SENS
#define NameStart(device,inumber)
#endif	NWB_SENS

/* parameters for NextComponent are from:
    device :   ip->i_dev	   
    inumber:   ip->i_number
    filename:  u.u_dbuf[0..15]
*/
#if	NWB_SENS > 0
#define NextComponent(device,inumber,filename)				\
if (*(mon_enablevector+0) & 1<<2)					\
    {									\
    if (mon_semaphore++ == 0)						\
	{								\
	if (mon_eventvector_count <					\
		MON_EVENTVECSIZE - 260 - sizeof(mon_errrec))		\
	    {								\
	    register mon_putevent *reg_ptr  = (mon_putevent *)mon_write_ptr;\
	    register short	*sen_fields  = reg_ptr->fields;		\
	    register mon_string sen_f_ptr = (mon_string )filename;	\
	    register mon_string sen_f_end = (sen_f_ptr+127*2/sizeof(mon_string));\
	    register short sen_length;					\
	    reg_ptr->eventnumber = (short)2;	/* id of sensor */	\
	    reg_ptr->performer	 = 0;					\
	    reg_ptr->object	 = ((short)device<<16) |    /* int */	\
	    			   ((short)inumber&0xffff);   /* int */	\
	    reg_ptr->initiator	 = u.u_procp->p_pid;			\
	    reg_ptr->timestamp	 = (long)0;	/* nil timestamp */	\
	    do { *sen_fields++	 = PackStr(sen_f_ptr); }		\
	        while (NotEOS(sen_f_ptr,sen_f_end));			\
	    *(sen_fields - 1)	&= ntohs(0xff00);			\
	    sen_length		 = sen_fields - (short *)reg_ptr;	\
	    reg_ptr->cmd.type	 = MONOP_PUTEVENT_INT;			\
	    reg_ptr->cmd.length	 = sen_length;				\
	    mon_eventvector_count += sen_length*2;			\
	    if ( sen_fields > (short *)mon_eventvector_end )		\
	        mon_write_ptr	= Wraparound((unsigned char *)sen_fields);\
	    else							\
	        mon_write_ptr	= (unsigned char *)sen_fields;		\
	    }     /* if still room in vector */				\
	else mon_oflow_count++;						\
	mon_semaphore--;						\
	}								\
    }	/* end NextComponent  */
#else	NWB_SENS > 0
#define NextComponent(device,inumber,filename)
#endif	NWB_SENS > 0

/* parameters for INodeCreate are from:
    device :   ip->i_dev	   
    inumber:   ip->i_number
*/
#if	NWB_SENS > 0
#define INodeCreate(device,inumber)					\
if (*(mon_enablevector+0) & 1<<3)					\
    {									\
    if (mon_semaphore++ == 0)						\
	{								\
	if (mon_eventvector_count <					\
		MON_EVENTVECSIZE - 12*2 - sizeof(mon_errrec))		\
	    {								\
	    register mon_putevent *reg_ptr = (mon_putevent *)mon_write_ptr;\
	    register short	*sen_fields = reg_ptr->fields;		\
	    reg_ptr->cmd.type	 = MONOP_PUTEVENT_INT;			\
	    reg_ptr->cmd.length	 = sen_fields - (short *)reg_ptr;	\
	    mon_eventvector_count += reg_ptr->cmd.length*2;		\
	    reg_ptr->eventnumber = (short)3;	/* id of sensor */	\
	    reg_ptr->performer	 = 0;					\
	    reg_ptr->object	 = ((short)device<<16) |    /* int */	\
	    			   ((short)inumber&0xffff);   /* int */	\
	    reg_ptr->initiator	 = u.u_procp->p_pid;			\
	    TIMESTAMP(reg_ptr->timestamp);				\
	    					/*type */		\
	    if ( sen_fields > (short *)mon_eventvector_end )		\
	        mon_write_ptr = Wraparound((unsigned char *)sen_fields);\
	    else							\
	        mon_write_ptr = (unsigned char *)sen_fields;		\
	    }								\
	else mon_oflow_count++;						\
	mon_semaphore--;						\
	}								\
   }	/* end INodeCreate */
#else	NWB_SENS > 0
#define INodeCreate(device,inumber)
#endif	NWB_SENS > 0

/* parameters for INodeDelete are from:
    device :   ip->i_dev	   
    inumber:   ip->i_number
*/
#if	NWB_SENS > 0
#define INodeDelete(device,inumber)					\
if (*(mon_enablevector+0) & 1<<7)					\
    {									\
    if (mon_semaphore++ == 0)						\
	{								\
	if (mon_eventvector_count <					\
		MON_EVENTVECSIZE - 12*2 - sizeof(mon_errrec))		\
	    {								\
	    register mon_putevent *reg_ptr = (mon_putevent *)mon_write_ptr;\
	    register short	*sen_fields = reg_ptr->fields;		\
	    reg_ptr->cmd.type	 = MONOP_PUTEVENT_INT;			\
	    reg_ptr->cmd.length	 = sen_fields - (short *)reg_ptr;	\
	    mon_eventvector_count += reg_ptr->cmd.length*2;		\
	    reg_ptr->eventnumber = (short)7;	/* id of sensor */	\
	    reg_ptr->performer	 = 0;					\
	    reg_ptr->object	 = ((short)device<<16) |    /* int */	\
	    			   ((short)inumber&0xffff);   /* int */	\
	    reg_ptr->initiator	 = u.u_procp->p_pid;			\
	    TIMESTAMP(reg_ptr->timestamp);				\
	    								\
	    if ( sen_fields > (short *)mon_eventvector_end )		\
	        mon_write_ptr = Wraparound((unsigned char *)sen_fields);\
	    else							\
	        mon_write_ptr = (unsigned char *)sen_fields;		\
	    }								\
	else mon_oflow_count++;						\
	mon_semaphore--;						\
	}								\
   }	/* end INodeDelete */
#else	NWB_SENS > 0
#define INodeDelete(device,inumber)
#endif	NWB_SENS > 0

