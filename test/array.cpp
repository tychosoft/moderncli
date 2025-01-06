// Copyright (C) 2025 Tycho Softworks.
// This code is licensed under MIT license.

#undef  NDEBUG
#include "compiler.hpp"     // IWYU pragma: keep
#include "array.hpp"
#include <cstdlib>

auto main([[maybe_unused]] int argc, [[maybe_unused]] char **argv) -> int {
    try {
        const tycho::vector<int> vec1;
        const std::vector<int> vec2(5, 100);
        const std::vector<int> temp = {1, 2, 3, 4, 5};
        const tycho::vector<int> vec3(temp.begin(), temp.end());
        const tycho::vector<int> vec4(temp);
        const tycho::vector<int> vec5(vec4);
        auto even = vec3.filter([](int x) {
            return x % 2 == 0;
        });
        const std::vector<int> old(even);
        assert(even[0] == 2);
        assert(even.size() == 2);
        const std::vector<int> move = std::move(even);
        assert(even.empty());   // NOLINT
        assert(move.size() == 2);
        assert(move[0] == 2);
    }
    catch(...) {
        ::exit(-1);
    }
}

