// Copyright (C) 2022 Tycho Softworks.
// This code is licensed under MIT license.

#undef  NDEBUG
#include "compiler.hpp"     // IWYU pragma: keep
#include "print.hpp"
#include "strings.hpp"
#include "encoding.hpp"
#include "memory.hpp"

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
    assert(lower_case(std::string("Hi There")) == "hi there");
    static_assert(strip("   testing ") == "testing");
    static_assert(strip(" \t\r  ").empty());
    assert(strip(std::string("   testing ")) == "testing");
    assert(strip(std::string("   testing ")).size() == 7);
    assert(begins_with(std::string("belong"), "be"));
    static_assert(ends_with("belong", "ong"));
    static_assert(!begins_with("belong", "tr"));

    static_assert(unquote("'able '") == "able ");
    static_assert(unquote("'able ") == "'able ");
    assert(unquote(std::string("'able ")) == "'able ");
    assert(unquote(std::string("'able '")) == "able ");

    static_assert(!is_quoted(";able'"));
    assert(is_quoted(std::string("'able'")));
    char qt[] = {'{', 'a', '}', 0};
    qt[1] = 'b';
    assert(is_quoted(qt));

    static_assert(!is_unsigned("23e"));
    static_assert(is_unsigned("246"));
    assert(is_unsigned(std::string("246")) == true);
    assert(is_integer(std::string("-246")) == true);

    assert(to_hex(buf, sizeof(buf)) == "03ff");
    assert(from_hex(hex, tmp, sizeof(tmp)) == sizeof(tmp));
    assert(to_hex(tmp, sizeof(tmp)) == "03ff");
    hex[2] = 'z';
    assert(from_hex(hex, tmp, sizeof(tmp)) == 1);

    char yes[] = {'y', 'e', 's', 0};
    yes[0] = 'y';
    assert(!eq("yes", "no"));                           // NOLINT
    assert(eq("yes", yes));

    static_assert(u8verify("\xc3\xb1"));
    static_assert(!u8verify("\xa0\xa1"));
}


