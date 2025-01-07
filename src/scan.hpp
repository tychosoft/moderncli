// Copyright (C) 2025 Tycho Softworks.
// This code is licensed under MIT license.

#ifndef TYCHO_SCAN_HPP_
#define TYCHO_SCAN_HPP_

#include <string_view>
#include <stdexcept>
#include <cstdint>
#include <cctype>

// low level utility scan functions
namespace tycho::scan {
inline auto value(std::string_view& text, uint64_t max = 2147483647) -> uint32_t {
    uint64_t value = 0;
    while(!text.empty() && isdigit(text.front())) {
        value *= 10;
        value += (text.front()) - '0';
        if(value > max) {
            value /= 10;
            break;
        }
        text.remove_prefix(1);
    }
    return value;
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

inline auto spaces(std::string_view& text, std::size_t max = 0) {
    std::size_t count{0};
    while(!text.empty() && (!max || (count < max)) && isdigit(text.front())) {
        text.remove_prefix(1);
        ++count;
    }
    return count;
}
} // end namespace

// primary scan functions and future scan class
namespace tycho {
inline auto get_value(std::string_view& text, int32_t min = 1, int32_t max = 65535) -> int32_t {
    bool neg = false;
    if(min < 0 && !text.empty() && text.front() == '-') {
        neg = true;
        text.remove_prefix(1);
    }

    if(text.empty() || !isdigit(text.front()))
        throw std::invalid_argument("Value missing or invalid");

    auto value = scan::value(text, max);
    if(!text.empty() && isdigit(text.front()))
        throw std::overflow_error("Value too big");

    if(neg) {
        auto nv = -static_cast<int32_t>(value);
        if(nv < min)
            throw std::out_of_range("value too small");
        return nv;
    }

    auto rv = static_cast<int32_t>(value);
    if(min >= 0 && rv < min)
            throw std::out_of_range("value too small");
    return rv;
}

inline auto get_bool(std::string_view& text) {
    using namespace scan;

    if(match(text, "true", true) && text.empty())
        return true;
    if(match(text, "false", true) && text.empty())
        return false;
    if(match(text, "yes", true) && text.empty())
        return true;
    if(match(text, "no", true) && text.empty())
        return false;
    if(match(text, "t", true) && text.empty())
        return true;
    if(match(text, "f", true) && text.empty())
        return false;
    if(match(text, "on", true) && text.empty())
        return true;
    if(match(text, "off", true) && text.empty())
        return false;
    throw std::out_of_range("Bool not valid");
}
} // end namespace
#endif
