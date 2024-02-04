// Copyright (C) 2022 Tycho Softworks.
// This code is licensed under MIT license.

#undef  NDEBUG
#include "compiler.hpp"
#include "socket.hpp"

auto main([[maybe_unused]] int argc, [[maybe_unused]] char **argv) -> int {
    const socket_t unset;
    assert(!is(unset));
}


