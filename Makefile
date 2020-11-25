PREFIX ?= /usr/local
CXX ?= g++
CXXFLAGS ?= -O2 -g -Wall
LDFLAGS ?= -Wl,--as-needed

LLVM_CXXFLAGS = $(shell llvm-config --cxxflags)
LLVM_LDFLAGS = $(shell llvm-config --libs --system-libs --link-static)

all: dll-bundler

clean:
	rm -f *.o
	rm -f dll-bundler

install: all
	install -D -m755 dll-bundler $(DESTDIR)$(PREFIX)/bin/dll-bundler

dll-bundler: dll-bundler.o
	$(CXX) $< $(LDFLAGS) $(LLVM_LDFLAGS) -o $@

dll-bundler.o: dll-bundler.cpp
	$(CXX) $^ $(LLVM_CXXFLAGS) $(CXXFLAGS) -c -o $@
