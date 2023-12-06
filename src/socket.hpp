// Copyright (C) 2023 Tycho Softworks.
// This code is licensed under MIT license.

#ifndef SOCKET_HPP_
#define SOCKET_HPP_

#include <sys/types.h>
#ifndef _MSC_VER
#include <unistd.h>
#endif
#include <fcntl.h>

#if defined(_MSC_VER) || defined(__MINGW32__) || defined(__MINGW64__) || defined(WIN32)
#if _WIN32_WINNT < 0x0600 && !defined(_MSC_VER)
#undef  _WIN32_WINNT
#define _WIN32_WINNT    0x0600
#endif
#define USE_CLOSESOCKET
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
#endif

#include <string>
#include <cstring>

#if __has_include(<poll.h>)
#include <poll.h>
#endif

namespace tycho {
class address_t final {
public:
    inline address_t() {
        memset(&store_, 0, sizeof(store_));
    }

    inline address_t(const address_t& from) = default;
    inline auto operator=(const address_t& from) -> address_t& = default;

    inline address_t(const struct addrinfo *addr) {
        set(addr);
    }

    inline address_t(const struct sockaddr *addr) {
        set(addr);
    }

    inline address_t(const std::string& addr, uint16_t port = 0) {
        set(addr, port);
    }

    inline auto operator=(const struct addrinfo *addr) -> address_t& {
        set(addr);
        return *this;
    }

    inline auto operator=(const struct sockaddr *addr) -> address_t& {
        set(addr);
        return *this;
    }

    inline auto operator=(const std::string& addr) -> address_t& {
        set(addr);
        return *this;
    }

    inline operator bool() const {
        return store_.ss_family != 0;
    }

    inline auto operator!() const {
        return store_.ss_family == 0;
    }

    inline auto operator*() const {
        return reinterpret_cast<const struct sockaddr *>(&store_);
    }

    inline auto operator*() {
        return reinterpret_cast<struct sockaddr *>(&store_);
    }

    inline static constexpr auto maxsize() -> socklen_t {
        return sizeof(struct sockaddr_storage);
    }

    inline auto data() const {
        return reinterpret_cast<const struct sockaddr *>(&store_);
    }

    inline auto port() const -> uint16_t {
        switch(store_.ss_family) {
        case AF_INET:
            return ntohs((reinterpret_cast<const struct sockaddr_in *>(&store_))->sin_port);
        case AF_INET6:
            return ntohs((reinterpret_cast<const struct sockaddr_in6 *>(&store_))->sin6_port);
        default:
            return 0;
        }
    }

    inline auto size() const -> socklen_t {
        return size_(store_.ss_family);
    }

    inline auto family() const {
        return store_.ss_family;
    }

    inline void set(const std::string& str, uint16_t in_port = 0) {
        memset(&store_, 0, sizeof(store_));
        auto cp = str.c_str();
        if(cp && strchr(cp, ':') != nullptr) {
            auto ipv6 = reinterpret_cast<struct sockaddr_in6*>(&store_);
            inet_pton(AF_INET6, cp, &(ipv6->sin6_addr));
            ipv6->sin6_family = AF_INET6;
            ipv6->sin6_port = htons(in_port);
        } else if(cp && strchr(cp, '.') != nullptr) {
            auto ipv4 = reinterpret_cast<struct sockaddr_in*>(&store_);
            inet_pton(AF_INET, cp, &(ipv4->sin_addr));
            ipv4->sin_family = AF_INET;
            ipv4->sin_port = htons(in_port);
        }
    }

    inline auto to_string() const {
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

    inline auto data() {
        return reinterpret_cast<struct sockaddr *>(&store_);
    }

    inline void set(const struct addrinfo *addr) {
        if(!addr)
            memset(&store_, 0, sizeof(store_));
        else
            // FlawFinder: ignore
            memcpy(&store_, addr->ai_addr, addr->ai_addrlen);
    }

    inline void set(const struct sockaddr *addr) {
        if(!addr || !size_(addr->sa_family))
            memset(&store_, 0, sizeof(store_));
        else
            // FlawFinder: ignore
            memcpy(&store_, addr, size_(addr->sa_family));
   }

private:
    static inline auto size_(int family) -> socklen_t {
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

class service_t final {
public:
    inline service_t() = default;
    service_t(const service_t& from) = delete;
    auto operator=(const service_t& from) = delete;

    inline service_t(service_t&& from) noexcept : list_(from.list_) {
        from.list_ = nullptr;
    }

    inline service_t(const std::string& host, const std::string& service = "", int family = AF_UNSPEC, int type = SOCK_STREAM, int protocol = 0) {
        set(host, service, family, type, protocol);
    }

    inline ~service_t() {
        release();
    }

    inline auto operator=(service_t&& from) noexcept -> service_t& {
        release();
        list_ = from.list_;
        from.list_ = nullptr;
        return *this;
    }

    inline auto operator*() const {
        return list_;
    }

    inline operator bool() const {
        return list_ != nullptr;
    }

    inline auto operator!() const {
        return list_ == nullptr;
    }

    inline auto operator[](unsigned index) const {
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

    inline auto empty() const {
        return list_ == nullptr;
    }

    inline auto count() const {
        auto size{0U};
        auto addr = list_;
        while(addr) {
            ++size;
            addr = addr->ai_next;
        }
        return size;
    }

    inline void release() {
        if(list_) {
            ::freeaddrinfo(list_);
            list_ = nullptr;
        }
    }

    inline void set(const std::string& host = "*", const std::string& service = "", int family = AF_UNSPEC, int type = SOCK_STREAM, int protocol = 0) {
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

        getaddrinfo(addr, svc, &hint, &list_);  // NOLINT
    }

    auto store(unsigned index = 0) const {
        struct sockaddr_storage data{};
        auto size{0U};
        auto addr = list_;
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
    inline auto last_() const {
        auto addr = list_;
        while(addr && addr->ai_next)
            addr = addr->ai_next;
        return addr;
    }

    inline void join_(struct addrinfo *from) {
        auto addr = last_();
        if(addr)
            addr->ai_next = from;
        else
            list_ = from;
    }

    struct addrinfo *list_{nullptr};
};

class socket {
public:
    inline socket() = default;
    socket(const socket& from) = delete;
    auto operator=(const socket& from) = delete;

    inline socket(socket&& from) noexcept : so_(from.so_) {
        from.so_ = -1;
    }

    inline socket(const service_t& list) noexcept {
        bind(list);
    }

    inline socket(const address_t& addr, int type = 0, int protocol = 0) {
        bind(addr, type, protocol);
    }

    inline ~socket() {
        release();
    }

    inline auto operator=(socket&& from) -> socket& {
        release();
        so_ = from.so_;
        from.so_ = -1;
        return *this;
    }

    inline auto operator=(const service_t& to) -> socket& {
        connect(to);
        return *this;
    }

    inline operator bool() const {
        return so_ != -1;
    }

    inline auto operator!() const {
        return so_ == -1;
    }

    inline auto operator*() const {
        return so_;
    }

    inline void release() {
        if(so_ != -1) {
#ifdef  USE_CLOSESOCKET
            closesocket(so_);
#else
            close(so_);
#endif
            so_ = -1;
        }
    }

    inline void bind(const address_t& addr, int type = 0, int protocol = 0) {
        so_ = ::socket(addr.family(), type, protocol);
        if(so_ != -1)
            if(::bind(so_, addr.data(), addr.size()) == -1)
                release();
    }

    inline void bind(const service_t& list) {
        auto addr = *list;
        release();
        while(addr) {
            so_ = ::socket(addr->ai_family, addr->ai_socktype, addr->ai_protocol);
            if(so_ == -1)
                continue;

            if(::bind(so_, addr->ai_addr, addr->ai_addrlen) == -1) {
                release();
                continue;
            }
        }
    }

    inline auto connect(const address_t& addr) const {
        if(so_ != -1)
            return ::connect(so_, addr.data(), addr.size());
        return -1;
    }

    inline void connect(const service_t& list) {
        auto addr = *list;
        release();
        while(addr) {
            so_ = ::socket(addr->ai_family, addr->ai_socktype, addr->ai_protocol);
            if(so_ == -1)
                continue;

            if(::connect(so_, addr->ai_addr, addr->ai_addrlen) == -1) {
                release();
                continue;
            }
        }
    }

    inline void listen(int backlog = 5) {
        if(so_ != -1 && ::listen(so_, backlog) == -1)
            release();
    }

    inline auto accept() const {
        socket from{};
        if(so_ != -1)
            from.so_ = ::accept(so_, nullptr, nullptr);
        return from;
    }

    inline auto peer() const {
        address_t addr;
        socklen_t len = address_t::maxsize();
        memset(*addr, 0, sizeof(addr));
        if(so_ != -1)
            ::getpeername(so_, *addr, &len);
        return addr;
    }

    inline auto local() const {
        address_t addr;
        socklen_t len = address_t::maxsize();
        memset(*addr, 0, sizeof(addr));
        if(so_ != -1)
            ::getsockname(so_, *addr, &len);
        return addr;
    }

    inline auto send(const char *from, size_t size, int flags = 0) const -> int {
        if(so_ == -1)
            return 0;
        return static_cast<int>(::send(so_, from, size, flags));
    }

    inline auto recv(char *to, size_t size, int flags = 0) const -> int {
        if(so_ == -1)
            return 0;
        return static_cast<int>(::recv(so_, to, size, flags));
    }

    inline auto send(const char *from, size_t size, const address_t addr, int flags = 0) const -> int {
        if(so_ == -1)
            return 0;

        return static_cast<int>(::sendto(so_, from, size, flags, addr.data(), addr.size()));
    }

    inline auto recv(char *to, size_t size, address_t& addr, int flags = 0) const -> int {
        auto len = address_t::maxsize();
        if(so_ == -1)
            return 0;

        return static_cast<int>(::recvfrom(so_, to, size, flags, addr.data(), &len));
    }

protected:
    int so_{-1};
};
} // end namespace
#endif
