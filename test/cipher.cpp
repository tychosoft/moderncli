// Copyright (C) 2023 Tycho Softworks.
// This code is licensed under MIT license.

#undef  NDEBUG
#include "compiler.hpp"     // IWYU pragma: keep
#include "cipher.hpp"
#include "encoding.hpp"

auto main([[maybe_unused]] int argc, [[maybe_unused]] char **argv) -> int {
    const crypto::keyphrase_t key("hello there");
    assert(key.size() == 32);
    const auto pair = crypto::key_t(key);
    assert(to_b64(pair.first, pair.second) == "EpmMAXBm6w0qcLlObtMZKYWFXOOQ8yG724MgIoiL0lE=");
}
