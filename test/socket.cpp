// Copyright (C) 2022 Tycho Softworks.
// This code is licensed under MIT license.

#undef  NDEBUG
#include "compiler.hpp"     // IWYU pragma: keep
#include "print.hpp"        // IWYU pragma: keep
#include "templates.hpp"
#include "socket.hpp"

auto main([[maybe_unused]] int argc, [[maybe_unused]] char **argv) -> int {
    assert(tycho::socket::startup());
    const socket_t unset;
    address_t addr;
    uint32_t data = 0;
    assert(!is(unset));
    assert(recv(unset, data, addr, MSG_PEEK) == 0);
    tycho::socket::shutdown();
}


