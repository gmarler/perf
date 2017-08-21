CC  = /opt/solarisstudio12.4/bin/cc
CXX = /opt/solarisstudio12.4/bin/CC

CFLAGS   = -m64 -g3 -xO3 -xlang=c99 -D_XOPEN_SOURCE=600
CXXFLAGS = -m64 -g3 -xO3 -std=c++11

test1: test1.c
	$(CC) $(CFLAGS) -o $@ $<

test1_close_then_rename: test1_close_then_rename.c
	$(CC) $(CFLAGS) -o $@ $<

test1_close_then_fsync_dir: test1_close_then_fsync_dir.c
	$(CC) $(CFLAGS) -o $@ $<

aio_signal_test: aio_signal_test.cc
	$(CXX) $(CXXFLAGS) -o $@ $<

aio_portevent_test: aio_portevent_test.cc
	$(CXX) $(CXXFLAGS) -o $@ $<

aio_suspend_test: aio_suspend_test.cc
	$(CXX) $(CXXFLAGS) -o $@ $<

aio_listio_test: aio_listio_test.cc
	$(CXX) $(CXXFLAGS) -o $@ $<

test: main.o options.o buffer_initialize.o pwrite_test.o lio_listio_test.o
	$(CC) $(CFLAGS) -o $@ $^

clean:
	@rm test1 test1_close_then_rename test1_close_then_fsync_dir aio_signal_test aio_portevent_test aio_suspend_test aio_listio_test test
