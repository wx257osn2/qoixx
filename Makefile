OBJS=bin/qoibench bin/qoiconv bin/test
CXXFLAGS=-std=c++2a -O3 -march=native -mtune=native -Wall -Wextra -pedantic-errors
STB=-I .dependencies/stb
QOI=-I .dependencies/qoi
DOCTEST=-I .dependencies/doctest/doctest

DECODE_WITH_TABLES ?= auto
ifeq ($(DECODE_WITH_TABLES), enable)
  DWT := -DQOIXX_DECODE_WITH_TABLES=1
else
ifeq ($(DECODE_WITH_TABLES), disable)
  DWT := -DQOIXX_DECODE_WITH_TABLES=0
else
  DWT :=
endif
endif

all: $(OBJS)

clean:
	rm -f $(OBJS)

qoibench: bin/qoibench
qoiconv: bin/qoiconv
test: bin/test
	bin/test

.PHONY: all clean qoibench qoiconv test

bin/qoibench: src/qoibench.cpp include/qoixx.hpp
	$(CXX) $(CXXFLAGS) $(DWT) $(STB) $(QOI) -I include -o $@ $<

bin/qoiconv: src/qoiconv.cpp include/qoixx.hpp
	$(CXX) $(CXXFLAGS) $(DWT) $(STB) -I include -o $@ $<

bin/test: src/test.cpp include/qoixx.hpp
	$(CXX) $(CXXFLAGS) $(DWT) $(DOCTEST) -I include -o $@ $<
