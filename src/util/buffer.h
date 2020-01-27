//
// Created by dear on 20-1-26.
//

#ifndef MESHER_BUFFER_H
#define MESHER_BUFFER_H


#include <glob.h>
#include <cstdlib>

namespace sn {

    template<typename LEN>
    struct __attribute__ ((__packed__)) Segment {
        LEN len;
        char buf[];

        template<typename NEW_LEN = LEN>
        Segment<NEW_LEN> *sub(LEN offset = 0) {
            return reinterpret_cast<Segment<NEW_LEN> *>(buf + offset);
        }
    };

    struct BufStr {
        size_t len;
        const char *buf;

        template<typename LEN>
        BufStr(const Segment<LEN> &s) noexcept {
            len = s.len;
            buf = s.buf;
        }
    };

    inline std::ostream &operator<<(std::ostream &os, const sn::BufStr &s) {
        return os << "[" << s.len << "]" << (s.buf, s.len);
    }
}


namespace sn {

    class Buffer {
    private:
        const size_t capacity;
        size_t ri;
        size_t wi;
        char *buf;

    public:
        explicit Buffer(const size_t capacity) :
                capacity(capacity), ri(0), wi(0) {
            buf = static_cast<char *>(malloc(capacity));
        }

        Buffer(Buffer &&buffer) noexcept :
                capacity(buffer.capacity), buf(buffer.buf), ri(buffer.ri), wi(buffer.wi) {
            buffer.buf = nullptr;
        }

        ~Buffer() {
            if (buf != nullptr) {
                free(buf);
            }
        }

        void reset(char *buf, size_t ri, size_t wi) {
            if (buf != this->buf) {
                free(this->buf);
            }
            this->buf = buf;
            this->ri = ri;
            this->wi = ri;
        }

        size_t canWriteSize() {
            return capacity - wi;
        }

        char *readPtr() {
            return buf + ri;
        }

        char *writePtr() {
            return buf + wi;
        }

        size_t canReadSize() {
            return wi - ri;
        }

        template<typename T>
        T readUnint() const {
            return *reinterpret_cast<T *>(buf + ri);
        }

        void addWrited(int w) {
            wi += w;
        }

        void addReaded(int r) {
            ri += r;
        }

        template<typename LEN>
        Segment<LEN> *segment() {
            return reinterpret_cast<Segment<LEN> *>(buf + ri);
        }
    };
}


#endif //MESHER_BUFFER_H
