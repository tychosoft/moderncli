// Copyright (C) 2023 Tycho Softworks.
// This code is licensed under MIT license.

#ifndef RANDOM_HPP_
#define RANDOM_HPP_

#include <type_traits>
#include <random>
#include <utility>
#include <cstring>
#include <openssl/rand.h>

namespace crypto {
using key_t = std::pair<const uint8_t *, size_t>;
template <typename T>
inline void rand(T& data) {
    static_assert(std::is_trivial_v<T>, "T must be Trivial type");

    auto ptr = reinterpret_cast<uint8_t *>(&data);
    ::RAND_bytes(ptr, static_cast<int>(sizeof(data)));
}

inline void rand(uint8_t *ptr, size_t size) {
    ::RAND_bytes(ptr, static_cast<int>(size));
}

template <typename T>
inline void zero(T& data) {
    static_assert(std::is_trivial_v<T>, "T must be Trivial type");

    auto ptr = reinterpret_cast<uint8_t *>(&data);
    memset(ptr, 0, sizeof(data));
}

inline void zero(uint8_t *ptr, size_t size) {
    memset(ptr, 0, size);
}

template <size_t S>
class random_t final {
public:
    random_t() { // NOLINT
        rand(data_);
    }

    random_t(const random_t& other) {    // NOLINT
        memcpy(&data_, &other.data_, sizeof(data_));
    }

    ~random_t() {
        zero(data_);
    }

    auto operator=(const random_t& other) -> random_t& {
        if(this != &other)
            memcpy(&data_, &other.data_, S);    // FlawFinder: ignore
        return *this;
    }

    auto operator==(const random_t& other) const {
        return memcmp(&data_, &other.data_, S) == 0;
    }

    auto operator!=(const random_t& other) const {
        return memcmp(&data_, &other.data_, S) != 0;
    }

    operator key_t() const {
        return std::make_pair(data(), size());
    }

    auto key() const {
        return std::make_pair(data(), size());
    }

    auto data() const {
        return &data_[0];
    }

    auto size() const {
        return sizeof(data_);
    }

private:
    uint8_t data_[S];
};
} // end namespace
#endif
