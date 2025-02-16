// Copyright (C) 2024 Tycho Softworks.
// This code is licensed under MIT license.

#ifndef TYCHO_RANGES_HPP_
#define TYCHO_RANGES_HPP_

#include <algorithm>
#include <iterator>
#include <numeric>

namespace tycho::ranges {
template<typename Predicate>
class filter {
public:
    explicit filter(Predicate pred) : pred_(pred) {}
    filter(const filter&) = delete;
    auto operator=(const filter&) -> auto& = delete;

    template <typename Container>
    auto operator()(const Container& container) const {
        Container result{};
        std::copy_if(container.begin(), container.end(), std::back_inserter(result), pred_);
        return result;
    }

private:
    Predicate pred_;
};

template<typename Func>
class transform {
public:
    explicit transform(Func func) : func_(func) {}
    transform(const transform&) = delete;
    auto operator=(const transform&) -> auto& = delete;

    template <typename Container>
    auto operator()(const Container& container) const {
        Container result{};
        std::transform(container.begin(), container.end(), std::back_inserter(result), func_);
        return result;
    }

private:
    Func func_;
};

template<typename Container>
auto copy(const Container& container, std::size_t pos, std::size_t count) {
    Container result{};
    if(!count || pos >= container.size()) return result;
    if(pos + count >= container.size())
        count = container.size() - pos;
    std::copy_n(std::advance(container.begin(), pos), count, std::back_inserter(result));
    return result;
}

template<typename Container>
auto take(const Container& container, std::size_t size) {
    Container result{};
    std::copy_n(container.begin(), std::min(size, container.size()), std::back_inserter(result));
    return result;
}

template<typename Container>
auto drop(const Container& container, std::size_t size) {
    Container result{};
    if (size < container.size())
        std::copy(container.begin() + size, container.end(), std::back_inserter(result));
    return result;
}

template <typename Container>
auto join(const Container& a, const Container& b) {
    Container result = a;
    result.insert(result.end(), b.begin(), b.end());
    return result;
}

template<typename Container, typename Func>
auto each(Container& container, Func func) {
    auto it = container.begin();
    while(it != container.end()) {
        func(*it);
        ++it;
    }
    return container;
}

template<typename Container, typename T>
auto contains(const Container& container, T value) {
    auto it = container.begin();
    while(it != container.end()) {
        if(*it == value) return true;
        ++it;
    }
    return false;
}

template<typename Container, typename T, typename Func>
auto fold(const Container& container, T init, Func func) -> T {
    return std::accumulate(container.begin(), container.end(), init, func);
}

template<typename Container, typename Func>
auto make(std::size_t size, Func func) {
    Container result{};
    std::fill_n(std::back_inserter(result), size, func());
    return result;
}

template<typename Container, typename Range>
auto operator|(const Container& container, const Range& range) {
    return range(container);
}
} // end namespace
#endif
