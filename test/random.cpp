// Copyright (C) 2023 Tycho Softworks.
// This code is licensed under MIT license.

#undef  NDEBUG
#include "compiler.hpp"     // IWYU pragma: keep
#include "random.hpp"
#include "strings.hpp"
#include "encoding.hpp"

auto main([[maybe_unused]] int argc, [[maybe_unused]] char **argv) -> int {
    const crypto::random_t<crypto::sha512_key> key1, key2;
    assert(key1.bits() == 512);
    assert(key1.size() == 64);
    assert(key1 != key2);
    const crypto::key_t raw = key1;
    assert(raw.second == 64);

    const uint8_t txt[7] = {'A', 'B', 'C', 'D', 'Z', '1', '2'};
    uint8_t msg[8];
    msg[7] = 0;
    // cspell:disable-next-line
    assert(to_b64(txt, sizeof(txt)) == "QUJDRFoxMg==");
    // cspell:disable-next-line
    assert(size_b64("QUJDRFoxMg==") == 7);
    // cspell:disable-next-line
    assert(from_b64("QUJDRFoxMg==", msg, sizeof(msg)) == 7);
    // cspell:disable-next-line
    assert(eq("ABCDZ12", reinterpret_cast<const char *>(msg)));
}


