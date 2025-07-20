// Copyright (C) 2025 Tycho Softworks.
// This code is licensed under MIT license.

#ifndef TYCHO_ARRAY_HPP_
#define TYCHO_ARRAY_HPP_

#include <vector>
#include <array>
#include <algorithm>
#include <iterator>
#include <optional>
#include <stdexcept>
#include <cstdint>
#include <cstring>
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
    // cppcheck-suppress noExplicitConstructor
    array(const std::array<T,N>& from) : std::array<T, N>(from) {}

    auto operator[](size_type index) -> T& {
        if(index < Offset || index >= N + Offset) throw std::out_of_range("Index out of range");
        return std::array<T,N>::operator[](index - Offset);
    }

    auto operator[](size_type index) const -> const T& {
        if(index < Offset || index >= N + Offset) throw std::out_of_range("Index out of range");
        return std::array<T,N>::operator[](index - Offset);
    }

    auto at(size_type index) -> T& {
        if(index < Offset || index >= N + Offset) throw std::out_of_range("Index out of range");
        return std::array<T, N>::at(index - Offset);
    }

    auto at(size_type index) const -> const T& {
        if(index < Offset || index >= N + Offset) throw std::out_of_range("Index out of range");
        return std::array<T, N>::at(index - Offset);
    }

    auto get(size_type index) noexcept -> std::optional<T> {
        if(index < Offset || index >= N + Offset) return std::nullopt;
        return std::array<T, N>::at(index - Offset);
    }

    auto get_or(size_type index, const T* or_else = nullptr) const noexcept -> const T* {
        if(index < Offset || index >= N + Offset) return or_else;
        return this->data() + (index - Offset);
    }

    auto get_or(size_type index, T* or_else = nullptr) noexcept -> T* {
        if(index < Offset || index >= N + Offset) return or_else;
        return this->data() + (index - Offset);
    }

    constexpr auto min() const noexcept {
        return Offset;
    }

    constexpr auto max() const noexcept {
        return size_type(Offset + N - 1);
    }

    auto find(const T& value) const {
        return std::find(this->begin(), this->end(), value);
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
    // cppcheck-suppress noExplicitConstructor
    slice(const std::vector<T,Alloc>& vec) : std::vector<T,Alloc>(vec) {}

    // cppcheck-suppress noExplicitConstructor
    slice(std::vector<T,Alloc>&& vec) : std::vector<T,Alloc>(std::move(vec)) {}

    template <typename U = T, std::enable_if_t<sizeof(U) == 1, int> = 0>
    slice(crypto::key_t& key) : // cppcheck-suppress noExplicitConstructor
    std::vector<T>(key.second) {
        memcpy(this->data(), key.first, key.second);
    }

    template <typename Iterator>
    slice(Iterator first, Iterator last) {
        std::copy(first, last, std::back_inserter(*this));
    }

    template <typename Iterator, typename Pred>
    slice(Iterator first, Iterator last, Pred pred) {
        std::copy_if(first, last, std::back_inserter(*this), pred);
    }

    template <typename U = T, std::enable_if_t<sizeof(U) == 1, int> = 0>
    operator crypto::key_t() {
        return std::make_pair(reinterpret_cast<const uint8_t *>(this->data()), this->size());
    }

    explicit operator bool() const {
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

    auto get_or(size_type index, const T* or_else = nullptr) const -> const T* {
        if(index < this->size()) return this->data() + index;
        return or_else;
    }

    auto get_or(size_type index, T* or_else = nullptr) -> T* {
        if(index < this->size()) return this->data() + index;
        return or_else;
    }

    auto get(size_type index) const -> std::optional<T> {
        if(index < this->size()) return this->at(index);
        return std::nullopt;
    }

    auto find(const T& value) const {
        return std::find(this->begin(), this->end(), value);
    }

    auto contains(const T& value) const {
        return std::find(this->begin(), this->end(), value) != this->end();
    }

    auto subslice(size_type pos, size_type count = 0) const {
        if(pos + count > this->size()) throw std::out_of_range("Invalid subslice range");
        if(!count)
            count = this->size() - pos;
        return slice(this->begin() + pos, this->begin() + pos + count);
    }

    template <typename Func>
    void each(Func func) {
        using Iterator = decltype(std::begin(*this));
        using Value = typename std::iterator_traits<Iterator>::value_type;
        static_assert(std::is_invocable_v<Func, Value>, "Func must be callable");
        for(auto& element : *this)
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
        if(pos + count > this->size()) throw std::out_of_range("Invalid slice range");
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

template<typename T, std::size_t Offset = 0>
class span {
public:
    using size_type = std::size_t;
    using value_type = T;

    constexpr span() = default;

    constexpr span(T* ptr, size_type size) noexcept :
    ptr_(ptr), size_(size) {}

    constexpr span(void *ptr, size_type size) noexcept :
    ptr_(reinterpret_cast<T *>(ptr)), size_(size) {}

    template<size_type S>
    constexpr span(T(&arr)[S]) noexcept : span(arr, S) {} // cppcheck-suppress noExplicitConstructor

    template <typename Container, typename = std::enable_if_t<std::is_same_v<T, typename Container::value_type>>>
    span(Container& container) : span(container.data(), container.size()) {} // cppcheck-suppress noExplicitConstructor

    explicit operator bool() {
        return !empty();
    }

    auto operator!() {
        return empty();
    }

    constexpr auto operator[](size_type index) const -> T& {
        if(index < Offset || index >= Offset + size_) throw std::out_of_range("Span index past end");
        return ptr_[index - Offset];
    }

    constexpr auto at(size_type index) const -> T& {
        if(index < Offset || index >= Offset + size_) throw std::out_of_range("Span index past end");
        return ptr_[index - Offset];
    }

    constexpr auto get(size_type index) const noexcept -> std::optional<T>  {
        if(index < Offset || index >= Offset + size_) return std::nullopt;
        return ptr_[index - Offset];
    }

    constexpr auto get_or(size_type index, const T* or_else = nullptr) const noexcept -> const T* {
        if(index < Offset || index >= Offset + size_) return or_else;
        return ptr_ + (index - Offset);
    }

    constexpr auto get_or(size_type index, T* or_else = nullptr) noexcept -> T* {
        if(index < Offset || index >= Offset + size_) return or_else;
        return ptr_ + (index - Offset);
    }

    constexpr auto size_bytes() const noexcept {
        return size_ * sizeof(T);
    }

    constexpr auto size() const noexcept {
        return size_;
    }

    constexpr auto min() const noexcept {
        return Offset;
    }

    constexpr auto max() const noexcept {
        return size_type(Offset + size_ - 1);
    }

    constexpr auto front() const noexcept -> T& {
        return ptr_[0];
    }

    constexpr auto back() const noexcept -> T& {
        return ptr_[size_ - 1];
    }

    constexpr auto empty() const noexcept {
        return size_ == 0;
    }

    auto data() const noexcept {
        return ptr_;
    }

    auto begin() const noexcept {
        return ptr_;
    }

    auto end() const noexcept {
        return ptr_ + size_;
    }

    auto subspan(size_type pos, size_type count = 0) const {
        if(pos < Offset || (pos + count - Offset) > size_) throw std::out_of_range("Invalid subspan range");
        return span(ptr_ + (pos - Offset), count ? count : size_ - pos - Offset);
    }

    auto find(const T& value) const {
        return std::find(this->begin(), this->end(), value);
    }

    auto contains(const T& value) const {
        return std::find(this->begin(), this->end(), value) != this->end();
    }

private:
    T *ptr_{nullptr};
    size_type size_{0};
};

using byteslice_t = slice<uint8_t>;
using charslice_t = slice<char>;

template<typename T, std::size_t S>
constexpr auto make_span(T(&arr)[S]) {
        return span<T>(arr);
}

template<typename Container>
inline auto make_span(Container& container) -> span<typename Container::value_type> {
    return span<typename Container::value_type>(container);
}
} // end namespace

namespace std {
template<>
struct hash<tycho::byteslice_t> {
    auto operator()(const tycho::byteslice_t& obj) const {
        std::size_t result{0U};
        const auto data = obj.data();
        for(std::size_t pos = 0; pos < obj.size(); ++pos) {
            result = (result * 131) + data[pos];
        }
        return result;
    }
};
} // end namespace

auto inline operator==(const tycho::byteslice_t& lhs, const tycho::byteslice_t& rhs) {
    if(lhs.size() != rhs.size()) return false;
    if(lhs.data() == rhs.data()) return true;
    return memcmp(lhs.data(), rhs.data(), lhs.size()) == 0;
}

auto inline operator!=(const tycho::byteslice_t& lhs, const tycho::byteslice_t& rhs) {
    if(lhs.size() != rhs.size()) return true;
    if(lhs.data() == rhs.data()) return false;
    return memcmp(lhs.data(), rhs.data(), lhs.size()) != 0;
}
#endif
