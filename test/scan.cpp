// Copyright (C) 2025 Tycho Softworks.
// This code is licensed under MIT license.

#undef  NDEBUG
#include "compiler.hpp"     // IWYU pragma: keep
#include "scan.hpp"
#include <cstdlib>
#include <cstdio>

auto main([[maybe_unused]] int argc, [[maybe_unused]] char **argv) -> int {
    try {
        auto chars1 = "123";
        std::string_view text1(chars1);
        assert(get_value(text1) == 123);
    }
    catch(std::exception& e) {
        printf("Error: %s\n", e.what());
        ::exit(-1);
    }
}

