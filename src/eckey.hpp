// Copyright (C) 2023 Tycho Softworks.
// This code is licensed under MIT license.

#ifndef TYCHO_ECKEY_HPP_
#define TYCHO_ECKEY_HPP_

#include <utility>
#include <string>
#include <cstdio>
#include <cstdint>
#include <cstring>
#include <memory>
#include <openssl/ec.h>
#include <openssl/evp.h>
#include <openssl/kdf.h>
#include <openssl/bio.h>
#include <openssl/pem.h>
#include <openssl/ecdsa.h>
#include <openssl/obj_mac.h>
#include <openssl/core_names.h>

namespace tycho::crypto {
using key_t = std::pair<const uint8_t *, std::size_t>;

class eckey_t final {
public:
    eckey_t() : key_(EVP_EC_gen("secp521r1")) {}

    explicit eckey_t(const std::string& path) noexcept {
        auto fp = fopen(path.c_str(), "r"); // FlawFinder: ignore
        if(fp != nullptr) {
            key_ = PEM_read_PrivateKey(fp, nullptr, nullptr, nullptr);
            fclose(fp);
        }
    }

    explicit eckey_t(const key_t key, const std::string& curve = "secp521r1") noexcept :
    key_(EVP_EC_gen(curve.c_str())) {
        EVP_PKEY *new_key = nullptr;
        OSSL_PARAM params[] = {
            OSSL_PARAM_construct_octet_string(OSSL_PKEY_PARAM_PRIV_KEY, const_cast<uint8_t *>(key.first), key.second),
            OSSL_PARAM_construct_end()
        };
        auto ctx = EVP_PKEY_CTX_new_from_pkey(nullptr, key_, nullptr);
        EVP_PKEY_fromdata_init(ctx);
        EVP_PKEY_fromdata(ctx, &new_key, EVP_PKEY_KEYPAIR, params);
        EVP_PKEY_CTX_free(ctx);
        EVP_PKEY_free(key_);
        key_ = new_key;
    }

    eckey_t(const eckey_t& other) noexcept :
    key_(other.key_) {
        if(key_)
            EVP_PKEY_up_ref(key_);
        aes_size = other.aes_size;
        memcpy(aes_key, other.aes_key, sizeof(aes_key));// FLawFinder: ignore
    }

    ~eckey_t() {
        if(key_)
            EVP_PKEY_free(key_);
        memset(aes_key, 0, sizeof(aes_key));
        aes_size = 0;
    }

    operator EVP_PKEY *() const noexcept {
        return share();
    }

    operator bool() const noexcept {
        return key_ != nullptr;
    }

    auto operator!() const noexcept {
        return !key_;
    }

    auto operator=(const eckey_t& other) noexcept -> auto& {
        if(&other == this)
            return *this;
        if(key_ == other.key_)
            return *this;
        if(key_)
            EVP_PKEY_free(key_);
        key_ = other.key_;
        memcpy(aes_key, other.aes_key, sizeof(aes_key));// FLawFinder: ignore
        aes_size = other.aes_size;
        if(key_)
            EVP_PKEY_up_ref(key_);
        return *this;
    }

    auto share() const noexcept -> EVP_PKEY * {
        if(key_)
            EVP_PKEY_up_ref(key_);
        return key_;
    }

    auto pub() const noexcept {
        std::string pem;
        if(!key_)
            return pem;
        auto bp = BIO_new(BIO_s_mem());
        if(!bp)
            return pem;
        if(PEM_write_bio_PUBKEY(bp, key_) == 1) {
            BUF_MEM *buf{};
            BIO_get_mem_ptr(bp, &buf);
            pem = std::string(buf->data, buf->length);
        }
        BIO_free(bp);
        return pem;
    }

    auto save(const std::string& name) const noexcept -> bool {
        if(!key_)
            return false;

        auto result = false;
        std::remove(name.c_str());

        auto bio = BIO_new_file(name.c_str(), "w");
        if(bio) {
            result = PEM_write_bio_PrivateKey(bio, key_, nullptr, nullptr, 0, nullptr, nullptr) == 1;
            BIO_free(bio);
        }
        return result;
    }

    auto derived() const noexcept {
        if(!aes_size)
            return key_t{nullptr, 0};
        return key_t{aes_key, aes_size};
    }

    auto derive(EVP_PKEY *peer, std::string_view info, std::size_t keysize = 0, key_t salt = key_t{nullptr, 0}, const EVP_MD *md = EVP_sha256()) noexcept {
        if(!peer || !md || keysize > sizeof(aes_key))
            return key_t{nullptr, 0};
        auto ctx = EVP_PKEY_CTX_new(key_, nullptr);
        if(!ctx)
            return key_t{nullptr, 0};
        aes_size = 0;
        std::size_t size = 0;
        if(!keysize)
            keysize = EVP_MD_get_size(md);
        EVP_PKEY_derive_init(ctx);
        if((EVP_PKEY_derive_set_peer(ctx, peer) <= 0) || (EVP_PKEY_derive(ctx, nullptr, &size) <= 0) || (size < 1)) {
            EVP_PKEY_CTX_free(ctx);
            return key_t{nullptr, 0};
        }
        auto secret = std::make_unique<uint8_t[]>(size);
        EVP_PKEY_derive(ctx, &secret[0], &size);
        EVP_PKEY_CTX_free(ctx);

        ctx = EVP_PKEY_CTX_new_id(EVP_PKEY_HKDF, nullptr);
        if(!ctx)
            return key_t{nullptr, 0};
        EVP_PKEY_derive_init(ctx);
        EVP_PKEY_CTX_hkdf_mode(ctx, EVP_PKEY_HKDEF_MODE_EXTRACT_AND_EXPAND);
        EVP_PKEY_CTX_set_hkdf_md(ctx, md);
        EVP_PKEY_CTX_set1_hkdf_salt(ctx, salt.first, int(salt.second));
        EVP_PKEY_CTX_set1_hkdf_key(ctx, &secret[0], int(size));
        EVP_PKEY_CTX_add1_hkdf_info(ctx, reinterpret_cast<const uint8_t*>(info.data()), int(info.size()));
        EVP_PKEY_derive(ctx, aes_key, &keysize);
        EVP_PKEY_CTX_free(ctx);
        if(keysize < 8)
            return key_t{nullptr, 0};
        aes_size = keysize;
        return key_t{aes_key, keysize};
    }

private:
    EVP_PKEY *key_{nullptr};
    std::size_t aes_size{0};
    uint8_t aes_key[64]{};
};
} // end namespace
#endif
