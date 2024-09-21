// Copyright (C) 2022 Tycho Softworks.
// This code is licensed under MIT license.

#if !defined(TYCHO_SERIAL_HPP_)
#define TYCHO_SERIAL_HPP_

#if __has_include(<termios.h>)
#include <thread>
#include <chrono>
#include <csignal>
#include <climits>

#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include <sys/ioctl.h>
#else
#if defined(_MSC_VER) || defined(__MINGW32__) || defined(__MINGW64__) || defined(WIN32)
#if _WIN32_WINNT < 0x0600 && !defined(_MSC_VER)
#undef  _WIN32_WINNT
#define _WIN32_WINNT    0x0600  // NOLINT
#endif
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <handleapi.h>
#include <process.h>
#endif
#endif

#include <string>
#include <string_view>
#include <cstring>
#include <cstdint>

namespace tycho {
using crc16_t = uint16_t;
using crc32_t = uint32_t;

class bad_serial final : public std::exception {
public:
    enum class error : int {
        success = 0,
        error = EIO,
        access = EACCES,
        perms = EPERM,
        busy = EAGAIN,
        badfile = EBADF,
        fault = EFAULT,
        interupted = EINTR,
        invalid = EINVAL,
        timeout = ETIMEDOUT,
        disconnect = EPIPE,
        nofile = EISDIR,
        nodev = ENODEV,
        nomem = ENOMEM,
        loop = ELOOP,
        toomany = EMFILE,
        exhausted = ENFILE,
        badpath = ENAMETOOLONG,
        nodir = ENOTDIR,
        blocked = EWOULDBLOCK,
    };

    auto operator=(const bad_serial&) -> auto& = delete;

    auto err() const {
        return err_;
    }

    auto what() const noexcept -> const char * override {
        return "serial i/o error";
    }

private:
    friend class serial_t;

    bad_serial() : err_(bad_serial::error(errno)) {}
    bad_serial(const bad_serial& other) = default;
    explicit bad_serial(bad_serial::error code) : err_(code) {}

    error err_{bad_serial::error::success};
};

using serial_error = bad_serial::error;

#if __has_include(<termios.h>)
class serial_t final {
public:
    serial_t() = default;

    explicit serial_t(const std::string& path) {
        open(path); // FlawFinder: safe
    }

    serial_t(const serial_t& from) :
    timed_(from.timed_), original_(from.original_), current_(from.current_), err_(from.err_) {
        if(from.device_ > -1)
            device_ = ::dup(from.device_);
    }

    serial_t(serial_t&& from) noexcept :
    device_(from.device_), timed_(from.timed_), original_(from.original_), current_(from.current_), err_(from.err_) {
        from.device_ = -1;
    }

    ~serial_t() {
        close();
    }

    auto operator!() const noexcept {
        return device_ < 0;
    }

    operator bool() const noexcept {
        return device_ > -1;
    }

    auto err() const noexcept {
        return err_;
    }

    void open(const std::string& fname) {   // FlawFinder: safe
        close();
        device_ = ::open(fname.c_str(), O_RDWR | O_NDELAY); // FlawFinder: safe
        if(device_ < 0) {
            err_ = bad_serial::error(errno);
            return;
        }

        if(!isatty(device_)) {
            ::close(device_);
            device_ = -1;
            return;
        }

        auto ioflags = fcntl(device_, F_GETFL);
        tcgetattr(device_, &current_);
        tcgetattr(device_, &original_);
        reset();
        fcntl(device_, F_SETFL, ioflags & ~O_NDELAY);

#if defined(TIOCM_RTS) && defined(TIOCMODG)
        auto mcs = 0;
        ioctl(device_, TIOCMODG, &mcs);
        mcs |= TIOCM_RTS;
        ioctl(device_, TIOCMODS, &mcs);
#endif
    }

    void close() {
        if(device_ > -1) {
            tcsetattr(device_, TCSANOW, &original_);
            ::close(device_);
        }
        device_ = -1;
        timed_ = 0;
        err_ = bad_serial::error::success;
    }

    auto get(bool echo = false, int echocode = EOF, int eol = EOF) const {
        if(device_ > -1) {
            char buf{0};
            err_ = bad_serial::error::success;
            auto result = ::read(device_, &buf, 1); // FlawFinder: ignore
            if(result < 0)
                throw bad_serial(err_ = bad_serial::error(-result));
            if(result < 1)
                return EOF;
            if(echo && echocode != EOF && (eol == EOF || buf != eol))
                put(echocode);
            else if(echo)
                put(buf);
            return static_cast<int>(buf);
        }
        return EOF;
    }

    auto get(void *data, size_t size, bool echo = false) const {
        if(!data || !size)
            return size_t(0U);

        err_ = bad_serial::error::success;
        auto count = ::read(device_, data, size);   // FlawFinder: safe
        if(count < 0)
            throw bad_serial(err_ = bad_serial::error(-count));
        if(count > 0 && echo)
            put(data, count);
        if(count > 0)
            return size_t(count);
        return size_t(0U);
    }

    auto put(int code) const -> bool {
        if(device_ < 0)
            return false;

        if(code == EOF)
            return true;

        char buf = static_cast<char>(code);
        err_ = bad_serial::error::success;
        auto result = ::write(device_, &buf, 1);
        if(result < 0)
            throw bad_serial(err_ = bad_serial::error(-result));
        if(result < 1)
            return false;
        return true;
    }

    auto put(const void *data, size_t size) const -> size_t {
        if(!size || device_ < 0)
            return size_t(0U);

        err_ = bad_serial::error::success;
        auto count = ::write(device_, data, size);
        if(count > 0)
            return size_t(count);
        if(count < 0)
            throw bad_serial(err_ = bad_serial::error(-count));
        return size_t(0U);
    }

    auto put(const std::string_view& msg) const {
        return put(msg.data(), msg.size());
    }

    auto is_packet() const noexcept {
        if(device_ < 0)
            return false;
        return 0 == (current_.c_lflag & ICANON);
    }

    auto timed() const noexcept {
        if(is_packet())
            return timed_;
        return uint8_t(0);
    }

    auto timed_mode(size_t size, uint8_t timer = 1) noexcept -> size_t {
        if(device_ < 0)
            return 0U;

#ifdef  _PC_MAX_INPUT
        auto max = fpathconf(device_, _PC_MAX_INPUT);
#else
        auto max = MAX_INPUT;
#endif

        if(size > (size_t)max)
            size = max;

        if(size > 255)
            size = 255;

        current_.c_cc[VEOL] = current_.c_cc[VEOL2] = 0;
        current_.c_cc[VMIN] = (uint8_t)size;
        current_.c_cc[VTIME] = timer;
        current_.c_lflag &= ~ICANON;
        tcsetattr(device_, TCSANOW, &current_);
        timed_ = timer;
        return size;
    }

    auto line_mode(const char *nl = "\r\n", uint8_t min = 1) noexcept {
        if(device_ < 0)
            return size_t(0U);

        if(!nl)
            nl = "";

        char nl1{0}, nl2{0};

        if(*nl) {
            nl1 = nl[0];
            nl2 = nl[1];
        }

        timed_ = 0;
        current_.c_cc[VMIN] = min;
        current_.c_cc[VTIME] = 0;
        current_.c_cc[VEOL] = nl1;
        current_.c_cc[VEOL2] = nl2;
        current_.c_lflag |= ICANON;
        tcsetattr(device_, TCSANOW, &current_);

#ifdef  _PC_MAX_INPUT
        return size_t(fpathconf(device_, _PC_MAX_INPUT));
#else
        return size_t(MAX_INPUT);
#endif
    }

    void flush() const {
        if(device_ > -1)
            tcflush(device_, TCOFLUSH);
    }

    void purge() const {
        if(device_ > -1)
            tcflush(device_, TCIFLUSH);
    }

    void sync() const {
        if(device_ > -1)
            tcdrain(device_);
    }

    void hup() const {
        if(device_ >  -1)
            tcsendbreak(device_, 0);
    }

    void dtr(unsigned msec = 120) {
        if(device_ < 0)
            return;

        struct termios temp{};
        tcgetattr(device_, &temp);
        cfsetospeed(&current_, 0);
        cfsetispeed(&current_, 0);
        tcsetattr(device_, TCSANOW, &current_);
        if(msec) {
            std::this_thread::sleep_for(std::chrono::milliseconds(msec));
            tcsetattr(device_, TCSANOW, &temp);
            current_ = temp;
        }
    }

    void flow(bool hw = true, bool sw = true) {
        if(device_ < 0)
            return;

        current_.c_cflag &= ~CRTSCTS;
        current_.c_iflag &= ~(IXON | IXANY | IXOFF);

        if(sw)
            current_.c_iflag |= (IXON | IXANY | IXOFF);

        if(hw)
            current_.c_cflag |= CRTSCTS;

        tcsetattr(device_, TCSANOW, &current_);
    }

    auto format(const std::string_view& s) {
        char bits = '8';
        char stop = '1';
        char parity = 'n';
        unsigned pos = 0;

        if(s.size() > pos && strchr("5678", s[pos]))
            bits = s[pos++];

        if(s.size() > pos && strchr("oOeEnN", s[pos]))
            parity = s[pos++];

        if(s.size() > pos && strchr("12", s[pos]))
            stop = s[pos++];

        if(s.size() > pos)
            return false;

        if(device_ < 0)
            return false;

        current_.c_cflag &= ~(PARENB | PARODD | CSTOPB | CSIZE);

        switch(bits) {
        case '5':
            current_.c_cflag |= CS5;
            break;
        case '6':
            current_.c_cflag |= CS6;
            break;
        case '7':
            current_.c_cflag |= CS7;
            break;
        case '8':
            current_.c_cflag |= CS8;
            break;
        default:
            return false;
        }

        switch(stop) {
        case '1':
            break;
        case '2':
            current_.c_cflag |= CSTOPB;
            break;
        default:
            return false;
        }

        switch(parity) {
        case 'o':
        case 'O':
            current_.c_cflag |= (PARENB | PARODD);
            break;
        case 'e':
        case 'E':
            current_.c_cflag |= PARENB;
            break;
        case 'n':
        case 'N':
            break;
        default:
            return false;
        }

        tcsetattr(device_, TCSANOW, &current_);
        return true;
    }

    auto speed(unsigned long bytes) {
        unsigned long rate = 0;
        switch(bytes) {
#ifdef B256000
        case 256000:
            rate = B256000;
            break;
#endif
#ifdef B230400
        case 230400:
            rate = B230400;
            break;
#endif
#ifdef B128000
        case 128000:
            rate = B128000;
            break;
#endif
#ifdef B115200
        case 115200:
            rate = B115200;
            break;
#endif
#ifdef B57600
        case 57600:
            rate = B57600;
            break;
#endif
#ifdef B56000
        case 56000:
            rate = B56000;
            break;
#endif
#ifdef B38400
        case 38400:
            rate = B38400;
            break;
#endif
        case 19200:
            rate = B19200;
            break;
        case 9600:
            rate = B9600;
            break;
        case 4800:
            rate = B4800;
            break;
        case 2400:
            rate = B2400;
            break;
        case 1200:
            rate = B1200;
            break;
        case 600:
            rate = B600;
            break;
        case 300:
            rate = B300;
            break;
        case 110:
            rate = B110;
            break;
        default:
            return false;
        }

        if(device_ < 0)
            return false;

        cfsetispeed(&current_, rate);
        cfsetospeed(&current_, rate);
        tcsetattr(device_, TCSANOW, &current_);
        return true;
    }

private:
    int device_{-1};
    uint8_t timed_{0};
    struct termios original_{}, current_{};
    mutable bad_serial::error err_{bad_serial::error::success};

    void reset() {
        if(device_ < 0)
            return;

        current_.c_oflag = current_.c_lflag = 0;
        current_.c_cflag = CLOCAL | CREAD | HUPCL;
        current_.c_iflag = IGNBRK;

        memset(&current_.c_cc, 0, sizeof(current_.c_cc));
        current_.c_cc[VMIN] = 1;

        current_.c_cflag |= original_.c_cflag & (CRTSCTS | CSIZE | PARENB | PARODD | CSTOPB);
        current_.c_iflag |= original_.c_iflag & (IXON | IXANY | IXOFF);

        tcsetattr(device_, TCSANOW, &current_);
        timed_ = 0;
    }
};
#elif defined(_MSC_VER) || defined(__MINGW32__) || defined(__MINGW64__) || defined(WIN32)
class serial_t final {
public:
    serial_t() = default;

    explicit serial_t(const std::string& path) {
        open(path); // FlawFinder: ignore
    }

    serial_t(const serial_t& from) :
    timeouts_(from.timeouts_), saved_(from.saved_), active_(from.active_), err_(from.err_) {
        if(from.device_ != invalid_) {
            auto pid = GetCurrentProcess();
            HANDLE handle{INVALID_HANDLE_VALUE};
            DuplicateHandle(pid, from.device_, pid, &handle, 0, FALSE, DUPLICATE_SAME_ACCESS);
            device_ = static_cast<HINSTANCE>(handle);
        }
    }

    serial_t(serial_t&& from) noexcept :
    device_(from.device_), timed_(from.timed_), saved_(from.saved_), active_(from.active_), err_(from.err_) {
        from.device_ = invalid_;
    }

    ~serial_t() {
        close();
    }

    auto operator!() const {
        return device_ == invalid_;
    }

    operator bool() const {
        return device_ != invalid_;
    }

    auto err() const {
        return err_;
    }

    void open(const std::string& fname) { // FlawFinder: safe
        close();
        device_ = static_cast<HINSTANCE>(CreateFile(fname.c_str(),
            GENERIC_READ | GENERIC_WRITE,
            0,                      // exclusive access
            nullptr,                // no security attrs
            OPEN_EXISTING,
            FILE_FLAG_OVERLAPPED | FILE_ATTRIBUTE_NORMAL | FILE_FLAG_WRITE_THROUGH | FILE_FLAG_NO_BUFFERING,
            nullptr));

        if(device_ != invalid_) {
            if(GetCommTimeouts(device_, &timeouts_) == FALSE) {
                CloseHandle(device_);
                device_ = invalid_;
                return;
            }
            saved_.DCBlength = active_.DCBlength = sizeof(DCB);
            GetCommState(device_, &saved_);
            GetCommState(device_, &active_);
        }
        err_ = bad_serial::error(errno);
    }

    void close() {
        if(device_ != invalid_) {
            SetCommTimeouts(device_, &timeouts_);
            SetCommState(device_, &saved_);
            CloseHandle(device_);
            device_ = invalid_;
        }
        err_ = bad_serial::error::success;
    }

    auto get(bool echo = false, int echocode = EOF, int eol = EOF) const {
        if(device_ != invalid_) {
            char buf{0};
            DWORD count{0};
            err_ = bad_serial::error::success;
            if(ReadFile(device_, &buf, DWORD(1), &count, nullptr) == FALSE) {
                err_ = bad_serial::error(errno);
                throw bad_serial(err_);
            }
            if(count < 1)
                return EOF;
            if(echo && echocode != EOF && (eol == EOF || buf != eol))
                put(echocode);
            else if(echo)
                put(buf);
            return static_cast<int>(buf);
        }
        return EOF;
    }

    auto get(void *data, size_t size, bool echo = false) const {
        if(!data || !size)
             return size_t(0U);
        DWORD count{0};
        err_ = bad_serial::error::success;
        if(ReadFile(device_, data, DWORD(size), &count, nullptr) == FALSE) {
            err_ = bad_serial::error(errno);
            throw bad_serial(err_);
        }
        if(count > 0 && echo)
            put(data, count);
        if(count > 0)
            return size_t(count);
        return size_t(0U);
    }

    auto put(const void *data, size_t size) const -> size_t {
        if(!size || device_ == invalid_)
            return size_t(0U);

        DWORD count{0};
        err_ = bad_serial::error::success;
        if(WriteFile(device_, data, DWORD(size), &count, nullptr) == FALSE) {
            err_ = bad_serial::error(errno);
            throw bad_serial(err_);
        }
        if(count > 0)
            return size_t(count);
        return size_t(0U);
    }

    auto put(int code) const -> bool {
        if(device_ == invalid_)
            return false;

        if(code == EOF)
            return true;

        char buf = static_cast<char>(code);
        DWORD count{0};
        err_ = bad_serial::error::success;
        if(WriteFile(device_, &buf, 1, &count, nullptr) == FALSE) {
            err_ = bad_serial::error(errno);
            throw bad_serial(err_);
        }
        if(count < 1)
            return false;
        return true;
    }

    auto put(const std::string_view& msg) const {
        return put(msg.data(), msg.size());
    }

    auto timed_mode(size_t size, uint8_t timer = 1) noexcept -> size_t {
        if(device_ == invalid_)
            return size_t(0);

        COMMTIMEOUTS timed{0};
        timed.WriteTotalTimeoutMultiplier = 0;
        timed.WriteTotalTimeoutConstant = 0;
        timed.ReadTotalTimeoutMultiplier = 0;
        if(timer > 0 && size > 0) {
            timed.ReadIntervalTimeout = MAXDWORD;
            timed.ReadTotalTimeoutConstant = timer * 100;
        }
        else if(size > 0 && !timer) {
            timed.ReadIntervalTimeout = 0;
            timed.ReadTotalTimeoutConstant = 0;
        }
        else if(!size && timer > 0) {
            timed.ReadIntervalTimeout = timer * 100;
            timed.ReadTotalTimeoutConstant = 0;
        } else {
            timed.ReadIntervalTimeout = MAXDWORD;
            timed.ReadTotalTimeoutConstant = 0;
        }
        SetCommTimeouts(device_, &timed);
        return size;
    }

    auto line_mode(const char *nl = "\r\n", uint8_t min = 1) noexcept { // NOLINT
        return size_t(0U);
    }

    void flush() {
        if(device_ != invalid_)
            PurgeComm(device_, PURGE_TXABORT | PURGE_TXCLEAR);
    }

    void purge() {
        if(device_ != invalid_)
            PurgeComm(device_, PURGE_RXABORT | PURGE_RXCLEAR);
    }

    void sync() {
        if(device_ != invalid_)
            FlushFileBuffers(device_);
    }

    void hup() {
        if(device_ == invalid_)
            return;

        SetCommBreak(device_);
        Sleep(100L);
        ClearCommBreak(device_);
    }

    void dtr(unsigned msec = 120) {
        if(device_ == invalid_)
            return;

        EscapeCommFunction(device_, CLRDTR);
        if(msec) {
            Sleep(msec);
            EscapeCommFunction(device_, SETDTR);
        }
    }

    void flow(bool hw = true, bool sw = true) {
        if(device_ == invalid_)
            return;

        active_.XonChar = 0x11;
        active_.XoffChar = 0x13;
        active_.XonLim = 100;
        active_.XoffLim = 100;
        if(sw)
            active_.fInX = active_.fOutX = 1;
        else
            active_.fInX = active_.fOutX = 0;

        if(hw) {
            active_.fOutxCtsFlow = 1;
            active_.fRtsControl = RTS_CONTROL_HANDSHAKE;
        }
        else {
            active_.fOutxCtsFlow = 0;
            active_.fRtsControl = 0;
        }
        SetCommState(device_, &active_);
    }

    auto format(const std::string_view& s) {
        if(device_ == invalid_)
            return false;

        unsigned pos = 0;
        unsigned bits = 8;
        unsigned stop = 1;
        char parity = 'n';

        if(s.size() > pos && strchr("5678", s[pos]))
            bits = s[pos++] - '0';

        if(s.size() > pos && strchr("oOeEnNmMsS", s[pos]))
            parity = s[pos++];

        if(s.size() > pos && strchr("12", s[pos]))
            stop = s[pos] - '0';

        if(s.size() > pos)
            return false;

        switch(parity) {
        case 'o':
        case 'O':
            active_.Parity = ODDPARITY;
            break;
        case 'e':
        case 'E':
            active_.Parity = EVENPARITY;
            break;
        case 'n':
        case 'N':
            active_.Parity = NOPARITY;
            break;
        case 'm':
        case 'M':
            active_.Parity = MARKPARITY;
            break;
        case 's':
        case 'S':
            active_.Parity = SPACEPARITY;
            break;
        default:
            return false;
        }

        if(stop == 1)
            active_.StopBits = ONESTOPBIT;
        else
            active_.StopBits = TWOSTOPBITS;
        active_.ByteSize = bits;
        active_.fBinary = 1;
        return SetCommState(device_, &active_) == TRUE;
    }

    auto speed(unsigned long rate) {
        if(device_ == invalid_)
            return 0UL;
        active_.BaudRate = rate;
        if(SetCommState(device_, &active_) == FALSE)
            GetCommState(device_, &active_);
        return active_.BaudRate;
    }

private:
    static inline const auto invalid_ = static_cast<HINSTANCE>(INVALID_HANDLE_VALUE);

    COMMTIMEOUTS timeouts_{0};
    DCB saved_{0}, active_{0};
    uint8_t timed_{0};
    HINSTANCE device_{invalid_};
    mutable bad_serial::error err_{bad_serial::error::success};
};
#endif

template<typename T>
inline auto send(const serial_t& sio, const T& msg) {
    static_assert(std::is_trivial_v<T>, "T must be Trivial type");

    return sio.put(&msg, sizeof(msg));
}

template<typename T>
inline auto recv(const serial_t& sio, T& msg) {
    static_assert(std::is_trivial_v<T>, "T must be Trivial type");

    return sio.get(&msg, sizeof(msg));
}

inline auto getline(const serial_t& sio, char *buf, size_t max, int eol = '\n', bool echo = false, int echocode = EOF, const char *ignore = nullptr) {
    *buf = 0;
    --max;

    auto count = size_t(0);
    while(count < max) {
        auto code = sio.get(echo, echocode, eol);
        if(code == EOF)
            return count;
        if(ignore && strchr(ignore, code))
            continue;
        buf[count++] = static_cast<char>(code);
        if(eol && code == eol) {
            break;
        }
    }
    buf[count] = 0;
    return count;
}

inline auto putline(const serial_t& sio, std::string_view msg, const std::string_view& eol = "\n") {
    auto result = sio.put(msg);
    if(result > 0 && sio.put(eol) == eol.size())
        return result;
    return size_t(0U);
}

inline auto expect(const serial_t& sio, const std::string_view& match) {
    auto count = 0U;
    while(count < match.size()) {
        auto code = sio.get();
        if(code == EOF)
            return false;
        // strip lead-in noise...
        if(!count && match[0] != code)
            continue;
        if(match[count] != code)
            return false;
        ++count;
    }
    return true;
}

inline auto until(const serial_t& sio, int match = EOF, unsigned max = 1) {
    auto count = 0U;
    while(count < max) {
        auto code = sio.get();
        if(code == EOF)
            return false;
        if(match == EOF || code == match)
            ++count;
    };
    return true;
}

inline auto csum8(const uint8_t *data, size_t size) {
    uint8_t sum = 0;
    while (size--)
        sum += *data++;
    return sum;
}

inline auto crc16(const uint8_t *data, size_t size) {
    crc16_t crc = 0x0000;
    while(size--) {
        crc ^= *data++ << 8;
        for(uint8_t i = 0; i < 8; i++) {
            if(crc & 0x8000)
                crc = (crc << 1) ^ 0x8005;
            else
                crc <<= 1;
        }
    }
    return crc;
}

inline auto crc32(const uint8_t *data, size_t size) {
    static crc32_t table[256]{0};
    crc32_t crc = 0xffffffff;

    for (size_t i = 0; i < 256; i++) {
        crc32_t c = i;
        for (size_t j = 0; j < 8; j++)
            c = (c & 1) ? 0xedb88320 ^ (c >> 1) : c >> 1;
        table[i] = c;
    }

    for(size_t i = 0; i < size; i++)
        crc = table[(crc ^ data[i]) & 0xff] ^ (crc >> 8);

    return crc ^ 0xffffffff;
}

#ifdef  TYCHO_PRINT_HPP_
template<class... Args>
void print(serial_t& sio, std::string_view fmt, const Args&... args) {
    sio.put(fmt::format(fmt, args...));
}

template<class... Args>
void println(serial_t& sio, std::string_view fmt, const Args&... args) {
    putline(sio, fmt::format(fmt, args...));
}
#endif
} // end namespace
#endif
