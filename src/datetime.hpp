// Copyright (C) 2020 Tycho Softworks.
// This code is licensed under MIT license.

#ifndef TYCHO_DATETIME_HPP_
#define TYCHO_DATETIME_HPP_

#include <chrono>
#include <string>
#include <sstream>
#include <iomanip>
#include <ctime>

namespace tycho {
inline constexpr auto GENERIC_DATETIME = "%c";
inline constexpr auto LOCAL_DATETIME = "%x %X";
inline constexpr auto ZULU_TIMESTAMP = "%Y-%m-%dT%H:%M:%SZ";
inline constexpr auto ISO_TIMESTAMP = "%Y-%m-%d %H:%M:%S %z";
inline constexpr auto ISO_DATETIME = "%Y-%m-%d %H:%M:%S";
inline constexpr auto ISO_DATE = "%Y-%m-%d";
inline constexpr auto ISO_TIME = "%X";

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
} // end namespace

inline auto operator<<(std::ostream& out, const std::tm& current) -> std::ostream& {
    out << std::put_time(&current, tycho::ISO_DATETIME);
    return out;
}
#endif
