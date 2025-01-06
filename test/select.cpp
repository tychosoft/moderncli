// Copyright (C) 2025 Tycho Softworks.
// This code is licensed under MIT license.

#undef  NDEBUG
#include "compiler.hpp"     // IWYU pragma: keep
#include "select.hpp"
#include <cstdlib>
#include <iostream>

static select_when<int, std::string> selector{
    {1, []() { std::cout << "Case 1\n"; }},
    {"apple", []() { std::cout << "apple\n"; }},
    {2, []() { std::cout << "2 Case\n"; }}
};

auto main([[maybe_unused]] int argc, [[maybe_unused]] char **argv) -> int {
    try {
        assert(selector(1));
        assert(selector("apple"));
        assert(!selector(42));
    }
    catch(...) {
        ::exit(-1);
    }
}

