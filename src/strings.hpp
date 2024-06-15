// Copyright (C) 2020 Tycho Softworks.
// This code is licensed under MIT license.

#ifndef STRINGS_HPP_
#define STRINGS_HPP_

#include <type_traits>
#include <string>
#include <string_view>
#include <cctype>
#include <cstring>
#include <cstdlib>
#include <cstdarg>
#include <cstdint>
#include <vector>
#include <algorithm>
#include <set>
#include <sstream>

namespace tycho {
template <typename T>
inline constexpr bool is_string_type_v = std::is_convertible_v<T, std::string_view>;

template<typename S = std::string>
constexpr auto begins_with(const S& s, const std::string_view &b) {
    static_assert(is_string_type_v<S>, "S must be a string type");

    return s.find(b) == 0;
}

template<typename S = std::string>
constexpr auto ends_with(const S& s, const std::string_view &e) {
    static_assert(is_string_type_v<S>, "S must be a string type");

    if(s.size() < e.size())
        return false;
    auto pos = s.rfind(e);
    if(pos == S::npos)
        return false;
    return s.substr(pos) == e;
}

template<typename S = std::string>
constexpr auto upper_case(const S& s) {
    static_assert(is_string_type_v<S>, "S must be a string type");

    S out = s;
    std::transform(out.begin(), out.end(), out.begin(), ::toupper);
    return out;
}

template<typename S = std::string>
constexpr auto lower_case(const S& s) {
    static_assert(is_string_type_v<S>, "S must be a string type");

    S out = s;
    std::transform(out.begin(), out.end(), out.begin(), ::tolower);
    return out;
}

template<typename S = std::string>
constexpr auto trim(const S& str) -> S {
    static_assert(is_string_type_v<S>, "S must be a string type");

    auto last = str.find_last_not_of(" \t\f\v\n\r");
    if(last == S::npos)
        return "";
    return str.substr(0, ++last);
}

template<typename S = std::string>
constexpr auto strip(const S& str) -> S {
    static_assert(is_string_type_v<S>, "S must be a string type");

    const size_t first = str.find_first_not_of(" \t\f\v\n\r");
    const size_t last = str.find_last_not_of(" \t\f\v\n\r");
    if(last == S::npos)
        return "";
    return str.substr(first, (last-first+1));
}

template<typename S = std::string>
constexpr auto unquote(const S& str, std::string_view pairs = R"(""''{})") -> S {
    static_assert(is_string_type_v<S>, "S must be a string type");

    if(str.empty())
        return "";
    auto pos = pairs.find_first_of(str[0]);
    if(pos == S::npos || (pos & 0x01))
        return str;
    auto len = str.length();
    if(--len < 1)
        return str;
    if(str[len] == pairs[++pos])
        return str.substr(1, --len);
    return str;
}

template<typename S = std::string>
constexpr auto join(const std::vector<S>& list, const std::string_view& delim = ",") -> S {
    static_assert(is_string_type_v<S>, "S must be a string type");

    S separator, result;
    for(const auto& str : list) {
        result = result + separator + str;
        separator = delim;
    }
    return result;
}

template<typename S = std::string>
inline auto split(const S& str, std::string_view delim = " ", unsigned max = 0)
{
    static_assert(is_string_type_v<S>, "S must be a string type");

    std::vector<S> result;
    std::size_t current{}, prev{};
    unsigned count = 0;
    current = str.find_first_of(delim);
    while((!max || ++count < max) && current != S::npos) {
        result.emplace_back(str.substr(prev, current - prev));
        prev = current + 1;
        current = str.find_first_of(delim, prev);
    }
    result.emplace_back(str.substr(prev));
    return result;
}

template<typename T>
inline auto join(const std::set<T>& list, const std::string_view& delim = ",") {
    std::string sep;
    std::stringstream buf;
    for(auto const& value : list) {
        buf << sep << value;
        sep = delim;
    }
    return buf.str();
}

template<typename S=std::string>
inline auto tokenize(const S& str, std::string_view delim = " ", std::string_view quotes = R"(""''{})") {
    static_assert(is_string_type_v<S>, "S must be a string type");

    std::vector<S> result;
    auto current = str.find_first_of(delim);
    auto prev = str.find_first_not_of(delim);
    if(prev == S::npos)
        prev = 0;

    while(current != S::npos) {
        auto lead = quotes.find_first_of(str[prev]);
        if(lead != S::npos) {
            auto tail = str.find_first_of(quotes[lead + 1], prev + 1);
            if(tail != S::npos) {
                current = tail;
                result.emplace_back(str.substr(prev, current - prev + 1));
                goto finish; // NOLINT
            }
        }
        result.emplace_back(str.substr(prev, current - prev));
finish:
        prev = str.find_first_not_of(delim, ++current);
        if(prev == S::npos)
            prev = current;
        current = str.find_first_of(delim, prev);
    }
    if(prev < str.length())
        result.emplace_back(str.substr(prev));
    return result;
}

template<typename S = std::string_view>
inline auto is_line(const S& str) {
    static_assert(is_string_type_v<S>, "S must be a string type");

    if(str.size() < 1)
        return false;

    if(str[str.size() - 1] == '\n')
        return true;

    return false;
}

// convenience function for string conversions if not explicit for template

inline auto upper_case(const char *s) {
    return upper_case(std::string(s));
}

inline auto lower_case(const char *s) {
    return lower_case(std::string(s));
}

inline auto strip(const char *s) {
    return strip(std::string(s));
}

inline auto unquote(const char *s, const char *p = R"(""''{})") {
    return unquote(std::string(s), std::string(p));
}

constexpr auto begins_with(const char *s, const char *b) {
    return begins_with<std::string_view>(s, b);
}

constexpr auto ends_with(const char *s, const char *e) {
    return ends_with<std::string_view>(s, e);
}

inline auto eq(const char *p1, const char *p2) {
    if(!p1 && !p2)
        return true;

    if(!p1 || !p2)
        return false;

    return strcmp(p1, p2) == 0;
}

inline auto eq(const char *p1, const char *p2, size_t len) {
    if(!p1 && !p2)
        return true;

    if(!p1 || !p2)
        return false;

    return strncmp(p1, p2, len) == 0;
}

constexpr auto str_size(const char *cp, size_t max = 256) -> size_t {
    size_t count = 0;
    while(cp && *cp && count < max) {
        ++count;
        ++cp;
    }
    return count;
}

inline auto str_copy(char *cp, size_t max, std::string_view view) {
    auto count = view.size();
    auto dp = view.data();
    if(count >= max)
        count = max - 1;

    max = count;
    while(max--)
        *(cp++) = *(dp++);

    *cp = 0;
    return count;
}

inline auto str_append(char *cp, size_t max, ...) {  // NOLINT
    va_list list;
    auto pos = str_size(cp, max);
    va_start(list, max);
    for(;;) {
        auto sp = va_arg(list, const char *); // NOLINT
        if(!sp)
            break;

        if(pos >= max)
            continue;

        auto size = str_size(sp);
        if(size + pos >= max) {
            pos = max;
            continue;
        }

        str_copy(cp + pos, max - pos, sp);
        pos += size;
    }
    va_end(list);
    return pos < max;
}

inline auto to_hex(const uint8_t *from, size_t size) {
    std::string out;
    out.resize(size * 2);
    auto hex = out.data();

    for(auto pos = size_t(0); pos < size; ++pos) {
        snprintf(hex, 3, "%02x", from[pos]);
        hex += 2;
    }
    return out;
}

inline auto to_hex(const std::string_view str) {
    return to_hex(reinterpret_cast<const uint8_t *>(str.data()), str.size());
}

inline auto from_hex(std::string_view from, uint8_t *to, size_t size) {
    auto hex = from.data();
    auto max = size * 2;
    auto buf = 0U;
    if (from.size() <= max)
        max = from.size();

    for(auto pos = size_t(0); pos < max; pos += 2) {
        if (sscanf(hex + pos, "%2x", &buf) != 1)    // NOLINT
            return pos / 2;
        *(to++) = static_cast<uint8_t>(buf);
    }
    return max / 2;
}

inline void clobber(std::string& str, char fill = '*') {
    for(auto pos = size_t(0); pos < str.size(); ++pos) {
        str[pos] = fill;
    }
}

constexpr auto u8verify(const std::string_view& u8) noexcept {
    auto str = u8.data();
    auto len = u8.size();
    while (len && *str) {
        if ((*str & 0b10000000) != 0) {
            if ((*str & 0b01000000) == 0)
                return false;
            if ((*str & 0b00100000) != 0) {
                if ((*str & 0b00010000) != 0) {
                    if ((*str & 0b00001000) != 0)
                        return false;
                    if (!--len || ((*++str & 0b11000000) != 0b10000000))
                        return false;
                }
                if (!--len || ((*++str & 0b11000000) != 0b10000000))
                    return false;
            }
            if (!--len || ((*++str & 0b11000000) != 0b10000000))
                return false;
        }
        --len;
        ++str;
    }
    return true;
}
} // end namespace
#endif
