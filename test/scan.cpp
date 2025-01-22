// Copyright (C) 2025 Tycho Softworks.
// This code is licensed under MIT license.

#undef  NDEBUG
#include "compiler.hpp"     // IWYU pragma: keep
#include "scan.hpp"
#include <cstdlib>

auto main([[maybe_unused]] int argc, [[maybe_unused]] char **argv) -> int {
    try {
        std::string_view text = "123";
        assert(get_value(text) == 123);

        text = "true";
        assert(get_bool(text) == true);

        text = "Off";
        assert(get_bool(text) == false);

        text = "5m";
        assert(get_duration(text) == 300);

        text = "300";
        assert(get_duration(text) == 300);

        text = "1:26:10";
        assert(get_duration(text) == 5170);

        text = "hello";
        assert(get_string(text) == "hello");

        text = "'hello world'";
        assert(get_string(text) == "hello world");

        text = "\"hello\\nworld\""; // NOLINT
        assert(get_string(text) == "hello\nworld");

        auto myint = get_unsigned<uint16_t>("23");
        assert(myint == 23);
        assert(sizeof(myint) == 2); // NOLINT

        auto mydec = get_decimal("-17.05");
        assert(mydec == -17.05);

        assert(get_hex("f0") == 240);
    }
    catch(std::exception& e) {
        printf("Error: %s\n", e.what());
        ::exit(-1);
    }
}

