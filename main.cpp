#include <chrono>
#include <fstream>
#include <iostream>
#include <vector>
#include <memory>
#include <unistd.h>

#include "fenwick.h"
#include "task_scheduler.h"
#include "generator.h"

void print_help(int argc, char *argv[]) {
    std::cout << "Usage: " << argv[0] << " [options]\n\n"
              << "Options:\n"
              << "  -t <strategy>     Execution strategy (default: sequential)\n"
              << "  -p <threads>      Number of OpenMP threads to use (default: available hardware threads)\n"
              << "  -b <size>         Batch size (default: 65536)\n"
              << "  -n <count>        Number of batches (default: 1024)\n"
              << "  -s <size>         Total size of data (default: 1048576)\n"
              << "\n"
              << "Example:\n"
              << "  " << argv[0] << " -t parallel -p 4 -b 8192 -n 512 -s 2097152\n"
              << "  " << argv[0] << " -t pipeline -p 8 -b 8192 -n 2048 -s 2097152\n";
    
    exit(1);  // Exit with error code
}

std::unique_ptr<FenwickTreeBase> CreateFenwickTree(const std::string& type, int n, size_t num_threads) {
    if (type == "sequential") {
        return std::make_unique<FenwickTreeSequential>(n);
    }
    if (type == "lock") {
        return std::make_unique<FenwickTreeLocked>(n);
    }
    if (type == "pipeline") {
        omp_set_num_threads(num_threads);
        return std::make_unique<FenwickTreePipeline>(n, omp_get_max_threads());
    }
    if (type == "lazy") {
        return std::make_unique<FenwickTreeLSync>(n);
    }
    if (type == "within") {
        omp_set_num_threads(num_threads);
        return std::make_unique<FenwickTreeLWithin>(n, omp_get_max_threads());
    }
    throw std::invalid_argument("Unknown tree type");
}

int main(int argc, char* argv[]) {
    std::string strategy = "sequential";
    size_t num_threads = 1;
    size_t size = (1 << 25);
    size_t batch_size = (1 << 16);
    size_t num_batches = 1024;
    size_t num_operations = batch_size * num_batches;

    int opt;
    while ((opt = getopt(argc, argv, "t:p:b:n:s:h")) != -1) {
        switch (opt) {
            case 't':
                strategy = optarg;
                if (strategy != "sequential" && strategy != "lock" && strategy != "pipeline" && strategy != "lazy" && strategy != "within" && strategy != "parallel_task") {
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
    
    Generator generator(size, 1);
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

            // fenwick_tree.batchAdd(operations);
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

        // fenwick_tree.statistics();
        
        std::cout << "Performance:" << std::endl;
        std::cout << "Total operations: " << num_operations << std::endl;
        std::cout << "Total execution time: " << duration.count() << " microseconds" << std::endl;
        std::cout << "Total data generating time: " << generating_duration.count() << " microseconds" << std::endl;
        std::cout << "Total computation time: " << (duration - generating_duration).count() << " microseconds" << std::endl;
        std::cout << "Average time per operation: " << (duration.count() / num_operations) << " microseconds" << std::endl;
        std::cout << std::endl;
    } else if (strategy == "lazy")  {
        omp_set_num_threads(num_threads);
        std::string base_strategy = "sequential";
        std::unique_ptr<FenwickTreeBase> base_tree = CreateFenwickTree(base_strategy, size, num_threads);
        std::unique_ptr<FenwickTreeBase> test_tree = CreateFenwickTree(strategy, size, num_threads);
        double test_time = 0;
        double sequential_time = 0;
        auto start_time = std::chrono::steady_clock::now();

        for (size_t batch_start = 0; batch_start < num_operations; batch_start += batch_size) {
            int seq_res = 0;
            int test_res = 0;

            for (size_t i = 0; i < batch_size; ++i) {
                operations[i] = generator.next();
            }

            start_time = std::chrono::steady_clock::now();
            for (size_t i = 0; i < batch_size; ++i) {
                const auto& op = operations[i];
                if (op.command == 'a') {
                    base_tree->add(op.index, op.value);
                } else {
                    seq_res += base_tree->sum(op.index);
                }
            }
            sequential_time += std::chrono::duration_cast<std::chrono::duration<double>>(std::chrono::steady_clock::now() - start_time).count();

            start_time = std::chrono::steady_clock::now();
            size_t left = 0;
            for (size_t right = 0; right < batch_size; right++) {
                const auto& op = operations[right];
                if (op.command == 'q') {
                    #pragma omp parallel for
                    for (size_t i = left; i < right; i++) {
                        test_tree->add(operations[i].index, operations[i].value);                        
                    }
                    test_res += test_tree->sum(op.index);
                    left = right + 1;
                }
            }
            #pragma omp parallel for
            for (size_t i = left; i < batch_size; i++) {
                test_tree->add(operations[i].index, operations[i].value);                        
            }
            test_time += std::chrono::duration_cast<std::chrono::duration<double>>(std::chrono::steady_clock::now() - start_time).count();
            if (seq_res != test_res) {
                std::cout << "output diff at batch: " << batch_start << " t: " << test_res << " s: " << seq_res << std::endl;
                return -1;
            }
        }
        
        std::cout << "Performance:" << std::endl;
        std::cout << "Total operations: " << num_operations << std::endl;
        std::cout << "Seq time: " << sequential_time << " microseconds" << std::endl;
        std::cout << "Test Algo time: " << test_time << " microseconds" << std::endl; 
        std::cout << std::endl;
    }  else if (strategy == "parallel_task")  {
        std::string base_strategy = "sequential";
        std::unique_ptr<FenwickTreeBase> base_tree = CreateFenwickTree(base_strategy, size, num_threads);

        Scheduler scheduler = Scheduler(num_threads, size, batch_size);
        double test_time = 0;
        double sequential_time = 0;
        auto start_time = std::chrono::steady_clock::now();

        for (size_t batch_start = 0; batch_start < num_operations; batch_start += batch_size) {
            int seq_res = 0;
            int test_res = 0;

            for (size_t i = 0; i < batch_size; ++i) {
                operations[i] = generator.next();
            }

            start_time = std::chrono::steady_clock::now();
            for (size_t i = 0; i < batch_size; ++i) {
                const auto& op = operations[i];
                if (op.command == 'a') {
                    base_tree->add(op.index, op.value);
                } else {
                    seq_res += base_tree->sum(op.index);
                }
            }
            sequential_time += std::chrono::duration_cast<std::chrono::duration<double>>(std::chrono::steady_clock::now() - start_time).count();

            start_time = std::chrono::steady_clock::now();
            for (size_t i = 0; i < batch_size; ++i) {
                const auto& op = operations[i];
                if (op.command == 'a') {
                    scheduler.submit_update(op.index, op.value);
                } else {
                    scheduler.submit_query(op.index);
                }
            }
            test_time += std::chrono::duration_cast<std::chrono::duration<double>>(std::chrono::steady_clock::now() - start_time).count();
            test_res = scheduler.validate_sum();
            if (seq_res != test_res) {
                std::cout << "output diff at batch: " << batch_start << " t: " << test_res << " s: " << seq_res << std::endl;
                return -1;
            }
        }
        
        std::cout << "Performance:" << std::endl;
        std::cout << "Total operations: " << num_operations << std::endl;
        std::cout << "Seq time: " << sequential_time << " microseconds" << std::endl;
        std::cout << "Test Algo time: " << test_time << " microseconds" << std::endl; 
        std::cout << std::endl;
    }

    return 0;
}