// Copyright (C) 2023 Tycho Softworks.
// This code is licensed under MIT license.

#ifndef TYCHO_TASKS_HPP_
#define TYCHO_TASKS_HPP_

#include <queue>
#include <utility>
#include <thread>
#include <condition_variable>
#include <functional>
#include <atomic>
#include <memory>
#include <chrono>
#include <any>

namespace tycho {
// can be nullptr...
using action_t = void (*)();

class task_queue {
public:
    using timeout_strategy = std::function<std::chrono::milliseconds()>;
    using shutdown_strategy = std::function<void()>;
    using task_t = std::function<void(std::any)>;

    explicit task_queue(timeout_strategy timeout = &default_timeout, shutdown_strategy shutdown = [](){}) :
    timeout_(std::move(timeout)), shutdown_(std::move(shutdown)), thread_(std::thread(&task_queue::process, this)) {}

    task_queue(const task_queue&) = delete;
    auto operator=(const task_queue&) -> auto& = delete;

    ~task_queue() {
        shutdown();
    };

    operator bool() {
        const std::lock_guard lock(mutex_);
        return running_;
    }

    auto operator!() {
        const std::lock_guard lock(mutex_);
        return !running_;
    }

    auto dispatch(task_t task, std::any args) {
        std::unique_lock lock(mutex_);
        if(!running_)
            return false;

        tasks_.emplace(std::move(task), std::move(args));
        lock.unlock();
        cvar_.notify_one();
        return true;
    }

    void shutdown() {
        std::unique_lock lock(mutex_);
        if(!running_)
            return;
        running_ = false;

        lock.unlock();
        cvar_.notify_all();
        if(thread_.joinable())
            thread_.join();
    }

    void shutdown(shutdown_strategy handler) {
        // set shutdown from inside thread context
        using args_t = std::pair<shutdown_strategy *, shutdown_strategy>;
        dispatch([](std::any args) {
            auto [ptr, act] = std::any_cast<args_t>(args);
            *ptr = act;
        }, args_t{&shutdown_, handler});
    }

    void timeout(timeout_strategy handler) {
        using args_t = std::pair<timeout_strategy *, timeout_strategy>;
        dispatch([](std::any args) {
            auto [ptr, act] = std::any_cast<args_t>(args);
            *ptr = act;
        }, args_t{&timeout_, handler});
    }

    void clear() {
        const std::lock_guard lock(mutex_);
        while(!tasks_.empty())
            tasks_.pop();
    }

    auto empty() {
        const std::lock_guard lock(mutex_);
        if(!running_)
            return true;
        return tasks_.empty();
    }

private:
    timeout_strategy timeout_;
    shutdown_strategy shutdown_;
    std::queue<std::pair<task_t, std::any>> tasks_;
    std::mutex mutex_;
    std::condition_variable cvar_;
    std::thread thread_;
    bool running_{true};

    static auto default_timeout() -> std::chrono::milliseconds {
        return std::chrono::minutes(1);
    }

    void process() {
        task_t task;
        std::any args;
        bool used{false};

        for(;;) {
            std::unique_lock lock(mutex_);
            if(!running_)
                break;

            if(tasks_.empty()) {
                if(!used)
                    cvar_.wait(lock);
                else
                    cvar_.wait_for(lock, timeout_());
            }

            if(tasks_.empty())
                continue;

            std::tie(task, args) = std::move(tasks_.front());
            tasks_.pop();

            // unlock before running task
            lock.unlock();
            task(args);
            used = true;
        }

        // run shutdown strategy in this context before joining...
        if(used)
            shutdown_();
    }
};

// Optimized function queue
class func_queue {
public:
    using timeout_strategy = std::chrono::milliseconds (*)();
    using task_t = void (*)(std::any);

    explicit func_queue(timeout_strategy timeout = default_timeout, action_t shutdown = nullptr) :
    timeout_(timeout), shutdown_(shutdown), thread_(std::thread(&func_queue::process, this)) {}

    func_queue(const func_queue&) = delete;
    auto operator=(const func_queue&) -> auto& = delete;

    ~func_queue() {
        shutdown();
    };

    operator bool() {
        const std::lock_guard lock(mutex_);
        return running_;
    }

    auto operator!() {
        const std::lock_guard lock(mutex_);
        return !running_;
    }


    auto dispatch(task_t task, std::any args) {
        std::unique_lock lock(mutex_);
        if(!running_)
            return false;

        tasks_.emplace(std::move(task), std::move(args));
        lock.unlock();
        cvar_.notify_one();
        return true;
    }

    void shutdown() {
        std::unique_lock lock(mutex_);
        if(!running_)
            return;
        running_ = false;

        lock.unlock();
        cvar_.notify_all();
        if(thread_.joinable())
            thread_.join();
    }

    void shutdown(action_t handler) {
        // set shutdown from inside thread context
        using args_t = std::pair<action_t *, action_t>;
        dispatch([](std::any args) {
            auto [ptr, act] = std::any_cast<args_t>(args);
            *ptr = act;
        }, args_t{&shutdown_, handler});
    }

    void timeout(timeout_strategy handler) {
        using args_t = std::pair<timeout_strategy *, timeout_strategy>;
        dispatch([](std::any args) {
            auto [ptr, act] = std::any_cast<args_t>(args);
            *ptr = act;
        }, args_t{&timeout_, handler});
    }

    void clear() {
        const std::lock_guard lock(mutex_);
        while(!tasks_.empty())
            tasks_.pop();
    }

    auto empty() {
        const std::lock_guard lock(mutex_);
        if(!running_)
            return true;
        return tasks_.empty();
    }

private:
    timeout_strategy timeout_;
    action_t shutdown_{nullptr};
    std::queue<std::pair<task_t, std::any>> tasks_;
    std::mutex mutex_;
    std::condition_variable cvar_;
    std::thread thread_;
    bool running_{true};

    static auto default_timeout() -> std::chrono::milliseconds {
        return std::chrono::minutes(1);
    }

    void process() {
        task_t task{};
        std::any args;
        bool used{false};

        for(;;) {
            std::unique_lock lock(mutex_);
            if(!running_)
                break;

            if(tasks_.empty()) {
                if(!used)
                    cvar_.wait(lock);
                else
                    cvar_.wait_for(lock, timeout_());
            }

            if(tasks_.empty())
                continue;

            std::tie(task, args) = std::move(tasks_.front());
            tasks_.pop();

            // unlock before running task
            lock.unlock();
            task(args);
            used = true;
        }

        // run shutdown strategy in this context before joining...
        if(used && shutdown_)
            shutdown_();
    }
};

inline void invoke(action_t action) {
    if(action != nullptr)
        action();
}
} // end namespace
#endif
