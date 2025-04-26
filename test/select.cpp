// Copyright (C) 2025 Tycho Softworks.
// This code is licensed under MIT license.

#undef  NDEBUG
#include "compiler.hpp"     // IWYU pragma: keep
#include "select.hpp"
#include <cstdlib>
#include <iostream>

auto main([[maybe_unused]] int argc, [[maybe_unused]] char **argv) -> int {
    enum class selects : int {
        Five = 5,
        Six = 6,
        Seven = 7
    };

    try {
        const select_when<int, std::string> selector{
            {1, []{ std::cout << "Case 1\n"; }},
            {"apple", []{ std::cout << "apple\n"; }},
            {2, []{ std::cout << "2 Case\n"; }}
        };

        const select_type<selects, int, std::string> mapped {
            {5, selects::Five},
            {"five", selects::Five},
            {6, selects::Six}
        };

        assert(selector(1));
        assert(selector("apple"));
        assert(!selector(42));
        assert(mapped("five") == selects::Five);

        assert(selector(2, [](const int& key, const int& value) {
            return key >= value;
        }) == true);
    }
    catch(...) {
        ::exit(-1);
    }
}

