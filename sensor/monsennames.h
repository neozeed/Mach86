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
/* monsennames.h
 * The two tables are, respectively, the names of the system sensors
 * and the names of their respective arguments.
 * These tables are used to label the output from the sensors.
 */
static char *monsenname[] = {
	"NameStart",
	"NextComponent",
	"INodeCreate",
	"OpenSuccessful",
	"FileClose",
	"INodeDelete",
	"ReadSensor",
	"WriteSensor",
	"Fork",
	"exec",
	"Send",
	"Receive"
	};
static char *monsenargs[] = {
	"device,inumber",
	"device,inumber,filename",
	"device,inumber",
	"mode,initsize",
	"device,inumber,finalsize",
	"device,inumber",
	"device,inumber,filepos,actualcount",
	"device,inumber,filepos,actualcount"		
	};
