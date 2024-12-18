// Copyright (C) 2024 Tycho Softworks.
// This code is licensed under MIT license.

#ifndef TYCHO_MEMORY_HPP_
#define TYCHO_MEMORY_HPP_

#include "encoding.hpp"

#include <type_traits>
#include <stdexcept>
#include <memory>
#include <iostream>
#include <string_view>
#include <utility>
#include <cstring>
#include <cstdint>
#include <cstddef>
#include <cstdlib>
#include <climits>

#if __has_include(<unistd.h>)
#include <unistd.h>
#endif

namespace tycho {
namespace crypto {
using key_t = std::pair<const uint8_t *, std::size_t>;
} // end namespace

template<typename T>
class bytes_array final {
public:
    bytes_array() = default;
    bytes_array(const bytes_array& other) = default;

    explicit bytes_array(std::size_t size) :
    array_(std::make_shared<T[]>(size)), size_(size) {}

    template <typename U = T, std::enable_if_t<sizeof(U) == 1, int> = 0>
    explicit bytes_array(const crypto::key_t& key) :
    array_(std::make_shared<T[]>(key.second)), size_(key.second) {
        memcpy(array_.get(), key.first, size_); // FlawFinder: ignore
    }

    bytes_array(const T* from, std::size_t size) :
    array_(std::make_shared<T[]>(size)), size_(size) {
        memcpy(array_.get(), from, sizeof(T) * size);   // FlawFinder: ignore
    }

    bytes_array(bytes_array&& other) noexcept :
    array_(std::move(other.array_)), size_(other.size_) {
        other.size_ = 0;
    }

    operator crypto::key_t() const noexcept {
        return key();
    }

    operator bool() const noexcept {
        return size_ > 0;
    }

    operator std::string() const {
        return to_hex();
    }

    auto operator!() const noexcept {
        return size_ == 0;
    }

    auto operator=(bytes_array&& other) noexcept -> auto& {
        if(this != &other) {
            array_ = std::move(other.array_);
            size_ = other.size_;
            other.size_ = 0;
        }
        return *this;
    }

    auto operator[](std::size_t index) -> T& {
        if(index >= size_)
            throw std::out_of_range("Index is out of range");
        return array_[index];
    }

    auto operator[](std::size_t index) const -> const T& {
        if(index >= size_)
            throw std::out_of_range("Index is out of range");
        return array_[index];
    }

    auto operator *() noexcept -> uint8_t * {
        return reinterpret_cast<uint8_t>(array_.get());
    }

    auto operator *() const noexcept -> const uint8_t * {
        return reinterpret_cast<uint8_t>(array_.get());
    }

    auto operator()() {
        if(!size_)
            throw std::out_of_range("Cannot return empty object");
        return *array_;
    }

    auto operator()() const {
        if(!size_)
            throw std::out_of_range("Cannot return empty object");
        return *array_;
    }

    auto get() {
        if(!size_)
            throw std::out_of_range("Cannot return empty object");
        return array_.get();
    }

    auto get() const {
        if(!size_)
            throw std::out_of_range("Cannot return empty object");
        return array_.get();
    }

    auto key() const -> crypto::key_t {
        return std::make_pair(reinterpret_cast<uint8_t*>(array_.get()), size_);
    }

    auto empty() const noexcept {
        return size_ == 0;
    }

    auto size() const noexcept {
        sizeof(T) * size_;
    }

    auto view() const {
        return size_ ? std::string_view(reinterpret_cast<const char *>(&array_[0]), size_) : std::string_view();
    }

    auto count() const {
        return array_.use_count();
    }

    auto hash() const {
        return std::hash(array_);
    }

    auto zero() noexcept {
        if(size_)
            memset(array_, 0, sizeof(T) * size_);
    }

    auto to_hex() const {
        return to_hex(get(), size());
    }

    auto to_b64() const {
        return to_b64(get(), size());
    }

    static auto from_hex(std::string_view from) {
        auto bsize = from.size() / 2;
        while(sizeof(T) > 1 && bsize % sizeof(T))
            ++bsize;
        auto mem = bytes_array(bsize / sizeof(T));
        if(tycho::from_hex(from, mem.get(), bsize) < from.size() / 2)
            return bytes_array();
        return mem;
    }

    static auto from_b64(std::string_view from) {
        auto bsize = size_b64(from);
        auto alloc = bsize;
        while(sizeof(T) > 1 && alloc % sizeof(T))
            ++alloc;
        auto mem = bytes_array(alloc / sizeof(T));
        if(tycho::from_b64(from, mem.get(), bsize) < bsize)
            return bytes_array();
        return mem;
    }

private:
    static_assert(std::is_trivial_v<T>, "T must be Trivial type");

    std::shared_ptr<T> array_;
    std::size_t size_{0};
};

class imemstream : protected std::streambuf, public std::istream {
public:
    imemstream() = delete;
    imemstream(const imemstream&) = delete;
    auto operator=(const imemstream&) -> auto& = delete;

    imemstream(const uint8_t *data, std::size_t size) :
    std::istream(static_cast<std::streambuf *>(this)), pos(data), count(size) {}

    explicit imemstream(const char *cp) :
    std::istream(static_cast<std::streambuf *>(this)), pos(reinterpret_cast<const uint8_t *>(cp)), count(::strlen(cp)) {} // FlawFinder: ignore

    auto size() const noexcept {
        return count;
    }

    auto get() const noexcept {
        return pos;
    }

    auto c_char() const noexcept {
        return reinterpret_cast<const char *>(pos);
    }

protected:
    const uint8_t *pos{nullptr};
    std::size_t count{0};

    auto underflow() -> int override {
        if(!count || !pos)
            return EOF;
        return *pos;
    }

    auto uflow() -> int override {
        if(!count || !pos)
            return EOF;
        --count;
        return *(pos++);
    }
};

class omemstream : protected std::streambuf, public std::ostream {
public:
    omemstream() = delete;
    omemstream(const omemstream&) = delete;
    auto operator=(const omemstream&) -> auto& = delete;

    omemstream(uint8_t *data, std::size_t size, bool flag = false) :
    std::ostream(static_cast<std::streambuf *>(this)), base(data), limit(size), zero(flag) {
        if(flag && size) {
            *base = 0;
            ++count;
        }
    }

    auto size() const noexcept {
        return count;
    }

    auto get() const noexcept {
        return base;
    }

    auto c_char() const noexcept {
        return reinterpret_cast<const char *>(base);
    }

protected:
    uint8_t *base{nullptr};
    std::size_t count{0}, limit{0};
    bool zero{false};

    auto overflow(int ch) -> int override {
        if(ch == EOF || count >= limit || !base)
            return EOF;

        base[count++] = static_cast<uint8_t>(ch);
        if(zero)
            base[count] = 0;
        return ch;
    }
};

class mempager {
public:
    mempager(const mempager&) = delete;
    auto operator=(const mempager&) -> auto& = delete;

    explicit mempager(std::size_t size = 0) noexcept :
    size_(size), align_(aligned_page()) {
        if(size_ <= align_)
            align_ = aligned_cache();
    }

    mempager(mempager&& move) noexcept :
    size_(move.size_), align_(move.align_), count_(move.count_), current_(move.current_) {
        move.current_ = nullptr;
        move.count_ = 0;
    }

    virtual ~mempager() {
        clear();
    }

    auto operator=(mempager&& move) noexcept -> auto& {
        if(&move == this)
            return *this;

        size_ = move.size_;
        align_ = move.align_;
        count_ = move.count_;
        current_ = move.current_;
        move.current_ = nullptr;
        move.count_ = 0;
        return *this;
    }

    template<typename T>
    auto make(std::size_t adjust = 0) {
        return static_cast<T*>(alloc(sizeof(T) + adjust));
    }

    auto alloc(std::size_t size) -> void * {
        while(size % sizeof(void *))
            ++size;

        if(size > (size_ - sizeof(page_ptr)))
            return nullptr;

        if(!current_ || size > size_ - current_->used) {
            page_ptr page{nullptr};
            page_alloc(&page, size_, align_);
            if(!page)
                return nullptr;

            page->aligned = nullptr;    // To make dumb checkers happy
            page->used = sizeof(page_t);
            page->next = current_;
            ++count_;
            current_ = page;
        }

        uint8_t *mem = (reinterpret_cast<uint8_t *>(current_)) + current_->used;
        current_->used += static_cast<unsigned>(size);
        return mem;
    }

    auto dup(const std::string_view& str) -> char * {
        auto len = str.size();
        auto mem = static_cast<char *>(alloc(len + 1));
        if(mem) {
            memcpy(mem, str.data(), len); // FlawFinder: ignore
            mem[len] = 0;
        }
        return mem;
    }

    void clear() noexcept {
        page_ptr next{};
        while(current_) {
            next = current_->next;
            ::free(current_); // NOLINT
            current_ = next;
        }
        count_ = 0;
    }

    auto empty() const noexcept {
        return count_ == 0;
    }

    auto pages() const noexcept {
        return count_;
    }

    auto size() const noexcept {
        return count_ * size_;
    }

    auto used() const noexcept {
        return size();
    }

    static constexpr auto aligned_size(std::size_t size) -> std::size_t {
        std::size_t align = 1;
        while(align < size)
            align <<= 1;
        return align;
    }

    static auto aligned_page(std::size_t min = 0) -> std::size_t{
#if defined(_SC_PAGESIZE)
        std::size_t asize = aligned_size(static_cast<std::size_t>(sysconf(_SC_PAGESIZE)));
#elif defined(PAGESIZE)
        std::size_t asize = aligned_size(PAGESIZE);
#elif defined(PAGE_SIZE)
        std::size_t asize = aligned_size(PAGE_SIZE);
#else
        std::size_t asize = 1024;
#endif
        min = aligned_size(min);
        while(asize < min)
            asize <<= 2;
        return asize;
    }

    static auto aligned_cache() -> std::size_t {
        static volatile std::size_t line_size = 0;
#if defined(_SC_LEVEL1_DCHACHE_LINESIZE)
        line_size = sysconf(_SC_LEVEL1_DCACHE_LINESIZE);
#endif
        if(line_size < 64)
            line_size = 64;
        return aligned_size(line_size);
    }

protected:
    std::size_t size_{0}, align_{0};
    unsigned count_{0};

private:
    using page_ptr = struct page_t {
        page_t *next;
        union {
            [[maybe_unused]] void *aligned;
            unsigned used;
        };
    } *;

    page_ptr current_{nullptr};

    static void page_alloc(page_ptr *mem, std::size_t size, std::size_t align = 0) {
#if defined(_MSC_VER) || defined(__MINGW32__) || defined(__MINGW64__) || defined(WIN32)
        *mem = static_cast<page_ptr>(::malloc(size));  // NOLINT
#else
        if(!align)
            *mem = static_cast<page_ptr>(::malloc(size));  // NOLINT
        else
#ifdef  __clang__
            posix_memalign(reinterpret_cast<void **>(mem), align, size);
#else
            *mem = static_cast<page_ptr>(::aligned_alloc(align, size));
#endif
#endif
    }
};

using bytearray_t = bytes_array<std::byte>;
using chararray_t = bytes_array<char>;
using wordarray_t = bytes_array<uint16_t>;
using longarray_t = bytes_array<uint32_t>;
using mempager_t = mempager;

template<typename T>
inline void mem_alloc(T **mem, std::size_t size, std::size_t align = 0) {
    if(!mem)
        throw std::runtime_error("memory handle is null");
    if(*mem) {
        ::free(*mem);   // NOLINT
        *mem = nullptr;
    }
#if defined(_MSC_VER) || defined(__MINGW32__) || defined(__MINGW64__) || defined(WIN32)
    *mem = static_cast<T*>(::malloc(size));  // NOLINT
#else
    if(!align)
        *mem = static_cast<T *>(::malloc(size));  // NOLINT
    else
#ifdef  __clang__
        posix_memalign(reinterpret_cast<void **>(mem), align, size);
#else
        *mem = static_cast<T *>(::aligned_alloc(align, size));
#endif
#endif
}

template<typename T>
inline void mem_free(T **mem) {
    if(!mem)
        throw std::runtime_error("memory handle is null");

    if(*mem) {
        ::free(*mem);   // NOLINT
        *mem = nullptr;
    }
}

inline auto mem_index(const uint8_t *mem, std::size_t size) -> unsigned {
    if(!mem)
        throw std::runtime_error("memory index is null");

    unsigned val = 0;
    while(size--)
        val = (val << 1) ^ (*(mem++) & 0x1f);

    return val;
}

inline auto mem_size(const char *cp, std::size_t max = 128) -> std::size_t {
    if(!cp)
        throw std::runtime_error("memory size for null");

    std::size_t count = 0;
    while(cp && *cp && count++ <= max)
        ++cp;

    if(count > max)
        throw std::runtime_error("memory size too large");

    return count;
}

inline auto mem_append(char *dest, std::size_t max, const char *value) -> std::size_t {
    if(!dest || !value || !max)
        throw std::runtime_error("memory append invalid");

    while(max && *dest) {
        ++dest;
        --max;
    }
    if(!max)
        throw std::runtime_error("memory append size invalid");

    --max; // null byte at end...

    std::size_t pos = 0;
    while(*value && pos < max) {
        *(dest++) = *(value++);
        ++pos;
    }
    *dest = 0;
    return pos;
}

inline auto mem_view(char *target, std::size_t size, std::string_view s) -> bool {
    if(!target)
        throw std::runtime_error("memory view target invalid");

    auto count = s.size();
    auto from = s.data();

    if(count >= size)
        return false;

    while(count--)
        *(target++) = *(from++);
    *target = 0;
    return true;
}

inline auto mem_copy(char *target, std::size_t size, const char *from) -> bool {
    if(!from || !target)
        throw std::runtime_error("memory view target invalid");

    auto count = strnlen(from, size);
    if(count == size)
        return false;

    while(count--)
        *(target++) = *(from++);
    *target = 0;
    return true;
}

inline auto mem_value(char *target, std::size_t size, unsigned value) -> bool {
    if(!target)
        throw std::runtime_error("memory view target invalid");

    auto max = 1U;
    auto zero = false;

    while(--size)
        max *= 10;

    if(value >= max)
        return false;

    while(max) {
        auto dig = 0U;
        max /= 10;
        if(max) {
            dig = value / max;
            value %= max;
        }

        if(dig || zero) {
            *(target++) = static_cast<char>('0' + dig);
            zero = true;
        }
        if(max == 1)
            break;
    }
    *target = 0;
    return true;
}

inline auto mem_index(std::string_view s) {
    return mem_index(reinterpret_cast<const uint8_t *>(s.data()), s.size());
}

inline auto key_index(const char *cp, std::size_t max) {
    return mem_index(reinterpret_cast<const uint8_t *>(cp), mem_size(cp, max));
}
} // end namespace

inline auto operator new(std::size_t size, tycho::mempager& pager) -> void * {
    return pager.alloc(size);
}

inline void operator delete([[maybe_unused]] void *page, [[maybe_unused]] tycho::mempager& pager) {}

template <typename T>
inline auto operator<<(std::ostream& out, const tycho::bytes_array<T>& bin) -> std::ostream& {
    static_assert(std::is_trivial_v<T>, "T must be Trivial type");
    out << bin.to_hex();
    return out;
}
#endif
