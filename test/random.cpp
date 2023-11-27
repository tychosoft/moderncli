// Copyright (C) 2023 Tycho Softworks.
// This code is licensed under MIT license.

#include "compiler.hpp"
#include "random.hpp"

auto main([[maybe_unused]] int argc, [[maybe_unused]] char **argv) -> int {
    const crypto::random_t<128> key1, key2;
    assert(key1.size() == 128);
    assert(key1 != key2);
}


