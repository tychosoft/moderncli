// Copyright (C) 2022 Tycho Softworks.
// This code is licensed under MIT license.

#ifndef PRINT_HPP_
#define PRINT_HPP_

#include <iostream>
#include <string_view>

#ifndef _MSC_VER
#include <unistd.h>
#endif

#include <fmt/ranges.h>

namespace tycho {
using namespace fmt;

template<class... Args>
[[deprecated]] auto print_format(std::string_view fmt, const Args&... args) {
    return fmt::format(fmt, args...);
}

template<class... Args>
void die(int code, std::string_view fmt, const Args&... args) {
    std::cerr << fmt::format(fmt, args...);
    ::exit(code);
}

template<class... Args>
void print(std::ostream& out, std::string_view fmt, const Args&... args) {
    out << fmt::format(fmt, args...);
}

template<class... Args>
void println(std::ostream& out, std::string_view fmt, const Args&... args) {
    out << fmt::format(fmt, args...) << std::endl;
}
} // end namespace
#endif
