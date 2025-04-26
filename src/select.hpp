// Copyright (C) 2025 Tycho Softworks.
// This code is licensed under MIT license.

#ifndef TYCHO_SELECT_HPP_
#define TYCHO_SELECT_HPP_

#include <variant>
#include <string>
#include <functional>
#include <unordered_map>
#include <utility>

namespace tycho {
template <typename... Ts>
class select_when {
public:
    using variant_type = std::variant<Ts...>;
    using action_type = void(*)();

    select_when(const select_when&) = delete;
    auto operator=(const select_when&) -> auto& = delete;

    select_when(std::initializer_list<std::pair<variant_type,action_type>> list) {
        for(const auto& item : list) {
            cases_[item.first] = item.second;
        }
    }

    auto operator()(const variant_type& value) const {
        auto it = cases_.find(value);
        if(it != cases_.end()) {
            it->second();
            return true;
        }
        return false;
    }

    template<typename T, typename Func>
    auto operator()(const T& key, Func&& cmp) const {
        for(const auto& item : cases_) {
            if(std::holds_alternative<T>(item.first)) {
                if(cmp(key, std::get<T>(item.first))) {
                    item.second();
                    return true;
                }
            }
        }
        return false;
    }

private:
    std::unordered_map<variant_type,action_type> cases_;
};

template <typename T, typename... Ts>
class select_type {
public:
    using variant_type = std::variant<Ts...>;
    select_type(const select_type&) = delete;
    auto operator=(const select_type&) -> auto& = delete;

    select_type(std::initializer_list<std::pair<variant_type,T>> list) {
        for(const auto& item : list) {
            cases_[item.first] = item.second;
        }
    }

    auto operator()(const variant_type& value, const T& or_value = T{}) const {
        auto it = cases_.find(value);
        if(it != cases_.end()) return it->second;
        return or_value;
    }

private:
    std::unordered_map<variant_type,T> cases_;
};
} // end namespace
#endif
