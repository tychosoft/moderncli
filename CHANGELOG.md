# Release Notes

## v1.0.0
- Some experimental headers removed (will be back in 1.1)

## v0.9.9
- Updated memory for bordeaux
- Fix bind and make any addresses
- Add expected host method to address
- Deprecate old and unused functions
- Supports C++20 and later std::format
- Change current thread priority added
- Reduce confusion around socket type
- Add pending limit to task dispatch

## v0.9.8
- Fix event sync
- Extra string conversions
- Add gcm mode ciphers
- X509 string and time conversions
- Simple internet host and bind resolvers
- Updated presets
- Vcpkg support and build update
- Expand socket address functionality

## v0.9.7
- Prepare print header for future C++20 format behaviors
- Remove older function dispatch in favor of lambda tasks
- Simplify and use captures only for timer tasks
- Simplify task queue to use lambda capture only
- Add function queue notify method

## v0.9.6
- Narrow scope of tycho print and format
- Introduce ssl streaming
- Introduce memory header
- Separate character encoding from random
- Optimize and differentiate function from task queues

## v0.9.5
- Fixed inet host for sockaddr
- Fixed crypto namespace
- Added generic x509 type
- Add x509 certificate verification
- Cross-platform fifo client and server
- Elliptical key pairs and signing support
- Introduce timer queue
- Fix and simplify semaphore
- Fix and improve barrier
- Extend sync with events
- Add C++ waitgroup to sync
- Expand socket functionality

## v0.9.4
- Big integer math support
- Task and function thread queue dispatch
- Optimized code

## v0.9.3
- Simplify socket and serial errors
- Acceptor lambda for listener sockets
- Added network interfaces service
- Added multicast socket support
- Introduced streams and network tcp
- Improved b64 support
- Pure standalone crypto header
- Improved socket address api
- Improved serial error handling
- Improved socket error handling
- Updated project info
- Created gitlab ci pipeline
- Do not open non-tty device on windows serial

## v0.9.2
- Enhanced i/o templates
- Print output to serial device
- Improve serial format parsing
- Windows serial support
- Remove non-portable serial features
- Mini cross-platform posix i/o in fsys

## v0.9.1
- Add checksum and crc support
- Serial timed support extended
- Serial session functions and echo mode
- Serial errors and better input support
- Improve and simplify key support
- Redo project structure
- Fix windows headers
- Fix flow control, add drain flag
- Improve headers
- Mapped data memory access

## v0.9.0
- Standardize templates.hpp for language features
- Get process posix time value
- Basic memory map support
- Improved lockfree containers
- Added generic abstract templates header
- Added scan shell command from popen
- Added scan for c file pointers
- Introduced cipher support

## v0.8.5
- Add is\_tty function
- Introduce dso plugin loader
- Improve key management
- Add hmac support to digest
- Make endian constexpr
- Added b64 support to crypto
- Add u8verify function

## v0.8.4
- Added expected header
- Add process detach for posix
- Introduce crit with newer quick\_exit
- Fix exit on spawn failure

## v0.8.3
- Modernize streams, time, and print plugin support
- Add atomics / lockfree types
- Native file handle support
- Socket test fix for netbsd
- Introduced init\_t for golang-like init() lambda.

## v0.8.2
- Iso date and time output
- Print socket address
- Add filesystem print support
- Updated linting practices
- Added formatted system logger
- Serial support requires termios.h header

## v0.8.1
- Start of socket testing
- Deprecate print\_format()
- Updated basic docs
- Fixed serial for posix

## v0.8.0
- Project re-introduced by Tycho Softworks.

