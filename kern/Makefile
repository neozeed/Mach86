MACH_INTERFACE =  mach.h mach_server.c mach_user.c

PAGER_INTERFACE = pager.h pager_server.c pager_user.c

all: ${MACH_INTERFACE} ${PAGER_INTERFACE}

mach.h: Mach.h
	cp Mach.h mach.h

mach_server.c: MachServer.c mach_server.sed
	sed -f mach_server.sed < MachServer.c > mach_server.c

mach_user.c: MachUser.c mach_user.sed
	sed -f mach_user.sed < MachUser.c > mach_user.c

Mach.h MachServer.c MachUser.c: mach.defs
	mm mach.defs c c

pager.h: Pager.h
	cp Pager.h pager.h

pager_server.c: PagerServer.c pager_server.sed
	sed -f pager_server.sed < PagerServer.c > pager_server.c

pager_user.c: PagerUser.c pager_user.sed
	sed -f pager_user.sed < PagerUser.c > pager_user.c

Pager.h PagerServer.c PagerUser.c: pager.defs
	mm pager.defs c c
