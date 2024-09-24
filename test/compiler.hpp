// Copyright (C) 2020 Tycho Softworks.
// This code is licensed under MIT license.

#ifndef	COMPILER_HPP_
#define	COMPILER_HPP_

namespace tycho {}
using namespace tycho;

#ifdef __clang__
#pragma clang diagnostic ignored "-Wpadded"
#pragma clang diagnostic ignored "-Wswitch-enum"
#pragma clang diagnostic ignored "-Wweak-vtables"
#pragma clang diagnostic ignored "-Wunused-member-function"
#endif

#ifdef __GNUC__
#pragma GCC diagnostic ignored "-Wunused-result"
#pragma GCC diagnostic ignored "-Wzero-as-null-pointer-constant"
#endif

#if !defined(_MSC_VER) && __cplusplus < 201703L
#error C++17 compliant compiler required
#endif

#ifdef  _MSC_VER
#include <direct.h>
#else
#include <unistd.h>
#endif

#ifdef  MODERN_TESTING
#undef  NDEBUG
#ifndef DEBUG
#define DEBUG
#endif
#endif

#include <cassert>
#include <cstdint>
#endif
