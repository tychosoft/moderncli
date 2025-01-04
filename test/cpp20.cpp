// Copyright (C) 2025 Tycho Softworks.
// This code is licensed under MIT license.

#undef  NDEBUG
#include "compiler.hpp"     // IWYU pragma: keep
#include "print.hpp"

auto main([[maybe_unused]] int argc, [[maybe_unused]] char **argv) -> int {
#if __cplusplus >= 202002L
    tycho::print("hello {}", "world\n");
#endif
}


