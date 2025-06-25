// Copyright (C) 2022 Tycho Softworks.
// This code is licensed under MIT license.

#undef  NDEBUG
#include "compiler.hpp"     // IWYU pragma: keep
#include "tasks.hpp"
#include "sync.hpp"
#include "print.hpp"

#include <string>
#include <tuple>
#include <memory>

using namespace tycho;

namespace {
auto count = 0;
std::string str;
task_queue tq;

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
}

