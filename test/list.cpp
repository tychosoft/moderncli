// Copyright (C) 2025 Tycho Softworks.
// This code is licensed under MIT license.

#undef  NDEBUG
#include "compiler.hpp"     // IWYU pragma: keep
#include "list.hpp"
#include <cstdlib>

auto main([[maybe_unused]] int argc, [[maybe_unused]] char **argv) -> int {
    try {
        tycho::list<int> list{1, 2, 3, 4, 5};
        assert(list.front() == 1);
        assert(list.back() == 5);
        list.pop_front_if([](const int& v){
            return v < 3;
        });
        assert(list.front() == 3);
        list.remove_if([](const int& v){return v == 4;});
        assert(list.size() == 2);
    }
    catch(...) {
        ::exit(-1);
    }
}

