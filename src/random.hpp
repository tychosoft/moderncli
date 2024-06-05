// Copyright (C) 2023 Tycho Softworks.
// This code is licensed under MIT license.

#ifndef RANDOM_HPP_
#define RANDOM_HPP_

#include <type_traits>
#include <memory>
#include <random>
#include <utility>
#include <cstring>
#include <openssl/rand.h>

namespace crypto {
using key_t = std::pair<const uint8_t *, size_t>;

inline constexpr auto md5_key = 128UL;
inline constexpr auto sha1_key = 160UL;
inline constexpr auto sha256_key = 256UL;
inline constexpr auto sha384_key = 384UL;
inline constexpr auto sha512_key = 512UL;

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
    explicit random_t(const uint8_t *raw = nullptr) {
        if(raw)
            memcpy(&data_, raw, sizeof(data_)); // FlawFinder: ignore
        else
            rand(data_);
    }

    random_t(const random_t& other) {
        static_assert(other.size() == size());
        memcpy(&data_, &other.data_, sizeof(data_));    // FlawFinder: ignore
    }

    ~random_t() {
        zero(data_);
    }

    auto operator=(const random_t& other) -> random_t& {
        static_assert(other.size() == size());
        if(this != &other)
            memcpy(&data_, &other.data_, sizeof(data_));    // FlawFinder: ignore
        return *this;
    }

    auto operator=(const uint8_t *raw) -> random_t& {
        if(raw)
            memcpy(&data_, raw, sizeof(data_));     // FlawFinder: ignore
        return *this;
    }

    auto operator==(const random_t& other) const {
        if(other.size() != size())
            return false;
        static_assert(other.size() == size());
        return memcmp(&data_, &other.data_, S / 8) == 0;
    }

    auto operator!=(const random_t& other) const {
        if(other.size() != size())
            return true;
        return memcmp(&data_, &other.data_, S / 8) != 0;
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

    auto bits() const {
        return S;
    }

    auto size() const {
        return sizeof(data_);
    }

    auto data() {
        return &data_[0];
    }

private:
    static_assert(!(S % 8));

    uint8_t data_[S / 8]{0};
};

template <size_t S>
inline auto shared_key() {
    return std::make_shared<random_t<S>>();
}

template <size_t S>
inline auto unique_key() {
    return std::make_unique<random_t<S>>();
}
} // end namespace
#endif
