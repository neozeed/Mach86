.c.o : 
	cc -c -DKERNEL -I. -I../WB $<

all : kern_smalloc.o	mp_sys.o	remote_subr.o	syscall_subr.o\
 mp_queue.o	mp_sysent.o	remote_sw.o	syscall_sw.o\
 mp_shmem.o	remote_prim.o	sem_sem.o
