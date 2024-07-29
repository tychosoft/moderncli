// Copyright (C) 2022 Tycho Softworks.
// This code is licensed under MIT license.

#if !defined(TYCHO_SERIAL_HPP_) && __has_include(<termios.h>)
#define TYCHO_SERIAL_HPP_

#include <thread>
#include <chrono>
#include <string>
#include <string_view>
#include <cctype>
#include <cstring>
#include <csignal>
#include <climits>

#include <fcntl.h>
#include <unistd.h>
#include <poll.h>
#include <termios.h>
#include <sys/ioctl.h>

namespace tycho {
using crc16_t = uint16_t;
using crc32_t = uint32_t;

class bad_serial final : public std::exception {
public:
    bad_serial(const bad_serial&) = delete;
    auto operator=(const bad_serial&) -> auto& = delete;

    auto err() const {
        return err_;
    }

    auto what() const noexcept -> const char * override {
        return "serial i/o error";
    }

private:
    friend class serial_t;

    bad_serial() : err_(errno) {}

    int err_{-1};
};

class drop_serial final : public std::exception {
public:
    drop_serial(const drop_serial&) = delete;
    auto operator=(const drop_serial&) -> auto& = delete;

    auto what() const noexcept -> const char * override {
        return "serial hangup";
    }

private:
    friend class serial_t;

    drop_serial() = default;
};

class serial_t final {
public:
    serial_t() = default;

    explicit serial_t(const std::string& path) {
        open(path); // FlawFinder: safe
    }

    serial_t(const serial_t& from) :
    original_(from.original_), current_(from.current_) {
        if(from.device_ > -1)
            device_ = ::dup(from.device_);
    }

    serial_t(serial_t&& from) noexcept :
    device_(from.device_), original_(from.original_), current_(from.current_) {
        from.device_ = -1;
    }

    ~serial_t() {
        close();
    }

    auto operator!() const {
        return device_ < 0;
    }

    operator bool() const {
        return device_ > -1;
    }

    void open(const std::string& fname) {   // FlawFinder: safe
        close();
        device_ = ::open(fname.c_str(), O_RDWR | O_NDELAY); // FlawFinder: safe
        if(device_ < 0)
            return;

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
    }

    auto wait(int msec = -1) const -> bool {
        if(device_ < 0)
            return false;

        int status{0};
        struct pollfd pfd{};
        pfd.fd = device_;
        pfd.revents = 0;
        pfd.events = POLLIN | POLLHUP;

        status = ::poll(&pfd, 1, msec);
        if(status < 0)
            throw bad_serial();

        if(status < 1)
            return false;

        if(pfd.revents & POLLHUP)
            throw drop_serial();

        if(pfd.revents & POLLERR)
            throw bad_serial();

        return pfd.revents & POLLIN;
    }

    auto pending(int msec = -1) const -> bool {
        if(device_ < 0)
            return false;

        int status{0};
        struct pollfd pfd{};
        pfd.fd = device_;
        pfd.revents = 0;
        pfd.events = POLLOUT | POLLHUP;

        status = ::poll(&pfd, 1, msec);
        if(status < 0)
            throw bad_serial();

        if(status < 1)
            return false;

        if(pfd.revents & POLLHUP)
            throw drop_serial();

        if(pfd.revents & POLLERR)
            throw bad_serial();

        return pfd.revents & POLLOUT;
    }

    auto get(bool echo = false, int echocode = EOF, int eol = EOF) const {
        if(device_ > -1) {
            char buf{0};
            auto result = ::read(device_, &buf, 1); // FlawFinder: ignore
            if(result < 0)
                throw bad_serial();
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

    auto get(char *data, size_t size, bool echo = false) const {
        if(!data || !size)
            return 0U;

        auto count = ::read(device_, data, size);   // FlawFinder: safe
        if(count < 0)
            throw bad_serial();
        if(count > 0 && echo)
            put(data, count);
        if(count > 0)
            return static_cast<unsigned>(count);
        return 0U;
    }

    auto getline(char *buf, size_t max, int eol = '\n', bool echo = false, int echocode = EOF, const char *ignore = nullptr) const {
        *buf = 0;
        --max;

        auto count{0U};
        while(count < max) {
            auto code = get(echo, echocode, eol);
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

    auto expect(const std::string_view& match) const {
        auto count = 0U;
        while(count < match.size()) {
            auto code = get();
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

    auto until(int match = EOF, unsigned max = 1) const {
        auto count = 0U;
        while(count < max) {
            auto code = get();
            if(code == EOF)
                return false;
            if(match == EOF || code == match)
                ++count;
        };
        return true;
    }

    auto put(int code, bool echo = false, int echocode = EOF) const -> bool {
        if(device_ < 0)
            return false;

        if(code == EOF)
            return true;

        char buf = static_cast<char>(code);
        auto result = ::write(device_, &buf, 1);
        if(result < 0)
            throw bad_serial();
        if(result < 1)
            return false;
        if(echo)
            return until(echocode);
        return true;
    }

    auto put(const char *data, size_t size, bool echo = false, int echocode = EOF) const -> unsigned {
        if(!size || device_ < 0)
            return 0U;

        auto count = ::write(device_, data, size);
        if(count > 0) {
            if(echo && !until(echocode, count))
                return 0U;
            return static_cast<unsigned>(count);
        }
        if(count < 0)
            throw bad_serial();
        return 0U;
    }

    auto put(const std::string_view msg, bool echo = false, int echomode = EOF) const {
        return put(msg.data(), msg.size(), echo, echomode);
    }

    auto putline(std::string_view msg, const std::string_view& eol = "\n", bool echo = false, int echocode = EOF) const {
        auto result = put(msg, echo, echocode);
        if(put(eol, echo) == eol.size())
            return result;
        return 0U;
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

        current_.c_cc[VEOL] = current_.c_cc[VEOL2] = 0;
        current_.c_cc[VMIN] = (uint8_t)size;
        current_.c_cc[VTIME] = timer;
        current_.c_lflag &= ~ICANON;
        tcsetattr(device_, TCSANOW, &current_);
        bufsize_ = max;
        timed_ = timer;
        return size;
    }

    void line_mode(const char *nl = "\r\n", uint8_t min = 1) noexcept {
        if(device_ < 0)
            return;

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
#ifdef  _PC_MAX_CANON
        bufsize_ = fpathconf(device_, _PC_MAX_CANON);
#else
        bufsize_ = MAX_CANON
#endif
    }

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
        bufsize_ = 0;
        timed_ = 0;
    }

    auto size() const {
        return bufsize_;
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

    auto format(const char *s) {
        unsigned bits = 8;
        unsigned stop = 1;
        unsigned parity = 'n';

        if(s && *s && isdigit(*s))
            bits = *(s++) - '0';

        if(s && *s && !isdigit(*s))
            parity = static_cast<unsigned>(tolower(*(s++)));

        if(s && *s && isdigit(*s))
            stop = *(s++) - '0';

        if(stop < 1 || stop > 2 || bits < 5 || bits > 8)
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
            current_.c_cflag |= (PARENB | PARODD);
            break;
        case 'e':
            current_.c_cflag |= PARENB;
            break;
        case 'n':
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
    size_t bufsize_{0};
    uint8_t timed_{0};
    struct termios original_{}, current_{};
};

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
} // end namespace
#endif
