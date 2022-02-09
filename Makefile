OBJS=bin/qoibench bin/qoiconv
CXXFLAGS=-std=c++2a -O3 -march=native -mtune=native -Wall -Wextra -pedantic-errors
STB=-I .dependencies/stb
QOI=-I .dependencies/qoi

all: $(OBJS)

clean:
	rm -f $(OBJS)

qoibench: bin/qoibench
qoiconv: bin/qoiconv

.PHONY: all clean qoibench qoiconv

bin/qoibench: src/qoibench.cpp include/qoixx.hpp
	$(CXX) $(CXXFLAGS) $(STB) $(QOI) -I include -o $@ $<

bin/qoiconv: src/qoiconv.cpp include/qoixx.hpp
	$(CXX) $(CXXFLAGS) $(STB) -I include -o $@ $<
