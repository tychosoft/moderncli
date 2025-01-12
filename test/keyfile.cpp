// Copyright (C) 2022 Tycho Softworks.
// This code is licensed under MIT license.

#undef  NDEBUG
#include "compiler.hpp"     // IWYU pragma: keep
#include "keyfile.hpp"
#include "scan.hpp"

#ifndef TEST_DATA
#define TEST_DATA "."   // NOLINT
#endif

auto main([[maybe_unused]] int argc, [[maybe_unused]] char **argv) -> int {
    keyfile test_keys = keyfile::create({"initial"});
    assert(!::chdir(TEST_DATA));
    test_keys.load("test.conf");

    auto keys = test_keys["test"];
    assert(!keys.empty());
    assert(keys["test1"] == "hello");
    assert(keys["test2"] == "");
    assert(get_quoted(keys["test3"]) == "hello world");
    assert(get_quoted(keys["test1"]) == "hello");
    assert(get_quoted(keys["test4"]) == "hello\"world");
    assert(get_string(keys["test4"]) == "hello\\\"world");
    assert(get_literal(keys["password"]) == "\"hi\\");
    assert(test_keys.exists("initial"));

    test_keys.load("more", {
        {"hello", "world"},
    });

    assert(test_keys.exists("more"));
    keys = test_keys["more"];
    assert(keys["hello"] == "world");
}


