// Copyright (C) 2022 Tycho Softworks.
// This code is licensed under MIT license.

#undef  NDEBUG
#include "compiler.hpp"     // IWYU pragma: keep
#include "templates.hpp"
#include "print.hpp"
#include "secure.hpp"
#include "socket.hpp"
#include <cstdlib>

namespace {
const uint16_t port = 9789;
address_t local_host("127.0.0.1", port);
} // end anon namespace

auto main([[maybe_unused]] int argc, [[maybe_unused]] char **argv) -> int {
    assert(Socket::startup());
    const Socket unset;
    try {
        assert(!unset.accept([](int so, const struct sockaddr *peer) {
            // could return false if filtering...
            // could dispatch and do tcpstream::accept in a thread...
            tcpstream tcp(so, peer);
            print(tcp, "Not Send, compile test\n");
            return true;
        }));
        [[maybe_unused]] tcpstream tcp(static_cast<const struct sockaddr *>(nullptr));        // NOLINT
    }
    catch(...) {
        // creates a connected socket...
        try {
            Socket dummy(local_host, SOCK_STREAM);
            assert(dummy.err() == 0);
            assert(is(dummy));
            dummy.listen();
            assert(dummy.err() == 0);
            tcpstream tcp_orig(local_host);
            tcpstream tcp(std::move(tcp_orig));
            assert(tcp.is_open() == true);
            assert(tcp_orig.is_open() == false);    // NOLINT
            print(tcp, "HERE");
            assert(tcp.out_pending() == 4);
            assert(tcp.in_avail() == 0);
        }
        catch(...) {
            ::exit(1);
        }
        ::exit(0);
    }
    ::exit(1);
}


