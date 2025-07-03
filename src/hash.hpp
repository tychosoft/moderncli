// Copyright (C) 2023 Tycho Softworks.
// This code is licensed under MIT license.

#ifndef TYCHO_HASH_HPP_
#define TYCHO_HASH_HPP_

#include "digest.hpp"

#include <map>
#include <iterator>
#include <exception>
#include <algorithm>
#include <utility>
#include <shared_mutex>
#include <mutex>
#include <string>

namespace tycho::crypto {
inline constexpr const EVP_MD* (*SHA256)()      = EVP_sha256;
inline constexpr const EVP_MD* (*SHA512)()      = EVP_sha512;
inline constexpr const EVP_MD* (*SHA2_256)()    = EVP_sha256;
inline constexpr const EVP_MD* (*SHA2_512)()    = EVP_sha512;
inline constexpr const EVP_MD* (*SHA3_256)()    = EVP_sha3_256;
inline constexpr const EVP_MD* (*SHA3_512)()    = EVP_sha3_512;

template <typename T, const EVP_MD* (*Algo)() = SHA256>
struct hash_t final {
    static_assert((std::is_convertible_v<decltype(std::declval<const T&>().data()), const char*> || std::is_convertible_v<decltype(std::declval<const T&>().data()), const uint8_t*>) && std::is_convertible_v<decltype(std::declval<const T&>().size()), std::size_t>,
        "T must have data() convertible to const char* or const uint8_t* and size() convertible to std::size_t"
    );

    // let's us use this as a std::hash too...
    auto operator()(const T& key) const -> std::size_t {
        return std::size_t(to_u64(key));
    }

    template<uint64_t B = 16>
    auto to_bits(const T& key) const -> uint64_t {
        static_assert(B >= 1 && B <= 64, "B must be [1, 64]");
        return to_64(key) & ((1ULL << B) - 1);
    }

    auto to_u32(const T& key) const -> uint32_t {
        return static_cast<uint32_t>(to_u64(key) & 0xffffffffUL);
    }

    auto to_u64(const T& key) const -> uint64_t {
        const auto size = digest_size(Algo());
        if(size == 0) return 0;

        alignas(std::max_align_t) uint8_t buf[EVP_MAX_MD_SIZE]{};
        auto count = digest(key, buf, Algo());
        if(count < sizeof(uint64_t)) throw std::out_of_range("Consistent digest too small");

        // big endian byte order, consistent value
        uint64_t result{0};
        for(std::size_t i = 0; i < sizeof(result); ++i) {
            result = (result << 8) | buf[i];
        }
        return result;
    }
};

template<typename Key, const EVP_MD* (*Algo)() = SHA256>
class hash64_ring {
public:
    using Hash = hash_t<std::string, Algo>;

    explicit hash64_ring(int vnodes = 100) : vnodes_(vnodes) {}

    hash64_ring(std::initializer_list<std::string> nodes, int vnodes = 100) :
    vnodes_(vnodes) {
        for (const auto& node : nodes) {
            for(auto i = 0; i < vnodes_; ++i) {
                std::string vnode = node + "#" + std::to_string(i);
                ring_.emplace(hash_.to_u64(vnode), node);
            }
        }
    }

    operator bool() const {
        return !empty();
    }

    auto operator!() const {
        return empty();
    }

    auto operator*() const {
        return get();
    }

    auto operator+=(const std::string& node) -> auto& {
        insert(node);
        return *this;
    }

    auto operator-=(const std::string& node) -> auto& {
        remove(node);
        return *this;
    }

    auto empty() const -> bool {
        const std::shared_lock lock(mutex_);
        return ring_.empty();
    }

    auto size() const {
        const std::shared_lock lock(mutex_);
        return ring_.size() / vnodes_;
    }

    void insert(const std::string& node) {
        const std::unique_lock lock(mutex_);
        for(auto i = 0; i < vnodes_; ++i) {
            std::string vnode = node + "#" + std::to_string(i);
            ring_.emplace(hash_.to_u64(vnode), node);
        }
    }

    void remove(const std::string& node) {
        const std::unique_lock lock(mutex_);
        for(auto i = 0; i < vnodes_; ++i) {
            std::string vnode = node + "#" + std::to_string(i);
            ring_.erase(hash_.to_u64(vnode));
        }
    }

    auto get(const Key& key) const -> const std::string& {
        const std::shared_lock lock(mutex_);
        auto hash = hash_.to_u64(to_string(key));
        auto it = ring_.lower_bound(hash);
        if(it == ring_.end())
            it = ring_.begin();
        return it->second;
    }

private:
    static_assert(std::is_convertible_v<Key, std::string>, "Key must be convertible to std::string.");

    auto to_string(const Key& key) const -> std::string {
        return key;
    }

    mutable std::shared_mutex mutex_;
    std::map<uint64_t, std::string> ring_;
    int vnodes_;
    Hash hash_;
};

// customary conventions of porable version
template<const EVP_MD* (*Algo)() = SHA256>
using ring64 = hash64_ring<std::string, Algo>;
} // end namespace
#endif
