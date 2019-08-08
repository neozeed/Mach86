#ifndef	_Pager
#define	_Pager	       1

/* Module Pager */
/* 
 * WARNING FROM MATCHMAKER
 */
#include "AccentType.h"

 extern void InitPager () ;

#include "mach_types.h"

 extern void pager_init ( );

 extern boolean_t	pager_read ( );

 extern boolean_t	pager_write ( );

#endif	_Pager
