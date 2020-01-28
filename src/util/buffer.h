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
        const LEN len;
        char buf[];

        template<typename NEW_LEN = LEN>
        Segment<NEW_LEN> *sub(LEN offset = 0) {
            return reinterpret_cast<Segment<NEW_LEN> *>(buf + offset);
        }
    };

    struct BufStr {
        uint len;
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

    struct SegmentRef {
        uint offset;
        uint len;
        char *buf;
        SegmentRef *prev;
        SegmentRef *next;
    };
    static thread_local SegmentRef *cache;// thread local

    class Buffer {
    private:
    private:
        const uint capacity;
        uint wi;
        char *buf;

        SegmentRef *head;
        SegmentRef *tail;

    public:
        explicit Buffer(const uint capacity) :
                capacity(capacity), wi(0), head(nullptr), tail(nullptr) {
            buf = static_cast<char *>(malloc(capacity));
        }

        Buffer(Buffer &&buffer) noexcept : capacity(buffer.capacity), buf(buffer.buf), head(buffer.head),
                                           tail(buffer.tail), wi(buffer.wi) {
            buffer.buf = nullptr;
            buffer.head = nullptr;
            buffer.tail = nullptr;
        }

        ~Buffer() {
            if (buf) {
                free(buf);
                buf = nullptr;
            }
            if (head) {
                head->prev = cache;
                cache = tail;
                head = nullptr;
                tail = nullptr;
            }
        }

        uint canWriteSize() {
            return capacity - wi;
        }

        char *readPtr() {
            return tail ? tail->buf + tail->len : buf;
        }

        char *writePtr() {
            return buf + wi;
        }

        uint canReadSize() {
            return static_cast<uint>((ulong) wi - (ulong) readPtr());
        }

        template<typename T>
        T readUint() const {
            return *reinterpret_cast<T *>(tail ? tail->buf + tail->len : buf);
        }

        void addWrited(int w) {
            wi += w;
        }

        template<typename LEN>
        Segment<LEN> *segment(LEN len = 0) {
            return reinterpret_cast<Segment<LEN> *>((tail ? tail->buf + tail->len : buf) - len);
        }
    };
}


#endif //MESHER_BUFFER_H
