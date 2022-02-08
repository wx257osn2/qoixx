OBJS=bin/qoibench
CXXFLAGS=-std=c++2a -O3 -march=native -mtune=native -Wall -Wextra -pedantic-errors
STB=-I .dependencies/stb
QOI=-I .dependencies/qoi

all: $(OBJS)

clean:
	rm -f $(OBJS)

qoibench: bin/qoibench

.PHONY: all clean qoibench

bin/qoibench: src/qoibench.cpp include/qoixx.hpp
	$(CXX) $(CXXFLAGS) $(STB) $(QOI) -I include -o $@ $<
