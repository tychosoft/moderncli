# About ModernCLI

Moderncli is a package that collects and centralizes testing for many of my
inline C++ header libraries, which I had duplicated or vendored in other
packages in the past. These headers are being gathered under a common package
of their own that can then be installed as a core dependency. Hence, one
interpretation for this package is "Modern C++" Library Interfaces. Moderncli
is meant to support C++ 17 or later.

Moderncli headers are meant to be usable stand-alone and provide generic
cross-platform low level wrappers around common system features and services.
This makes it easier to include just what you need without drawing in
additional headers you may not be using. This also makes it easy to substitute
other common C++ libraries without having to include duplication functionality
from moderncli.

Moderncli requires CMake to build the testing framework. It should build and
work with GCC (9 or later), with Clang (14? or later), and MSVC. Besides
GNU/Linux and BSD systems, Moderncli is portable to and can support the MingW
platform target as installed by Debian when selecting posix runtime, and even
native MS Visual Studio builds. The minimum requirement is a C++17 compiler (or
later) that supports the C++ filesystem header and runtime library.

## Dependencies

Moderncli and applications also often make use of the C++ library libfmt. This
allows C++17 (and C++20) applications to use modern string and print formatting
operations like from C++23 and later. If you are building applications for
C++20 or later, or you do not use the moderncli print header for output, you do
not need to use or link with libfmt.

Moderncli also uses openssl for crypto operations and ssl streams in any crypto
namespace related headers. Generally, if your not using secure streams, but
does use a moderncli crypto header (sign, cipher, digests, bignum, and random),
you should do require libcrypto from openssl. If you use secure streaming you
will also require openssl libssl. In addition, C++ thread support may have to
be enabled to use C++ thread operations. The cmake/features.cmake file shows
how to test for and enable these dependencies correctly from CMake.

## Distributions

Distributions of this package are provided as detached source tarballs made
from a tagged release from our internal source repository. These stand-alone
detached tarballs can be used to make packages for many GNU/Linux systems, and
for BSD ports. They may also be used to build and install the software directly
on a target platform.

The latest public release source tarball can be produced by an auto-generated
tarball from a tagged release in the projects public git repository at
https://gitlab.com/tychosoft/moderncli. Moderncli can also be easily vendored
in other software using git modules from this public repo. I also package
Moderncli for Alpine Linux. There is no reason this cannot easily be packaged
for use on other distributions, for BSD ports, vcpkg, etc, as well.

## Participation

This project is offered as free (as in freedom) software for public use and has
a public project page at https://www.gitlab.com/tychosoft/moderncli which has
an issue tracker where you can submit public bug reports and a public git
repository. Patches and merge requests may be submitted in the issue tracker or
thru email. Support requests and other kinds of inquiries may also be sent thru
the tychosoft gitlab help desktop service. Other details about participation
may be found in the Contributing page.

## Testing

There is a testing program for each header. These run simple tests that will be
expanded to improve code coverage over time. The test programs are the only
build target making this library by itself, and the test programs in this
package work with the cmake ctest framework. They may also be used as simple
examples of how a given header works. There is also a **lint** target that can
be used to verify code changes.

