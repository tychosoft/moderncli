// Copyright (C) 2022 Tycho Softworks.
// This code is licensed under MIT license.

#undef  NDEBUG
#include "compiler.hpp"
#include "args.hpp"
#include "print.hpp"
#include "filesystem.hpp"
#include "socket.hpp"

auto main([[maybe_unused]] int argc, [[maybe_unused]] char **argv) -> int { // NOLINT
    const uint16_t val = 23;
    auto out = print_format("X{}Y", val);
    assert(out == "X23Y");
    fsys::remove("/tmp/xyz");
}


