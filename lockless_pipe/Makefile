CXX=/opt/developerstudio12.6-bin/CC
#CXX=/perfwork/gcc/7.2.0/bin/g++

CC=/opt/developerstudio12.6-bin/cc
#CC=/perfwork/gcc/7.2.0/bin/gcc

CFLAGS= -I. -m64 -xtarget=generic -mt -D_POSIX_PTHREAD_SEMANTICS -xO0

Pipe/perftest/performance_test.o: Atom/atomic_int32.il Atom/atomic_uint32.il Atom/atomic_int64_64.il Atom/atomic_uint64_64.il Pipe/perftest/performance_test.cc
	$(CXX) $(CFLAGS) -o $@ -c $<

all: Pipe/perftest/performance_test.o

.PHONY: all
