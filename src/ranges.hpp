// Copyright (C) 2024 Tycho Softworks.
// This code is licensed under MIT license.

#ifndef TYCHO_RANGES_HPP_
#define TYCHO_RANGES_HPP_

#include <algorithm>
#include <iterator>
#include <numeric>
#include <type_traits>

namespace tycho::ranges {
template <typename, typename = std::void_t<>>
struct is_stl_container : std::false_type {};

template <typename T>
struct is_stl_container<T, std::void_t<typename T::iterator>> : std::true_type {};

template <typename T>
constexpr bool is_stl_container_v = is_stl_container<T>::value;

template<typename Pred>
class filter {
public:
    explicit filter(Pred pred) : pred_(pred) {}
    filter(const filter&) = delete;
    auto operator=(const filter&) -> auto& = delete;

    template <typename Container,
    typename = std::enable_if_t<is_stl_container_v<Container>>>
    auto operator()(const Container& container) const {
        using Value = typename Container::value_type;
        static_assert(std::is_invocable_v<Pred, Value>, "Pred must be callable");
        static_assert(std::is_convertible_v<std::invoke_result_t<Pred, Value>, bool>, "Pred result must be convertible to bool");
        Container result{};
        std::copy_if(container.begin(), container.end(), std::back_inserter(result), pred_);
        return result;
    }

private:
    Pred pred_;
};

template<typename Func>
class transform {
public:
    explicit transform(Func func) : func_(func) {}
    transform(const transform&) = delete;
    auto operator=(const transform&) -> auto& = delete;

    template <typename Container,
    typename = std::enable_if_t<is_stl_container_v<Container>>>
    auto operator()(const Container& container) const {
        using Value = typename Container::value_type;
        static_assert(std::is_invocable_v<Func, Value>, "Finct must be callable");
        static_assert(std::is_convertible_v<std::invoke_result_t<Func, Value>, Value>, "Result must be Container value");
        Container result{};
        std::transform(container.begin(), container.end(), std::back_inserter(result), func_);
        return result;
    }

private:
    Func func_;
};

template<typename Container,
typename = std::enable_if_t<is_stl_container_v<Container>>>
auto copy(const Container& container, std::size_t pos, std::size_t count) {
    Container result{};
    if(!count || pos >= container.size()) return result;
    if(pos + count >= container.size())
        count = container.size() - pos;
    std::copy_n(std::advance(container.begin(), pos), count, std::back_inserter(result));
    return result;
}

template <typename Container,
typename = std::enable_if_t<is_stl_container_v<Container>>>
auto take(const Container& container, std::size_t size) {
    Container result{};
    std::copy_n(container.begin(), std::min(size, container.size()), std::back_inserter(result));
    return result;
}

template<typename Container,
typename = std::enable_if_t<is_stl_container_v<Container>>>
auto drop(const Container& container, std::size_t size) {
    Container result{};
    if (size < container.size())
        std::copy(container.begin() + size, container.end(), std::back_inserter(result));
    return result;
}

template <typename Container,
typename = std::enable_if_t<is_stl_container_v<Container>>>
auto join(const Container& a, const Container& b) {
    Container result = a;
    result.insert(result.end(), b.begin(), b.end());
    return result;
}

template<typename Container, typename Func,
typename = std::enable_if_t<is_stl_container_v<Container>>>
auto each(Container& container, Func func) {
    auto it = container.begin();
    while(it != container.end()) {
        func(*it);
        ++it;
    }
    return container;
}

template<typename Container, typename T,
typename = std::enable_if_t<is_stl_container_v<Container>>>
auto contains(const Container& container, T value) {
    auto it = container.begin();
    while(it != container.end()) {
        if(*it == value) return true;
        ++it;
    }
    return false;
}

template<typename Container, typename T, typename Func,
typename = std::enable_if_t<is_stl_container_v<Container>>>
auto fold(const Container& container, T init, Func func) -> T {
    return std::accumulate(container.begin(), container.end(), init, func);
}

template<typename Container,
typename = std::enable_if_t<is_stl_container_v<Container>>>
auto count(const Container& container) {
    return std::distance(container.begin(), container.end());
}

template<typename Container, typename Pred,
typename = std::enable_if_t<is_stl_container_v<Container>>>
auto count(const Container& container, Pred&& pred) {
    return std::count_if(container.begin(), container.end(), std::forward<Pred>(pred));
}

template<typename Container, typename Pred,
typename = std::enable_if_t<is_stl_container_v<Container>>>
auto all(const Container& container, Pred&& pred) {
    return std::all_of(container.begin(), container.end(), std::forward<Pred>(pred));
}

template<typename Container, typename Pred,
typename = std::enable_if_t<is_stl_container_v<Container>>>
auto any(const Container& container, Pred&& pred) {
    return std::any_of(container.begin(), container.end(), std::forward<Pred>(pred));
}

template<typename Container, typename Pred,
typename = std::enable_if_t<is_stl_container_v<Container>>>
auto none(const Container& container, Pred&& pred) {
    return std::none_of(container.begin(), container.end(), std::forward<Pred>(pred));
}

template<typename Container, typename Func,
typename = std::enable_if_t<is_stl_container_v<Container>>>
auto make(std::size_t size, Func&& func) {
    Container result{};
    std::generate_n(std::back_inserter(result), size, std::forward<Func>(func));
    return result;
}

template<typename Container, typename Range,
typename = std::enable_if_t<is_stl_container_v<Container>>>
auto operator|(const Container& container, const Range& range) {
    return range(container);
}
} // end namespace
#endif
