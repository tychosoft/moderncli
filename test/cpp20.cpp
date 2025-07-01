// Copyright (C) 2025 Tycho Softworks.
// This code is licensed under MIT license.

#undef  NDEBUG
#include "compiler.hpp"     // IWYU pragma: keep
#include "print.hpp"
#include "ranges.hpp"
#include "memory.hpp"
#include "array.hpp"
#include "sync.hpp"
#include "tasks.hpp"
#include "filesystem.hpp"
#include <vector>
#include <ranges>
#include <span>
#include <atomic>
#include <cstdlib>

namespace {
struct test {
    int v1{2};
    //int v2{7};
};

auto count = 0;
std::string str;
task_queue tq;
unique_sync<int> counter(3);
shared_sync<struct test> testing;

auto process_command(const std::string& text, int number) {
    return tq.dispatch([text, number] {
        str = text;
        count += number;
    });
}

auto test_async(int x) -> int {
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    return x;
}
} // end namespace

auto main([[maybe_unused]] int argc, [[maybe_unused]] char **argv) -> int {
#if __cplusplus >= 202002L
    wait_group wg(1);
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


        tq.startup();
        assert(process_command("test", 42));
        assert(process_command("more", 10));
        while(!tq.empty()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        tq.shutdown();
        assert(count == 52);
        assert(str == "more");

        task_queue tq1; // NOLINT
        const std::shared_ptr<int> ptr = std::make_shared<int>(count);
        auto use = ptr.use_count();
        event_sync done;    // NOLINT
        tq1.startup();
        tq1.dispatch([ptr, &use, &done] {
            use = ptr.use_count();
            ++count;
            done.notify();
        });
        done.wait();
        tq1.shutdown();
        assert(count == 53);
        assert(use == 2);

        auto future = tycho::await(test_async, 42);
        assert(future.get() == 42);
        std::this_thread::sleep_for(std::chrono::milliseconds(20));

        std::atomic<int> total = 0;
        parallel_threads(3, [&total]{
            total += 2;
        });
        assert(total == 6);

        task_pool pool(4);
        std::mutex cout_mutex;
        for (int i = 0; i < 8; ++i) {
            pool.dispatch([i, &cout_mutex] {
                {
                    const std::lock_guard lock(cout_mutex);
                    std::cout << "Task " << i << " is running on thread "
                        << std::this_thread::get_id() << '\n';
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(120));
            });
        }

        fsys::path path{"testing"};
        auto text = tycho::format("test {}", to_string(path));
        assert(text == "test testing");

        const sync_group arrival(wg);
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
    }
    catch(...) {
        ::exit(-1);
    }
    assert(wg.count() == 0);
#endif
    return 0;
}


