// Copyright (C) 2023 Tycho Softworks.
// This code is licensed under MIT license.

#undef  NDEBUG
#include "compiler.hpp"     // IWYU pragma: keep
#include "bignum.hpp"
#include "strings.hpp"

using namespace crypto;

auto main([[maybe_unused]] int argc, [[maybe_unused]] char **argv) -> int {
    bignum_t v1;

    assert(*v1 == "0");
    v1 = "-23451234567890";

    const bignum_t v2("25");
    v1 += v2;
    assert(*v1 == "-23451234567865");

    ++v1;
    assert(*v1 == "-23451234567864");

    auto a = abs(v1);
    assert(*a == "23451234567864");

}


