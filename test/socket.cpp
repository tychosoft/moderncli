// Copyright (C) 2022 Tycho Softworks.
// This code is licensed under MIT license.

#define MODERN_TESTING
#include "compiler.hpp"     // IWYU pragma: keep
#include "templates.hpp"
#include "socket.hpp"

namespace {
const uint16_t port = 9789;
address_t local_host("127.0.0.1", port);
address_t local_bind("*", port);
} // end anon namespace

auto main([[maybe_unused]] int argc, [[maybe_unused]] char **argv) -> int {
    using raw_t = const struct sockaddr *;
    assert(Socket::startup());
    Socket::interfaces ifs;
    assert(!ifs.empty());
    const Socket unset;
    address_t addr; // NOLINT
    raw_t raw = addr;
    uint32_t data = 0;
    auto from = unset.accept();
    assert(raw == *addr);
    assert(!is(unset));
    assert(recv(unset, data, addr, MSG_PEEK) == 0);
    assert(local_bind.is_any() == true);
    assert(local_host.is_any() == false);
    assert(local_host.port() == port);
    assert(local_bind.port() == port);
    Socket::shutdown();

    ifs = Socket::interfaces();
    auto ip = get_ipaddress_or("127.0.0.1", address_t(), port);
    assert(ip == local_host);

    address_t bind;
    assert(bind.to_format() == "none");
    bind.set("127.0.0.1", 5060);
    assert(bind.to_format() == "127.0.0.1:5060");
}

