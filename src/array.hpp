// Copyright (C) 2025 Tycho Softworks.
// This code is licensed under MIT license.

#ifndef TYCHO_ARRAY_HPP_
#define TYCHO_ARRAY_HPP_

#include <vector>
#include <array>
#include <algorithm>
#include <iterator>
#include <stdexcept>
#include <cstdint>
#include <type_traits>

namespace tycho {
namespace crypto {
using key_t = std::pair<const uint8_t *, std::size_t>;
} // end namespace

template <typename T, std::size_t N, std::ptrdiff_t Offset = 0>
class array : public std::array<T,N> {
public:
    using reference = T&;
    using const_reference = const T&;
    using size_type = typename std::array<T,N>::size_type;
    using iterator = typename std::array<T,N>::iterator;
    using const_iterator = typename std::array<T,N>::const_iterator;
    using reverse_iterator = typename std::array<T,N>::reverse_iterator;
    using const_reverse_iterator = typename std::array<T,N>::const_reverse_iterator;

    using std::array<T,N>::array;
    explicit array(const std::array<T,N>& from) : std::array<T, N>(from) {}

    auto operator[](size_type index) -> T& {
        if(index < Offset || index >= N + Offset)
            throw std::out_of_range("Index out of range");
        return std::array<T,N>::operator[](index - Offset);
    }

    auto operator[](size_type index) const -> const T& {
        if(index < Offset || index >= N + Offset)
            throw std::out_of_range("Index out of range");
        return std::array<T,N>::operator[](index - Offset);
    }

    constexpr auto min() const {
        return Offset;
    }

    constexpr auto max() const {
        return size_type(Offset + N - 1);
    }

    auto contains(const T& value) const {
        return std::find(this->begin(), this->end(), value) != this->end();
    }
};

template<typename T, class Alloc = std::allocator<T>>
class slice : public std::vector<T,Alloc> {
public:
    using reference = T&;
    using const_reference = const T&;
    using size_type = typename std::vector<T,Alloc>::size_type;
    using iterator = typename std::vector<T,Alloc>::iterator;
    using const_iterator = typename std::vector<T,Alloc>::const_iterator;
    using reverse_iterator = typename std::vector<T,Alloc>::reverse_iterator;
    using const_reverse_iterator = typename std::vector<T,Alloc>::const_reverse_iterator;

    using std::vector<T,Alloc>::vector;
    explicit slice(const std::vector<T,Alloc>& vec) : std::vector<T,Alloc>(vec) {}
    explicit slice(std::vector<T,Alloc>&& vec) : std::vector<T,Alloc>(std::move(vec)) {}

    template <typename U = T, std::enable_if_t<sizeof(U) == 1, int> = 0>
    explicit slice(crypto::key_t& key) :
    std::vector<T>(key.second) {
        memcpy(this->data(), key.first, key.second); // FlawFinder: ignore
    }

    template <typename Iterator>
    slice(Iterator first, Iterator last) {
        std::copy(first, last, std::back_inserter(*this));
    }

    template <typename Iterator, typename Predicate>
    slice(Iterator first, Iterator last, Predicate pred) {
        std::copy_if(first, last, std::back_inserter(*this), pred);
    }

    template <typename U = T, std::enable_if_t<sizeof(U) == 1, int> = 0>
    operator crypto::key_t() {
        return std::make_pair(reinterpret_cast<const uint8_t *>(this->data()), this->size());
    }

    operator bool() const {
        return !this->empty();
    }

    auto operator!() const {
        return this->empty();
    }

    auto operator*() const -> const T* {
        return this->data();
    }

    auto operator*() -> T* {
        return this->data();
    }

    auto find(const T& value) const {
        return std::find(this->begin(), this->end(), value);
    }

    auto contains(const T& value) const {
        return std::find(this->begin(), this->end(), value) != this->end();
    }

    auto subslice(size_type pos, size_type count = 0) const {
        if(pos + count > this->size())
            throw std::out_of_range("Invalid subslice range");
        if(!count)
            count = this->size() - pos;
        return slice(this->begin() + pos, this->begin() + pos + count);
    }

    template <typename Func>
    void each(Func func) {
        for (auto& element : *this)
            func(element);
    }

    template <typename Pred>
    auto filter_if(Pred pred) const {
        slice<T,Alloc> result;
        std::copy_if(this->begin(), this->end(), std::back_inserter(result), pred);
        return result;
    }

    template <typename Pred>
    auto extract_if(Pred pred) const {
        slice<T,Alloc> result;
        auto it = this->begin();
        while(it != this->end()) {
            if(pred(*it)) {
                result.push_back(*it);
                it = this->erase(it);
            }
            else
                ++it;
        }
        return result;
    }

    template <typename Pred>
    void remove_if(Pred pred) const {
        this->erase(std::remove_if(this->begin(), this->end(), pred), this->end());
    }

    void remove(size_type pos, size_type count = 0) const {
        if(pos + count > this->size())
            throw std::out_of_range("Invalid slice range");
        if(!count)
            count = this->size() - pos;
        if(!count)
            this->clear();
        else
            this->erase(this->begin() + pos, this->begin() + pos + count);
    }

    void remove(const T& value) {
        this->erase(std::remove(this->begin(), this->end(), value), this->end());
    }
};

template<typename T>
class span {
public:
    using size_type = std::size_t;
    using value_type = T;

    constexpr span(T* ptr, size_type size) : ptr_(ptr), size_(size) {}

    template<size_type S>
    explicit constexpr span(T(&arr)[S]) : span(arr, S) {}

    template <typename Container, typename = std::enable_if_t<std::is_same_v<T, typename Container::value_type>>>
    explicit span(Container& container) : span(container.data(), container.size()) {}

    constexpr auto operator[](size_type index) const -> T& {
        if(index >= size_)
            throw std::out_of_range("Span index past end");
        return ptr_[index];
    }

    constexpr auto at(size_type index) const -> T& {
        if(index >= size_)
            throw std::out_of_range("Span index past end");
        return ptr_[index];
    }

    constexpr auto size_bytes() const {
        return size_ * sizeof(T);
    }

    constexpr auto size() const {
        return size_;
    }

    constexpr auto data() const -> T* {
        return ptr_;
    }

    constexpr auto begin() const {
        return ptr_;
    }

    constexpr auto end() const {
        return ptr_ + size_;
    }

    constexpr auto front() const -> T& {
        return ptr_[0];
    }

    constexpr auto back() const -> T& {
        return ptr_[size_ - 1];
    }

    constexpr auto empty() const {
        return size_ == 0;
    }

    auto subspan(size_type pos, size_type count = 0) const {
        if(pos + count > size_)
            throw std::out_of_range("Invalid subspan range");
        return span(ptr_ + pos, count ? count : size_ - pos);
    }

private:
    T *ptr_{nullptr};
    size_type size_{0};
};

using byteslice = slice<std::byte>;

template<typename T, std::size_t S>
constexpr auto make_span(T(&arr)[S]) {
        return span<T>(arr);
}

template<typename Container>
auto make_span(Container& container) -> span<typename Container::value_type> {
    return span<typename Container::value_type>(container);
}
} // end namespace
#endif
