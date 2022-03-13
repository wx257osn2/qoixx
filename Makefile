OBJS=bin/qoibench bin/qoiconv
CXXFLAGS=-std=c++2a -O3 -march=native -mtune=native -Wall -Wextra -pedantic-errors
STB=-I .dependencies/stb
QOI=-I .dependencies/qoi

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

.PHONY: all clean qoibench qoiconv

bin/qoibench: src/qoibench.cpp include/qoixx.hpp
	$(CXX) $(CXXFLAGS) $(DWT) $(STB) $(QOI) -I include -o $@ $<

bin/qoiconv: src/qoiconv.cpp include/qoixx.hpp
	$(CXX) $(CXXFLAGS) $(DWT) $(STB) -I include -o $@ $<
