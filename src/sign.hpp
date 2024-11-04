// Copyright (C) 2023 Tycho Softworks.
// This code is licensed under MIT license.

#ifndef TYCHO_SIGN_HPP_
#define TYCHO_SIGN_HPP_

#include <string>
#include <string_view>
#include <openssl/evp.h>
#include <openssl/pem.h>

namespace cipher {
class pubkey_t final {
public:
    explicit pubkey_t(const std::string& pem) noexcept {
        auto bio = BIO_new_mem_buf(pem.c_str(), -1);
        if(bio) {
            key_ = PEM_read_bio_PUBKEY(bio, nullptr, nullptr, nullptr);
            BIO_free(bio);
        }
    }

    explicit pubkey_t(X509 *cert) noexcept {
        if(cert)
            key_ = X509_get_pubkey(cert);
        X509_free(cert);
    }

    pubkey_t(const pubkey_t& other) noexcept :
    key_(other.key_) {
        EVP_PKEY_up_ref(key_);
    }

    ~pubkey_t() {
        if(key_)
            EVP_PKEY_free(key_);
    }

    operator bool() const noexcept {
        return key_ != nullptr;
    }

    auto operator!() const noexcept {
        return !key_;
    }

    auto operator=(const pubkey_t& other) noexcept -> auto& {
        if(&other == this)
            return *this;
        if(key_ == other.key_)
            return *this;
        if(key_)
            EVP_PKEY_free(key_);
        key_ = other.key_;
        EVP_PKEY_up_ref(key_);
        return *this;
    }

    auto key() const noexcept {
        return key_;
    }

    auto share() const noexcept {
        if(key_)
           EVP_PKEY_up_ref(key_);
        return key_;
    }

private:
    EVP_PKEY *key_{nullptr};
};

class sign_t final {
public:
    explicit sign_t(EVP_PKEY *key, const EVP_MD *md = EVP_sha256()) noexcept :
    key_(key) {
        if(key_) {
            EVP_PKEY_up_ref(key_);
            ctx_ = EVP_MD_CTX_new();
        }
        if(!ctx_)
            return;

        EVP_DigestSignInit(ctx_, nullptr, md, nullptr, key_);
    }

    sign_t(sign_t&& other) noexcept {
        if(this == &other)
            return;
        ctx_ = other.ctx_;
        key_ = other.key_;
        other.key_ = nullptr;
        other.ctx_ = nullptr;
    }

    sign_t(const sign_t&) = delete;
    auto operator=(const sign_t&) -> auto& = delete;

    ~sign_t() {
        if(ctx_)
            EVP_MD_CTX_free(ctx_);
        if(key_)
            EVP_PKEY_free(key_);
    }

    operator bool() const noexcept {
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
        return std::string_view(reinterpret_cast<const char *>(data_), size_);
    }

    auto update(const uint8_t *data, std::size_t size) noexcept {
        return !ctx_ ? false : EVP_DigestSignUpdate(ctx_, data, size) > 0;
    }

    auto update(const std::string_view& view) noexcept {
        return update(reinterpret_cast<const uint8_t *>(view.data()), view.size());
    }

    auto finish() noexcept {
        if(!ctx_ || size_ > 0)
            return false;
        return EVP_DigestSignFinal(ctx_, data_, &size_) > 0;
    }

private:
    EVP_MD_CTX *ctx_{nullptr};
    EVP_PKEY *key_{nullptr};
    std::size_t size_{0};
    uint8_t data_[EVP_MAX_MD_SIZE]{};
};

class verify_t final {
public:
    explicit verify_t(EVP_PKEY *key, const EVP_MD *md = EVP_sha256()) noexcept : key_(key) {
        if(key_) {
            EVP_PKEY_up_ref(key_);
            ctx_ = EVP_MD_CTX_new();
        }
        if(!ctx_)
            return;

        EVP_DigestVerifyInit(ctx_, nullptr, md, nullptr, key_);
    }

    verify_t(verify_t&& other) noexcept {
        if(this == &other)
            return;
        ctx_ = other.ctx_;
        key_ = other.key_;
        other.key_ = nullptr;
        other.ctx_ = nullptr;
    }

    verify_t(const verify_t&) = delete;
    auto operator=(const verify_t&) -> auto& = delete;

    ~verify_t() {
        if(ctx_)
            EVP_MD_CTX_free(ctx_);
        if(key_)
             EVP_PKEY_free(key_);
    }

    operator bool() const noexcept {
        return ctx_ != nullptr;
    }

    auto operator!() const noexcept {
        return ctx_ == nullptr;
    }

    auto update(const uint8_t *data, std::size_t size) noexcept {
        return !ctx_ ? false : EVP_DigestVerifyUpdate(ctx_, data, size) > 0;
    }

    auto update(const std::string_view& view) noexcept {
        return update(reinterpret_cast<const uint8_t *>(view.data()), view.size());
    }

    auto finish(const uint8_t *sign, std::size_t size) noexcept {
        return !ctx_ ? false : EVP_DigestVerifyFinal(ctx_, sign, size) == 1;
    }

private:
    EVP_MD_CTX *ctx_{nullptr};
    EVP_PKEY *key_{nullptr};
};

inline auto sign_X509(const std::string& path) -> X509 * {
    auto fp = fopen(path.c_str(), "r");
    if(!fp)
        return nullptr;
    auto bp = BIO_new(BIO_s_file());
    BIO_set_fp(bp, fp, BIO_NOCLOSE);
    auto cert = PEM_read_bio_X509(bp, nullptr, nullptr, nullptr);
    BIO_free(bp);
    fclose(fp);
    return cert;
}
} // end namespace
#endif
