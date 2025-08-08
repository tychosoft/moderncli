// Copyright (C) 2023 Tycho Softworks.
// This code is licensed under MIT license.

#ifndef TYCHO_ENCODING_HPP_
#define TYCHO_ENCODING_HPP_

#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <array>
#include <cstring>
#include <cstdint>

namespace tycho::crypto {
using key_t = std::pair<const uint8_t *, std::size_t>;
} // namespace tycho::crypto

namespace tycho {
inline auto to_b64(const uint8_t *data, std::size_t size) {
    constexpr std::array<char, 64> base64_chars = {
    'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
    'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f',
    'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v',
    'w', 'x', 'y', 'z', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '+', '/'};

    std::string out;

    for (std::size_t i = 0; i < size; i += 3) {
        uint32_t c = 0;
        for (std::size_t j = 0; j < 3; ++j) {
            c <<= 8;
            if (i + j < size)
                c |= uint32_t(data[i + j]);
        }

        out += base64_chars[(c >> 18) & 0x3F];
        out += base64_chars[(c >> 12) & 0x3F];
        if (i < (size - 1))
            out += base64_chars[(c >> 6) & 0x3F];
        if (i < (size - 2))
            out += base64_chars[c & 0x3F];
    }

    if (size % 3 == 1)
        out += "==";
    else if (size % 3 == 2)
        out += '=';
    return out;
}

constexpr auto base64_index(char c) {
    if (c >= 'A' && c <= 'Z') return c - 'A';
    if (c >= 'a' && c <= 'z') return c - 'a' + 26;
    if (c >= '0' && c <= '9') return c - '0' + 52;
    if (c == '+') return 62;
    if (c == '/') return 63;
    return -1;
}

inline auto size_b64(std::string_view from) {
    auto size = from.size();
    if (!size) return std::size_t(0);
    --size;
    while (size && from[size] == '=')
        --size;
    auto out = ((size / 4) * 3) + 3;

    switch (size % 4) {
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

inline auto from_b64(std::string_view from, uint8_t *to, std::size_t maxsize) {
    auto out = size_b64(from);

    if (out > maxsize) return std::size_t(0);
    uint32_t val = 0;
    std::size_t bits = 0, count = 0;
    for (const auto& ch : from) {
        if (count >= out) break;
        auto index = base64_index(ch);
        if (index >= 0) {
            val = (val << 6) | uint32_t(index);
            bits += 6;
            if (bits >= 8) {
                to[count++] = (val >> (bits - 8));
                bits -= 8;
            }
        }
    }
    return count;
}

inline auto to_hex(const uint8_t *from, std::size_t size) {
    std::string out;
    out.resize(size * 2);
    auto hex = out.data();

    for (auto pos = std::size_t(0); pos < size; ++pos) {
        snprintf(hex, 3, "%02x", from[pos]);
        hex += 2;
    }
    return out;
}

inline auto to_hex(const std::string_view str) {
    return to_hex(reinterpret_cast<const uint8_t *>(str.data()), str.size());
}

inline auto to_hex(const crypto::key_t& key) {
    return to_hex(key.first, key.second);
}

inline auto from_hex(std::string_view from, uint8_t *to, std::size_t size) {
    auto hex = from.data();
    auto maxsize = size * 2;
    maxsize = std::min(from.size(), maxsize);
    for (auto pos = std::size_t(0); pos < maxsize; pos += 2) {
        char buf[3]{};
        buf[0] = hex[pos];
        buf[1] = hex[pos + 1];
        buf[2] = 0;
        char *end = nullptr;
        auto value = strtoul(buf, &end, 16);
        if (*end != 0) return pos / 2;
        *(to++) = uint8_t(value);
    }
    return maxsize / 2;
}
} // namespace tycho

namespace tycho::crypto {
inline auto to_b64(const uint8_t *from, size_t size) {
    return tycho::to_b64(from, size);
}

inline auto to_b64(const key_t& key) {
    return tycho::to_b64(key.first, key.second);
}
} // namespace tycho::crypto
#endif
