CXX = g++
CXXFLAGS = -std=c++17 -Wall -O2 -fopenmp

all: fenwick

fenwick: main.cpp fenwick.h
	$(CXX) $(CXXFLAGS) -o $@ $<

run: fenwick
	./fenwick input.txt

clean:
	rm -f fenwick

.PHONY: all run clean
