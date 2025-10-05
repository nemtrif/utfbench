# Generic Makefile: build one executable per .cpp source in this directory.
# It will build `xlbench` from `xlbench.cpp` if that file is added.

CXX := clang++
CXXFLAGS := -std=c++20 -O2 -fno-jump-tables -Wall -Wextra

# discover sources and corresponding targets
SRCS := slbench.cpp
TARGETS := slbench

.PHONY: all clean help

all: $(TARGETS)

# build each executable from its .cpp file
%: %.cpp
	$(CXX) $(CXXFLAGS) -o $@ $< 

clean:
	rm -f $(TARGETS) *.o a.out

