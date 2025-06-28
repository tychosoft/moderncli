// Copyright (C) 2023 Tycho Softworks.
// This code is licensed under MIT license.

#undef  NDEBUG
#include "compiler.hpp"     // IWYU pragma: keep
#include "sync.hpp"
#include "tasks.hpp"
#include <cstdlib>

struct test {
    int v1{2};
    //int v2{7};
};

namespace {
unique_sync<int> counter(3);
shared_sync<struct test> testing;
} // end namespace

auto main([[maybe_unused]] int argc, [[maybe_unused]] char **argv) -> int {
    wait_group wg(1);
    try {
        const sync_group done(wg);
        assert(wg.count() == 1);
        sync_ptr<int> count(counter);
        assert(*count == 3);
        ++*count;
        assert(*count == 4);
        count.unlock();

        guard_ptr<int> fixed(counter);
        assert(*fixed == 4);

        const reader_ptr<struct test> tester(testing);
        assert(tester->v1 == 2);

        semaphore_t sem(1);     // binary semaphore
        const thread_t thr([&sem]{
            const semaphore_guard ses(sem);
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            assert(sem.acquired());
            assert(sem.active() == 1);
        });
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        assert(sem.active() == 1);
        assert(sem.size() == 1);
    }
    catch(...) {
        ::exit(1);
    }
    assert(wg.count() == 0);
}

