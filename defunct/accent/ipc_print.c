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
/* ************************************************************************ *\
 *									    *
 * Package:	IPC							    *
 *									    *
 *									    *
 * File:	print.c							    *
 *									    *
 * Abstract:								    *
 *	Print out IPC Structure and various error values symbolically.
 *									    *
 * Author:	Robert V. Baron						    *
 *		Copyright (c) 1984 by Robert V. Baron			    *
 *									    *
 * History:	Created Nov/30/84					    *
 *									    *
\* ************************************************************************ */

/*CMU:	%M%	%I%	%G%	*/

#include "../sync/mp_queue.h"
#include "../accent/accent.h"
#include "../accent/acc_errorstrs.h"

printmsg(msgp)
register ptrMsg msgp;
{
	if (!msgp->MsgType)
		printf("msg: %d=>%d #%d %s %s L=%d+%d\n",
			msgp->LocalPort, msgp->RemotePort,
			msgp->ID,
			"NormalPri",
			msgp->SimpleMsg ? "Simple" : "Complex",
			sizeof (Msg), msgp->MsgSize-sizeof (Msg));
	else
		printf("msg: %s \"%s\" #%d %s %d=>%d L=%d+%d\n",
			"EmergPri",
			kernel_msgid_strs[msgp->ID-M_KernID], msgp->ID,
			msgp->SimpleMsg ? "Simple" : "Complex",
			msgp->LocalPort, msgp->RemotePort,
			sizeof (Msg), msgp->MsgSize-sizeof (Msg));
	if (msgp->MsgSize == sizeof (Msg)) {
		/* printdescriptor has been designed so that if it gets called
		   as printdescriptor(x,x), it will try to print one descriptor
		   thus you don't really have to know how big a descriptor is 
		   to print it.  Unfortunately printmsg now has to be smart, if
		   there is no message */
		printf("NO DATA IN MESSAGE\n");
	} else {
		printf("DESCRIPTOR: START\n");
		printdescriptor(&((char *) msgp)[sizeof (Msg)],
				&((char *) msgp)[msgp->MsgSize]);
		printf("DESCRIPTOR: END\n");
	}
}

printdescriptor(saddr, endaddr)
register char *saddr, *endaddr;
{
register char *base = saddr;
register TypeType *tp;
register int num;
int elts, ts;
int type;

 do {
	tp = (TypeType *) saddr;
	if ( tp->LongForm) {
			register LongTypeType *ltp = (LongTypeType *) tp;
		type = printlongtype(ltp);
		elts = ltp->NumObjects;
		ts = ltp->TypeSizeInBits;
		saddr +=  sizeof (LongTypeType);
	} else {
		type = printipctype(tp);
		ts = tp->TypeSizeInBits;
		elts = tp->NumObjects;
		saddr += sizeof (TypeType);
	}

	if (endaddr - saddr > 100) {
		printf("    MSG TOO BIG TOO PRINT\n");
		return;
	}
	if (type < 0 || !*(int *) tp) {
		printf("    TILT!! TypeTypeMacro==0\n");
		return;
	}
	num = ((elts * ts) + 7) >> 3;

	if ( tp->InLine) {
		printf("    Inline ");
		printdata(type, saddr, num, 1);
		saddr += ( (num + 3) & (~0x3) );
	} else {
		printf("    Ptr to ");
		printdata(type, * (int *) saddr, num, 0);
		saddr += sizeof (Pointer);
	}
 } while (saddr < endaddr);
}

printdata(typename, addr, size, inline)
int *addr;
{

 if (size > 100) {
	printf("    MSG TOO BIG TOO PRINT\n");
	size = 20;
 }
 do {
	switch (typename) {
	case TypePtOwnership:
	case TypePtReceive:
	case TypePtAll:
	case TypePt:
		printf("port#%d (L%d)\n", *addr, size);
		size -= 4; addr += 1;
		break;
	case TypeLong:
		printf("%d(0x%x[@%x][L=%d])\n", *addr, *addr, addr, size);
		size -= 4; addr += 1;
		break;
	case TypeShort:
		printf("%d(0x%x[@%x][L=%d])\n",
			0xffff & *addr, 0xffff & *addr, addr, size);
		size -= 2; addr = (int *) ( ((short *) addr) + 1);
		break;
	case TypeByte:
		printf("%d(0x%x[@%x][L=%d])\n",
			0xff & *addr, 0xff & *addr, addr, size);
		size -= 2; addr = (int *) ( ((char *) addr) + 1);
		break;
	case TypeString:
	case TypeChar:
		printf("\"%s\" (L%d)[@%x]\n", addr, size, addr);
		size -= size;
		break;
	case TypeBoolean:
	case TypeReal:
	case TypePStat:
	case TypeSegID:

		printf("%d(0x%x[@%x])\n", *addr, *addr, addr);
		size = 0;
		break;
	}
	if (size > 0) {
		printf("\t   ");
	}
 } while (size > 0);
}

printipctype(typep)
register TypeType *typep;
{
int type;

	type = typep->TypeName;
	if (type < 0 || type > (sizeof type_strs / sizeof (char *)))
		return -1;

	printf("    TypeType: %s Size = %d No. = %d Inline = %d Long = %d DeAl = %d\n",
		type_strs[type],
		typep->TypeSizeInBits,
		typep->NumObjects,
		typep->InLine,
		typep->LongForm,
		typep->Deallocate);
	return type;
}

printlongtype(typep)
register LongTypeType *typep;
{
register int type;

	type = typep->TypeName;
	if (type < 0 || type > (sizeof type_strs / sizeof (char *)))
		return -1;

	printf("    LongTypeType: %s Size = %d No. = %d Inline = %d Long = %d DeAl = %d\n",
		type_strs[type],
		typep->TypeSizeInBits,
		typep->NumObjects,
		typep->InLine,
		typep->LongForm,
		typep->Deallocate);
	return type;

}

printmsgheader(msgp)
ptrMsg msgp;
{
	if (!msgp->MsgType || msgp->ID < M_KernID || msgp->ID > M_KernIDEnd)
		printf("msg: %d=>%d #%d %s %s L=%d+%d\n",
			msgp->LocalPort, msgp->RemotePort,
			msgp->ID,
			(!msgp->MsgType) ? "NormalPri" : "EmergPri",
			msgp->SimpleMsg ? "Simple" : "Complex",
			sizeof (Msg), msgp->MsgSize-sizeof (Msg));
	else
		printf("msg: %s \"%s\" #%d %s %d=>%d L=%d+%d\n",
			"EmergPri",
			kernel_msgid_strs[msgp->ID-M_KernID], msgp->ID,
			msgp->SimpleMsg ? "Simple" : "Complex",
			msgp->LocalPort, msgp->RemotePort,
			sizeof (Msg), msgp->MsgSize-sizeof (Msg));
}

print_error(str, arg)
{
	if (arg <= AccErrEnd && arg > AccErr) {
		arg -= AccErr;
		printf("%s: \"%s\" error#%d\n",
			str, accent_errors[arg], arg+AccErr);
	} else 	if (arg <= MchmkErrEnd && arg > MchmkErr) {
		arg -= MchmkErr;
		printf("%s: \"%s\" condition#%d\n",
			str, matchmaker_errors[arg], arg+MchmkErr);
	} else 	if (arg <= M_KernIDEnd && arg > M_KernID) {
		arg -= M_KernID;
		printf("%s: \"%s\" Kernel ID#%d\n",
			str, kernel_msgid_strs[arg], arg+M_KernID);
	} else
		printf("%s: (# OUT OF BOUNDS) error#%d\n", str, arg);
}
