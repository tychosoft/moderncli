// Copyright (C) 2023 Tycho Softworks.
// This code is licensed under MIT license.

#ifndef TYCHO_SLICE_HPP_
#define TYCHO_SLICE_HPP_

#include <list>
#include <memory>
#include <iterator>
#include <initializer_list>
#include <algorithm>

namespace tycho {
template<typename T>
class slice {
public:
    using value_type = T;
    using size_type = std::size_t;
    using reference = T&;
    using const_reference = const T&;
    using iterator = typename std::list<std::shared_ptr<T>>::iterator;
    using const_iterator = typename std::list<std::shared_ptr<T>>::const_iterator;
    using reverse_iterator = typename std::list<std::shared_ptr<T>>::reverse_iterator;
    using const_reverse_iterator = typename std::list<std::shared_ptr<T>>::const_reverse_iterator;

    slice() = default;

    slice(std::initializer_list<T> init) {
        for(const auto& item : init) {
            assign(item);
        }
    }

    operator bool() const {
        return !empty();
    }

    auto operator!() const {
        return empty();
    }

    auto operator[](std::size_t index) -> T& {
        if(index >= list.size()) {
            throw std::out_of_range("Index out of range");
        }
        auto it = list.begin();
        std::advance(it, index);
        return **it;
    }

    auto operator+=(const slice<T>& other) -> auto& {
        append(other);
        return *this;
    }

    auto operator+=(const T& item) -> auto& {
        append(item);
        return *this;
    }

    auto operator<<(const T& item) -> auto& {
        append(item);
        return *this;
    }

    auto size() const {
        return list.size();
    }

    auto max_size() const {
        return list.max_size();
    }

    auto empty() const {
        return list.empty();
    }

    auto begin() noexcept {
        return list.begin();
    }

    auto begin() const noexcept {
        return list.cbegin();
    }

    auto rbegin() noexcept {
        return list.rbegin();
    }

    auto rbegin() const noexcept {
        return list.crbegin();
    }

    auto end() noexcept {
        return list.end();
    }

    auto end() const noexcept {
        return list.cend();
    }

    auto rend() noexcept {
        return list.rend();
    }

    auto rend() const noexcept {
        return list.crend();
    }

    void assign(const T& item) {
        list.clear();
        list.push_back(std::make_shared<T>(item));
    }

    void assign(const slice<T>& other) {
        list.clear();
        std::copy(other.list.begin(), other.list.end(), std::back_inserter(list));
    }

    void assign(std::initializer_list<T> items) {
        list.clear();
        for(const auto& item : items) {
            append(item);
        }
    }

    void append(const T& item) {
        list.push_back(std::make_shared<T>(item));
    }

    void append(const slice<T>& other) {
        std::copy(other.list.begin(), other.list.end(), std::back_inserter(list));
    }

    void append(std::initializer_list<T> items) {
        for(const auto& item : items) {
            assign(item);
        }
    }

    auto insert(iterator it, const T& value) {
        return list.insert(it, std::make_shared<T>(value));
    }

    auto erase(iterator it) {
        return list.erase(it);
    }

    void clear() {
        list.clear();
    }

    auto clone() const {
        slice<T> cloned;
        for (const auto& item : list) {
            cloned->data.push_back(std::make_shared<T>(*item));
        }
        return cloned;
    }

    void resize(std::size_t count) {
        list.resize(count);
    }

    void swap(const slice<T>& other) noexcept {
        list.swap(other.list);
    }

    void remove(const T& value) {
        list.remove_if([&](const std::shared_ptr<T>& item) {
            return *item == value;
        });
    }

    template<typename Predicate>
    void remove_if(Predicate pred) {
        list.remove_if([&](const std::shared_ptr<T>& item) {
            return pred(*item);
        });
    }

    void copy(const slice<T>& other, size_type pos = 0) {
        if(pos > list.size())
            throw std::out_of_range("Copy position out of range");
        auto it = list.begin();
        std::advance(it, pos);
        std::copy(other.list.begin(), other.list.end(), std::inserter(list, it));
    }

    auto subslice(size_type start, size_type end) const {
        if(start >= list.size() || end > list.size() || start > end)
            throw std::out_of_range("Invalid subslice range");
        slice<T>  result;
        auto it = list.begin();
        std::advance(it, start);
        for (auto i = start; i < end; ++i) {
            result.add(**it++);
        }
        return result;
    }

private:
    std::list<std::shared_ptr<T>> list;
};
} // end namespace
#endif
