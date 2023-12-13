// Copyright (C) 2023 Tycho Softworks.
// This code is licensed under MIT license.

#undef  NDEBUG
#include "compiler.hpp"
#include "endian.hpp"

auto main([[maybe_unused]] int argc, [[maybe_unused]] char **argv) -> int {
    const uint8_t bytes[] = {0x01, 0x02};

    assert(be_get16(bytes) == 258U);
    assert(le_get16(bytes) == 513U);
}


