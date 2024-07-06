// Copyright (C) 2024 Tycho Softworks.
// This code is licensed under MIT license.

#ifndef TYCHO_OUTPUT_HPP_
#define TYCHO_OUTPUT_HPP_

#include <sstream>
#include <iostream>
#include <string_view>
#include <cstdlib>

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

    explicit die(int code) noexcept : excode_(code) {}
    [[noreturn]] ~die() final {
        std::cerr << str() << std::endl;
        std::cerr.flush();
        ::exit(excode_);
    }

private:
    int excode_{-1};
};

class crit final : public std::ostringstream {
public:
    crit(const crit&) = delete;
    auto operator=(const crit&) -> auto& = delete;

    explicit crit(int code) noexcept : excode_(code) {}
    [[noreturn]] ~crit() final {
        std::cerr << str() << std::endl;
        std::cerr.flush();
        quick_exit(excode_);
    }

private:
    int excode_{-1};
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
        if(nl_)
            std::cout << std::string(nl_, '\n');
        std::cout.flush();
    }

private:
    unsigned nl_{1};
};
} // end namespace
#endif
