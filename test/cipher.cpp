// Copyright (C) 2023 Tycho Softworks.
// This code is licensed under MIT license.

#undef  NDEBUG
#include "compiler.hpp"     // IWYU pragma: keep
#include "cipher.hpp"

auto main([[maybe_unused]] int argc, [[maybe_unused]] char **argv) -> int {
    const crypto::keyphrase_t key("hello there");
    assert(key.size() == 32);
    assert(key.to_string() == "EpmMAXBm6w0qcLlObtMZKYWFXOOQ8yG724MgIoiL0lE=");
}
