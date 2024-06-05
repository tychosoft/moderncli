// Copyright (C) 2023 Tycho Softworks.
// This code is licensed under MIT license.

#undef  NDEBUG
#include "compiler.hpp"
#include "random.hpp"

auto main([[maybe_unused]] int argc, [[maybe_unused]] char **argv) -> int {
    const crypto::random_t<crypto::sha512_key> key1, key2;
    assert(key1.bits() == 512);
    assert(key1.size() == 64);
    assert(key1 != key2);
}


