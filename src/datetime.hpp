// Copyright (C) 2020 Tycho Softworks.
// This code is licensed under MIT license.

#ifndef TYCHO_DATETIME_HPP_
#define TYCHO_DATETIME_HPP_

#include <chrono>
#include <string>
#include <string_view>
#include <sstream>
#include <iomanip>
#include <ctime>
#include <cctype>

// low level utility scan functions
namespace tycho::scan {
inline auto year(std::string_view& text) {
    unsigned value{0};
    if(text.size() < 4)
        return ~0U;

    for(auto pos = 0; pos < 4; ++pos) {
        if(!isdigit(text[pos]))
            return ~0U;
        value *= 10;
        value += text[pos] - '0';
    }
    text.remove_prefix(4);
    return value;
}

inline auto month(std::string_view& text) {
    if(text.size() < 2 || !isdigit(text[0]) || !isdigit(text[1]))
        return ~0U;
    const unsigned value = ((text[0] - '0') * 10) + (text[1] - '0');
    if(value < 1 || value > 12)
        return ~0U;
    text.remove_prefix(2);
    return value;
}

inline auto day(std::string_view& text) {
    if(text.size() < 2 || !isdigit(text[0]) || !isdigit(text[1]))
        return ~0U;
    const unsigned value = ((text[0] - '0') * 10) + (text[1] - '0');
    if(value < 1 || value > 31)
        return ~0U;
    text.remove_prefix(2);
    return value;
}

inline auto hours(std::string_view& text) {
    if(text.size() >= 2 && text[0] >= '0' && text[0] <= '3' && isdigit(text[1])) {
        const unsigned value = ((text[0] - '0') * 10) + (text[1] - '0');
        text.remove_prefix(2);
        return value;
    }
    if(!text.empty() && isdigit(text[0])) {
        const unsigned value = text[0] - '0';
        text.remove_prefix(1);
        return value;
    }
    return ~0U;
}

inline auto minsec(std::string_view& text) {
    if(text.size() >= 2 && text[0] >= '0' && text[0] < '6' && isdigit(text[1])) {
        const unsigned value = ((text[0] - '0') * 10) + (text[1] - '0');
        text.remove_prefix(2);
        return value;
    }

    if(!text.empty() && isdigit(text[0])) {
        const unsigned value = text[0] - '0';
        text.remove_prefix(1);
        return value;
    }

    return ~0U;
}
} // end namespace

namespace tycho {
inline constexpr auto GENERIC_DATETIME = "%c";
inline constexpr auto LOCAL_DATETIME = "%x %X";
inline constexpr auto ZULU_TIMESTAMP = "%Y-%m-%dT%H:%M:%SZ";
inline constexpr auto ISO_TIMESTAMP = "%Y-%m-%d %H:%M:%S %z";
inline constexpr auto ISO_DATETIME = "%Y-%m-%d %H:%M:%S";
inline constexpr auto ISO_DATE = "%Y-%m-%d";
inline constexpr auto ISO_TIME = "%X";
inline constexpr auto MAX_TIME = time_t(24 * 3600);

inline auto steady_time() {
    return std::chrono::steady_clock::now();
}

inline auto local_time(const std::time_t& time) {
    std::tm local{};
#if defined(_MSC_VER) || defined(__MINGW32__) || defined(__MINGW64__) || defined(WIN32)
    localtime_s(&local, &time);
#else
    localtime_r(&time, &local);
#endif
    return local;
}

inline auto gmt_time(const std::time_t& time) {
    std::tm local{};
#if defined(_MSC_VER) || defined(__MINGW32__) || defined(__MINGW64__) || defined(WIN32)
    gmtime_s(&local, &time);
#else
    gmtime_r(&time, &local);
#endif
    return local;
}

inline auto to_string(const std::tm& current, const char *fmt = ISO_DATETIME) {
    std::stringstream text;
    text <<  std::put_time(&current, fmt);
    return text.str();
}

inline auto gmt_string(const std::time_t& gmt) {
    std::stringstream text;
    auto current = gmt_time(gmt);
    text <<  std::put_time(&current, ZULU_TIMESTAMP);
    return text.str();
}

inline auto iso_string(const std::tm& current) {
    std::stringstream text;
    text <<  std::put_time(&current, ISO_DATETIME);
    return text.str();
}

inline auto iso_string(const std::time_t& current) {
    return iso_string(local_time(current));
}

inline auto iso_date(const std::tm& current) {
    return iso_string(current).substr(0, 10);
}

inline auto iso_date(const std::time_t& current) {
    return iso_string(current).substr(0, 10);
}

inline auto iso_time(const std::tm& current) {
    return iso_string(current).substr(11, 8);
}

inline auto iso_time(const std::time_t& current) {
    return iso_string(current).substr(11, 8);
}

inline auto iso_date(std::string_view text, std::time_t or_else = std::time_t(0), std::time_t current_time = std::time_t(0), std::time_t timezone = std::time_t(0)) {
    unsigned year{0}, month{0}, day{0};
    year = scan::year(text);
    if(text.empty() || text.front() != '-' || year == ~0U)
        return or_else;

    text.remove_prefix(1);
    month = scan::month(text);
    if(text.empty() || text.front() != '-' || month == ~0U)
        return or_else;

    text.remove_prefix(1);
    day = scan::day(text);
    if(!text.empty() || day == ~0U)
        return or_else;

    struct tm date = {0};
    date.tm_year = int(year - 1900U);
    date.tm_mon = int(month - 1U);
    date.tm_mday = int(day);
    auto gmt = mktime(&date);
    if(gmt == -1)
        return or_else;
    return gmt + current_time + timezone;
}

inline auto iso_time(std::string_view text, std::time_t or_else = MAX_TIME) -> std::time_t {
    unsigned hour{0}, min{0}, sec{0};

    hour = scan::hours(text);
    if(text.empty() || text.front() != ':' || hour > 23)
        return or_else;
    text.remove_prefix(1);

    min = scan::minsec(text);
    if(text.empty() && min > 59)
        return or_else;

    if(!text.empty() && text.front() == ':') {
        text.remove_prefix(1);
        sec = scan::minsec(text);
    }

    if(!text.empty() || min > 59 || sec > 59)
        return or_else;
    return (static_cast<time_t>(hour) * 3600U) + (static_cast<long long>(min) * 60U) + sec;
}
} // end namespace

inline auto operator<<(std::ostream& out, const std::tm& current) -> std::ostream& {
    out << std::put_time(&current, tycho::ISO_DATETIME);
    return out;
}
#endif
