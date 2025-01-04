// Copyright (C) 2023 Tycho Softworks.
// This code is licensed under MIT license.

#undef  NDEBUG
#include "compiler.hpp"     // IWYU pragma: keep
#include "slice.hpp"
#include <cstdlib>

auto main([[maybe_unused]] int argc, [[maybe_unused]] char **argv) -> int {
    try {
        slice<int>  ints;
        ints.assign({1, 2, 3});
        assert(ints.size() == 3);
        assert(ints[1] == 2);
    }
    catch(...) {
        ::exit(-1);
    }
}


