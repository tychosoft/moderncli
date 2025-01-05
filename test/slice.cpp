// Copyright (C) 2023 Tycho Softworks.
// This code is licensed under MIT license.

#undef  NDEBUG
#include "compiler.hpp"     // IWYU pragma: keep
#include "slice.hpp"

#include <vector>
#include <cstdlib>

auto main([[maybe_unused]] int argc, [[maybe_unused]] char **argv) -> int {
    try {
        slice<int>  ints;
        ints.assign({1, 2, 3});
        assert(ints.size() == 3);
        assert(ints[1] == 2);
        ints[1] = 7;
        assert(ints[1] == 7);
        auto ptr = ints(1);
        assert(*ptr == 7);
        assert(ptr.use_count() == 2);
        ints.each([](int& value) {
            return value += 1;
        });
        assert(*ptr == 8);

        std::vector<int> numbers = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
        const slice<int> some(numbers.begin(), numbers.end(), [](int x) {
            return x % 2 == 0;
        });
        auto val = some[1];
        assert(val == 4);
    }
    catch(...) {
        ::exit(-1);
    }
}


