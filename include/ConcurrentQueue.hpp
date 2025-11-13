//
// Created by fjasis on 11/13/25.
//

#ifndef OPTIPRO_CNC_CONCURRENTQUEUE_HPP
#define OPTIPRO_CNC_CONCURRENTQUEUE_HPP
#include <condition_variable>
#include <mutex>
#include <optional>
#include <queue>

template <typename  T>
class ConcurrentQueue {
    public:
    void push(T value) {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            queue_.push(std::move(value));
        }
        cv_.notify_one();
    }
    //blocking pop
    std::optional<T> pop_for(std::chrono::milliseconds timeout) {
        std::unique_lock<std::mutex> lock(mutex_);
        if (!cv_.wait_for(lock,timeout,  [this] {return ! queue.empty();})) {
            return std::nullopt;
        }
        T value = std::move(queue_.front());
        queue_.pop();
        return value;
    }

    //non blocking
    std::optional<T> try_pop() {
        std::lock_guard<std::mutex> lock(mutex_);
        if (queue_.empty()) {
            return std::nullopt;
        }
        T value = std::move(queue_.front());
        queue_.pop();
        return value;
    }
private:
    std::queue<T> queue_;
    std::mutex mutex_;
    std::condition_variable cv_;
};


#endif //OPTIPRO_CNC_CONCURRENTQUEUE_HPP