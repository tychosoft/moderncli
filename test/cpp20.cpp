// Copyright (C) 2025 Tycho Softworks.
// This code is licensed under MIT license.

#undef  NDEBUG
#include "compiler.hpp"     // IWYU pragma: keep
#include "print.hpp"
#include "ranges.hpp"
#include "memory.hpp"
#include "array.hpp"
#include "sync.hpp"
#include "filesystem.hpp"
#include <vector>
#include <ranges>
#include <span>
#include <cstdlib>

auto main([[maybe_unused]] int argc, [[maybe_unused]] char **argv) -> int {
#if __cplusplus >= 202002L
    try {
        using namespace tycho;

        print("hello {}", "world\n");

        const std::vector<int> numbers = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
        auto evens = numbers |
            ranges::filter([](const int& n) {return n % 2 == 0;}) |
            ranges::transform([](const int& n) { return n * n; });
        assert(evens.size() == 5);
        assert(evens[0] == 4);

        ranges::each(evens, [](int& n) { n = n * 2;});
        assert(evens[0] == 8);

        auto made = ranges::make<std::vector<int>>(3, []{return -1;});
        assert(made.size() == 3);
        assert(made[0] == -1);

        const slice<int> vec1;
        const std::vector<int> vec2(5, 100);
        const std::vector<int> temp = {1, 2, 3, 4, 5};
        const slice<int> vec3(temp.begin(), temp.end());
        const slice<int> vec4(temp);
        const slice<int> vec5 = vec4;
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

        const slice<std::string> strings = {"hello", "goodbye"};
        assert(strings[1] == "goodbye");
        assert(strings.contains("goodbye"));

        array<std::string, 80, 10> sa;
        sa[10] = "first";
        sa[89] = "last";
        assert(sa[10] == "first");

        slice<std::string> slicer(20);
        assert(slicer.size() == 20);
        slicer[0] = "first";
        slicer[19] = "last";
        assert(slicer.contains("last"));
        auto copy = slicer;
        assert(copy.size() == slicer.size());
        assert(copy[0] == slicer[0]);   // independent memory copies...
        assert(copy.data() != slicer.data());
        assert(copy == slicer);

        const auto spanner = make_span(slicer);
        assert(spanner.front() == "first");
        assert(spanner.size() == 20);

        auto shared = bytearray_t(3, 7);
        auto shared1(shared);
        {
            auto shared2(shared);
            shared2[0] = 9;
        }
        assert(shared1.count() == 2);
        assert(shared1[2] == 7);
        assert(shared1[0] == 9);

    }
    catch(...) {
        ::exit(-1);
    }
#endif
}


