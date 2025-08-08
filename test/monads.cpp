// Copyright (C) 2025 Tycho Softworks.
// This code is licensed under MIT license.

#undef NDEBUG
#include "compiler.hpp" // IWYU pragma: keep
#include "monadic.hpp"
#include <cstdlib>

using namespace tycho::monadic;

namespace {
auto add_one(int x) {
    return maybe(x + 1);
}
} // end namespace

auto main([[maybe_unused]] int argc, [[maybe_unused]] char **argv) -> int {
    try {
        auto something = some(10);
        auto nothing = none<int>();
        auto result = something.bind(add_one);
        auto filtered = filter(result, [](int x) { return x > 10; });
        auto mapped = map(result, [](int x) { return x * 2; });

        assert((bool)result == true);
        assert(*result == 11);
        assert(!nothing == true);
        assert((bool)nothing == false);
        assert(or_else(result, -1) == 11);
        assert(or_else(nothing, 7) == 7);
        assert(or_else(filtered, -1) == 11);
        assert(or_else(mapped, -1) == 22);

        auto nested = some(some(5));
        auto flat = flatten(nested);
        assert(or_else(flat, -1) == 5);

        const maybe<std::optional<int>> opt = some(std::optional<int>(20));
        assert(or_else(opt, -1) == 20);

        auto flat_mapped = flat_map(result, add_one);
        assert(or_else(flat_mapped, -1) == 12);

        auto func = some([](int x) { return x * 3; });
        auto applied = apply(func, result);
        assert(or_else(applied, -1) == 33);

        const std::vector<maybe<int>> maybe_vec = {some(1), some(2), some(3)};
        auto traversed = traverse(maybe_vec, add_one);
        if (traversed.has_value()) {
            auto vec = *traversed;
            assert(vec[0] == 2);
            assert(vec[2] == 4);
        }

        auto sequenced = sequence(maybe_vec);
        if (sequenced.has_value()) {
            auto vec = *sequenced;
            assert(vec[0] == 1);
            assert(vec[2] == 3);
        }

        const std::vector<maybe<int>> sums = {some(1), some(2), some(3), none<int>(), some(4)};
        auto sum = fold(sums, [](int acc, int val) { return acc + val; }, 0);
        assert(sum == 10);
    } catch (...) {
        ::exit(-1);
    }
}
