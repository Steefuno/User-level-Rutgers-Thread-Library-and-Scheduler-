CC = gcc
CFLAGS = -g -c
AR = ar -rc
RANLIB = ranlib

SCHED = PSJF

all: rpthread.a

rpthread.a: rpthread.o
	$(AR) librpthread.a rpthread.o
	$(RANLIB) librpthread.a

rpthread.o: rpthread.h

ifeq ($(SCHED), PSJF)
	$(CC) -pthread $(CFLAGS) rpthread.c
else ifeq ($(SCHED), MLFQ)
	$(CC) -pthread $(CFLAGS) -DMLFQ rpthread.c
else
	echo "no such scheduling algorithm"
endif

clean:
	rm -rf testfile *.o *.a
