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
/*
 * C_Types.h
 *
 * Common type definitions for C.
 *
 * Michael W. Young, Mark Hjelm
 *
 * Copyright (C) 1985 Carnegie-Mellon University
 *
 * History:
 *
 *  24-Oct-85	Michael B. Jones
 *		Added Char_To_P, PascalString for Cris Mooney.  Used by ptc.
 *		Removed caddr_t, since it conflicts with unix definition.
 *
 *  27-Jun-85	Mark Hjelm
 *		Created common version: Split off from AccentType.h.
 *
 */

#ifndef _C_Types
#define _C_Types

#ifdef	KERNEL
#include "../accent/vax/machdep.h"
#else	KERNEL
#include <accent/vax/machdep.h>
#endif	KERNEL

/*
 * Simple Types.
 */

typedef char		int8;
typedef short		int16;
typedef long		int32;

typedef unsigned char	byte;
typedef Unit_Type	*Pointer;	/* Native pointer on machine */

typedef	byte		byte_bool;	/* Booleans where size matters */
typedef	unsigned short	short_bool;
typedef	unsigned long	long_bool;

typedef int		boolean;	/* Size is dependent upon machine */

/*
 * Definition of Dynamic "String" type.
 *
 * There are currently two implementations:
 * 
 * The newest provides:
 * 
 *     PascalString	- Macro which returns a string structure
 *     DefineString	- Macro which defines a string type with a given
 *			  maximum length
 *     StringLen	- Macro which returns the length of a string
 *     StringCopy	- Macro which copies a string into another (possibly
 *			  with a different maximum length)
 *     P_To_C		- Macro which converts a dymnamic string to a Null-
 *			  terminated C string
 *     C_To_P		- Macro which does the opposite
 *
 * The older version supplies only DefineString.
 *
 * To access the new string stuff, all modules must be compiled with a
 * -DNewStrings switch.
 *
 */

#ifdef NewStrings

#define PascalString(MaxLength) \
    union { \
	unsigned char   len; \
	char            Char[MaxLength+1]; \
	short		Align; \
    }


#define DefineString(TypeName, MaxLength) \
    typedef PascalString(MaxLength) TypeName


#define StringLen(str)	((str).len)


#define StringCopy(str1,str2) \
	{ \
	    register int _i; \
	    for (_i=1; _i<=StringLen(str2); _i++) \
		(str1).Char[_i] = (str2).Char[_i]; \
	    (str1).len = StringLen(str2); \
	}

/*
 * Macros to convert a p string to c string and vesa versa
 */

#define P_To_C(cstr,pstr) \
	{ \
	    register int _i; \
	    for(_i=1; _i<=StringLen(pstr); _i++) \
		(cstr)[_i-1] = (pstr).Char[_i]; \
	    (cstr)[_i-1] = '\0'; \
	}

#define C_To_P(pstr,cstr) \
	{ \
	    register int _i; \
	    for (_i=0; (cstr)[_i]!='\0'; _i++) \
		(pstr).Char[_i+1] = (cstr)[_i]; \
	    (pstr).len =  _i; \
	}

#define Char_To_P(pstr,chr) \
	{ \
	    (pstr).Char[1] = (chr); \
	    (pstr).len = 1; \
	}

#else NewStrings

#define         DefineString(NameOfType, SizeOfString) \
typedef         union \
			{ \
			unsigned char   _StrSize; \
			char            _StringChars[SizeOfString+1]; \
			short		Align; \
			} \
				NameOfType

#endif NewStrings


DefineString(String80, 80);
DefineString(String255, 255);

typedef String80   String;


#endif _C_Types
