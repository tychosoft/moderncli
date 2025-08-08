// Copyright (C) 2022 Tycho Softworks.
// This code is licensed under MIT license.

#undef NDEBUG
#include "compiler.hpp" // IWYU pragma: keep
#include "funcs.hpp"
#include "print.hpp"

using namespace tycho;

namespace {
auto test_async(int x) -> int {
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    return x;
}
} // end namespace

auto main([[maybe_unused]] int argc, [[maybe_unused]] char **argv) -> int {
    auto future = tycho::await(test_async, 42);
    assert(future.get() == 42);
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    return 0;
}
