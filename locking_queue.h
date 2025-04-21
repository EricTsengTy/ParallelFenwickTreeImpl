#ifndef LOCKINGQUEUE_H
#define LOCKINGQUEUE_H

#include <condition_variable>
#include <mutex>
#include <string>
#include <queue>
#include <thread>
using namespace std;

template<typename T>
class LockingQueue {
public:
    explicit LockingQueue(size_t max_length_ = 10000) {
        this->max_length_ = max_length_;
        queue_ = new queue<T>();
    }

    LockingQueue(const LockingQueue&) = delete;
    LockingQueue& operator=(const LockingQueue&) = delete;
    LockingQueue(LockingQueue&&) = delete;
    LockingQueue& operator=(LockingQueue&&) = delete;


    ~LockingQueue() {
        lock_guard<mutex> locker(m);
        delete queue_;
        closed = true;
        producer_cv_.notify_all();
        consumer_cv_.notify_all();
    }

    void close() {
        while (!empty()) {
            consumer_cv_.notify_one();
        }
        closed = true;
        producer_cv_.notify_all();
        consumer_cv_.notify_all();
    }

    bool is_closed() {
        return closed;
    }

    bool empty() {
        lock_guard<mutex> locker(m);
        return queue_->empty();
    }

    void push(T value) {
        {
            unique_lock<mutex> locker(m);
            // queue full now, wait for flush
            while (queue_->size() == max_length_) {
                producer_cv_.wait(locker);
            }

            queue_->emplace(value);
        }
        consumer_cv_.notify_one();
    }

    T pop() { 
        unique_lock<mutex> locker(m);
        while (queue_->empty()) {
            consumer_cv_.wait(locker, [this]{return closed || !queue_->empty();});
            // if (closed) {
            //     return false;
            // }
        }
        T rc(std::move(queue_->front()));
        queue_->pop();
        producer_cv_.notify_one();
        return rc;
    }

private:
    queue<T> *queue_;
    size_t max_length_;
    condition_variable producer_cv_, consumer_cv_;
    mutex m;
    bool closed = false;
};


#endif //LOCKINGQUEUE_H