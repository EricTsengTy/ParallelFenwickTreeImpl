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
              << "  -p <threads>      Number of OpenMP threads to use (default: 1)\n"
              << "  -b <size>         Batch size (default: 65536)\n"
              << "  -n <count>        Number of batches (default: 1024)\n"
              << "  -s <size>         Total size of data (default: 1048575 = 2^10 - 1)\n"
              << "\n"
              << "Strategies:\n"
              << "  sequential, lock, pipeline, pipeline-semi-static, pipeline-aggregate, lazy,\n"
              << "  central_scheduler, lockfree_scheduler, pure_parallel, query_percentage_lazy,\n"
              << "  query_percentage_pure\n"
              << "\n"
              << "Examples:\n"
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
    throw std::invalid_argument("Unknown tree type");
}

int main(int argc, char* argv[]) {
    std::string strategy = "sequential";
    size_t num_threads = 1;
    size_t size = (1 << 16);
    size_t batch_size = (1 << 16);
    size_t num_batches = 1024;

    int opt;
    while ((opt = getopt(argc, argv, "t:p:b:n:s:h")) != -1) {
        switch (opt) {
            case 't':
                strategy = optarg;
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

    omp_set_num_threads(num_threads);
    size_t num_operations = batch_size * num_batches;
    
    Generator generator(size, 0, 15618);
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

            #pragma omp parallel for
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
    } else if (strategy == "pipeline-semi-static") {
        FenwickTreePipelineSemiStatic fenwick_tree(size, omp_get_max_threads());

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
    } else if (strategy == "pipeline-aggregate") {
        FenwickTreePipelineAggregate fenwick_tree(size, omp_get_max_threads());

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
    }  else if (strategy == "lazy")  {
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
    } else if (strategy == "central_scheduler")  {
        std::string base_strategy = "sequential";
        std::unique_ptr<FenwickTreeBase> base_tree = CreateFenwickTree(base_strategy, size, num_threads);
        Scheduler scheduler = Scheduler(num_threads-1, size, batch_size);

        double test_time = 0;
        double sequential_time = 0;
        double schedule_time = 0;
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

            scheduler.init();
            start_time = std::chrono::steady_clock::now();  
            for (size_t i = 0; i < batch_size; ++i) {
                const auto& op = operations[i];
                if (op.command == 'a') {
                    scheduler.submit_update(op.index, op.value);
                } else {
                    scheduler.submit_query(op.index, i);
                }
            }
            schedule_time += std::chrono::duration_cast<std::chrono::duration<double>>(std::chrono::steady_clock::now() - start_time).count(); 

            scheduler.sync();
            test_time += std::chrono::duration_cast<std::chrono::duration<double>>(std::chrono::steady_clock::now() - start_time).count();
            test_res = scheduler.validate_sum();
            if (seq_res != test_res) {
                std::cout << "output diff at batch: " << batch_start << " t: " << test_res << " s: " << seq_res << std::endl;
                return -1;
            }
        }
        scheduler.shutdown();
        
        std::cout << "Performance:" << std::endl;
        std::cout << "Worker threads: " << num_threads - 1 << std::endl;
        std::cout << "Seq time: " << sequential_time << " seconds" << std::endl;
        std::cout << "Central Scheduler time: " << test_time << " seconds" << std::endl; 
        std::cout << "Speedup: " << sequential_time / test_time << "x" << std::endl; 
        std::cout << std::endl;
    } else if (strategy == "lockfree_scheduler")  {
        std::string base_strategy = "sequential";
        std::unique_ptr<FenwickTreeBase> base_tree = CreateFenwickTree(base_strategy, size, num_threads);
        LockFreeScheduler scheduler = LockFreeScheduler(num_threads-1, size, batch_size);

        double test_time = 0;
        double sequential_time = 0;
        double schedule_time = 0;
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

            scheduler.init();
            start_time = std::chrono::steady_clock::now();  
            for (size_t i = 0; i < batch_size; ++i) {
                const auto& op = operations[i];
                if (op.command == 'a') {
                    scheduler.submit_update(op.index, op.value);
                } else {
                    scheduler.submit_query(op.index, i);
                }
            }
            schedule_time += std::chrono::duration_cast<std::chrono::duration<double>>(std::chrono::steady_clock::now() - start_time).count(); 

            scheduler.sync();
            test_time += std::chrono::duration_cast<std::chrono::duration<double>>(std::chrono::steady_clock::now() - start_time).count();
            test_res = scheduler.validate_sum();
            if (seq_res != test_res) {
                std::cout << "output diff at batch: " << batch_start << " t: " << test_res << " s: " << seq_res << std::endl;
                return -1;
            }
        }
        scheduler.shutdown();
        
        std::cout << "Performance:" << std::endl;
        std::cout << "Worker threads: " << num_threads - 1 << std::endl;
        std::cout << "Seq time: " << sequential_time << " seconds" << std::endl;
        std::cout << "Lockfree Scheduler time: " << test_time << " seconds" << std::endl; 
        std::cout << "Speedup: " << sequential_time / test_time << "x" << std::endl; 
        std::cout << std::endl;
    } else if (strategy == "pure_parallel")  {
        std::string base_strategy = "sequential";
        std::unique_ptr<FenwickTreeBase> base_tree = CreateFenwickTree(base_strategy, size, num_threads);
        std::vector<FenwickTreeSequential> local_trees;
        local_trees.reserve(num_threads - 1);
        for (size_t i = 0; i < num_threads; ++i) {
            local_trees.emplace_back(FenwickTreeSequential(size));
        }

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
            DecentralizedScheduler scheduler = DecentralizedScheduler(num_threads - 1, batch_size, operations, local_trees);
            scheduler.sync();
            test_time += std::chrono::duration_cast<std::chrono::duration<double>>(std::chrono::steady_clock::now() - start_time).count();
            test_res = scheduler.validate_sum();
            if (seq_res != test_res) {
                std::cout << "output diff at batch: " << batch_start << " t: " << test_res << " s: " << seq_res << std::endl;
                return -1;
            }
        }
        
        std::cout << "Performance:" << std::endl;
        std::cout << "Num threads: " << num_threads - 1 << std::endl;
        std::cout << "Seq time: " << sequential_time << " seconds" << std::endl;
        std::cout << "Pure Parallel time: " << test_time << " seconds" << std::endl;
        std::cout << "Speedup: " << sequential_time / test_time << "x" << std::endl;
        std::cout << std::endl;
    } else if (strategy == "query_percentage_lazy") {
        std::vector<int> query_percentages = {0, 1, 5, 10, 50, 100, 500, 1000};
        for (auto q_percentage : query_percentages) {
            generator = Generator(size, q_percentage);
            std::string base_strategy = "sequential";
            std::string lazy_strategy = "lazy";
            std::unique_ptr<FenwickTreeBase> base_tree = CreateFenwickTree(base_strategy, size, num_threads);
            std::unique_ptr<FenwickTreeBase> lazy_tree = CreateFenwickTree(lazy_strategy, size, num_threads);
    
            double lazy_time = 0;
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
                            lazy_tree->add(operations[i].index, operations[i].value);                        
                        }
                        test_res += lazy_tree->sum(op.index);
                        left = right + 1;
                    }
                }
                #pragma omp parallel for
                for (size_t i = left; i < batch_size; i++) {
                    lazy_tree->add(operations[i].index, operations[i].value);                        
                }
                lazy_time += std::chrono::duration_cast<std::chrono::duration<double>>(std::chrono::steady_clock::now() - start_time).count();
            }
            
            std::cout << "Performance:" << std::endl;
            std::cout << "Query Percentage: " << q_percentage / 10.0 << "%" << std::endl;
            std::cout << "Seq time: " << sequential_time << " seconds" << std::endl;
            std::cout << "Lazy time: " << lazy_time << " seconds" << std::endl;
            std::cout << "Lazy Speedup: " <<  sequential_time / lazy_time<< "x" << std::endl;
            std::cout << std::endl;
        }
    }  else if (strategy == "query_percentage_pure") {
        std::vector<int> query_percentages = {0, 1, 5, 10, 50, 100, 500, 1000};
        for (auto q_percentage : query_percentages) {
            generator = Generator(size, q_percentage);
            std::string base_strategy = "sequential";
            std::unique_ptr<FenwickTreeBase> base_tree = CreateFenwickTree(base_strategy, size, num_threads);
            std::vector<FenwickTreeSequential> local_trees;
            local_trees.reserve(num_threads - 1);
            for (size_t i = 0; i < num_threads; ++i) {
                local_trees.emplace_back(FenwickTreeSequential(size));
            }
    
            double parallel_time = 0;
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
                DecentralizedScheduler scheduler = DecentralizedScheduler(num_threads - 1, batch_size, operations, local_trees);
                scheduler.sync();
                parallel_time += std::chrono::duration_cast<std::chrono::duration<double>>(std::chrono::steady_clock::now() - start_time).count();
                test_res = scheduler.validate_sum();
            }
            
            std::cout << "Performance:" << std::endl;
            std::cout << "Query Percentage: " << q_percentage / 10.0 << "%" << std::endl;
            std::cout << "Seq time: " << sequential_time << " seconds" << std::endl;
            std::cout << "Para time: " << parallel_time << " seconds" << std::endl; 
            std::cout << "Parallel Speedup: " << sequential_time / parallel_time << "x" << std::endl;
            std::cout << std::endl;
        }
    } else {
        print_help(argc, argv);
    }

    return 0;
}