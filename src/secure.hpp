// Copyright (C) 2023 Tycho Softworks.
// This code is licensed under MIT license.

#ifndef TYCHO_SECURE_HPP_
#define TYCHO_SECURE_HPP_

#include "stream.hpp"

#include <openssl/ssl.h>

namespace tycho {
struct secure_certs {
    std::string capath;
    std::string keyfile;
    std::string certfile;
};

template <std::size_t S = 512>
class secure_stream : public socket_stream<S> {
public:
    secure_stream(int from, const struct sockaddr *peer, const secure_certs& certs = secure_certs{}, std::size_t size = S, const SSL_METHOD *method = TLS_server_method()) : socket_stream<S>(from, peer, size), ctx_(SSL_CTX_new(method)) {
        if (!super::is_open() || !ctx_)
            return;

        if (!certs.certfile.empty())
            SSL_CTX_use_certificate_file(ctx_, certs.certfile.c_str(), SSL_FILETYPE_PEM);
        if (!certs.keyfile.empty())
            SSL_CTX_use_PrivateKey_file(ctx_, certs.keyfile.c_str(), SSL_FILETYPE_PEM);

        if (!certs.keyfile.empty() || !SSL_CTX_check_private_key(ctx_)) {
            SSL_CTX_free(ctx_);
            ctx_ = nullptr;
            return;
        }

        if (!certs.capath.empty() && SSL_CTX_load_verify_locations(ctx_, certs.capath.c_str(), nullptr))
            SSL_CTX_set_verify(ctx_, SSL_VERIFY_PEER, nullptr);

        ssl_ = SSL_new(ctx_);
        if (!ssl_) return;
        SSL_set_fd(ssl_, super::io_socket());
        if (SSL_accept(ssl_) <= 0) return;
        bio_ = SSL_get_wbio(ssl_);
        peer_cert = SSL_get_peer_certificate(ssl_);
        if (peer_cert && !certs.capath.empty()) {
            switch (SSL_get_verify_result(ssl_)) {
            case X509_V_OK:
                verified = VERIFIED;
                break;
            case X509_V_ERR_SELF_SIGNED_CERT_IN_CHAIN:
                verified = SIGNED;
                break;
            default:
                break;
            }
        }
    }

    explicit secure_stream(const struct sockaddr *peer, const secure_certs& certs = secure_certs{}, std::size_t size = S, const SSL_METHOD *method = TLS_client_method()) : socket_stream<S>(peer, size), ctx_(SSL_CTX_new(method)) {
        if (!super::is_open() || !ctx_) return;
        if (!certs.certfile.empty())
            SSL_CTX_use_certificate_file(ctx_, certs.certfile.c_str(), SSL_FILETYPE_PEM);

        if (!certs.keyfile.empty())
            SSL_CTX_use_PrivateKey_file(ctx_, certs.keyfile.c_str(), SSL_FILETYPE_PEM);

        if (!certs.keyfile.empty() && !SSL_CTX_check_private_key(ctx_)) {
            SSL_CTX_free(ctx_);
            ctx_ = nullptr;
            return;
        }

        if (!certs.capath.empty() && SSL_CTX_load_verify_locations(ctx_, certs.capath.c_str(), nullptr))
            SSL_CTX_set_verify(ctx_, SSL_VERIFY_PEER, nullptr);

        ssl_ = SSL_new(ctx_);
        if (!ssl_) return;
        SSL_set_fd(ssl_, super::io_socket());
        if (SSL_connect(ssl_) <= 0) return;
        bio_ = SSL_get_wbio(ssl_);
        peer_cert = SSL_get_peer_certificate(ssl_);
        if (peer_cert && !certs.capath.empty()) {
            switch (SSL_get_verify_result(ssl_)) {
            case X509_V_OK:
                verified = VERIFIED;
                break;
            case X509_V_ERR_SELF_SIGNED_CERT_IN_CHAIN:
                verified = SIGNED;
                break;
            default:
                break;
            }
        }
    }

    secure_stream(secure_stream&& from) noexcept : socket_stream<S>(from), peer_cert(from.peer_cert), ctx_(from.ctx_), accepted(from.accepted), verified(from.verified), bio_(from.bio_), ssl_(from.ssl_) {
        from.ctx_ = nullptr;
        from.ssl_ = nullptr;
        from.bio_ = nullptr;
        from.peer_cert = nullptr;
    }

    ~secure_stream() override {
        if (bio_)
            SSL_shutdown(ssl_);
        if (peer_cert)
            X509_free(peer_cert);
        if (ssl_)
            SSL_free(ssl_);
        if (ctx_)
            SSL_CTX_free(ctx_);
        super::stop();
        super::clear();
    }

    auto operator=(secure_stream&& from) noexcept -> secure_stream& {
        if (this != &from) {
            socket_stream<S>::operator=(std::move(from));
            if (ssl_) SSL_free(ssl_);
            if (bio_) BIO_free_all(bio_);
            if (peer_cert) X509_free(peer_cert);

            ctx_ = from.ctx_;
            ssl_ = from.ssl_;
            bio_ = from.bio_;
            peer_cert = from.peer_cert;
            accepted = from.accepted;
            verified = from.verified;

            from.ctx_ = nullptr;
            from.ssl_ = nullptr;
            from.bio_ = nullptr;
            from.peer_cert = nullptr;
        }
        return *this;
    }

    auto peer() const noexcept {
        if (peer_cert)
            X509_up_ref(peer_cert);
        return peer_cert;
    }

    auto is_accepted() const noexcept {
        return accepted;
    }

    auto is_secure() const noexcept {
        return bio_ != nullptr;
    }

    auto is_signed() const noexcept {
        return verified == SIGNED || verified == VERIFIED;
    }

    auto is_verified() const noexcept {
        return verified == VERIFIED;
    }

    auto underflow() -> int override {
        if (!bio_) return super::underflow();
        if (super::gptr() == super::egptr()) {
            auto len = SSL_read(ssl_, super::gbuf, super::getsize);
            if (len <= 0) return EOF;
            super::setg(super::gbuf, super::gbuf, super::gbuf + len);
        }
        return super::get_type(*super::gptr());
    }

    auto sync() -> int override {
        if (!bio_) return super::sync();
        auto len = super::pptr() - super::pbase();
        if (len && !SSL_write(ssl_, super::pbase(), len)) return 0;
        return BIO_flush(bio_);
    }

protected:
    using verify_t = enum { NONE,
        SIGNED,
        VERIFIED };
    using super = secure_stream<S>;

    X509 *peer_cert{nullptr};
    bool accepted{false};
    verify_t verified = NONE;

private:
    SSL_CTX *ctx_{nullptr};
    SSL *ssl_{nullptr};
    BIO *bio_{nullptr};
};

// 512 is really more for cipher block alignment and optimized under ipv4
// fragment size.
using sslstream = secure_stream<512>;
} // namespace tycho

#endif
