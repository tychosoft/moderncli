// Copyright (C) 2023 Tycho Softworks.
// This code is licensed under MIT license.

#ifndef TYCHO_EXPECTED_HPP_
#define TYCHO_EXPECTED_HPP_

#include <variant>

namespace tycho {
template <typename T, typename E>
class expected {
public:
    expected() = default;
    expected(const expected&) = default;
    explicit expected(const T& value) : value_(value) {}
    explicit expected(const E& error) : value_(error) {}

    auto operator=(const expected&) -> expected& = default;

    auto operator*() const -> const T& {
        return value();
    }

    auto operator*() -> T& {
        return value();
    }

    auto operator->() -> T* {
        return &(value());
    }

    auto operator->() const -> const T* {
        return &(value());
    }

    operator bool() const {
        return has_value();
    }

    auto operator!() const {
        return !has_value();
    }

    auto has_value() const {
        return std::holds_alternative<T>(value_);
    }

    auto value() -> T& {
        return std::get<T>(value_);
    }

    auto value() const -> const T& {
        return std::get<T>(value_);
    }

    auto value_or(T& alt) -> T& {
        if(has_value())
            return value();
        return alt;
    }

    auto error() const -> const E& {
        return std::get<E>(value_);
    }

private:
    std::variant<T, E> value_;
};
} // end namespace
#endif
