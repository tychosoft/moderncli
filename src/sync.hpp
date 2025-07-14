// Copyright (C) 2023 Tycho Softworks.
// This code is licensed under MIT license.

#ifndef TYCHO_SYNC_HPP_
#define TYCHO_SYNC_HPP_

#include <mutex>
#include <chrono>
#include <thread>
#include <memory>
#include <shared_mutex>
#include <unordered_set>
#include <condition_variable>
#include <stdexcept>
#include <utility>

#if __cplusplus >= 202002L
#include <barrier>
#include <ranges>
#include <semaphore>
#endif

namespace tycho {
using sync_timepoint = std::chrono::steady_clock::time_point;
using sync_millisecs = std::chrono::milliseconds;

class semaphore_cancelled : public std::runtime_error {
public:
    semaphore_cancelled() : std::runtime_error("Acquire cancelled") {};
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
    auto operator=(const sync_ptr&) -> auto& = delete;

    explicit sync_ptr(unique_sync<U>& obj) :
    unique_lock(obj.lock), sync_(obj), ptr_(&obj.data) {}

    explicit sync_ptr(sync_ptr&& from) noexcept :
    std::unique_lock<std::mutex>(std::move(from)), sync_(from.sync_), ptr_(from.ptr_) {}

    ~sync_ptr() = default;

    auto operator->() {
        if(!owns_lock()) throw std::runtime_error("unique lock error");
        return ptr_;
    }

    auto operator*() -> U& {
        if(!owns_lock()) throw std::runtime_error("unique lock error");
        return *ptr_;
    }

    auto operator=(sync_ptr&& from) noexcept -> sync_ptr& {
        if (this != &from) {
            std::unique_lock<std::mutex>::operator=(std::move(from));
            sync_ = from.sync_;
            ptr_ = from.ptr_;
            from.ptr_ = nullptr;
            from.sync_ = nullptr;
        }
        return *this;
    }

    template<typename I>
    auto operator[](const I& index) -> decltype(std::declval<U&>()[index]) {
        return ptr_->operator[](index);
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
    auto operator=(const guard_ptr&) -> auto& = delete;

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

    template<typename I>
    auto operator[](const I& index) -> decltype(std::declval<U&>()[index]) {
        return ptr_->operator[](index);
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
    auto operator=(const reader_ptr&) -> auto& = delete;

    explicit reader_ptr(shared_sync<U>& obj) :
    std::shared_lock<std::shared_mutex>(obj.lock), sync_(obj), ptr_(&obj.data) {}

    explicit reader_ptr(reader_ptr&& from) noexcept :
    std::shared_lock<std::shared_mutex>(std::move(from)), sync_(from.sync_), ptr_(from.ptr_) {}

    ~reader_ptr() = default;

    auto operator->() const -> const U* {
        if(!owns_lock()) throw std::runtime_error("read lock error");
        return ptr_;
    }

    auto operator*() const -> const U& {
        if(!owns_lock()) throw std::runtime_error("read lock error");
        return *ptr_;
    }

    auto operator=(reader_ptr&& from) noexcept -> reader_ptr& {
        if (this != &from) {
            std::shared_lock<std::shared_mutex>::operator=(std::move(from));
            sync_ = from.sync_;
            ptr_ = from.ptr_;
            from.ptr_ = nullptr;
            from.sync_ = nullptr;
        }
        return *this;
    }

    template<typename I>
    auto operator[](const I& index) const -> decltype(std::declval<const U&>()[index]) {
        return ptr_->operator[](index);
    }

    template<typename I>
    auto at(const I& index) const {
        return ptr_->at(index);
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
    auto operator=(const writer_ptr&) -> auto& = delete;

    explicit writer_ptr(shared_sync<U>& obj) :
    std::unique_lock<std::shared_mutex>(obj.lock), sync_(obj), ptr_(&obj.data) {}

    explicit writer_ptr(writer_ptr&& from) noexcept :
    std::unique_lock<std::shared_mutex>(std::move(from)), sync_(from.sync_), ptr_(from.ptr_) {}

    ~writer_ptr() = default;

    auto operator->() {
        if(!owns_lock()) throw std::runtime_error("write lock error");
        return ptr_;
    }

    auto operator*() -> U& {
        if(!owns_lock()) throw std::runtime_error("write lock error");
        return *ptr_;
    }

    auto operator=(writer_ptr&& from) noexcept -> writer_ptr& {
        if (this != &from) {
            std::unique_lock<std::shared_mutex>::operator=(std::move(from));
            sync_ = from.sync_;
            ptr_ = from.ptr_;
            from.ptr_ = nullptr;
            from.sync_ = nullptr;
        }
        return *this;
    }

    template<typename I>
    auto operator[](const I& index) -> decltype(std::declval<U&>()[index]) {
        return ptr_->operator[](index);
    }

private:
    shared_sync<U> &sync_;  // NOLINT
    U* ptr_;
};

class semaphore_t final {
public:
    semaphore_t(const semaphore_t&) = delete;
    auto operator=(const semaphore_t&) -> auto& = delete;

    explicit semaphore_t(unsigned count = 0) noexcept : count_(count) {
        if(count == 0)
            count_ = std::thread::hardware_concurrency();
        if(count_ == 0)
            count_ = 1;
    }

    ~semaphore_t() {
        wait();
    }

    operator bool() const noexcept {
        return !empty();
    }

    auto operator!() const noexcept {
        return empty();
    }

    auto operator++() -> auto& {
        acquire();
        return *this;
    }

    auto operator--() noexcept -> auto& {
        release();
        return *this;
    }

    void release() noexcept {
        auto id = std::this_thread::get_id();
        const std::lock_guard lock(lock_);
        if(threads_.find(id) == threads_.end()) return;
        threads_.erase(id);
        if(active_) {
            --active_;
            cond_.notify_one();
        }
    }

    void acquire() {
        auto id = std::this_thread::get_id();
        std::unique_lock lock(lock_);
        if(threads_.find(id) != threads_.end()) throw semaphore_cancelled();
        if(++active_ > count_)
            cond_.wait(lock, [this]{return active_ <= count_;});

        if(count_ == ~0U) {
            --active_;
            throw semaphore_cancelled();
        }
        threads_.insert(id);
    }

    auto try_acquire() {
        auto id = std::this_thread::get_id();
        std::unique_lock lock(lock_);
        if(threads_.find(id) != threads_.end()) return false;
        if(++active_ > count_)
            cond_.wait(lock, [this]{return active_ <= count_;});

        if(count_ == ~0U) {
            --active_;
            return false;
        }

        threads_.insert(id);
        return true;
    }

    auto try_acquire_for(const sync_millisecs& timeout) {
        auto id = std::this_thread::get_id();
        std::unique_lock lock(lock_);
        if(threads_.find(id) != threads_.end()) return false;
        ++active_;
        if(active_ <= count_ || cond_.wait_for(lock, timeout, [this]{return active_ <= count_;})) {
            if(count_ == ~0U) {
                --active_;
                throw semaphore_cancelled();
            }
            threads_.insert(id);
            return true;
        }

        --active_;
        return false;
    }

    auto try_acquire_until(const sync_timepoint& time_point) {
        auto id = std::this_thread::get_id();
        std::unique_lock lock(lock_);
        if(threads_.find(id) != threads_.end()) return false;
        ++active_;
        if(active_ <= count_ || cond_.wait_until(lock, time_point, [this]{return active_ <= count_;})) {
            if(count_ == ~0U) {
                --active_;
                throw semaphore_cancelled();
            }
            threads_.insert(id);
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

    auto acquired() const noexcept {
        auto id = std::this_thread::get_id();
        const std::lock_guard lock(lock_);
        return threads_.find(id) != threads_.end();
    }

    auto active() const noexcept {
        const std::lock_guard lock(lock_);
        return active_;
    }

    auto reset() noexcept {
        const std::lock_guard lock(lock_);
        count_ = ~0U;
        cond_.notify_all();
    }

    void wait() noexcept {
        auto id = std::this_thread::get_id();
        std::unique_lock lock(lock_);
        if(threads_.find(id) != threads_.end()) return;
        count_ = ~0U;
        if(!active_) return;
        cond_.notify_all();
        cond_.wait(lock, [this]{return !active_ ;});
    }

    auto resize(unsigned count) noexcept {
        const std::lock_guard lock(lock_);
        count_ = count;
        cond_.notify_all();
    }

private:
    mutable std::mutex lock_;
    std::condition_variable cond_;
    std::unordered_set<std::thread::id> threads_;
    unsigned count_{0};
    unsigned active_{0};
};

class semaphore_guard {
public:
    semaphore_guard(const semaphore_guard&) = delete;
    auto operator=(const semaphore_guard&) -> auto& = delete;

    explicit semaphore_guard(semaphore_t& sem) : sem_(sem) {
        sem_.acquire();
    }

    ~semaphore_guard() {
        sem_.release();
    }

private:
    semaphore_t& sem_;
};

class barrier_t final {
public:
    explicit barrier_t(unsigned limit) noexcept : count_(limit), limit_(limit) {}
    barrier_t(const barrier_t&) = delete;
    auto operator=(const barrier_t&) -> auto& = delete;

    void wait() noexcept {
        std::unique_lock lock(lock_);
        if(!count_) return;
        auto sequence = sequence_;
        if(--count_ == 0) {
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
        if(!count_) return false;
        auto sequence = sequence_;
        if(--count_ == 0) {
            sequence_++;
            count_ = limit_;
            cond_.notify_all();
            return true;
        }
        return cond_.wait_for(lock, timeout, [this, sequence]{return sequence != sequence_;});
    }

    auto wait_until(const sync_timepoint& time_point) noexcept {
        std::unique_lock lock(lock_);
        if(!count_) return false;
        auto sequence = sequence_;
        if(--count_ == 0) {
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
    explicit event_sync(bool reset = false) noexcept : auto_reset_(reset) {}
    event_sync(const event_sync&) = delete;
    ~event_sync() = default;
    auto operator=(const event_sync&) noexcept -> auto& = delete;

    operator bool() const noexcept {
        return is_notified();
    }

    auto operator!() const noexcept {
        return !is_notified();
    }

    auto is_notified() const noexcept -> bool {
        const std::lock_guard lock(lock_);
        return signaled_;
    }

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
    mutable std::mutex lock_;
    std::condition_variable cond_;
    bool signaled_{false};
    bool auto_reset_{true};
};

class wait_group final {
public:
    explicit wait_group(unsigned init) noexcept : count_(init) {}
    wait_group(const wait_group&) = delete;
    wait_group() = default;
    auto operator=(const wait_group&) noexcept -> auto& = delete;

    ~wait_group() {
        wait();
    }

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

    auto release() noexcept {
        const std::lock_guard lock(lock_);
        if(!count_) return true;
        if(--count_ == 0) {
            cond_.notify_all();
            return true;
        }
        return false;
    }

    void wait() noexcept {
        std::unique_lock lock(lock_);
        if(!count_) return;
        cond_.wait(lock, [this]{return !count_ ;});
    }

    auto wait_for(const sync_millisecs& timeout) noexcept {
        std::unique_lock lock(lock_);
        if(!count_) return true;
        return cond_.wait_for(lock, timeout, [this]{return !count_ ;});
    }

    auto wait_until(const sync_timepoint& time_point) noexcept {
        std::unique_lock lock(lock_);
        if(!count_) return true;
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

class sync_group final {
public:
    sync_group() = delete;
    sync_group(const sync_group&) = delete;
    auto operator=(const sync_group&) -> auto& = delete;
    explicit sync_group(wait_group& wg) : wg_(wg) {}
    ~sync_group() {
        wg_.release();
    }

private:
    wait_group& wg_;
};

#if __cplusplus >= 202002L
using arrival = std::barrier<>;

template<typename T>
concept ThreadContainer =
    std::ranges::range<T> &&
    std::same_as<std::remove_cvref_t<std::ranges::range_value_t<T>>, std::thread>;

template<typename Completion = void(*)()>
class sync_arrival final {
public:
    explicit sync_arrival(std::barrier<Completion>& barrier) : barrier_(barrier) {
        static_assert(std::is_invocable_v<Completion>, "Completion must be callable with zero arguments");
    }
    ~sync_arrival() { barrier_.arrive_and_wait(); }

private:
    std::barrier<Completion>& barrier_;
};

template <std::ptrdiff_t Max>
class sync_counting final {
    static_assert(Max > 0, "Max must be a positive integer");

public:
    explicit sync_counting(std::counting_semaphore<Max>& sem) : sem_(sem) {
        sem_.acquire();
    }

    ~sync_counting() {
        sem_.release();
    }

private:
    std::counting_semaphore<Max>& sem_;
};

// for std::jthread call barrier.arrive_and_wait directly or use sync_arrival
template<ThreadContainer Container, typename Func = void(*)()>
void wait_arrival(Container& threads, std::barrier<Func>& barrier) {
    barrier.arrive_and_wait();
    for(auto& thread : threads) {
        if(thread.joinable())
            thread.join();
    }
}
#endif
} // end namespace
#endif
