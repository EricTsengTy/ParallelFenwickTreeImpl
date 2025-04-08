#ifndef FENWICK_H
#define FENWICK_H

#include <algorithm>
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
        for (++x; x < (int)bits.size(); x += x & -x) {
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

            for (++x; x < (int)bits.size(); x += x & -x) {
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
        for (; x < (int)bits.size(); x += x & -x) {
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

    void initialize_ranges(int n, int num_threads) {
        std::vector<ulong> dp(n + 1);
        ulong total = 0;
        for (int x = 1; x <= n; ++x) {
            ++dp[x];
            total += dp[x];

            int next_x = x;
            next_x += next_x & -next_x;
            if (next_x <= n) {
                dp[next_x] += dp[x];
            }
        }

        ulong average = total / num_threads;
        int cur = 1;

        for (int i = 0; i != num_threads; ++i) {
            ulong thread_total = 0;
            ranges[i].first = cur;
            while (cur < (int)bits.size() && thread_total < average) {
                thread_total += (ulong)dp[cur];
                ++cur;
            }
            while (cur < (int)bits.size() && cur % 64 != 0) {
                ++cur;
            }
            ranges[i].second = cur;
        }

        ranges.back().second = bits.size();
    }

  public:
    FenwickTreePipeline(int n, int num_threads):bits(n + 1, 0), ranges(num_threads) {
        initialize_ranges(n, num_threads);
    }

    void add(int x, int val) override {
        for (++x; x < (int)bits.size(); x += x & -x) {
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
            auto &[lower, upper] = ranges[t];

            for (auto &operation : operations) {
                int x = operation.index;
                int val = operation.value;

                /** 
                 * Compute the smallest index derived from `x` that lies 
                 * within the target range.
                 *    
                 * This avoids having each thread start from the original
                 * `x` and loop through unnecessarily to locate their valid
                 * range.
                 */
                ++x;
                if (x < lower) {
                    auto highest_diff_bit 
                        = 0x8000000000000000ULL >> __builtin_clzll(x ^ lower);

                    x |= highest_diff_bit;
                    x &= ~(highest_diff_bit - 1);

                    if (x < lower) {
                        x += x & -x;
                    }
                }

                for (; x < upper; x += x & -x) {
                    bits[x] += val;
                }
            }
        }
    }

    void statistics() {
        std::vector<std::pair<int, int>> values;
        long total = 0;
        for (int i = 1; i < (int)bits.size(); ++i) {
            values.emplace_back(bits[i], i);
            total += (long) bits[i];
        }

        std::sort(values.begin(), values.end(), std::greater<std::pair<int, int>>());
        for (int i = 0; i < (int)values.size() && i < 20; ++i) {
            std::cerr << values[i].second << ' ' << values[i].first << '\n';
        }

        std::cerr << "Total: " << total << '\n';
        std::cerr << "Average: " << (double)total / bits.size() << '\n';
    }
};

#endif