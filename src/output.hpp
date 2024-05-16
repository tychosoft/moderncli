// Copyright (C) 2024 Tycho Softworks.
// This code is licensed under MIT license.

#ifndef OUTPUT_HPP_
#define OUTPUT_HPP_

#include <sstream>
#include <iostream>
#include <string_view>

namespace tycho {
class die final : public std::ostringstream {
public:
    die(const die&) = delete;
    auto operator=(const die&) -> die& = delete;

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
    auto operator=(const crit&) -> crit& = delete;

    explicit crit(int code) noexcept : excode_(code) {}
    [[noreturn]] ~crit() final {
        std::cerr << str() << std::endl;
        std::cerr.flush();
        std::quick_exit(excode_);
    }

private:
    int excode_{-1};
};

class output final : public std::ostringstream {
public:
    class debug final : public std::ostringstream {
    public:
        debug(const debug&) = delete;
        auto operator=(const debug&) -> debug& = delete;

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
        auto operator=(const error&) -> error& = delete;

        error() = default;
        ~error() final {
            std::cerr << str() << std::endl;
            std::cerr.flush();
        }
    };

    output() = default;
    output(const output&) = delete;
    explicit output(unsigned nl) : nl_(nl) {}
    auto operator=(const output&) -> output& = delete;

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
