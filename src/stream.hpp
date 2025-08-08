// Copyright (C) 2023 Tycho Softworks.
// This code is licensed under MIT license.

#ifndef TYCHO_STREAM_HPP_
#define TYCHO_STREAM_HPP_

#include <system_error>
#include <iostream>
#include <utility>
#include <cerrno>
#include <algorithm>

#include <sys/types.h>
#include <fcntl.h>

#ifndef _MSC_VER
#include <unistd.h>
#else
#include <BaseTsd.h>
using ssize_t = SSIZE_T;
#endif

#if defined(_MSC_VER) || defined(__MINGW32__) || defined(__MINGW64__) || defined(WIN32)
#if _WIN32_WINNT < 0x0600 && !defined(_MSC_VER)
#undef _WIN32_WINNT
#define _WIN32_WINNT 0x0600 // NOLINT
#endif
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <sys/socket.h>
#include <net/if.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/un.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>
#include <poll.h>
#endif

#ifndef MSG_NOSIGNAL
#define MSG_NOSIGNAL 0 // NOLINT
#endif

namespace tycho {
template <std::size_t S = 536>
class socket_stream : protected std::streambuf, public std::iostream {
public:
    inline static const auto maxsize = S;

    // typically used to accept a tcp session from a listener socket
    socket_stream(int from, const struct sockaddr *peer, std::size_t size = S) : std::iostream(static_cast<std::streambuf *>(this)), so_(from), family_(peer ? peer->sa_family : AF_UNSPEC) {
        allocate(size);
    }

    // typically used to connect a tcp session to a remote service
    explicit socket_stream(const struct sockaddr *peer, std::size_t size = S) : std::iostream(static_cast<std::streambuf *>(this)), family_(peer ? peer->sa_family : AF_UNSPEC) {
        socklen_t plen = sizeof(sockaddr_storage);
        if (peer)
            switch (peer->sa_family) {
            case AF_INET:
                plen = sizeof(struct sockaddr_in);
                break;
            case AF_INET6:
                plen = sizeof(struct sockaddr_in6);
                break;
            default:
                break;
            }

        auto to = -1;
        if (peer)
            to = make_socket(::socket(peer->sa_family, SOCK_STREAM, IPPROTO_TCP));
        if (to == -1 || ::connect(to, peer, plen) == -1) {
            if (to != -1)
                close_socket(to);
            else
                errno = EBADF;
            throw std::system_error(errno, std::generic_category(), "Stream failed to connect socket");
        }
        so_ = to;
        allocate(size);
    }

    socket_stream(socket_stream&& from) noexcept : std::iostream(static_cast<std::streambuf *>(this)), so_(from.so_), family_(from.family_) {
        from.so_ = -1;
        from.allocate(0);
        allocate(S);
    }

    ~socket_stream() override {
        close_socket(so_);
        clear();
    }

    socket_stream(const socket_stream&) = delete;
    auto operator=(const socket_stream&) -> socket_stream& = delete;

    auto operator=(socket_stream&& from) noexcept -> socket_stream& {
        if (this != &from) {
            if (so_ != -1)
                close_socket(so_);

            so_ = from.so_;
            family_ = from.family_;

            from.so_ = -1;
            from.allocate(0);
            allocate(S);
        }
        return *this;
    }

    auto sync() -> int override {
        auto len = pptr() - pbase();
        if (!len) return 0;
        if (send_socket(pbase(), len)) {
            setp(pbuf, pbuf + bufsize);
            return 0;
        }
        return -1;
    }

    auto is_open() const noexcept {
        return so_ != -1;
    }

    auto out_pending() const noexcept {
        return std::streamsize(pptr() - pbase());
    }

    auto in_avail() const noexcept {
        return std::streamsize(egptr() - gptr());
    }

    auto family() const noexcept {
        return family_;
    }

    auto buffer_size() const noexcept {
        return bufsize;
    }

    void stop() { // may be called from another thread
        auto so = so_;
        so_ = -1;
        close_socket(so);
    }

    void close() {
        sync();
        stop();
        setstate(std::ios::eofbit);
    }

    auto wait(int timeout = -1) -> bool {
        struct pollfd pfd{0};
        pfd.fd = so_;
        pfd.revents = 0;
        pfd.events = POLLIN;

        if (pfd.fd == -1) return false;
        if (gptr() < egptr()) return true;
        auto status = wait_socket(&pfd, timeout);
        if (status < 0) { // low level error...
            io_err(status);
            return false;
        }
        if (!status) return false; // timeout...
        // return if low level is pending...
        return (pfd.revents & POLLIN);
    }

protected:
    char gbuf[S]{}, pbuf[S]{};
    // cppcheck-suppress unusedStructMember
    std::size_t bufsize{0}, getsize{0};

    auto io_socket() const noexcept {
        return so_;
    }

    auto io_err(ssize_t result) {
        if (result == -1) {
            auto error = errno;
            switch (error) {
            case EAGAIN:
            case EINTR:
                return std::size_t(0);
            case EPIPE: {
                auto so = so_;
                so_ = -1;
                close_socket(so);
                setstate(std::ios::eofbit);
                return std::size_t(0);
            }
            default:
                setstate(std::ios::failbit);
                throw std::system_error(errno, std::generic_category(), "Stream i/o error");
            }
        }
        return std::size_t(result);
    }

    void set_blocking(bool enable) {
        block_socket(so_, enable);
    }

    void allocate(std::size_t size) {
        if (!size)
            ++size;

        size = (std::min)(size, maxsize);
        setg(gbuf, gbuf, gbuf);
        setp(pbuf, pbuf + size);
        bufsize = getsize = size;
    }

    auto underflow() -> int override {
        if (gptr() == egptr()) {
            auto len = recv_socket(gbuf, getsize);
            if (!len) return EOF;
            setg(gbuf, gbuf, gbuf + len);
        }
        return get_type(*gptr());
    }

    auto overflow(int c) -> int override {
        if (c == EOF) {
            if (sync() == 0) return not_eof(c);
            return EOF;
        }

        if (pptr() == epptr() && sync() != 0) return EOF;
        *pptr() = put_type(c);
        pbump(1);
        return c;
    }

    static auto constexpr is_eof(char x) {
        return std::streambuf::traits_type::eq_int_type(x, EOF);
    }

    static auto constexpr not_eof(int x) {
        return std::streambuf::traits_type::not_eof(x);
    }

    static auto constexpr get_type(char x) {
        return std::streambuf::traits_type::to_int_type(x);
    }

    static auto constexpr put_type(int x) {
        return std::streambuf::traits_type::to_char_type(x);
    }

private:
    volatile int so_{-1};
    int family_{AF_UNSPEC};

#if defined(_MSC_VER) || defined(__MINGW32__) || defined(__MINGW64__) || defined(WIN32)
    static auto make_socket(SOCKET so) noexcept {
        return int(so);
    }

    static void block_socket(SOCKET so, bool enable) noexcept {
        DWORD mode = enable ? 1 : 0;
        ioctlsocket(so, FIONBIO, &mode);
    }

    static void close_socket(SOCKET so) noexcept {
        ::shutdown(so, SD_BOTH);
        closesocket(so);
    }

    auto wait_socket(struct pollfd *pfd, int timeout) noexcept {
        auto status = WSAPoll(pfd, 1, timeout);
        if (pfd->revents & (POLLNVAL | POLLHUP)) {
            auto so = so_;
            so_ = -1;
            close_socket(so);
            setstate(std::ios::eofbit);
            return 0;
        }
        return status;
    }

    auto send_socket(const void *buffer, std::size_t size) {
        return io_err(::send(so_, static_cast<const char *>(buffer), int(size), MSG_NOSIGNAL));
    }

    auto recv_socket(void *buffer, std::size_t size) {
        return io_err(::recv(so_, static_cast<char *>(buffer), int(size), 0));
    }
#else
    static auto make_socket(int so) noexcept {
        return so;
    }

    static void close_socket(int so) noexcept {
        ::shutdown(so, SHUT_RDWR);
        ::close(so);
    }

    static void block_socket(int so, bool enable) noexcept {
        auto flags = fcntl(so, F_GETFL, 0);
        if (enable)
            fcntl(so, F_SETFL, flags & ~O_NONBLOCK);
        else
            fcntl(so, F_SETFL, flags | O_NONBLOCK);
    }

    auto wait_socket(struct pollfd *pfd, int timeout) noexcept {
        auto status = ::poll(pfd, 1, timeout);
        if (pfd->revents & (POLLNVAL | POLLHUP)) {
            auto so = so_;
            so_ = -1;
            close_socket(so);
            setstate(std::ios::eofbit);
            return 0;
        }
        return status;
    }

    auto send_socket(const void *buffer, std::size_t size) {
        return io_err(::send(so_, buffer, size, MSG_NOSIGNAL));
    }

    auto recv_socket(void *buffer, std::size_t size) {
        return io_err(::recv(so_, buffer, size, 0));
    }
#endif
};

using tcpstream = socket_stream<536>;
using tcpstream6 = socket_stream<1220>;
} // namespace tycho

#endif
