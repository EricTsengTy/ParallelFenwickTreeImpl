#include <mutex>
#include <shared_mutex>
#include <atomic>
#include <omp.h>

// Original sequential Fenwick Tree
struct FTree {
private:
    std::vector<int> bits;

public:
    FTree(int n):bits(n + 1, 0){}

    void add(int x, int val) {
        for (++x; x < bits.size(); x += x & -x) {
            bits[x] += val;
        }
    }

    int sum(int x) {
        int total = 0;
        for (++x; x > 0; x -= x & -x) {
            total += bits[x];
        }
        return total;
    }
};

// Improved parallel Fenwick Tree
struct FenwickTreeParallel {
private:
    // Padding to avoid false sharing (assuming 64-byte cache lines)
    struct alignas(64) PaddedAtomic {
        std::atomic<int> value;
        // Padding will be added automatically by alignas
        
        PaddedAtomic() : value(0) {}
    };
    
    std::vector<PaddedAtomic> bits;
    const int num_locks;
    std::vector<std::mutex> locks; // Finer-grained locks

public:
    FenwickTreeParallel(int n) : bits(n + 1), num_locks(std::min(32, n+1)), locks(num_locks) {
        // Already initialized to 0 in constructor
    }

    // Thread-safe add operation with local lock
    void add(int x, int val) {
        ++x;
        if (x < bits.size()) {
            // Acquire lock for this index's region
            int lock_idx = x % num_locks;
            std::lock_guard<std::mutex> guard(locks[lock_idx]);
            
            // Update the tree
            for (int i = x; i < bits.size(); i += i & -i) {
                bits[i].value.fetch_add(val, std::memory_order_relaxed);
            }
        }
    }

    // Thread-safe sum operation (no locks needed with atomics)
    int sum(int x) {
        int total = 0;
        for (++x; x > 0; x -= x & -x) {
            total += bits[x].value.load(std::memory_order_relaxed);
        }
        return total;
    }
};

// Parallel Fenwick Tree with batch processing
struct FenwickTreeBatch {
private:
    std::vector<int> bits;
    const int batch_size;
    mutable std::mutex tree_mutex;

public:
    FenwickTreeBatch(int n) : bits(n + 1, 0), batch_size(1000) {}

    // Process a batch of add operations at once
    void batch_add(const std::vector<std::pair<int, int>>& operations) {
        // Lock the entire tree for the batch
        std::lock_guard<std::mutex> guard(tree_mutex);
        
        for (const auto& [x, val] : operations) {
            int idx = x + 1;
            for (int i = idx; i < bits.size(); i += i & -i) {
                bits[i] += val;
            }
        }
    }

    int sum(int x) {
        std::lock_guard<std::mutex> guard(tree_mutex);
        int total = 0;
        for (++x; x > 0; x -= x & -x) {
            total += bits[x];
        }
        return total;
    }
};