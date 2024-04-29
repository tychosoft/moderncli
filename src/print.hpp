// Copyright (C) 2022 Tycho Softworks.
// This code is licensed under MIT license.

#ifndef PRINT_HPP_
#define PRINT_HPP_

#include <iostream>
#include <string_view>
#include <mutex>
#include <sstream>
#include <iomanip>
#include <ctime>

#include <fmt/ranges.h>

#ifndef _MSC_VER
#include <unistd.h>
#endif

#if __has_include(<syslog.h>)
#define USE_SYSLOG
#include <syslog.h>
#else
#define LOG_AUTH 0
#define LOG_AUTHPRIV 0
#define LOG_DAEMON 0
#define LOG_EMERG 0
#define LOG_CRIT 0
#define LOG_INFO 0
#define LOG_WARNING 0
#define LOG_NOTICE 0
#define LOG_ERR 0
#define LOG_DAEMON 0
#define LOG_CONS 0
#define LOG_NDELAY 0
#define LOG_NOWAIT 0
#define LOG_PERROR 0
#define LOG_PID 0
#endif

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

class system_logger final {
public:
    using notify_t = void (*)(const std::string&, const char *type);

    system_logger() = default;
    system_logger(const system_logger&) = delete;
    auto operator=(const system_logger&) -> system_logger& = delete;

    template<class... Args>
    void debug(unsigned level, std::string_view fmt, const Args&... args) {
#ifndef NDEBUG
        if(level <= logging) {
            try {
                if(fmt.back() == '\n')
                    fmt.remove_suffix(1);
                auto msg = format(fmt, args...);
                const std::lock_guard lock(locking);
                print(std::cerr, "debug: {}\n", msg);
                notify_(msg, "debug");
            }
            catch(const std::exception& e) {
                print(std::cerr, "debug: {}\n", e.what());
                return;
            }
        }
#endif
    }

    template<class... Args>
    void info(std::string_view fmt, const Args&... args) {
        if(fmt.back() == '\n')
            fmt.remove_suffix(1);
        auto msg = format(fmt, args...);
        const std::lock_guard lock(locking);
#ifdef  USE_SYSLOG
        ::syslog(LOG_INFO, "%s", msg.c_str());
#endif
        notify_(msg, "info");
        if(logging > 1)
            print(std::cerr, "info: {}\n", msg);
    }

    template<class... Args>
    void notice(std::string_view fmt, const Args&... args) {
        if(fmt.back() == '\n')
            fmt.remove_suffix(1);
        auto msg = format(fmt, args...);
        const std::lock_guard lock(locking);
#ifdef  USE_SYSLOG
        ::syslog(LOG_NOTICE, "%s", msg.c_str());
#endif
        notify_(msg, "notice");
        if(logging > 0)
            print(std::cerr, "notice: {}\n", msg);
    }

    template<class... Args>
    void warn(std::string_view fmt, const Args&... args) {
        if(fmt.back() == '\n')
            fmt.remove_suffix(1);
        auto msg = format(fmt, args...);
        const std::lock_guard lock(locking);
#ifdef  USE_SYSLOG
        ::syslog(LOG_WARNING, "%s", msg.c_str());
#endif
        notify_(msg, "warning");
        if(logging)
            print(std::cerr, "warn: {}\n", msg);
    }

    template<class... Args>
    void error(std::string_view fmt, const Args&... args) {
        if(fmt.back() == '\n')
            fmt.remove_suffix(1);
        auto msg = format(fmt, args...);
        const std::lock_guard lock(locking);
#ifdef  USE_SYSLOG
        ::syslog(LOG_ERR, "%s", msg.c_str());
#endif
        notify_(msg, "error");
        if(logging)
            print(std::cerr, "error: {}\n", msg);
    }

    template<class... Args>
    void fail(int excode, std::string_view fmt, const Args&... args) {
        if(fmt.back() == '\n')
            fmt.remove_suffix(1);
        auto msg = format(fmt, args...);
        const std::lock_guard lock(locking);
#ifdef  USE_SYSLOG
        ::syslog(LOG_CRIT, "%s", msg.c_str());
#endif
        notify_(msg, "fatal");
        if(logging)
            print(std::cerr, "fail: {}\n", msg);
        std::cerr << std::ends;
        ::exit(excode);
    }

    void set(unsigned level, notify_t notify = [](const std::string& str, const char *type){}) {
        logging = level;
        notify_ = notify;
    }

#ifdef  USE_SYSLOG
    static void open(const char *id, int level = LOG_INFO, int facility = LOG_DAEMON, int flags = LOG_CONS | LOG_NDELAY) { // FlawFinder: ignore
        ::openlog(id, flags, facility);
        ::setlogmask(LOG_UPTO(level));
    }

    static void close() {
        ::closelog();
    }
#else
    // FlawFinder: ignore
    static void open(const char *id, int level, int facility, int flags) {}
    static void close() {}
#endif

private:
    std::mutex locking;
    unsigned logging{1};
    notify_t notify_{[](const std::string& str, const char *type){}};
};

inline auto iso_string(const std::tm& current) {
    std::stringstream text;
    text <<  std::put_time(&current, "%Y-%m-%d %X");
    return text.str();
}

inline auto iso_string(const std::time_t& current) {
    std::tm local{};
#if defined(_MSC_VER) || defined(__MINGW32__) || defined(__MINGW64__) || defined(WIN32)
    localtime_s(&local, &current);
#else
    localtime_r(&current, &local);
#endif
    return iso_string(local);
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

template <> class fmt::formatter<std::tm> {
public:
    static constexpr auto parse(format_parse_context& ctx) {
        return ctx.begin();
    }

    template <typename Context>
    constexpr auto format(const std::tm& current, Context& ctx) const {
        return format_to(ctx.out(), "{}", tycho::iso_string(current));
    }
};
#endif
