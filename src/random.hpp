// Copyright (C) 2023 Tycho Softworks.
// This code is licensed under MIT license.

#ifndef TYCHO_RANDOM_HPP_
#define TYCHO_RANDOM_HPP_

#include <type_traits>
#include <iostream>
#include <memory>
#include <random>
#include <array>
#include <utility>
#include <cstring>
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

inline auto to_b64(const uint8_t *data, std::size_t size) {
    constexpr std::array<char, 64> base64_chars = {
        'A','B','C','D','E','F','G','H','I','J','K','L','M','N','O','P',
        'Q','R','S','T','U','V','W','X','Y','Z','a','b','c','d','e','f',
        'g','h','i','j','k','l','m','n','o','p','q','r','s','t','u','v',
        'w','x','y','z','0','1','2','3','4','5','6','7','8','9','+','/'
    };

    std::string out;

    for (std::size_t i = 0; i < size; i += 3) {
        uint32_t c = 0;
        for (std::size_t j = 0; j < 3; ++j) {
            c <<= 8;
            if(i + j < size)
                c |= static_cast<uint32_t>(data[i + j]);
        }
        out += base64_chars[(c >> 18) & 0x3F];
        out += base64_chars[(c >> 12) & 0x3F];
        if(i < (size - 1))
            out += base64_chars[(c >> 6) & 0x3F];
        if(i < (size - 2))
            out += base64_chars[c & 0x3F];
    }

    if (size % 3 == 1)
        out += "==";
    else if (size % 3 == 2)
        out += '=';
    return out;
}

constexpr auto base64_index(char c) {
    if (c >= 'A' && c <= 'Z')
        return c - 'A';

    if (c >= 'a' && c <= 'z')
        return c - 'a' + 26;

    if (c >= '0' && c <= '9')
        return c - '0' + 52;

    if (c == '+')
        return 62;

    if (c == '/')
        return 63;

    return -1;
}

inline auto size_b64(std::string_view from) {
    auto size = from.size();
    if(!size)
        return std::size_t(0);

    --size;
    while(size && from[size] == '=')
        --size;
    auto out = ((size / 4) * 3) + 3;

    switch(size % 4) {
    case 0:
        return out;
    case 1:
        return out - 2;
    case 2:
        return out - 1;
    default:
        return std::size_t(0);
    }
}

inline auto from_b64(std::string_view from, uint8_t *to, std::size_t max) {
    auto out = size_b64(from);

    if(out > max)
        return std::size_t(0);

    uint32_t val = 0;
    std::size_t bits = 0, count = 0;

    for (const auto &ch : from) {
        if(count >= out)
            break;
        auto index = base64_index(ch);
        if (index >= 0) {
            val = (val << 6) | static_cast<uint32_t>(index);
            bits += 6;
            if (bits >= 8) {
                to[count++] = (val >> (bits - 8));
                bits -= 8;
            }
        }
    }
    return count;
}

template <typename T>
inline void rand(T& data) {
    static_assert(std::is_trivial_v<T>, "T must be Trivial type");

    auto ptr = reinterpret_cast<uint8_t *>(&data);
    ::RAND_bytes(ptr, static_cast<int>(sizeof(data)));
}

inline void rand(uint8_t *ptr, std::size_t size) {
    ::RAND_bytes(ptr, static_cast<int>(size));
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

inline auto to_b64(const key_t& key) {
    return to_b64(key.first, key.second);
}

template <std::size_t S>
class random_t final {
public:
    random_t() {
        rand(data_);
    }

    explicit random_t(const std::string_view& b64) {
        if(from_b64(b64, data_, sizeof(data_)) != sizeof(data_))
            throw std::runtime_error("key size mismatch");
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

    auto operator=(const std::string_view& b64) -> auto& {
        zero(data_);
        if(from_b64(b64, data_, sizeof(data_)) != sizeof(data_))
            throw std::runtime_error("key size mismatch");
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

    auto to_string() const {
        return to_b64(data_, S / 8);
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

template <std::size_t S>
inline auto shared_key(const std::string_view& b64) {
    return std::make_shared<random_t<S>>(b64);
}

using salt_t = random_t<salt>;
} // end namespace

#ifdef  TYCHO_PRINT_HPP_
template <> class fmt::formatter<tycho::crypto::key_t const> {
public:
    static constexpr auto parse(format_parse_context& ctx) {
        return ctx.begin();
    }

    template <typename Context>
    constexpr auto format(tycho::crypto::key_t const& key, Context& ctx) const {
        return format_to(ctx.out(), "{}", tycho::crypto::to_b64(key));
    }
};
#endif

inline auto operator<<(std::ostream& out, const tycho::crypto::key_t& key) -> std::ostream& {
    out << tycho::crypto::to_b64(key);
    return out;
}
#endif
