// Copyright (C) 2025 Tycho Softworks.
// This code is licensed under MIT license.

#ifndef TYCHO_SCAN_HPP_
#define TYCHO_SCAN_HPP_

#include <string_view>
#include <stdexcept>
#include <chrono>
#include <cstdint>
#include <cctype>

// low level utility scan functions
namespace tycho::scan {
inline auto text(std::string_view& text) -> std::string {
    std::string result;
    char quote = 0;

    if(!text.empty() && (text.front() == '\'' || text.front() == '\"' || text.front() == '`')) {
        if(text.size() < 2) // if only 1 char, no closing quote...
            return {};

        quote = text.front();
        text.remove_prefix(1);
    }

    while(!text.empty()) {
        if(quote && text.front() == quote) {
            text.remove_prefix(1);
            return result;
        }

        if(quote == '\"' && text.front() == '\\') {
            if(text.size() < 3)
                return {};
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
                result += text.front();
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
inline auto get_string(std::string_view& text) {
    auto result = scan::text(text);
    if(!text.empty())
        throw std::invalid_argument("Incomplete string");
    return result;
}

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

inline auto get_seconds(std::string_view& text) -> time_t {
    if(text.empty() || !isdigit(text.front()))
        throw std::invalid_argument("Duration missing or invalid");

    auto value = time_t(scan::value(text));
    if(!text.empty() && isdigit(text.front()))
        throw std::overflow_error("Duration too big");

    if(text.empty())
        return value;

    if(text.size() == 1) {
        auto ch = tolower(text.front());
        if(ch == 's')
            return value;
        if(ch == 'm')
            return value * 60;
        if(ch == 'h')
            return value * 3600;
        if(ch == 'd')
            return value * 86400;
        if(ch == 'w')
            return value * 604800;
    }
    throw std::invalid_argument("Duration is invalid");
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
