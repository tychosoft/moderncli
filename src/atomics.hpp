// Copyright (C) 2023 Tycho Softworks.
// This code is licensed under MIT license.

#ifndef ATOMICS_HPP_
#define ATOMICS_HPP_

#include <memory>
#include <atomic>
#include <optional>
#include <stdexcept>
#include <type_traits>

namespace atomics {
template<typename T = unsigned>
class sequence_t final {
public:
    sequence_t() noexcept = default;

    explicit sequence_t(T initial) noexcept : seq_(initial) {};

    sequence_t(const sequence_t& from) noexcept {
        seq_.store(from.seq_.load(std::memory_order_relaxed), std::memory_order_release);
    }

    auto operator=(const sequence_t& from) noexcept -> sequence_t& {
        if(&from != this)
            seq_.store(from.seq_.load(std::memory_order_relaxed), std::memory_order_release);
        return *this;
    }

    auto operator*() noexcept {
        return seq_.fetch_add(1);
    }

    auto operator=(const T v) noexcept -> sequence_t& {
        seq_.store(v, std::memory_order_relaxed);
        return *this;
    }

    operator T() const noexcept {
        return seq_.fetch_add(1);
    }

    void set(const T v) noexcept {
        seq_.store(v, std::memory_order_relaxed);
    }

    auto is_lock_free() const noexcept {
        return seq_.is_lock_free();
    }

private:
    static_assert(std::is_integral_v<T>, "Type must be numeric");

    mutable std::atomic<T> seq_{0};
};

class once_t final {
public:
    once_t() = default;
    once_t(const once_t&) = delete;
    auto operator=(const once_t&) = delete;

    operator bool() const noexcept {
        return flag_.exchange(false);
    }

    auto operator!() const noexcept {
        return !flag_.exchange(false);
    }

    void reset() noexcept {
        flag_.store(true, std::memory_order_release);
    }
private:
    mutable std::atomic<bool> flag_{true};
};

template<typename T, size_t S>
class stack_t final {
public:
    stack_t() = default;
    stack_t(const stack_t&) = delete;
    auto operator=(const stack_t&) -> stack_t& = delete;

    operator bool() const noexcept {
        const auto count = count_.load();
        return count > 0;
    }

    auto operator!() const noexcept {
        const auto count = count_.load();
        return count < 1;
    }

    auto operator*() noexcept {
        return pop();
    }

    auto operator<=(const T& item) {
        return push(item);
    }

    auto size() const noexcept -> size_t {
        auto count = count_.load();
        if(count < 0)
            return 0;
        if(count > S)
            return S;
        return count;
    }

    auto push(const T& item) noexcept {
        const auto count = count_.fetch_add(1);
        if(count >= S) {
            count_.fetch_sub(1);
            return false;
        }
        data_[count] = item;
        return true;
    }

    auto pull(T& item) noexcept {
        const auto count = count_.fetch_sub(1);
        if(count < 0) {
            count_.fetch_add(1);
            return false;
        }
        item = data_[count];
        return true;
    }

    auto pop() noexcept -> std::optional<T> {
        const auto count = count_.fetch_sub(1);
        if(count < 0) {
            count_.fetch_add(1);
            return {};
        }
        return data_[count];
    }

private:
    static_assert(S > 2, "Queue size must be bigger than 2");

    std::atomic<int> count_{0};
    T data_[S];
};

template<typename T, size_t S>
class buffer_t final {
public:
    buffer_t() = default;
    buffer_t(const buffer_t&) = delete;
    auto operator=(const buffer_t&) -> buffer_t& = delete;

    operator bool() const noexcept {
        const auto head = head_.load(std::memory_order_relaxed);
        const auto tail = tail_.load(std::memory_order_relaxed);
        return head != tail;
    }

    auto operator!() const noexcept {
        const auto head = head_.load(std::memory_order_relaxed);
        const auto tail = tail_.load(std::memory_order_relaxed);
        return head == tail;
    }

    auto operator*() noexcept {
        return pop();
    }

    auto operator<=(const T& item) noexcept {
        return push(item);
    }

    auto push(const T& item) noexcept {
        const auto tail = tail_.load(std::memory_order_relaxed);
        auto next = tail;
        if(++next >= S)
            next -= S;

        const auto head = head_.load(std::memory_order_acquire);
        if(next == head)
            return false;

        data_[tail] = item;
        tail_.store(next, std::memory_order_release);
        return true;
    }

    auto pull(T& item) noexcept {
        auto head = head_.load(std::memory_order_relaxed);
        const auto tail = tail_.load(std::memory_order_acquire);
        if(head == tail)
            return false;

        item = data_[head];
        if(++head >= S)
            head -= S;

        tail_.store(tail, std::memory_order_release);
        return true;
    }

    auto pop() noexcept -> std::optional<T> {
        auto head = head_.load(std::memory_order_relaxed);
        const auto tail = tail_.load(std::memory_order_acquire);
        if(head == tail)
            return {};

        auto item = data_[head];
        if(++head >= S)
            head -= S;

        tail_.store(tail, std::memory_order_release);
        return head;
    }

private:
    static_assert(S > 2, "Queue size must be bigger than 2");

    std::atomic<unsigned> head_{0U}, tail_{0U};
    T data_[S];
};
} // end namespace
#endif
