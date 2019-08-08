#! /bin/sh
#set -x$-
#$Header: make.ws,v 4.1 85/07/30 18:09:09 webb Exp $
#$Source: /ibm/acis/usr/sys_ca/conf/RCS/make.ws,v $
#
# shell script to generate a workstation binary with the debugger and
# symbol table merged in.
#
#
# this version deals with two cases (determined from the current directory
#	name - host name could be used instead)
# 1. cross compiled  (/acis/ in directory pathway)
# 2. native compile. (/sys/ in directory pathway)
# 3. other cases 	(treated as native compile)
#
PATH=/xri/bin:/usr/ibmtools:/usr/public:/usr/local:/bin:/usr/bin:/ucb
export PATH
input=${1-vmunix}
output=${3-$input.ws}
cross=

rm -f $output

case `pwd` in
*/acis/*)	# cross compiled on vax
	cross=cross
	VMRDB=${2-../standca/rdb.ws}

# produce a 'vmunix.nm'
	rm -f $input.out
	if file $input | grep "demand paged" ; then
		cvtsym -S -s -p2048 -N $input $input.out || exit 1
		nm -n $input.out > $input.nm || exit 1
		rm $input.out
	else
		nm -n $input > $input.nm || exit 1
	fi
	;;
*)		# native compile
	VMRDB=${2-../standca/rdb.out}
	cross=native
	nm -n $input > $input.nm || exit 1
	;;

#*)			# where are we?
#	cross=unknown
#	echo "what machine is this anyway?"
#	nm -n $input > $input.nm
#	;;
esac

	echo "symbol table produced in $input.nm"
case $cross in
cross)
	cvtsym -S -s -p2048 -N $input $input.out || exit 1
	;;
*)
	rm -f $input.out
	cp $input $input.out
	;;
esac

OMERGE=omerge
# make a vmunix.out that is -N format (vax compatible)
	${OMERGE} -p2048 -f800 -l73728 ${VMRDB} $input.out || exit 1
# following sed makes the 'int' routines visable and also changes the
# virtual kernel addresses back into real addresses
	( sed -e 's;^e;0;' -e 's; int; _int;' $input.nm ) |
		makesym -h > $input.sym
	${OMERGE} -p2048 -8000 -l28672 $input.sym $input.out || exit 1
	echo "debugger (from ${VMRDB}) merged into $input.out"

case $cross in
cross)
# when cross generating generate a version for the workstation byte order
	cvtsym -p2048 -N $input.out $output || exit 1
	rm -f $input.out
	;;
*)
	ln $input.out $output
	;;
esac
	echo "$cross generated kernel with debugger now in $output"
