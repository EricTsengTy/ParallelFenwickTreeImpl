#include <iostream>
#include <fstream>
#include <vector>
#include <chrono>

#include "fenwick.h"
#include "generator.h"

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <algorithm>" << std::endl;
        return 1;
    }

    size_t size = 16384;
    size_t batch_size = 1024;
    size_t num_batches = 16384;
    size_t num_operations = batch_size * num_batches;
    
    Generator generator(size);
    std::vector<Operation> operations(batch_size);

    // Run sequential version
    if (argv[1][0] == 's') {
        FenwickTreeSequential fenwick_tree(size);
        auto start_time = std::chrono::steady_clock::now();
        
        for (size_t batch_start = 0; batch_start < num_operations; batch_start += batch_size) {
            for (int i = 0; i < batch_size; ++i) {
                operations[i] = generator.next();
            }

            for (int i = 0; i < batch_size; ++i) {
                const auto& op = operations[i];
                if (op.command == 'a') {
                    fenwick_tree.add(op.index, op.value);
                } else {
                    int result = fenwick_tree.sum(op.index);
                }
            }
        }

        
        auto end_time = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
        
        std::cout << "Sequential Performance:" << std::endl;
        std::cout << "Total operations: " << num_operations << std::endl;
        std::cout << "Total execution time: " << duration.count() << " microseconds" << std::endl;
        std::cout << "Average time per operation: " << (duration.count() / num_operations) << " microseconds" << std::endl;
        std::cout << std::endl;
    }

    if (argv[1][0] == 'l') {
        FenwickTreeLocked fenwick_tree(size);
        auto start_time = std::chrono::steady_clock::now();
        
        // Only parallelize if we have enough operations
        for (size_t batch_start = 0; batch_start < num_operations; batch_start += batch_size) {
            for (int i = 0; i < batch_size; ++i) {
                operations[i] = generator.next();
            }

            #pragma omp parallel for num_threads(2)
            for (int i = 0; i < batch_size; ++i) {
                const auto& op = operations[i];
                if (op.command == 'a') {
                    fenwick_tree.add(op.index, op.value);
                } else {
                    int result = fenwick_tree.sum(op.index);
                }
            }
        }

        
        auto end_time = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
        
        std::cout << "Atomic with Padding Performance:" << std::endl;
        std::cout << "Total operations: " << num_operations << std::endl;
        std::cout << "Total execution time: " << duration.count() << " microseconds" << std::endl;
        std::cout << "Average time per operation: " << (duration.count() / num_operations) << " microseconds" << std::endl;
        std::cout << std::endl;
    }

    return 0;
}