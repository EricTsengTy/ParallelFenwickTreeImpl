#include <iostream>
#include <fstream>
#include <vector>
#include <chrono>

#include "fenwick.h"

struct Operation {
    char command;
    int index;
    int value;
};

int main(int argc, char* argv[]) {
    if (argc < 3) {
        std::cerr << "Usage: " << argv[0] << " <algorithm> <input_file>" << std::endl;
        return 1;
    }

    std::ifstream infile(argv[2]);
    if (!infile) {
        std::cerr << "Error: Could not open file " << argv[1] << std::endl;
        return 1;
    }

    int size, num_operations;
    infile >> size >> num_operations;
    
    std::vector<Operation> operations(num_operations);

    for (int i = 0; i < num_operations; ++i) {
        auto& op = operations[i];
        infile >> op.command;
        if (op.command == 'a') {
            // Add command: a index value
            infile >> op.index >> op.value;
        } else if (op.command == 'q') {
            // Query command (if you want to support it): q index
            infile >> op.index;
        } else {
            std::cerr << "Unknown command: " << op.command << std::endl;
        }
    }

    // Run sequential version
    if (argv[1][0] == 's') {
        FenwickTreeSequential fenwick_tree(size);
        auto start_time = std::chrono::steady_clock::now();
        
        for (int i = 0; i < num_operations; ++i) {
            const auto& op = operations[i];
            if (op.command == 'a') {
                fenwick_tree.add(op.index, op.value);
            } else {
                int result = fenwick_tree.sum(op.index);
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
        #pragma omp parallel for num_threads(8)
        for (int i = 0; i < num_operations; ++i) {
            const auto& op = operations[i];
            if (op.command == 'a') {
                fenwick_tree.add(op.index, op.value);
            } else {
                int result = fenwick_tree.sum(op.index);
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