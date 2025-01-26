// Copyright (C) 2023 Tycho Softworks.
// This code is licensed under MIT license.

#undef  NDEBUG
#include "compiler.hpp"     // IWYU pragma: keep
#include "atomics.hpp"
#include "templates.hpp"
#include <cstdint>

auto main([[maybe_unused]] int argc, [[maybe_unused]] char **argv) -> int {
    const atomic::once_t once;
    assert(is(once));
    assert(!is(once));
    assert(!once);

    atomic::sequence_t<uint8_t> bytes(3);
    assert(*bytes == 3);
    assert(static_cast<uint8_t>(bytes) == 4);

    atomic::dictionary_t<int, std::string> dict;
    dict.insert_or_assign(1, "one");
    dict.insert_or_assign(2, "two");
    assert(dict.find(1).value() == "one");  // NOLINT
    assert(dict.size() == 2);
    assert(dict.contains(2));
    dict.remove(1);
    assert(!dict.contains(1));
    assert(dict.size() == 1);
    dict.each([](const int& key, std::string& value) {
        assert(key == 2);
        assert(value == "two");
        value = "two two";
    });
    assert(dict.find(2).value() == "two two"); // NOLINT
}


