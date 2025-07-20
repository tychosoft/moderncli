// Copyright (C) 2023 Tycho Softworks.
// This code is licensed under MIT license.

#ifndef TYCHO_DIGEST_HPP_
#define TYCHO_DIGEST_HPP_

#include <string_view>
#include <type_traits>
#include <cstring>
#include <cstddef>
#include <openssl/evp.h>
#include <openssl/hmac.h>

namespace tycho::crypto {
class digest_t final {
public:
    // cppcheck-suppress noExplicitConstructor
    digest_t(const EVP_MD *md = EVP_sha256()) noexcept : ctx_(EVP_MD_CTX_create()) {
        if(ctx_ && EVP_DigestInit_ex(ctx_, md, nullptr) != 1) {
            EVP_MD_CTX_destroy(ctx_);
            ctx_ = nullptr;
        }
    }

    explicit digest_t(const char *cp) noexcept : ctx_(EVP_MD_CTX_create()) {
        if(ctx_ && EVP_DigestInit_ex(ctx_, EVP_get_digestbyname(cp), nullptr) != 1) {
            EVP_MD_CTX_destroy(ctx_);
            ctx_ = nullptr;
        }
    }

    digest_t(const digest_t& from) noexcept {
        if(from.ctx_)
            ctx_ = EVP_MD_CTX_create();
        if(ctx_ && EVP_MD_CTX_copy_ex(ctx_, from.ctx_) != 1) {
            EVP_MD_CTX_destroy(ctx_);
            ctx_ = nullptr;
        }
        if(ctx_)
            size_ = from.size_;
        if(size_)
            memcpy(data_, from.data_, size_);
    }

    digest_t(digest_t&& from) noexcept : ctx_(from.ctx_), size_(from.size_) {
        if(size_)
            memcpy(data_, from.data_, size_);
        from.ctx_ = nullptr;
        from.size_ = 0;
    }

    ~digest_t() {
        if(ctx_)
            EVP_MD_CTX_destroy(ctx_);
    }

    auto operator=(const digest_t& from) noexcept -> auto& {
        if(this == &from) return *this;
        if(ctx_) {
            EVP_MD_CTX_destroy(ctx_);
            ctx_ = nullptr;
        }
        if(from.ctx_)
            ctx_ = EVP_MD_CTX_create();
        if(ctx_ && EVP_MD_CTX_copy_ex(ctx_, from.ctx_) != 1) {
            EVP_MD_CTX_destroy(ctx_);
            ctx_ = nullptr;
        }
        if(ctx_)
            size_ = from.size_;
        if(size_)
            memcpy(data_, from.data_, size_);
        return *this;
    }

    explicit operator bool() const noexcept {
        return ctx_ != nullptr;
    }

    auto operator!() const noexcept {
        return ctx_ == nullptr;
    }

    auto size() const noexcept {
        return size_;
    }

    auto data() const noexcept {
        return data_;
    }

    auto view() const {
        return std::string_view(reinterpret_cast<const char *>(&data_), size_);
    }

    auto c_str() const noexcept {
        return reinterpret_cast<const char *>(data_);
    }

    auto update(const char *cp, std::size_t size) noexcept {
        return !ctx_ || size_ ? false : EVP_DigestUpdate(ctx_, reinterpret_cast<uint8_t *>(const_cast<char *>(cp)), size) == 1;
    }

    auto update(const uint8_t *cp, std::size_t size) noexcept {
        return !ctx_ || size_ ? false : EVP_DigestUpdate(ctx_, cp, size) == 1;
    }

    auto update(const std::string_view& view) noexcept {
        update(view.data(), view.size());
    }

    auto finish() noexcept {
        if(!ctx_ || size_) return false;
        return EVP_DigestFinal_ex(ctx_, data_, &size_) == 1;
    }

    void reinit() noexcept {
        if(ctx_)
            EVP_DigestInit_ex(ctx_, nullptr, nullptr);
        size_ = 0;
    }

private:
    EVP_MD_CTX *ctx_{nullptr};
    unsigned size_{0};
    uint8_t data_[EVP_MAX_MD_SIZE]{};
};

#if OPENSSL_API_LEVEL >= 30000
inline auto hmac(const std::string_view& key, const uint8_t *msg, std::size_t size, uint8_t *out, const EVP_MD *md = EVP_sha256()) {
    unsigned olen{0};
    if(!HMAC(md, key.data(), int(key.size()), msg, size, out, &olen))
        olen = 0;
    return std::size_t(olen);
}
#else
inline auto hmac(const std::string_view& key, const uint8_t *msg, std::size_t size, uint8_t *out, const EVP_MD *md = EVP_sha256()) {
    unsigned olen{0};
    auto ctx = HMAC_CTX_new();
    if(!ctx) return std::size_t(0);
    if(!HMAC_Init_ex(ctx, key.data(), int(key.size()), md, nullptr)) {
        HMAC_CTX_free(ctx);
        return std::size_t(0);
    }

    HMAC_Update(ctx, msg, size);
    HMAC_Final(ctx, out, &olen);
    HMAC_CTX_free(ctx);
    return std::size_t(olen);
}
#endif

inline auto hmac(const std::string_view& key, const std::string_view& msg, uint8_t *out, const EVP_MD *md = EVP_sha256()) {
    return hmac(key, reinterpret_cast<const uint8_t *>(msg.data()), msg.size(), out, md);
}

inline auto digest(const uint8_t *msg, std::size_t size, uint8_t *out, const EVP_MD *md = EVP_sha256()) {
    unsigned olen{0};
    auto ctx = EVP_MD_CTX_create();
    if(!ctx) return std::size_t(0);
    if(!EVP_DigestInit_ex(ctx, md, nullptr)) {
        EVP_MD_CTX_destroy(ctx);
        return std::size_t(0);
    }

    EVP_DigestUpdate(ctx, msg, size);
    EVP_DigestFinal_ex(ctx, out, &olen);
    EVP_MD_CTX_destroy(ctx);
    return std::size_t(olen);
}

template<typename T,
typename = std::enable_if_t<(
std::is_convertible_v<decltype(std::declval<const T&>().data()), const char*> || std::is_convertible_v<decltype(std::declval<const T&>().data()), const uint8_t*>
) && std::is_convertible_v<decltype(std::declval<const T&>().size()), std::size_t>>>
inline auto digest(const T& msg, uint8_t *out, const EVP_MD *md = EVP_sha256()) {
    return digest(reinterpret_cast<const uint8_t *>(msg.data()), msg.size(), out, md);
}

inline auto digest_size(const EVP_MD *md = EVP_sha256()) {
    auto sz = EVP_MD_get_size(md);
    if(sz < 1) return std::size_t(0);
    return std::size_t(sz);
}

inline auto digest_id(const char *name) {
    return EVP_get_digestbyname(name);
}

inline auto digest_name(const EVP_MD *md = EVP_sha256()) {
    return EVP_MD_get0_name(md);
}
} // end namespace
#endif
