// Copyright (C) 2023 Tycho Softworks.
// This code is licensed under MIT license.

#ifndef TYCHO_RANDOM_HPP_
#define TYCHO_RANDOM_HPP_

#include "encoding.hpp"

#include <type_traits>
#include <iostream>
#include <random>
#include <utility>
#include <cstring>
#include <memory>
#include <openssl/rand.h>

namespace tycho::crypto {
using key_t = std::pair<const uint8_t *, std::size_t>;

inline constexpr auto salt = 64UL;
inline constexpr auto md5_key = 128UL;
inline constexpr auto sha1_key = 160UL;
inline constexpr auto ecdsa_key = 256UL;
inline constexpr auto sha256_key = 256UL;
inline constexpr auto sha384_key = 384UL;
inline constexpr auto sha512_key = 512UL;
inline constexpr auto aes128_key = 128UL;
inline constexpr auto aes256_key = 256UL;

template <typename T>
inline void rand(T& data) {
    static_assert(std::is_trivial_v<T>, "T must be Trivial type");

    auto ptr = reinterpret_cast<uint8_t *>(&data);
    ::RAND_bytes(ptr, int(sizeof(data)));
}

inline void rand(uint8_t *ptr, std::size_t size) {
    ::RAND_bytes(ptr, int(size));
}

template <typename T>
inline void zero(T& data) {
    static_assert(std::is_trivial_v<T>, "T must be Trivial type");

    auto ptr = reinterpret_cast<uint8_t *>(&data);
    memset(ptr, 0, sizeof(data));
}

inline void zero(uint8_t *ptr, std::size_t size) {
    memset(ptr, 0, size);
}

inline auto random_dist(int min, int max) {
    std::random_device rd;
    std::mt19937 rand_gen(rd());
    std::uniform_int_distribution<> dist(min, max);
    return dist(rand_gen);
}

inline auto make_key(const uint8_t *data, std::size_t size) -> key_t {
    return std::make_pair(data, size);
}

inline auto make_key(const std::string_view& from) {
    return make_key(reinterpret_cast<const uint8_t *>(from.data()), from.size());
}

template <std::size_t S>
class random_t final {
public:
    random_t() {
        rand(data_);
    }

    explicit random_t(const key_t key) {
        zero(data_);
        if(key.first && key.second && key.second <= (S / 8))
            memcpy(data_, key.first, key.second);   // FlawFinder: ignore
        else
            throw std::runtime_error("key size mismatch");
    }

    random_t(const random_t& other) {
        static_assert(other.size() == size());
        memcpy(data_, other.data_, sizeof(data_));  // FlawFinder: ignore
    }

    ~random_t() {
        zero(data_);
    }

    operator key_t() const {
        return std::make_pair(data_, S / 8);
    }

    auto operator *() const {
        return std::make_pair(data_, S / 8);
    }

    auto operator=(const key_t key) -> auto& {
        zero(data_);
        if(key.first && key.second && key.second  <= (S / 8))
            memcpy(data_, key.first, key.second);       // FlawFinder: ignore
        else
            throw std::runtime_error("key size mismatch");
        return *this;
    }

    auto operator=(const random_t& other) -> auto& {
        static_assert(other.size() == size());
        if(this != &other)
            memcpy(data_, other.data_, sizeof(data_));  // FlawFinder: ignore
        return *this;
    }

    auto operator==(const random_t& other) const noexcept {
        return memcmp(data(), other.data(), S / 8) == 0;
    }

    auto operator!=(const random_t& other) const noexcept {
        return memcmp(data(), other.data(), S / 8) != 0;
    }

    auto data() const noexcept -> const uint8_t * {
        return data_;
    }

    auto size() const noexcept -> std::size_t {
        return sizeof(data_);
    }

    auto bits() const noexcept {
        return S;
    }

private:
    static_assert(S != 0 && !(S % 8), "S must be byte aligned non-zero");

    uint8_t data_[S / 8]{0};
};

template <std::size_t S>
inline auto shared_key() {
    return std::make_shared<random_t<S>>();
}

template <std::size_t S>
inline auto unique_key() {
    return std::make_unique<random_t<S>>();
}

using salt_t = random_t<salt>;
} // end namespace

inline auto operator<<(std::ostream& out, const tycho::crypto::key_t& key) -> std::ostream& {
    out << tycho::crypto::to_b64(key);
    return out;
}
#endif
