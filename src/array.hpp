// Copyright (C) 2025 Tycho Softworks.
// This code is licensed under MIT license.

#ifndef TYCHO_ARRAY_HPP_
#define TYCHO_ARRAY_HPP_

#include <vector>
#include <array>
#include <algorithm>
#include <stdexcept>

namespace tycho {
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

    template <typename Iterator>
    slice(Iterator first, Iterator last) {
        std::copy(first, last, std::back_inserter(*this));
    }

    template <typename Iterator, typename Predicate>
    slice(Iterator first, Iterator last, Predicate pred) {
        std::copy_if(first, last, std::back_inserter(*this), pred);
    }

    operator bool() const {
        return !this->empty();
    }

    auto operator!() const {
        return this->empty();
    }

    auto find(const T& value) const {
        return std::find(this->begin(), this->end(), value);
    }

    auto contains(const T& value) const {
        return std::find(this->begin(), this->end(), value) != this->end();
    }

    auto subslice(size_type start, size_type last) const {
        if (start > this->size() || last > this->size() || start > last)
            throw std::out_of_range("Invalid subslice range");
        return slice(this->begin() + start, this->begin() + last);
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

    void remove(size_type start, size_type last) const {
        if (start > this->size() || last > this->size() || start > last)
            throw std::out_of_range("Invalid subslice range");
        this->erase(this->begin() + start, this->begin() + last);
    }

    void remove(const T& value) {
        this->erase(std::remove(this->begin(), this->end(), value), this->end());
    }
};
} // end namespace
#endif
