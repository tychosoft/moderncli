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

## endian.hpp

Functions to store into and access memory pointer data by endian order.

## expected.hpp

A C++17 simplified emulation of C++23 std::expected using std::variant.

## filesystem.hpp

Support for filesystem, posix file functions, and file scanning closures. This
header presumes a c++ compiler with filesystem runtime support.

## keyfile.hpp

This allows for parsing config files that may be broken into \[sections\] and
have key=value key pairs in each section.  It allows for an instantiation of a
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

Access process properties and execute programs. Automatic management of system
descriptors and mapping is offered. This also includes dso plugin load support
and some generic posix features that may require deep platform support to
effectively emulate on some targets.

## random.hpp

Generate random keys and data using openssl rand functions. Also has some
utility functions like b64 support for binary data and to manipulate keys.

## serial.hpp

Support serial I/O operations through a serial port or ptty device. Includes
support for line buffered and timed input. Serial support is provided for both
modern posix and windows platforms.

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

In the future this will include secure socket support.

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

A task and function queue dispatch system. This is used to queue and dispatch
arbitration actor lambdas with function arguments wrapped in an outer service
function inside the scope of a service thread. This makes it easy to write
service components that offers ordered execute in a thread queue without
requiring locking. This is a generic and reusable representation of basic
Coventry server architecture.

Tasks includes timers. This allows timer context lambdas to be queued and dispatched
at known times. Timers also support the idea of periodic tasks by auto restarting.
Timers provide an application level cron service and have a private thread context to
execute a timer in.

## templates.hpp

Some very generic, universal, miscellaneous templates and functions. This also
is used to introduce new language-like "features" like init and defer.

## linting

Since this is common code extensive support exists for linting and static
analysis.
