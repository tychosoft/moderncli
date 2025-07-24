# Features

Moderncli consists of a series of individual and roughly stand-alone C++ header
files that typically are installed under and included from
/usr/include/moderncli. Some of these are template headers, but all are meant
to be used directly inline. These currently include:

## args.hpp

An argument parsing library header for use in a C++ main().  It consists of
statically configured command line parsing objects and a method to generate
help text. Support for a kind of golang init() thru init\_t also introduced.

## array.hpp

Specialized array class with offset. This also includes an enhanced vector
class that acts more like what slice does in other languages, and a simplified
C++17 friendly version of spans.

## atomics.hpp

Atomic types and lockfree data structures and features. This includes lockfree
stack and buffer implimentations which perhaps something like C#
ConcurrentStack and ConcurrentQueue.

## bignum.hpp

Arbitrary precision integer math. This will eventually be extended to support
core cryptograhic services including signing and key exchange.

## cipher.hpp

Encode and decode encrypted data. Create cipher keys from passphrases using
a specified digest and cipher. Manage salt.

## datetime.hpp

Basic date / time processing and formatting functions.

## digest.hpp

Uses openssl libcrypto to generate hash digests.

## eckey.hpp

Creation and management of Eliptical Curve key pairs for both public and
private keys.

## encoding.hpp

Various common character encoding formats, starting with b64 and hex.

## endian.hpp

Functions to store into and access memory pointer data by endian order.

## expected.hpp

A C++17 simplified emulation of C++23 std::expected using std::variant.

## filesystem.hpp

Support for filesystem, posix file functions, and file scanning closures. This
header presumes a c++ compiler with filesystem runtime support.

## func.hpp

Separate header for mostly async and template optimized functions. Some of
this used to be part of tasks.hpp.

## hash.hpp

Thread-safe consistent hash support for cross-platform distributed computing.
Any openssl EVP digest can be used with the default being sha256. This
currently supports 64 bit big endian consistent hash function and a ring
buffer for 64 bit scattered distributed keys.

## keyfile.hpp

This allows for parsing config files that may be divided into \[sections\] and
have key=value key pairs in each section.  It allows for an instantiation of a
section as a collection of keys as well as iterating over named sections and
key collections. It can also be used to modify and re-write the key file with
changed data.

## list.hpp

Specialized optimized single linked list class.

## memory.hpp

Low level memory operations, allocator schemes, and byte array classes. This
includes safe low level memory functions.

## monadic.hpp

Monadic operations and wrappers based on std::optional.

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

Access process properties and execute programs. Automatic management of system
descriptors and mapping is offered. This also includes dso plugin load support
and some generic posix features that may require deep platform support to
effectively emulate on some targets.

## random.hpp

Generate random keys and data using openssl rand functions. Also has some
utility functions like b64 support for binary data and to manipulate keys.

## ranges.hpp

A simplified C++17 version of std::ranges. It also includes some features not
found in C++20.

## scan.hpp

Common functions to parse and extract fields like numbers and quoted strings
from a string view. As a scan function extracts, it also updates the view. The
low level scan functions will eventually be driven from a scan template class
that has a format string much like format. Other upper level utility functions
will also be provided.

## select.hpp

Select is a kind of functional enumerated generic type that can do simple
switch selection for generic types thru lambda expressions. A selection can
even include multiple types, such as selecting from both integers and
strings.

## serial.hpp

Support serial I/O operations through a serial port or ptty device. Includes
support for line buffered and timed input. Serial support is provided for both
modern posix and windows platforms.

## sign.hpp

Public key signing and verification support using pem files and certificate
objects.

## socket.hpp

Generic C++ socket library. This stand-alone header deals with low level socket
behavior, including multicast, and could be thought of as a C++ wrapper for the
BSD socket api. This also provides address lookup and manipulations, access to
name services, and many convenient low level utility functions. A special
service is provided for identifying network interfaces.

## stream.hpp

Generic C++ network streams based on i/o stream classes. These are typically
used to make client connections to a server, or to accept a listener. The
design assumption is one would create an instance for a given network session
that may process in it's own thread. The lifetime of such a thread and object
would be associated with the lifetime of the network session itself.

The primary constructors can create a new connected socket, or can receive an
accepted socket from a listener. The header is purely stand-alone, though it
often would also be used with socket.hpp for servers or useful utility
functions. Being stand-alone it could be combined with other kinds of C++
networking libraries easily without a lot of overlap.

## secure.hpp

Secure socket support (ssl) as C++ streams. This is based on the streams
class, but, of course must be linked with libssl to use.

## strings.hpp

Generic C++ string related templates and functions.  This typically covers
string functions either still missing in the C++ standard, or that are
introduced only very recently.

## sync.hpp

Thread and sync templates. This includes special sync pointer and container
templates that offer public access objects to protected data only when a lock
(mutex or shared) is being actively asserted. This behavior is very similar to
rust locking access and gives stronger guarantees about access safety while
also associating the actual lock with the data being protected rather than as
completely unrelated data structures. Sync also includes other kinds of thread
synchronization objects such as semaphores, windows style events, golang style
wait groups, and barriers.

## tasks.hpp

This provides a task and function queue dispatch system. This is used to queue
and dispatch arbitrary anonymous lambda captures wrapped that will run inside
the scope of a single service dispatch thread, or scheduled and ran from a
timer thread. The use of a single service dispatch thread makes it easy to
write service components offering ordered execution that can alteres private
object states without requiring thread locking. Async provides calling
functions with detached threads and await provides futures much like what
await does for asynchronous methods in C#.

I primarly use std::function<void()> in tasks.hpp because often I am placing
different arbitrary functions onto queues that are called later from another
thread. I also avoid argukment wrapping when I do this so I pass arguments by
capture. I may later introduce a saperate templated version of tasks that
only bind and queue one specific kind of function template for optimized
performance.

## templates.hpp

Some very generic, universal, miscellaneous templates and functions. This also
is used to introduce new language-like "features" like init and defer.

## x509.hpp

Basic support for x509 certificate objects.

## linting

Since this is common code extensive support exists for linting and static
analysis.
