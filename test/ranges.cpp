// Copyright (C) 2025 Tycho Softworks.
// This code is licensed under MIT license.

#undef  NDEBUG
#include "compiler.hpp"     // IWYU pragma: keep
#include "ranges.hpp"
#include <vector>

auto main([[maybe_unused]] int argc, [[maybe_unused]] char **argv) -> int {
    try {
        using namespace tycho::ranges;
        const std::vector<int> numbers = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
        auto evens = numbers |
            filter([](const int& n) {return n % 2 == 0;}) |
            transform([](const int& n) { return n * n; });
        assert(evens.size() == 5);
        assert(evens[0] == 4);

        each(evens, [](int& n) { n = n * 2;});
        assert(evens[0] == 8);

        auto made = make<std::vector<int>>(3, []{return -1;});
        assert(made.size() == 3);
        assert(made[0] == -1);

    }
    catch(...) {
        ::exit(-1);
    }
}

