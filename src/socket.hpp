// Copyright (C) 2023 Tycho Softworks.
// This code is licensed under MIT license.

#ifndef TYCHO_SOCKET_HPP_
#define TYCHO_SOCKET_HPP_

#include <string>
#include <string_view>
#include <ostream>
#include <stdexcept>
#include <cstring>
#include <cstdint>

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
#undef  _WIN32_WINNT
#define _WIN32_WINNT    0x0600  // NOLINT
#endif
#define USE_CLOSESOCKET
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#ifdef AF_UNIX
#include <afunix.h>
#endif
#else
#include <sys/socket.h>
#include <net/if.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/un.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>
#include <poll.h>
#include <ifaddrs.h>
#define SOCKET int
#endif

#ifndef MSG_NOSIGNAL
#define MSG_NOSIGNAL    0   // NOLINT
#endif

#ifndef IPV6_ADD_MEMBERSHIP
#define IPV6_ADD_MEMBERSHIP     IP_ADD_MEMBERSHIP
#endif

#ifndef IPV6_DROP_MEMBERSHIP
#define IPV6_DROP_MEMBERSHIP    IP_DROP_MEMBERSHIP
#endif

namespace tycho {
using multicast_t = union {
    struct ip_mreq ipv4;
    struct ipv6_mreq ipv6;
};

class address_t final {
public:
    static inline const socklen_t maxsize = sizeof(struct sockaddr_storage);

    address_t() noexcept {
        memset(&store_, 0, sizeof(store_));
    }

    address_t(const address_t& from) = default;
    auto operator=(const address_t& from) -> address_t& = default;

    explicit address_t(int family, uint16_t value = 0) noexcept {
        memset(&store_, 0, sizeof(store_));
        make_any(family);
        port(value);
    }

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

    auto operator==(const address_t& other) const noexcept {
        return memcmp(&store_, &other.store_, max_(size(), other.size())) == 0;
    }

    auto operator!=(const address_t& other) const noexcept {
        return memcmp(&store_, &other.store_, max_(size(), other.size())) != 0;
    }

    auto operator<(const address_t& other) const noexcept {
        return memcmp(&store_, &other.store_, max_(size(), other.size())) < 0;
    }

    auto operator>(const address_t& other) const noexcept {
        return memcmp(&store_, &other.store_, max_(size(), other.size())) > 0;
    }

    auto operator<=(const address_t& other) const noexcept {
        return memcmp(&store_, &other.store_, max_(size(), other.size())) <= 0;
    }

    auto operator>=(const address_t& other) const noexcept {
        return memcmp(&store_, &other.store_, max_(size(), other.size())) >= 0;
    }

    operator bool() const noexcept {
        return store_.ss_family != 0;
    }

    operator const struct sockaddr *() const noexcept {
        return reinterpret_cast<const struct sockaddr *>(&store_);
    }

    operator std::string() const {
        return to_string();
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

    auto data() const noexcept {
        return reinterpret_cast<const struct sockaddr *>(&store_);
    }

    void port_if(uint16_t value) {
        if(!port())
            port(value);
    }

    void port(uint16_t value) {
        switch(store_.ss_family) {
        case AF_INET:
            (reinterpret_cast<struct sockaddr_in *>(&store_))->sin_port = htons(value);
            break;
        case AF_INET6:
             (reinterpret_cast<struct sockaddr_in6 *>(&store_))->sin6_port = htons(value);
            break;
        default:
            break;
        }
    }

    auto host() const noexcept {
        return to_string();
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

    auto empty() const noexcept {
        return store_.ss_family == AF_UNSPEC;
    }

    auto is(int fam) const noexcept {
        return store_.ss_family == fam;
    }

    void assign_any() noexcept {
        switch(store_.ss_family) {
        case AF_INET:
            memset(&(reinterpret_cast<struct sockaddr_in*>(&store_))->sin_addr.s_addr, 0, 4);
            break;
        case AF_INET6:
            memset(&(reinterpret_cast<struct sockaddr_in6*>(&store_))->sin6_addr.s6_addr, 0, 16);
            break;
        default:
            break;
        }
    }

    auto is_any() const noexcept {
        switch(store_.ss_family) {
        case AF_INET:
            return zero_(&(reinterpret_cast<const struct sockaddr_in*>(&store_))->sin_addr.s_addr, 4);
        case AF_INET6:
            return zero_(&(reinterpret_cast<const struct sockaddr_in6*>(&store_))->sin6_addr.s6_addr, 16);
        default:
            return false;
        }
    }

#ifdef  AF_UNIX
    auto un() const noexcept {
        return store_.ss_family == AF_UNIX ? reinterpret_cast<const struct sockaddr_un*>(&store_) : nullptr;
    }
#endif

    auto in() const noexcept {
        return store_.ss_family == AF_INET ? reinterpret_cast<const struct sockaddr_in*>(&store_) : nullptr;
    }

    auto in6() const noexcept {
        return store_.ss_family == AF_INET6 ? reinterpret_cast<const struct sockaddr_in6*>(&store_) : nullptr;
    }

    auto assign_in6(const struct sockaddr *addr) noexcept {
        if(!addr || (addr->sa_family != AF_INET && addr->sa_family != AF_INET6)) return false;
        if(addr->sa_family == AF_INET6) {
            set(addr);
            return true;
        }

        auto addr6 = reinterpret_cast<struct sockaddr_in6 *>(&store_);
        auto addr4 = reinterpret_cast<const struct sockaddr_in *>(addr);
        memset(addr6, 0, sizeof(struct sockaddr_in6));
        addr6->sin6_family = AF_INET6;
        addr6->sin6_port = addr4->sin_port;
        addr6->sin6_addr.s6_addr[10] = 0xff;
        addr6->sin6_addr.s6_addr[11] = 0xff;
        // FlawFinder: ignore
        memcpy(&addr6->sin6_addr.s6_addr[12], &addr4->sin_addr, sizeof(addr4->sin_addr));
        return true;
    }

    void assign(const struct sockaddr_storage& from) noexcept {
        store_ = from;
    }

    void set(const std::string& str, uint16_t in_port = 0, int any = AF_INET) noexcept {
        memset(&store_, 0, sizeof(store_));
        auto cp = str.c_str();
        if(str == "*" && any == AF_INET6) {
            auto ipv6 = reinterpret_cast<struct sockaddr_in6*>(&store_);
            make_any(AF_INET6);
            ipv6->sin6_family = AF_INET6;
            ipv6->sin6_port = htons(in_port);
        }
#ifdef  AF_UNIX
        else if(strchr(cp, '/') != nullptr) {
            auto sun = reinterpret_cast<struct sockaddr_un *>(&store_);
            sun->sun_family = AF_UNIX;
            auto tp = sun->sun_path;
            auto ep = tp + sizeof(sun->sun_path) - 1;
            while(*cp && tp < ep)
                *(tp++) = *(cp++);
            *tp = 0;
        }
#endif
        else if(strchr(cp, ':') != nullptr) {
            auto ipv6 = reinterpret_cast<struct sockaddr_in6*>(&store_);
            if(str == "::*" || str == "::" || str == "[::]")
                make_any(AF_INET6);
            else if(inet_pton(AF_INET6, cp, &(ipv6->sin6_addr)) < 1)
                return;
            ipv6->sin6_family = AF_INET6;
            ipv6->sin6_port = htons(in_port);
        } else if((str == "*") || (strchr(cp, '.') != nullptr)) {
            auto ipv4 = reinterpret_cast<struct sockaddr_in*>(&store_);
            if(str == "*")
                make_any(AF_INET);
            else if(inet_pton(AF_INET, cp, &(ipv4->sin_addr)) < 1)
                return;
            ipv4->sin_family = AF_INET;
            ipv4->sin_port = htons(in_port);
        }
    }

    auto to_string() const -> std::string {
        char buf[256];
        const struct sockaddr_in *ipv4{nullptr};
        const struct sockaddr_in6 *ipv6{nullptr};
        switch(store_.ss_family) {
        case AF_INET:
            if(is_any()) return {"*"};
            ipv4 = reinterpret_cast<const struct sockaddr_in*>(&store_);
            if(::inet_ntop(AF_INET, &(ipv4->sin_addr), buf, sizeof(buf))) return {buf};
            break;
        case AF_INET6:
            if(is_any()) return {"::"};
            ipv6 = reinterpret_cast<const struct sockaddr_in6*>(&store_);
            if(::inet_ntop(AF_INET, &(ipv6->sin6_addr), buf, sizeof(buf))) return {buf};
            break;
#ifdef  AF_UNIX
        case AF_UNIX: {
            auto sun = reinterpret_cast<const struct sockaddr_un*>(&store_);
            return {sun->sun_path};
        }
#endif
        default:
            break;
        }
        return {};
    }

    auto put(struct sockaddr_storage& out) const noexcept {
        // FlawFinder: ignore
        memcpy(&out, &store_, sizeof(store_));
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
    struct sockaddr_storage store_{AF_UNSPEC};

    void make_any(int family) noexcept {
        switch(family) {
        case AF_INET:
            memset(&(reinterpret_cast<struct sockaddr_in*>(&store_))->sin_addr.s_addr, 0, 4);
            store_.ss_family = AF_INET;
            break;
        case AF_INET6:
            memset(&(reinterpret_cast<struct sockaddr_in6*>(&store_))->sin6_addr.s6_addr, 0, 16);
            store_.ss_family = AF_INET6;
            break;
        default:
            break;
        }
    }

    auto size_(int family) const noexcept -> socklen_t {
        switch(family) {
        case AF_INET:
            return sizeof(struct sockaddr_in);
        case AF_INET6:
            return sizeof(struct sockaddr_in6);
#ifdef  AF_UNIX
        case AF_UNIX: {
            const auto sun = reinterpret_cast<const struct sockaddr_un *>(&store_);
            // FlawFinder: ignore
            return offsetof(struct sockaddr_un, sun_path) + strlen(sun->sun_path);
        }
#endif
        default:
            return sizeof(store_);
        }
    }

    static auto max_(socklen_t x, socklen_t y) -> socklen_t {
        return (x > y) ? x : y;
    }

    static auto zero_(const void *addr, std::size_t size) -> bool {
        auto ptr = static_cast<const std::byte *>(addr);
        while(size--) {
            if(*ptr != std::byte(0)) return false;
            ++ptr;
        }
        return true;
    }
};

#ifndef EAI_ADDRFAMILY
#define EAI_ADDRFAMILY EAI_NODATA + 2000    // NOLINT
#endif

#ifndef EAI_SYSTEM
#define EAI_SYSTEM EAI_NODATA + 2001        // NOLINT
#endif

class Socket {
public:
    class service final {
    public:
        service() = default;
        service(const service& from) = delete;
        auto operator=(const service& from) = delete;

        service(service&& from) noexcept : list_(from.list_) {
            from.list_ = nullptr;
        }

        explicit service(const std::string& host, const std::string& svc = "", int family = AF_UNSPEC, int type = SOCK_STREAM, int protocol = 0) noexcept {
            set(host, svc, family, type, protocol);
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

        operator const struct addrinfo *() const noexcept {
            return list_;
        }

        auto operator!() const noexcept {
            return list_ == nullptr;
        }

        auto operator[](unsigned index) const noexcept {
            auto size{0U};
            auto addr = list_;
            while(addr) {
                if(size == index) return addr;
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
            err_ = 0;
        }

        void set(const std::string& host = "*", const std::string& service = "", int family = AF_UNSPEC, int type = SOCK_STREAM, int protocol = 0) noexcept {
            release();
            auto svc = service.c_str();
            if(service.empty() || service == "0")
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

            err_ = getaddrinfo(addr, svc, &hint, &list_);
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

        template <typename Func>
        auto each(Func func) const {
            auto list = list_;
            while(list) {
                if(!func(list))
                    return false;
                list = list->ai_next;
            }
            return true;
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
        mutable int err_{0};
    };

#ifdef USE_CLOSESOCKET
    class interfaces final {
    public:
        interfaces(const interfaces& from) = delete;
        auto operator=(const interfaces&) -> interfaces& = delete;

        interfaces() noexcept {
            ULONG bufsize{8192};
            // cppcheck-suppress useInitializationList
            list_ = static_cast<PIP_ADAPTER_ADDRESSES>(malloc(bufsize));    // NOLINT
            if(list_ == nullptr) return;

            auto result = GetAdaptersAddresses(AF_UNSPEC, GAA_FLAG_INCLUDE_PREFIX, nullptr, list_, &bufsize);
            if (result == ERROR_BUFFER_OVERFLOW) {
                free(list_); // NOLINT
                list_ = static_cast<PIP_ADAPTER_ADDRESSES>(malloc(bufsize));    // NOLINT
                result = GetAdaptersAddresses(AF_UNSPEC, GAA_FLAG_INCLUDE_PREFIX, nullptr, list_, &bufsize);
            };

            if(result != NO_ERROR) {
                free(list_);    // NOLINT
                list_ = nullptr;
                return;
            }

            for(auto entry = list_; entry != nullptr; entry = entry->Next) {
                for(auto unicast = entry->FirstUnicastAddress; unicast != nullptr; unicast = unicast->Next) {
                    ++count_;
                }
            }
        }

        ~interfaces() {
            if(list_)
                free(list_);    // NOLINT
            list_ = nullptr;
        }

        operator bool() const noexcept {
            return count_ > 0;
        }

        auto operator!() const noexcept {
            return count_ == 0;
        }

        auto operator[](std::size_t index) const noexcept -> const struct sockaddr *{
            if(index >= count_) return nullptr;
            auto entry = list_;
            while(entry && index) {
                auto unicast = entry->FirstUnicastAddress;
                while(unicast && index) {
                    --index;
                    unicast = unicast->Next;
                }
                if(unicast && !index && unicast->Address.lpSockaddr) return unicast->Address.lpSockaddr;
            }
            return nullptr;
        }

        auto empty() const noexcept {
            return count_ == 0;
        }

        auto size() const noexcept {
            return count_;
        }

        auto find(const std::string_view& id, int family) const noexcept -> const sockaddr * {
            for(auto entry = list_; entry != nullptr; entry = entry->Next) {
                auto unicast = entry->FirstUnicastAddress;
                while(unicast) {
                    if(entry->AdapterName && id == entry->AdapterName && unicast->Address.lpSockaddr && unicast->Address.lpSockaddr->sa_family == family) return unicast->Address.lpSockaddr;
                    unicast = unicast->Next;
                }
            }
            return nullptr;
        }

        auto mask(std::size_t index) const noexcept {
            if(index >= count_) return 0;
            auto entry = list_;
            while(entry && index) {
                auto unicast = entry->FirstUnicastAddress;
                while(unicast && index) {
                    --index;
                    unicast = unicast->Next;
                }
                if(unicast && !index) return int(unicast->OnLinkPrefixLength);
            }
            return 0;
        }

        auto name(std::size_t index) const noexcept -> std::string {
            if(index >= count_) return {};
            auto entry = list_;
            while(entry && index) {
                auto unicast = entry->FirstUnicastAddress;
                while(unicast && index) {
                    --index;
                    unicast = unicast->Next;
                }
                if(unicast && !index) return {entry->AdapterName};
            }
            return {};
        }

        auto ifaddr(const std::string_view& id, int family) const noexcept -> const struct sockaddr *{
            for(auto entry = list_; entry != nullptr; entry = entry->Next) {
                auto unicast = entry->FirstUnicastAddress;
                while(unicast) {
                    if(entry->AdapterName && id == entry->AdapterName && unicast->Address.lpSockaddr && unicast->Address.lpSockaddr->sa_family == family) return unicast->Address.lpSockaddr;;
                    unicast = unicast->Next;
                }
            }
            return nullptr;
        }

        auto native_list() const noexcept {
            return list_;
        }

    private:
        PIP_ADAPTER_ADDRESSES list_{nullptr};
        std::size_t count_{0};
    };

#else
    class interfaces final {
    public:
        interfaces(const interfaces& from) = delete;
        auto operator=(const interfaces&) -> interfaces& = delete;

        interfaces() noexcept {
            getifaddrs(&list_);
            for(auto entry = list_; entry != nullptr; entry = entry->ifa_next) {
                ++count_;
            }
        }

        ~interfaces() {
            freeifaddrs(list_);
            list_ = nullptr;
        }

        operator bool() const noexcept {
            return count_ > 0;
        }

        auto operator!() const noexcept {
            return count_ == 0;
        }

        auto operator[](std::size_t index) const noexcept -> const struct sockaddr *{
            if(index >= count_) return nullptr;
            auto entry = list_;
            while(entry && index--)
                entry = entry->ifa_next;
            if(!entry|| !entry->ifa_addr) return nullptr;
            return entry->ifa_addr;
        }

        auto empty() const noexcept {
            return count_ == 0;
        }

        auto size() const noexcept {
            return count_;
        }

        auto find(const std::string_view& id, int family) const noexcept -> const sockaddr * {
            for(auto entry = list_; entry != nullptr; entry = entry->ifa_next) {
                if(entry->ifa_name && id == entry->ifa_name && entry->ifa_addr->sa_family == family)
                    return entry->ifa_addr;
            }
            return nullptr;
        }

        auto mask(std::size_t index) const noexcept {
            if(index >= count_) return 0;
            auto entry = list_;
            while(entry && index--)
                entry = entry->ifa_next;

            if(!entry|| !entry->ifa_addr || !entry->ifa_netmask) return 0;
            switch(entry->ifa_netmask->sa_family) {
            case AF_INET: {
                auto ipv4 = reinterpret_cast<const struct sockaddr_in*>(entry->ifa_netmask);
                return prefix_ipv4(ntohl(ipv4->sin_addr.s_addr));
            }
            case AF_INET6: {
                auto ipv6 = reinterpret_cast<const struct sockaddr_in6*>(entry->ifa_netmask);
                return prefix_ipv6(ipv6->sin6_addr.s6_addr);
            }
            default:
                return 0;
            }
        }

        auto name(std::size_t index) const noexcept -> std::string {
            if(index >= count_) return {};
            auto entry = list_;
            while(entry && index--)
                entry = entry->ifa_next;
            return {(entry && entry->ifa_name) ? entry->ifa_name : ""};
        }

        auto ifaddr(const std::string_view& id, int family) const noexcept -> const sockaddr *{
            for(auto entry = list_; entry != nullptr; entry = entry->ifa_next) {
                if(entry->ifa_name && id == entry->ifa_name && entry->ifa_addr->sa_family == family) return entry->ifa_addr;
            }
            return nullptr;
        }

        auto native_list() const noexcept {
            return list_;
        }

    private:
        struct ifaddrs *list_{nullptr};
        std::size_t count_{0};

        static auto prefix_ipv4(uint32_t mask) noexcept -> int {
            auto count = 0;
            while (mask & 0x80000000) {
                count++;
                mask <<= 1;
            }
            return count;
        }

        static auto prefix_ipv6(const uint8_t *addr) noexcept -> int {
            auto count = 0;
            for (int i = 0; i < 16; i++) {
                std::uint8_t byte = addr[i];
                for(int j = 0; j < 8; j++) {
                    if (byte & 0x80) {
                        count++;
                        byte <<= 1;
                    }
                    else
                        return count;
                }
            }
            return count;
        }
    };
#endif

    Socket() = default;

    explicit Socket(int family, int type = 0, int protocol = 0) :
    so_(set_error(make_socket(::socket(family, type, protocol)))) {}

    Socket(const Socket& from) = delete;
    auto operator=(const Socket& from) = delete;

    Socket(Socket&& from) noexcept : so_(from.so_), err_(from.err_) {
        from.so_ = -1;
    }

    explicit Socket(SOCKET from) noexcept : so_(make_socket(from)) {
        set_error(so_);
    }

    explicit Socket(const Socket::service& list) noexcept {
        bind(list);
    }

    explicit Socket(const address_t& addr, int type = 0, int protocol = 0) noexcept {
        bind(addr, type, protocol);
    }

    ~Socket() {
        release();
    }

    auto operator=(SOCKET from) noexcept -> auto& {
        release();
        so_ = make_socket(from);
        set_error(so_);
        return *this;
    }

    auto operator=(Socket&& from) noexcept -> auto& {
        release();
        so_ = from.so_;
        err_ = from.err_;
        from.so_ = -1;
        return *this;
    }

    auto operator=(const Socket::service& to) noexcept -> auto& {
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

    auto err() const noexcept {
        return err_;
    }

    void release() noexcept {
        auto so = so_;
        so_ = -1;
        if(so != -1) {
#ifdef  USE_CLOSESOCKET
            ::shutdown(so, SD_BOTH);
            ::closesocket(so);
#else
            ::shutdown(so, SHUT_RDWR);
            ::close(so);
#endif
        }
    }

    void bind(const address_t& addr, int type = 0, int protocol = 0) noexcept {
        release();
        so_ = set_error(make_socket(::socket(addr.family(), type, protocol)));
        if(so_ != -1) {
            reuse(true);
            if(set_error(::bind(so_, addr.data(), addr.size())) == -1)
                release();
        }
        else
            err_ = EBADF;
    }

    void bind(const Socket::service& list) noexcept {
        auto addr = *list;
        release();
        while(addr) {
            so_ = set_error(make_socket(::socket(addr->ai_family, addr->ai_socktype, addr->ai_protocol)));
            if(so_ == -1) continue;
            if(set_error(::bind(so_, addr->ai_addr, make_socklen(addr->ai_addrlen)) == -1)) {
                release();
                continue;
            }
        }
    }

    auto connect(const address_t& addr) const noexcept {
        if(so_ != -1) return set_error(::connect(so_, addr.data(), addr.size()));
        return -1;
    }

    void connect(const Socket::service& list) noexcept {
        auto addr = *list;
        release();
        while(addr) {
            so_ = set_error(make_socket(::socket(addr->ai_family, addr->ai_socktype, addr->ai_protocol)));
            if(so_ == -1) continue;
            if(set_error(::connect(so_, addr->ai_addr, make_socklen(addr->ai_addrlen))) == -1) {
                release();
                continue;
            }
        }
    }

    auto join(const address_t& member, unsigned ifindex = 0) {
        if(so_ == -1) return EBADF;
        const address_t from = local();
        if(from.family() != member.family()) return set_error(-EAI_FAMILY);
        auto res = 0;
        multicast_t multicast;
        memset(&multicast, 0, sizeof(multicast));
        switch(member.family()) {
        case AF_INET:
            multicast.ipv4.imr_interface.s_addr = INADDR_ANY;
            multicast.ipv4.imr_multiaddr = member.in()->sin_addr;
            if(setsockopt(so_, IPPROTO_IP, IP_ADD_MEMBERSHIP, opt_cast(&multicast), sizeof(multicast.ipv4)) == -1)
                res = errno;
            break;
        case AF_INET6:
            multicast.ipv6.ipv6mr_interface = ifindex;
            multicast.ipv6.ipv6mr_multiaddr = member.in6()->sin6_addr;
            if(setsockopt(so_, IPPROTO_IPV6, IPV6_ADD_MEMBERSHIP, opt_cast(&multicast), sizeof(multicast.ipv6)) == -1)
                res = errno;
            break;
        default:
            res = EAI_FAMILY;
        }
        err_ = res;
        return res;
    }

    auto join(const Socket::service& list, unsigned ifindex) noexcept {
        auto addr = *list;
        auto count = 0;

        while(addr && join(address_t(addr->ai_addr), ifindex) == 0)
            ++count;
        return count;
    }

    auto drop(const address_t& member, unsigned ifindex = 0) {
        if(so_ == -1) return EBADF;
        const address_t from = local();
        if(from.family() != member.family()) return set_error(-EAI_FAMILY);
        auto res = 0;
        multicast_t multicast;
        memset(&multicast, 0, sizeof(multicast));
        switch(member.family()) {
        case AF_INET:
            multicast.ipv4.imr_interface.s_addr = INADDR_ANY;
            multicast.ipv4.imr_multiaddr = member.in()->sin_addr;
            if(setsockopt(so_, IPPROTO_IP, IP_DROP_MEMBERSHIP, opt_cast(&multicast), sizeof(multicast.ipv4)) == -1)
                res = errno;
            break;
        case AF_INET6:
            multicast.ipv6.ipv6mr_interface = ifindex;
            multicast.ipv6.ipv6mr_multiaddr = member.in6()->sin6_addr;
            if(setsockopt(so_, IPPROTO_IPV6, IPV6_DROP_MEMBERSHIP, opt_cast(&multicast), sizeof(multicast.ipv6)) == -1)
                res = errno;
            break;
        default:
            res = EAI_FAMILY;
        }
        err_ = res;
        return res;
    }

    auto drop(const Socket::service& list, unsigned ifindex) noexcept {
        auto addr = *list;
        auto count = 0;

        while(addr && drop(address_t(addr->ai_addr), ifindex) == 0)
            ++count;
        return count;
    }

    auto wait(short events, int timeout = -1) const noexcept -> int {
        struct pollfd pfd{};
        pfd.fd = so_;
        pfd.events = events;
        pfd.revents = 0;
        set_error(Socket::poll(&pfd, 1, timeout));
        return pfd.revents;
    }

    void reuse(bool flag) {
        int opt = flag ? 1 : 0;
        set_error(setsockopt(so_, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char*>(&opt), sizeof(opt)));

#ifdef SO_REUSEPORT
        set_error(setsockopt(so_, SOL_SOCKET, SO_REUSEPORT, reinterpret_cast<const char *>(&opt), sizeof(opt)));
#endif
    }

    void listen(int backlog = 5) noexcept {
        if(so_ != -1 && set_error(::listen(so_, backlog)) == -1)
            release();
    }

    void assign(int so) noexcept {
        release();
        so_ = so;
        err_ = 0;
    }

    auto accept() const noexcept {
        Socket from{};
        if(so_ != -1) {
            from.so_ = make_socket(::accept(so_, nullptr, nullptr));
            set_error(from.so_);
            from.set_error(from.so_);
        }
        return from;
    }

    template <typename Func>
    auto accept(Func acceptor) const {
        if(so_ == -1) return false;
        address_t remote;
        auto slen = address_t::maxsize;
        auto to = set_error(make_socket(::accept(so_, *remote, &slen)));
        if(to < 0) return false;
        if(!acceptor(to, *remote)) {
#ifdef  USE_CLOSESOCKET
            closesocket(to);
#else
            ::close(to);
#endif
            err_ = ECONNREFUSED;
            return false;
        }
        return true;
    }

    auto peer() const noexcept {
        address_t addr;
        socklen_t len = address_t::maxsize;
        if(so_ != -1)
            set_error(::getpeername(so_, *addr, &len));
        return addr;
    }

    auto local() const noexcept -> address_t {
        address_t addr;
        socklen_t len = address_t::maxsize;
        if(so_ != -1)
            set_error(::getsockname(so_, *addr, &len));
        return addr;
    }

    auto send(const void *from, std::size_t size, int flags = 0) const noexcept {
        if(so_ == -1) return io_error(-EBADF);
        return io_error(::send(so_, static_cast<const char *>(from), int(size), flags));
    }

    auto recv(void *to, std::size_t size, int flags = 0) const noexcept {
        if(so_ == -1) return io_error(-EBADF);
        return io_error(::recv(so_, static_cast<char *>(to), int(size), flags));
    }

    auto send(const void *from, std::size_t size, const address_t& addr, int flags = 0) const noexcept {
        if(so_ == -1) return io_error(-EBADF);
        return io_error(::sendto(so_, static_cast<const char *>(from), int(size), flags, addr.data(), addr.size()));
    }

    auto recv(void *to, std::size_t size, address_t& addr, int flags = 0) const noexcept {
        auto len = address_t::maxsize;
        if(so_ == -1) return io_error(-EBADF);
        return io_error(::recvfrom(so_, static_cast<char *>(to), int(size), flags, addr.data(), &len));
    }

#ifdef USE_CLOSESOCKET
    static auto poll(struct pollfd *fds, std::size_t count, int timeout) noexcept -> int {
        return WSAPoll(fds, count, timeout);
    }

    static auto if_index(const std::string& name) noexcept -> unsigned {
        NET_LUID uid;
        if(ConvertInterfaceNameToLuidA(name.c_str(), &uid) != NO_ERROR) return 0;
        NET_IFINDEX idx{0};
        if(ConvertInterfaceLuidToIndex(&uid, &idx) != NO_ERROR) return 0;
        return idx;
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
    static auto poll(struct pollfd *fds, std::size_t count, int timeout) noexcept -> int {
        return ::poll(fds, count, timeout);
    }

    static auto if_index(const std::string& name) noexcept -> unsigned {
        return if_nametoindex(name.c_str());
    }

    static auto startup() noexcept {
        return true;
    }

    static void shutdown() noexcept {
    }
#endif

protected:
    volatile int so_{-1};
    mutable int err_{0};

    auto io_error(ssize_t size) const noexcept -> std::size_t {
        if(size == -1) {
            size = 0;
            err_ = errno;
        }
        else if(size < 0) {
            err_ = int(-size);
            size = 0;
        }
        else
            err_ = 0;
        return std::size_t(size);
    }

    auto set_error(int code) const noexcept -> int {
        if(code == -1)
            err_ = errno;
        else
            err_ = 0;
        return code;
    }

    static auto opt_cast(const void *from) -> const char * {
        return static_cast<const char *>(from);
    }

private:
#ifdef USE_CLOSESOCKET
    static auto make_socket(SOCKET so) noexcept -> int {
        return static_cast<int>(so);
    }

    static auto make_socklen(std::size_t size) noexcept -> socklen_t {
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
using service_t = Socket::service;
using interface_t = Socket::interfaces;

template<typename T>
inline auto send(const Socket& sock, const T& msg, int flags = 0) {
    static_assert(std::is_trivial_v<T>, "T must be Trivial type");

    return sock.send(&msg, sizeof(msg), flags);
}

template<typename T>
inline auto recv(const Socket& sock, T& msg, int flags = 0) {
    static_assert(std::is_trivial_v<T>, "T must be Trivial type");

    return sock.recv(&msg, sizeof(msg), flags);
}

template<typename T>
inline auto send(const Socket& sock, const T& msg, const address_t& addr, int flags = 0) {
    static_assert(std::is_trivial_v<T>, "T must be Trivial type");

    return sock.send(&msg, sizeof(msg), addr, flags);
}

template<typename T>
inline auto recv(const Socket& sock, T& msg, address_t& addr, int flags = 0) {
    static_assert(std::is_trivial_v<T>, "T must be Trivial type");

    return sock.recv(&msg, sizeof(msg), addr, flags);
}

// internet helper utils...

inline auto inet_size(const struct sockaddr *addr) noexcept -> socklen_t {
    if(!addr) return 0;
    switch(addr->sa_family) {
    case AF_INET:
        return sizeof(struct sockaddr_in);
    case AF_INET6:
        return sizeof(struct sockaddr_in6);
#ifdef  AF_UNIX
    case AF_UNIX: {
        const auto un = reinterpret_cast<const struct sockaddr_un *>(addr);
        // FlawFinder: ignore
        return offsetof(struct sockaddr_un, sun_path) + strlen(un->sun_path);
    }
#endif
    default:
        return 0;
    }
}

inline auto inet_port(const struct sockaddr *addr) noexcept -> uint16_t {
    if(!addr) return 0;
    switch(addr->sa_family) {
    case AF_INET:
        return ntohs((reinterpret_cast<const struct sockaddr_in *>(addr))->sin_port);
    case AF_INET6:
        return ntohs((reinterpret_cast<const struct sockaddr_in6 *>(addr))->sin6_port);
    default:
        return 0;
    }
}

inline auto system_hostname() noexcept -> std::string {
    char buf[256]{0};
    auto result = gethostname(buf, sizeof(buf));

    if(result != 0) return {};
    buf[sizeof(buf) - 1] = 0;
    return {buf};
}

inline auto inet_host(const struct sockaddr *addr) noexcept -> std::string {
    if(!addr) return {};
    const address_t address(addr);
    return address.to_string();
}

inline auto inet_family(const std::string& host, int any = AF_UNSPEC) {
    int dots = 0;
#ifdef  AF_UNIX
    if(host.find('/') != std::string::npos) return AF_UNIX;
#endif
    if(host[0] == '[' || host.find(':') != std::string::npos) return AF_INET6;
    for(const auto ch : host) {
        if(ch == '.') {
            ++dots;
            continue;
        }
        if(ch < '0' || ch > '9') return any;
    }
    if(dots == 3) return AF_INET;
    return AF_UNSPEC;
}

inline auto inet_in4(struct sockaddr_storage& store) {
    return store.ss_family == AF_INET ? reinterpret_cast<struct sockaddr_in *>(&store) : nullptr;
}

inline auto inet_in6(struct sockaddr_storage& store) {
    return store.ss_family == AF_INET6 ? reinterpret_cast<struct sockaddr_in6 *>(&store) : nullptr;
}

#ifdef  AF_UNIX
inline auto inet_un(struct sockaddr_storage& store) {
    return store.ss_family == AF_UNIX ? reinterpret_cast<struct sockaddr_un *>(&store) : nullptr;
}
#endif

inline auto inet_any(const std::string& host, int any = AF_INET) {
    if(host == "*") return any;
    if(host == "0.0.0.0") return AF_INET;
    if(host == "[*]" || host == "::" || host == "[::]") return AF_INET6;
    return AF_UNSPEC;
}

inline auto inet_find(const std::string& host_id, const std::string& service = "", int family = AF_UNSPEC, int type = 0, int protocol = 0) {
    auto host(host_id);
    uint16_t port = 0;
    try {
        port = std::stoi(service);
    } catch(...) {
        port = 0;
    }

    if(family != AF_INET6 && (host == "loopback" || host == "localhost"))
        host = "127.0.0.1";
    else if(family != AF_INET && (host == "loopback6" || host == "localhost6" || host == "loopback" || host == "localhost"))
        host = "::1";
    else if(host.empty())
        host = system_hostname();

    address_t address(host, port);
    if(!address.empty()) return address;
    const char *svc = service.c_str();
    if(service.empty() || service == "0")
        svc = nullptr;

    struct addrinfo hints{};
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = family;
    hints.ai_socktype = type;
    hints.ai_protocol = protocol;

    if(port > 0)
        hints.ai_flags |= AI_NUMERICSERV;

    if(family != AF_UNSPEC)
        hints.ai_flags |= AI_ADDRCONFIG;

    struct addrinfo *list{nullptr};
    auto result = getaddrinfo(host.c_str(), svc, &hints, &list);
    if(list && !result)
        address = list;
    if(list)
        freeaddrinfo(list);

    if(port && !address.empty())
        address.port_if(port);
    return address;
}

inline auto inet_bind(const std::string& host, const std::string& service = "", int family = AF_UNSPEC, int type = 0, int protocol = 0) {
    if(host.empty()) return inet_find(host, service, family, type, protocol);
    uint16_t port = 0;
    try {
        port = std::stoi(service);
    } catch(...) {
        port = 0;
    }

#ifdef AF_UNIX
    if((family == AF_UNIX || family == AF_UNSPEC) && host.find_first_of('/') != std::string::npos) {
        return address_t(host);
    }
#endif

    if((family != AF_INET6) && (host == "any" || host == "*")) return address_t(AF_INET, port);
    if((family != AF_INET) && (host == "any6" || host == "[*]" || host == "::" || host == "::*" || host == "any" || host == "*")) return address_t(AF_INET6, port);
    if(host.find_first_of(".") == std::string::npos) {
        const Socket::interfaces ifa;
        auto addr = ifa.find(host, AF_INET);
        auto addr6 = ifa.find(host, AF_INET6);
        if(family == AF_INET)
            addr6 = nullptr;
        else if(family == AF_INET6)
            addr = nullptr;
        if((!addr && addr6) || (addr6 && family == AF_INET6)) return address_t(addr6);
        if(addr) return address_t(addr);
    }
    return inet_find(host, service, family, type, protocol);
}

inline auto inet_bind(const std::string& host, uint16_t port, int family = AF_UNSPEC, int type = 0, int protocol = 0) {
    return inet_bind(host, std::to_string(port), family, type, protocol);
}

inline auto is_ipv4(const std::string_view& addr) {
    // cppcheck-suppress useStlAlgorithm
    for(const auto ch : addr) { // NOLINT
        if(ch != '.' && !isdigit(ch)) return false;
    }
    return true;
}

inline auto is_ipv6(const std::string_view& addr) {
    return addr.find_first_of(':') != std::string_view::npos;
}

#ifdef AF_UNIX
inline auto is_unix(const std::string_view& addr) {
    return addr.find_first_of('/') != std::string_view::npos;
}
#endif

inline auto get_ipaddress(const std::string_view& from, uint16_t port = 0) {
    address_t address;
    if(is_ipv4(from) || is_ipv6(from))
        address.set(std::string{from}, port);
    if(address.empty()) throw std::out_of_range("Invalid ip address");
    return address;
}

inline auto get_ipaddress_or(const std::string_view& from, address_t or_else = address_t(), uint16_t port = 0) {
    address_t address;
    if(is_ipv4(from) || is_ipv6(from))
        address.set(std::string{from}, port);
    if(address.empty()) return or_else;
    return address;
}
} // end namespace

namespace std {
template<>
struct hash<tycho::address_t> {
    auto operator()(const tycho::address_t& obj) const {
        std::size_t result{0U};
        const auto data = reinterpret_cast<const char *>(obj.data());
        for(socklen_t pos = 0; pos < obj.size(); ++pos) {
            result = (result * 131) + data[pos];
        }
        return result;
    }
};
} // end namespace

inline auto operator<<(std::ostream& out, const tycho::address_t& addr) -> std::ostream& {
    out << addr.to_string();
    return out;
}
#endif
