// Copyright (C) 2024 Tycho Softworks.
// This code is licensed under MIT license.

#ifndef TYCHO_OUTPUT_HPP_
#define TYCHO_OUTPUT_HPP_

#include "encoding.hpp"

#include <sstream>
#include <iostream>
#include <cstdlib>
#include <mutex>

#if __has_include(<syslog.h>)
#define USE_SYSLOG
#include <syslog.h>
#else
#ifndef LOG_SYSLOG
#define LOG_SYSLOG
constexpr auto LOG_AUTH = 0;
constexpr auto LOG_AUTHPRIV = 0;
constexpr auto LOG_DAEMON = 0;
constexpr auto LOG_EMERG = 0;
constexpr auto LOG_CRIT = 0;
constexpr auto LOG_INFO = 0;
constexpr auto LOG_WARNING = 0;
constexpr auto LOG_NOTICE = 0;
constexpr auto LOG_ERR = 0;
constexpr auto LOG_NDELAY = 0;
constexpr auto LOG_NOWAIT = 0;
constexpr auto LOG_PERROR = 0;
constexpr auto LOG_PID = 0;
#endif
#endif

#if defined(_MSC_VER) || defined(__MINGW32__) || defined(__MINGW64__) || defined(WIN32)
#ifndef quick_exit
#define quick_exit(x) ::exit(x)
#define at_quick_exit(x) ::atexit(x)
#endif
#endif

namespace tycho {
class die final : public std::ostringstream {
public:
    die(const die&) = delete;
    auto operator=(const die&) -> auto& = delete;

    explicit die(int code) noexcept : exit_code_(code) {}
    [[noreturn]] ~die() final {
        std::cerr << str() << std::endl;
        std::cerr.flush();
        ::exit(exit_code_);
    }

private:
    int exit_code_{-1};
};

class crit final : public std::ostringstream {
public:
    crit(const crit&) = delete;
    auto operator=(const crit&) -> auto& = delete;

    explicit crit(int code) noexcept : exit_code_(code) {}
    [[noreturn]] ~crit() final {
        std::cerr << str() << std::endl;
        std::cerr.flush();
        quick_exit(exit_code_);
    }

private:
    int exit_code_{-1};
};

class output final : public std::ostringstream {
public:
    class debug final : public std::ostringstream {
    public:
        debug(const debug&) = delete;
        auto operator=(const debug&) -> auto& = delete;

        debug() = default;
        ~debug() final {
#ifndef NDEBUG
            std::cout << str() << std::endl;
            std::cout.flush();
#endif
        }
    };

    class error final : public std::ostringstream {
    public:
        error(const error&) = delete;
        auto operator=(const error&) -> auto& = delete;

        error() = default;
        ~error() final {
            std::cerr << str() << std::endl;
            std::cerr.flush();
        }
    };

    output() = default;
    output(const output&) = delete;
    explicit output(unsigned nl) : nl_(nl) {}
    auto operator=(const output&) -> auto& = delete;

    ~output() final {
        std::cout << str();
        if (nl_)
            std::cout << std::string(nl_, '\n');
        std::cout.flush();
    }

private:
    unsigned nl_{1};
};

class logger_stream final {
public:
    using notify_t = void (*)(const std::string&, const char *type);

    class logger final : public std::ostringstream {
    public:
        logger(const logger&) = delete;

        ~logger() final {
            const std::lock_guard lock(from_.locker_);
#ifdef USE_SYSLOG
            if (from_.opened_)
                ::syslog(type_, "%s", str().c_str());
#endif
            from_.notify_(str(), prefix_);
            if (from_.verbose_ >= level_)
                std::cerr << prefix_ << ": " << str() << std::endl;
            if (exit_)
                quick_exit(exit_);
        }

    private:
        friend class logger_stream;

        logger(unsigned level, int type, const char *prefix, logger_stream& from, int ex = 0) : from_(from), level_{level}, type_{type}, prefix_{prefix}, exit_{ex} {}

        logger_stream& from_;
        unsigned level_;
        int type_;
        const char *prefix_;
        int exit_{0};
    };

    void set(unsigned verbose, notify_t notify = [](const std::string& str, const char *type) {}) {
        verbose_ = verbose;
        notify_ = notify;
    }

#ifdef USE_SYSLOG
    void open(const char *id, int level = LOG_INFO, int facility = LOG_DAEMON, int flags = LOG_CONS | LOG_NDELAY) {
        ::openlog(id, flags, facility);
        ::setlogmask(LOG_UPTO(level));
        opened_ = true;
    }

    void close() {
        ::closelog();
        opened_ = false;
    }
#else
    void open(const char *id, int level = LOG_INFO, int facility = 0, int flags = 0) {}
    void close() {}
#endif

    auto fatal(int ex = -1) {
        return logger(1, LOG_CRIT, "fatal", *this, ex);
    }

    auto error() {
        return logger(1, LOG_ERR, "error", *this);
    }

    auto warning() {
        return logger(2, LOG_WARNING, "warn", *this);
    }

    auto notice() {
        return logger(3, LOG_NOTICE, "note", *this);
    }

    auto info() {
        return logger(4, LOG_INFO, "info", *this);
    }

private:
    notify_t notify_{[](const std::string& str, const char *type) {}};
    unsigned verbose_{1};
    std::mutex locker_;
#ifdef USE_SYSLOG
    bool opened_{false};
#endif
};
} // namespace tycho
#endif
