// Copyright (C) 2023 Tycho Softworks.
// This code is licensed under MIT license.

#ifndef	AUDIOFILE_HPP_
#define	AUDIOFILE_HPP_

#include "endian.hpp"
#include "filesystem.hpp"

namespace tycho {
class audiofile final {
public:
    using offset_t = off_t;
    using handle_t = int;
    static const handle_t invalid = -1;

    inline audiofile() = default;
    inline audiofile(const audiofile& from) = delete;

    inline ~audiofile() {
        close();
    }

    inline auto operator=(const audiofile& from) -> audiofile& = delete;

    inline operator bool() const {
        return stream_ != -1;
    }

    inline auto operator!() const {
        return stream_ == -1;
    }

    inline auto magic() const { // 0x2e736e64
        return be_get32(&header_[0]);
    }

    inline auto offset() const { // at least 28....
        return be_get32(&header_[4]);
    }

    inline auto size() const { // -1 = unknown
        return be_get32(&header_[8]);
    }

    inline auto encoding() const { // 1=ulaw, 27=alaw, 3=16bit pcm
        return be_get32(&header_[12]);
    }

    inline auto sample_rate() const { // 8000...
        return be_get32(&header_[16]);
    }

    inline auto channels() const {  // 1...
        return be_get32(&header_[20]);
    }

    inline auto bytes_per_sample() const -> long {
        switch(encoding()) {
        case 1:
        case 2:
        case 11:
        case 17:
        case 27:
            return channels() * 1L;
        case 3:
        case 12:
        case 18:
        case 19:
        case 20:
            return channels() * 2L;
        case 4:
        case 13:
            return channels() * 3L;
        case 5:
        case 6:
        case 14:
            return channels() * 4L;
        case 7:
            return channels() * 8L;
        }
        return 0L;
    }

    inline auto annotation() const {
        return std::string_view(annotation_);
    }

    inline auto current() const {
        return static_cast<uint32_t>(lseek(stream_, 0L, SEEK_CUR));
    }

    inline auto handle() const {
        return stream_;
    }

    inline auto get(void *buf, size_t size) const {
        return ::read(stream_, buf, size);  // FlawFinder: no
    }

    inline auto put(const void *buf, size_t size) const {
        return ::write(stream_, buf, size);
    }

    inline auto pos() const {
        if(stream_ == invalid)
            return 0U;
        return static_cast<uint32_t>(::lseek(stream_, 0L, SEEK_CUR) - offset());
    }

    inline auto pos(uint32_t position) const {
        if(stream_ == invalid)
            return 0U;
        return static_cast<uint32_t>(::lseek(stream_, position + offset(), SEEK_SET) - offset());
    }

    inline auto rewind() const {
        return static_cast<uint32_t>(lseek(stream_, offset(), SEEK_SET));
    }

    inline auto append() const {
        return static_cast<uint32_t>(lseek(stream_, 0L, SEEK_END));
    }

    auto detach() {
        if(stream_ != -1) {
            if(update_) {
                be_set32(&header_[8], current() - offset());
                ::lseek(stream_, 0L, SEEK_SET);
                ::write(stream_, header_, sizeof(header_) - 4);
            }
            auto h = stream_;
            annotation_ = "";
            update_ = false;
            stream_ = -1;
            return h;
        }
        return invalid;
    }

    void close() {
        if(stream_ != -1) {
            if(update_) {
                be_set32(&header_[8], current() - offset());
                ::lseek(stream_, 0L, SEEK_SET);
                ::write(stream_, header_, sizeof(header_) - 4);
            }
            ::close(stream_);
            annotation_ = "";
            update_ = false;
            stream_ = -1;
        }
    }

    auto make(const char *filename, uint32_t type = 0, uint32_t rate = 8000, uint32_t count = 1, unsigned mode = 0664, std::string_view note = "") {
        close();
        if(!type)
            stream_ = ::open(filename, O_RDWR); // FlawFinder: ignore
        else
            stream_ = ::open(filename, O_RDWR | O_CREAT | O_TRUNC, mode);   // FlawFinder: ignore

        if(stream_ < 0) {
            close();
            return false;
        }

        if(!type && ::read(stream_, header_, 28) < 28) {    // FlawFinder: ignore
            close();
            return false;
        }

        if(type) {
            auto note_size = note.size();
            if(note_size < 3)
                note_size = 4;
            else
                ++note_size;
            while((note_size % 4) != 0)
                ++note_size;

            lseek(stream_, 0L, SEEK_SET);
            be_set32(&header_[0], 0x2e736e64);
            be_set32(&header_[4], 24 + note_size);
            be_set32(&header_[8], -1);
            be_set32(&header_[12], type);
            be_set32(&header_[16], rate);
            be_set32(&header_[20], count);
            be_set32(&header_[24], 0);
            if(::write(stream_, header_, 28) != 28) {
                close();
                return false;
            }
            lseek(stream_, 24, SEEK_SET);
            if(!note.empty())
                ::write(stream_, note.data(), note.size());

            rewind();
        }
        else
            append();

        update_ = true;
        return true;
    }

    auto play(const char *filename) {
        close();
        stream_ = ::open(filename, O_RDONLY);   // FlawFinder: no
        if(stream_ < 0 || ::read(stream_, header_, 24) != 24) { // FlawFinder: no
fail:
            close();
            return false;
        }
        if(magic() != 0x2e736e64 || offset() > 256)
            goto fail;
        char buf[offset() - 23];
        if(::read(stream_, buf, offset() - 24) != static_cast<ssize_t>(offset() - 24)) // FlawFinder: no
            goto fail;
        buf[sizeof(buf) - 1] = 0;
        annotation_ = buf;
        return true;
    }

private:
    uint8_t header_[28]{};
    bool update_{false};
    handle_t stream_{-1};
    std::string annotation_;
};
} // end namespace
#endif
