# @(#)Makefile	1.6	ULTRIX	12/3/84
#
#	Makefile	4.3	82/10/27
#
#	new stack.o stuff

DESTROOT= 
DFLAGS = -DADB
CFLAGS=	-O ${DFLAGS}
CFILES= access.c command.c expr.c format.c input.c main.c message.c \
	opset.c optab.c output.c pcs.c print.c runpcs.c setup.c sym.c \
	stack.c
OFILES= access.o command.o expr.o format.o input.o main.o message.o \
	opset.o optab.o output.o pcs.o print.o runpcs.o setup.o sym.o \
	stack.o

OLIB=	reset.o strcmp.o ctype_.o
LIBC=	/lib/libc.a

#
#	path to the directory containing as
#
SRCAS =	../as

lib:
	make DFLAGS=-DKDB ../../lib/libkdb.a

adb:	${OFILES}
	${CC} -o adb ${OFILES}

kdb:	kdb.o lib
	${CC} -o kdb -N kdb.o -lkdb

../../lib/libkdb.a: ${OFILES} ${OLIB}
	ld -X -r ${OFILES} ${OLIB}
	mv a.out libkdb.a
	mv libkdb.a ../../lib

${OLIB}: ${LIBC}
	ar xv ${LIBC} $@

defs.h:		mac.h mode.h
mode.h:		machine.h

access.o:	defs.h access.c
command.o:	defs.h command.c
expr.o:		defs.h expr.c
format.o:	defs.h format.c
input.o:	defs.h input.c
main.o:		defs.h main.c
message.o:	mac.h mode.h message.c
	${CC} -c -R message.c
pcs.o:		defs.h pcs.c
print.o:	defs.h print.c
runpcs.o:	defs.h runpcs.c
setup.o:	defs.h setup.c
sym.o:		defs.h sym.c

opset.o:	defs.h opset.c
	${CC} ${CFLAGS} -c    -I. -I$(SRCAS) opset.c
optab.o:	defs.h optab.c instrs.adb
	${CC} ${CFLAGS} -c -R -I. -I$(SRCAS) optab.c
instrs.adb: $(SRCAS)/instrs
	(echo FLAVOR ADB; cat $(SRCAS)/instrs) | awk -f $(SRCAS)/instrs > instrs.adb

install: adb
	install -c -s adb ${DESTROOT}/bin

clean:
	rm -f adb ${OFILES}
	rm -f a.out core *.s errs

clobber: clean
	rm -f *.[hclyp] Makefile instrs instrs.adb

sccsinfo:
	sccs info

sccsget:
	sccs get SCCS

print:
	@ls -l | pr
	@pr -f Makefile *.h ${CFILES}
