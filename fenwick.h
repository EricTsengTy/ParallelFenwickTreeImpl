#ifndef FENWICK_H
#define FENWICK_H

#include <iostream>
#include <fstream>
#include <vector>
#include <chrono>
#include <mutex>
#include <shared_mutex>
#include <omp.h>

#include "generator.h"

class FenwickTreeBase {
  public:
    virtual void add(int x, int val) = 0;
    virtual int sum(int x) = 0;
};

// Original sequential Fenwick Tree
class FenwickTreeSequential : FenwickTreeBase {
  private:
    std::vector<int> bits;

  public:
    FenwickTreeSequential(int n):bits(n + 1, 0){}

    void add(int x, int val) override {
        for (++x; x < bits.size(); x += x & -x) {
            bits[x] += val;
        }
    }

    int sum(int x) override {
        int total = 0;
        for (++x; x > 0; x -= x & -x) {
            total += bits[x];
        }
        return total;
    }

    void batchAdd(std::vector<Operation> &operations) {
        for (auto &operation : operations) {
            int x = operation.index;
            int val = operation.value;

            for (++x; x < bits.size(); x += x & -x) {
                bits[x] += val;
            }
        }
    }
};

// Improved parallel Fenwick Tree
class FenwickTreeLocked : FenwickTreeBase {
  private:
    std::vector<int> bits;
    std::vector<std::mutex> mutexes;

  public:
    FenwickTreeLocked(int n) : bits(n + 1), mutexes(n + 1) {
        // Already initialized to 0 in constructor
    }

    // Thread-safe add operation with local lock
    void add(int x, int val) {
        // TODO: make this variable more scalable
        const int lock_size = 16384;
        ++x;
        // Update the tree
        std::unique_lock<std::mutex> lock(mutexes[x / lock_size]);
        int prev_x = x;
        for (; x < bits.size(); x += x & -x) {
            if (prev_x / lock_size != x / lock_size) {
                lock.unlock();
                lock = std::unique_lock<std::mutex>(mutexes[x / lock_size]);
                prev_x = x;
            }
            bits[x] += val;
        }
    }

    // Thread-safe sum operation (no locks needed with atomics)
    int sum(int x) {
        int total = 0;
        for (++x; x > 0; x -= x & -x) {
            total += bits[x];
        }
        return total;
    }
};

// Pipeline Fenwick Tree
class FenwickTreePipeline : FenwickTreeBase {
  private:
    std::vector<int> bits;
    std::vector<std::pair<int, int>> ranges;

  public:
    FenwickTreePipeline(int n, int num_threads):bits(n + 1, 0), ranges(num_threads) {
        for (int i = 0; i != num_threads; ++i) {
            ranges[i].first = n / num_threads * i + 1;
            ranges[i].second = std::min(n / num_threads * (i + 1) + 1, n + 1);
        }
    }

    void add(int x, int val) override {
        for (++x; x < bits.size(); x += x & -x) {
            bits[x] += val;
        }
    }

    int sum(int x) override {
        int total = 0;
        for (++x; x > 0; x -= x & -x) {
            total += bits[x];
        }
        return total;
    }

    void batchAdd(std::vector<Operation> &operations) {
        #pragma omp parallel
        {
            int t = omp_get_thread_num();
            for (auto &operation : operations) {
                int x = operation.index;
                int val = operation.value;

                for (++x; x < ranges[t].second; x += x & -x) {
                    if (x >= ranges[t].first) {
                        bits[x] += val;
                    }
                }
            }
        }
    }
};

#endif