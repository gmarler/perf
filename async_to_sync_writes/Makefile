CC  = /opt/solarisstudio12.4/bin/cc
CXX = /opt/solarisstudio12.4/bin/CC

CFLAGS   = -m64 -g3 -xO3
CXXFLAGS = -m64 -g3 -xO3 -std=c++11

test1: test1.c
	$(CC) $(CFLAGS) -o $@ $<

aio_signal_test: aio_signal_test.cc
	$(CXX) $(CXXFLAGS) -o $@ $<

aio_portevent_test: aio_portevent_test.cc
	$(CXX) $(CXXFLAGS) -o $@ $<

aio_suspend_test: aio_suspend_test.cc
	$(CXX) $(CXXFLAGS) -o $@ $<

clean:
	@rm test1 aio_signal_test aio_portevent_test aio_suspend_test
