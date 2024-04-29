// Copyright (C) 2022 Tycho Softworks.
// This code is licensed under MIT license.

#undef  NDEBUG
#include "compiler.hpp"
#include "args.hpp"
#include "sync.hpp"
#include "print.hpp"
#include "filesystem.hpp"

auto main([[maybe_unused]] int argc, [[maybe_unused]] char **argv) -> int { // NOLINT
    const uint16_t val = 23;
    auto out = format("X{}Y", val);
    assert(out == "X23Y");
    fsys::remove("/tmp/xyz");
    fsys::path path = "/here";
    assert(format("!{}!", path) == "!/here!");

    auto now = local_time(time_t{1714422317});
    assert(format("{}", now) == "2024-04-29 16:25:17");
    assert(iso_date(now) == "2024-04-29");
    assert(iso_time(now) == "16:25:17");
}


