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

inline auto system_clock(std::time_t offset = 0) {
    return std::time(nullptr) + offset;
}

inline auto sync_clock(long timeout = 0) {
    return std::chrono::steady_clock::now() + std::chrono::milliseconds(timeout);
}

inline auto sync_sleep(sync_timepoint timepoint) {
    std::this_thread::sleep_until(timepoint);
}

inline auto sync_yield() {
    std::this_thread::yield();
}

inline auto sync_duration(const sync_timepoint& start, const sync_timepoint& end) {
    return std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
}

inline auto sync_elapsed(const sync_timepoint& start) {
    return std::chrono::duration_cast<std::chrono::milliseconds>(sync_clock() - start);
}

inline auto sync_remains(const sync_timepoint& end) {
    return std::chrono::duration_cast<std::chrono::milliseconds>(end - sync_clock());
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

    auto operator->() const {
        if (!owns_lock())
            throw std::runtime_error("read lock error");
        return ptr_;
    }

    auto operator*() const -> U& {
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
    semaphore_t(const semaphore_t&) = delete;
    auto operator=(const semaphore_t&) = delete;

    explicit semaphore_t(unsigned limit = 1) : limit_(limit) {}

    ~semaphore_t() {
        release();
        std::this_thread::yield();
        while(count_) {
            std::this_thread::yield();
        }
    }

    void release() {
        const std::unique_lock lock(lock_);
        release_ = true;
        if(count_ > limit_)
            cond_.notify_all();
    }

    void post() {
        const std::unique_lock lock(lock_);
        if(--count_ >= limit_)
            cond_.notify_one();
    }

    void wait() {
        std::unique_lock lock(lock_);
        while(!release_ && count_ > limit_)
            cond_.wait(lock);
    }

    auto wait_until(sync_timepoint timepoint) {
        std::unique_lock lock(lock_);
        while(!release_ && count_ > limit_) {
            if(cond_.wait_until(lock, timepoint) == std::cv_status::timeout)
                return false;
        }
        return true;
    }

    auto pending() const {
        const std::unique_lock lock(lock_);
        if(count_ > limit_)
            return limit_ - count_;
        return 0U;
    }

    auto count() const {
        const std::unique_lock lock(lock_);
        return count_;
    }

    auto active() const {
        const std::unique_lock lock(lock_);
        if(count_ <= limit_)
            return count_;
        return limit_;
    }

private:
    mutable std::mutex lock_;
    std::condition_variable cond_;
    unsigned count_{0}, limit_;
    bool release_{false};
};

class barrier_t final {
public:
    barrier_t(const barrier_t&) = delete;
    auto operator=(const barrier_t&) = delete;

    explicit barrier_t(unsigned limit = 1) : limit_(limit) {}

    ~barrier_t() {
        release();
    }

    void release() {
        const std::unique_lock lock(lock_);
        if(count_)
            cond_.notify_all();
    }

    void wait() {
        std::unique_lock lock(lock_);
        if(++count_ >= limit_) {
            cond_.notify_all();
            return;
        }
        cond_.wait(lock);
        if(++ending_ < limit_)
            return;
        count_ = ending_ = 0;
    }

    auto count() const {
        const std::unique_lock lock(lock_);
        return count_;
    }

    auto pending() const {
        const std::unique_lock lock(lock_);
        return count_ - ending_;
    }

private:
    mutable std::mutex lock_;
    std::condition_variable cond_;
    unsigned count_{0}, ending_{0}, limit_;
};

class event_sync {
public:
    explicit event_sync(bool reset = true) noexcept : auto_reset_(reset) {}
    event_sync(const event_sync&) = delete;
    event_sync() = default;
    ~event_sync() = default;
    auto operator=(const event_sync&) noexcept -> auto& = delete;

    void set() noexcept {
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

    auto wait_for(const std::chrono::milliseconds& timeout) noexcept {
        std::unique_lock lock(lock_);
        auto result = cond_.wait_for(lock, timeout, [this]{return signaled_;});
        if(result && auto_reset_)
            signaled_ = false;
        return result;
    }

    auto wait_until(const std::chrono::steady_clock::time_point& time_point) noexcept {
        std::unique_lock lock(lock_);
        auto result = cond_.wait_until(lock, time_point, [this]{return signaled_;});
        if(result && auto_reset_)
            signaled_ = false;
        return result;
    }

private:
    std::mutex lock_;
    std::condition_variable cond_;
    bool auto_reset_{true};
    bool signaled_{false};
};

class wait_group {
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

    auto wait_for(const std::chrono::milliseconds& timeout) noexcept {
        std::unique_lock lock(lock_);
        return cond_.wait_for(lock, timeout, [this]{return !count_ ;});
    }

    auto wait_until(const std::chrono::steady_clock::time_point& time_point) noexcept {
        std::unique_lock lock(lock_);
        return cond_.wait_until(lock, time_point, [this]{return !count_ ;});
    }

    auto count() noexcept {
        const std::lock_guard lock(lock_);
        return count_;
    }

private:
    unsigned count_{0};
    std::mutex lock_;
    std::condition_variable cond_;
};
} // end namespace
#endif
