#!/bin/sh -
#
# Copyright (c) 1980 Regents of the University of California.
# All rights reserved.  The Berkeley software License Agreement
# specifies the terms and conditions for redistribution.
#
#	@(#)newvers.sh	5.1 (Berkeley) 5/8/85
#
touch version
awk '	{	version = $1 + 1; }\
END	{	printf "char *version = \"Berkeley VAX/UNIX Version 4.%d  ", version > "vers.c";\
		printf "%d\n", version > "version"; }' < version
echo `date`'\n";' >> vers.c
