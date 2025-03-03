// Copyright (C) 2025 Tycho Softworks.
// This code is licensed under MIT license.

#undef  NDEBUG
#include "compiler.hpp"     // IWYU pragma: keep
#include "array.hpp"
#include "memory.hpp"
#include <string>
#include <cstdlib>

auto main([[maybe_unused]] int argc, [[maybe_unused]] char **argv) -> int {
    try {
        const tycho::slice<int> vec1;
        const std::vector<int> vec2(5, 100);
        const std::vector<int> temp = {1, 2, 3, 4, 5};
        const tycho::slice<int> vec3(temp.begin(), temp.end());
        const tycho::slice<int> vec4 = temp;
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

        tycho::array<std::string, 10> st;
        st[0] = "first";
        st[9] = "last";
        assert(st[0] == "first");
        assert(st.get_or(10) == nullptr);
        assert(st.get(1));
        assert(!st.get(10));

        tycho::array<std::string, 80, 10> sa;
        sa[10] = "first";
        sa[89] = "last";
        assert(sa[10] == "first");
        assert(sa.at(10) == "first");

        auto pool = make_pool(sa);
        assert(!pool.empty());
        assert(*pool == "first");
        assert(pool.size() == 1);

        const tycho::span<std::string> span(sa);
        assert(span[0] == "first");
        assert(span[79] == "last");

        auto ptr = sa.get_or(10);
        assert(ptr->size() == 5);
        assert(!sa.get(0));

        tycho::slice<std::string> slicer(20);
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
}

