CXX ?= g++
CXXFLAGS ?= -O2 -g
LDFLAGS ?= -Wl,--as-needed

LLVM_CXXFLAGS = $(shell llvm-config --cxxflags)
LLVM_LDFLAGS = $(shell llvm-config --libs --system-libs --link-static)

all: dll-bundler

clean:
	rm -f *.o
	rm -f dll-bundler

dll-bundler: dll-bundler.o
	$(CXX) $< $(LLVM_LDFLAGS) $(LDFLAGS) -o $@

dll-bundler.o: dll-bundler.cpp
	$(CXX) $^ $(LLVM_CXXFLAGS) $(CXXFLAGS) -c -o $@
