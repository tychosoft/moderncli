// Copyright (C) 2025 Tycho Softworks.
// This code is licensed under MIT license.

#undef  NDEBUG
#include "compiler.hpp"     // IWYU pragma: keep
#include "map.hpp"
#include <cstdlib>

auto main([[maybe_unused]] int argc, [[maybe_unused]] char **argv) -> int {
    try {
        const tycho::hash_map<int,int> pairs{
            {1, 10},
            {2, 20}
        };

        assert(pairs.contains(2));
        assert(!pairs.contains(20));
    }
    catch(...) {
        ::exit(-1);
    }
}

