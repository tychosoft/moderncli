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

// just construction / destruction semantics test if global works
[[maybe_unused]] task_pool dummy_pool;
[[maybe_unused]] task_queue dummy_queue;
[[maybe_unused]] timer_queue dummy_timer;

auto process_command(const std::string& text, int number) {
    return tq.dispatch([text, number] {
        str = text;
        count += number;
    });
}

auto move_command(std::string&& text, int number) {
    return tq.dispatch([text = std::move(text), number] {
        str = std::move(text);
        count += number;
    });
}
} // end namespace

auto main([[maybe_unused]] int argc, [[maybe_unused]] char **argv) -> int {
    tq.startup();
    std::string old = "here";
    assert(!old.empty());
    move_command(std::move(old), 0);
    assert(old.empty());    // NOLINT

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

    std::atomic<int> total = 0;
    parallel_task(3, [&total]{
        yield(3);
        total += 2;
    });
    assert(total == 6);

    timer_queue timers;
    auto fast = 0;
    auto heartbeat = 0;
    timers.startup([]{print("Start timer thread\n");});

    timers.periodic(std::chrono::milliseconds(150), [&heartbeat]{
        ++heartbeat;
    });

    auto id = timers.periodic(50, [&fast]{
        ++fast;
    });

    yield(400);
    assert(timers.size() == 2);
    assert(timers.exists(id));
    timers.cancel(id);
    assert(!timers.exists(id));
    assert(timers.size() == 1);
    auto saved = fast;
    auto prior = heartbeat;
    yield(240);
    assert(fast == saved);
    timers.shutdown();
    assert(heartbeat >= 2 && fast > heartbeat && heartbeat <= 5);
    assert(heartbeat > prior);
    saved = fast;

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

    // definately not still running...
    assert(saved == fast);
    assert(fast > heartbeat);
    return 0;
}


