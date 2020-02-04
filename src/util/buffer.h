//
// Created by dear on 20-1-26.
//

#ifndef MESHER_BUFFER_H
#define MESHER_BUFFER_H


#include <glob.h>
#include <cstdlib>
#include <iostream>

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

#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
//当系统为大端时，把face_code结构体中的以小端模式存储的数据转换为大端
#define SEGMENT_LEN_64(segment) __builtin_bswap64((segment)->len)
#define SEGMENT_LEN_32(segment) __builtin_bswap32((segment)->len)
#define SEGMENT_LEN_16(segment) __builtin_bswap16((segment)->len)
#define SEGMENT_LEN_8(segment) (segment)->len

//================================================
#define WRITELE_VAL_64(ptr, val) *reinterpret_cast<uint64_t *>(ptr)=__builtin_bswap64(val)
#define WRITELE_VAL_32(ptr, val) *reinterpret_cast<uint32_t *>(ptr)=__builtin_bswap32(val)
#define WRITELE_VAL_16(ptr, val) *reinterpret_cast<uint16_t *>(ptr)=__builtin_bswap16(val)
#define WRITELE_VAL_8(ptr, val) *reinterpret_cast<uint8_t *>(ptr)=val

#else //小端模式则什么也不做直接返回
#define SEGMENT_LEN_64(segment) (segment)->len
#define SEGMENT_LEN_32(segment) (segment)->len
#define SEGMENT_LEN_16(segment) (segment)->len
#define SEGMENT_LEN_8(segment) (segment)->len

//================================================

#define WRITELE_VAL_64(ptr, val) *reinterpret_cast<uint64_t *>(ptr)=val
#define WRITELE_VAL_32(ptr, val) *reinterpret_cast<uint32_t *>(ptr)=val
#define WRITELE_VAL_16(ptr, val) *reinterpret_cast<uint16_t *>(ptr)=val
#define WRITELE_VAL_8(ptr, val) *reinterpret_cast<uint8_t *>(ptr)=val
#endif

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
        const uint32_t capacity;
        uint32_t wi;
        char *buf;

        SegmentRef *head;
        SegmentRef *tail;

    public:
        explicit Buffer(const uint32_t capacity) :
                capacity(capacity), wi(0), head(nullptr), tail(nullptr) {
            buf = static_cast<char *>(malloc(capacity));
        }

        Buffer(Buffer &&buffer) noexcept : capacity(buffer.capacity),
                                           buf(buffer.buf), head(buffer.head),
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

        uint32_t canWriteSize() {
            return capacity - wi;
        }

        char *readPtr() {
            return tail ? tail->buf + tail->len : buf;
        }

        char *writePtr() {
            return buf + wi;
        }

        uint32_t canReadSize() {
            return static_cast<uint32_t>( wi - static_cast<uint32_t>(readPtr() - buf));
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
            return reinterpret_cast<Segment<LEN> *>((tail ? tail->buf + tail->len : buf) + len);
        }

    };
}


#endif //MESHER_BUFFER_H
