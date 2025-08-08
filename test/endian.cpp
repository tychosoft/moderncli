// Copyright (C) 2023 Tycho Softworks.
// This code is licensed under MIT license.

#undef NDEBUG
#include "compiler.hpp" // IWYU pragma: keep
#include "endian.hpp"

auto main([[maybe_unused]] int argc, [[maybe_unused]] char **argv) -> int {
    constexpr uint8_t bytes[] = {0x01, 0x02};

    static_assert(be_get16(bytes) == 258U);
    static_assert(le_get16(bytes) == 513U);
}
