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
#define CONFIG_BLOCK	1		/* block where configuration is */
#define CONFIG_BLOCK2	30		/* second block where configuration is */
#define BOOT_BLOCK	0		/* block for boot information */
/*
 * hard disk configuration format 
 */
struct hdconfig
{				/* manufacturers configuration record */
int	conf_magic;		/* 4: always 0xF8E9DACB */
int	conf_sectorcount;	/* 8: number of sectors on device */
short	conf_landing;		/* 12: landing zone */
char	conf_interleave;	/* 14: interleave */
char	conf_sectsize;		/* 15: sectors size 02 = 512 */
short	conf_lastcyl;		/* 16: last usable cylinder */
char	conf_lasttrack;		/* 18: last head number */
char	conf_lastsect;		/* 19: last sector number */
char	conf_precomp;		/* 20: precomp or 0xff */
char	conf_status;		/* 21: used by loadable post */
short	conf_maxcyl;		/* 22: last physical cyl */
#define conf_ce_cyl conf_maxcyl
short	conf_xx;		/* 24 reserved */
char	conf_seek_char;		/* 26 seek curve characeristics */
char	conf_type[3];		/* 27...29 type of disk drive */
char	conf_fill[2];		/* 30...31 */
char	conf_fill2[512-32];	/* fill up to the end */
				/* 512: total size */
} ;
#define MAXBADBLKS	1000		/* max number of bad sectors */
#define BAD_BLOCK_START	8		/* start of bad block table */

struct hdbad {
char hddefect[6];	/* "DEFECT" 	*/
short	hdcount;	/* count of entries in table */
struct hdmap {
	unsigned hdreason:8,	/* the reason block was unusable */
		hdbad:24;	/* the bad block number */
	int	hdgood;		/* the good block number */
	} hdmap[MAXBADBLKS];	/* the table of bad to good blocks */
} ;			/* the drives bad sector maps */

#define HDREASON_MFR	0x00	/* manufacturer found defect */
#define HDREASON_SURF	0xaa	/* surface verification found defect */
#define HDREASON_SYS	0xbb	/* operating system found defect */

struct hdbadtmp {		/* like hdmap but with only 1 entry defined */
char hddefect[6];		/* "DEFECT" 	*/
short	hdcount;		/* count of entries in table */
struct hdmap hdmap[1];		/* the table of bad to good blocks */
} ;				/* the drives bad sector maps */

struct boothdr {
char	boot_ibma[4];		/* IBMA in EBCDIC */
long	boot_check;			/* consistency check */
short	boot_lastcyl;		/* last available cyl number */
char	boot_lasttrack;		/* last track number */
char	boot_lastsect;		/* last sector number */
short	boot_sectorsize;		/* block/sector size */
short	boot_defects;		/* number of defects */
unsigned	:24,		/* reserved */
	boot_interleave:8;		/* interleave factor */
long	boot_llpblock;		/* load list processor block */
long	boot_sectorcount;		/* the number of sectors on device */
long	boot_formatdate;		/* date format was done */
short	boot_cyl;		/* cyl number of boot */
char	boot_track;		/* track number of boot */
char	boot_sector;		/* sector to boot */
long	boot_length;		/* length in sectors */
long	boot_entry;		/* entry point */
long	boot_vrmminidisk;	/* cyl, head, sector of vrm */
int	boot_fill[116];		/* VRM stuff */
} ;

/* predicate to test if a valid bad block table exists */
#define BAD_BLOCK_TABLE_OK(hdb) !((hdb)->hddefect[0] != 'D' || (hdb)->hddefect[1] != 'E' || \
	    (hdb)->hddefect[2] != 'F' || (hdb)->hddefect[3] != 'E' || \
	    (hdb)->hddefect[4] != 'C' || (hdb)->hddefect[5] != 'T' || \
	    (hdb)->hdcount < 0 || (hdb)->hdcount >= MAXBADBLKS) 
