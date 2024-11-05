// Copyright (C) 2023 Tycho Softworks.
// This code is licensed under MIT license.

#ifndef TYCHO_ECKEY_HPP_
#define TYCHO_ECKEY_HPP_

#include <utility>
#include <string>
#include <cstdio>
#include <cstdint>
#include <openssl/ec.h>
#include <openssl/evp.h>
#include <openssl/bio.h>
#include <openssl/pem.h>
#include <openssl/ecdsa.h>
#include <openssl/obj_mac.h>
#include <openssl/core_names.h>

namespace crypto {
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
        OSSL_PARAM params[] = {
            OSSL_PARAM_construct_octet_string(OSSL_PKEY_PARAM_PRIV_KEY, const_cast<uint8_t *>(key.first), key.second),
            OSSL_PARAM_construct_end()
        };
        auto ctx = EVP_PKEY_CTX_new_from_pkey(nullptr, key_, nullptr);
        EVP_PKEY_fromdata_init(ctx);
        EVP_PKEY_fromdata(ctx, &key_, EVP_PKEY_KEYPAIR, params);
        EVP_PKEY_CTX_free(ctx);
    }

    eckey_t(const eckey_t& other) noexcept :
    key_(other.key_) {
        if(key_)
            EVP_PKEY_up_ref(key_);
    }

    ~eckey_t() {
        if(key_)
            EVP_PKEY_free(key_);
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

private:
    EVP_PKEY *key_{nullptr};
};
} // end namespace
#endif
