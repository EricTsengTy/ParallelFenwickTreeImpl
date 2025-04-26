/**
 * Centralized scheduler that distribute tasks to all workers. 
 * For each update task, a worker will be assigned to do the work. 
 * For each query task, all worker will be assigned to write back the result to the scheduler. 
 * The scheduler should also try to take work as possible
 */
#ifndef TASK_SCHEDULER_H
#define TASK_SCHEDULER_H

#include <iostream>
#include <thread>
#include <vector>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <functional>
#include <pthread.h>  // for pthread_setaffinity_np
#include <sched.h>    // for CPU_SET, cpu_set_t

#include "fenwick.h"
#include "readerwriterqueue.h"
#include "generator.h"

using namespace moodycamel;

enum class TaskType { Update, Query, Sync, Finish };

struct Task {
    TaskType type;
    int index;
    int value; // used only for updates
};

struct TaskQueue {
    std::queue<Task> queue;
    std::mutex mutex;
    std::condition_variable cv;
};

void pin_thread_to_core(int core_id) {
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(core_id, &cpuset);
    int rc = pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);
    if (rc != 0) {
        std::cerr << "Error calling pthread_setaffinity_np: " << rc << std::endl;
    }
}

class Scheduler {
    public:
    Scheduler(int num_workers, int tree_size, int batch_size)
        : num_workers_(num_workers), tree_size_(tree_size), batch_size_(batch_size), results_(batch_size) {
        local_trees_.reserve(num_workers);
        task_queues_.reserve(num_workers);

        for (int i = 0; i < num_workers_; ++i) {
            local_trees_.emplace_back(FenwickTreeSequential(tree_size_));
            workers_.emplace_back(&Scheduler::worker_loop, this, i, i+1);
            task_queues_.emplace_back(std::make_unique<TaskQueue>());
        }
    }

    void init() {
        for (auto& val : results_) {
            val.store(0, std::memory_order_relaxed);
        }
        sync_ = 0;
    }

    void submit_update(int index, int value) {
        enqueue_task({TaskType::Update, index, value});
    }

    void submit_query(int index, int batch_id) {
        broadcast_task({TaskType::Query, index, batch_id});
    }

    void sync() {
        broadcast_task({TaskType::Sync, 0, 0});
        while (sync_.load() != num_workers_);
        return;
    }

    void shutdown() {
        broadcast_task({TaskType::Finish, 0, 0});
        for (auto& t : workers_) {
            t.join();
        }
    }

    int validate_sum() {
        int res = 0;
        for (int i = 0; i < batch_size_; i++) {
            res += results_[i].load();
        }
        return res;
    }

private:
    std::vector<std::thread> workers_;
    std::vector<std::unique_ptr<TaskQueue>> task_queues_;
    std::vector<std::atomic<int>> results_;
    std::vector<FenwickTreeSequential> local_trees_;
    std::atomic<int> sync_ = 0;
    int num_workers_;
    int tree_size_;
    int batch_size_;
    int counter_ = 0;

    void enqueue_task(Task task) {
        int worker_id = counter_++ % num_workers_;
        auto& q = *task_queues_[worker_id];
        {
            std::lock_guard<std::mutex> lock(q.mutex);
            q.queue.push(task);
        }
        q.cv.notify_one();
    }

    void broadcast_task(Task task) {
        for (int i = 0; i < num_workers_; ++i) {
            auto& q = *task_queues_[i];
            {
                std::lock_guard<std::mutex> lock(q.mutex);
                q.queue.push(task);
            }
            q.cv.notify_one();
        }
    }

    void worker_loop(int worker_id, int core_id) {
        pin_thread_to_core(core_id);

        TaskQueue& q = *task_queues_[worker_id];
        while (true) {
            Task task;
            {
                std::unique_lock<std::mutex> lock(q.mutex);
                q.cv.wait(lock, [&]() { return !q.queue.empty(); });
                task = q.queue.front();
                q.queue.pop();
            }

            if (task.type == TaskType::Update) {
                local_trees_[worker_id].add(task.index, task.value);
            } else if (task.type == TaskType::Query) {
                int result = local_trees_[worker_id].sum(task.index);
                results_[task.value].fetch_add(result, std::memory_order_relaxed);
            } else if (task.type == TaskType::Sync) {
                sync_++;
            } else if (task.type == TaskType::Finish) {
                return;
            }
        }
    }
};

class LockFreeScheduler {
    public:
    LockFreeScheduler(int num_workers, int tree_size, int batch_size)
        : num_workers_(num_workers), tree_size_(tree_size), batch_size_(batch_size), results_(batch_size) {
        local_trees_.reserve(num_workers);
        task_queues_.reserve(num_workers);

        for (int i = 0; i < num_workers_; ++i) {
            local_trees_.emplace_back(FenwickTreeSequential(tree_size_));
            workers_.emplace_back(&LockFreeScheduler::worker_loop, this, i, i+1);
            task_queues_.emplace_back(BlockingReaderWriterQueue<Task>(100));
        }
    }

    void init() {
        for (auto& val : results_) {
            val.store(0, std::memory_order_relaxed);
        }
        sync_ = 0;
    }

    void submit_update(int index, int value) {
        enqueue_task({TaskType::Update, index, value});
    }

    void submit_query(int index, int batch_id) {
        broadcast_task({TaskType::Query, index, batch_id});
    }

    void sync() {
        broadcast_task({TaskType::Sync, 0, 0});
        while (sync_.load() != num_workers_);
        return;
    }

    void shutdown() {
        broadcast_task({TaskType::Finish, 0, 0});
        for (auto& t : workers_) {
            t.join();
        }
    }

    int validate_sum() {
        int res = 0;
        for (int i = 0; i < batch_size_; i++) {
            res += results_[i].load();
        }
        return res;
    }

private:
    std::vector<std::thread> workers_;
    std::vector<BlockingReaderWriterQueue<Task>> task_queues_;
    std::vector<std::atomic<int>> results_;
    std::vector<FenwickTreeSequential> local_trees_;
    std::atomic<int> sync_ = 0;
    int num_workers_;
    int tree_size_;
    int batch_size_;
    int counter_ = 0;

    void enqueue_task(Task task) {
        int worker_id = counter_++ % num_workers_;
        auto& q = task_queues_[worker_id];
        q.enqueue(task);
    }

    void broadcast_task(Task task) {
        for (int i = 0; i < num_workers_; ++i) {
            auto& q = task_queues_[i];
            q.enqueue(task);
        }
    }

    void worker_loop(int worker_id, int core_id) {
        pin_thread_to_core(core_id);

        auto& q = task_queues_[worker_id];
        while (true) {
            Task task;
            q.wait_dequeue(task);

            if (task.type == TaskType::Update) {
                local_trees_[worker_id].add(task.index, task.value);
            } else if (task.type == TaskType::Query) {
                int result = local_trees_[worker_id].sum(task.index);
                results_[task.value].fetch_add(result, std::memory_order_relaxed);
            } else if (task.type == TaskType::Sync) {
                sync_++;
            } else if (task.type == TaskType::Finish) {
                return;
            }
        }
    }
};

class DecentralizedScheduler {
    public:
    DecentralizedScheduler(int num_workers, int batch_size, 
        std::vector<Operation>& operations, std::vector<FenwickTreeSequential>& local_trees)
        : num_workers_(num_workers), batch_size_(batch_size), results_(batch_size) {
        for (int i = 0; i < num_workers_; ++i) {
            workers_.emplace_back(&DecentralizedScheduler::worker_loop, this, i, i+1, std::ref(operations), std::ref(local_trees[i]));
        }
    }

    void sync() {
        for (auto& t : workers_) {
            t.join();
        }
    }

    int validate_sum() {
        int res = 0;
        for (int i = 0; i < batch_size_; i++) {
            res += results_[i].load();
        }
        return res;
    }

private:
    std::vector<std::thread> workers_;
    std::vector<std::atomic<int>> results_;
    int num_workers_;
    int batch_size_;

    void worker_loop(int worker_id, int core_id, std::vector<Operation>& operations, FenwickTreeSequential& local_tree) {
        pin_thread_to_core(core_id);

        int counter = 0;
        for (size_t i = 0; i < batch_size_; ++i) {
            const auto& op = operations[i];
            if (op.command == 'a') {
                if (counter++ % num_workers_ == worker_id) {
                    local_tree.add(op.index, op.value);
                }
            } else {
                int result = local_tree.sum(op.index);
                results_[i].fetch_add(result, std::memory_order_relaxed);
            }
        }
    }
};
#endif