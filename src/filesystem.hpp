// Copyright (C) 2022 Tycho Softworks.
// This code is licensed under MIT license.

#ifndef FILESYSTEM_HPP_
#define FILESYSTEM_HPP_

#include <filesystem>
#include <functional>
#include <algorithm>
#include <iostream>
#include <fstream>
#include <string>

#include <sys/types.h>
#ifndef _MSC_VER
#include <unistd.h>
#else
#include <BaseTsd.h>
typedef SSIZE_T ssize_t;
#endif
#include <fcntl.h>

#if defined(_MSC_VER) || defined(__MINGW32__) || defined(__MINGW64__) || defined(WIN32)
extern "C" {
#include <stdio.h>
#include <stdlib.h>
#include <io.h>
}
#endif

#if defined(__OpenBSD__)
#define stat64 stat     // NOLINT
#define fstat64 fstat   // NOLINT
#endif

namespace fsys {
using namespace std::filesystem;
} // end fsys namespace

namespace tycho {
inline auto scan_stream(std::istream& input, std::function<bool(const std::string&)> proc) {
    std::string line;
    size_t count{0};
    while(std::getline(input, line)) {
        if (!proc(line))
            break;
        ++count;
    }
    return count;
}

inline auto scan_file(const fsys::path& path, std::function<bool(const std::string&)> proc) {
    std::fstream input(path);
    std::string line;
    size_t count{0};
    while(std::getline(input, line)) {
        if (!proc(line))
            break;
        ++count;
    }
    return count;
}

inline auto scan_directory(const fsys::path& path, std::function<bool(const fsys::directory_entry&)> proc) {
    auto dir = fsys::directory_iterator(path);
    return std::count_if(begin(dir), end(dir), proc);
}

inline auto scan_recursive(const fsys::path& path, std::function<bool(const fsys::directory_entry&)> proc) {
    auto dir = fsys::recursive_directory_iterator(path, fsys::directory_options::skip_permission_denied);
    return std::count_if(begin(dir), end(dir), proc);
}
} // end namespace
#endif
