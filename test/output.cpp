// Copyright (C) 2022 Tycho Softworks.
// This code is licensed under MIT license.

#undef  NDEBUG
#include "compiler.hpp"     // IWYU pragma: keep
#include "output.hpp"
#include "datetime.hpp"

using namespace tycho;

auto main([[maybe_unused]] int argc, [[maybe_unused]] char **argv) -> int {
    auto now = gmt_time(time(nullptr));
    output() << "hello " << "world, " << to_string(now, GENERIC_DATETIME);
}


