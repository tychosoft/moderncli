// Copyright (C) 2023 Tycho Softworks.
// This code is licensed under MIT license.

#ifndef TYCHO_CIPHER_HPP_
#define TYCHO_CIPHER_HPP_

#include <string_view>
#include <utility>
#include <stdexcept>
#include <cstring>
#include <cstdint>
#include <openssl/evp.h>

namespace tycho::crypto {
using key_t = std::pair<const uint8_t *, std::size_t>;

inline constexpr auto nosalt = key_t{nullptr, 64};
constexpr auto is_salt(const key_t& key) {
    return key.second == 64;
}

class keyphrase_t final {
public:
    keyphrase_t() = default;

    explicit keyphrase_t(const std::string_view& phrase, const key_t salt = nosalt, const EVP_CIPHER *algo = EVP_aes_256_cbc(), const EVP_MD *md = EVP_sha256(), int rounds = 1) noexcept :
    cipher_(algo), size_(EVP_CIPHER_key_length(algo)) {
        if(!is_salt(salt)) {
            size_ = 0;
            return;
        }

        auto kp = reinterpret_cast<const uint8_t *>(phrase.data());
        auto ks = std::size_t(EVP_BytesToKey(algo, md, salt.first, kp, int(phrase.size()), rounds, data_, iv_));
        if(ks < size_)
            size_ = 0;
        else
            size_ = ks;
    }

    explicit keyphrase_t(const key_t& key, const key_t salt = nosalt, const EVP_CIPHER *algo = EVP_aes_256_cbc(), const EVP_MD *md = EVP_sha256(), int rounds = 1) noexcept :
    cipher_(algo), size_(EVP_CIPHER_key_length(algo)) {
        if(!is_salt(salt)) {
            size_ = 0;
            return;
        }

        auto ks = std::size_t(EVP_BytesToKey(algo, md, salt.first, key.first, int(key.second), rounds, data_, iv_));
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

    ~keyphrase_t() {
        memset(data_, 0, sizeof(data_));
    }

    operator key_t() const {
        return std::make_pair(data_, size_);
    }

    operator bool() const {
        return size_ > 0;
    }

    auto  operator!() const {
        return size_ == 0;
    }

    auto operator *() const {
        return std::make_pair(data_, size_);
    }

    auto operator=(const keyphrase_t& other) noexcept -> auto& {
        if(&other == this) return *this;
        size_ = other.size_;
        cipher_ = other.cipher_;
        if(size_) {
            memcpy(data_, other.data_, size_);          // FlawFinder: ignore
            memcpy(iv_, other.iv_, size_);              // FlawFinder: ignore
        }
        return *this;
    }

    void set(const std::string_view& phrase, const key_t salt = nosalt, const EVP_CIPHER *algo = EVP_aes_256_cbc(), const EVP_MD *md = EVP_sha256(), int rounds = 1) noexcept {
        if(!is_salt(salt)) {
            size_ = 0;
            return;
        }

        size_ = EVP_CIPHER_key_length(algo);
        auto kp = reinterpret_cast<const uint8_t *>(phrase.data());
        auto ks = std::size_t(EVP_BytesToKey(algo, md, salt.first, kp, int(phrase.size()), rounds, data_, iv_));
        if(ks < size_)
            size_ = 0;
        else
            size_ = ks;
    }

    void set(const key_t& key, const key_t salt = nosalt, const EVP_CIPHER *algo = EVP_aes_256_cbc(), const EVP_MD *md = EVP_sha256(), int rounds = 1) noexcept {
        if(!is_salt(salt)) {
            size_ = 0;
            return;
        }

        size_ = EVP_CIPHER_key_length(algo);
        auto ks = std::size_t(EVP_BytesToKey(algo, md, salt.first, key.first, int(key.second), rounds, data_, iv_));
        if(ks < size_)
            size_ = 0;
        else
            size_ = ks;
    }

    auto iv() const noexcept -> const uint8_t * {
        return iv_;
    }

    auto data() const noexcept -> const uint8_t * {
        return data_;
    }

    auto size() const noexcept {
        return size_;
    }

    auto cipher() const noexcept -> const EVP_CIPHER * {
        return cipher_;
    }

private:
    static constexpr std::size_t maxsize = 64;

    const EVP_CIPHER *cipher_{nullptr};
    uint8_t data_[maxsize]{0};
    uint8_t iv_[maxsize]{0};
    std::size_t size_{0U};
};

inline auto get_tag_size(const EVP_CIPHER *algo) -> std::size_t {
    if(algo) {
        auto nid = EVP_CIPHER_nid(algo);
        if(nid == NID_aes_128_gcm || nid == NID_aes_192_gcm || nid == NID_aes_256_gcm) return 16;
    }
    return 0;
}

class decrypt_t final {
public:
    // cppcheck-suppress noExplicitConstructor
    decrypt_t(const EVP_CIPHER *algo = EVP_aes_256_cbc()) noexcept : algo_(algo) {}

    decrypt_t(decrypt_t&& other) noexcept {
        if(other.ctx_) {
            ctx_ = other.ctx_;
            algo_ = other.algo_;
            tag_ = other.tag_;
            other.ctx_ = nullptr;
        }
    }

    // cppcheck-suppress noExplicitConstructor
    decrypt_t(const keyphrase_t& key) noexcept :
    ctx_(EVP_CIPHER_CTX_new()), algo_(key.cipher()) {
        if(!ctx_) return;
        tag_ = get_tag_size(algo_);
        if(tag_) {
            EVP_DecryptInit_ex(ctx_, algo_, nullptr, nullptr, nullptr);
            EVP_CIPHER_CTX_ctrl(ctx_, EVP_CTRL_GCM_SET_IVLEN, 12, nullptr);
        }

        if(key.size() != keysize() || !EVP_DecryptInit_ex(ctx_, tag_ ? nullptr :  algo_, nullptr, key.data(), key.iv())) {
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
        if(this == &other) return *this;
        if(ctx_)
            EVP_CIPHER_CTX_free(ctx_);
        ctx_ = nullptr;
        if(other.ctx_) {
            ctx_ = other.ctx_;
            algo_ = other.algo_;
            tag_ = other.tag_;
            other.ctx_ = nullptr;
        }
        return *this;
    }

    auto operator=(const keyphrase_t& key) -> decrypt_t& {
        if(key.cipher() != algo_) throw std::runtime_error("cipher type mismatch");
        if(ctx_)
            EVP_CIPHER_CTX_free(ctx_);

        if(keysize() != key.size()) {
            ctx_ = nullptr;
            return *this;
        }

        ctx_ = EVP_CIPHER_CTX_new();
        if(!ctx_) return *this;
        tag_ = get_tag_size(algo_);
        if(tag_) {
            EVP_DecryptInit_ex(ctx_, algo_, nullptr, nullptr, nullptr);
            EVP_CIPHER_CTX_ctrl(ctx_, EVP_CTRL_GCM_SET_IVLEN, 12, nullptr);
        }

        if(!EVP_DecryptInit_ex(ctx_, tag_ ? nullptr : algo_, nullptr, key.data(), key.iv())) {
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

    auto keysize() const noexcept -> std::size_t {
        return EVP_CIPHER_key_length(algo_);
    }

    auto tagsize() const noexcept -> std::size_t {
        return tag_;
    }

    auto update(const uint8_t *in, uint8_t *out, std::size_t size) noexcept {
        auto used = 0;

        if(!ctx_) return std::size_t(0);
        if(!EVP_DecryptUpdate(ctx_, out, &used, in, int(size))) return std::size_t(0);
        return std::size_t(used);
    }

    auto finish(uint8_t *out, const uint8_t *tag) noexcept {
        auto used = 0;

        if(!ctx_) return std::size_t(0);
        if(tag_ && tag)
            EVP_CIPHER_CTX_ctrl(ctx_, EVP_CTRL_GCM_SET_TAG, int(tag_), const_cast<uint8_t *>(tag));

        if(!EVP_DecryptFinal_ex(ctx_, out, &used))
            used = 0;

        EVP_CIPHER_CTX_free(ctx_);
        ctx_ = nullptr;
        return std::size_t(used);
    }

private:
    EVP_CIPHER_CTX *ctx_{nullptr};
    const EVP_CIPHER *algo_{nullptr};
    std::size_t tag_{0};
};

class encrypt_t final {
public:
    // cppcheck-suppress noExplicitConstructor
    encrypt_t(const EVP_CIPHER *algo = EVP_aes_256_cbc()) noexcept : algo_(algo) {}

    encrypt_t(encrypt_t&& other) noexcept {
        if(other.ctx_) {
            ctx_ = other.ctx_;
            algo_ = other.algo_;
            tag_ = other.tag_;
            other.ctx_ = nullptr;
        }
    }

    // cppcheck-suppress noExplicitConstructor
    encrypt_t(const keyphrase_t& key) noexcept :
    ctx_(EVP_CIPHER_CTX_new()), algo_(key.cipher()) {
        if(!ctx_) return;
        if(keysize() != key.size()) {
            EVP_CIPHER_CTX_free(ctx_);
            ctx_ = nullptr;
            return;
        }

        tag_ = get_tag_size(algo_);
        if(tag_) {
            EVP_EncryptInit_ex(ctx_, algo_, nullptr, nullptr, nullptr);
            EVP_CIPHER_CTX_ctrl(ctx_, EVP_CTRL_GCM_SET_IVLEN, 12, nullptr);
        }

        if(!EVP_EncryptInit_ex(ctx_, tag_ ? nullptr : algo_, nullptr, key.data(), key.iv())) {
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
        if(this == &other) return *this;
        if(ctx_)
            EVP_CIPHER_CTX_free(ctx_);
        ctx_ = nullptr;
        if(other.ctx_) {
            ctx_ = other.ctx_;
            algo_ = other.algo_;
            tag_ = other.tag_;
            other.ctx_ = nullptr;
        }
        return *this;
    }

    auto operator=(const keyphrase_t& key) -> encrypt_t& {
        if(key.cipher() != algo_) throw std::runtime_error("cipher type mismatch");
        if(ctx_)
            EVP_CIPHER_CTX_free(ctx_);

        if(keysize() != key.size()) {
            ctx_ = nullptr;
            return *this;
        }

        ctx_ = EVP_CIPHER_CTX_new();
        if(!ctx_) return *this;
        tag_ = get_tag_size(algo_);
        if(tag_) {
            EVP_EncryptInit_ex(ctx_, algo_, nullptr, nullptr, nullptr);
            EVP_CIPHER_CTX_ctrl(ctx_, EVP_CTRL_GCM_SET_IVLEN, 12, nullptr);
        }

        if(!EVP_EncryptInit_ex(ctx_, tag_ ? nullptr : algo_, nullptr, key.data(), key.iv())) {
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

    auto keysize() const noexcept -> std::size_t {
        return EVP_CIPHER_key_length(algo_);
    }

    auto tagsize() const noexcept -> std::size_t {
        return tag_;
    }

    auto update(const uint8_t *in, uint8_t *out, std::size_t size) noexcept {
        auto used = 0;

        if(!ctx_) return std::size_t(0);
        if(!EVP_EncryptUpdate(ctx_, out, &used, in, int(size))) return std::size_t(0);
        return std::size_t(used);
    }

    auto finish(uint8_t *out, uint8_t *tag = nullptr) noexcept {
        auto used = 0;

        if(!ctx_) return std::size_t(0);
        if(!EVP_EncryptFinal_ex(ctx_, out, &used))
            used = 0;

        if(tag_ && tag)
            EVP_CIPHER_CTX_ctrl(ctx_, EVP_CTRL_GCM_GET_TAG, int(tag_), tag);

        EVP_CIPHER_CTX_free(ctx_);
        ctx_ = nullptr;
        return std::size_t(used);
    }

private:
    EVP_CIPHER_CTX *ctx_{nullptr};
    const EVP_CIPHER *algo_{nullptr};
    std::size_t tag_{0};
};
} // end namespace
#endif
