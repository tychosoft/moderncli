// Copyright (C) 2023 Tycho Softworks.
// This code is licensed under MIT license.

#undef  NDEBUG
#include "compiler.hpp"
#include "sync.hpp"

struct test {
    int v1{2};
    //int v2{7};
};

namespace {
unique_sync<int> counter(3);
shared_sync<struct test> testing;
} // end namespace

auto main([[maybe_unused]] int argc, [[maybe_unused]] char **argv) -> int {
    try {
        sync_ptr<int> count(counter);
        assert(*count == 3);
        ++*count;
        assert(*count == 4);
        count.unlock();

        guard_ptr<int> fixed(counter);
        assert(*fixed == 4);

        const reader_ptr<struct test> tester(testing);
        assert(tester->v1 == 2);
    }
    catch(...) {
        exit(1);
    }
}

