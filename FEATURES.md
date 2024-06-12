# Features

Moderncli consists of a series of individual and roughly stand-alone C++ header
files that typically are installed under and included from
/usr/include/moderncli. Some of these are template headers, but all are meant
to be used directly inline. These currently include:

## args.hpp

An argument parsing library header for use in a C++ main().  It consists of
statically configured command line parsing objects and a method to generate
help text. Support for a kind of golang init() thru init\_t also introduced.

## atomics.hpp

Atomic / lockfree data structures and features.

## cipher.hpp

Encode and decode encrypted data. Create cipher keys from passphrases using
a specified digest and cipher. Manage salt.

## datetime.hpp

Basic date / time processing and formatting functions.

## digest.hpp

Uses openssl libcrypto to generate hash digests.

## endian.hpp

Functions to store into and access memory pointer data by endian order.

## expected.hpp

A C++17 simplified emulation of C++23 std::expected using std::variant.

## filesystem.hpp

Support for filesystem, posix file functions, and file scanning closures. This
header presumes a c++ compiler with filesystem runtime support.

## keyfile.hpp

This allows for parsing config files that may be broken into \[sections\] and
have key=value key pairs in each section.  It allows for instanciating a
section as a collection of keys as well as iterating over named sections and
key collections. It can also be used to modify and re-write the key file with
changed data.

## output.hpp

Older stream based output that pre-dates print. By not requiring format() it
produces smaller binaries, and is desirable for making tiny cli utilities.

## print.hpp

Uses libfmt to both format strings and to print output somewhat like C++23
std::print.  It is also much more efficient than std::format, especially if you
have multiple C++ applications. Finally, formatted support for system logging
is introduced, along with serializing logging requests and the ability to
notify logging events.

## process.hpp

Access process properties and execute programs. This includes dso plugin load
support.

## random.hpp

Generate random keys and data using openssl rand functions.

## serial.hpp

Support serial I/O operations through a serial port or ptty device. Includes
support for line buffered and timed input. Serial support depends on posix
termios and is disabled on non-posix platforms.

## socket.hpp

Generic C++ socket library.  This deals with low level socket behavior and
could be thought of as a C++ wrapper for the BSD socket api.

## strings.hpp

Generic C++ string related templates and functions.  This typically covers
string functions either still missing in the C++ standard, or that are
introduced only very recently.

## sync.hpp

Thread and sync templates. This includes special sync ptrs and container
templates that offer public access objects to protected data only when a lock
(mutex or shared) is being actively asserted. This behavior is very similar to
rust locking access and gives stronger guarantees about access safety while
also associating the actual lock with the data being protected rather than as
completely unrelated data structures. Sync also includes other kinds of thread
synchronization objects such as semaphores and barriers.

## templates.hpp

Some very generic, universal, and miscellanous templates and functions.

## linting

Since this is common code extensive support exists for linting and static
analysis.
