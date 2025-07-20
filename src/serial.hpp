// Copyright (C) 2022 Tycho Softworks.
// This code is licensed under MIT license.

#if !defined(TYCHO_SERIAL_HPP_)
#define TYCHO_SERIAL_HPP_

#if __has_include(<termios.h>)
#include <thread>
#include <chrono>
#include <utility>
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

#include <system_error>
#include <string>
#include <string_view>
#include <cstring>
#include <cstdint>
#include <cerrno>

namespace tycho {
using crc16_t = uint16_t;
using crc32_t = uint32_t;

#if __has_include(<termios.h>)
class serial_t final {
public:
    serial_t() = default;

    explicit serial_t(const std::string& path) {
        open(path);
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

    explicit operator bool() const noexcept {
        return device_ > -1;
    }

    auto operator!() const noexcept {
        return device_ < 0;
    }

    auto operator=(serial_t&& from) noexcept -> auto& {
        if(this != &from) {
            close();
            device_ = from.device_;
            timed_ = from.timed_;
            original_ = from.original_;
            current_ = from.current_;
            err_ = from.err_;
            from.device_ = -1;
            from.err_ = 0;
        }
        return *this;
    }

    auto err() const noexcept {
        return err_;
    }

    void open(const std::string& fname) {
        close();
        device_ = ::open(fname.c_str(), O_RDWR | O_NDELAY);
        if(device_ < 0) {
            err_ = errno;
            return;
        }

        if(!isatty(device_)) {
            ::close(device_);
            device_ = -1;
            return;
        }

        auto io_flags = fcntl(device_, F_GETFL);
        tcgetattr(device_, &current_);
        tcgetattr(device_, &original_);
        reset();
        fcntl(device_, F_SETFL, io_flags & ~O_NDELAY);

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
        err_ = 0;
    }

    auto get(bool echo = false, int echo_code = EOF, int eol = EOF) const {
        if(device_ > -1) {
            char buf{0};
            err_ = 0;
            auto result = ::read(device_, &buf, 1);
            if(result < 0) throw std::system_error(err_ = int(-result), std::generic_category(), "Serial i/o error");
            if(result < 1) return EOF;
            if(echo && echo_code != EOF && (eol == EOF || buf != eol))
                put(echo_code);
            else if(echo)
                put(buf);
            return static_cast<int>(buf);
        }
        return EOF;
    }

    auto get(void *data, std::size_t size, bool echo = false) const {
        if(!data || !size) return std::size_t(0U);
        err_ = 0;
        auto count = ::read(device_, data, size);
        if(count < 0) throw std::system_error(err_ = int(-count), std::generic_category(), "Serial i/o error");
        if(count > 0 && echo)
            put(data, count);
        if(count > 0) return std::size_t(count);
        return std::size_t(0U);
    }

    auto put(int code) const -> bool {
        if(device_ < 0) return false;
        if(code == EOF) return true;

        char buf = static_cast<char>(code);
        err_ = 0;
        auto result = ::write(device_, &buf, 1);
        if(result < 0) throw std::system_error(err_ = int(-result), std::generic_category(), "Serial i/o error");
        if(result < 1) return false;
        return true;
    }

    auto put(const void *data, std::size_t size) const -> std::size_t {
        if(!size || device_ < 0) return std::size_t(0U);
        err_ = 0;
        auto count = ::write(device_, data, size);
        if(count > 0) return std::size_t(count);
        if(count < 0) throw std::system_error(err_ = int(-count), std::generic_category(), "Serial i/o error");
        return std::size_t(0U);
    }

    auto put(const std::string_view& msg) const {
        return put(msg.data(), msg.size());
    }

    auto is_packet() const noexcept {
        if(device_ < 0) return false;
        return 0 == (current_.c_lflag & ICANON);
    }

    auto timed() const noexcept {
        if(is_packet()) return timed_;
        return uint8_t(0);
    }

    auto timed_mode(std::size_t size, uint8_t timer = 1) noexcept -> std::size_t {
        if(device_ < 0) return 0U;
#ifdef  _PC_MAX_INPUT
        auto max = fpathconf(device_, _PC_MAX_INPUT);
#else
        auto max = MAX_INPUT;
#endif

        if(size > std::size_t(max))
            size = max;

        size = std::min<std::size_t>(size, 255);
        current_.c_cc[VEOL] = current_.c_cc[VEOL2] = 0;
        current_.c_cc[VMIN] = (uint8_t)size;
        current_.c_cc[VTIME] = timer;
        current_.c_lflag &= ~ICANON;
        tcsetattr(device_, TCSANOW, &current_);
        timed_ = timer;
        return size;
    }

    auto line_mode(const char *nl = "\r\n", uint8_t min = 1) noexcept {
        if(device_ < 0) return std::size_t(0U);

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
        return std::size_t(fpathconf(device_, _PC_MAX_INPUT));
#else
        return std::size_t(MAX_INPUT);
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
        if(device_ < 0) return;
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
        if(device_ < 0) return;
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

        if(s.size() > pos) return false;
        if(device_ < 0) return false;
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

        if(device_ < 0) return false;
        cfsetispeed(&current_, rate);
        cfsetospeed(&current_, rate);
        tcsetattr(device_, TCSANOW, &current_);
        return true;
    }

private:
    int device_{-1};
    uint8_t timed_{0};
    struct termios original_{}, current_{};
    mutable int err_{0};

    void reset() {
        if(device_ < 0) return;
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
        open(path);
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
    saved_(from.saved_), active_(from.active_), timed_(from.timed_), device_(from.device_), err_(from.err_) {
        from.device_ = invalid_;
    }

    ~serial_t() {
        close();
    }

    auto operator!() const {
        return device_ == invalid_;
    }

    explicit operator bool() const {
        return device_ != invalid_;
    }

    auto operator=(serial_t&& from) noexcept -> auto& {
        if(this != &from) {
            close();
            device_ = from.device_;
            timed_ = from.timed_;
            saved_ = from.saved_;
            active_ = from.active_;
            err_ = from.err_;
            from.device_ = invalid_;
            from.err_ = 0;
        }
        return *this;
    }

    auto err() const {
        return err_;
    }

    void open(const std::string& fname) {
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
        err_ = errno;
    }

    void close() {
        if(device_ != invalid_) {
            SetCommTimeouts(device_, &timeouts_);
            SetCommState(device_, &saved_);
            CloseHandle(device_);
            device_ = invalid_;
        }
        err_ = 0;
    }

    auto get(bool echo = false, int echo_code = EOF, int eol = EOF) const {
        if(device_ != invalid_) {
            char buf{0};
            DWORD count{0};
            err_ = 0;
            if(ReadFile(device_, &buf, DWORD(1), &count, nullptr) == FALSE) {
                err_ = errno;
                throw std::system_error(err_, std::generic_category(), "Serial i/o error");
            }
            if(count < 1) return EOF;
            if(echo && echo_code != EOF && (eol == EOF || buf != eol))
                put(echo_code);
            else if(echo)
                put(buf);
            return static_cast<int>(buf);
        }
        return EOF;
    }

    auto get(void *data, std::size_t size, bool echo = false) const {
        if(!data || !size) return std::size_t(0U);
        DWORD count{0};
        err_ = 0;
        if(ReadFile(device_, data, DWORD(size), &count, nullptr) == FALSE) {
            err_ = errno;
            throw std::system_error(err_, std::generic_category(), "Serial i/o error");
        }
        if(count > 0 && echo)
            put(data, count);
        if(count > 0) return std::size_t(count);
        return std::size_t(0U);
    }

    auto put(const void *data, std::size_t size) const -> std::size_t {
        if(!size || device_ == invalid_) return std::size_t(0U);
        DWORD count{0};
        err_ = 0;
        if(WriteFile(device_, data, DWORD(size), &count, nullptr) == FALSE) {
            err_ = errno;
            throw std::system_error(errno, std::generic_category(), "Serial i/o error");
        }
        if(count > 0) return std::size_t(count);
        return std::size_t(0U);
    }

    auto put(int code) const -> bool {
        if(device_ == invalid_) return false;
        if(code == EOF) return true;
        char buf = static_cast<char>(code);
        DWORD count{0};
        err_ = 0;
        if(WriteFile(device_, &buf, 1, &count, nullptr) == FALSE) {
            err_ = errno;
            throw std::system_error(err_, std::generic_category(), "Serial i/o error");
        }
        if(count < 1) return false;
        return true;
    }

    auto put(const std::string_view& msg) const {
        return put(msg.data(), msg.size());
    }

    auto timed_mode(std::size_t size, uint8_t timer = 1) noexcept -> std::size_t {
        if(device_ == invalid_) return std::size_t(0);
        COMMTIMEOUTS timed{};
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
        }
        else {
            timed.ReadIntervalTimeout = MAXDWORD;
            timed.ReadTotalTimeoutConstant = 0;
        }
        SetCommTimeouts(device_, &timed);
        return size;
    }

    auto line_mode(const char *nl = "\r\n", uint8_t min = 1) noexcept { // NOLINT
        return std::size_t(0U);
    }

    void flush() const {
        if(device_ != invalid_)
            PurgeComm(device_, PURGE_TXABORT | PURGE_TXCLEAR);
    }

    void purge() const {
        if(device_ != invalid_)
            PurgeComm(device_, PURGE_RXABORT | PURGE_RXCLEAR);
    }

    void sync() const {
        if(device_ != invalid_)
            FlushFileBuffers(device_);
    }

    void hup() const {
        if(device_ == invalid_) return;
        SetCommBreak(device_);
        Sleep(100L);
        ClearCommBreak(device_);
    }

    void dtr(unsigned msec = 120) {
        if(device_ == invalid_) return;
        EscapeCommFunction(device_, CLRDTR);
        if(msec) {
            Sleep(msec);
            EscapeCommFunction(device_, SETDTR);
        }
    }

    void flow(bool hw = true, bool sw = true) {
        if(device_ == invalid_) return;
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
        if(device_ == invalid_) return false;
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

        if(s.size() > pos) return false;
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
        if(device_ == invalid_) return 0UL;
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
    mutable int err_{0};
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

inline auto getline(const serial_t& sio, char *buf, std::size_t max, int eol = '\n', bool echo = false, int echo_code = EOF, const char *ignore = nullptr) {
    *buf = 0;
    --max;

    auto count = std::size_t(0);
    while(count < max) {
        auto code = sio.get(echo, echo_code, eol);
        if(code == EOF) return count;
        if(ignore && strchr(ignore, code)) continue;
        buf[count++] = static_cast<char>(code);
        if(eol && code == eol) break;
    }
    buf[count] = 0;
    return count;
}

inline auto putline(const serial_t& sio, std::string_view msg, const std::string_view& eol = "\n") {
    auto result = sio.put(msg);
    if(result > 0 && sio.put(eol) == eol.size()) return result;
    return std::size_t(0U);
}

inline auto expect(const serial_t& sio, const std::string_view& match) {
    auto count = 0U;
    while(count < match.size()) {
        auto code = sio.get();
        if(code == EOF) return false;
        // strip lead-in noise...
        if(!count && match[0] != code) continue;
        if(match[count] != code) return false;
        ++count;
    }
    return true;
}

inline auto until(const serial_t& sio, int match = EOF, unsigned max = 1) {
    auto count = 0U;
    while(count < max) {
        auto code = sio.get();
        if(code == EOF) return false;
        if(match == EOF || code == match)
            ++count;
    };
    return true;
}

inline auto csum8(const uint8_t *data, std::size_t size) {
    uint8_t sum = 0;
    while (size--)
        sum += *data++;
    return sum;
}

inline auto crc16(const uint8_t *data, std::size_t size) {
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

inline auto crc32(const uint8_t *data, std::size_t size) {
    static crc32_t table[256]{0};
    crc32_t crc = 0xffffffff;

    for (std::size_t i = 0; i < 256; i++) {
        crc32_t c = i;
        for (std::size_t j = 0; j < 8; j++)
            c = (c & 1) ? 0xedb88320 ^ (c >> 1) : c >> 1;
        table[i] = c;
    }

    for(std::size_t i = 0; i < size; i++)
        crc = table[(crc ^ data[i]) & 0xff] ^ (crc >> 8);

    return crc ^ 0xffffffff;
}
} // end namespace
#endif
