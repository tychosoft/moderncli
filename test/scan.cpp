// Copyright (C) 2025 Tycho Softworks.
// This code is licensed under MIT license.

#undef  NDEBUG
#include "compiler.hpp"     // IWYU pragma: keep
#include "scan.hpp"
#include <cstdlib>
#include <cstdio>

auto main([[maybe_unused]] int argc, [[maybe_unused]] char **argv) -> int {
    try {
        std::string_view text = "123";
        assert(get_value(text) == 123);

        text = "true";
        assert(get_bool(text) == true);

        text = "Off";
        assert(get_bool(text) == false);

        text = "5m";
        assert(get_seconds(text) == 300);

        text = "300";
        assert(get_seconds(text) == 300);
    }
    catch(std::exception& e) {
        printf("Error: %s\n", e.what());
        ::exit(-1);
    }
}

