// Copyright (C) 2025 Tycho Softworks.
// This code is licensed under MIT license.

#undef  NDEBUG
#include "compiler.hpp"     // IWYU pragma: keep
#include "array.hpp"
#include <string>
#include <cstdlib>

auto main([[maybe_unused]] int argc, [[maybe_unused]] char **argv) -> int {
    try {
        const tycho::slice<int> vec1;
        const std::vector<int> vec2(5, 100);
        const std::vector<int> temp = {1, 2, 3, 4, 5};
        const tycho::slice<int> vec3(temp.begin(), temp.end());
        const tycho::slice<int> vec4(temp);
        const tycho::slice<int> vec5 = vec4;
        auto even = vec3.filter_if([](int x) {
            return x % 2 == 0;
        });
        const std::vector<int> old(even);
        assert(even[0] == 2);
        assert(even.size() == 2);
        const std::vector<int> move = std::move(even);
        assert(even.empty());   // NOLINT
        assert(move.size() == 2);
        assert(move[0] == 2);

        const tycho::slice<std::string> strings = {"hello", "goodbye"};
        assert(strings[1] == "goodbye");
        assert(strings.contains("goodbye"));

        tycho::array<std::string, 80, 10> sa;
        sa[10] = "first";
        sa[89] = "last";
        assert(sa[10] == "first");

        tycho::slice<std::string> slicer(20);
        assert(slicer.size() == 20);
        slicer[0] = "first";
        slicer[19] = "last";
        assert(slicer.contains("last"));
    }
    catch(...) {
        ::exit(-1);
    }
}

