// Copyright (C) 2023 Tycho Softworks.
// This code is licensed under MIT license.

#ifndef RANDOM_HPP_
#define RANDOM_HPP_

#include <type_traits>
#include <memory>
#include <random>
#include <array>
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

inline auto to_b64(const uint8_t *data, size_t size) {
    constexpr std::array<char, 64> base64_chars = {
        'A','B','C','D','E','F','G','H','I','J','K','L','M','N','O','P',
        'Q','R','S','T','U','V','W','X','Y','Z','a','b','c','d','e','f',
        'g','h','i','j','k','l','m','n','o','p','q','r','s','t','u','v',
        'w','x','y','z','0','1','2','3','4','5','6','7','8','9','+','/'
    };

    std::string out;

    for (size_t i = 0; i < size; i += 3) {
        uint32_t c = 0;
        for (size_t j = 0; j < 3; ++j) {
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

inline auto from_b64(std::string_view from, uint8_t *to, size_t max) {
    size_t size = from.size(), pad = 0;
    while(size > 0) {
        if(from[size - 1] == '=') {
            ++pad;
            --size;
        }
        else
            break;
    }

    const size_t out = ((size * 6) / 8) - pad;
    if(out > max)
        return size_t(0);

    uint32_t val = 0;
    size_t bits = 0, count = 0;

    for (const auto &ch : from) {
        auto index = base64_index(ch);
        if (index >= 0) {
            val = (val << 6) | static_cast<uint32_t>(index);
            bits += 6;
            if (bits >= 8) {
                *(to++) = (val >> (bits - 8));
                ++count;
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

    virtual auto data() const noexcept -> const uint8_t * = 0;
    virtual auto size() const noexcept -> size_t = 0;

    virtual auto bits() const noexcept -> size_t {
        return size() * 8;
    }

    auto operator==(const Key& other) const noexcept {
        if(other.size() != size())
            return false;
        return memcmp(data(), other.data(), size()) == 0;
    }

    auto operator!=(const Key& other) const noexcept {
        if(other.size() != size())
            return true;
        return memcmp(data(), other.data(), size()) != 0;
    }

    operator bool() const noexcept {
        return size() > 0;
    }

    auto operator!() const noexcept {
        return size() == 0;
    }

    operator key_t() const {
        return std::make_pair(data(), size());
    }

    auto operator*() const {
        return std::make_pair(data(), size());
    }

    auto key() const {
        return std::make_pair(data(), size());
    }

    auto to_string() const {
        return to_b64(data(), size());
    }

protected:
    Key() = default;
};

template <size_t S>
class random_t final : public Key {
public:
    random_t() : Key() {
        rand(data_);
    }

    explicit random_t(const std::string_view& b64) : Key() {
        from_b64(b64, data_, sizeof(data_));
    }

    random_t(const random_t& other) : Key() {
        static_assert(other.size() == size());
        memcpy(&data_, &other.data_, sizeof(data_));    // FlawFinder: ignore
    }

    ~random_t() final {
        zero(data_);
    }

    auto operator=(const random_t& other) -> auto& {
        static_assert(other.size() == size());
        if(this != &other)
            memcpy(&data_, &other.data_, sizeof(data_));    // FlawFinder: ignore
        return *this;
    }

    auto operator=(const std::string_view& b64) -> auto& {
        zero(data_);
        from_b64(b64, data_, sizeof(data_));
        return *this;
    }

    auto data() const noexcept -> const uint8_t * final {
        return data_;
    }

    auto size() const noexcept -> size_t final {
        return sizeof(data_);
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

template <size_t S>
inline auto shared_key(const std::string_view& b64) {
    return std::make_shared<random_t<S>>(b64);
}
} // end namespace

#ifdef  PRINT_HPP_
template <> class fmt::formatter<crypto::Key> {
public:
    static constexpr auto parse(format_parse_context& ctx) {
        return ctx.begin();
    }

    template <typename Context>
    constexpr auto format(crypto::Key const& key, Context& ctx) const {
        return format_to(ctx.out(), "{}", key.to_string());
    }
};
#endif

inline auto operator<<(std::ostream& out, const crypto::Key& key) -> std::ostream& {
    out << key.to_string();
    return out;
}
#endif
