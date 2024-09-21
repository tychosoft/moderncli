// Copyright (C) 2023 Tycho Softworks.
// This code is licensed under MIT license.

#ifndef TYCHO_SOCKET_HPP_
#define TYCHO_SOCKET_HPP_

#include <string>
#include <ostream>
#include <cstring>

#include <sys/types.h>
#include <fcntl.h>

#ifndef _MSC_VER
#include <unistd.h>
#endif

#if defined(_MSC_VER) || defined(__MINGW32__) || defined(__MINGW64__) || defined(WIN32)
#if _WIN32_WINNT < 0x0600 && !defined(_MSC_VER)
#undef  _WIN32_WINNT
#define _WIN32_WINNT    0x0600  // NOLINT
#endif
#define USE_CLOSESOCKET
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
#define SOCKET int
#endif

#if __has_include(<poll.h>)
#include <poll.h>
#endif

#include <cstdint>

namespace tycho {
class address_t final {
public:
    address_t() noexcept {
        memset(&store_, 0, sizeof(store_));
    }

    address_t(const address_t& from) = default;
    auto operator=(const address_t& from) -> address_t& = default;

    explicit address_t(const struct addrinfo *addr) noexcept {
        set(addr);
    }

    explicit address_t(const struct sockaddr *addr) noexcept {
        set(addr);
    }

    explicit address_t(const std::string& addr, uint16_t port = 0) noexcept {
        set(addr, port);
    }

    auto operator=(const struct addrinfo *addr) noexcept -> auto& {
        set(addr);
        return *this;
    }

    auto operator=(const struct sockaddr *addr) noexcept -> auto& {
        set(addr);
        return *this;
    }

    auto operator=(const std::string& addr) noexcept -> auto& {
        set(addr);
        return *this;
    }

    operator bool() const noexcept {
        return store_.ss_family != 0;
    }

    auto operator!() const noexcept {
        return store_.ss_family == 0;
    }

    auto operator*() const noexcept {
        return reinterpret_cast<const struct sockaddr *>(&store_);
    }

    auto operator*() noexcept {
        return reinterpret_cast<struct sockaddr *>(&store_);
    }

    static constexpr auto maxsize() -> socklen_t {
        return sizeof(struct sockaddr_storage);
    }

    auto data() const noexcept {
        return reinterpret_cast<const struct sockaddr *>(&store_);
    }

    auto port() const noexcept -> uint16_t {
        switch(store_.ss_family) {
        case AF_INET:
            return ntohs((reinterpret_cast<const struct sockaddr_in *>(&store_))->sin_port);
        case AF_INET6:
            return ntohs((reinterpret_cast<const struct sockaddr_in6 *>(&store_))->sin6_port);
        default:
            return 0;
        }
    }

    auto size() const noexcept -> socklen_t {
        return size_(store_.ss_family);
    }

    auto family() const noexcept {
        return store_.ss_family;
    }

    void set(const std::string& str, uint16_t in_port = 0) noexcept {
        memset(&store_, 0, sizeof(store_));
        auto cp = str.c_str();
        if(strchr(cp, ':') != nullptr) {
            auto ipv6 = reinterpret_cast<struct sockaddr_in6*>(&store_);
            inet_pton(AF_INET6, cp, &(ipv6->sin6_addr));
            ipv6->sin6_family = AF_INET6;
            ipv6->sin6_port = htons(in_port);
        } else if(strchr(cp, '.') != nullptr) {
            auto ipv4 = reinterpret_cast<struct sockaddr_in*>(&store_);
            inet_pton(AF_INET, cp, &(ipv4->sin_addr));
            ipv4->sin_family = AF_INET;
            ipv4->sin_port = htons(in_port);
        }
    }

    auto to_string() const {
        char buf[256];
        const struct sockaddr_in *ipv4{nullptr};
        const struct sockaddr_in6 *ipv6{nullptr};
        switch(store_.ss_family) {
        case AF_INET:
            ipv4 = reinterpret_cast<const struct sockaddr_in*>(&store_);
            if(::inet_ntop(AF_INET, &(ipv4->sin_addr), buf, sizeof(buf)))
                return std::string(buf);
            break;
        case AF_INET6:
            ipv6 = reinterpret_cast<const struct sockaddr_in6*>(&store_);
            if(::inet_ntop(AF_INET, &(ipv6->sin6_addr), buf, sizeof(buf)))
                return std::string(buf);
            break;
        default:
            break;
        }
        return std::string();
    }

    auto data() noexcept {
        return reinterpret_cast<struct sockaddr *>(&store_);
    }

    void set(const struct addrinfo *addr) noexcept {
        if(!addr)
            memset(&store_, 0, sizeof(store_));
        else
            // FlawFinder: ignore
            memcpy(&store_, addr->ai_addr, addr->ai_addrlen);
    }

    void set(const struct sockaddr *addr) noexcept {
        if(!addr || !size_(addr->sa_family))
            memset(&store_, 0, sizeof(store_));
        else
            // FlawFinder: ignore
            memcpy(&store_, addr, size_(addr->sa_family));
   }

private:
    static auto size_(int family) noexcept -> socklen_t {
        switch(family) {
        case AF_INET:
            return sizeof(struct sockaddr_in);
        case AF_INET6:
            return sizeof(struct sockaddr_in6);
        default:
            return 0;
        }
    }

    struct sockaddr_storage store_{};
};

#ifndef EAI_ADDRFAMILY
#define EAI_ADDRFAMILY EAI_NODATA + 2000    // NOLINT
#endif

#ifndef EAI_SYSTEM
#define EAI_SYSTEM EAI_NODATA + 2001        // NOLINT
#endif

class socket_t {
public:
    enum class error : int {
        success = 0,
        family_empty = EAI_ADDRFAMILY,
        family_invalid = EAI_FAMILY,
        failure = EAI_FAIL,
        failure_temp = EAI_AGAIN,
        failure_memory = EAI_MEMORY,
        failure_service = EAI_SERVICE,
        failure_type = EAI_SOCKTYPE,
        failure_system = EAI_SYSTEM,
        bad_flags = EAI_BADFLAGS,
        empty = EAI_NODATA,
        unknown = EAI_NONAME,

        access = EACCES,
        perms = EPERM,
        inuse = EADDRINUSE,
        unavailable = EADDRNOTAVAIL,
        nofamily = EAFNOSUPPORT,
        busy = EAGAIN,
        connecting = EALREADY,
        badfile = EBADF,
        refused = ECONNREFUSED,
        fault = EFAULT,
        pending = EINPROGRESS,
        interupted = EINTR,
        connected = EISCONN,
        unreachable = ENETUNREACH,
        notsocket = ENOTSOCK,
        noprotocol = EPROTOTYPE,
        timeout = ETIMEDOUT,
        reset = ECONNRESET,
        nopeer = EDESTADDRREQ,
        invalid_argument = EINVAL,
        maxsize = EMSGSIZE,
        nobufs = ENOBUFS,
        nomem = ENOMEM,
        noconnection = ENOTCONN,
        invalid_flags = EOPNOTSUPP,
        disconnect = EPIPE,
        toomany = EMFILE,
        exhausted = ENFILE,
    };

    enum class flag : int {
        none = 0,
    #ifdef MSG_CONFIRM
        confirm = MSG_CONFIRM,
    #else
        confirm = -1,
    #endif
        noroute = MSG_DONTROUTE,
    #ifdef MSG_DONTWAIT
        nowait = MSG_DONTWAIT,
    #else
        nowait = -2,
    #endif
    #ifdef MSG_EOR
        eor = MSG_EOR,
    #else
        eor = -4,
    #endif
    #ifdef MSG_MORE
        more = MSG_MORE,
    #else
        more = -8,
    #endif
    #ifdef MSG_NOSIGNAL
        nosignal = MSG_NOSIGNAL,
    #else
        MSG_NOSIGNAL = -16,
    #endif
        oob = MSG_OOB,
    #ifdef MSG_FASTOPEN
        fast = MSG_FASTOPEN,
    #else
        fast = -32,
    #endif
    #ifdef MSG_ERRROUTE
        error = MSG_ERRQUEUE,
    #else
        error = -64,
    #endif
        peek = MSG_PEEK,
    #ifdef MSG_TRUNC
        truncate = MSG_TRUNC,
    #else
        truncate = -128,
    #endif
        waitall = MSG_WAITALL,
    };

    class service final {
    public:
        service() = default;
        service(const service& from) = delete;
        auto operator=(const service& from) = delete;

        service(service&& from) noexcept : list_(from.list_) {
            from.list_ = nullptr;
        }

        service(const std::string& host, const std::string& service, int family = AF_UNSPEC, int type = SOCK_STREAM, int protocol = 0) noexcept {
            set(host, service, family, type, protocol);
        }

        ~service() {
            release();
        }

        auto operator=(service&& from) noexcept -> auto& {
            release();
            list_ = from.list_;
            from.list_ = nullptr;
            return *this;
        }

        auto operator*() const noexcept {
            return list_;
        }

        operator bool() const noexcept {
            return list_ != nullptr;
        }

        auto operator!() const noexcept {
            return list_ == nullptr;
        }

        auto operator[](unsigned index) const noexcept {
            auto size{0U};
            auto addr = list_;
            while(addr) {
                if(size == index)
                    return addr;
                ++size;
                addr = addr->ai_next;
            }
            return static_cast<struct addrinfo *>(nullptr);
        }

        auto err() const noexcept {
            return err_;
        }

        auto empty() const noexcept {
            return list_ == nullptr;
        }

        auto count() const noexcept {
            auto size{0U};
            const auto *addr = list_;
            while(addr) {
                ++size;
                addr = addr->ai_next;
            }
            return size;
        }

        void release() noexcept {
            if(list_) {
                ::freeaddrinfo(list_);
                list_ = nullptr;
            }
            err_ = error::success;
        }

        void set(const std::string& host = "*", const std::string& service = "", int family = AF_UNSPEC, int type = SOCK_STREAM, int protocol = 0) noexcept {
            release();
            auto svc = service.c_str();
            if(service.empty())
                svc = nullptr;

            struct addrinfo hint{};
            memset(&hint, 0, sizeof(hint));
            hint.ai_family = family;
            hint.ai_socktype = type;
            hint.ai_protocol = protocol;

            // FlawFinder: ignore
            if(svc && atoi(svc) > 0)    // NOLINT
                hint.ai_flags |= NI_NUMERICSERV;

            auto addr = host.c_str();
            if(host.empty() || host == "*")
                addr = nullptr;
            else if(strchr(addr, ':'))
                hint.ai_flags |= AI_NUMERICHOST;

            if(protocol)
                hint.ai_flags |= AI_PASSIVE;

            err_ = error(getaddrinfo(addr, svc, &hint, &list_));  // NOLINT
        }

        auto store(unsigned index = 0) const noexcept {
            struct sockaddr_storage data{};
            auto size{0U};
            const auto *addr = list_;
            memset(&data, 0, sizeof(data));
            while(addr) {
                if(size == index) {
                    // FlawFinder: ignore
                    memcpy(&data, addr->ai_addr, addr->ai_addrlen);
                    return data;
                }
                ++size;
                addr = addr->ai_next;
            }
            return data;
        }

    private:
        auto last_() const noexcept {
            auto addr = list_;
            while(addr && addr->ai_next)
                addr = addr->ai_next;
            return addr;
        }

        void join_(struct addrinfo *from) noexcept {
            auto addr = last_();
            if(addr)
                addr->ai_next = from;
            else
                list_ = from;
        }

        struct addrinfo *list_{nullptr};
        mutable error err_{error::success};
    };

    socket_t() = default;
    socket_t(const socket_t& from) = delete;
    auto operator=(const socket_t& from) = delete;

    socket_t(socket_t&& from) noexcept : so_(from.so_), err_(from.err_) {
        from.so_ = -1;
    }

    explicit socket_t(const socket_t::service& list) noexcept {
        bind(list);
    }

    explicit socket_t(const address_t& addr, int type = 0, int protocol = 0) noexcept {
        bind(addr, type, protocol);
    }

    ~socket_t() {
        release();
    }

    auto operator=(socket_t&& from) noexcept -> auto& {
        release();
        so_ = from.so_;
        err_ = from.err_;
        from.so_ = -1;
        return *this;
    }

    auto operator=(const socket_t::service& to) noexcept -> auto& {
        connect(to);
        return *this;
    }

    operator bool() const noexcept {
        return so_ != -1;
    }

    auto operator!() const noexcept {
        return so_ == -1;
    }

    auto operator*() const noexcept {
        return static_cast<SOCKET>(so_);
    }

;   auto err() const noexcept {
        return err_;
    }

    void release() noexcept {
        if(so_ != -1) {
#ifdef  USE_CLOSESOCKET
            closesocket(so_);
#else
            close(so_);
#endif
            so_ = -1;
        }
        err_ = error::success;
    }

    void bind(const address_t& addr, int type = 0, int protocol = 0) noexcept {
        so_ = set_error(make_socket(::socket(addr.family(), type, protocol)));
        if(so_ != -1)
            if(set_error(::bind(so_, addr.data(), addr.size())) == -1)
                release();
    }

    void bind(const socket_t::service& list) noexcept {
        auto addr = *list;
        release();
        while(addr) {
            so_ = set_error(make_socket(::socket(addr->ai_family, addr->ai_socktype, addr->ai_protocol)));
            if(so_ == -1)
                continue;

            if(set_error(::bind(so_, addr->ai_addr, make_socklen(addr->ai_addrlen)) == -1)) {
                release();
                continue;
            }
        }
    }

    auto connect(const address_t& addr) const noexcept {
        if(so_ != -1)
            return set_error(::connect(so_, addr.data(), addr.size()));
        return -1;
    }

    void connect(const socket_t::service& list) noexcept {
        auto addr = *list;
        release();
        while(addr) {
            so_ = set_error(make_socket(::socket(addr->ai_family, addr->ai_socktype, addr->ai_protocol)));
            if(so_ == -1)
                continue;

            if(set_error(::connect(so_, addr->ai_addr, make_socklen(addr->ai_addrlen))) == -1) {
                release();
                continue;
            }
        }
    }

    auto wait(short events, int timeout) const noexcept -> int {
        if(so_ == -1)
            return -1;

        struct pollfd pfd{static_cast<SOCKET>(so_), events, 0};
        auto result = set_error(socket_t::poll(&pfd, 1, timeout));
        if(result <= 0)
            return result;

        return pfd.revents;
    }

    void listen(int backlog = 5) noexcept {
        if(so_ != -1 && set_error(::listen(so_, backlog)) == -1)
            release();
    }

    auto accept() const noexcept {
        socket_t from{};
        if(so_ != -1) {
            from.so_ = make_socket(::accept(so_, nullptr, nullptr));
            from.set_error(from.so_);
        }
        return from;
    }

    auto peer() const noexcept {
        address_t addr;
        socklen_t len = address_t::maxsize();
        memset(*addr, 0, sizeof(addr));
        if(so_ != -1)
            set_error(::getpeername(so_, *addr, &len));
        return addr;
    }

    auto local() const noexcept {
        address_t addr;
        socklen_t len = address_t::maxsize();
        memset(*addr, 0, sizeof(addr));
        if(so_ != -1)
            set_error(::getsockname(so_, *addr, &len));
        return addr;
    }

    auto send(const void *from, socklen_t size, flag flags = flag::none) const noexcept -> socklen_t {
        if(so_ == -1)
            return 0;
        return io_error(::send(so_, static_cast<const char *>(from), size, int(flags)));
    }

    auto recv(void *to, socklen_t size, flag flags = flag::none) const noexcept -> socklen_t {
        if(so_ == -1)
            return 0;
        return io_error(::recv(so_, static_cast<char *>(to), size, int(flags)));
    }

    auto send(const void *from, socklen_t size, const address_t addr, flag flags = flag::none) const noexcept -> socklen_t {
        if(so_ == -1)
            return 0;

        return io_error(::sendto(so_, static_cast<const char *>(from), size, int(flags), addr.data(), addr.size()));
    }

    auto recv(void *to, socklen_t size, address_t& addr, flag flags = flag::none) const noexcept -> socklen_t {
        auto len = address_t::maxsize();
        if(so_ == -1)
            return 0;

        return io_error(::recvfrom(so_, static_cast<char *>(to), size, int(flags), addr.data(), &len));
    }

    static auto constexpr has(flag id) {
        return int(id) < 0;
    }

#ifdef USE_CLOSESOCKET
    static auto poll(struct pollfd *fds, size_t count, int timeout) noexcept -> int {
        return WSAPoll(fds, count, timeout);
    }

    static auto startup() noexcept {
        WSADATA data;
        auto ver = MAKEWORD(2, 2);
        return WSAStartup(ver, &data) == 0;
    }

    static void shutdown() noexcept {
        static_cast<void>(WSACleanup());
    }
#else
    static auto poll(struct pollfd *fds, size_t count, int timeout) noexcept -> int {
        return ::poll(fds, count, timeout);
    }

    static auto startup() noexcept {
        return true;
    }

    static void shutdown() noexcept {
    }
#endif
protected:
    int so_{-1};
    mutable error err_{error::success};

    auto io_error(ssize_t code) const noexcept -> ssize_t {
        if(code == -1)
            err_ = error(errno);
        else if(code < 0)
            err_ = error(-code);
        else
            err_ = error::success;
        return code;
    }

    auto set_error(int code) const noexcept -> int {
        if(code == -1)
            err_ = error(errno);
        else
            err_ = error::success;
        return code;
    }

private:
#ifdef USE_CLOSESOCKET
    static auto make_socket(SOCKET so) noexcept -> int {
        return static_cast<int>(so);
    }

    static auto make_socklen(size_t size) noexcept -> socklen_t {
        return static_cast<socklen_t>(size);
    }
#else
    static auto make_socket(int so) noexcept -> int {
        return so;
    }

    static auto make_socklen(socklen_t size) noexcept -> socklen_t {
        return size;
    }
#endif
};
using Socket = socket_t;
using service_t = socket_t::service;
using socket_flags = socket_t::flag;
using socket_error = socket_t::error;

template<typename T>
inline auto send(const socket_t& sock, const T& msg, socket_flags flags = socket_flags::none) {
    static_assert(std::is_trivial_v<T>, "T must be Trivial type");

    return sock.send(&msg, sizeof(msg), flags);
}

template<typename T>
inline auto recv(const socket_t& sock, T& msg, socket_flags flags = socket_flags::none) {
    static_assert(std::is_trivial_v<T>, "T must be Trivial type");

    return sock.recv(&msg, sizeof(msg), flags);
}

template<typename T>
inline auto send(const socket_t& sock, const T& msg, const address_t& addr, socket_flags flags = socket_flags::none) {
    static_assert(std::is_trivial_v<T>, "T must be Trivial type");

    return sock.send(&msg, sizeof(msg), addr, flags);
}

template<typename T>
inline auto recv(const socket_t& sock, T& msg, address_t& addr, socket_flags flags = socket_flags::none) {
    static_assert(std::is_trivial_v<T>, "T must be Trivial type");

    return sock.recv(&msg, sizeof(msg), addr, flags);
}

auto constexpr operator|(socket_flags lhs, socket_flags rhs) {
    return static_cast<socket_flags>(int(lhs) | int(rhs));
}
} // end namespace

#ifdef  TYCHO_PRINT_HPP_
template <> class fmt::formatter<tycho::address_t> {
public:
    static constexpr auto parse(format_parse_context& ctx) {
        return ctx.begin();
    }

    template <typename Context>
    constexpr auto format(tycho::address_t const& addr, Context& ctx) const {
        return format_to(ctx.out(), "{}", addr.to_string());
    }
};
#endif

inline auto operator<<(std::ostream& out, const tycho::address_t& addr) -> std::ostream& {
    out << addr.to_string();
    return out;
}
#endif
