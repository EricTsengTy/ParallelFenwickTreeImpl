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
#include <future>
#include <chrono>

#include "locking_queue.h"
#include "fenwick.h"

enum class TaskType { Update, Query };

struct Task {
    TaskType type;
    int index;
    int value; // used only for updates
};

class Scheduler {
    public:
    Scheduler(int num_workers, int tree_size, int batch_size)
        : num_workers_(num_workers), tree_size_(tree_size), batch_size_(batch_size), results_(batch_size + 1) {
        local_trees_.reserve(num_workers);
        // results_.reserve(batch_size);
        // for (int i = 0; i < num_workers; i++) {
            // results_.emplace_back();
        // }

        for (int i = 0; i < num_workers_; ++i) {
            local_trees_.emplace_back(FenwickTreeSequential(tree_size_));
            workers_.emplace_back(&Scheduler::worker_loop, this, i);
            task_queues_.emplace_back(std::make_unique<LockingQueue<Task>>());
        }
        // TODO: do we need a master thread?
    }

    ~Scheduler() {
        for (int i = 0; i < num_workers_; ++i) {
            task_queues_[i]->close();
        }
        for (auto& t : workers_) {
            t.join();
        }
    }

    void submit_update(int index, int value) {
        // Task task{TaskType::Update, index, value};
        enqueue_task({TaskType::Update, index, value});
    }

    void submit_query(int index) {
        Task task{TaskType::Query, index, 0};
        // TODO: use future to check if query finished?
        broadcast_task(std::move(task));
    }

    int validate_sum() {
        int res = 0;
        for (int i = 0; i < batch_size_; i++) {
            res += results_[i].load();
        }
    }

private:
    std::vector<std::thread> workers_;
    // std::thread scheduler_thread_;
    std::vector<std::unique_ptr<LockingQueue<Task>>> task_queues_;
    std::vector<std::atomic<int>> results_;
    std::vector<FenwickTreeSequential> local_trees_;
    int num_workers_;
    int tree_size_;
    int batch_size_;
    std::atomic<int> round_robin_counter_ = 0;

    void enqueue_task(Task task) {
        int worker_id = round_robin_counter_++ % num_workers_;
        task_queues_[worker_id]->push(std::move(task));
    }

    void broadcast_task(const Task& task) {
        for (int i = 0; i < num_workers_; i++) {
            task_queues_[i]->push(task);
        }
    }

    void worker_loop(int worker_id) {
        while (!task_queues_[worker_id]->is_closed()) {
            Task task = task_queues_[worker_id]->pop();

            if (task.type == TaskType::Update) {
                local_trees_[worker_id].add(task.index, task.value);
            } else if (task.type == TaskType::Query) {
                int result = 0;
                for (int i = 0; i < num_workers_; ++i) {
                    result += local_trees_[i].sum(task.index);
                }
                results_[task.index].fetch_add(result, std::memory_order_relaxed);
            }
        }
    }
};

#endif