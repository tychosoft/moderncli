// Copyright (C) 2022 Tycho Softworks.
// This code is licensed under MIT license.

#ifndef TYCHO_PROCESS_HPP_
#define TYCHO_PROCESS_HPP_

#include <fstream>
#include <memory>
#include <string_view>
#include <thread>
#include <string>
#include <cstdlib>
#include <cstring>
#include <optional>
#include <stdexcept>
#include <functional>
#include <type_traits>

#if defined(_MSC_VER) || defined(__MINGW32__) || defined(__MINGW64__) || defined(WIN32)
#if _WIN32_WINNT < 0x0600 && !defined(_MSC_VER)
#undef  _WIN32_WINNT
#define _WIN32_WINNT    0x0600  // NOLINT
#endif
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winsock2.h>
#include <tlhelp32.h>
#include <process.h>
#include <handleapi.h>
#include <io.h>
#include <fcntl.h>
#ifndef quick_exit
#define quick_exit(x) ::exit(x)         // NOLINT
#define at_quick_exit(x) ::atexit(x)    // NOLINT
#endif
#define MAP_FAILED  nullptr
#else
#include <csignal>
#include <unistd.h>
#include <dlfcn.h>
#include <sched.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <sys/stat.h>

#ifndef RTLD_GLOBAL
#define RTLD_GLOBAL 0
#endif
#endif

#ifdef __FreeBSD__
#define execvpe(p,a,e)  exect(p,a,e)
#endif

namespace tycho::process {
#if defined(_MSC_VER) || defined(__MINGW32__) || defined(__MINGW64__) || defined(WIN32)
using id_t = intptr_t;
using addr_t = FARPROC;
using handle_t = HANDLE;

inline constexpr auto dso_suffix = ".dll";

inline auto invalid_handle() noexcept -> handle_t {
    return INVALID_HANDLE_VALUE;
}

class ref_t final {
public:
    ref_t() noexcept = default;

    // cppcheck-suppress noExplicitConstructor
    ref_t(handle_t handle) noexcept : handle_(handle) {}
    ref_t(const ref_t& other) noexcept {
        auto pid = GetCurrentProcess();
        DuplicateHandle(pid, other.handle_, pid, &handle_, 0, FALSE, DUPLICATE_SAME_ACCESS);
    }

    ref_t(ref_t&& other) noexcept : handle_(other.handle_) {
        other.handle_ = invalid_handle();
    }

    ~ref_t() {
        if(handle_ != invalid_handle())
            CloseHandle(handle_);
    }

    auto operator=(const ref_t& other) noexcept -> ref_t& {
        if(&other == this) return *this;
        if(handle_ != invalid_handle())
            CloseHandle(handle_);

        auto pid = GetCurrentProcess();
        DuplicateHandle(pid, other.handle_, pid, &handle_, 0, FALSE, DUPLICATE_SAME_ACCESS);
        return *this;
    }

    auto operator=(ref_t&& other) noexcept -> ref_t& {
        if(handle_ != invalid_handle())
            CloseHandle(handle_);
        handle_ = other.handle_;
        other.handle_ = invalid_handle();
        return *this;
    }

    auto operator=(handle_t handle) noexcept -> ref_t& {
        if(handle_ != invalid_handle())
            CloseHandle(handle_);
        handle_ = handle;
        return *this;
    }

    operator bool() const noexcept {
        return handle_ != invalid_handle();
    }

    auto operator!() const noexcept {
        return handle_ == invalid_handle();
    }

    operator HANDLE() const noexcept {
        return handle_;
    }

    auto operator*() const noexcept {
        return handle_;
    }

    auto operator==(const ref_t& other) const noexcept {
        return handle_ == other.handle_;
    }

    auto operator!=(const ref_t& other) const noexcept {
        return handle_ == other.handle_;
    }

    void release() noexcept {
        if(handle_ != invalid_handle())
            CloseHandle(handle_);
        handle_ = invalid_handle();
    }

private:
    handle_t handle_{invalid_handle()};
};

class map_t final {
public:
    map_t() = default;
    map_t(const map_t&) = delete;
    auto operator=(const map_t&) -> auto& = delete;

    explicit map_t(const std::string& path, int mode = 0640) {
        access(path, mode);
    }

    map_t(handle_t h, std::size_t size, bool rw = true, [[maybe_unused]] bool priv = false, off_t offset = 0) noexcept : size_(size) {
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4293)
#endif
        const int64_t offset64 = offset;
        const DWORD dwFileOffsetLow = (sizeof(off_t) <= sizeof(DWORD)) ?
                        (DWORD)offset : (DWORD)(offset64 & 0xFFFFFFFFL);
        const DWORD dwFileOffsetHigh = (sizeof(off_t) <= sizeof(DWORD)) ?
                        (DWORD)0 : (DWORD)((offset64 >> 32) & 0xFFFFFFFFL);

        const DWORD protectAccess = (rw) ? PAGE_READWRITE : PAGE_READONLY;
        const DWORD desiredAccess = (rw) ? FILE_MAP_READ | FILE_MAP_WRITE : FILE_MAP_READ;

        const auto max = offset + off_t(size);
        const DWORD dwMaxSizeLow = (sizeof(off_t) <= sizeof(DWORD)) ? (DWORD)max : (DWORD)(max & 0xFFFFFFFFL);
        const DWORD dwMaxSizeHigh = (sizeof(off_t) <= sizeof(DWORD)) ?
                        (DWORD)0 : (DWORD)((max >> 32) & 0xFFFFFFFFL);
#ifdef _MSC_VER
#pragma warning(pop)
#endif
        handle_ = CreateFileMapping(h, nullptr, protectAccess, dwMaxSizeHigh, dwMaxSizeLow, nullptr);

        if(handle_ == nullptr) {
            addr_ = MAP_FAILED;
            return;
        }

        addr_ = MapViewOfFile(handle_, desiredAccess, dwFileOffsetHigh, dwFileOffsetLow, size);
    }

    map_t(map_t&& other) noexcept :
    addr_(other.addr_), size_(other.size_), handle_(other.handle_) {
        other.addr_ = MAP_FAILED;
        other.size_ = 0;
        other.handle_ = nullptr;
    }

    ~map_t() {
        release();
    }

    operator bool() const noexcept {
        return addr_ != MAP_FAILED;
    };

    auto operator!() const noexcept {
        return addr_ == MAP_FAILED;
    }

    auto operator*() const {
        if(addr_ == MAP_FAILED) throw std::runtime_error("no mapped handle");
        return addr_;
    }

    auto operator=(map_t&& other) noexcept -> auto& {
        addr_ = other.addr_;
        size_ = other.size_;
        handle_ = other.handle_;
        other.addr_ = MAP_FAILED;
        other.size_ = 0;
        other.handle_ = nullptr;
        return *this;
    }

    auto operator[](std::size_t pos) -> uint8_t& {
        if(addr_ == MAP_FAILED) throw std::runtime_error("no mapped handle");
        if(pos >= size_) throw std::runtime_error("outside of map range");
        return (static_cast<uint8_t *>(addr_))[pos];
    }

    auto operator[](std::size_t pos) const -> const uint8_t& {
        if(addr_ == MAP_FAILED) throw std::runtime_error("no mapped handle");
        if(pos >= size_) throw std::runtime_error("outside of map range");
        return (static_cast<uint8_t *>(addr_))[pos];
    }

    auto get_or(std::size_t pos, const void *or_else = nullptr) const -> const void * {
        if(addr_ == MAP_FAILED || pos >= size_) return or_else;
        return (static_cast<uint8_t *>(addr_)) + pos;
    }

    auto get_or(std::size_t pos, void *or_else = nullptr) -> void * {
        if(addr_ == MAP_FAILED || pos >= size_) return or_else;
        return (static_cast<uint8_t *>(addr_)) + pos;
    }

    auto data() const noexcept {
        return addr_;
    }

    auto size() const noexcept {
        return size_;
    }

    auto sync([[maybe_unused]]bool wait = false) noexcept {
        if(addr_ == MAP_FAILED) return false;
        return FlushViewOfFile(addr_, size_) == TRUE;
    }

    auto lock() noexcept {
        if(addr_ == MAP_FAILED) return false;
        if(VirtualLock((LPVOID)addr_, size_)) return true;
        return false;
    }

    auto unlock() noexcept {
        if(addr_ == MAP_FAILED) return false;
        if(VirtualUnlock((LPVOID)addr_, size_)) return true;
        return false;
    }

    auto detach() {
        auto tmp = addr_;
        addr_ = MAP_FAILED;
        size_ = 0;
        return tmp;
    }

    auto set(handle_t h, std::size_t size, bool rw = true, [[maybe_unused]]bool priv = false, off_t offset = 0) noexcept -> void *{
        if(addr_ != MAP_FAILED)
            UnmapViewOfFile(addr_);
        addr_ = MAP_FAILED;
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4293)
#endif
        const int64_t offset64 = offset;
        const DWORD dwFileOffsetLow = (sizeof(off_t) <= sizeof(DWORD)) ?
                        (DWORD)offset : (DWORD)(offset64 & 0xFFFFFFFFL);
        const DWORD dwFileOffsetHigh = (sizeof(off_t) <= sizeof(DWORD)) ?
                        (DWORD)0 : (DWORD)((offset64 >> 32) & 0xFFFFFFFFL);

        const DWORD protectAccess = (rw) ? PAGE_READWRITE : PAGE_READONLY;
        const DWORD desiredAccess = (rw) ? FILE_MAP_READ | FILE_MAP_WRITE : FILE_MAP_READ;

        const auto max = offset + off_t(size);
        [[maybe_unused]] const int64_t max64 = max;
        const DWORD dwMaxSizeLow = (sizeof(off_t) <= sizeof(DWORD)) ? (DWORD)max : (DWORD)(max64 & 0xFFFFFFFFL);
        const DWORD dwMaxSizeHigh = (sizeof(off_t) <= sizeof(DWORD)) ?
                        (DWORD)0 : (DWORD)((max64 >> 32) & 0xFFFFFFFFL);
#ifdef _MSC_VER
#pragma warning(pop)
#endif
        handle_ = CreateFileMapping(h, nullptr, protectAccess, dwMaxSizeHigh, dwMaxSizeLow, nullptr);

        if(handle_ == nullptr) {
            addr_ = MAP_FAILED;
            return MAP_FAILED;
        }

        addr_ = MapViewOfFile(handle_, desiredAccess, dwFileOffsetHigh, dwFileOffsetLow, size);
        size_ = size;
        return addr_;
    }

    void release() noexcept {
        if(addr_ != MAP_FAILED) {
            UnmapViewOfFile(addr_);
            addr_ = MAP_FAILED;
        }
        if(handle_) {
            CloseHandle(handle_);
            handle_ = nullptr;
        }
        size_ = 0;
    }

    auto create(const std::string& path, size_t size, [[maybe_unused]]int mode = 0640) noexcept -> void * {
        release();
        handle_ = CreateFileMappingA(INVALID_HANDLE_VALUE, nullptr, PAGE_READWRITE, 0, size, path.c_str());
        if(handle_ == nullptr) return MAP_FAILED;

        size_ = size;
        addr_ = MapViewOfFile(handle_, FILE_MAP_READ | FILE_MAP_WRITE, 0, 0, size);
        return addr_;
    }

    auto access(const std::string& path, [[maybe_unused]]int mode = 0640) noexcept -> void * {
        release();
        handle_ = OpenFileMappingA(FILE_MAP_READ, FALSE, path.c_str());
        if(handle_ == nullptr) return MAP_FAILED;

        addr_ = MapViewOfFile(handle_, FILE_MAP_READ, 0, 0, 0);
        if(!addr_) {
            CloseHandle(handle_);
            handle_ = nullptr;
            return MAP_FAILED;
        }

        MEMORY_BASIC_INFORMATION mbi;
        if(VirtualQuery(addr_, &mbi, sizeof(mbi))) {
            size_ = mbi.RegionSize;
            return addr_;
        }
        release();
        return MAP_FAILED;
    }

private:
    void *addr_{MAP_FAILED};
    std::size_t size_{0};
    HANDLE handle_{nullptr};
};

inline auto page_size() noexcept -> off_t {
    SYSTEM_INFO si;
    GetSystemInfo(&si);
    return off_t(si.dwPageSize);
}

inline void time(struct timeval *tp) noexcept {
    static const uint64_t EPOCH = uint64_t(116444736000000000ULL);
    SYSTEMTIME  system_time;
    FILETIME    file_time;

    GetSystemTime(&system_time);
    SystemTimeToFileTime(&system_time, &file_time);
    auto time =  uint64_t(file_time.dwLowDateTime);
    time += (static_cast<uint64_t>(file_time.dwHighDateTime)) << 32;

    tp->tv_sec  = long((time - EPOCH) / 10000000L);
    tp->tv_usec = long(system_time.wMilliseconds * 1000L);
}

inline auto is_tty(handle_t handle) noexcept {
    if(handle == INVALID_HANDLE_VALUE) return false;
    auto type = GetFileType(handle);
    return type == FILE_TYPE_CHAR;
}

inline auto load(const std::string& path) {
    return LoadLibrary(path.c_str());
}

inline auto find(HINSTANCE dso, const std::string& sym) -> addr_t {
    return GetProcAddress(dso, sym.c_str());
}

inline void unload(HINSTANCE dso) {
    FreeLibrary(dso);
}

inline auto spawn(const std::string& path, char *const *argv) {
    return _spawnvp(_P_WAIT, path.c_str(), argv);
}

inline auto spawn(const std::string& path, char *const *argv, char *const *env) {
    return _spawnvpe(_P_WAIT, path.c_str(), argv, env);
}

inline auto exec(const std::string& path, char *const *argv) {
    return _spawnvp(_P_OVERLAY, path.c_str(), argv);
}

inline auto exec(const std::string& path, char *const *argv, char *const *env) {
    return _spawnvpe(_P_OVERLAY, path.c_str(), argv, env);
}

inline auto async(const std::string& path, char *const *argv) -> id_t {
    return _spawnvp(_P_NOWAIT, path.c_str(), argv);
}

inline auto async(const std::string& path, char *const *argv, char *const *env) -> id_t {
    return _spawnvpe(_P_NOWAIT, path.c_str(), argv, env);
}

inline auto input() noexcept {
    return GetStdHandle(STD_INPUT_HANDLE);
}

inline auto output() noexcept {
    return GetStdHandle(STD_OUTPUT_HANDLE);
}

inline auto error() noexcept {
    return GetStdHandle(STD_ERROR_HANDLE);
}

inline auto input(const std::string& cmd) {
    return _popen(cmd.c_str(), "rt");
}

inline auto output(const std::string& cmd) {
    return _popen(cmd.c_str(), "wt");
}

inline auto detach(const std::string& path, char *const *argv) {
    return _spawnvp(_P_DETACH, path.c_str(), argv);
}

inline auto detach(const std::string& path, char *const *argv, char *const *env) {
    return _spawnvpe(_P_DETACH, path.c_str(), argv, env);
}

inline auto wait(id_t pid) noexcept {
    int status{-1};
    _cwait(&status, pid, _WAIT_CHILD);
    return status;
}

inline auto wait(std::FILE *fp) {
    return _pclose(fp);
}

inline auto stop(id_t pid) noexcept {
    return CloseHandle(reinterpret_cast<HANDLE>(pid)) == TRUE;
}

inline auto is_service() noexcept {
    auto isService = false;
    auto pid = GetCurrentProcessId();
    auto hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if(hSnapshot == INVALID_HANDLE_VALUE) return false;

    PROCESSENTRY32 pe;
    pe.dwSize = sizeof(PROCESSENTRY32);
    if(Process32First(hSnapshot, &pe)) {
        do {
            if(pe.th32ProcessID == pid) {
                DWORD parentPid = pe.th32ParentProcessID;
                if(Process32First(hSnapshot, &pe)) {
                    do {
                        if(pe.th32ProcessID == parentPid) {
                            isService = (std::string(pe.szExeFile) == "services.exe");
                            break;
                        }
                    } while(Process32Next(hSnapshot, &pe));
                }
                break;
            }
        } while(Process32Next(hSnapshot, &pe));
    }

    CloseHandle(hSnapshot);
    return isService;
}

inline auto id() noexcept -> id_t {
    return _getpid();
}

inline void env(const std::string& id, const std::string& value) {
    static_cast<void>(_putenv((id + "=" + value).c_str()));
}
#else
using id_t = pid_t;
using addr_t = void *;
using handle_t = int;

inline constexpr auto dso_suffix = ".so";

constexpr auto invalid_handle() -> handle_t {
    return -1;
}

class ref_t final {
public:
    ref_t() noexcept = default;

    // cppcheck-suppress noExplicitConstructor
    ref_t(handle_t handle) noexcept : handle_(handle) {}
    ref_t(const ref_t& other) noexcept : handle_(dup(other.handle_)) {}

    ref_t(ref_t&& other) noexcept : handle_(other.handle_) {
        other.handle_ = invalid_handle();
    }

    ~ref_t() {
        if(handle_ != invalid_handle())
            ::close(handle_);
    }

    auto operator=(const ref_t& other) noexcept -> ref_t& {
        if(&other == this) return *this;
        if(handle_ != invalid_handle())
            ::close(handle_);
        handle_ = dup(other.handle_);
        return *this;
    }

    auto operator=(ref_t&& other) noexcept -> ref_t& {
        if(handle_ != invalid_handle())
            ::close(handle_);
        handle_ = other.handle_;
        other.handle_ = invalid_handle();
        return *this;
    }

    auto operator=(handle_t handle) noexcept -> ref_t& {
        if(handle_ != invalid_handle())
            ::close(handle_);
        handle_ = handle;
        return *this;
    }

    operator bool() const noexcept {
        return handle_ != invalid_handle();
    }

    auto operator!() const noexcept {
        return handle_ == invalid_handle();
    }

    operator int() const noexcept {
        return handle_;
    }

    auto operator*() const noexcept {
        return handle_;
    }

    auto operator==(const ref_t& other) const noexcept {
        return handle_ == other.handle_;
    }

    auto operator!=(const ref_t& other) const noexcept {
        return handle_ == other.handle_;
    }

    void release() noexcept {
        if(handle_ != invalid_handle())
            ::close(handle_);
        handle_ = invalid_handle();
    }

private:
    handle_t handle_{invalid_handle()};
};

class map_t final {
public:
    map_t() = default;
    map_t(const map_t&) = delete;
    auto operator=(const map_t&) -> auto& = delete;

    explicit map_t(const std::string& path, int mode = 0640) {
        access(path, mode);
    }

    map_t(handle_t fd, std::size_t size, bool rw = true, bool priv = false, off_t offset = 0) noexcept :
    addr_(::mmap(nullptr, size, (rw) ? PROT_READ | PROT_WRITE : PROT_READ, (priv) ? MAP_PRIVATE : MAP_SHARED, fd, offset)), size_(size) {}

    map_t(map_t&& other) noexcept :
    addr_(other.addr_), size_(other.size_), path_(std::move(other.path_)) {
        other.addr_ = MAP_FAILED;
        other.size_ = 0;
        other.path_ = "";
    }

    ~map_t() {
        release();
    }

    operator bool() const noexcept {
        return addr_ != MAP_FAILED;
    };

    auto operator!() const noexcept {
        return addr_ == MAP_FAILED;
    }

    auto operator*() const {
        if(addr_ == MAP_FAILED) throw std::runtime_error("no mapped handle");
        return addr_;
    }

    auto operator=(map_t&& other) noexcept -> auto& {
        addr_ = other.addr_;
        size_ = other.size_;
        path_ = other.path_;
        other.addr_ = MAP_FAILED;
        other.size_ = 0;
        other.path_ = "";
        return *this;
    }

    auto operator[](std::size_t pos) -> uint8_t& {
        if(addr_ == MAP_FAILED) throw std::runtime_error("no mapped handle");
        if(pos >= size_) throw std::runtime_error("outside of map range");
        return (static_cast<uint8_t *>(addr_))[pos];
    }

    auto operator[](std::size_t pos) const -> const uint8_t& {
        if(addr_ == MAP_FAILED) throw std::runtime_error("no mapped handle");
        if(pos >= size_) throw std::runtime_error("outside of map range");
        return (static_cast<uint8_t *>(addr_))[pos];
    }

    auto size() const noexcept {
        return size_;
    }

    auto get_or(std::size_t pos, const void *or_else = nullptr) const -> const void * {
        if(addr_ == MAP_FAILED || pos >= size_) return or_else;
        return (static_cast<uint8_t *>(addr_)) + pos;
    }

    auto get_or(std::size_t pos, void *or_else = nullptr) -> void * {
        if(addr_ == MAP_FAILED || pos >= size_) return or_else;
        return (static_cast<uint8_t *>(addr_)) + pos;
    }

    auto data() const noexcept {
        return addr_;
    }

    auto sync(bool wait = false) noexcept {
        return (addr_ == MAP_FAILED) ? false : ::msync(addr_, size_, (wait)? MS_SYNC : MS_ASYNC) == 0;
    }

    auto lock() noexcept {
        return (addr_ == MAP_FAILED) ? false : ::mlock(addr_, size_) == 0;
    }

    auto unlock() noexcept {
        return (addr_ == MAP_FAILED) ? false : ::munlock(addr_, size_) == 0;
    }

    auto detach() {
        auto tmp = addr_;
        addr_ = MAP_FAILED;
        size_ = 0;
        return tmp;
    }

    auto set(handle_t fd, std::size_t size, bool rw = true, bool priv = false, off_t offset = 0) noexcept {
        if(addr_ != MAP_FAILED)
            munmap(addr_, size_);

        addr_ = ::mmap(nullptr, size, (rw) ? PROT_READ | PROT_WRITE : PROT_READ, (priv) ? MAP_PRIVATE : MAP_SHARED, fd, offset);
        size_ = size;
        return addr_;
    }

    auto create(const std::string& path, size_t size, int mode = 0640) noexcept {
        release();
        shm_unlink(path.c_str());
        auto shm = shm_open(path.c_str(), O_RDWR | O_CREAT, mode);
        if(shm < 0) return MAP_FAILED;
        path_ = path;
        if(::ftruncate(shm, off_t(size)) < 0) {
            ::close(shm);
            return MAP_FAILED;
        }
        auto addr = set(shm, size, true);
        ::close(shm);
        return addr;
    }

    auto access(const std::string& path, int mode = 0640) noexcept -> void * {
        release();
        auto shm = shm_open(path.c_str(), O_RDWR | O_CREAT, mode);
        if(shm < 0) return MAP_FAILED;
        struct stat ino{};
        if(::fstat(shm, &ino) < 0) {
            ::close(shm);
            return MAP_FAILED;
        }
        auto addr = set(shm, ino.st_size, false);
        ::close(shm);
        return addr;
    }

    void release() noexcept {
        if(addr_ != MAP_FAILED) {
            munmap(addr_, size_);
            addr_ = MAP_FAILED;
        }
        if(!path_.empty()) {
            shm_unlink(path_.c_str());
            path_ = "";
        }
        size_ = 0;
    }

private:
    void *addr_{MAP_FAILED};
    std::size_t size_{0};
    std::string path_;
};

inline auto page_size() noexcept -> off_t {
    return sysconf(_SC_PAGESIZE);
}

inline void time(struct timeval *tp) noexcept {
    gettimeofday(tp, nullptr);
}

inline auto is_tty(handle_t fd) noexcept {
    if(fd < 0) return false;
    if(isatty(fd)) return true;
    return false;
}

inline auto load(const std::string& path) {
    return dlopen(path.c_str(), RTLD_NOW | RTLD_GLOBAL);
}

inline auto find(void *dso, const std::string& sym) -> addr_t {
    return dlsym(dso, sym.c_str());
}

inline void unload(void *dso) {
    dlclose(dso);
}

inline auto input() noexcept {
    return 0;
}

inline auto output() noexcept {
    return 1;
}

inline auto error() noexcept {
    return 2;
}

inline auto input(const std::string& cmd) {
    return popen(cmd.c_str(), "r");
}

inline auto output(const std::string& cmd) {
    return popen(cmd.c_str(), "w");
}

inline auto wait(id_t pid) noexcept {
    int status{-1};
    waitpid(pid, &status, 0);
    return WEXITSTATUS(status);
}

inline auto wait(std::FILE *fp) {
    return pclose(fp);
}

inline auto stop(id_t pid) noexcept {
    return !kill(pid, SIGTERM);
}

inline auto spawn(const std::string& path, char *const *argv) {
    const id_t child = fork();
    if(!child) {
        execvp(path.c_str(), argv);
        ::_exit(-1);
    }
    return wait(child);
}

inline auto spawn(const std::string& path, char *const *argv, char *const *env) {
    const id_t child = fork();
    if(!child) {
        execvpe(path.c_str(), argv, env);
        ::_exit(-1);
    }
    return wait(child);
}

inline auto exec(const std::string& path, char *const *argv) {
    return execvp(path.c_str(), argv);
}

inline auto exec(const std::string& path, char *const *argv, char *const *env) {
    return execvpe(path.c_str(), argv, env);
}

inline auto async(const std::string& path, char *const *argv) -> id_t {
    const id_t child = fork();
    if(!child) {
        execvp(path.c_str(), argv);
        ::_exit(-1);
    }
    return child;
}

inline auto async(const std::string& path, char *const *argv, char *const *env) -> id_t {
    const id_t child = fork();
    if(!child) {
        execvpe(path.c_str(), argv, env);
        ::_exit(-1);
    }
    return child;
}

inline auto detach(const std::string& path, char *const *argv) -> id_t {
    const id_t child = fork();
    if(!child) {
#if defined(SIGTSTP) && defined(TIOCNOTTY)
        if(setpgid(0, getpid()) == -1)
            ::_exit(-1);

        auto fd = open("/dev/tty", O_RDWR);
        if(fd >= 0) {
            ::ioctl(fd, TIOCNOTTY, nullptr);
            ::close(fd);
        }
#else
#if defined(__linux__)
        if(setpgid(0, getpid()) == -1)
            ::_exit(-1);
#else
        if(setpgrp() == -1)
            ::_exit(-1);
#endif
        if(getppid() != 1) {
            auto pid = fork();
            if(pid < 0)
                ::_exit(-1);
            else if(pid > 0)
                ::_exit(0);
        }
#endif
        execvp(path.c_str(), argv);
        ::_exit(-1);
    }
    return child;
}

inline auto detach(const std::string& path, char *const *argv, char *const *env) -> id_t {
    const id_t child = fork();
    if(!child) {
#if defined(SIGTSTP) && defined(TIOCNOTTY)
        if(setpgid(0, getpid()) == -1)
            ::_exit(-1);

        auto fd = open("/dev/tty", O_RDWR);
        if(fd >= 0) {
            ::ioctl(fd, TIOCNOTTY, nullptr);
            ::close(fd);
        }
#else
#if defined(__linux__)
        if(setpgid(0, getpid()) == -1)
            ::_exit(-1);
#else
        if(setpgrp() == -1)
            ::_exit(-1);
#endif
        if(getppid() != 1) {
            auto pid = fork();
            if(pid < 0)
                ::_exit(-1);
            else if(pid > 0)
                ::_exit(0);
        }
#endif

        execvpe(path.c_str(), argv, env);
        ::_exit(-1);
    }
    return child;
}

inline auto is_service() noexcept {
    return getpid() == 1 || getppid() == 1 || getuid() == 0;
}

inline auto id() noexcept -> id_t {
    return getpid();
}

inline void env(const std::string& id, const std::string& value) {
    setenv(id.c_str(), value.c_str(), 1);
}
#endif

[[noreturn]] inline auto exit(int code) {
    quick_exit(code);
}

// cppcheck-suppress constParameterPointer
inline auto on_exit(void(*handler)()) {
    return at_quick_exit(handler) == 0;
}

inline auto env(const std::string& id, std::size_t max = 256) noexcept -> std::optional<std::string> {
    auto buf = std::make_unique<char []>(max);
    const char *out = getenv(id.c_str());
    if(!out) return {};
    buf[max - 1] = 0;
    strncpy(&buf[0], out, max);
    if(buf[max - 1] != 0) return {};
    return std::string(&buf[0]);
}

inline auto shell(const std::string& cmd) noexcept {
    return system(cmd.c_str());
}
} // end namespace

namespace tycho::this_thread {
using namespace std::this_thread;

#if defined(_MSC_VER) || defined(__MINGW32__) || defined(__MINGW64__) || defined(WIN32)
inline auto priority(int priority) {
    switch(priority) {
    case 0:
        priority = THREAD_PRIORITY_NORMAL;
        break;
    case -1:
    case -2:
        priority = THREAD_PRIORITY_BELOW_NORMAL;
        break;
    case 1:
    case 2:
        priority = THREAD_PRIORITY_ABOVE_NORMAL;
        break;
    case -3:
        priority = THREAD_PRIORITY_LOWEST;
        break;
    case 3:
        priority = THREAD_PRIORITY_HIGHEST;
        break;
    case 4:
        priority = THREAD_PRIORITY_TIME_CRITICAL;
        break;
    default:
        return false;
    }
    return SetThreadPriority(GetCurrentThread(), priority) != 0;
}
#else
inline auto priority(int priority) {
    auto tid = pthread_self();
    struct sched_param sp{};
    int policy{};

    std::memset(&sp, 0, sizeof(sp));
    policy = SCHED_OTHER;
#ifdef  SCHED_FIFO
    if(priority > 0) {
        policy = SCHED_FIFO;
        priority = sched_get_priority_min(policy) + priority - 1;
        if(priority > sched_get_priority_max(policy))
            priority = sched_get_priority_max(policy);
    }
#endif
#ifdef  SCHED_BATCH
    if(priority < 0) {
        policy = SCHED_BATCH;
        priority = 0;
    }
#endif
    if(policy == SCHED_OTHER)
        priority = 0;

    sp.sched_priority = priority;
    return pthread_setschedparam(tid, policy, &sp) == 0;
}
#endif
} // end namespace
#endif
