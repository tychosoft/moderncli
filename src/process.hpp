// Copyright (C) 2022 Tycho Softworks.
// This code is licensed under MIT license.

#ifndef PROCESS_HPP_
#define PROCESS_HPP_

#include <stdexcept>
#include <memory>
#include <string>
#include <cstdlib>
#include <cstring>
#include <optional>
#include <utility>

#if defined(_MSC_VER) || defined(__MINGW32__) || defined(__MINGW64__) || defined(WIN32)
#if _WIN32_WINNT < 0x0600 && !defined(_MSC_VER)
#undef  _WIN32_WINNT
#define _WIN32_WINNT    0x0600
#endif
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <process.h>
#include <handleapi.h>
#include <io.h>
#ifndef quick_exit
#define quick_exit(x) ::exit(x)
#define at_quick_exit(x) ::atexit(x)
#endif
#define MAP_FAILED  nullptr
#else
#include <csignal>
#include <unistd.h>
#include <dlfcn.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/ioctl.h>
#include <sys/mman.h>

#ifndef RTLD_GLOBAL
#define RTLD_GLOBAL 0
#endif
#endif

#ifdef __FreeBSD__
#define execvpe(p,a,e)  exect(p,a,e)
#endif

namespace process {
using map_t = std::pair<void *, size_t>;
#if defined(_MSC_VER) || defined(__MINGW32__) || defined(__MINGW64__) || defined(WIN32)
using id_t = intptr_t;
using addr_t = FARPROC;
using handle_t = HANDLE;
constexpr auto dso_suffix = ".dll";

inline auto map(handle_t h, size_t size, bool rw = true, bool priv = false, off_t offset = 0) -> map_t {
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4293)
#endif
    const DWORD dwFileOffsetLow = (sizeof(off_t) <= sizeof(DWORD)) ?
                    (DWORD)offset : (DWORD)(offset & 0xFFFFFFFFL);
    const DWORD dwFileOffsetHigh = (sizeof(off_t) <= sizeof(DWORD)) ?
                    (DWORD)0 : (DWORD)((offset >> 32) & 0xFFFFFFFFL);

    const DWORD protectAccess = (rw) ? PAGE_READWRITE : PAGE_READONLY;
    const DWORD desiredAccess = (rw) ? FILE_MAP_READ | FILE_MAP_WRITE : FILE_MAP_READ;

    const auto max = offset + static_cast<off_t>(size);
    const DWORD dwMaxSizeLow = (sizeof(off_t) <= sizeof(DWORD)) ? (DWORD)max : (DWORD)(max & 0xFFFFFFFFL);
    const DWORD dwMaxSizeHigh = (sizeof(off_t) <= sizeof(DWORD)) ?
                    (DWORD)0 : (DWORD)((max >> 32) & 0xFFFFFFFFL);
#ifdef _MSC_VER
#pragma warning(pop)
#endif
    auto fm = CreateFileMapping(h, nullptr, protectAccess, dwMaxSizeHigh, dwMaxSizeLow, nullptr);

    if(fm == nullptr)
        return {MAP_FAILED, size};

    auto mv = MapViewOfFile(fm, desiredAccess, dwFileOffsetHigh, dwFileOffsetLow, size);
    CloseHandle(fm);

    if(mv == nullptr)
        return {MAP_FAILED, size};

    return {mv, size};
}

inline auto map(const std::string& path, size_t size, bool rw = true, bool priv = false, off_t offset = 0) -> map_t {
    auto fh = CreateFile(path.c_str(), (rw) ? GENERIC_READ | GENERIC_WRITE : GENERIC_READ, (rw) ? 0 : FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if(fh == INVALID_HANDLE_VALUE)
        return {nullptr, 0};

    auto ref = map(fh, size, rw, priv, offset);
    CloseHandle(fh);
    return ref;
}

inline auto sync(const map_t& ref, [[maybe_unused]]bool wait = false) {
    if(FlushViewOfFile(ref.first, ref.second))
        return true;

    return false;
}

inline auto lock(const map_t& ref) {
    if(VirtualLock((LPVOID)ref.first, ref.second))
        return true;

    return false;
}

inline auto unlock(const map_t& ref) {
    if(VirtualUnlock((LPVOID)ref.first, ref.second))
        return true;

    return false;
}

inline auto unmap(const map_t& ref) {
    if(UnmapViewOfFile(ref.first))
        return true;

    return false;
}

inline auto is_tty(handle_t handle) {
    if(handle == INVALID_HANDLE_VALUE)
        return false;
    auto type = GetFileType(handle);
    if(type == FILE_TYPE_CHAR)
        return true;
    return false;
}

inline auto load(const std::string& path) {
    return LoadLibrary(path.c_str());   // FlawFinder: ignore
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

inline auto wait(id_t pid) {
    int status{-1};
    _cwait(&status, pid, _WAIT_CHILD);
    return status;
}

inline auto wait(std::FILE *fp) {
    return _pclose(fp);
}

inline auto stop(id_t pid) {
    return CloseHandle((HANDLE)pid) == TRUE;
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
constexpr auto dso_suffix = ".so";

inline auto map(handle_t fd, size_t size, bool rw = true, bool priv = false, off_t offset = 0) -> map_t {
    return {::mmap(nullptr, size, (rw) ? PROT_READ | PROT_WRITE : PROT_READ, (priv) ? MAP_PRIVATE : MAP_SHARED, fd, offset), size};
}

inline auto map(const std::string& path, size_t size, bool rw = true, bool priv = false, off_t offset = 0) -> map_t {
    auto fd = ::open(path.c_str(), rw ? O_RDWR : O_RDONLY, 0);
    if(fd < 0)
        return {MAP_FAILED, size_t(0)};

    auto mapped = map(fd, size, rw, priv, offset);
    ::close(fd);
    return mapped;
}

inline auto sync(const map_t& ref, bool wait = false) {
    if(!::msync(ref.first, ref.second, (wait)? MS_SYNC : MS_ASYNC))
        return true;

    return false;
}

inline auto lock(const map_t& ref) {
    if(!::mlock(ref.first, ref.second))
        return true;

    return false;
}

inline auto unlock(const map_t& ref) {
    if(!::munlock(ref.first, ref.second))
        return true;

    return false;
}

inline auto unmap(const map_t& ref) {
    ::munmap(ref.first, ref.second);
}

inline auto is_tty(handle_t fd) {
    if(fd < 0)
        return false;

    if(isatty(fd))
        return true;

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

inline auto input(const std::string& cmd) {
    // FlawFinder: ignore
    return popen(cmd.c_str(), "r");
}

inline auto output(const std::string& cmd) {
    // FlawFinder: ignore
    return popen(cmd.c_str(), "w");
}

inline auto wait(id_t pid) {
    int status{-1};
    waitpid(pid, &status, 0);
    return WEXITSTATUS(status);
}

inline auto wait(std::FILE *fp) {
    return pclose(fp);
}

inline auto stop(id_t pid) {
    return !kill(pid, SIGTERM);
}

inline auto spawn(const std::string& path, char *const *argv) {
    const id_t child = fork();
    if(!child) {
        // FlawFinder: ignore
        execvp(path.c_str(), argv);
        ::_exit(-1);
    }
    return wait(child);
}

inline auto spawn(const std::string& path, char *const *argv, char *const *env) {
    const id_t child = fork();
    if(!child) {
        // FlawFinder: ignore
        execvpe(path.c_str(), argv, env);
        ::_exit(-1);
    }
    return wait(child);
}

inline auto exec(const std::string& path, char *const *argv) {
    // FlawFinder: ignore
    return execvp(path.c_str(), argv);
}

inline auto exec(const std::string& path, char *const *argv, char *const *env) {
    // FlawFinder: ignore
    return execvpe(path.c_str(), argv, env);
}

inline auto async(const std::string& path, char *const *argv) -> id_t {
    const id_t child = fork();
    if(!child) {
        // FlawFinder: ignore
        execvp(path.c_str(), argv);
        ::_exit(-1);
    }
    return child;
}

inline auto async(const std::string& path, char *const *argv, char *const *env) -> id_t {
    const id_t child = fork();
    if(!child) {
        // FlawFinder: ignore
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

        // FlawFinder: ignore
        auto fd = open("/dev/tty", O_RDWR);
        if(fd >= 0) {
            ::ioctl(fd, TIOCNOTTY, NULL);
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
        // FlawFinder: ignore
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

        // FlawFinder: ignore
        auto fd = open("/dev/tty", O_RDWR);
        if(fd >= 0) {
            ::ioctl(fd, TIOCNOTTY, NULL);
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

        // FlawFinder: ignore
        execvpe(path.c_str(), argv, env);
        ::_exit(-1);
    }
    return child;
}

inline auto id() noexcept -> id_t {
    return getpid();
}

inline void env(const std::string& id, const std::string& value) {
    setenv(id.c_str(), value.c_str(), 1);
}
#endif

inline auto map(const map_t& ref) {
    return ref.first;
}

inline auto size(const map_t& ref) {
    return ref.second;
}

[[noreturn]] inline auto exit(int code) {
    quick_exit(code);
}

// cppcheck-suppress constParameterPointer
inline auto on_exit(void(*handler)()) {
    return at_quick_exit(handler) == 0;
}

inline auto env(const std::string& id, size_t max = 256) noexcept -> std::optional<std::string> {
    auto buf = std::make_unique<char []>(max);
    // FlawFinder: ignore
    const char *out = getenv(id.c_str());
    if(!out)
        return {};

    buf[max - 1] = 0;
    // FlawFinder: ignore
    strncpy(&buf[0], out, max);
    if(buf[max - 1] != 0)
        return {};

    return std::string(&buf[0]);
}

inline auto shell(const std::string& cmd) noexcept {
    // FlawFinder: ignore
    return system(cmd.c_str());
}
} // end namespace

#endif
