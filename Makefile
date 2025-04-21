CXX = g++
CXXFLAGS = -std=c++17 -Wall -O3 -fopenmp

all: fenwick

fenwick: main.cpp fenwick.h task_scheduler.h locking_queue.h
	$(CXX) $(CXXFLAGS) -o $@ $<

run: fenwick
	./fenwick input.txt

clean:
	rm -f fenwick

.PHONY: all run clean