// Copyright (C) 2023 Tycho Softworks.
// This code is licensed under MIT license.

#ifndef TYCHO_SYNC_HPP_
#define TYCHO_SYNC_HPP_

#include <mutex>
#include <chrono>
#include <thread>
#include <shared_mutex>
#include <condition_variable>
#include <stdexcept>

namespace tycho {
using sync_timepoint = std::chrono::steady_clock::time_point;
using sync_millisecs = std::chrono::milliseconds;

class semaphore_cancelled : public std::runtime_error {
public:
    semaphore_cancelled() : std::runtime_error("Future cancelled") {};
};

inline auto system_clock(std::time_t offset = 0) {
    return std::time(nullptr) + offset;
}

inline auto sync_clock(long timeout = 0) {
    return std::chrono::steady_clock::now() + sync_millisecs(timeout);
}

inline auto sync_sleep(const sync_timepoint& timepoint) {
    std::this_thread::sleep_until(timepoint);
}

inline auto sync_yield() {
    std::this_thread::yield();
}

inline auto sync_duration(const sync_timepoint& start, const sync_timepoint& end) {
    return std::chrono::duration_cast<sync_millisecs>(end - start);
}

inline auto sync_elapsed(const sync_timepoint& start) {
    return std::chrono::duration_cast<sync_millisecs>(sync_clock() - start);
}

inline auto sync_remains(const sync_timepoint& end) {
    return std::chrono::duration_cast<sync_millisecs>(end - sync_clock());
}

template <typename T>
class unique_sync final {
public:
    template <typename... Args>
    explicit unique_sync(Args&&... args) :
    data(std::forward<Args>(args)...) {}

private:
    template <typename U> friend class sync_ptr;
    template <typename U> friend class guard_ptr;
    T data{};
    std::mutex lock;
};

template <typename T>
class shared_sync final {
public:
    template <typename... Args>
    explicit shared_sync(Args&&... args) :
    data(std::forward<Args>(args)...) {}

private:
    template <typename U> friend class reader_ptr;
    template <typename U> friend class writer_ptr;
    T data{};
    std::shared_mutex lock;
};

template <typename U>
class sync_ptr final : public std::unique_lock<std::mutex> {
public:
    sync_ptr() = delete;
    sync_ptr(const sync_ptr&) = delete;
    auto operator=(const sync_ptr&) = delete;

    explicit sync_ptr(unique_sync<U>& obj) :
    unique_lock(obj.lock), sync_(obj), ptr_(&obj.data) {}
    ~sync_ptr() = default;

    auto operator->() {
        if (!owns_lock())
            throw std::runtime_error("unique lock error");
        return ptr_;
    }

    auto operator*() -> U& {
        if (!owns_lock())
            throw std::runtime_error("unique lock error");
        return *ptr_;
    }

private:
    unique_sync<U> &sync_;  // NOLINT
    U* ptr_;
};

template <typename U>
class guard_ptr final {
public:
    guard_ptr() = delete;
    guard_ptr(const guard_ptr&) = delete;
    auto operator=(const guard_ptr&) = delete;

    explicit guard_ptr(unique_sync<U>& obj) :
    sync_(obj), ptr_(&obj.data) {
        sync_.lock.lock();
    }

    ~guard_ptr() {
        sync_.lock.unlock();
    }

    auto operator->() {
        return ptr_;
    }

    auto operator*() -> U& {
        return *ptr_;
    }

private:
    unique_sync<U> &sync_;  // NOLINT
    U* ptr_;
};

template <typename U>
class reader_ptr final : public std::shared_lock<std::shared_mutex> {
public:
    reader_ptr() = delete;
    reader_ptr(const reader_ptr&) = delete;
    auto operator=(const reader_ptr&) = delete;

    explicit reader_ptr(shared_sync<U>& obj) :
    std::shared_lock<std::shared_mutex>(obj.lock), sync_(obj), ptr_(&obj.data) {}
    ~reader_ptr() = default;

    auto operator->() const -> const U* {
        if (!owns_lock())
            throw std::runtime_error("read lock error");
        return ptr_;
    }

    auto operator*() const -> const U& {
        if (!owns_lock())
            throw std::runtime_error("read lock error");
        return *ptr_;
    }

private:
    shared_sync<U> &sync_;  // NOLINT
    const U* ptr_;
};

template <typename U>
class writer_ptr final : public std::unique_lock<std::shared_mutex> {
public:
    writer_ptr() = delete;
    writer_ptr(const writer_ptr&) = delete;
    auto operator=(const writer_ptr&) = delete;

    explicit writer_ptr(shared_sync<U>& obj) :
    std::unique_lock<std::shared_mutex>(obj.lock), sync_(obj), ptr_(&obj.data) {}
    ~writer_ptr() = default;

    auto operator->() {
        if (!owns_lock())
            throw std::runtime_error("write lock error");
        return ptr_;
    }

    auto operator*() -> U& {
        if (!owns_lock())
            throw std::runtime_error("write lock error");
        return *ptr_;
    }

private:
    shared_sync<U> &sync_;  // NOLINT
    U* ptr_;
};

class semaphore_t final {
public:
    explicit semaphore_t(unsigned count = 0) noexcept : count_(count) {}
    semaphore_t(const semaphore_t&) = delete;
    auto operator=(const semaphore_t&) = delete;

    operator bool() const noexcept {
        return !empty();
    }

    auto operator!() const noexcept {
        return empty();
    }

    auto operator++() -> auto& {
        wait();
        return *this;
    }

    auto operator--() noexcept -> auto& {
        post();
        return *this;
    }

    void post() noexcept {
        const std::unique_lock lock(lock_);

        if(active_) {
            --active_;
            cond_.notify_one();
        }
    }

    void wait() {
        std::unique_lock lock(lock_);
        if(++active_ > count_)
            cond_.wait(lock, [this]{return active_ <= count_;});

        if(count_ == ~0U) {
            --active_;
            throw semaphore_cancelled();
        }
    }

    auto try_wait() {
        std::unique_lock lock(lock_);
        if(++active_ > count_)
            cond_.wait(lock, [this]{return active_ <= count_;});

        if(count_ == ~0U) {
            --active_;
            return false;
        }
        return true;
    }

    auto wait_for(const sync_millisecs& timeout) {
        std::unique_lock lock(lock_);
        ++active_;
        if(active_ <= count_ || cond_.wait_for(lock, timeout, [this]{return active_ <= count_;})) {
            if(count_ == ~0U) {
                --active_;
                throw semaphore_cancelled();
            }
            return true;
        }

        --active_;
        return false;
    }

    auto wait_until(const sync_timepoint& time_point) {
        std::unique_lock lock(lock_);
        ++active_;
        if(active_ <= count_ || cond_.wait_until(lock, time_point, [this]{return active_ <= count_;})) {
            if(count_ == ~0U) {
                --active_;
                throw semaphore_cancelled();
            }
            return true;
        }

        --active_;
        return false;
    }

    auto empty() const noexcept -> bool {
        const std::lock_guard lock(lock_);
        return count_ == ~0U;
    }

    auto size() const noexcept {
        const std::lock_guard lock(lock_);
        return count_;
    }

    auto active() const noexcept {
        const std::lock_guard lock(lock_);
        return active_;
    }

    auto release() noexcept {
        const std::lock_guard lock(lock_);
        count_ = ~0U;
        cond_.notify_all();
    }

    auto reset(unsigned count) noexcept {
        const std::lock_guard lock(lock_);
        count_ = count;
        cond_.notify_all();
    }

private:
    mutable std::mutex lock_;
    std::condition_variable cond_;
    unsigned count_{0};
    unsigned active_{0};
};

class semaphore_guard {
public:
    semaphore_guard(const semaphore_guard&) = delete;
    auto operator=(const semaphore_guard&) -> auto& = delete;

    explicit semaphore_guard(semaphore_t& sem) : sem_(sem) {
        sem_.wait();
    }

    ~semaphore_guard() {
        sem_.post();
    }

private:
    semaphore_t& sem_;
};

class barrier_t final {
public:
    explicit barrier_t(unsigned limit) noexcept : count_(limit), limit_(limit) {}
    barrier_t(const barrier_t&) = delete;
    auto operator=(const barrier_t&) = delete;

    void wait() noexcept {
        std::unique_lock lock(lock_);
        if(!count_)
            return;

        auto sequence = sequence_;
        if (--count_ == 0) {
            sequence_++;
            count_ = limit_;
            cond_.notify_all();
        }
        else {
            cond_.wait(lock, [this, sequence]{return sequence != sequence_;});
        }
    }

    auto wait_for(const sync_millisecs& timeout) noexcept {
        std::unique_lock lock(lock_);
        if(!count_)
            return false;

        auto sequence = sequence_;
        if (--count_ == 0) {
            sequence_++;
            count_ = limit_;
            cond_.notify_all();
            return true;
        }
        return cond_.wait_for(lock, timeout, [this, sequence]{return sequence != sequence_;});
    }

    auto wait_until(const sync_timepoint& time_point) noexcept {
        std::unique_lock lock(lock_);
        if(!count_)
            return false;

        auto sequence = sequence_;
        if (--count_ == 0) {
            sequence_++;
            count_ = limit_;
            cond_.notify_all();
            return true;
        }
        return cond_.wait_until(lock, time_point, [this, sequence]{return sequence != sequence_;});
    }

    auto count() const noexcept {
        const std::lock_guard lock(lock_);
        return count_;
    }

    auto release() noexcept {
        const std::lock_guard lock(lock_);
        count_ = 0;
        ++sequence_;
        cond_.notify_all();
    }

    auto reset(unsigned limit) noexcept {
        const std::lock_guard lock(lock_);
        count_ = limit_ = limit;
        ++sequence_;
        cond_.notify_all();
    }

private:
    mutable std::mutex lock_;
    std::condition_variable cond_;
    unsigned count_{0}, sequence_{0}, limit_{0};
};

class event_sync final {
public:
    explicit event_sync(bool reset = true) noexcept : auto_reset_(reset) {}
    event_sync(const event_sync&) = delete;
    ~event_sync() = default;
    auto operator=(const event_sync&) noexcept -> auto& = delete;

    void notify() noexcept {
        const std::lock_guard lock(lock_);
        signaled_ = true;
        if(auto_reset_)
            cond_.notify_one();
        else
            cond_.notify_all();
    }

    void reset() noexcept {
        const std::lock_guard lock(lock_);
        signaled_ = false;
    }

    void wait() noexcept {
        std::unique_lock lock(lock_);
        cond_.wait(lock, [this]{return signaled_;});
        if(auto_reset_)
            signaled_ = false;
    }

    auto wait_for(const sync_millisecs& timeout) noexcept {
        std::unique_lock lock(lock_);
        auto result = cond_.wait_for(lock, timeout, [this]{return signaled_;});
        if(result && auto_reset_)
            signaled_ = false;
        return result;
    }

    auto wait_until(const sync_timepoint& time_point) noexcept {
        std::unique_lock lock(lock_);
        auto result = cond_.wait_until(lock, time_point, [this]{return signaled_;});
        if(result && auto_reset_)
            signaled_ = false;
        return result;
    }

private:
    std::mutex lock_;
    std::condition_variable cond_;
    bool signaled_{false};
    bool auto_reset_{true};
};

class wait_group final {
public:
    explicit wait_group(unsigned init) noexcept : count_(init) {}
    wait_group(const wait_group&) = delete;
    wait_group() = default;
    ~wait_group() = default;
    auto operator=(const wait_group&) noexcept -> auto& = delete;

    auto operator++() noexcept -> auto& {
        add(1);
        return *this;
    }

    auto operator+=(unsigned count) noexcept -> auto& {
        add(count);
        return *this;
    }

    void add(unsigned count) noexcept {
        const std::lock_guard lock(lock_);
        count_ += count;
    }

    auto done() noexcept {
        const std::lock_guard lock(lock_);
        if(!count_)
            return true;

        if(--count_ == 0) {
            cond_.notify_all();
            return true;
        }
        return false;
    }

    void wait() noexcept {
        std::unique_lock lock(lock_);
        cond_.wait(lock, [this]{return !count_ ;});
    }

    auto wait_for(const sync_millisecs& timeout) noexcept {
        std::unique_lock lock(lock_);
        return cond_.wait_for(lock, timeout, [this]{return !count_ ;});
    }

    auto wait_until(const sync_timepoint& time_point) noexcept {
        std::unique_lock lock(lock_);
        return cond_.wait_until(lock, time_point, [this]{return !count_ ;});
    }

    auto count() const noexcept {
        const std::lock_guard lock(lock_);
        return count_;
    }

private:
    unsigned count_{0};
    mutable std::mutex lock_;
    std::condition_variable cond_;
};
} // end namespace
#endif
