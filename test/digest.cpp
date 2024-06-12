// Copyright (C) 2022 Tycho Softworks.
// This code is licensed under MIT license.

#undef  NDEBUG
#include "compiler.hpp"
#include "digest.hpp"
#include "strings.hpp"
#include "templates.hpp"

auto main([[maybe_unused]] int argc, [[maybe_unused]] char **argv) -> int {
    crypto::digest_t digest(EVP_sha256());
    assert(is(digest) == true);

    digest.update("hello world");
    digest.finish();
    assert(digest.size() == 32);
    assert(to_hex(digest.view()) == "b94d27b9934d3e08a52e52d7da7dabfac484efe37a5380ee9088f7ace2efcde9");
}


