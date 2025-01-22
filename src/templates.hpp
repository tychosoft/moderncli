// Copyright (C) 2023 Tycho Softworks.
// This code is licensed under MIT license.

#ifndef TYCHO_TEMPLATES_HPP_
#define TYCHO_TEMPLATES_HPP_

#include <type_traits>
#include <functional>
#include <stdexcept>
#include <memory>

namespace tycho {
template<typename T>
constexpr auto is(const T& object) {
    return object.operator bool();
}

template<typename T>
constexpr auto is_null(const T& ptr) {
    if constexpr (std::is_pointer_v<T>)
        return ptr == nullptr;
    else
        return ptr.operator bool();
}

template<typename T>
constexpr auto bound_ptr(const T* pointer, const T* base, std::size_t count) {
    if(pointer < base || pointer >= &base[count])
        return false;
    if((std::size_t(pointer - base)) % sizeof(T))
        return false;
    return true;
}

template<typename T>
constexpr auto deref_ptr(T *ptr) -> T& {
    return *ptr;
}

template<typename T>
constexpr auto const_copy(const T& obj) {
    if constexpr (std::is_pointer_v<T>)
        return *obj;
    else
        return obj;
}

template<typename T>
constexpr auto const_max(const T& x, const T& y) {
    return (x > y) ? x : y;
}

template<typename T>
constexpr auto const_min(const T& x, const T& y) {
    return (x > y) ? y : x;
}

template<typename T>
constexpr auto const_clamp(const T& value, const T& min, const T& max) {
    return (value < min) ? min : ((value > max) ? max : value); // NOLINT
}

template <typename T>
constexpr auto in_range(const T& value, const T& min, const T& max) {
    return (value >= min && value <= max);
}

template <typename T>
constexpr auto in_list(const T& value, const T& or_else, const std::initializer_list<T>& list) {
    for(const auto& item : list) {
        // cppcheck-suppress useStlAlgorithm
        if(item == value)
            return value;
    }
    return or_else;
}

template<typename T>
constexpr auto const_abs(T value) {
    static_assert(std::is_integral_v<T> && !std::is_unsigned_v<T>, "T must be signed number");

    return (value < 0) ? -value : value;
}

template<typename T>
constexpr auto multiple_of(T value, T mult) {
    static_assert(std::is_integral_v<T>, "T must be integral");

    if constexpr (!std::is_unsigned_v<T>) {
        if(value < 0)
            return value;

        mult = abs(mult);
    }

    if(!mult)
        return value;

    auto adjust = value % mult;
    return (adjust)? value + mult - adjust : value;
}

template<typename T>
inline auto tmparray(size_t size) {
    return std::make_unique<T[]>(size);
}

class init final {
public:
    using run_t = void (*)();

    init(const init&) = delete;
    auto operator=(const init&) -> auto& = delete;

    explicit init(const run_t start, const run_t stop = {[](){}}) : exit_(stop) {
        (start)();
    }

    ~init() {
        (exit_)();
    }

private:
    const run_t exit_{[](){}};
};

class defer final {
public:
    template <typename F>
    explicit defer(F&& func) : action_(std::forward<F>(func)) {}
    ~defer() {if (action_) action_();}

    defer(const defer&) = delete;
    defer(defer&&) = delete;
    auto operator=(const defer&) -> auto& = delete;
    auto operator=(defer&&) -> auto& = delete;

private:
    std::function<void()> action_;
};

template<typename T>
inline auto try_func(std::function<T()> func, const T& or_value) {
    try {
        return func();
    }
    catch(...) {
        return or_value;
    }
}

template<typename Func>
inline auto try_proc(Func proc) {
    try {
        proc();
        return true;
    }
    catch(...) {
        return false;
    }
}

inline void runtime_assert(bool check, const char *error = "runtime assert") {
    if(!check) throw std::runtime_error(error);
}

// some commonly forwarded classes...
class keyfile;
} // end namespace

#endif
