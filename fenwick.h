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
    int num_threads;
    std::vector<int> bits;
    std::vector<std::pair<int, int>> ranges;
    std::vector<double> execution_times;
    
    void initialize_ranges(int n, int num_threads) {
        std::vector<long> dp(n + 1);
        double total = 0;
        for (int x = 1; x <= n; ++x) {
            ++dp[x];
            total += dp[x];

            int next_x = x;
            next_x += next_x & -next_x;
            if (next_x <= n) {
                dp[next_x] += dp[x];
            }
        }

        // Split the internal array to several subarray and assign to each thread
        int cur = 1;

        for (int i = 0; i != num_threads; ++i) {
            double average = total / (num_threads - i);
            double thread_total = 0;
            ranges[i].first = cur;
            while (cur < (int)bits.size() && thread_total < average) {
                thread_total += dp[cur];
                ++cur;
            }

            if (cur > ranges[i].first && labs(thread_total - average) > labs(thread_total - dp[cur - 1] - average) && cur > ranges[i].first + 1) {
                --cur;
                thread_total -= dp[cur];
            }

            ranges[i].second = cur;
            total -= thread_total;
        }

        ranges.back().second = bits.size();
    }

  public:
    FenwickTreePipeline(int n, int num_threads):
        num_threads(num_threads),
        bits(n + 1, 0), 
        ranges(num_threads), 
        execution_times(num_threads) {
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
            const auto [lower, upper] = ranges[t];

            #ifdef TIMING
            auto start_time = omp_get_wtime();
            #endif

            for (const auto &operation : operations) {
                int x = operation.index + 1;
                int val = operation.value;

                /** 
                 * Compute the smallest index derived from `x` that lies 
                 * within the target range.
                 *    
                 * This avoids having each thread start from the original
                 * `x` and loop through unnecessarily to locate their valid
                 * range.
                 */
                if (x < lower) {
                    auto highest_diff_bit 
                        = 0x8000000000000000ULL >> __builtin_clzll(x ^ lower);

                    x |= highest_diff_bit;
                    x &= ~(highest_diff_bit - 1);

                    if (x < lower) {
                        x += x & -x;
                    }
                }

                // [lower, upper)
                for (; x < upper; x += x & -x) {
                    bits[x] += val;
                }
            }

            #ifdef TIMING
            auto end_time = omp_get_wtime();
            execution_times[t] += end_time - start_time;
            #endif
        }
    }

    void printRanges() {
        for (int i = 0; i != num_threads; ++i) {
            std::cerr << "Thread " << i << ' ' << ranges[i].first << ' ' << ranges[i].second << '\n';
        }
    }

    void statistics() {
        for (auto time : execution_times) {
            std::cerr << time << '\n';
        }
    }
};

// Pipeline Fenwick Tree
class FenwickTreePipelineSemiStatic : FenwickTreeBase {
  private:
    int num_threads;
    std::vector<int> bits;
    std::vector<std::pair<int, int>> ranges;
    std::vector<double> execution_times;
    
    // should be an odd number
    const int step = 127;

    void initialize_ranges(int n, int num_threads) {
        std::vector<long> dp(n + 1);
        double total = 0;
        for (int x = 1; x <= n; ++x) {
            ++dp[x];
            total += dp[x];

            int next_x = x;
            next_x += next_x & -next_x;
            if (next_x <= n) {
                dp[next_x] += dp[x];
            }
        }

        // Split the internal array to several subarray and assign to each thread
        int cur = 1;

        for (int i = 0; i != num_threads; ++i) {
            double average = total / (num_threads - i);
            double thread_total = 0;
            ranges[i].first = cur;
            while (cur < (int)bits.size() && thread_total < average) {
                thread_total += dp[cur];
                ++cur;
            }

            if (cur > ranges[i].first && labs(thread_total - average) > labs(thread_total - dp[cur - 1] - average) && cur > ranges[i].first + 1) {
                --cur;
                thread_total -= dp[cur];
            }

            ranges[i].second = cur;
            total -= thread_total;
        }

        ranges.back().second = bits.size();
    }

  public:
    FenwickTreePipelineSemiStatic(int n, int num_threads, int step = 127):
        num_threads(num_threads),
        bits(n + 1, 0), 
        ranges(num_threads), 
        execution_times(num_threads),
        step((step & ~1) + 1) {
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
            const auto [lower, upper] = ranges[t];

            // Add a barrier for more accurate estimation of execution time
            #pragma omp barrier
            #ifdef TIMING
            auto start_time = omp_get_wtime();
            #endif
            for (const auto &operation : operations) {
                int x = operation.index + 1;
                int val = operation.value;

                /** 
                 * Compute the smallest index derived from `x` that lies 
                 * within the target range.
                 *    
                 * This avoids having each thread start from the original
                 * `x` and loop through unnecessarily to locate their valid
                 * range.
                 */
                if (x < lower) {
                    auto highest_diff_bit 
                        = 0x8000000000000000ULL >> __builtin_clzll(x ^ lower);

                    x |= highest_diff_bit;
                    x &= ~(highest_diff_bit - 1);

                    if (x < lower) {
                        x += x & -x;
                    }
                }

                // Each thread deal with the range: [lower, upper)
                for (; x < upper; x += x & -x) {
                    bits[x] += val;
                }
            }

            #ifdef TIMING
            auto end_time = omp_get_wtime();
            execution_times[t] += end_time - start_time;
            #endif
            
            // Semi-static scheduling: adjust ranges based on the execution results
            #pragma omp single nowait
            {
                if (ranges[t].first == 1) {
                    if (ranges[t].second + step < (int)bits.size()) {
                        ranges[t].second += step;
                        ranges[t+1].first += step;
                    }
                } else if (ranges[t].second == (int)bits.size()) {
                    if (ranges[t].first - step >= 1) {
                        ranges[t].first -= step;
                        ranges[t-1].second -= step;
                    }
                } else {
                    int b = (ranges[t].first + ranges[t].second) & 1;
                    if (b == 0 && ranges[t].first - step >= 1) {
                        ranges[t].first -= step;
                        ranges[t-1].second -= step;
                    } else if (ranges[t].second + step < (int)bits.size()) {
                        ranges[t].second += step;
                        ranges[t+1].first += step;
                    }
                }
            }
        }
    }

    void printRanges() {
        for (int i = 0; i != num_threads; ++i) {
            std::cerr << "Thread " << i << ' ' << ranges[i].first << ' ' << ranges[i].second << '\n';
        }
    }

    void statistics() {
        for (auto time : execution_times) {
            std::cerr << time << '\n';
        }
    }
};

// Pipeline Fenwick Tree
class FenwickTreePipelineAggregate : FenwickTreeBase {
  private:
    int num_threads;
    std::vector<int> bits;
    std::vector<int> local_bits;
    std::vector<std::pair<int, int>> ranges;
    std::vector<double> execution_times;
    
    void initialize_ranges(int n, int num_threads) {
        std::vector<long> dp(n + 1);
        double total = 0;
        for (int x = 1; x <= n; ++x) {
            ++dp[x];
            total += dp[x];

            int next_x = x;
            next_x += next_x & -next_x;
        }

        // Split the internal array to several subarray and assign to each thread
        double average = total / num_threads;
        int cur = 1;

        for (int i = 0; i != num_threads; ++i) {
            double thread_total = 0;
            ranges[i].first = cur;
            while (cur < (int)bits.size() && thread_total < average) {
                thread_total += dp[cur];
                ++cur;
            }

            if (cur > ranges[i].first && labs(thread_total - average) > labs(thread_total - dp[cur - 1] - average)) {
                --cur;
                thread_total -= dp[cur];
            }

            ranges[i].second = cur;
        }

        ranges.back().second = bits.size();
    }

  public:
    FenwickTreePipelineAggregate(int n, int num_threads):
        num_threads(num_threads),
        bits(n + 1, 0), 
        local_bits(n + 1, 0),
        ranges(num_threads), 
        execution_times(num_threads) {
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
            const auto [lower, upper] = ranges[t];

            #ifdef TIMING
            auto start_time = omp_get_wtime();
            #endif
            for (const auto &operation : operations) {
                int x = operation.index + 1;
                int val = operation.value;

                /** 
                 * Compute the smallest index derived from `x` that lies 
                 * within the target range.
                 *    
                 * This avoids having each thread start from the original
                 * `x` and loop through unnecessarily to locate their valid
                 * range.
                 */
                if (x < lower) {
                    auto highest_diff_bit 
                        = 0x8000000000000000ULL >> __builtin_clzll(x ^ lower);

                    x |= highest_diff_bit;
                    x &= ~(highest_diff_bit - 1);

                    if (x < lower) {
                        x += x & -x;
                    }
                }

                if (x < upper) {
                    local_bits[x] += val;
                }
            }

            for (int x = lower; x < upper; ++x) {
                int next_x = x;
                next_x += x & -x;
                int val_agg = local_bits[x];
                if (next_x < upper) {
                    local_bits[next_x] += val_agg;
                }
                bits[x] += val_agg;
                local_bits[x] = 0;
            }

            #ifdef TIMING
            auto end_time = omp_get_wtime();
            execution_times[t] += end_time - start_time;
            #endif
        }
    }

    void printRanges() {
        for (int i = 0; i != num_threads; ++i) {
            std::cerr << "Thread " << i << ' ' << ranges[i].first << ' ' << ranges[i].second << '\n';
        }
    }

    void statistics() {
        for (auto time : execution_times) {
            std::cerr << time << '\n';
        }
    }
};

#endif