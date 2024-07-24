// Copyright (C) 2023 Tycho Softworks.
// This code is licensed under MIT license.

#ifndef TYCHO_CIPHER_HPP_
#define TYCHO_CIPHER_HPP_

#include <random.hpp>
#include <openssl/evp.h>

namespace crypto {
using salt_t = random_t<salt>;

class keyphrase_t final : public Key {
public:
    keyphrase_t() = default;

    keyphrase_t(const salt_t& salt, const std::string_view& phrase, const EVP_CIPHER *algo = EVP_aes_256_cbc(), const EVP_MD *md = EVP_sha256(), int rounds = 1) noexcept :
    cipher_(algo), size_(EVP_CIPHER_key_length(algo)) {
        auto kp = reinterpret_cast<const uint8_t *>(phrase.data());
        auto ks = static_cast<size_t>(EVP_BytesToKey(algo, md, salt.data(), kp, static_cast<int>(phrase.size()), rounds, data_, iv_));
        if(ks < size_)
            size_ = 0;
        else
            size_ = ks;
    }

    explicit keyphrase_t(const std::string_view& phrase, const EVP_CIPHER *algo = EVP_aes_256_cbc(), const EVP_MD *md = EVP_sha256(), int rounds = 1) noexcept :
    cipher_(algo), size_(EVP_CIPHER_key_length(algo)) {
        auto kp = reinterpret_cast<const uint8_t *>(phrase.data());
        auto ks = static_cast<size_t>(EVP_BytesToKey(algo, md, nullptr, kp, static_cast<int>(phrase.size()), rounds, data_, iv_));
        if(ks < size_)
            size_ = 0;
        else
            size_ = ks;
    }

    keyphrase_t(const salt_t& salt, const Key& key, const EVP_CIPHER *algo = EVP_aes_256_cbc(), const EVP_MD *md = EVP_sha256(), int rounds = 1) noexcept :
    cipher_(algo), size_(EVP_CIPHER_key_length(algo)) {
        auto ks = static_cast<size_t>(EVP_BytesToKey(algo, md, salt.data(), key.data(), static_cast<int>(key.size()), rounds, data_, iv_));
        if(ks < size_)
            size_ = 0;
        else
            size_ = ks;
    }

    explicit keyphrase_t(const Key& key, const EVP_CIPHER *algo = EVP_aes_256_cbc(), const EVP_MD *md = EVP_sha256(), int rounds = 1) noexcept :
    cipher_(algo), size_(EVP_CIPHER_key_length(algo)) {
        auto ks = static_cast<size_t>(EVP_BytesToKey(algo, md, nullptr, key.data(), static_cast<int>(key.size()), rounds, data_, iv_));
        if(ks < size_)
            size_ = 0;
        else
            size_ = ks;
    }

    keyphrase_t(const keyphrase_t& other) noexcept :
    cipher_(other.cipher_), size_(other.size_) {
        if(size_) {
            memcpy(data_, other.data_, size_);    // FlawFinder: ignore
            memcpy(iv_, other.iv_, size_);        // FlawFinder: ignore
        }
    }

    ~keyphrase_t() final {
        zero(data_);
        zero(iv_);
    }

    auto operator=(const keyphrase_t& other) noexcept -> auto& {
        if(&other == this)
            return *this;

        size_ = other.size_;
        cipher_ = other.cipher_;
        if(size_) {
            memcpy(data_, other.data_, size_);          // FlawFinder: ignore
            memcpy(iv_, other.iv_, size_);              // FlawFinder: ignore
        }
        return *this;
    }

    void set(const salt_t& salt, const std::string_view& phrase, const EVP_CIPHER *algo = EVP_aes_256_cbc(), const EVP_MD *md = EVP_sha256(), int rounds = 1) noexcept {
        size_ = EVP_CIPHER_key_length(algo);
        auto kp = reinterpret_cast<const uint8_t *>(phrase.data());
        auto ks = static_cast<size_t>(EVP_BytesToKey(algo, md, salt.data(), kp, static_cast<int>(phrase.size()), rounds, data_, iv_));
        if(ks < size_)
            size_ = 0;
        else
            size_ = ks;
    }

    void set(const salt_t& salt, const Key& key, const EVP_CIPHER *algo = EVP_aes_256_cbc(), const EVP_MD *md = EVP_sha256(), int rounds = 1) noexcept {
        size_ = EVP_CIPHER_key_length(algo);
        auto ks = static_cast<size_t>(EVP_BytesToKey(algo, md, salt.data(), key.data(), static_cast<int>(key.size()), rounds, data_, iv_));
        if(ks < size_)
            size_ = 0;
        else
            size_ = ks;
    }

    auto iv() const noexcept -> const uint8_t *{
        return iv_;
    }

    auto data() const noexcept -> const uint8_t * final {
        return data_;
    }

    auto size() const noexcept -> size_t final {
        return size_;
    }

    auto cipher() const noexcept -> const EVP_CIPHER * final {
        return cipher_;
    }

private:
    static constexpr size_t maxsize = 64;

    const EVP_CIPHER *cipher_{nullptr};
    uint8_t data_[maxsize]{0};
    uint8_t iv_[maxsize]{0};
    size_t size_{0U};
};

class decrypt_t final {
public:
    explicit decrypt_t(const EVP_CIPHER *algo = EVP_aes_256_cbc()) noexcept : algo_(algo) {}

    decrypt_t(decrypt_t&& other) noexcept {
        if(other.ctx_) {
            ctx_ = other.ctx_;
            algo_ = other.algo_;
            other.ctx_ = nullptr;
        }
    }

    explicit decrypt_t(const keyphrase_t& key) noexcept :
    ctx_(EVP_CIPHER_CTX_new()), algo_(key.cipher()) {
        if(!ctx_)
            return;

        if(key.size() != keysize() || !EVP_DecryptInit_ex(ctx_, algo_, nullptr, key.data(), key.iv())) {
            EVP_CIPHER_CTX_free(ctx_);
            ctx_ = nullptr;
            return;
        }

        EVP_CIPHER_CTX_set_key_length(ctx_, EVP_MAX_KEY_LENGTH);
    }

    ~decrypt_t() {
        if(ctx_)
            EVP_CIPHER_CTX_free(ctx_);
    }

    auto operator=(decrypt_t&& other) noexcept -> decrypt_t& {
        if(ctx_)
            EVP_CIPHER_CTX_free(ctx_);
        ctx_ = nullptr;
        if(other.ctx_) {
            ctx_ = other.ctx_;
            algo_ = other.algo_;
            other.ctx_ = nullptr;
        }
        return *this;
    }

    auto operator=(const keyphrase_t& key) -> decrypt_t& {
        if(key.cipher() != algo_)
            throw std::runtime_error("cipher type mismatch");

        if(ctx_)
            EVP_CIPHER_CTX_free(ctx_);

        if(keysize() != key.size()) {
            ctx_ = nullptr;
            return *this;
        }

        ctx_ = EVP_CIPHER_CTX_new();
        if(!ctx_)
            return *this;

        if(!EVP_DecryptInit_ex(ctx_, algo_, nullptr, key.data(), key.iv())) {
            EVP_CIPHER_CTX_free(ctx_);
            return *this;
        }

        EVP_CIPHER_CTX_set_key_length(ctx_, EVP_MAX_KEY_LENGTH);
        return *this;
    }

    operator bool() const noexcept {
        return ctx_ != nullptr;
    }

    auto operator!() const noexcept {
        return ctx_ == nullptr;
    }

    auto size() const noexcept {
        return EVP_CIPHER_block_size(algo_);
    }

    auto cipher() const noexcept {
        return algo_;
    }

    auto keysize() const noexcept -> size_t {
        return EVP_CIPHER_key_length(algo_);
    }

    auto update(const uint8_t *in, uint8_t *out, size_t size) noexcept {
        auto used = 0;

        if(!ctx_)
            return size_t(0);

        if(!EVP_DecryptUpdate(ctx_, out, &used, in, static_cast<int>(size)))
            return size_t(0);

        return static_cast<size_t>(used);
    }

    auto finish(uint8_t *out) noexcept {
        auto used = 0;

        if(!ctx_)
            return size_t(0);

        if(!EVP_DecryptFinal_ex(ctx_, out, &used))
            used = 0;

        EVP_CIPHER_CTX_free(ctx_);
        ctx_ = nullptr;
        return static_cast<size_t>(used);
    }

private:
    EVP_CIPHER_CTX *ctx_{nullptr};
    const EVP_CIPHER *algo_{nullptr};
};

class encrypt_t final {
public:
    explicit encrypt_t(const EVP_CIPHER *algo = EVP_aes_256_cbc()) noexcept : algo_(algo) {}

    encrypt_t(encrypt_t&& other) noexcept {
        if(other.ctx_) {
            ctx_ = other.ctx_;
            algo_ = other.algo_;
            other.ctx_ = nullptr;
        }
    }

    explicit encrypt_t(const keyphrase_t& key) noexcept :
    ctx_(EVP_CIPHER_CTX_new()), algo_(key.cipher()) {
        if(!ctx_)
            return;

        if(keysize() != key.size()) {
            EVP_CIPHER_CTX_free(ctx_);
            ctx_ = nullptr;
            return;
        }

        if(!EVP_EncryptInit_ex(ctx_, algo_, nullptr, key.data(), key.iv())) {
            EVP_CIPHER_CTX_free(ctx_);
            return;
        }

        EVP_CIPHER_CTX_set_key_length(ctx_, EVP_MAX_KEY_LENGTH);
    }

    ~encrypt_t() {
        if(ctx_)
            EVP_CIPHER_CTX_free(ctx_);
    }

    auto operator=(encrypt_t&& other) noexcept -> encrypt_t& {
        if(ctx_)
            EVP_CIPHER_CTX_free(ctx_);
        ctx_ = nullptr;
        if(other.ctx_) {
            ctx_ = other.ctx_;
            algo_ = other.algo_;
            other.ctx_ = nullptr;
        }
        return *this;
    }

    auto operator=(const keyphrase_t& key) -> encrypt_t& {
        if(key.cipher() != algo_)
            throw std::runtime_error("cipher type mismatch");

        if(ctx_)
            EVP_CIPHER_CTX_free(ctx_);

        if(keysize() != key.size()) {
            ctx_ = nullptr;
            return *this;
        }

        ctx_ = EVP_CIPHER_CTX_new();
        if(!ctx_)
            return *this;

        if(!EVP_EncryptInit_ex(ctx_, algo_, nullptr, key.data(), key.iv())) {
            EVP_CIPHER_CTX_free(ctx_);
            return *this;
        }

        EVP_CIPHER_CTX_set_key_length(ctx_, EVP_MAX_KEY_LENGTH);
        return *this;
    }

    operator bool() const noexcept {
        return ctx_ != nullptr;
    }

    auto operator!() const noexcept {
        return ctx_ == nullptr;
    }

    auto size() const noexcept {
        return EVP_CIPHER_block_size(algo_);
    }

    auto cipher() const noexcept {
        return algo_;
    }

    auto keysize() const noexcept -> size_t {
        return EVP_CIPHER_key_length(algo_);
    }

    auto update(const uint8_t *in, uint8_t *out, size_t size) noexcept {
        auto used = 0;

        if(!ctx_)
            return size_t(0);

        if(!EVP_EncryptUpdate(ctx_, out, &used, in, static_cast<int>(size)))
            return size_t(0);

        return static_cast<size_t>(used);
    }

    auto finish(uint8_t *out) noexcept {
        auto used = 0;

        if(!ctx_)
            return size_t(0);

        if(!EVP_EncryptFinal_ex(ctx_, out, &used))
            used = 0;

        EVP_CIPHER_CTX_free(ctx_);
        ctx_ = nullptr;
        return static_cast<size_t>(used);
    }

private:
    EVP_CIPHER_CTX *ctx_{nullptr};
    const EVP_CIPHER *algo_{nullptr};
};


} // end namespace
#endif
