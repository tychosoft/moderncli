// Copyright (C) 2022 Tycho Softworks.
// This code is licensed under MIT license.

#ifndef PRINT_HPP_
#define PRINT_HPP_

#include <iostream>
#include <string_view>

#ifndef _MSC_VER
#include <unistd.h>
#endif

#if !defined(PREFER_LIBFMT) && __has_include(<format>) && __cplusplus >= 202002L
#include <format>
#else
#undef  __cpp_lib_format
#endif

#ifdef  __cpp_lib_format
#define print_format std::format
#else
#include <fmt/format.h>
template<class... Args>
auto print_format(std::string_view fmt, const Args&... args) {
    return fmt::format(fmt, args...);
}
#endif

namespace tycho {
template<class... Args>
void print(std::string_view fmt, const Args&... args) {
    auto str = print_format(fmt, args...);
#if defined(_MSC_VER) || defined(__MINGW32__) || defined(__MINGW64__) || defined(WIN32)
    fputs(str.data(), stdout);
    fflush(stdout);
#else
    ::write(1, str.data(), str.size());
#endif
}

template<class... Args>
void println(std::string_view fmt = "", const Args&... args) {
    puts(print_format(fmt, args...).data());
}

template<class... Args>
void print(std::FILE *stream, std::string_view fmt, const Args&... args) {
    fputs(print_format(fmt, args...).c_str(), stream);
}

template<class... Args>
void println(const std::FILE *stream, std::string_view fmt = "", const Args&... args) {
    std::printf(stream, "%s\n", print_format(fmt, args...).c_str()); // FlawFinder: ignore
}

template<class... Args>
void die(int code, std::string_view fmt, const Args&... args) {
    std::cerr << print_format(fmt, args...);
    ::exit(code);
}

template<class... Args>
void print(std::ostream& out, std::string_view fmt, const Args&... args) {
    out << print_format(fmt, args...);
}

template<class... Args>
void println(std::ostream& out, std::string_view fmt = "", const Args&... args) {
    out << print_format(fmt, args...) << std::endl;
}

template<class Out, class... Args>
void print_to(Out& out, std::string_view fmt, const Args&... args) {
    out << print_format(fmt, args...);
}

template<class Out, class... Args>
[[deprecated]] void println_to(Out& out, std::string_view fmt = "", const Args&... args) {
    out << print_format(fmt, args...) << std::endl;
}
} // end namespace
#endif
