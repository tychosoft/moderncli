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

class Key {
public:
    virtual ~Key() = default;

    virtual auto data() const -> const uint8_t * = 0;
    virtual auto size() const -> size_t = 0;

    auto bits() const {
        return size() * 8;
    }

    auto operator==(const Key& other) const {
        if(other.size() != size())
            return false;
        return memcmp(data(), other.data(), size()) == 0;
    }

    auto operator!=(const Key& other) const {
        if(other.size() != size())
            return true;
        return memcmp(data(), other.data(), size()) != 0;
    }

    operator key_t() const {
        return std::make_pair(data(), size());
    }

    auto key() const {
        return std::make_pair(data(), size());
    }

protected:
    Key() = default;
};

template <size_t S>
class random_t final : public Key {
public:
    explicit random_t(const uint8_t *raw = nullptr) : Key() {
        if(raw)
            memcpy(&data_, raw, sizeof(data_)); // FlawFinder: ignore
        else
            rand(data_);
    }

    random_t(const random_t& other) : Key() {
        static_assert(other.size() == size());
        memcpy(&data_, &other.data_, sizeof(data_));    // FlawFinder: ignore
    }

    ~random_t() final {
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

    auto data() const -> const uint8_t * final {
        return &data_[0];
    }

    auto size() const -> size_t final {
        return sizeof(data_);
    }

private:
    static_assert(!(S % 8));

    uint8_t data_[S / 8]{0};
};

template <size_t S>
inline auto shared_key(const uint8_t *raw = nullptr) {
    return std::make_shared<random_t<S>>(raw);
}

template <size_t S>
inline auto unique_key(const uint8_t *raw = nullptr) {
    return std::make_unique<random_t<S>>(raw);
}
} // end namespace
#endif
