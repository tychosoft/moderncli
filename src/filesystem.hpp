// Copyright (C) 2022 Tycho Softworks.
// This code is licensed under MIT license.

#ifndef TYCHO_FILESYSTEM_HPP_
#define TYCHO_FILESYSTEM_HPP_

#include <filesystem>
#include <type_traits>
#include <algorithm>
#include <iostream>
#include <fstream>
#include <string>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cerrno>

#include <sys/types.h>
#include <fcntl.h>

#ifndef _MSC_VER
#include <unistd.h>
#else
#include <BaseTsd.h>
using ssize_t = SSIZE_T;
#endif

#if defined(_MSC_VER) || defined(__MINGW32__) || defined(__MINGW64__) || defined(WIN32)
#if _WIN32_WINNT < 0x0600 && !defined(_MSC_VER)
#undef  _WIN32_WINNT
#define _WIN32_WINNT    0x0600  // NOLINT
#endif
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <aclapi.h>
#include <io.h>
#else
#include <sys/mman.h>
#include <termios.h>
#endif

#if defined(__OpenBSD__)
#define stat64 stat     // NOLINT
#define fstat64 fstat   // NOLINT
#endif

#if defined(_MSC_VER) || defined(__MINGW32__) || defined(__MINGW64__) || defined(WIN32)
inline auto getline_w32(char **lp, std::size_t *size, FILE *fp) -> ssize_t {
    std::size_t pos{0};
    int c{EOF};

    if(lp == nullptr || fp == nullptr || size == nullptr) return -1;
    c = getc(fp);
    if(c == EOF) return -1;

    if(*lp == nullptr) {
        *lp = static_cast<char *>(std::malloc(128)); // NOLINT
        if(*lp == nullptr) return -1;
        *size = 128;
    }

    pos = 0;
    while(c != EOF) {
        if (pos + 1 >= *size) {
            auto new_size = *size + (*size >> 2);
            if(new_size < 128) {
                new_size = 128;
            }
            auto new_ptr = static_cast<char *>(realloc(*lp, new_size)); // NOLINT
            if(new_ptr == nullptr) return -1;
            *size = new_size;
            *lp = new_ptr;
        }

        (reinterpret_cast<unsigned char *>(*lp))[pos ++] = c;
        if(c == '\n') break;
        c = getc(fp);
    }
    (*lp)[pos] = '\0';
    return static_cast<ssize_t>(pos);
}
#endif

namespace tycho::fsys {
using namespace std::filesystem;

#if defined(_MSC_VER) || defined(__MINGW32__) || defined(__MINGW64__) || defined(WIN32)
enum class mode : int {rw = _O_RDWR, rd = _O_RDONLY, wr = _O_WRONLY, append = _O_APPEND | _O_CREAT | _O_WRONLY, always = _O_CREAT | _O_RDWR, rewrite = _O_CREAT | _O_RDWR | _O_TRUNC, exists = _O_RDWR};

template<typename T>
inline auto read(int fd, T& data) noexcept {
    static_assert(std::is_trivial_v<T>, "T must be Trivial type");
    return _read(fd, &data, sizeof(data));
}

template<typename T>
inline auto write(int fd, const T& data) noexcept {
    static_assert(std::is_trivial_v<T>, "T must be Trivial type");
    return _write(fd, &data, sizeof(data));
}

inline auto seek(int fd, off_t pos) noexcept {
    return _lseek(fd, pos, SEEK_SET);
}

inline auto tell(int fd) noexcept {
    return _lseek(fd, 0, SEEK_CUR);
}

inline auto append(int fd) noexcept {
    return _lseek(fd, 0, SEEK_END);
}

inline auto open(const fsys::path& path, mode flags = mode::rw) noexcept {
    const auto filename = path.string();
    return _open(filename.c_str(), int(flags) | _O_BINARY, 0664);
}

inline auto close(int fd) noexcept {
    return _close(fd);
}

inline auto read(int fd, void *buf, std::size_t len) {
    return _read(fd, buf, len);
}

inline auto write(int fd, const void *buf, std::size_t len) {
    return _write(fd, buf, len);
}

inline auto read_at(int fd, void *buf, std::size_t len, off_t pos) {
    auto handle = (HANDLE)_get_osfhandle(fd);
    if(handle == INVALID_HANDLE_VALUE) return (DWORD)-1;
    OVERLAPPED overlapped{0};
    overlapped.Offset = static_cast<DWORD>(pos);
    overlapped.OffsetHigh = 0;
    DWORD bytesRead{0};
    if(ReadFile(handle, buf, static_cast<DWORD>(len), &bytesRead, &overlapped)) return bytesRead;
    return (DWORD)-1;
}

inline auto write_at(int fd, const void *buf, std::size_t len, off_t pos) {
    auto handle = (HANDLE)_get_osfhandle(fd);
    if(handle == INVALID_HANDLE_VALUE) return (DWORD)-1;
    OVERLAPPED overlapped{0};
    overlapped.Offset = static_cast<DWORD>(pos);
    overlapped.OffsetHigh = 0;
    DWORD bytesWritten{0};
    if(WriteFile(handle, buf, static_cast<DWORD>(len), &bytesWritten, &overlapped)) return bytesWritten;
    return (DWORD)-1;
}

inline auto tty_input() {
    return _open("CONIN$", _O_RDONLY);
}

inline auto tty_output() {
    return _open("CONOUT$", _O_WRONLY);
}

template<typename T>
inline auto read_at(int fd, T& data, off_t pos) noexcept {
    static_assert(std::is_trivial_v<T>, "T must be Trivial type");
    return fsys::read_at(fd, &data, sizeof(data), pos);
}

template<typename T>
inline auto write_at(int fd, const T& data, off_t pos) noexcept {
    static_assert(std::is_trivial_v<T>, "T must be Trivial type");
    return fsys::write_at(fd, &data, sizeof(data), pos);
}

inline auto map(int fd, std::size_t size, bool rw = false) -> void* {
    auto handle = (HANDLE)_get_osfhandle(fd);
    if(handle == INVALID_HANDLE_VALUE) return nullptr;

    auto map = CreateFileMapping(handle, nullptr, rw ? PAGE_READWRITE : PAGE_READONLY, 0, size, nullptr);
    if(map == nullptr) return nullptr;

    auto addr = MapViewOfFile(map, rw ? FILE_MAP_READ | FILE_MAP_WRITE : FILE_MAP_READ, 0, 0, size);
    CloseHandle(map);
    return addr;
}

inline void unmap(void *addr, [[maybe_unused]]std::size_t size) {
    if(addr == nullptr) return;
    UnmapViewOfFile(addr);
}

inline auto sync(int fd) {
    auto handle = (HANDLE)_get_osfhandle(fd);
    if(handle == INVALID_HANDLE_VALUE) return -1;
    if(FlushFileBuffers(handle))
        return 0;
    return -1;
}

inline auto resize(int fd, off_t size) {
    auto handle = (HANDLE)_get_osfhandle(fd);
    if(handle == INVALID_HANDLE_VALUE) return -1;
    if(SetFilePointer(handle, static_cast<DWORD>(size), nullptr, FILE_BEGIN) == EINVAL) return -1;
    return 0;
}

inline auto open_exclusive(const fsys::path& path, [[maybe_unused]] bool all = false) {
    auto filename = path.string();
    auto handle = CreateFile(filename.c_str(), GENERIC_READ|GENERIC_WRITE, FILE_SHARE_READ, nullptr, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if(handle == INVALID_HANDLE_VALUE) return -1;
    auto fd = _open_osfhandle((intptr_t)handle, _O_RDWR);
    if(fd == -1)
        CloseHandle(handle);
    return fd;
}

inline auto open_shared(const fsys::path& path) {
    auto filename = path.string();
    auto handle = CreateFile(filename.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if(handle == INVALID_HANDLE_VALUE) return -1;
    auto fd = _open_osfhandle((intptr_t)handle, _O_RDONLY);
    if(fd == -1)
        CloseHandle(handle);
    return fd;
}

inline auto native_handle(int fd) {
    auto handle = _get_osfhandle(fd);
    if(handle == -1) return static_cast<void *>(nullptr);
    return reinterpret_cast<void *>(handle);
}

inline auto native_handle(std::FILE *fp) {
    return native_handle(_fileno(fp));
}
#else
enum class mode : int {rw = O_RDWR, rd = O_RDONLY, wr = O_WRONLY, append = O_APPEND | O_CREAT | O_WRONLY, always = O_CREAT | O_RDWR, rewrite = O_CREAT | O_RDWR | O_TRUNC, exists = O_RDWR};

template<typename T>
inline auto read(int fd, T& data) noexcept {
    static_assert(std::is_trivial_v<T>, "T must be Trivial type");
    return ::read(fd, &data, sizeof(data));
}

template<typename T>
inline auto write(int fd, const T& data) noexcept {
    static_assert(std::is_trivial_v<T>, "T must be Trivial type");
    return ::write(fd, &data, sizeof(data));
}

template<typename T>
inline auto read_at(int fd, T& data, off_t pos) noexcept {
    static_assert(std::is_trivial_v<T>, "T must be Trivial type");
    return ::pread(fd, &data, sizeof(data), pos);
}

template<typename T>
inline auto write_at(int fd, const T& data, off_t pos) noexcept {
    static_assert(std::is_trivial_v<T>, "T must be Trivial type");
    return ::pwrite(fd, &data, sizeof(data), pos);
}

inline auto tty_input() {
    return open("/dev/tty", O_RDONLY);
}

inline auto tty_output() {
    return open("/dev/tty", O_WRONLY);
}

inline auto seek(int fd, off_t pos) noexcept {
    return ::lseek(fd, pos, SEEK_SET);
}

inline auto tell(int fd) noexcept {
    return ::lseek(fd, 0, SEEK_CUR);
}

inline auto append(int fd) noexcept {
    return ::lseek(fd, 0, SEEK_END);
}

inline auto open(const fsys::path& path, mode flags = mode::rw) noexcept {
    return ::open(path.string().c_str(), int(flags), 0664);
}

inline auto close(int fd) noexcept {
    return ::close(fd);
}

inline auto read(int fd, void *buf, std::size_t len) {
    return ::read(fd, buf, len);
}

inline auto read_at(int fd, void *buf, std::size_t len, off_t pos) {
    return ::pread(fd, buf, len, pos);
}

inline auto map(int fd, std::size_t size, bool rw = false) -> void * {
    void *addr = ::mmap(nullptr, size, rw ? PROT_READ | PROT_WRITE : PROT_READ, MAP_SHARED, fd, 0);
    if(addr == MAP_FAILED) return nullptr;
    return addr;
}

inline auto unmap(void *addr, std::size_t size) {
    if(addr == MAP_FAILED || addr == nullptr)
        return;
    munmap(addr, size);
}

inline auto write(int fd, const void *buf, std::size_t len) {
    return ::write(fd, buf, len);
}

inline auto write_at(int fd, const void *buf, std::size_t len, off_t pos) {
    return ::pwrite(fd, buf, len, pos);
}

inline auto sync(int fd) {
    return ::fsync(fd);
}

inline auto resize(int fd, off_t size) {
    return ::ftruncate(fd, size);
}

inline auto native_handle(int fd) {
    return fd;
}

inline auto open_exclusive(const fsys::path& path, bool all = false) {
    const auto file = path.string();
    auto fd = ::open(file.c_str(), O_RDWR | O_CREAT | O_EXCL, all ? 0644 : 0640);
    if(fd == -1) return -1;
    struct flock lock{};
    lock.l_type = F_WRLCK;
    lock.l_whence = SEEK_SET;
    lock.l_start = 0;
    lock.l_len = 0;
    if(fcntl(fd, F_SETLK, &lock) == -1) {
        close(fd);
        return -1;
    }
    return fd;
}

inline auto open_shared(const fsys::path& path) {
    const auto filename = path.string();
    return ::open(filename.c_str(), O_RDONLY);
}

inline auto native_handle(std::FILE *fp) {
    return fileno(fp);
}
#endif

class posix_file {
public:
    using size_type = off_t;

    posix_file() noexcept = default;
    posix_file(const posix_file&) = delete;
    auto operator=(const posix_file&) noexcept -> auto& = delete;

    // cppcheck-suppress noExplicitConstructor
    posix_file(int fd) noexcept : fd_(fd) {}

    // cppcheck-suppress noExplicitConstructor
    posix_file(const fsys::path& path, mode flags = mode::rw) noexcept :
    fd_(fsys::open(path, flags)) {}

    posix_file(posix_file&& other) noexcept : fd_(other.fd_) {
        other.fd_ = -1;
    }

    virtual ~posix_file() {
        if(fd_ != -1)
            fsys::close(fd_);
    }

    auto operator=(posix_file&& other) noexcept -> auto& {
        if(this == &other) return *this;
        release();
        fd_ = other.fd_;
        return *this;
    }

    auto operator=(int fd) noexcept -> posix_file& {
        release();
        fd_ = fd;
        return *this;
    }

    auto operator*() const noexcept {
        return fd_;
    }

    operator int() const noexcept {
        return fd_;
    }

    explicit operator bool() const noexcept {
        return fd_ != -1;
    }

    auto operator!() const noexcept {
        return fd_ == -1;
    }

    auto is_open() const noexcept {
        return fd_ != -1;
    }

    void open(const fsys::path& path, mode flags = mode::rw) noexcept {
        release();
        fd_ = fsys::open(path, flags);
    }

    auto seek(off_t pos) const noexcept -> off_t {
        return fd_ == -1 ? -1 : fsys::seek(fd_, pos);
    }

    auto tell() const noexcept -> off_t {
        return fd_ == -1 ? -1 : fsys::tell(fd_);
    }

    auto size() const noexcept -> off_t {
        if(fd_ == -1) return -1;
        auto pos = tell();
        if(pos < 0) return pos;
        if(fsys::append(fd_) < 0) return -1;
        return seek(pos);
    }

    auto resize(off_t size) const noexcept -> off_t {
        return fd_ == -1 ? -1 : fsys::resize(fd_, size);
    }

    auto rewrite() const noexcept -> off_t {
        return resize(0);
    }

    auto rewind() const noexcept -> off_t {
        return seek(0);
    }

    auto append() const noexcept -> off_t {
        return fd_ == -1 ? -1 : fsys::append(fd_);
    }

    auto read(void *buf, std::size_t len) const noexcept {
        return fd_ == -1 ? -1 : fsys::read(fd_, buf, len);
    }

    auto write(const void *buf, std::size_t len) const noexcept {
        return fd_ == -1 ? -1 : fsys::write(fd_, buf, len);
    }

    auto read_at(void *buf, std::size_t len, off_t pos) const noexcept {
        return fd_ == -1 ? -1 : fsys::read_at(fd_, buf, len, pos);
    }

    auto write_at(const void *buf, std::size_t len, off_t pos) const noexcept {
        return fd_ == -1 ? -1 : fsys::write_at(fd_, buf, len, pos);
    }

    auto map(std::size_t size, bool rw = false) const noexcept {
        return fd_ == -1 ? nullptr : fsys::map(fd_, size, rw);
    }

    auto sync() const noexcept {
        if(fd_ == -1) return;
        fsys::sync(fd_);
    }

    template<typename T>
    auto map(int fd) const noexcept -> T* {
        return fd_ == -1 ? nullptr : fsys::map(fd, sizeof(T));
    }

    template<typename T>
    auto read(T& data) const noexcept {
        static_assert(std::is_trivial_v<T>, "T must be Trivial type");
        return fd_ == -1 ? -1 : fsys::read(fd_, &data, sizeof(data));
    }

    template<typename T>
    auto write(const T& data) const noexcept {
        static_assert(std::is_trivial_v<T>, "T must be Trivial type");
        return fd_ == -1 ? -1 : fsys::write(fd_, &data, sizeof(data));
    }

    template<typename T>
    auto read_at(T& data, off_t pos) const noexcept {
        static_assert(std::is_trivial_v<T>, "T must be Trivial type");
        return fd_ == -1 ? -1 : fsys::read_at(fd_, &data, sizeof(data), pos);
    }

    template<typename T>
    auto write_at(const T& data, off_t pos) const noexcept {
        static_assert(std::is_trivial_v<T>, "T must be Trivial type");
        return fd_ == -1 ? -1 : fsys::write_at(fd_, &data, sizeof(data), pos);
    }

protected:
    int fd_{-1};

    void release() noexcept {
        if(fd_ != -1)
            fsys::close(fd_);
        fd_ = -1;
    }

    void assign(int fd) noexcept {
        if(fd_ != -1)
            fsys::close(fd_);
        fd_ = fd;
    }
};

template<typename T, std::size_t S = sizeof(T)>
class pager_file : private posix_file {
public:
    static constexpr std::size_t page_size = S;
    using size_type = off_t;
    using page_type = T;

    pager_file() noexcept : posix_file() {}
    pager_file(const pager_file&) = delete;
    auto operator=(const pager_file&) -> auto& = delete;

    pager_file(pager_file&& other) noexcept :
    posix_file(), offset_(other.offset_), header_(other.header_), access_(other.access_), max_(other.max_), err_(other.err_) {
        posix_file::assign(other.fd_);
        other.offset_ = 0;
        other.header_ = nullptr;
        other.max_ = 0;
        other.err_ = 0;
    }

    ~pager_file() override {
        sync();
        if(header_) {
            delete[] header_;
            header_ = nullptr;
        }
    }

    //cppcheck-suppress duplInheritedMember
    explicit operator bool() const noexcept {
        return posix_file::is_open();
    }

    //cppcheck-suppress duplInheritedMember
    auto operator!() const noexcept {
        return !posix_file::is_open();
    }

    auto operator=(pager_file&& other) noexcept -> auto& {
        if(this == &other) return *this;
        posix_file::assign(other.fd_);
        offset_ = other.offset_;
        header_ = other.header_;
        access_ = other.access_;
        max_ = other.max_;
        err_ = other.err_;
        other.offset_ = 0;
        other.header_ = nullptr;
        other.max_ = 0;
        other.err_ = 0;
    }

    auto get(off_t page, T& ref) const noexcept {
        if(access_ == mode::wr) return false;
        if(max_ && page >= max_) return false;
        if(read_at(ref, offset_ + (page * page_size)) == sizeof(T)) return true;
        err_ = errno;
        return false;
    }

    auto put(off_t page, const T& ref) const noexcept {
        if(access_ == mode::rd) return false;
        if(max_ && page >= max_) return false;
        if(write_at(ref, offset_ + (page * page_size)) == sizeof(T)) return true;
        err_ = errno;
        return false;
    }

    auto err() const noexcept {
        auto code = err_;
        if(code != EBADF)
            err_ = 0;
        return code;
    }

    template<typename H>
    auto header() -> H& {
        static_assert(std::is_trivial_v<H>, "Header must be Trivial type");

        if(!header_) throw std::runtime_error("No header loaded");
        if(sizeof(H) > offset_) throw std::runtime_error("Header too large");
        return *(reinterpret_cast<H*>(header_));
    }

    //cppcheck-suppress duplInheritedMember
    auto is_open() const noexcept {
        return posix_file::is_open();
    }

    //cppcheck-suppress duplInheritedMember
    void sync() const noexcept {
        if(!posix_file::is_open() || access_ == mode::rd) return;
        if(header_) {
            if(write_at(header_, offset_, 0) != offset_)
                err_ = errno;
        }
        posix_file::sync();
    }

    //cppcheck-suppress duplInheritedMember
    auto size() const noexcept {
        if(!posix_file::is_open()) return 0;
        auto fs = posix_file::size() - offset_;
        return (fs % page_size) ? (fs / page_size) + 1 : fs / page_size;
    }

    //cppcheck-suppress duplInheritedMember
    auto resize(off_t size) noexcept {
        if(access_ == mode::rd) return false;
        if(posix_file::resize((size * page_size) + offset_) < 0) {
            err_ = errno;
            return false;
        }
        max_ = size;
        return true;
    }

    //cppcheck-suppress duplInheritedMember
    auto rewrite() noexcept {
        return resize(0);
    }

    //cppcheck-suppress duplInheritedMember
    auto rewind() noexcept {
        max_ = 0;
    }

    static auto shared(const fsys::path& path, off_t offset = 0) noexcept {
        return pager_file(open_shared(path), offset, mode::rd);
    }

    static auto exclusive(const fsys::path& path, bool all = false, off_t offset = 0) noexcept {
        return pager_file(open_exclusive(path, all), offset);
    }

    static auto append(const fsys::path& path, off_t offset = 0) noexcept {
        return pager_file(fsys::open(path, mode::append), offset, mode::wr);
    }

    static auto create(const fsys::path& path, off_t offset = 0) noexcept {
        return pager_file(fsys::open(path, mode::always), offset);
    }

protected:
    explicit pager_file(int fd, off_t offset, mode access = mode::rw) noexcept :
    posix_file(), offset_(offset), access_(access), err_(fd < 0 ? EBADF : 0) {
        posix_file::assign(fd);
        if(!offset_ || !posix_file::is_open() || access_ == mode::wr) return;
        header_ = new(std::nothrow) uint8_t[offset_];
        memset(header_, 0, offset_);
        read_at(header_, offset_, 0);
    }

private:
    static_assert(sizeof(T) <= S, "pager alignment broken");
    static_assert(std::is_trivial_v<T>, "T must be Trivial type");

    off_t offset_{0};
    uint8_t *header_{nullptr};
    mode access_{mode::rw};
    off_t max_{0};
    mutable int err_{0};
};

using fd_t = posix_file;

template<typename T>
inline auto map(int fd) noexcept -> T* {
    return fsys::map(fd, sizeof(T));
}

template<typename T>
inline auto unmap(T* obj) noexcept {
    fsys::unmap(obj, sizeof(T));
}

inline auto make_exclusive(const fsys::path& path, bool all = false) noexcept {
    return posix_file(open_exclusive(path, all));
}

inline auto make_shared(const fsys::path& path) noexcept {
    return posix_file(open_shared(path));
}

inline auto make_access(const fsys::path& path, mode access = mode::exists) noexcept {
    return posix_file(fsys::open(path, access));
}
} // end fsys namespace

namespace tycho {
template<typename Func>
inline auto scan_stream(std::istream& input, Func func) {
    static_assert(std::is_invocable_v<Func, std::string>, "Func must be callable");
    static_assert(std::is_convertible_v<std::invoke_result_t<Func, std::string>, bool>, "Result must be bool");

    std::string line;
    std::size_t count{0};
    while(std::getline(input, line)) {
        if(!func(line)) break;
        ++count;
    }
    return count;
}

template<typename Func>
inline auto scan_file(const fsys::path& path, Func func) {
    static_assert(std::is_invocable_v<Func, std::string>, "Func must be callable");
    static_assert(std::is_convertible_v<std::invoke_result_t<Func, std::string>, bool>, "Result must be bool");

    std::fstream input(path);
    std::string line;
    std::size_t count{0};
    while(std::getline(input, line)) {
        if(!func(line)) break;
        ++count;
    }
    return count;
}

#if !defined(_MSC_VER) && !defined(__MINGW32__) && !defined(__MINGW64__) && !defined(WIN32)
template<typename Func>
inline auto scan_file(std::FILE *file, Func func, std::size_t size = 0) {
    static_assert(std::is_invocable_v<Func, const char*, std::size_t>, "Func must be callable");
    static_assert(std::is_convertible_v<std::invoke_result_t<Func, const char*, std::size_t>, bool>, "Func  must return bool");

    char *buf{nullptr};
    std::size_t count{0};
    if(size)
        buf = static_cast<char *>(std::malloc(size));    // NOLINT
    while(!feof(file)) {
        auto len = getline(&buf, &size, file);
        if(len < 0 || !func({buf, std::size_t(len)})) break;
        ++count;
    }
    if(buf)
        std::free(buf);  // NOLINT
    return count;
}

template<typename Func>
inline auto scan_command(const std::string& cmd, Func func, std::size_t size = 0) {
    auto file = popen(cmd.c_str(), "r");
    if(!file) return std::size_t(0);

    auto count = scan_file(file, func, size);
    pclose(file);
    return count;
}
#else
template<typename Func>
inline auto scan_file(std::FILE *file, Func func, std::size_t size = 0) {
    static_assert(std::is_invocable_v<Func, const char*, std::size_t>, "Func must be callable");
    static_assert(std::is_convertible_v<std::invoke_result_t<Func, const char*, std::size_t>, bool>, "Func  must return bool");

    char *buf{nullptr};
    std::size_t count{0};
    if(size)
        buf = static_cast<char *>(std::malloc(size));    // NOLINT
    while(!feof(file)) {
        auto len = getline_w32(&buf, &size, file);
        if(len < 0 || !func({buf, std::size_t(len)})) break;
        ++count;
    }
    if(buf)
        std::free(buf);  // NOLINT
    return count;
}

template<typename Func>
inline auto scan_command(const std::string& cmd, Func func, std::size_t size = 0) {
    auto file = _popen(cmd.c_str(), "r");

    if(!file) return std::size_t(0);
    auto count = scan_file(file, func, size);
    _pclose(file);
    return count;
}
#endif

inline auto make_input(const fsys::path& path, std::ios::openmode mode = std::ios::binary) {
    std::ifstream file;
    file.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    file.open(path, mode);
    return file;
}

inline auto make_output(const fsys::path& path, std::ios::openmode mode = std::ios::binary) {
    std::ofstream file;
    file.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    file.open(path, mode);
    return file;
}

template<typename Func>
inline auto scan_directory(const fsys::path& path, Func func) {
    using Entry = const fsys::directory_entry&;
    static_assert(std::is_invocable_v<Func, Entry>, "Func must be callable");
    static_assert(std::is_convertible_v<std::invoke_result_t<Func, Entry>, bool>, "Func must return bool");

    auto dir = fsys::directory_iterator(path);
    return std::count_if(begin(dir), end(dir), func);
}

template<typename Func>
inline auto scan_recursive(const fsys::path& path, Func func) {
    using Entry = const fsys::directory_entry&;
    static_assert(std::is_invocable_v<Func, Entry>, "Func must be callable");
    static_assert(std::is_convertible_v<std::invoke_result_t<Func, Entry>, bool>, "Func must return bool");

    auto dir = fsys::recursive_directory_iterator(path, fsys::directory_options::skip_permission_denied);
    return std::count_if(begin(dir), end(dir), func);
}

inline auto to_string(const fsys::path& path) {
    return path.string();
}
} // end namespace

#if defined(TYCHO_PRINT_HPP_) && (__cplusplus < 202002L)
template<>
class fmt::formatter<tycho::fsys::path> {
public:
    static constexpr auto parse(format_parse_context& ctx) {
        return ctx.begin();
    }

    template<typename Context>
    constexpr auto format(const tycho::fsys::path& path, Context& ctx) const {
        return format_to(ctx.out(), "{}", std::string{path.string()});
    }
};
#elif defined(TYCHO_PRINT_HPP)
template<>
struct std::formatter<tycho::fsys::path> {
    constexpr auto parse(std::format_parse_context& ctx) {
        return ctx.begin();
    }

    template<typename FormatContext>
    auto format(const tycho::fsys::path& path, FormatContext& ctx) const {
        return std::format_to(ctx.out(), "{}", path.string());
    }
};
#endif

inline auto operator<<(std::ostream& out, const tycho::fsys::path& path) -> std::ostream& {
    out << path.string();
    return out;
}
#endif
