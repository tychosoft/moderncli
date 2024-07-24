// Copyright (C) 2023 Tycho Softworks.
// This code is licensed under MIT license.

#undef  NDEBUG
#include "compiler.hpp"     // IWYU pragma: keep
#include "expected.hpp"

#include <string>

namespace {
auto ret_error() -> expected<std::string, int> {
    return expected<std::string, int>(23);
}

auto ret_string() -> expected<std::string, int> {
    return expected<std::string, int>("hello");
}
} // namespace

auto main([[maybe_unused]] int argc, [[maybe_unused]] char **argv) -> int {
    try {
        auto e1 = ret_error();
        auto e2 = ret_string();

        assert(e1.error() == 23);
        assert(e2.value() == "hello");
    } catch(...) {
        abort();
    }
}


