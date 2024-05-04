// Copyright (C) 2022 Tycho Softworks.
// This code is licensed under MIT license.

#undef  NDEBUG
#include "compiler.hpp"
#include "args.hpp"
#include "sync.hpp"
#include "print.hpp"
#include "filesystem.hpp"
#include "output.hpp"

auto main([[maybe_unused]] int argc, [[maybe_unused]] char **argv) -> int {
    const uint16_t val = 23;
    auto out = format("X{}Y", val);
    assert(out == "X23Y");
    fsys::remove("/tmp/xyz");
    fsys::path path = "/here";
    assert(format("!{}!", path) == "!/here!");
}


