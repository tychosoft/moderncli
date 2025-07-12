// Copyright (C) 2025 Tycho Softworks.
// This code is licensed under MIT license.

#ifndef TYCHO_MONADIC_HPP_
#define TYCHO_MONADIC_HPP_

#include <vector>
#include <optional>
#include <tuple>
#include <type_traits>

namespace tycho::monadic {
template<typename T>
class maybe {
public:
    maybe() : value_(std::nullopt) {}
    explicit maybe(const T& val) : value_(val) {}

    template<typename Func>
    auto bind(Func func) -> maybe<decltype(func(std::declval<T>()).value_)> {
        static_assert(std::is_invocable_v<Func, T>, "Func must be invocable with T");
        if(value_) return maybe<decltype(func(value_.value()).value_)>(func(value_.value()).value_);
        return maybe<decltype(func(std::declval<T>()).value_)>();
    }

    operator bool() const {
        return value_.has_value();
    }

    auto operator!() const {
        return !value_.has_value();
    }

    auto operator*() const {
        return value_.value();
    }

    auto has_value() const {
        return value_.has_value();
    }

    auto get_value() const {
        return value_.value();
    }

    auto get_value(const T& or_else) const {
        return value_.has_value() ? value_.value() : or_else;
    }

private:
    std::optional<T> value_;
};

template<typename T>
auto some(T value) {
    return maybe<T>(value);
}

template<typename T>
auto none() {
    return maybe<T>();
}

template<typename T, typename Func>
auto maybe_try(Func func) -> maybe<T> {
    static_assert(std::is_invocable_v<Func>, "Func must be callable");
    static_assert(std::is_convertible_v<std::invoke_result_t<Func>, T>, "Result must be convertible to T");
    try {
        return maybe<T>(func());
    } catch(...) {
        return maybe<T>();
    }
}

template<typename T, typename Func>
auto map(maybe<T> maybe_val, Func func) -> maybe<decltype(func(std::declval<T>()))> {
    static_assert(std::is_invocable_v<Func, T>, "Func must be callable");
    static_assert(std::is_constructible_v<maybe<decltype(func(std::declval<T>()))>, decltype(func(std::declval<T>()))>, "Func(T) must return a maybe<U>");
    if(maybe_val.has_value()) return maybe<decltype(func(std::declval<T>()))>(func(maybe_val.get_value()));
    return maybe<decltype(func(std::declval<T>()))>();
}

template<typename T, typename Func>
auto map(maybe<std::optional<T>> maybe_val, Func func) -> maybe<std::optional<decltype(func(std::declval<T>()))>> {
    static_assert(std::is_invocable_v<Func, T>, "Func must be callable");
    if(maybe_val.has_value() && maybe_val.get_value().has_value()) return maybe<std::optional<decltype(func(std::declval<T>()))>>(func(maybe_val.get_value().value()));
    return maybe<std::optional<decltype(func(std::declval<T>()))>>();
}

template<typename T, typename Pred>
auto filter(maybe<T> maybe_val, Pred pred) {
    static_assert(std::is_invocable_v<Pred, T>, "Pred must be callable");
    static_assert(std::is_convertible_v<std::invoke_result_t<Pred, T>, bool>, "Pred must return bool");
    if(maybe_val.has_value() && pred(maybe_val.get_value())) return maybe_val;
    return none<T>();
}

template<typename T, typename Pred>
auto filter(maybe<std::optional<T>> maybe_val, Pred pred) {
    static_assert(std::is_invocable_v<Pred, T>, "Pred must be callable");
    static_assert(std::is_convertible_v<std::invoke_result_t<Pred, T>, bool>, "Pred must return bool");
    if(maybe_val.has_value() && maybe_val.get_value().has_value() && pred(maybe_val.get_value().value())) return maybe_val;
    return none<std::optional<T>>();
}

template<typename T>
auto or_else(maybe<T> maybe_val, const T& default_value) {
    if(maybe_val.has_value()) return maybe_val.get_value();
    return default_value;
}

template<typename T>
auto or_else(maybe<std::optional<T>> maybe_val, const T& default_value) {
    if(maybe_val.has_value() && maybe_val.get_value().has_value()) return maybe_val.get_value().value();
    return default_value;
}

template <typename T, typename Func>
auto and_then(maybe<T> maybe_val, Func func) {
    static_assert(std::is_invocable_v<Func, T>, "Func must be callable");
    static_assert(std::is_convertible_v<std::invoke_result_t<Func, T>, T>, "Func(T) must return T");
    if(maybe_val.has_value()) return maybe<T>(func(maybe_val.get_value()));
    return none<T>();
}

template<typename T>
auto flatten(maybe<maybe<T>> maybe_val) {
    if(maybe_val.has_value()) return maybe_val.get_value();
    return none<T>();
}

template<typename T, typename Func>
auto flat_map(maybe<T> maybe_val, Func func) -> maybe<decltype(func(std::declval<T>()).get_value())> {
    static_assert(std::is_invocable_v<Func, T>, "Func must be callable");
    if(maybe_val.has_value()) return func(maybe_val.get_value());
    return none<decltype(func(std::declval<T>()).get_value())>();
}

template<typename T, typename Func>
auto flat_map(maybe<std::optional<T>> maybe_val, Func func) -> maybe<decltype(func(std::declval<T>()).get_value())> {
    static_assert(std::is_invocable_v<Func, T>, "Func must be callable");
    if(maybe_val.has_value() && maybe_val.get_value().has_value()) return func(maybe_val.get_value().value());
    return none<decltype(func(std::declval<T>()).get_value())>();
}

template<typename T, typename Func>
auto apply(maybe<Func> maybe_func, maybe<T> maybe_val) -> maybe<decltype(maybe_func.get_value()(maybe_val.get_value()))> {
    static_assert(std::is_invocable_v<Func, T>, "Func must be callable");
    if(maybe_func.has_value() && maybe_val.has_value()) return some(maybe_func.get_value()(maybe_val.get_value()));
    return none<decltype(maybe_func.get_value()(maybe_val.get_value()))>();
}

template<typename T, typename Func>
auto apply(maybe<Func> maybe_func, maybe<std::optional<T>> maybe_val) -> maybe<decltype(maybe_func.get_value()(maybe_val.get_value().value()))> {
    static_assert(std::is_invocable_v<Func, T>, "Func must be callable");
    if(maybe_func.has_value() && maybe_val.has_value() && maybe_val.get_value().has_value()) return some(maybe_func.get_value()(maybe_val.get_value().value()));
    return none<decltype(maybe_func.get_value()(maybe_val.get_value().value()))>();
}

template<typename T, typename Func>
auto traverse(const std::vector<maybe<T>>& maybe_vec, Func func) -> maybe<std::vector<decltype(func(std::declval<T>()).get_value())>> {
    static_assert(std::is_invocable_v<Func, T>, "Func must be callable");
    std::vector<decltype(func(std::declval<T>()).get_value())> result;
    for(const auto& maybe_val : maybe_vec) {
        if(maybe_val.has_value())
            result.push_back(func(maybe_val.get_value()).get_value());
        else
            return none<std::vector<decltype(func(std::declval<T>()).get_value())>>();
    }
    return some(result);
}

template<typename T>
auto  sequence(const std::vector<maybe<T>>& maybe_vec) {
    std::vector<T> result;
    for (const auto& maybe_val : maybe_vec) {
        if(maybe_val.has_value())
            result.push_back(maybe_val.get_value());
        else
            return none<std::vector<T>>();
    }
    return some(result);
}

template<typename T, typename Func, typename Acc>
auto fold(const std::vector<maybe<T>>& maybe_vec, Func func, Acc init = Acc{}) {
    static_assert(std::is_invocable_v<Func, Acc, T>, "Func must be callable");
    static_assert(std::is_convertible_v<std::invoke_result_t<Func, Acc, T>, Acc>, "Func must return Acc");
    Acc result(init);
    for(const auto& maybe_val : maybe_vec) {
        if(maybe_val.has_value())
            result = func(result, maybe_val.get_value());
    }
    return result;
}
} // end namespace
#endif
