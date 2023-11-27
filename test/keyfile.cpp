// Copyright (C) 2022 Tycho Softworks.
// This code is licensed under MIT license.

#include "compiler.hpp"
#include "keyfile.hpp"

auto main([[maybe_unused]] int argc, [[maybe_unused]] char **argv) -> int {
    keyfile test_keys;

    assert(!::chdir("test"));
    test_keys.load("test.conf");

    auto keys = test_keys["test"];
    assert(!keys.empty());
    assert(keys["test1"] == "hello");
}


