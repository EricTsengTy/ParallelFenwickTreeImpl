# Parallel Fenwick Tree Implementation

This program allows executing a Fenwick Tree in parallel using various concurrency and processing strategies. You can control the number of threads, batch size, and the specific strategy used for execution via command-line options.

- Task Parallelism Optimizations:
    - Lazy Sync
    - Central Scheduler
    - Lock-free Scheduler
    - Pure Parallelism
- Batch-Add Optimizations:
    - Lock
    - Model-Parallel Fixed-Size
    - Model-Parallel Access-Aware
    - Model-Parallel Semi-Static
    - Model-Parallel Aggregate

For more details, check our [paper](https://erictsengty.github.io/ParallelFenwickTree/assets/pdfs/final_report.pdf)!

## Instructions

### Build
```shell
$ make clean
$ make
```

### Run
```
Usage: ./fenwick [options]

Options:
  -t <strategy>     Execution strategy (default: sequential)
  -p <threads>      Number of OpenMP threads to use (default: 1)
  -b <size>         Batch size (default: 65536)
  -n <count>        Number of batches (default: 1024)
  -s <size>         Total size of data (default: 1048575 = 2^10 - 1)

Strategies:
  sequential, lock, pipeline-fixed-size, pipeline-access-aware, 
  pipeline-semi-static, pipeline-aggregate, lazy, central_scheduler, 
  lockfree_scheduler, pure_parallel, query_percentage_lazy, 
  query_percentage_pure

Examples:
  ./fenwick -t parallel -p 4 -b 8192 -n 512 -s 2097152
  ./fenwick -t pipeline -p 8 -b 8192 -n 2048 -s 2097152
```

## Benchmark
The GHC performance benchmark and Query Frequency benchmark of task parallelism optimizations can be reproduced by running:
```shell
$ ./task_parallel_bench.sh
```

The GHC performance benchmark of batch-add optimizations can be reproduced by running:
```shell
$ ./model_parallel_bench.sh
```
