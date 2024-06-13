// Copyright (C) 2023 Tycho Softworks.
// This code is licensed under MIT license.

#ifndef TEMPLATES_HPP_
#define TEMPLATES_HPP_

#include <type_traits>
#include <functional>
#include <memory>

namespace tycho {
template<typename T>
constexpr auto is(const T& object) {
    return object.operator bool();
}

template<typename T>
constexpr auto is_null(const T ptr) {
    if constexpr (std::is_pointer_v<T>)
        return ptr == nullptr;
    else
        return false;
}

template<typename T>
constexpr auto bound_ptr(const T* pointer, const T* base, size_t count) {
    if(pointer < base || pointer >= &base[count])
        return false;
    if((static_cast<size_t>(pointer - base)) % sizeof(T))
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

template<typename T, class... Args>
inline auto temporary(const Args&... args) {
    return std::make_unique<T>(args...);
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

    explicit init(run_t start, run_t stop = {[](){}}) : exit_(stop) {
        (start)();
    }

    ~init() {
        (exit_)();
    }

private:
    run_t exit_{[](){}};
};

class defer final {
public:
    defer() = delete;
    defer(const defer&) = delete;
    auto operator=(const defer&) = delete;

    explicit defer(std::function<void()> func) : action_(std::move(func)) {}
    ~defer() {action_();}

private:
    std::function<void()> action_;
};
} // end namespace

#endif