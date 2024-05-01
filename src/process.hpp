// Copyright (C) 2022 Tycho Softworks.
// This code is licensed under MIT license.

#ifndef PROCESS_HPP_
#define PROCESS_HPP_

#include <stdexcept>
#include <memory>
#include <cstdlib>
#include <cstring>

#if defined(_MSC_VER) || defined(__MINGW32__) || defined(__MINGW64__) || defined(WIN32)
#include <process.h>
#else
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#endif

namespace process {
#if defined(_MSC_VER) || defined(__MINGW32__) || defined(__MINGW64__) || defined(WIN32)
using id_t = intptr_t;

inline auto spawn(const std::string& path, const char **argv) {
    return _spawnvp(_P_WAIT, path.c_str(), argv);
}

inline auto spawn(const std::string& path, const char **argv, const char **env) {
    return _spawnvpe(_P_WAIT, path.c_str(), argv, env);
}

inline auto exec(const std::string& path, const char **argv) {
    return _spawnvp(_P_OVERLAY, path.c_str(), argv);
}

inline auto exec(const std::string& path, const char **argv, const char **env) {
    return _spawnvpe(_P_OVERLAY, path.c_str(), argv, env);
}

inline auto async(const std::string& path, const char **argv) -> id_t {
    return _spawnvp(_P_NOWAIT, path.c_str(), argv);
}

inline auto async(const std::string& path, const char **argv, const char **env) -> id_t {
    return _spawnvpe(_P_NOWAIT, path.c_str(), argv, env);
}

inline auto input(const std::string& cmd) {
    return _popen(cmd.c_str(), "rb");
}

inline auto output(const std::string& cmd) {
    return _popen(cmd.c_str(), "wb");
}

inline auto detach(const std::string& path, const char **argv) {
    return _spawnvp(_P_DETACH, path.c_str(), argv);
}

inline auto detach(const std::string& path, const char **argv, const char **env) {
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

inline auto id() -> id_t {
    return _getpid();
}

inline void env(const std::string& id, const std::string& value) {
    _putenv((id + "=" + value).c_str());
}
#else
using id_t = pid_t;

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
    return WEXITSTATUS(pclose(fp));
}

inline auto spawn(const std::string& path, char *const *argv) {
    const id_t child = fork();
    if(!child) {
        // FlawFinder: ignore
        execvp(path.c_str(), argv);
        ::exit(-1);
    }
    return wait(child);
}

inline auto spawn(const std::string& path, char *const *argv, char *const *env) {
    const id_t child = fork();
    if(!child) {
        // FlawFinder: ignore
        execvpe(path.c_str(), argv, env);
        ::exit(-1);
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
        ::exit(-1);
    }
    return child;
}

inline auto async(const std::string& path, char *const *argv, char *const *env) -> id_t {
    const id_t child = fork();
    if(!child) {
        // FlawFinder: ignore
        execvpe(path.c_str(), argv, env);
        ::exit(-1);
    }
    return child;
}

inline auto id() -> id_t {
    return getpid();
}

inline void env(const std::string& id, const std::string& value) {
    setenv(id.c_str(), value.c_str(), 1);
}
#endif

inline auto env(const std::string& id, size_t max = 256) {
    auto buf = std::make_unique<char []>(max);
    // FlawFinder: ignore
    const char *out = getenv(id.c_str());
    if(!out)
        throw std::invalid_argument("cannot get env");

    buf[max - 1] = 0;
    // FlawFinder: ignore
    strncpy(&buf[0], out, max);
    if(buf[max - 1] != 0)
        throw std::invalid_argument("env too large");

    return std::string(&buf[0]);
}

inline auto shell(const std::string& cmd) {
    // FlawFinder: ignore
    return system(cmd.c_str());
}
} // end namespace

#endif
