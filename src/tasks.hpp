// Copyright (C) 2023 Tycho Softworks.
// This code is licensed under MIT license.

#ifndef TYCHO_TASKS_HPP_
#define TYCHO_TASKS_HPP_

#include <queue>
#include <utility>
#include <thread>
#include <future>
#include <type_traits>
#include <condition_variable>
#include <functional>
#include <stdexcept>
#include <atomic>
#include <chrono>
#include <tuple>
#include <map>

namespace tycho {
// can be nullptr...
using action_t = void (*)();
using error_t = void (*)(const std::exception&);
using task_t = std::function<void()>;

class future_cancelled : public std::runtime_error {
public:
    future_cancelled() : std::runtime_error("Future cancelled") {};
};

template<typename Func, typename... Args>
inline void detach(Func&& func, Args&&... args) {
    std::thread([func = std::forward<Func>(func), tuple = std::make_tuple(std::forward<Args>(args)...)]() mutable {
        std::apply(func, std::move(tuple));
    }).detach();
}

#if __cplusplus < 202002L || (defined(_LIBCPP_VERSION) && (!defined(_LIBCPP_STD_VER) || _LIBCPP_STD_VER < 20 || defined(_LIBCPP_HAS_NO_EXPERIMENTAL_STOP_TOKEN)))
class thread_t final {
public:
    thread_t() = default;
    thread_t(thread_t&& other) noexcept = default;
    thread_t(const thread_t&) = delete;
    auto operator=(const thread_t&) -> thread_t& = delete;
    auto operator=(thread_t&&) -> thread_t& = default;

    template<typename Callable, typename... Args,
    typename = std::enable_if_t<!std::is_same_v<std::decay_t<Callable>, thread_t>>>
    explicit thread_t(Callable&& f, Args&&... args) : thread_(std::forward<Callable>(f), std::forward<Args>(args)...) {}

    ~thread_t() {
        if(thread_.joinable())
            thread_.join();
    }

    operator std::thread::id() const noexcept { return thread_.get_id(); }

    auto native_handle() {return thread_.native_handle();}
    auto get_id() const noexcept {return thread_.get_id();}
    auto joinable() const noexcept -> bool { return thread_.joinable(); }

    void join() {
        if(thread_.joinable())
            thread_.join();
    }

    void swap(thread_t& other) noexcept {
        std::swap(thread_, other.thread_);
    }

    void detach() noexcept {
        if(thread_.joinable())
            thread_.detach();
    }

private:
    std::thread thread_;
};
#else
#define MODERNCLI_HAS_JTHREAD   1
using thread_t = std::jthread;
#endif

// We may derive a timer subsystem from a protected timer queue
class timer_queue {
public:
    using period_t = std::chrono::milliseconds;
    using timepoint_t = std::chrono::steady_clock::time_point;

    explicit timer_queue(error_t handler = [](const std::exception& e){}) noexcept :
    errors_(std::move(handler)) {}

    timer_queue(const timer_queue&) = delete;
    auto operator=(const timer_queue&) -> auto& = delete;

    ~timer_queue() {
        shutdown();
    }

    operator bool() const noexcept {
        return !stop_.load();
    }

    auto operator!() const noexcept {
        return stop_.load();
    }

    void startup() noexcept {
        thread_ = thread_t(&timer_queue::run, this);
    }

    void shutdown() {
        std::unique_lock lock(lock_);
        if(stop_) return;
        stop_ = true;
        lock.unlock();
        cond_.notify_all();
    }

    auto at(const timepoint_t& expires, task_t task) {
        const std::lock_guard lock(lock_);
        const auto id = next_++;
        timers_.emplace(expires, std::make_tuple(id, period_t(0), task));
        cond_.notify_all();
        return id;
    }

    auto at(time_t expires, task_t task) {
        const auto now = std::chrono::system_clock::now();
        const auto when = std::chrono::system_clock::from_time_t(expires);
        const auto delay = when - now;
        const auto target = std::chrono::steady_clock::now() +
            std::chrono::duration_cast<std::chrono::steady_clock::duration>(delay);
        return at(target, std::move(task)); // delegate to your existing overload
    }

    auto periodic(unsigned period, task_t task) {
        const auto expires = std::chrono::steady_clock::now() + std::chrono::milliseconds(period);
        const std::lock_guard lock(lock_);
        const auto id = next_++;
        timers_.emplace(expires, std::make_tuple(id, period, task));
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
            if(std::get<0>(item) == id) return expires;
        }
        return timepoint_t::min();
    }

    void clear() noexcept {
        const std::lock_guard lock(lock_);
        timers_.clear();
    }

    auto empty() const noexcept {
        const std::lock_guard lock(lock_);
        if(stop_) return true;
        return timers_.empty();
    }

private:
    using timer_t = std::tuple<uint64_t, period_t, task_t>;
    error_t errors_{[](const std::exception& e) {}};
    std::multimap<timepoint_t, timer_t> timers_;
    mutable std::mutex lock_;
    std::condition_variable cond_;
    thread_t thread_;
    std::atomic<bool> stop_{false};
    uint64_t next_{0};

    void run() noexcept {
        for(;;) {
            std::unique_lock lock(lock_);
            if(!stop_ && timers_.empty())
                cond_.wait(lock);

            if(stop_) break;
            if(timers_.empty()) continue;

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
                continue;
            }
            cond_.wait_until(lock, expires);
        }
    }
};

// Generic task queue, matches other languages
class task_queue {
public:
    using timeout_strategy = std::function<std::chrono::milliseconds()>;
    using shutdown_strategy = std::function<void()>;

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
        if(!running_) return false;
        tasks_.push_front(std::move(task));
        lock.unlock();
        cvar_.notify_one();
        return true;
    }

    auto dispatch(task_t task, std::size_t max = 0) {
        const std::lock_guard lock(mutex_);
        if(!running_) return false;
        if(max && tasks_.size() >= max) return false;
        tasks_.push_back(std::move(task));
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
        if(running_) return;
        running_ = true;
        thread_ = thread_t(&task_queue::process, this);
    }

    void shutdown() {
        std::unique_lock lock(mutex_);
        if(!running_) return;
        running_ = false;
        lock.unlock();

        cvar_.notify_all();
    }

    auto shutdown(shutdown_strategy handler) -> auto& {
        const std::lock_guard lock(mutex_);
        if(running_) throw std::runtime_error("cannot modify running task queue");
        shutdown_ = handler;
        return *this;
    }

    auto timeout(timeout_strategy handler) -> auto& {
        const std::lock_guard lock(mutex_);
        if(running_) throw std::runtime_error("cannot modify running task queue");
        timeout_ = handler;
        return *this;
    }

    auto errors(error_t handler) -> auto& {
        const std::lock_guard lock(mutex_);
        if(running_) throw std::runtime_error("cannot modify running task queue");
        errors_ = std::move(handler);
        return *this;
    }

    void clear() noexcept {
        const std::lock_guard lock(mutex_);
        tasks_.clear();
    }

    auto empty() const noexcept {
        const std::lock_guard lock(mutex_);
        if(!running_) return true;
        return tasks_.empty();
    }

    auto size() const noexcept {
        const std::lock_guard lock(mutex_);
        return tasks_.size();
    }

    auto active() const noexcept {
        const std::lock_guard lock(mutex_);
        return running_;
    }

private:
    timeout_strategy timeout_{default_timeout};
    shutdown_strategy shutdown_{[](){}};
    error_t errors_{[](const std::exception& e) {}};
    std::deque<task_t> tasks_;
    mutable std::mutex mutex_;
    std::condition_variable cvar_;
    thread_t thread_;
    volatile bool running_{false};

    static auto default_timeout() -> std::chrono::milliseconds {
        return std::chrono::minutes(1);
    }

    void process() noexcept {
        for(;;) {
            std::unique_lock lock(mutex_);
            if(!running_) break;
            if(tasks_.empty())
                cvar_.wait_for(lock, timeout_());

            if(tasks_.empty()) continue;
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

class task_pool {
public:
    task_pool() = default;
    task_pool(const task_pool&) = delete;
    auto operator=(const task_pool&) -> auto& = delete;

    explicit task_pool(std::size_t count) {
        start(count);
    }

    ~task_pool() {
        drain();
    }

    auto size() const noexcept {
        const std::lock_guard lock(mutex_);
        return workers_.size();
    }

    void resize(std::size_t count) {
        drain();
        if(count) start(count);
    }

    void start(std::size_t count = 0) {
        if(count == 0)
            count = std::thread::hardware_concurrency();
        if(count == 0)
            count = 1;
        const std::lock_guard lock(mutex_);
        if(started_) return;
        accepting_ = true;
        started_ = true;
        workers_.clear();
        workers_.reserve(count);
        for(std::size_t i = 0; i < count; ++i) {
            workers_.emplace_back([this] {
                while (true) {
                    std::function<void()> task;
                    std::unique_lock lock(mutex_);
                    cvar_.wait(lock, [this] {
                        return !accepting_ || !tasks_.empty();
                    });

                    if(!accepting_ && tasks_.empty()) return;
                    task = std::move(tasks_.front());
                    tasks_.pop();
                    lock.unlock();
                    task();
                }
            });
        }
    }

    auto dispatch(task_t task) {
        const std::lock_guard lock(mutex_);
        if(!accepting_) return false;
        tasks_.push(std::move(task));
        cvar_.notify_one();
        return true;
    }

    void drain() noexcept {
        std::unique_lock lock(mutex_);
        accepting_ = false;
        cvar_.notify_all();
        lock.unlock();

        // joins are outside lock so we dont block if waiting to join
        for(auto& t : workers_)
            if(t.joinable()) t.join();

        lock.lock();
        workers_.clear();
        started_ = false;
    }

private:
    std::vector<thread_t> workers_;
    std::queue<std::function<void()>> tasks_;
    mutable std::mutex mutex_;
    std::condition_variable cvar_;
    std::atomic<bool> accepting_{false};
    bool started_{false};
};

template<typename T, typename Pred>
inline auto get_future(std::function<T>& future, Pred pred, std::chrono::milliseconds interval = std::chrono::milliseconds(100)) {
    do {    // NOLINT
        if(!pred()) throw future_cancelled();
    } while(future.wait_for(interval) != std::future_status::ready);
    return future.get();
}

template<typename Func, typename... Args>
inline auto await(Func&& func, Args&&... args) -> std::future<typename std::invoke_result_t<Func, Args...>> {
    return std::async(std::launch::async, std::forward<Func>(func), std::forward<Args>(args)...);
}

template<typename Func, typename... Args>
inline void parallel_threads(std::size_t count, Func&& func, Args&&... args) {
    if(!count)
        count = std::thread::hardware_concurrency();
    if(!count)
        count = 1;

    std::vector<thread_t> threads;
    threads.reserve(count);
    auto argv = std::make_tuple(std::forward<Args>(args)...);

    for (std::size_t i = 0; i < count; ++i) {
        threads.emplace_back([f = std::forward<Func>(func), argv]() mutable {
            std::apply(f, argv);
        });
    }
    std::this_thread::yield();
}

inline void yield(unsigned msec) {
    std::this_thread::sleep_for(std::chrono::milliseconds(msec));
}

inline void yield() {
    std::this_thread::yield();
}

inline void invoke(action_t action) {
    if(action != nullptr)
        action();
}
} // end namespace
#endif
