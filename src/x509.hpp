// Copyright (C) 2023 Tycho Softworks.
// This code is licensed under MIT license.

#ifndef TYCHO_X509_HPP_
#define TYCHO_X509_HPP_

#include <algorithm>
#include <utility>
#include <string>
#include <cstdio>
#include <cstdint>
#include <ctime>
#include <openssl/x509.h>
#include <openssl/evp.h>
#include <openssl/bio.h>
#include <openssl/pem.h>
#include <openssl/ecdsa.h>
#include <openssl/obj_mac.h>
#include <openssl/core_names.h>

namespace tycho::crypto {
class x509_t final {
public:
    x509_t() = default;

    explicit x509_t(X509 *cert) noexcept : cert_(cert) {}

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
        if(&other == this) return *this;
        if(cert_ == other.cert_) return *this;
        if(cert_)
            X509_free(cert_);
        cert_ = other.cert_;
        if(cert_)
            X509_up_ref(cert_);
        return *this;
    }

    auto subject() const noexcept {
        return cert_ ? X509_get_subject_name(cert_) : nullptr;
    }

    auto issuer() const noexcept {
        return cert_ ? X509_get_issuer_name(cert_) : nullptr;
    }

    auto subject(int nid) const noexcept -> std::string {
        auto subj = subject();
        if(!subj) return {};
        auto pos = X509_NAME_get_index_by_NID(subj, nid, -1);

        if(pos < 0) return {};
        auto entry = X509_NAME_get_entry(subj, pos);

        if(!entry) return {};
        return asn1_to_string(X509_NAME_ENTRY_get_data(entry));
    }

    auto issuer(int nid) const noexcept -> std::string {
        auto subj = issuer();
        if(!subj) return {};
        auto pos = X509_NAME_get_index_by_NID(subj, nid, -1);
        if(pos < 0) return {};
        auto entry = X509_NAME_get_entry(subj, pos);
        if(!entry) return {};
        return asn1_to_string(X509_NAME_ENTRY_get_data(entry));
    }

    auto cn() const noexcept -> std::string {
        return subject(NID_commonName);
    }

    auto issued() const noexcept -> std::time_t {
        return cert_ ? asn1_to_time(X509_get0_notBefore(cert_)) : 0;
    }

    auto expires() const noexcept -> std::time_t {
        return cert_ ? asn1_to_time(X509_get0_notAfter(cert_)) : 0;
    }

    auto share() const noexcept -> X509 * {
        if(cert_)
            X509_up_ref(cert_);
        return cert_;
    }

private:
    constexpr static const char* months[] = {
        "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
    };

    X509 *cert_{nullptr};

    static auto asn1_to_string(const ASN1_STRING* asn1) noexcept -> std::string {
        if (asn1) {
            const std::size_t len = ASN1_STRING_length(asn1);
            auto data = reinterpret_cast<const char*>(ASN1_STRING_get0_data(asn1));
            return {data, len};
        }
        return {};
    }

    static auto asn1_to_time(const ASN1_TIME *asn1) noexcept -> std::time_t {
        if(!asn1) return 0;
        auto bio = BIO_new(BIO_s_mem());
        ASN1_TIME_print(bio, asn1);

        char buf[128];
        memset(buf, 0, sizeof(buf));
        BIO_read(bio, buf, sizeof(buf) - 1);
        BIO_free(bio);

        int year{}, day{}, hour{}, minute{}, second{};
        sscanf(buf + 4, "%d %d:%d:%d %d", &day, &hour, &minute, &second, &year); // NOLINT
        auto it = std::find(std::begin(months), std::end(months), std::string(buf, 3));
        if(it == std::end(months)) return 0;
        std::tm tm = {};
        tm.tm_mon = int(std::distance(std::begin(months), it));
        tm.tm_mday = day;
        tm.tm_hour = hour;
        tm.tm_min = minute;
        tm.tm_sec = second;
        tm.tm_year = year - 1900;
        return std::mktime(&tm);
    }
};

inline auto make_x509(const std::string& cert) {
    auto bp = BIO_new_mem_buf(cert.c_str(), -1);
    auto crt = PEM_read_bio_X509(bp, nullptr, nullptr, nullptr);
    BIO_free(bp);
    return x509_t(crt);
}

inline auto load_X509(const std::string& path) {
    auto fp = fopen(path.c_str(), "r"); // FlawFinder: ignore
    if(!fp) return x509_t();
    auto bp = BIO_new(BIO_s_file());
    BIO_set_fp(bp, fp, BIO_NOCLOSE);
    auto cert = PEM_read_bio_X509(bp, nullptr, nullptr, nullptr);
    BIO_free(bp);
    fclose(fp);
    return x509_t(cert);
}
} // end namespace
#endif
