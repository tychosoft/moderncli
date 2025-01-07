// Copyright (C) 2025 Tycho Softworks.
// This code is licensed under MIT license.

#undef  NDEBUG
#include "compiler.hpp"     // IWYU pragma: keep
#include "list.hpp"
#include <cstdlib>

auto main([[maybe_unused]] int argc, [[maybe_unused]] char **argv) -> int {
    try {
        tycho::slist<int> list{1, 2, 3, 4, 5};
        assert(list.front() == 1);
        assert(list.back() == 5);
        list.pop_if([](const int& v){
            return v < 3;
        });
        assert(list.front() == 3);
    }
    catch(...) {
        ::exit(-1);
    }
}

