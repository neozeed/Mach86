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
/****************************************************************
 * HISTORY
 * 23-May-86  David Golub (dbg) at Carnegie-Mellon University
 *	Created.
 ****************************************************************/

/*
 *	Paging to/from inodes.
 *
 *	Objects currently supported are:
 *
 *		text files: vm_pager_text.
 *		swap files: vm_pager_inode.
 */

/*
 *	inode pager
 */

boolean_t	inode_pagein();
boolean_t	inode_pageout();
boolean_t	inode_dealloc();
boolean_t	inode_alloc();
boolean_t	inode_pager_setup();

/*
 *	text pager */

#define text_pagein	inode_pagein
#define text_pageout	inode_pageout	/* better not! */

boolean_t	text_dealloc();
boolean_t	text_pager_setup();

