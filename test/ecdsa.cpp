// Copyright (C) 2023 Tycho Softworks.
// This code is licensed under MIT license.

#undef NDEBUG
#include "compiler.hpp" // IWYU pragma: keep
#include "templates.hpp"
#include "eckey.hpp"
#include "sign.hpp"
#include "x509.hpp"

auto main([[maybe_unused]] int argc, [[maybe_unused]] char **argv) -> int {
    const crypto::eckey_t keypair;
    assert(is(keypair));
}
