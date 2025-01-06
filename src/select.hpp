// Copyright (C) 2025 Tycho Softworks.
// This code is licensed under MIT license.

#ifndef TYCHO_SELECT_HPP_
#define TYCHO_SELECT_HPP_

#include <variant>
#include <string>
#include <functional>
#include <unordered_map>
#include <utility>
#include <stdexcept>

namespace tycho {
template <typename... Ts>
class select_when {
public:
    using variant_type = std::variant<Ts...>;
    select_when(const select_when&) = delete;
    auto operator=(const select_when&) -> auto& = delete;

    select_when(std::initializer_list<std::pair<variant_type, std::function<void()>>> list) {
        for (const auto& item : list) {
            cases_[item.first] = item.second;
        }
    }

    template <typename F>
    auto when(const variant_type& value, F&& func) -> auto& {
        cases_[value] = std::forward<F>(func);
        return *this;
    }

    auto operator()(const variant_type& value) const {
        auto it = cases_.find(value);
        if (it != cases_.end()) {
            it->second();
            return true;
        }
        return false;
    }

private:
    std::unordered_map<variant_type, std::function<void()>> cases_;
};

template <typename Enum, typename... Ts>
class select_enum {
public:
    using variant_type = std::variant<Ts...>;
    select_enum(const select_enum&) = delete;
    auto operator=(const select_enum&) -> auto& = delete;

    select_enum(std::initializer_list<std::pair<variant_type, Enum>> list) {
        for (const auto& item : list) {
            cases_[item.first] = item.second;
        }
    }

    auto operator()(const variant_type& value) const {
        auto it = cases_.find(value);
        if (it != cases_.end())
            return it->second;
        throw std::out_of_range("Selection not mapped to an enum");
    }

private:
    static_assert(std::is_enum_v<Enum>, "Enum must be an enumerated type");
    std::unordered_map<variant_type, Enum> cases_;
};
} // end namespace
#endif
