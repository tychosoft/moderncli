// Copyright (C) 2022 Tycho Softworks.
// This code is licensed under MIT license.

#undef  NDEBUG
#include "compiler.hpp"     // IWYU pragma: keep
#include "templates.hpp"
#include "print.hpp"
#include "secure.hpp"
#include "socket.hpp"

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
            println(tcp, "Not Send, compile test");
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
            tcpstream tcp(local_host);
            assert(tcp.is_open() == true);
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


