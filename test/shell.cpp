// Copyright (C) 2022 Tycho Softworks.
// This code is licensed under MIT license.

#undef  NDEBUG
#include "compiler.hpp"
#include "args.hpp"
#include "print.hpp"
#include "filesystem.hpp"
#include "templates.hpp"
#include "output.hpp"

// Test of init trick, a "kind of" atinit() function or golang init().
namespace {
int value = 0;

const init _init([]{
    ++value;
});

void caller() {
    const defer stack([]{
        ++value;
    });
}
} // end anon namespace

auto main([[maybe_unused]] int argc, [[maybe_unused]] char **argv) -> int {
    const uint16_t val = 23;
    auto out = format("X{}Y", val);
    assert(out == "X23Y");
    fsys::remove("/tmp/xyz");
    fsys::path path = "/here";
    assert(format("!{}!", path) == "!/here!");
    assert(value == 1);

    auto *p = &value;
    auto mv = 2;
    assert(const_copy(value) == 1);
    assert(const_copy(p) == 1);
    assert(const_max(value, mv) == 2);

    auto tv = temporary<int>(3);
    assert(*tv == 3);

    caller();
    assert(value == 2);
}

