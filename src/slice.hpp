// Copyright (C) 2023 Tycho Softworks.
// This code is licensed under MIT license.

#ifndef TYCHO_SLICE_HPP_
#define TYCHO_SLICE_HPP_

#include <list>
#include <memory>
#include <iterator>
#include <initializer_list>
#include <algorithm>
#include <stdexcept>

namespace tycho {
template<typename T>
class slice {
public:
    using value_type = T;
    using size_type = std::size_t;
    using pointer = typename std::shared_ptr<T>;
    using reference = T&;
    using const_reference = const T&;
    using iterator = typename std::list<std::shared_ptr<T>>::iterator;
    using const_iterator = typename std::list<std::shared_ptr<T>>::const_iterator;
    using reverse_iterator = typename std::list<std::shared_ptr<T>>::reverse_iterator;
    using const_reverse_iterator = typename std::list<std::shared_ptr<T>>::const_reverse_iterator;

    slice() = default;
    explicit slice(const std::list<std::shared_ptr<T>>& from) : list_(from) {}

    slice(std::initializer_list<T> init) {
        assign(init);
    }

    template <typename Iterator>
    slice(Iterator first, Iterator last) {
        std::copy(first, last, std::back_inserter(*this));
    }

    template <typename Iterator, typename Predicate>
    slice(Iterator first, Iterator last, Predicate pred) {
        std::copy_if(first, last, std::back_inserter(*this), pred);
    }

    operator bool() const {
        return !empty();
    }

    auto operator!() const {
        return empty();
    }

    auto operator=(const std::list<std::shared_ptr<T>>& from) -> auto& {
        list_ = from;
        return *this;
    }

    auto operator=(std::initializer_list<T> init) -> auto& {
        assign(init);
        return *this;
    }

    auto operator()(size_type index) {
        return at(index);
    }

    auto operator()(size_type index) const {
        return at(index);
    }

    auto operator[](size_type index) -> T& {
        if(index >= list_.size())
            throw std::out_of_range("Index out of range");
        auto it = list_.begin();
        std::advance(it, index);
        return **it;
    }

    auto operator[](size_type index) const -> const T& {
        if(index >= list_.size())
            throw std::out_of_range("Index out of range");
        auto it = list_.begin();
        std::advance(it, index);
        return **it;
    }

    auto operator*() const noexcept -> std::list<T>& {
        return list_;
    }

    auto operator*() noexcept -> std::list<T>& {
        return list_;
    }

    auto operator<<(const T& item) -> auto& {
        append(item);
        return *this;
    }

    auto operator<<(const slice<T>& other) -> auto& {
        std::copy(other.list_.begin(), other.list_.end(), std::back_inserter(list_));
        return *this;
    }

    auto operator<<(std::initializer_list<T> items) -> auto& {
        for(const auto& item : items) {
            append(item);
        }
    }

    auto operator>>(const T& item) -> auto& {
        prepend(item);
        return *this;
    }

    auto operator>>(const slice<T>& other) -> auto& {
        std::copy(other.list_.rbegin(), other.list_.rend(), std::front_inserter(list_));
        return *this;
    }

    constexpr auto min() const {
        return size_type(0);
    }

    auto max() const {
        if(this->empty())
            throw std::out_of_range("Slice is empty");
        return size_type(size() - 1);
    }

    void push_back(const T& item) {
        append(item);
    }

    void push_front(const T& item) {
        prepend(item);
    }

    auto size() const {
        return list_.size();
    }

    auto max_size() const {
        return list_.max_size();
    }

    auto at(size_type index) {
        if(index >= list_.size())
            throw std::out_of_range("Index out of range");
        auto it = list_.begin();
        std::advance(it, index);
        return *it;
    }

    auto at(size_type index) const {
        if(index >= list_.size())
            throw std::out_of_range("Index out of range");
        auto it = list_.begin();
        std::advance(it, index);
        return *it;
    }

    template<typename Func>
    void each(Func func) const {
        for(auto& ptr : list_)
            func(*ptr);
    }

    template<typename Predicate>
    auto filter(Predicate pred) const {
        slice<T> result;
        for(auto& ptr : list_) {
            if(pred(*ptr))
               // cppcheck-suppress useStlAlgorithm
               result.push_back(ptr);
        }
        return result;
    }

    template<typename Func, typename Acc>
    auto fold(Func func, Acc init = Acc{}) {
        Acc result(init);
        for(auto& ptr : list_) {
            result = func(result, *ptr);
        }
        return result;
    }

    auto empty() const {
        return list_.empty();
    }

    auto begin() noexcept {
        return list_.begin();
    }

    auto begin() const noexcept {
        return list_.cbegin();
    }

    auto rbegin() noexcept {
        return list_.rbegin();
    }

    auto rbegin() const noexcept {
        return list_.crbegin();
    }

    auto end() noexcept {
        return list_.end();
    }

    auto end() const noexcept {
        return list_.cend();
    }

    auto rend() noexcept {
        return list_.rend();
    }

    auto rend() const noexcept {
        return list_.crend();
    }

    void assign(const T& item) {
        list_.clear();
        list_.push_back(std::make_shared<T>(item));
    }

    void assign(const slice<T>& other) {
        list_.clear();
        std::copy(other.list_.begin(), other.list_.end(), std::back_inserter(list_));
    }

    void assign(const std::list<std::shared_ptr<T>>& from) {
        list_ = from;
    }

    void assign(std::initializer_list<T> items) {
        list_.clear();
        for(const auto& item : items) {
            append(item);
        }
    }

    template <typename Iterator>
    void assign(Iterator first, Iterator last) {
        list_.clear();
        std::copy(first, last, std::back_inserter(*this));
    }

    template <typename Iterator, typename Predicate>
    void assign(Iterator first, Iterator last, Predicate pred) {
        list_.clear();
        std::copy_if(first, last, std::back_inserter(*this), pred);
    }

    void append(const T& item) {
        list_.push_back(std::make_shared<T>(item));
    }

    void append(const slice<T>& other) {
        std::copy(other.list_.begin(), other.list_.end(), std::back_inserter(list_));
    }

    void append(std::initializer_list<T> items) {
        for(const auto& item : items) {
            append(item);
        }
    }

    template <typename Iterator>
    void append(Iterator first, Iterator last) {
        std::copy(first, last, std::back_inserter(*this));
    }

    template <typename Iterator, typename Predicate>
    void append(Iterator first, Iterator last, Predicate pred) {
        std::copy_if(first, last, std::back_inserter(*this), pred);
    }

    void prepend(const T& item) {
        list_.push_front(std::make_shared<T>(item));
    }

    void prepend(const slice<T>& other) {
        std::copy(other.list_.rbegin(), other.list_.rend(), std::front_inserter(list_));
    }

    auto insert(iterator it, const T& value) {
        return list_.insert(it, std::make_shared<T>(value));
    }

    auto erase(iterator it) {
        return list_.erase(it);
    }

    auto erase(iterator start, iterator last) {
        return list_.erase(start, last);
    }

    void erase(size_type start, size_type last) {
        if (start >= list_.size() || last > list_.size() || start > last)
            throw std::out_of_range("Invalid range");
        auto it_start = list_.begin();
        auto it_end = list_.begin();
        std::advance(it_start, start);
        std::advance(it_end, last);
        list_.erase(it_start, it_end);
    }

    void clear() {
        list_.clear();
    }

    void resize(size_type count) {
        list_.resize(count);
    }

    void swap(const slice<T>& other) noexcept {
        list_.swap(other.list_);
    }

    void remove(const T& value) {
        list_.remove_if([&](const std::shared_ptr<T>& item) {
            return *item == value;
        });
    }

    template<typename Predicate>
    void remove_if(Predicate pred) {
        list_.remove_if([&](const std::shared_ptr<T>& item) {
            return pred(*item);
        });
    }

    void copy(const slice<T>& other, size_type pos = 0) {
        if(pos > list_.size())
            throw std::out_of_range("Copy position out of range");
        auto it = list_.begin();
        std::advance(it, pos);
        std::copy(other.list_.begin(), other.list_.end(), std::inserter(list_, it));
    }

    template<typename Predicate>
    auto clone(Predicate pred) const {
        slice<T> result;
        for(const auto ptr : list_) {
            if(pred(*ptr))
               result.append(*ptr);
        }
        return result;
    }

    auto clone(size_type start, size_type last) const {
        if(start >= list_.size() || last > list_.size() || start > last)
            throw std::out_of_range("Invalid subslice range");
        slice<T>  result;
        auto it = list_.begin();
        std::advance(it, start);
        for(auto i = start; i < last; ++i) {
            result.add(**it++);
        }
        return result;
    }

    auto clone(size_type start = 0) {
        return clone(start, list_.size());
    }

    auto subslice(size_type start, size_type last) const {
        if(start >= list_.size() || last > list_.size() || start > last)
            throw std::out_of_range("Invalid subslice range");
        slice<T>  result;
        auto it = list_.begin();
        std::advance(it, start);
        for(auto i = start; i < last; ++i) {
            result.list_.push_back(*it++);
        }
        return result;
    }

    auto subslice(iterator start_it, iterator end_it) const {
        if(std::distance(list_.begin(), start_it) > std::distance(start_it, end_it) || std::distance(list_.begin(), end_it) > list_.size())
            throw std::out_of_range("Invalid slice iterator range");
        slice<T> result;
        std::copy(start_it, end_it, std::back_inserter(result.list_));
        return result;
    }

private:
    std::list<std::shared_ptr<T>> list_;
};
} // end namespace
#endif
