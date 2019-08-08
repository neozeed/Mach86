/*
 * Copyright (c) 1980, 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#ifndef lint
char copyright[] =
"@(#) Copyright (c) 1980, 1986 Regents of the University of California.\n\
 All rights reserved.\n";
#endif not lint

#ifndef lint
static char sccsid[] = "@(#)format.c	7.1 (Berkeley) 6/5/86";
#endif not lint


/* 
 * Standalone program to do media checking
 * and record bad block information on any 
 * disk with the appropriate driver and RM03-style headers.
 * TODO:
 *	add new bad sectors to bad-sector table when formatting by track
 *		(rearranging replacements ala bad144 -a)
 *	multi-pass format for disks with skip-sector capability
 */
#include "../h/param.h"
#include "../h/fs.h"
#include "../h/inode.h"
#include "../h/dkbad.h"
#include "../h/vmmac.h"

#include "saio.h"
#include "savax.h"

#define MAXBADDESC	126		/* size of bad block table */
#define CHUNK		48		/* max # of sectors/io operation */
#define SECTSIZ		512		/* standard sector size */
#define HDRSIZ		4		/* number of bytes in sector header */

#define SSERR		0
#define BSERR		1

#define SSDEV(fd)	(ioctl((fd), SAIOSSDEV, (char *)0) == 0)
#define MAXECCBITS	3

struct sector {
	u_short	header1;
	u_short header2;
	char	buf[SECTSIZ];
};

struct	dkbad dkbad;		/* bad sector table */
struct	dkbad oldbad;		/* old bad sector table */
struct	dkbad sstab;		/* skip sector table */

#define	NERRORS		6
static char *
errornames[NERRORS] = {
#define	FE_BSE		0
	"Bad sector",
#define	FE_WCE		1
	"Write check",
#define	FE_ECC		2
	"Hard ECC",
#define	FE_HARD		3
	"Other hard",
#define	FE_TOTAL	4
	"Marked bad",
#define	FE_SSE		5
	"Skipped",
};

int	errors[NERRORS];	/* histogram of errors */
int	pattern;
int	maxeccbits;

/*
 * Purdue/EE severe burnin patterns.
 */
unsigned short ppat[] = {
0xf00f, 0xec6d, 0031463,0070707,0133333,0155555,0161616,0143434,
0107070,0016161,0034343,0044444,0022222,0111111,0125252, 052525,
0125252,0125252,0125252,0125252,0125252,0125252,0125252,0125252,
#ifndef	SHORTPASS
0125252,0125252,0125252,0125252,0125252,0125252,0125252,0125252,
 052525, 052525, 052525, 052525, 052525, 052525, 052525, 052525,
#endif
 052525, 052525, 052525, 052525, 052525, 052525, 052525, 052525
 };

#define	NPT	(sizeof (ppat) / sizeof (short))
int	maxpass, npat;	/* subscript to ppat[] */
int	severe;		/* nz if running "severe" burnin */
int	ssdev;			/* device supports skip sectors */
int	startcyl, endcyl, starttrack, endtrack;
int	nbads;			/* subscript for bads */
daddr_t	bads[2*MAXBADDESC]; 	/* Bad blocks accumulated */

char	*malloc();
int	qcompar();
char	*prompt();
daddr_t	badsn();
extern	int end;

main()
{
	register int sector, sn;
	int lastsector, tracksize, rtracksize;
	int unit, fd, resid, i, trk, cyl, debug;
	struct st st;
	struct sector *bp, *cbp;
	char *rbp, *rcbp;
	int pass;
	char *cp;

	printf("Disk format/check utility\n\n");

again:
	nbads = 0;
	cp = prompt("Enable debugging (0=none, 1=bse, 2=ecc, 3=bse+ecc)? ");
	debug = atoi(cp);
	if (debug < 0)
		debug = 0;
	for (i = 0; i < NERRORS; i++)
		errors[i] = 0;
	fd = getdevice();
	ioctl(fd, SAIODEVDATA, &st);
	printf("Device data: #cylinders=%d, #tracks=%d, #sectors=%d\n",
	  st.ncyl, st.ntrak, st.nsect);
	ssdev = SSDEV(fd);
	if (ssdev) {
		ioctl(fd, SAIOSSI, (char *)0);	/* set skip sector inhibit */
		st.nsect++;
		st.nspc += st.ntrak;
		printf("(not counting skip-sector replacement)\n");
	}
	getrange(&st);
	if (getpattern())
		goto again;
	printf("Start formatting...make sure the drive is online\n");
	ioctl(fd, SAIONOBAD, (char *)0);
	ioctl(fd, SAIORETRIES, (char *)0);
	ioctl(fd, SAIOECCLIM, (char *)maxeccbits);
	ioctl(fd, SAIODEBUG, (char *)debug);
	tracksize = sizeof (struct sector) * st.nsect;
	rtracksize = SECTSIZ * st.nsect;
	bp = (struct sector *)malloc(tracksize);
	rbp = malloc(rtracksize);
	pass = 0;
	npat = 0;
more:
	for (; pass < maxpass; pass++) {
		if (severe)
			printf("Begin pass %d\n", pass);
		bufinit(bp, tracksize);
		if (severe)
			npat++;
		/*
		 * Begin check, for each track,
		 *
		 * 1) Write header and test pattern.
		 * 2) Read data.  Hardware checks header and data ECC.
		 *    Read data (esp on Eagles) is much faster than write check.
		 */
		sector = ((startcyl * st.ntrak) + starttrack) * st.nsect;
		lastsector = ((endcyl * st.ntrak) + endtrack) * st.nsect
			+ st.nsect;
		for ( ; sector < lastsector; sector += st.nsect) {
			cyl = sector / st.nspc;
			trk = (sector % st.nspc) / st.nsect;
			for (i = 0; i < st.nsect; i++) {
				bp[i].header1 =
					(u_short) cyl | HDR1_FMT22 | HDR1_OKSCT;
				bp[i].header2 = ((u_short)trk << 8) + i;
			}
			if (sector && (sector % (st.nspc * 100)) == 0)
				printf("cylinder %d\n", cyl);
			/*
			 * Try and write the headers and data patterns into
			 * each sector in the track.  Continue until such
			 * we're done, or until there's less than a sector's
			 * worth of data to transfer.
			 *
			 * The lseek call is necessary because of
			 * the odd sector size (516 bytes)
			 */
			for (resid = tracksize, cbp = bp, sn = sector;;) {
				int cc;

				lseek(fd, sn * SECTSIZ, 0);
				ioctl(fd, SAIOHDR, (char *)0);
				cc = write(fd, cbp, resid);
				if (cc == resid)
					break;
				/*
				 * Don't record errors during write,
				 * all errors will be found during
				 * check performed below.
				 */
				sn = iob[fd - 3].i_errblk;
				cbp += sn - sector;
				resid -= (sn - sector) * sizeof (struct sector);
				if (resid < sizeof (struct sector)) 
					break;
			}
			/*
			 * Read test patterns.
			 * Retry remainder of track on error until
			 * we're done, or until there's less than a
			 * sector to verify.
			 */
			for (resid = rtracksize, rcbp = rbp, sn = sector;;) {
				int cc, rsn;

				lseek(fd, sn * SECTSIZ, 0);
				cc = read(fd, rcbp, resid);
				if (cc == resid)
					break;
				sn = iob[fd-3].i_errblk;
				if (ssdev) {
					rsn = sn - (sn / st.nsect);
					printf("data ");
				} else
					rsn = sn;
				printf("sector %d, read error\n\n", rsn);
				if (recorderror(fd, sn, &st) < 0 && pass > 0)
					goto out;
				/* advance past bad sector */
				sn++;
				resid = rtracksize - ((sn - sector) * SECTSIZ);
				rcbp = rbp + ((sn - sector) * SECTSIZ);
				if (resid < SECTSIZ) 
					break;
			}
		}
	}
	/*
	 * Checking finished.
	 */
out:
	if (severe && maxpass < NPT) {
		cp = prompt("More passes? (0 or number) ");
		maxpass = atoi(cp);
		if (maxpass > 0) {
			maxpass += pass;
			goto more;
		}
	}
	if (severe && nbads) {
		/*
		 * Sort bads and insert in bad block table.
		 */
		qsort(bads, nbads, sizeof (daddr_t), qcompar);
		severe = 0;
		errno = 0;
		for (i = 0; i < nbads; i++)
			recorderror(fd, bads[i], &st);
		severe++;
	}
	if (errors[FE_TOTAL] || errors[FE_SSE]) {
		/* change the headers of all the bad sectors */
		writebb(fd, errors[FE_SSE], &sstab, &st, SSERR);
		writebb(fd, errors[FE_TOTAL], &dkbad, &st, BSERR);
	}
	if (errors[FE_TOTAL] || errors[FE_SSE]) {
		printf("Errors:\n");
		for (i = 0; i < NERRORS; i++)
			printf("%s: %d\n", errornames[i], errors[i]);
		printf("Total of %d hard errors revectored\n",
			errors[FE_TOTAL] + errors[FE_SSE]);
	}
	if (endcyl == st.ncyl - 1 &&
	    (startcyl < st.ncyl - 1 || starttrack == 0)) {
		while (errors[FE_TOTAL] < MAXBADDESC) {
			int i = errors[FE_TOTAL]++;

			dkbad.bt_bad[i].bt_cyl = -1;
			dkbad.bt_bad[i].bt_trksec = -1;
		}
		printf("\nWriting bad sector table at sector #%d\n",
			st.ncyl * st.nspc - st.nsect);
		/* place on disk */
		for (i = 0; i < 10 && i < st.nsect; i += 2) {
			lseek(fd, SECTSIZ * (st.ncyl * st.nspc - st.nsect + i), 0);
			write(fd, &dkbad, sizeof (dkbad));
		}
	} else if (errors[FE_TOTAL]) {
		struct bt_bad *bt;

		printf("New bad sectors (not added to table):\n");
		bt = dkbad.bt_bad;
		for (i = 0; i < errors[FE_TOTAL]; i++) {
			printf("bn %d (cn=%d, tn=%d, sn=%d)\n", badsn(bt, &st),
			    bt->bt_cyl, bt->bt_trksec>>8, bt->bt_trksec&0xff);
			bt++;
		}
	}
	printf("Done\n");
	ioctl(fd,SAIONOSSI,(char *)0);
	close(fd);
#ifndef JUSTEXIT
	goto again;
#endif
}

qcompar(l1, l2)
register daddr_t *l1, *l2;
{
	if (*l1 < *l2)
		return(-1);
	if (*l1 == *l2)
		return(0);
	return(1);
}

daddr_t
badsn(bt, st)
	register struct bt_bad *bt;
	register struct st *st;
{

	if (ssdev)
	    return ((bt->bt_cyl * st->ntrak + (bt->bt_trksec>>8)) *
		(st->nsect - 1) + (bt->bt_trksec&0xff)) - 1;
	else
	    return ((bt->bt_cyl*st->ntrak + (bt->bt_trksec>>8)) * st->nsect
		+ (bt->bt_trksec&0xff));
}

/*
 * Mark the bad/skipped sectors.
 * Bad sectors on skip-sector devices are assumed to be skipped also,
 * and must be done after the (earlier) first skipped sector.
 */
writebb(fd, nsects, dbad, st, sw)
	int nsects, fd;
	struct dkbad *dbad;
	register struct st *st;
{
	struct sector bb_buf; /* buffer for one sector plus 4 byte header */
	register int i;
	int bn, j;
	struct bt_bad *btp;

	for (i = 0; i < nsects; i++) {
		btp = &dbad->bt_bad[i];
		if (sw == BSERR) {
			bb_buf.header1 = HDR1_FMT22|btp->bt_cyl;
			if (ssdev)
				bb_buf.header1 |= HDR1_SSF;
		} else
			bb_buf.header1 =
			       btp->bt_cyl | HDR1_FMT22 | HDR1_SSF | HDR1_OKSCT;
		bb_buf.header2 = btp->bt_trksec;
		bn = st->nspc * btp->bt_cyl +
		     st->nsect * (btp->bt_trksec >> 8) +
		     (btp->bt_trksec & 0xff);
		lseek(fd, bn * SECTSIZ, 0);
		ioctl(fd, SAIOHDR, (char *)0);
		write(fd, &bb_buf, sizeof (bb_buf));
		/*
		 * If skip sector, mark all remaining
		 * sectors on the track.
		 */
		if (sw == SSERR) {
			for (j = (btp->bt_trksec & 0xff) + 1, bn++;
			    j < st->nsect; j++, bn++) {
				bb_buf.header2 = j | (btp->bt_trksec & 0xff00);
				lseek(fd, bn * SECTSIZ, 0);
				ioctl(fd, SAIOHDR, (char *)0);
				write(fd, &bb_buf, sizeof (bb_buf));
			}
		}
	}
}

/*
 * Record an error, and if there's room, put
 * it in the appropriate bad sector table.
 *
 * If severe burnin store block in a list after making sure
 * we have not already found it on a prev pass.
 */
recorderror(fd, bn, st)
	int fd, bn;
	register struct st *st;
{
	int cn, tn, sn;
	register i;

	
	if (severe) {
		for (i = 0; i < nbads; i++)
			if (bads[i] == bn)
				return(0);	/* bn already flagged */
		if (nbads >= (ssdev ? 2 * MAXBADDESC : MAXBADDESC)) {
			printf("Bad sector table full, format terminating\n");
			return(-1);
		}
		bads[nbads++] = bn;
		if (errno < EBSE || errno > EHER)
			return(0);
		errno -= EBSE;
		errors[errno]++;
		return(0);
	}
	if (errno >= EBSE && errno <= EHER) {
		errno -= EBSE;
		errors[errno]++;
	}
	cn = bn / st->nspc;
	sn = bn % st->nspc;
	tn = sn / st->nsect;
	sn %= st->nsect;
	if (ssdev) {		/* if drive has skip sector capability */
		int ss = errors[FE_SSE];

		if (errors[FE_SSE] >= MAXBADDESC) {
			/* this is bogus, we don't maintain skip sector table */
			printf("Too many skip sector errors\n");
			return(-1);
		}
		  /* only one skip sector/track */
		if (ss == 0 ||
		    tn != (sstab.bt_bad[ss - 1].bt_trksec >> 8) ||
		    cn != sstab.bt_bad[ss - 1].bt_cyl) {
			/*
			 * Don't bother with skipping the extra sector
			 * at the end of the track.
			 */
			if (sn == st->nsect - 1)
				return(0);
			sstab.bt_bad[ss].bt_cyl = cn;
			sstab.bt_bad[ss].bt_trksec = (tn<<8) + sn;
			errors[FE_SSE]++;
			return(0);
		}
	}
	if (errors[FE_TOTAL] >= MAXBADDESC) {
		printf("Too many bad sectors\n");
		return(-1);
	}
	/* record the bad sector address and continue */
	dkbad.bt_bad[errors[FE_TOTAL]].bt_cyl = cn;
	dkbad.bt_bad[errors[FE_TOTAL]++].bt_trksec = (tn << 8) + sn;
	return(0);
}

/*
 * Allocate memory on a page-aligned address.
 * Round allocated chunk to a page multiple to
 * ease next request.
 */
char *
malloc(size)
	int size;
{
	char *result;
	static caddr_t last = 0;

	if (last == 0)
		last = (caddr_t)(((int)&end + 511) & ~0x1ff);
	size = (size + 511) & ~0x1ff;
	result = (char *)last;
	last += size;
	return (result);
}

/*
 * Prompt and verify a device name from the user.
 */
getdevice()
{
	register char *cp;
	register struct devsw *dp;
	int fd;

top:
	cp = prompt("Device to format? ");
	if ((fd = open(cp, 2)) < 0) {
		printf("Known devices are: ");
		for (dp = devsw; dp->dv_name; dp++)
			printf("%s ",dp->dv_name);
		printf("\n");
		goto top;
	}
	printf("Formatting drive %c%c%d on adaptor %d: ",
		cp[0], cp[1], iob[fd - 3].i_unit % 8, iob[fd - 3].i_unit / 8);
	cp = prompt("verify (yes/no)? ");
	while (*cp != 'y' && *cp != 'n')
		cp = prompt("Huh, yes or no? ");
	if (*cp == 'y')
		return (fd);
	goto top;
}

/*
 * Find range of tracks to format.
 */
getrange(st)
	struct st *st;
{
	startcyl = getnum("Starting cylinder", 0, st->ncyl - 1, 0);
	starttrack = getnum("Starting track", 0, st->ntrak - 1, 0);
	endcyl = getnum("Ending cylinder", 0, st->ncyl - 1, st->ncyl - 1);
	endtrack = getnum("Ending track", 0, st->ntrak - 1, st->ntrak - 1);
}

getnum(s, low, high, dflt)
{
	char buf[132];
	unsigned val;

	while (1) {
		printf("%s (%d): ", s, dflt);
		gets(buf);
		if (buf[0] == 0)
			return (dflt);
		val = atoi(buf);
		if (val >= low && val <= high)
			return ((int)val);
		printf("Value must be in range [%d,%d]\n", low, high);
	}
}

static struct pattern {
	long	pa_value;
	char	*pa_name;
} pat[] = {
	{ 0xf00ff00f, 	"RH750 worst case" },
	{ 0xec6dec6d,	"media worst case" },
	{ 0xa5a5a5a5,	"alternate 1's and 0's" },
	{ 0xFFFFFFFF,	"Severe burnin (up to 48 passes)" },
	{ 0, 0 },
};

getpattern()
{
	register struct pattern *p;
	int npatterns;
	char *cp;

	printf("Available test patterns are:\n");
	for (p = pat; p->pa_value; p++)
		printf("\t%d - (%x) %s\n", (p - pat) + 1,
		  p->pa_value & 0xffff, p->pa_name);
	npatterns = p - pat;
	cp = prompt("Pattern (one of the above, other to restart)? ");
	pattern = atoi(cp) - 1;
	if (pattern < 0 || pattern >= npatterns)
		return(1);
	severe = 0;
	maxpass = 1;
	if (pat[pattern].pa_value == -1) {
		severe = 1;
		cp = prompt("How many passes (up to 48)? ");
		maxpass = atoi(cp);
		if (maxpass > NPT)
			maxpass = NPT;
	}
	maxeccbits = getnum(
		"Maximum number of bit errors to allow for soft ECC",
		0, 11, MAXECCBITS);
	return (0);
}

struct xsect {
	u_short	hd1;
	u_short	hd2;
	long	buf[128];
};

/*
 * Initialize the buffer with the requested pattern. 
 */
bufinit(bp, size)
	register struct xsect *bp;
	int size;
{
	register struct pattern *pptr;
	register long *pp, *last;
	register struct xsect *lastbuf;
	int patt;

	size /= sizeof (struct sector);
	lastbuf = bp + size;
	if (severe) {
		patt = ppat[npat] | ((long)ppat[npat] << 16);
		printf("Write pattern 0x%x\n", patt&0xffff);
	} else {
		pptr = &pat[pattern];
		patt = pptr->pa_value;
	}
	while (bp < lastbuf) {
		last = &bp->buf[128];
		for (pp = bp->buf; pp < last; pp++)
			*pp = patt;
		bp++;
	}
}

char *
prompt(msg)
	char *msg;
{
	static char buf[132];

	printf("%s", msg);
	gets(buf);
	return (buf);
}
