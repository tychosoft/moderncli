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

#if defined(_MSC_VER) || defined(__MINGW32__) || defined(__MINGW64__) || defined(WIN32)
#if _WIN32_WINNT < 0x0600 && !defined(_MSC_VER)
#undef  _WIN32_WINNT
#define _WIN32_WINNT    0x0600
#endif
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <process.h>
#include <handleapi.h>
#ifndef quick_exit
#define quick_exit(x) ::exit(x)
#define at_quick_exit(x) ::atexit(x)
#endif
#else
#include <csignal>
#include <unistd.h>
#include <dlfcn.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/ioctl.h>

#ifndef RTLD_GLOBAL
#define RTLD_GLOBAL 0
#endif
#endif

#ifdef __FreeBSD__
#define execvpe(p,a,e)  exect(p,a,e)
#endif

namespace process {
#if defined(_MSC_VER) || defined(__MINGW32__) || defined(__MINGW64__) || defined(WIN32)
using id_t = intptr_t;
using addr_t = FARPROC;

class dso_t final {
public:
    dso_t() = default;

    explicit dso_t(const std::string& path) :
    ptr_(LoadLibrary(path.c_str())) {}  // FlawFinder: ignore

    dso_t(const dso_t&) = delete;
    auto operator=(const dso_t&) -> auto& = delete;

    ~dso_t() {
        release();
    }

    auto operator[](const char *symbol) const {
        return find(symbol);
    }

    auto operator()(const char *symbol) const {
        return find(symbol);
    }

    operator bool() const {
        return ptr_ != nullptr;
    }

    auto operator!() const {
        return ptr_ == nullptr;
    }

    auto load(const std::string& path) {
        release();
        ptr_ = LoadLibrary(path.c_str());   // FlawFinder: ignore
        return ptr_ != nullptr;
    }

    void release() {
        if(ptr_) {
            FreeLibrary(ptr_);
            ptr_ = nullptr;
        }
    }

    auto find(const std::string& sym) const -> addr_t {
        return ptr_ ? GetProcAddress(ptr_, sym.c_str()) : nullptr;
    }

private:
    HINSTANCE   ptr_{nullptr};
};

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
    return _popen(cmd.c_str(), "rb");
}

inline auto output(const std::string& cmd) {
    return _popen(cmd.c_str(), "wb");
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

class dso_t final {
public:
    dso_t() = default;

    explicit dso_t(const std::string& path) :
    ptr_(dlopen(path.c_str(), RTLD_NOW | RTLD_GLOBAL)) {}

    dso_t(const dso_t&) = delete;
    auto operator=(const dso_t&) -> auto& = delete;

    ~dso_t() {
        release();
    }

    auto operator[](const char *symbol) const {
        return find(symbol);
    }

    auto operator()(const char *symbol) const {
        return find(symbol);
    }

    operator bool() const {
        return ptr_ != nullptr;
    }

    auto operator!() const {
        return ptr_ == nullptr;
    }

    auto load(const std::string& path) {
        release();
        ptr_ = dlopen(path.c_str(), RTLD_NOW | RTLD_GLOBAL);
        return ptr_ != nullptr;
    }

    void release() {
        if(ptr_) {
            dlclose(ptr_);
            ptr_ = nullptr;
        }
    }

    auto find(const std::string& sym) const -> addr_t {
        return ptr_ ? dlsym(ptr_, sym.c_str()) : nullptr;
    }

private:
    void *ptr_{nullptr};
};


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
