// Copyright (C) 2022 Tycho Softworks.
// This code is licensed under MIT license.

#undef NDEBUG
#include "compiler.hpp" // IWYU pragma: keep
#include "hash.hpp"

auto main([[maybe_unused]] int argc, [[maybe_unused]] char **argv) -> int {
    try {
        const crypto::hash_t<std::string> hash;
        const std::string a = "alpha";
        const std::string b = "beta";
        const std::string c = "alpha";

        auto ha = hash(a);
        auto hb = hash(b);
        auto hc = hash(c);

        // Ensure determinism: same input, same hash
        assert(ha == hc);

        // Ensure reasonable uniqueness: different inputs, different hashes
        assert(ha != hb);

        crypto::ring64<> ring;
        assert(ring.insert("nodeA"));
        assert(ring.insert("nodeB"));
        assert(ring.insert("nodeC"));
        assert(ring.size() == 3);
        assert(ring.usage() > 200);

        const std::string key = "user:67";
        assert(ring.get(key) == "nodeC");

        assert(ring.remove("nodeB"));
        assert(!ring.remove("nodeD"));
        assert(ring.usage() <= 200);
    } catch (...) {
        ::exit(1);
    }
}
