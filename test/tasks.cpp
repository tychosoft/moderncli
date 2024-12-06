// Copyright (C) 2022 Tycho Softworks.
// This code is licensed under MIT license.

#undef  NDEBUG
#include "compiler.hpp"     // IWYU pragma: keep
#include "tasks.hpp"
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
} // end namespace

auto test_async(int x) -> int {
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    return x;
}

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
    tq1.startup();
    tq1.dispatch([ptr, &use] {
        use = ptr.use_count();
        ++count;
    });
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    tq1.shutdown();
    assert(count == 53);
    assert(use == 2);

    auto future = tycho::await(test_async, 42);
    assert(future.get() == 42);
}

