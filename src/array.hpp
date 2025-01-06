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
        return Offset + N - 1;
    }
};

template<typename T, std::ptrdiff_t Offset = 0>
class vector : public std::vector<T> {
public:
    using reference = T&;
    using const_reference = const T&;
    using size_type = typename std::vector<T>::size_type;
    using iterator = typename std::vector<T>::iterator;
    using const_iterator = typename std::vector<T>::const_iterator;
    using reverse_iterator = typename std::vector<T>::reverse_iterator;
    using const_reverse_iterator = typename std::vector<T>::const_reverse_iterator;

    using std::vector<T>::vector;
    explicit vector(const std::vector<T>& vec) : std::vector<T>(vec) {}
    explicit vector(std::vector<T>&& vec) : std::vector<T>(std::move(vec)) {}

    template <typename Iterator>
    vector(Iterator first, Iterator last) {
        std::copy(first, last, std::back_inserter(*this));
    }

    template <typename Iterator, typename Predicate>
    vector(Iterator first, Iterator last, Predicate pred) {
        std::copy_if(first, last, std::back_inserter(*this), pred);
    }

    operator bool() const {
        return !this->empty();
    }

    auto operator!() const {
        return this->empty();
    }

    auto operator[](size_type index) -> T& {
        if(index < Offset || index >= this->size() + Offset)
            throw std::out_of_range("Index out of range");
        return std::vector<T>::operator[](index - Offset);
    }

    auto operator[](size_type index) const -> const T& {
        if(index < Offset || index >= this->size() + Offset)
            throw std::out_of_range("Index out of range");
        return std::vector<T>::operator[](index - Offset);
    }

    constexpr auto min() const {
        return Offset;
    }

    auto max() const {
        return Offset + this->size() - 1;
    }

    auto subvector(size_type start, size_type last) const {
        start -= Offset;
        last -= Offset;
        if (start > this->size() || last > this->size() || start > last)
            throw std::out_of_range("Invalid subvector range");
        return vector(this->begin() + start, this->begin() + last);
    }

    template <typename Func>
    void each(Func func) {
        for (auto& element : *this)
            func(element);
    }

    template <typename Predicate>
    auto filter(Predicate pred) const {
        vector<T,Offset> result;
        std::copy_if(this->begin(), this->end(), std::back_inserter(result), pred);
        return result;
    }

    template <typename Predicate>
    void remove_if(Predicate pred) const {
        this->erase(std::remove_if(this->begin(), this->end(), pred), this->end());
    }

    void remove(size_type start, size_type last) const {
        start -= Offset;
        last -= Offset;
        if (start > this->size() || last > this->size() || start > last)
            throw std::out_of_range("Invalid subvector range");
        this->erase(this->begin() + start, this->begin() + last);
    }

    void remove(const T& value) {
        this->erase(std::remove(this->begin(), this->end(), value), this->end());
    }
};
} // end namespace
#endif
