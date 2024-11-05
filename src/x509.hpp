// Copyright (C) 2023 Tycho Softworks.
// This code is licensed under MIT license.

#ifndef TYCHO_X509_HPP_
#define TYCHO_X509_HPP_

#include <utility>
#include <string>
#include <cstdio>
#include <cstdint>
#include <openssl/x509.h>
#include <openssl/evp.h>
#include <openssl/bio.h>
#include <openssl/pem.h>
#include <openssl/ecdsa.h>
#include <openssl/obj_mac.h>
#include <openssl/core_names.h>

namespace crypto {
class x509_t final {
public:
    x509_t() = default;

    explicit x509_t(X509 *cert) noexcept {
        cert_ = cert;
    }

    x509_t(const x509_t& other) noexcept :
    cert_(other.cert_) {
        if(cert_)
            X509_up_ref(cert_);
    }

    ~x509_t() {
        if(cert_)
            X509_free(cert_);
    }

    operator X509 *() const noexcept {
        return share();
    }

    operator bool() const noexcept {
        return cert_ != nullptr;
    }

    auto operator!() const noexcept {
        return !cert_;
    }

    auto operator=(const x509_t& other) noexcept -> auto& {
        if(&other == this)
            return *this;
        if(cert_ == other.cert_)
            return *this;
        if(cert_)
            X509_free(cert_);
        cert_ = other.cert_;
        if(cert_)
            X509_up_ref(cert_);
        return *this;
    }

    auto share() const noexcept -> X509 * {
        if(cert_)
            X509_up_ref(cert_);
        return cert_;
    }

private:
    X509 *cert_{nullptr};
};

inline auto make_x509(const std::string& cert) {
    auto bp = BIO_new_mem_buf(cert.c_str(), -1);
    auto crt = PEM_read_bio_X509(bp, nullptr, nullptr, nullptr);
    BIO_free(bp);
    return x509_t(crt);
}

inline auto load_X509(const std::string& path) {
    auto fp = fopen(path.c_str(), "r");
    if(!fp)
        return x509_t();
    auto bp = BIO_new(BIO_s_file());
    BIO_set_fp(bp, fp, BIO_NOCLOSE);
    auto cert = PEM_read_bio_X509(bp, nullptr, nullptr, nullptr);
    BIO_free(bp);
    fclose(fp);
    return x509_t(cert);
}
} // end namespace
#endif
