// Copyright (C) 2023 Tycho Softworks.
// This code is licensed under MIT license.

#undef  NDEBUG
#include "compiler.hpp"
#include "atomics.hpp"
#include <cstdint>

auto main([[maybe_unused]] int argc, [[maybe_unused]] char **argv) -> int {
    const atomics::once_t once;
    assert(is(once));
    assert(!is(once));
    assert(!once);

    atomics::sequence_t<uint8_t> bytes(3);
    assert(*bytes == 3);
    assert(static_cast<uint8_t>(bytes) == 4);
}


