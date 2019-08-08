/*
 * Copyright (c) 1982, 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)cpudata.c	7.1 (Berkeley) 6/5/86
 */

#include "pte.h"

#include "param.h"

#include "cpu.h"
#include "nexus.h"
#include "ioa.h"
#include "../vaxuba/ubareg.h"

/*
 * Initialization of per-cpu data structures.
 */

/*
 * These are the (fixed) addresses of the (last 8k bytes of)
 * unibus memory for each of the possible unibus adapters.  Note that the
 * unibus memory addresses are actually indexed by the unibus adapter type code.
 */
#if VAX8600
caddr_t umaddr8600a[4] = {
	(caddr_t) UMEMA8600(0),	(caddr_t) UMEMA8600(1),
	(caddr_t) UMEMA8600(2),	(caddr_t) UMEMA8600(3),
};
caddr_t umaddr8600b[4] = {
	(caddr_t) UMEMB8600(0),	(caddr_t) UMEMB8600(1),
	(caddr_t) UMEMB8600(2),	(caddr_t) UMEMB8600(3),
};
#endif
#if VAX780
caddr_t	umaddr780[4] = {
	(caddr_t) UMEM780(0), (caddr_t) UMEM780(1),
	(caddr_t) UMEM780(2), (caddr_t) UMEM780(3)
};
#endif
#if VAX750
caddr_t	umaddr750[2] = {
	(caddr_t) UMEM750(0), (caddr_t) UMEM750(1),
};
#endif
#if VAX730
caddr_t	umaddr730[1] = {
	(caddr_t) UMEM730
};
#endif
#if VAX630
caddr_t	umaddr630[1] = {
	(caddr_t) UMEM630
};
#endif

/*
 * Information to patch around the stupidity of configuration
 * registers not returning types on some of the processors.
 */
#if VAX750
short	nexty750[NNEX750] = {
	NEX_MEM16,	NEX_MEM16,	NEX_MEM16,	NEX_MEM16,
	NEX_MBA,	NEX_MBA,	NEX_MBA,	NEX_MBA,
	NEX_UBA0,	NEX_UBA1,	NEX_ANY,	NEX_ANY,
	NEX_ANY,	NEX_ANY,	NEX_ANY,	NEX_ANY
};
#endif
#if VAX730
short	nexty730[NNEX730] = {
	NEX_MEM16,	NEX_ANY,	NEX_ANY,	NEX_ANY,
	NEX_ANY,	NEX_ANY,	NEX_ANY,	NEX_ANY,
};
#endif

#if VAX8600
struct nexusconnect sbi8600[] = {
	{ NNEX8600, NEXA8600, umaddr8600a, NBDP8600, 1, 0 },
	{ NNEX8600, NEXB8600, umaddr8600b, NBDP8600, 1, 0 },
};
struct iobus io8600[] = {
	{ IO_ABUS, IOA8600(0), IOAMAPSIZ, (caddr_t)&sbi8600[0] },
	{ IO_ABUS, IOA8600(1), IOAMAPSIZ, (caddr_t)&sbi8600[1] },
};
#endif
#if VAX630
short	nexty630[NNEX630] = {
	NEX_UBA0,
};
#endif

#if VAX780
struct nexusconnect sbi780 = {
	NNEX780, NEX780, umaddr780, NBDP780, 1, 0,
};
struct iobus io780[] = { IO_SBI780, 0, 0, (caddr_t)&sbi780 };
#endif

#if VAX750
struct nexusconnect cmi750 = {
	NNEX750, NEX750, umaddr750, NBDP750, 0, nexty750,
};
struct iobus io750[] = { IO_CMI750, 0, 0, (caddr_t)&cmi750 };
#endif

#if VAX730
struct nexusconnect xxx730 = {
	NNEX730, NEX730, umaddr730, NBDP730, 0, nexty730,
};
struct iobus io730[] = { IO_XXX730, 0, 0, (caddr_t)&xxx730 };
#endif
#if VAX630
struct nexusconnect xxx630 = {
	NNEX630, NEX630, umaddr630, NBDP630, 0, nexty630,
};
struct iobus io630[] = { IO_QBUS, 0, 0, (caddr_t)&xxx630 };
#endif


struct percpu percpu[] = {
#if VAX8600
	{ VAX_8600, 6, 2, io8600 },
#endif
#if VAX780
	{ VAX_780, 2, 1, io780 },
#endif
#if VAX750
	{ VAX_750, 1, 1, io750 },
#endif
#if VAX730
	{ VAX_730, 1, 1, io730 },
#endif
#if VAX630
	{ VAX_630, 2, 1, io630 },
#endif
	0,
};
