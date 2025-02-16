// Copyright (C) 2025 Tycho Softworks.
// This code is licensed under MIT license.

#ifndef TYCHO_SCAN_HPP_
#define TYCHO_SCAN_HPP_

#include <string_view>
#include <string>
#include <stdexcept>
#include <limits>
#include <cstdint>
#include <cctype>
#include <cmath>

// low level utility scan functions
namespace tycho::scan {
constexpr std::string_view hex_digits("0123456789abcdef");

inline auto count(const std::string_view& text, char code) {
    std::size_t count = 0;
    for(const char ch : text) {
        if(ch == code) 
            ++count;    // cppcheck-suppress useStlAlgorithm
    }
    return count;
}

inline auto pow(long base, long exp) {
    long result = 1;
    for(;;) {
        if(exp & 1)
            result *= base;
        exp >>= 1;
        if(!exp)
            break;
        base *= base;
    }
    return result;
}

inline auto hex(std::string_view& text, unsigned digits = 8) -> uint64_t {
    uint64_t val = 0;

    if(digits > 8) return val;
    while(digits-- && !text.empty()) {
        auto pos = hex_digits.find(char(tolower(text.front())));
        if(pos > 15) return val;
        val <<= 4;
        val |= pos;
        text.remove_prefix(1);
    }
    return val;
}

inline auto text(std::string_view& text, bool quoted = false) -> std::string {
    std::string result;
    char quote = 0;

    if(!quoted && !text.empty() && (text.front() == '\'' || text.front() == '\"' || text.front() == '`')) {
        if(text.size() < 2) return {};  // if only 1 char, no closing quote...
        quote = text.front();
        text.remove_prefix(1);
    }

    while(!text.empty()) {
        if(quote && text.front() == quote) {
            text.remove_prefix(1);
            return result;
        }

        // incomplete quoted string, keep last char in text
        if(quote && text.size() == 1) {
            result += text.front();
            return result;
        }

        if((quoted || quote == '\"') && (text.front() == '\\')) {
            if(text.size() < 2) return {};
            switch(text[1]) {
            case 'n':
                result += '\n';
                break;
            case 'e':
                result += char(27);
                break;
            case 't':
                result += '\t';
                break;
            case 'b':
                result += '\b';
                break;
            case 's':
                result += ' ';
                break;
            case 'r':
                result += '\r';
                break;
            case 'f':
                result += '\f';
                break;
            case '%':
            case '\'':
            case '\"':
            case '`':
            case '\\':
                result += text[1];
                break;
            default:
                return result;
            }
            text.remove_prefix(2);
            continue;
        }
        result += text.front();
        text.remove_prefix(1);
    }
    return result;
}

inline auto value(std::string_view& text, uint64_t max = 2147483647) -> uint64_t {
    uint64_t value = 0;
    while(!text.empty() && isdigit(text.front())) {
        value *= 10;
        value += uint64_t(text.front() - '0');
        if(value > max) {
            value /= 10;
            break;
        }
        text.remove_prefix(1);
    }
    return value;
}

inline auto decimal(std::string_view& text, uint64_t max = 2147483647) -> double {
    auto integer = value(text, max);
    double fraction = 0.0;
    double divisor = 1.0;

    if(!text.empty() && text.front() == '.') {
        text.remove_prefix(1);
        while(!text.empty() && isdigit(text.front())) {
            divisor /= 10;
            fraction += (text.front() - '0') * divisor;
            text.remove_prefix(1);
        }
    }
    return double(integer) + fraction;
}

inline auto real(std::string_view& text, uint64_t max = 2147483647) -> double {
    double number = decimal(text, max);
    if(!text.empty() && (text.front() == 'e' || text.front() == 'E')) {
        text.remove_prefix(1);
        bool negative = false;
        if(!text.empty() && (text.front() == '+' || text.front() == '-')) {
            negative = (text.front() == '-');
            text.remove_prefix(1);
        }

        auto exponent = int(value(text));
        if(negative)
            number *= std::pow(10, -exponent);
        else
            number *= std::pow(10, exponent);
    }
    return number;
}

inline auto match(std::string_view& text, const std::string_view& find, bool insensitive = false) {
    std::size_t pos = 0;
    while(pos < text.size() && pos < find.size()) {
        if(text[pos] == find[pos]) {
            ++pos;
            continue;
        }
        if(insensitive && tolower(text[pos]) == tolower(find[pos])) {
            ++pos;
            continue;
        }
        break;
    }
    if(pos == find.size()) {
        text.remove_prefix(pos);
        return true;
    }
    return false;
}

inline auto spaces(std::string_view& text, std::size_t max = 0, const std::string_view& delim=" \t\f\v\n\r") {
    std::size_t scount{0};
    while(!text.empty() && (!max || (scount < max)) && delim.find_first_of(text.front()) == std::string_view::npos) {
        text.remove_prefix(1);
        ++scount;
    }
    return scount;
}
} // end namespace

// primary scan functions and future scan class
namespace tycho {
inline auto get_string(std::string_view text, bool quoted = false) {
    auto result = scan::text(text, quoted);
    if(!text.empty()) throw std::invalid_argument("Incomplete string");
    return result;
}

inline auto get_quoted(std::string_view text, char quote = '\"') {
    std::string result;
    if(text.size() > 1 && text.front() == text.back() && text.front() == quote)
        result = scan::text(text);
    else
        result = scan::text(text, true);

    if(!text.empty()) throw std::invalid_argument("Incomplete string");
    return result;
}

inline auto get_lower(std::string_view text, char quote = '\"') {
    std::string result;
    if(text.size() > 1 && text.front() == text.back() && text.front() == quote)
        result = scan::text(text);
    else
        result = scan::text(text, true);

    if(!text.empty()) throw std::invalid_argument("Incomplete string");
    for(auto& ch : result)
        // cppcheck-suppress useStlAlgorithm
        ch = char(tolower(ch));
    return result;
}

inline auto get_literal(std::string_view text, char quote = '\"') {
    if(text.size() > 1 && text.front() == text.back() && text.front() == quote) {
        text.remove_prefix(1);
        text.remove_suffix(1);
    }
    return std::string{text};
}

inline auto get_decimal(std::string_view text) {
    bool neg = false;
    if(!text.empty() && text.front() == '-') {
        neg = true;
        text.remove_prefix(1);
    }

    auto value = scan::decimal(text);
    if(!text.empty()) throw std::invalid_argument("Value invalid");
    if(neg) return -value;
    return value;
}

inline auto get_decimal_or(std::string_view text, double or_else = 0.0) {
    try {
        return get_decimal(text);
    }
    catch(const std::exception& e) {
        return or_else;
    }
}

inline auto get_real(std::string_view text) {
    bool neg = false;
    if(!text.empty() && text.front() == '-') {
        neg = true;
        text.remove_prefix(1);
    }

    auto value = scan::real(text);
    if(!text.empty()) throw std::invalid_argument("Value invalid");
    if(neg) return -value;
    return value;
}

inline auto get_real_or(std::string_view text, double or_else = 0.0) {
    try {
        return get_real(text);
    }
    catch(const std::exception& e) {
        return or_else;
    }
}

inline auto get_value(std::string_view text, int32_t min = 1, int32_t max = 65535) -> int32_t {
    bool neg = false;
    if(min < 0 && !text.empty() && text.front() == '-') {
        neg = true;
        text.remove_prefix(1);
    }

    if(text.empty() || !isdigit(text.front())) throw std::invalid_argument("Value missing or invalid");

    auto value = int32_t(scan::value(text, max));
    if(!text.empty() && isdigit(text.front())) throw std::overflow_error("Value too big");
    if(!text.empty()) throw std::invalid_argument("value invalid");
    if(neg) {
        auto nv = -value;
        if(nv < min) throw std::out_of_range("value too small");
        return nv;
    }

    auto rv = value;
    if(min >= 0 && rv < min) throw std::out_of_range("value too small");
    return rv;
}

inline auto get_duration(std::string_view text, bool ms = false) -> unsigned {
    if(text.empty() || !isdigit(text.front())) throw std::invalid_argument("Duration missing or invalid");
    auto value = unsigned(scan::value(text));
    unsigned scale = 1;
    if(ms)
        scale = 1000UL;

    if(!text.empty() && isdigit(text.front())) throw std::overflow_error("Duration too big");
    if(text.empty()) return value;
    if(text.size() == 2 && text == "ms" && ms) {
        text.remove_prefix(2);
        return value;
    }

    if(text.size() == 1) {
        auto ch = tolower(text.front());
        text.remove_prefix(1);
        switch(ch) {
        case 's':
            return value * scale;
        case 'm':
            return value * 60UL * scale;
        case 'h':
            return value * 3600UL * scale;
        case 'd':
            return value * 86400UL * scale;
        case 'w':
            return value * 604800UL * scale;
        default:
            break;
        }
    }

    // hour and minute markers...
    auto count = scan::count(text, ':');
    if(text.front() == ':' && !ms && count < 4) {
        text.remove_prefix(1);
        value *= (scan::pow(60UL, long(count)));
        return value + get_duration(text);
    }
    throw std::invalid_argument("Duration is invalid");
}

inline auto get_bool(std::string_view& text) {
    using namespace scan;

    if(match(text, "true", true) && text.empty()) return true;
    if(match(text, "false", true) && text.empty()) return false;
    if(match(text, "yes", true) && text.empty()) return true;
    if(match(text, "no", true) && text.empty()) return false;
    if(match(text, "t", true) && text.empty()) return true;
    if(match(text, "f", true) && text.empty()) return false;
    if(match(text, "on", true) && text.empty()) return true;
    if(match(text, "off", true) && text.empty()) return false;
    throw std::out_of_range("Bool not valid");
}

template<typename T = unsigned>
inline auto get_hex(std::string_view text) {
    static_assert(
        std::is_same_v<T, unsigned> ||
        std::is_same_v<T, unsigned short> ||
        std::is_same_v<T, unsigned long> ||
        std::is_same_v<T, unsigned long long> ||
        std::is_same_v<T, uint64_t> ||
        std::is_same_v<T, uint32_t> ||
        std::is_same_v<T, uint16_t> ||
        std::is_same_v<T, uint8_t>,
        "Invalid unsigned type" );

    auto value = T(scan::hex(text, sizeof(T) * 2));
    if(!text.empty()) throw std::overflow_error("Value too big or invalid");
    return value;
}

template<typename T = unsigned>
inline auto get_hex_or(std::string_view text, const T& or_else) {
    try {
        return get_hex<T>(text);
    }
    catch(const std::exception& e) {
        return or_else;
    }
}

template<typename T = unsigned>
inline auto get_unsigned(std::string_view text, T min = 0, T max = std::numeric_limits<T>::max()) {
    static_assert(
        std::is_same_v<T, unsigned> ||
        std::is_same_v<T, unsigned short> ||
        std::is_same_v<T, unsigned long> ||
        std::is_same_v<T, uint32_t> ||
        std::is_same_v<T, uint16_t> ||
        std::is_same_v<T, uint8_t>,
        "Invalid unsigned type" );

    if(text.empty() || !isdigit(text.front())) throw std::invalid_argument("Value missing or invalid");
    auto value = T(scan::value(text, max));
    if(!text.empty() && isdigit(text.front())) throw std::overflow_error("Value too big");
    if(!text.empty()) throw std::invalid_argument("value invalid");
    if(value < min) throw std::out_of_range("value too small");
    return value;
}

template<typename T = unsigned>
inline auto get_unsigned_or(std::string_view text, T or_else = 0, T min = 0, T max = std::numeric_limits<T>::max()) {
    try {
        return get_unsigned<T>(text, min, max);
    }
    catch(const std::exception& e) {
        return or_else;
    }
}

template<typename T = int>
inline auto get_integer(std::string_view text, T min = std::numeric_limits<T>::min(), T max = std::numeric_limits<T>::max()) {
    static_assert(
        std::is_same_v<T, int> ||
        std::is_same_v<T, long> ||
        std::is_same_v<T, short> ||
        std::is_same_v<T, int32_t> ||
        std::is_same_v<T, int16_t> ||
        std::is_same_v<T, char> ||
        std::is_same_v<T, int8_t>,
        "Invalid integer type");

    return T(get_value(text, int32_t(min), int32_t(max)));
}

template<typename T = int>
inline auto get_integer_or(std::string_view text, T or_else = 0, T min = std::numeric_limits<T>::min(), T max = std::numeric_limits<T>::max()) {
    try {
        return get_integer<T>(text, min, max);
    }
    catch(const std::exception& e) {
        return T(or_else);
    }
}

inline auto get_bool_or(std::string_view text, bool or_else) {
    try {
        return get_bool(text);
    }
    catch(const std::exception& e) {
        return or_else;
    }
}

inline auto get_count_or(std::string_view text, uint16_t or_else = 0, uint16_t max = 65535) {
    return get_unsigned_or<uint16_t>(text, or_else, 1, max);
}

inline auto get_range_or(std::string_view text, uint32_t or_else = 0, uint32_t min = 1, uint32_t max = 65535UL) {
    return get_unsigned_or<uint32_t>(text, or_else, min, max);
}

inline auto get_seconds_or(std::string_view text, uint32_t or_else = 0) {
    try {
        return get_duration(text);
    }
    catch(const std::exception& e) {
        return or_else;
    }
}

inline auto get_timeout_or(std::string_view text, uint32_t or_else = 0) {
    try {
        return get_duration(text, true);
    }
    catch(const std::exception& e) {
        return or_else;
    }
}

inline auto get_quoted_or(std::string_view text, const std::string& or_else = "") {
    try {
        return get_string(text, true);
    }
    catch(const std::exception& e) {
        return or_else;
    }
}

inline auto get_string_or(std::string_view text, const std::string& or_else = "") {
    try {
        return get_string(text);
    }
    catch(const std::exception& e) {
        return or_else;
    }
}
} // end namespace
#endif
