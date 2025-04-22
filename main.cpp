#include <chrono>
#include <fstream>
#include <iostream>
#include <vector>
#include <unistd.h>

#include "fenwick.h"
#include "generator.h"

void print_help(int argc, char *argv[]) {
    std::cout << "Usage: " << argv[0] << " [options]\n\n"
              << "Options:\n"
              << "  -t <strategy>     Execution strategy (default: sequential)\n"
              << "  -p <threads>      Number of OpenMP threads to use (default: available hardware threads)\n"
              << "  -b <size>         Batch size (default: 65536)\n"
              << "  -n <count>        Number of batches (default: 1024)\n"
              << "  -s <size>         Total size of data (default: 1048575 = 2^10 - 1)\n"
              << "\n"
              << "Example:\n"
              << "  " << argv[0] << " -t parallel -p 4 -b 8192 -n 512 -s 2097152\n"
              << "  " << argv[0] << " -t pipeline -p 8 -b 8192 -n 2048 -s 2097152\n";
    
    exit(1);  // Exit with error code
}

int main(int argc, char* argv[]) {
    std::string strategy = "sequential";
    size_t num_threads = 1;
    size_t size = (1 << 20) - 1;
    size_t batch_size = (1 << 16);
    size_t num_batches = 1024;

    int opt;
    while ((opt = getopt(argc, argv, "t:p:b:n:s:h")) != -1) {
        switch (opt) {
            case 't':
                strategy = optarg;
                if (strategy != "sequential" && strategy != "lock" && strategy != "pipeline") {
                    std::cerr << "Error: Invalid strategy. Must be 'sequential', 'lock', or 'pipeline'.\n";
                    print_help(argc, argv);
                }
                break;
            case 'p':
                num_threads = std::stoi(optarg);
                break;
            case 'b':
                batch_size = std::stoi(optarg);
                break;
            case 'n':
                num_batches = std::stoi(optarg);
                break;
            case 's':
                size = std::stoi(optarg);
                break;
            case 'h':
            default:
                print_help(argc, argv);
        }
    }

    size_t num_operations = batch_size * num_batches;

    
    Generator generator(size, 0);
    std::vector<Operation> operations(batch_size);

    // Run sequential version
    if (strategy == "sequential") {
        FenwickTreeSequential fenwick_tree(size);

        std::chrono::microseconds generating_duration(0);
        auto start_time = std::chrono::steady_clock::now();

        for (size_t batch_start = 0; batch_start < num_operations; batch_start += batch_size) {
            auto generating_start_time = std::chrono::steady_clock::now();
            for (size_t i = 0; i < batch_size; ++i) {
                operations[i] = generator.next();
            }
            auto generating_end_time = std::chrono::steady_clock::now();
            generating_duration += std::chrono::duration_cast<std::chrono::microseconds>(
                generating_end_time - generating_start_time
            );

            fenwick_tree.batchAdd(operations);
        }
        
        auto end_time = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
        
        std::cout << "Performance:" << std::endl;
        std::cout << "Total operations: " << num_operations << std::endl;
        std::cout << "Total execution time: " << duration.count() << " microseconds" << std::endl;
        std::cout << "Total data generating time: " << generating_duration.count() << " microseconds" << std::endl;
        std::cout << "Total computation time: " << (duration - generating_duration).count() << " microseconds" << std::endl;
        std::cout << "Batch computation time: " << (duration - generating_duration).count() / num_batches << " microseconds" << std::endl;
        std::cout << "Average time per operation: " << (duration.count() / num_operations) << " microseconds" << std::endl;
        std::cout << std::endl;

    } else if (strategy == "lock") {
        FenwickTreeLocked fenwick_tree(size);
        auto start_time = std::chrono::steady_clock::now();
        
        std::chrono::microseconds generating_duration(0);
        // Only parallelize if we have enough operations
        for (size_t batch_start = 0; batch_start < num_operations; batch_start += batch_size) {
            auto generating_start_time = std::chrono::steady_clock::now();
            for (size_t i = 0; i < batch_size; ++i) {
                operations[i] = generator.next();
            }
            auto generating_end_time = std::chrono::steady_clock::now();
            generating_duration += std::chrono::duration_cast<std::chrono::microseconds>(
                generating_end_time - generating_start_time
            );

            #pragma omp parallel for num_threads(2)
            for (size_t i = 0; i < batch_size; ++i) {
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
        
        std::cout << "Performance:" << std::endl;
        std::cout << "Total operations: " << num_operations << std::endl;
        std::cout << "Total execution time: " << duration.count() << " microseconds" << std::endl;
        std::cout << "Total data generating time: " << generating_duration.count() << " microseconds" << std::endl;
        std::cout << "Total computation time: " << (duration - generating_duration).count() << " microseconds" << std::endl;
        std::cout << "Batch computation time: " << (duration - generating_duration).count() / num_batches << " microseconds" << std::endl;
        std::cout << "Average time per operation: " << (duration.count() / num_operations) << " microseconds" << std::endl;
        std::cout << std::endl;

    } else if (strategy == "pipeline") {
        omp_set_num_threads(num_threads);

        FenwickTreePipeline fenwick_tree(size, omp_get_max_threads());

        std::chrono::microseconds generating_duration(0);
        auto start_time = std::chrono::steady_clock::now();

        for (size_t batch_start = 0; batch_start < num_operations; batch_start += batch_size) {
            auto generating_start_time = std::chrono::steady_clock::now();
            for (size_t i = 0; i < batch_size; ++i) {
                operations[i] = generator.next();
            }
            auto generating_end_time = std::chrono::steady_clock::now();
            generating_duration += std::chrono::duration_cast<std::chrono::microseconds>(
                generating_end_time - generating_start_time
            );

            fenwick_tree.batchAdd(operations);
        }
        
        auto end_time = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);

        std::cout << "Performance:" << std::endl;
        std::cout << "Total operations: " << num_operations << std::endl;
        std::cout << "Total execution time: " << duration.count() << " microseconds" << std::endl;
        std::cout << "Total data generating time: " << generating_duration.count() << " microseconds" << std::endl;
        std::cout << "Total computation time: " << (duration - generating_duration).count() << " microseconds" << std::endl;
        std::cout << "Batch computation time: " << (duration - generating_duration).count() / num_batches << " microseconds" << std::endl;
        std::cout << "Average time per operation: " << (duration.count() / num_operations) << " microseconds" << std::endl;
        std::cout << std::endl;
    }

    return 0;
}