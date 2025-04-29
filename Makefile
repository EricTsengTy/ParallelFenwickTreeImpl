CXX = g++
CXXFLAGS = -std=c++17 -Wall -O3 -fopenmp -m64 -I. -Wextra

all: fenwick

fenwick: main.cpp fenwick.h task_scheduler.h readerwriterqueue.h atomicops.h
	$(CXX) $(CXXFLAGS) -o $@ $<

run: fenwick
	./fenwick input.txt

clean:
	rm -f fenwick

.PHONY: all run clean