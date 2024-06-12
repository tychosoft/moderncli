// Copyright (C) 2022 Tycho Softworks.
// This code is licensed under MIT license.

#undef  NDEBUG
#include "compiler.hpp"
#include "serial.hpp"
#include "templates.hpp"

auto main([[maybe_unused]] int argc, [[maybe_unused]] char **argv) -> int {
// This is so we can lint serial on non-termios systems without errors...
#ifdef SERIAL_HPP_
    const serial_t serial;
    assert(is(serial) == false);
#endif
}

