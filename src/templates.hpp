// Copyright (C) 2023 Tycho Softworks.
// This code is licensed under MIT license.

#ifndef TYCHO_TEMPLATES_HPP_
#define TYCHO_TEMPLATES_HPP_

#include <stdexcept>
#include <cstdint>
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
        return ptr.operator!();
}

template<typename T=void>
constexpr auto void_ptr(void *ptr, std::size_t offset) {
    return reinterpret_cast<T*>(static_cast<uint8_t *>(ptr) + offset);
}

template<typename T>
constexpr auto in_bounds(const T* pointer, const T* base, std::size_t count) {
    if(pointer < base || pointer >= &base[count]) return false;
    if((std::size_t(pointer - base)) % sizeof(T)) return false;
    return true;
}

template<typename T>
constexpr auto const_copy(const T& obj) {
    if constexpr (std::is_pointer_v<T>)
        return *obj;
    else
        return obj;
}

template <typename T>
constexpr auto in_range(const T& value, const T& min, const T& max) {
    return (value >= min && value <= max);
}

template <typename T>
constexpr auto in_list(const T& value, const T& or_else, const std::initializer_list<T>& list) {
    for(const auto& item : list) {
        // cppcheck-suppress useStlAlgorithm
        if(item == value) return value;
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
        if(value < 0) return value;
        mult = abs(mult);
    }

    if(!mult) return value;
    auto adjust = value % mult;
    return (adjust)? value + mult - adjust : value;
}

template<typename T>
inline auto tmparray(std::size_t size) {
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

template<typename E=std::runtime_error>
inline void runtime_assert(bool check, const char *error = "runtime assert") {
    if(!check) throw E(error);
}

constexpr auto align_2(std::size_t size) {
    size--;
    size |= size >> 1;
    size |= size >> 2;
    size |= size >> 4;
    size |= size >> 8;
    size |= size >> 16;
    size++;
    return size;
}

constexpr auto align_n(std::size_t size, std::size_t n) {
    return (size + n - 1) / n * n;
}

template<typename T>
constexpr auto sizeof_2(const T& tmp [[maybe_unused]]) {
    return align_2(sizeof(T));
}

template<typename T>
constexpr auto sizeof_n(const T& tmp [[maybe_unused]], std::size_t n) {
    std::size_t size = sizeof(T);
    return (size + n - 1) / n * n;
}

// some commonly forwarded classes...
class keyfile;
} // end namespace

#endif
