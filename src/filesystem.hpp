// Copyright (C) 2022 Tycho Softworks.
// This code is licensed under MIT license.

#ifndef TYCHO_FILESYSTEM_HPP_
#define TYCHO_FILESYSTEM_HPP_

#include <filesystem>
#include <functional>
#include <algorithm>
#include <iostream>
#include <fstream>
#include <string>
#include <cstdio>
#include <cstdlib>

#include <sys/types.h>
#include <fcntl.h>

#ifndef _MSC_VER
#include <unistd.h>
#else
#include <BaseTsd.h>
typedef SSIZE_T ssize_t;
#endif

#if defined(_MSC_VER) || defined(__MINGW32__) || defined(__MINGW64__) || defined(WIN32)
extern "C" {
#include <io.h>
}
#endif

#if defined(__OpenBSD__)
#define stat64 stat     // NOLINT
#define fstat64 fstat   // NOLINT
#endif

#if defined(_MSC_VER) || defined(__MINGW32__) || defined(__MINGW64__) || defined(WIN32)
auto getline_w32(char **lp, size_t *size, FILE *fp) -> ssize_t {
    size_t pos{0};
    int c{EOF};

    if(lp == nullptr || fp == nullptr || size == nullptr)
        return -1;

    c = getc(fp);   // FlawFinder: ignore
    if(c == EOF)
        return -1;

    if(*lp == nullptr) {
        *lp = static_cast<char *>(malloc(128));
        if(*lp == nullptr) {
            return -1;
        }
        *size = 128;
    }

    pos = 0;
    while(c != EOF) {
        if (pos + 1 >= *size) {
            auto new_size = *size + (*size >> 2);
            if(new_size < 128) {
                new_size = 128;
            }
            auto new_ptr = static_cast<char *>(realloc(*lp, new_size));
            if(new_ptr == nullptr)
                return -1;
            *size = new_size;
            *lp = new_ptr;
        }

        (reinterpret_cast<unsigned char *>(*lp))[pos ++] = c;
        if(c == '\n')
            break;
        c = getc(fp);   // FlawFinder: ignore
    }
    (*lp)[pos] = '\0';
    return static_cast<ssize_t>(pos);
}
#endif

namespace fsys {
using namespace std::filesystem;

#if defined(_MSC_VER) || defined(__MINGW32__) || defined(__MINGW64__) || defined(WIN32)
inline auto native_handle(int fd) {
    auto handle = _get_osfhandle(fd);
    if(handle == -1)
        return static_cast<void *>(nullptr);
    return reinterpret_cast<void *>(handle);
}

inline auto native_handle(std::FILE *fp) {
    return native_handle(_fileno(fp));
}
#else
inline auto native_handle(int fd) {
    return fd;
}

inline auto native_handle(std::FILE *fp) {
    return fileno(fp);
}
#endif
} // end fsys namespace

namespace tycho {
inline auto scan_stream(std::istream& input, std::function<bool(const std::string_view&)> proc) {
    std::string line;
    size_t count{0};
    while(std::getline(input, line)) {
        if (!proc(line))
            break;
        ++count;
    }
    return count;
}

inline auto scan_file(const fsys::path& path, std::function<bool(const std::string_view&)> proc) {
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

#if !defined(_MSC_VER) && !defined(__MINGW32__) && !defined(__MINGW64__) && !defined(WIN32)
inline auto scan_file(std::FILE *file, std::function<bool(const std::string_view&)> proc, size_t size = 0) {
    char *buf{nullptr};
    size_t count{0};
    if(size)
        buf = static_cast<char *>(malloc(size));    // NOLINT
    while(!feof(file)) {
        auto len = getline(&buf, &size, file);
        if(len < 0 || !proc({buf, static_cast<size_t>(len)}))
            break;
        ++count;
    }
    if(buf)
        free(buf);  // NOLINT
    return count;
}

inline auto scan_command(const std::string& cmd, std::function<bool(const std::string_view&)> proc, size_t size = 0) {
    auto file = popen(cmd.c_str(), "r");    // FlawFinder: ignore

    if(!file)
        return size_t(0);

    auto count = scan_file(file, proc, size);
    pclose(file);
    return count;
}
#else
inline auto scan_file(std::FILE *file, std::function<bool(const std::string_view&)> proc, size_t size = 0) {
    char *buf{nullptr};
    size_t count{0};
    if(size)
        buf = static_cast<char *>(malloc(size));
    while(!feof(file)) {
        auto len = getline_w32(&buf, &size, file);
        if(len < 0 || !proc({buf, static_cast<size_t>(len)}))
            break;
        ++count;
    }
    if(buf)
        free(buf);  // NOLINT
    return count;
}

inline auto scan_command(const std::string& cmd, std::function<bool(const std::string_view&)> proc, size_t size = 0) {
    auto file = _popen(cmd.c_str(), "r");    // FlawFinder: ignore

    if(!file)
        return size_t(0);

    auto count = scan_file(file, proc, size);
    _pclose(file);
    return count;
}
#endif

inline auto make_input(const fsys::path& path, std::ios::openmode mode = std::ios::binary) {
    std::ifstream file;
    file.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    file.open(path, mode);  // FlawFinder: ignore
    return file;
}

inline auto make_output(const fsys::path& path, std::ios::openmode mode = std::ios::binary) {
    std::ofstream file;
    file.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    file.open(path, mode);  // FlawFinder: ignore
    return file;
}

inline auto scan_directory(const fsys::path& path, std::function<bool(const fsys::directory_entry&)> proc) {
    auto dir = fsys::directory_iterator(path);
    return std::count_if(begin(dir), end(dir), proc);
}

inline auto scan_recursive(const fsys::path& path, std::function<bool(const fsys::directory_entry&)> proc) {
    auto dir = fsys::recursive_directory_iterator(path, fsys::directory_options::skip_permission_denied);
    return std::count_if(begin(dir), end(dir), proc);
}

inline auto to_string(const fsys::path& path) {
    return std::string{path.u8string()};
}
} // end namespace

#ifdef  TYCHO_PRINT_HPP_
template <> class fmt::formatter<fsys::path> {
public:
    static constexpr auto parse(format_parse_context& ctx) {
        return ctx.begin();
    }

    template <typename Context>
    constexpr auto format(fsys::path const& path, Context& ctx) const {
        return format_to(ctx.out(), "{}", std::string{path.u8string()});
    }
};
#endif

inline auto operator<<(std::ostream& out, const fsys::path& path) -> std::ostream& {
    out << path.u8string();
    return out;
}
#endif
