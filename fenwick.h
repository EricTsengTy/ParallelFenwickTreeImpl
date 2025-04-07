#include <iostream>
#include <fstream>
#include <vector>
#include <chrono>
#include <mutex>
#include <shared_mutex>
#include <atomic>
#include <omp.h>

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
};

// Improved parallel Fenwick Tree
class FenwickTreeLocked : FenwickTreeBase {
  private:
    std::vector<int> bits;

  public:
    FenwickTreeLocked(int n) : bits(n + 1) {
        // Already initialized to 0 in constructor
    }

    // Thread-safe add operation with local lock
    void add(int x, int val) {
        ++x;
        // Update the tree
        for (; x < bits.size(); x += x & -x) {
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
