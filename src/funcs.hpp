// Copyright (C) 2023 Tycho Softworks.
// This code is licensed under MIT license.

#ifndef TYCHO_FUNCS_HPP_
#define TYCHO_FUNCS_HPP_

#include <queue>
#include <thread>
#include <future>
#include <type_traits>
#include <stdexcept>
#include <atomic>
#include <memory>
#include <chrono>
#include <tuple>

namespace tycho {
class future_cancelled : public std::runtime_error {
public:
    future_cancelled() : std::runtime_error("Future cancelled") {};
};

template<typename Func, typename... Args>
inline void detach(Func&& func, Args&&... args) {
    static_assert(std::is_invocable_v<Func, Args...>, "Func must be invocable");
    std::thread([func = std::forward<Func>(func), tuple = std::make_tuple(std::forward<Args>(args)...)]() mutable {
        std::apply(func, std::move(tuple));
    }).detach();
}

template<typename T, typename Pred>
inline auto get_future(std::function<T>& future, Pred pred, std::chrono::milliseconds interval = std::chrono::milliseconds(100)) {
    static_assert(std::is_invocable_v<Pred>, "Pred must be invocable");
    static_assert(std::is_convertible_v<std::invoke_result_t<Pred, T>, bool>, "Pred must return bool");
    do {    // NOLINT
        if(!pred()) throw future_cancelled();
    } while(future.wait_for(interval) != std::future_status::ready);
    return future.get();
}

template<typename Func, typename... Args>
inline auto await(Func&& func, Args&&... args) -> std::future<typename std::invoke_result_t<Func, Args...>> {
    static_assert(std::is_invocable_v<Func, Args...>, "Func must be invocable");
    return std::async(std::launch::async, std::forward<Func>(func), std::forward<Args>(args)...);
}

template<typename Func, typename T = typename std::invoke_result_t<Func>>
inline auto parallel_async(std::size_t count, Func func) {
    static_assert(std::is_invocable_v<Func>, "Func must be invocable");
    if (!count)
        count = std::thread::hardware_concurrency();

    if (!count)
        count = 1;

    auto promise = std::make_shared<std::promise<T>>();
    auto result = promise->get_future();
    auto done = std::make_shared<std::atomic<bool>>(false);
    for(std::size_t i = 0; i < count; ++i) {
        std::thread([promise, done, func]{
            try {
                T val = func();
                if(!done->exchange(true))
                    promise->set_value(std::move(val));
            } catch (...) {
                if(!done->exchange(true))
                    promise->set_exception(std::current_exception());
            }
        }).detach();
    }
    return result;
}

template<typename Func, typename... Args>
class defer final {
public:
    static_assert(std::is_invocable_v<Func, Args...>, "Func must be invocable");
    explicit defer(Func&& func, Args&&... args) : // NOLINT(cppcoreguidelines-rvalue-reference-param-not-moved)
    func_(std::forward<Func>(func)), args_(std::forward<Args>(args)...) {}

    ~defer() {
        std::apply(func_, std::move(args_));
    }

    defer(const defer&) = delete;
    defer(defer&&) = delete;
    auto operator=(const defer&) -> defer& = delete;
    auto operator=(defer&&) -> defer& = delete;

private:
    Func func_;
    std::tuple<Args...> args_;
};

template<typename Func, typename Result = std::invoke_result_t<Func>>
inline auto try_func(Func&& func, Result or_fallback) -> Result {
    static_assert(std::is_invocable_v<Func>, "Func must be invocable");
    try {
        return std::forward<Func>(func)();
    } catch (...) {
        return or_fallback;
    }
}

template<typename Proc>
inline auto try_proc(Proc proc) -> bool {
    static_assert(std::is_invocable_v<Proc>, "Proc must be invocable");
    static_assert(std::is_same_v<std::invoke_result_t<Proc>, void>,"Proc must return void");
    try {
        proc();
        return true;
    }
    catch(...) {
        return false;
    }
}
} // end namespace
#endif
