// Copyright (C) 2022 Tycho Softworks.
// This code is licensed under MIT license.

#undef  NDEBUG
#include "compiler.hpp"
#include "print.hpp"
#include "strings.hpp"
#include "datetime.hpp"

#include <iostream>

auto main([[maybe_unused]] int argc, [[maybe_unused]] char **argv) -> int {
    const std::string text = "hi,bye,gone";
    const uint8_t buf[2] = {0x03, 0xff};
    uint8_t tmp[2];
    auto list = split(text, ",");
    auto hex = to_hex(buf, sizeof(buf));

    assert(list.size() == 3);
    assert(list[0] == "hi");
    assert(list[1] == "bye");
    assert(list[2] == "gone");

    assert(upper_case("Hi There") == "HI THERE");
    assert(lower_case<std::string>("Hi There") == "hi there");
    assert(strip("   testing ") == "testing");
    assert(begins_with<std::string>("belong", "be"));
    static_assert(ends_with("belong", "ong"));

    assert(unquote("'able '") == "able ");
    assert(unquote("'able ") == "'able ");

    assert(to_hex(buf, sizeof(buf)) == "03ff");
    assert(from_hex(hex, tmp, sizeof(tmp)) == sizeof(tmp));
    assert(to_hex(tmp, sizeof(tmp)) == "03ff");
    hex[2] = 'z';
    assert(from_hex(hex, tmp, sizeof(tmp)) == 1);

    static_assert(u8verify("\xc3\xb1"));
    static_assert(!u8verify("\xa0\xa1"));
}


