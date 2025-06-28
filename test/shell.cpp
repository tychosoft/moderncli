// Copyright (C) 2022 Tycho Softworks.
// This code is licensed under MIT license.

#undef  NDEBUG
#include "compiler.hpp"     // IWYU pragma: keep
#include "args.hpp"         // IWYU pragma: keep
#include "print.hpp"        // IWYU pragma: keep
#include "filesystem.hpp"   // IWYU pragma: keep
#include "templates.hpp"
#include "output.hpp"       // IWYU pragma: keep

// cspell:disable-next-line
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
    using namespace tycho;
    using namespace std;

    const uint16_t val = 23;
    auto out = tycho::format("X{}Y", val);
    assert(out == "X23Y");
    fsys::remove("/tmp/xyz");
    const fsys::path path = "/here";
    assert(tycho::format("!{}!", path.string()) == "!/here!");
    assert(value == 1);

    auto *p = &value;
    assert(const_copy(p) == 1);
    assert(in_range(value, 1, 2));
    assert(!in_range(value, 7, 10));

    auto tv = tmparray<int>(3);
    tv[2] = 3;
    assert(tv[2] == 3);

    caller();
    assert(value == 2);

    auto speed = 3600;
    assert(in_list(speed, 9600, {300, 1200, 2400, 9600}) == 9600);

    speed = 1200;
    assert(in_list(speed, 9600, {300, 1200, 2400, 9600}) == 1200);

    const short v1{}, v2[3]{};
    assert(sizeof_2(v1) == sizeof(short));
    assert(sizeof_2(v2) == sizeof(short) * 4);
    assert(sizeof_n(v2, 4) == 8);

    logger_stream logger;
    logger.error() << "Error testing";
}

