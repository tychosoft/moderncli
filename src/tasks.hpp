// Copyright (C) 2023 Tycho Softworks.
// This code is licensed under MIT license.

#ifndef TYCHO_TASKS_HPP_
#define TYCHO_TASKS_HPP_

#include <queue>
#include <utility>
#include <thread>
#include <condition_variable>
#include <functional>
#include <stdexcept>
#include <atomic>
#include <memory>
#include <chrono>
#include <any>
#include <map>

namespace tycho {
// can be nullptr...
using action_t = void (*)();
using error_t = void (*)(const std::exception&);

// We may derive a timer subsystem from a protected timer queue
class timer_queue {
public:
    using task_t = std::function<void()>;
    using period_t = std::chrono::milliseconds;
    using timepoint_t = std::chrono::steady_clock::time_point;

    explicit timer_queue(error_t handler = [](const std::exception& e){}) noexcept : errors_(std::move(handler)), thread_(std::thread(&timer_queue::run, this)) {}
    timer_queue(const timer_queue&) = delete;
    auto operator=(const timer_queue&) -> auto& = delete;

    ~timer_queue() {
        shutdown();
    }

    operator bool() const noexcept {
        const std::lock_guard lock(lock_);
        return !stop_;
    }

    auto operator!() const noexcept {
        const std::lock_guard lock(lock_);
        return stop_;
    }

    void shutdown() noexcept {
        if(stop_)
            return;

        const std::lock_guard lock(lock_);
        stop_ = true;
        cond_.notify_all();
        thread_.join();
    }

    auto at(const timepoint_t& expires, task_t task) {
        const std::lock_guard lock(lock_);
        const auto id = next_++;
        timers_.emplace(expires, std::make_tuple(id, period_t(0), task));
        cond_.notify_all();
        return id;
    }

    auto periodic(const period_t& period, task_t task) {
        const auto expires = std::chrono::steady_clock::now() + period;
        const std::lock_guard lock(lock_);
        const auto id = next_++;
        timers_.emplace(expires, std::make_tuple(id, period, task));
        cond_.notify_all();
        return id;
    }

    auto cancel(uint64_t id) {
        const std::lock_guard lock(lock_);
        for(auto it = timers_.begin(); it != timers_.end(); ++it) {
            if(std::get<0>(it->second) == id) {
                timers_.erase(it);
                cond_.notify_all();
                return true;
            }
        }
        return false;
    }

    auto find(uint64_t id) const noexcept {
        const std::lock_guard lock(lock_);
        for(const auto& [expires, item] : timers_) {
            if(std::get<0>(item) == id)
                return expires;
        }
        return timepoint_t::min();
    }

    void clear() noexcept {
        const std::lock_guard lock(lock_);
        timers_.clear();
    }

    auto empty() const noexcept {
        const std::lock_guard lock(lock_);
        if(stop_)
            return true;
        return timers_.empty();
    }

private:
    using timer_t = std::tuple<uint64_t, period_t, task_t>;
    error_t errors_{[](const std::exception& e) {}};
    std::multimap<timepoint_t, timer_t> timers_;
    mutable std::mutex lock_;
    std::condition_variable cond_;
    std::thread thread_;
    bool stop_{false};
    uint64_t next_{0};

    void run() noexcept {
        for(;;) {
            std::unique_lock lock(lock_);
            if (stop_ && timers_.empty())
                return;
            if(timers_.empty()) {
                cond_.wait(lock);
                lock.unlock();
                continue;
            }
            auto it = timers_.begin();
            auto expires = it->first;
            const auto now = std::chrono::steady_clock::now();
            if(expires <= now) {
                const auto item(std::move(it->second));
                timers_.erase(it);
                lock.unlock();
                const auto& [id, period, task] = item;
                try {
                    task();
                }
                catch(const std::exception& e) {
                    errors_(e);
                }
                lock.lock();
                if(period != period_t(0)) {
                    expires += period;
                    timers_.emplace(expires, std::make_tuple(id, period, task));
                    cond_.notify_all();
                }
            }
            else
                cond_.wait_until(lock, expires);
            lock.unlock();
        }
    }
};

// Generic task queue, matches other languages
class task_queue final {
public:
    using timeout_strategy = std::function<std::chrono::milliseconds()>;
    using shutdown_strategy = std::function<void()>;
    using task_t = std::function<void()>;

    explicit task_queue(timeout_strategy timeout = &default_timeout, shutdown_strategy shutdown = [](){}) noexcept :
    timeout_(std::move(timeout)), shutdown_(std::move(shutdown)) {}

    task_queue(const task_queue&) = delete;
    auto operator=(const task_queue&) -> auto& = delete;

    ~task_queue() {
        shutdown();
    };

    operator bool() const noexcept {
        const std::lock_guard lock(mutex_);
        return running_;
    }

    auto operator!() const noexcept {
        const std::lock_guard lock(mutex_);
        return !running_;
    }

    auto priority(task_t task) {
        std::unique_lock lock(mutex_);
        if(!running_)
            return false;

        tasks_.push_front(std::move(task));
        lock.unlock();
        cvar_.notify_one();
        return true;
    }

    auto dispatch(task_t task) {
        std::unique_lock lock(mutex_);
        if(!running_)
            return false;

        tasks_.push_back(std::move(task));
        lock.unlock();
        cvar_.notify_one();
        return true;
    }

    void notify() {
        const std::unique_lock lock(mutex_);
        if(running_)
            cvar_.notify_one();
    }

    void startup() {
        const std::unique_lock lock(mutex_);
        if(running_)
            return;

        running_ = true;
        thread_ = std::thread(&task_queue::process, this);
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

    auto shutdown(shutdown_strategy handler) -> auto& {
        const std::lock_guard lock(mutex_);
        if(running_)
            throw std::runtime_error("cannot modify running task queue");
        shutdown_ = handler;
        return *this;
    }

    auto timeout(timeout_strategy handler) -> auto& {
        const std::lock_guard lock(mutex_);
        if(running_)
            throw std::runtime_error("cannot modify running task queue");

        timeout_ = handler;
        return *this;
    }

    auto errors(error_t handler) -> auto& {
        const std::lock_guard lock(mutex_);
        if(running_)
            throw std::runtime_error("cannot modify running task queue");

        errors_ = std::move(handler);
        return *this;
    }

    void clear() noexcept {
        const std::lock_guard lock(mutex_);
        tasks_.clear();
    }

    auto empty() const noexcept {
        const std::lock_guard lock(mutex_);
        if(!running_)
            return true;
        return tasks_.empty();
    }

private:
    timeout_strategy timeout_{default_timeout};
    shutdown_strategy shutdown_{[](){}};
    error_t errors_{[](const std::exception& e) {}};
    std::deque<task_t> tasks_;
    mutable std::mutex mutex_;
    std::condition_variable cvar_;
    std::thread thread_;
    bool running_{false};

    static auto default_timeout() -> std::chrono::milliseconds {
        return std::chrono::minutes(1);
    }

    void process() noexcept {
        for(;;) {
            std::unique_lock lock(mutex_);
            if(!running_)
                break;

            if(tasks_.empty())
                cvar_.wait_for(lock, timeout_());

            if(tasks_.empty())
                continue;

            try {
                auto func(std::move(tasks_.front()));
                tasks_.pop_front();

                // unlock before running task
                lock.unlock();
                func();
            }
            catch(const std::exception& e) {
                errors_(e);
            }
        }

        // run shutdown strategy in this context before joining...
        shutdown_();
    }
};

// Optimized function queue for C++ components
class func_queue final {
public:
    using timeout_strategy = std::chrono::milliseconds (*)();
    using task_t = void (*)(std::any);

    explicit func_queue(timeout_strategy timeout = default_timeout, action_t shutdown = nullptr) noexcept :
    timeout_(timeout), shutdown_(shutdown), thread_(std::thread(&func_queue::process, this)) {}

    func_queue(const func_queue&) = delete;
    auto operator=(const func_queue&) noexcept -> auto& = delete;

    ~func_queue() {
        shutdown();
    };

    operator bool() const noexcept {
        const std::lock_guard lock(mutex_);
        return running_;
    }

    auto operator!() const noexcept {
        const std::lock_guard lock(mutex_);
        return !running_;
    }

    auto max() const noexcept {
        const std::lock_guard lock(mutex_);
        return tasks_.max_size();
    }

    auto size() const noexcept {
        const std::lock_guard lock(mutex_);
        return tasks_.max_size();
    }

    auto dispatch(task_t task, std::any args = std::any()) {
        std::unique_lock lock(mutex_);
        if(!running_)
            return false;

        tasks_.emplace_back(std::move(task), std::move(args));
        lock.unlock();
        cvar_.notify_one();
        return true;
    }

;   auto priority(task_t task, std::any args = std::any()) {
        std::unique_lock lock(mutex_);
        if(!running_)
            return false;

        tasks_.emplace_front(std::move(task), std::move(args));
        lock.unlock();
        cvar_.notify_one();
        return true;
    }

    void notify() {
        if(running_)
            cvar_.notify_one();
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

    auto shutdown(action_t handler) -> auto& {
        // set shutdown from inside thread context
        using args_t = std::pair<action_t *, action_t>;
        dispatch([](std::any args) {
            auto [ptr, act] = std::any_cast<args_t>(args);
            *ptr = act;
        }, args_t{&shutdown_, handler});
        return *this;
    }

    auto timeout(timeout_strategy handler) -> auto& {
        using args_t = std::pair<timeout_strategy *, timeout_strategy>;
        dispatch([](std::any args) {
            auto [ptr, act] = std::any_cast<args_t>(args);
            *ptr = act;
        }, args_t{&timeout_, handler});
        return *this;
    }

    auto errors(error_t handler) -> auto& {
        errors_ = std::move(handler);
        return *this;
    }

    void clear() noexcept {
        tasks_.clear();
    }

    void clean() noexcept {
        tasks_.shrink_to_fit();
    }

    auto empty() const noexcept {
        const std::lock_guard lock(mutex_);
        if(!running_)
            return true;
        return tasks_.empty();
    }

private:
    timeout_strategy timeout_{default_timeout};
    action_t shutdown_{[](){}};
    error_t errors_ = {[](const std::exception& e){}};
    std::deque<std::pair<task_t, std::any>> tasks_;
    mutable std::mutex mutex_;
    std::condition_variable cvar_;
    std::thread thread_;
    bool running_{true};

    static auto default_timeout() -> std::chrono::milliseconds {
        return std::chrono::minutes(1);
    }

    void process() noexcept {
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

            try {
                std::tie(task, args) = std::move(tasks_.front());
                tasks_.pop_front();

                // unlock before running task
                lock.unlock();
                task(args);
            }
            catch(const std::exception& e) {
                errors_(e);
            }
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
