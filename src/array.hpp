// Copyright (C) 2025 Tycho Softworks.
// This code is licensed under MIT license.

#ifndef TYCHO_CONTAINERS_HPP_
#define TYCHO_CONTAINERS_HPP_

#include <vector>
#include <array>
#include <algorithm>
#include <stdexcept>

namespace tycho {
template <typename T, std::size_t N, std::ptrdiff_t Offset = 0>
class array : public std::array<T,N> {
public:
    using std::array<T,N>::array;
    explicit array(const std::array<T,N>& from) : std::array<T, N>(from) {}

    auto operator[](std::size_t index) -> T& {
        if(index < Offset || index >= N + Offset)
            throw std::out_of_range("Index out of range");
        return std::array<T,N>::operator[](index - Offset);
    }

    auto operator[](std::size_t index) const -> const T& {
        if(index < Offset || index >= N + Offset)
            throw std::out_of_range("Index out of range");
        return std::array<T,N>::operator[](index - Offset);
    }
};

template<typename T, std::ptrdiff_t Offset = 0>
class vector : public std::vector<T> {
public:
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

    auto operator[](std::size_t index) -> T& {
        if(index < Offset || index >= this->size() + Offset)
            throw std::out_of_range("Index out of range");
        return std::vector<T>::operator[](index - Offset);
    }

    auto operator[](std::size_t index) const -> const T& {
        if(index < Offset || index >= this->size() + Offset)
            throw std::out_of_range("Index out of range");
        return std::vector<T>::operator[](index - Offset);
    }

    auto subvector(typename std::vector<T>::size_type start, typename std::vector<T>::size_type end) const {
        start -= Offset;
        end -= Offset;
        if (start > this->size() || end > this->size() || start > end)
            throw std::out_of_range("Invalid subvector range");
        return vector(this->begin() + start, this->begin() + end);
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

    void remove(typename std::vector<T>::size_type start, typename std::vector<T>::size_type end) const {
        start -= Offset;
        end -= Offset;
        if (start > this->size() || end > this->size() || start > end)
            throw std::out_of_range("Invalid subvector range");
        this->erase(this->begin() + start, this->begin() + end);
    }

    void remove(const T& value) {
        this->erase(std::remove(this->begin(), this->end(), value), this->end());
    }
};
} // end namespace
#endif
